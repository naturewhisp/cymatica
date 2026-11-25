#include "release_null.h"

#include "acao02_calibration.h"
#include "audio_engine.h"
#include "audio_probe.h"
#include "audio_scheduler.h"
#include "discontinuity.h"
#include "loudness.h"
#include "music_planner.h"
#include "onset.h"
#include "release_ab.h"
#include "stem_analysis.h"
#include "stft.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <system_error>
#include <utility>

namespace cymatica::cacp {

using nlohmann::json;

std::vector<bool> windowMask(std::span<const std::uint64_t> starts, std::uint64_t length, std::size_t frames) {
    std::vector<bool> m(frames, false);
    for (const auto s : starts) {
        for (std::uint64_t f = s; f < s + length && f < frames; ++f) m[static_cast<std::size_t>(f)] = true;
    }
    return m;
}

ResidualEnergy residualEnergy(std::span<const float> a, std::span<const float> b, const std::vector<bool>& mask) {
    ResidualEnergy e;
    const std::size_t frames = std::min({a.size() / 2, b.size() / 2, mask.size()});
    for (std::size_t f = 0; f < frames; ++f) {
        for (std::size_t c = 0; c < 2; ++c) {
            const double x = a[2 * f + c];
            const double d = static_cast<double>(b[2 * f + c]) - x;
            e.reference += x * x;
            e.total += d * d;
            if (mask[f]) e.inside += d * d;
            else e.maxAbsOutside = std::max(e.maxAbsOutside, std::abs(d));
        }
    }
    return e;
}

double medianDb(const std::vector<double>& v) { return percentileLinear(v, 50.0); } // percentileLinear takes q in [0, 100]

std::vector<double> highpass4k(std::span<const float> x) {
    std::vector<double> y(x.begin(), x.end());
    constexpr double kPi = 3.14159265358979323846;
    const double w = 2.0 * kPi * 4000.0 / 48000.0;
    for (const double q : {0.541196100146197, 1.306562964876377}) { // 4th-order Butterworth sections
        const double al = std::sin(w) / (2.0 * q), c = std::cos(w), a0 = 1.0 + al;
        const double b0 = (1.0 + c) / 2.0 / a0, b1 = -(1.0 + c) / a0, a1 = -2.0 * c / a0, a2 = (1.0 - al) / a0;
        for (std::size_t ch = 0; ch < 2; ++ch) {
            double s1 = 0.0, s2 = 0.0;
            for (std::size_t i = ch; i < y.size(); i += 2) {
                const double in = y[i], out = b0 * in + s1;
                s1 = b1 * in - a1 * out + s2;
                s2 = b0 * in - a2 * out;
                y[i] = out;
            }
        }
    }
    return y;
}

std::vector<double> cutRatioDb(std::span<const double> residual, std::span<const double> mix, std::span<const std::uint64_t> cuts,
                               std::size_t frames) {
    std::vector<double> out;
    const std::size_t n = std::min(residual.size(), mix.size()) / 2;
    for (const auto cut : cuts) {
        double r = 0.0, m = 0.0;
        for (std::size_t f = static_cast<std::size_t>(cut); f < cut + frames && f < n; ++f) {
            for (std::size_t c = 0; c < 2; ++c) {
                r += residual[2 * f + c] * residual[2 * f + c];
                m += mix[2 * f + c] * mix[2 * f + c];
            }
        }
        out.push_back(r <= 0.0 ? -1000.0 : (m <= 0.0 ? 1000.0 : 10.0 * std::log10(r / m)));
    }
    return out;
}

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::uint32_t kTick = 400;
constexpr std::size_t kCapacity = 9'000'000;
constexpr std::array<double, 4> kReleasesMs{15.0, 30.0, 50.0, 80.0};
constexpr std::size_t kCutWindow = 144; // 3 ms
constexpr double kFlagDb = -10.0;

double db(double num, double den) { return num <= 0.0 || den <= 0.0 ? -1000.0 : 10.0 * std::log10(num / den); }

struct Fixture {
    std::array<std::vector<float>, kRoleCount> stems{};
    std::vector<float> mix{};
    std::vector<AppliedEventRecord> applied{};
    std::vector<audio::MusicIntentEvent> intents{};
    std::vector<audio::PlannedNoteOff> noteOffs{};
    audio::VoiceConfounders conf{};
    std::string invalid{};
    std::string golden{};
    double limiterDb{0.0};
    std::uint64_t contentEnd{0};
};

Fixture renderFixture(StemCapture& cap, double ms, const std::string& dir) {
    Fixture o;
    cap.reset();
    std::uint64_t overflow = 0;
    audio::ProbeAnalysisCapture pc{};
    pc.stems = cap.sink();
    pc.applied = &o.applied;
    pc.appliedOverflow = &overflow;
    pc.intents = &o.intents;
    pc.noteOffs = &o.noteOffs;
    pc.confounders = &o.conf;
    pc.noteReleaseFrames = releaseOverrideFrames(ms);
    const auto p = audio::runFixtureProbeV1Offline(dir, false, audio::FixtureInjectedFault::None, &pc);
    o.invalid = captureInvalidReason(CaptureCheck{true, p.passed, p.failureReason, static_cast<std::size_t>(p.requestedSampleFrames),
                                                  cap.frames(), cap.overflowFrames(), overflow});
    o.golden = p.goldenPcmStatus;
    o.limiterDb = p.maxGainReductionDb;
    o.contentEnd = p.contentEndFrame;
    for (std::size_t r = 0; r < kRoleCount; ++r) o.stems[r].assign(cap.stem(r).begin(), cap.stem(r).end());
    o.mix.assign(cap.busMix().begin(), cap.busMix().end());
    return o;
}

struct PlanRender {
    std::vector<float> stem{};
    std::vector<AppliedEventRecord> applied{};
    audio::VoiceConfounders conf{};
    std::string invalid{};
};

// Plan events through a plain offline engine (no Stop, one epoch), one role stem kept
PlanRender renderPlan(const std::vector<audio::MusicIntentEvent>& ev, const std::vector<audio::PlannedNoteOff>& offs, StemCapture& cap,
                      double ms, std::size_t role, std::uint64_t frames) {
    PlanRender o;
    cap.reset();
    audio::AudioEngine engine;
    if (!engine.initOffline({kFs, 2, 220.0f})) {
        o.invalid = "offline engine initialisation failed";
        return o;
    }
    AppliedEventLog log(audio::kEventChainLogCapacity);
    (void)engine.setAppliedEventLog(&log);
    (void)engine.setRoleStemSink(cap.sink());
    if (releaseOverrideFrames(ms) != 0 && !engine.setNoteReleaseFrames(releaseOverrideFrames(ms))) o.invalid = "release override refused";
    audio::AudioScheduler scheduler({.lookaheadFrames = 7200, .sampleRate = kFs});
    engine.sendCommand(audio::AudioCommand{.commandId = 1, .type = audio::AudioCommandType::Start, .targetSampleFrame = 0});
    std::set<std::uint64_t> ids;
    for (auto e : ev) {
        e.transportEpoch = 1;
        scheduler.scheduleIntent(e, 1);
        ids.insert(e.eventId);
    }
    for (const auto& n : offs) {
        if (ids.contains(n.eventId)) scheduler.scheduleNoteOff(n.eventId, n.role, n.sampleFrame, 1);
    }
    std::vector<float> block(kTick * 2);
    for (std::uint64_t t = 0; t * kTick < frames; ++t) {
        (void)scheduler.dispatchDueEvents(t * kTick, engine);
        engine.processBlock(block.data(), kTick);
        audio::AudioCommandAck ack{};
        while (engine.pollAck(ack)) {}
    }
    (void)engine.setRoleStemSink(nullptr);
    (void)engine.setAppliedEventLog(nullptr);
    o.conf = audio::voiceConfoundersOf(engine.voiceAllocator());
    engine.shutdown();
    o.applied.assign(log.records().begin(), log.records().end());
    if (cap.overflowFrames() != 0 || log.overflow() != 0 || scheduler.backpressureDrops() != 0) o.invalid += "capture or scheduler overflow";
    o.stem.assign(cap.stem(role).begin(), cap.stem(role).end());
    return o;
}

// Applied NoteOff frames of a role, split by coincidence with a NoteOn of the same role
struct Offs {
    std::vector<std::uint64_t> all{}, free{}, coincident{}; // coincident: one entry per frame
    std::size_t coincidentNotes{0};
};
Offs roleOffs(const std::vector<AppliedEventRecord>& applied, std::size_t role) {
    const auto is = [&](const AppliedEventRecord& r, audio::AudioEventType t) {
        return static_cast<std::size_t>(r.id.role) == role && r.id.type == t && r.disposition == EventDisposition::Applied;
    };
    std::set<std::uint64_t> ons, co;
    for (const auto& r : applied) {
        if (is(r, audio::AudioEventType::NoteOn)) ons.insert(r.consumedFrame);
    }
    Offs o;
    for (const auto& r : applied) {
        if (!is(r, audio::AudioEventType::NoteOff)) continue;
        o.all.push_back(r.consumedFrame);
        if (ons.contains(r.consumedFrame)) {
            ++o.coincidentNotes;
            co.insert(r.consumedFrame);
        } else {
            o.free.push_back(r.consumedFrame);
        }
    }
    o.coincident.assign(co.begin(), co.end());
    return o;
}

bool sameEvents(const std::vector<AppliedEventRecord>& a, const std::vector<AppliedEventRecord>& b) {
    return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](const auto& x, const auto& y) {
        return x.id.type == y.id.type && x.id.eventId == y.id.eventId && x.id.role == y.id.role && x.consumedFrame == y.consumedFrame &&
               x.disposition == y.disposition;
    });
}

json summaryOf(const std::vector<double>& v) {
    if (v.empty()) return nullptr;
    return {{"median_db", medianDb(v)}, {"max_db", *std::max_element(v.begin(), v.end())}};
}

// Package 1 labelled calibration on the 2 ms fixture, intrinsic latency from synthetic set A
std::array<std::optional<RoleOnsetProfile>, kRoleCount> calibrate(const Fixture& base, StemCapture& cap) {
    std::array<std::optional<RoleOnsetProfile>, kRoleCount> out{};
    for (std::size_t role = 0; role < kRoleCount; ++role) {
        const auto mr = static_cast<audio::MusicRole>(role);
        const auto syn = renderSyntheticWithRelease(mr, syntheticNotesSetA(role), cap, 0);
        if (!captureInvalidReason(syn.check).empty()) continue;
        const auto intrinsic = evaluateRoleOnsets(cap.stem(role), roleReferenceOnsetsS(syn.applied, mr, kFs), RoleOnsetProfile{0.10, 0.0},
                                                  kFs, roleReleaseTimesS(syn.applied, mr, kFs));
        if (intrinsic.status != AnalysisStatus::Ok || !std::isfinite(intrinsic.errors.medianS)) continue; // no profile: INCONCLUSIVE
        const OffsetWindow window{intrinsic.errors.medianS - kLabeledOffsetMarginS, intrinsic.errors.medianS + kLabeledOffsetMarginS};
        const auto refs = roleReferenceOnsetsS(base.applied, mr, kFs);
        const auto rel = roleReleaseTimesS(base.applied, mr, kFs);
        const NoveltyCurve nov = spectralFluxNovelty(stftMagnitude(monoFromStereo(base.stems[role])), OnsetParams{}.logGamma);
        if (nov.status != AnalysisStatus::Ok) continue;
        std::vector<LabeledCalibrationPoint> grid;
        for (const double delta : kCalibrationDeltas) {
            OnsetParams p{};
            p.delta = delta;
            std::vector<double> est;
            const OnsetFrames fr = pickOnsetFrames(nov.values, p, kDescriptorsV1Stft, kFs);
            for (const auto f : fr.frames) est.push_back(frameCentreSeconds(f, kDescriptorsV1Stft, kFs));
            grid.push_back(labeledCalibrationPoint(refs, rel, est, delta, window));
        }
        if (const auto c = chooseLabeledPoint(grid)) out[role] = RoleOnsetProfile{c->delta, c->offsetS};
    }
    return out;
}

json onsetJson(const Fixture& fx, std::size_t role, const RoleOnsetProfile& p) {
    const auto mr = static_cast<audio::MusicRole>(role);
    const auto ev = evaluateRoleOnsets(fx.stems[role], roleReferenceOnsetsS(fx.applied, mr, kFs), p, kFs, roleReleaseTimesS(fx.applied, mr, kFs));
    if (ev.status != AnalysisStatus::Ok) return {{"status", analysisStatusName(ev.status)}};
    return {{"status", "MEASURED"},           {"references", ev.references},
            {"detections", ev.detections},    {"matched_50ms", ev.at50.matches},
            {"release_detections", ev.releaseDetections}, {"recall_50", ev.at50.recall}};
}

std::map<std::uint64_t, int> slotsOf(const std::vector<AppliedEventRecord>& applied, audio::MusicRole role) {
    std::map<std::uint64_t, int> m;
    for (const auto& r : applied) {
        if (r.id.role == role && r.id.type == audio::AudioEventType::NoteOn) m[r.id.eventId] = r.slot;
    }
    return m;
}

} // namespace

ReleaseNullResult runReleaseNullV1(const std::string& outputDir, bool writeFiles) {
    ReleaseNullResult res;
    json out{{"scenario", "release_null_v1"}, {"preregistration", "docs/research/M2_5_H03_Release_Null_Test_Preregistration.md"}};
    json reasons = json::array();
    json controls = json::object();
    bool controlsOk = true;
    const auto control = [&](const char* id, bool ok, json detail) {
        controls[id] = {{"passed", ok}, {"detail", std::move(detail)}};
        controlsOk = controlsOk && ok;
    };
    StemCapture cap(kCapacity);
    const Fixture base = renderFixture(cap, 2.0, outputDir);
    if (!base.invalid.empty()) reasons.push_back("2 ms: " + base.invalid);
    const std::size_t frames = base.mix.size() / 2;
    const std::size_t content = static_cast<std::size_t>(std::min<std::uint64_t>(base.contentEnd, frames));

    std::array<Offs, kRoleCount> offs;
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        offs[r] = roleOffs(base.applied, r);
        out["note_offs"][kRoleNames[r]] = {
            {"all", offs[r].all.size()}, {"coincident_with_note_on", offs[r].coincidentNotes}, {"free", offs[r].free.size()}};
    }

    { // C1 and C2
        const Fixture again = renderFixture(cap, 2.0, outputDir);
        bool same = again.mix == base.mix;
        for (std::size_t r = 0; r < kRoleCount; ++r) same = same && again.stems[r] == base.stems[r];
        control("C1", same, "repeat of the 2 ms fixture, every stem and the bus mix");
        std::vector<float> scaled = base.stems[3];
        const float g = static_cast<float>(std::pow(10.0, 1.0 / 20.0));
        for (auto& x : scaled) x *= g;
        const auto e = residualEnergy(base.stems[3], scaled, std::vector<bool>(frames, false));
        const double lvl = db(e.total, e.reference);
        control("C2", std::abs(lvl + 18.27) <= 0.05, lvl);
    }

    const auto cal = calibrate(base, cap);
    const auto frozen = frozenStemProfileV1();
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        out["onset_profile"][kRoleNames[r]] =
            cal[r] ? json{{"delta", cal[r]->delta}, {"offset_ms", cal[r]->offsetS * 1000.0}} : json("INCONCLUSIVE");
    }

    const auto lufs = [](const std::vector<float>& x) { return measureLoudnessStereo(x, kFs).integratedLufs; };
    const auto delta = [](const std::optional<double>& a, const std::optional<double>& b) { return a && b ? json(*a - *b) : json(nullptr); };
    std::array<std::optional<double>, kRoleCount> baseLufs;
    for (std::size_t r = 0; r < kRoleCount; ++r) baseLufs[r] = lufs(base.stems[r]);
    const auto baseMixLufs = lufs(base.mix);

    const auto onsets = [&](const Fixture& fx, std::size_t r) {
        json j = json::object();
        j["recalibrated"] = cal[r] ? onsetJson(fx, r, *cal[r]) : json{{"status", "INCONCLUSIVE"}};
        if (frozen) j["frozen_comparison_only"] = onsetJson(fx, r, frozen->roles[r]);
        return j;
    };
    for (std::size_t r = 0; r < kRoleCount; ++r) out["baseline_2ms"]["onsets"][kRoleNames[r]] = onsets(base, r);
    out["baseline_2ms"]["limiter_db"] = base.limiterDb;
    out["baseline_2ms"]["release_cuts"] = base.conf.roleReleaseCuts;
    out["baseline_2ms"]["voice_drops"] = base.conf.roleVoiceDrops;

    // Part A
    bool eventsSame = true;
    for (const double ms : kReleasesMs) {
        const Fixture fx = renderFixture(cap, ms, outputDir);
        if (!fx.invalid.empty()) reasons.push_back(std::to_string(ms) + " ms: " + fx.invalid);
        eventsSame = eventsSame && sameEvents(base.applied, fx.applied);
        json row{{"release_ms", ms},
                 {"limiter_db", fx.limiterDb},
                 {"release_cuts", fx.conf.roleReleaseCuts},
                 {"voice_drops", fx.conf.roleVoiceDrops},
                 {"mix_delta_lu", delta(lufs(fx.mix), baseMixLufs)}};
        std::array<double, kRoleCount> total{};
        double sum = 0.0;
        for (std::size_t r = 0; r < kRoleCount; ++r) {
            const auto len = static_cast<std::uint64_t>((ms + (r == 2 ? 120.0 : 5.0)) * 48.0);
            const auto mask = windowMask(r == 1 ? offs[r].free : offs[r].all, len, frames);
            const auto e = residualEnergy(base.stems[r], fx.stems[r], mask);
            total[r] = e.total;
            sum += e.total;
            std::vector<float> d(fx.stems[r].size());
            for (std::size_t i = 0; i < d.size(); ++i) d[i] = fx.stems[r][i] - base.stems[r][i];
            row["roles"][kRoleNames[r]] = {
                {"residual_rel_stem_db", db(e.total, e.reference)},
                {"residual_exact_zero", e.total == 0.0},
                {"residual_rel_mix_lu", delta(lufs(d), baseMixLufs)},
                {"energy_inside_windows", e.total > 0.0 ? json(e.inside / e.total) : json(nullptr)},
                {"window_coverage", static_cast<double>(std::count(mask.begin(), mask.end(), true)) / static_cast<double>(frames)},
                {"stem_delta_lu", delta(lufs(fx.stems[r]), baseLufs[r])},
                {"a2_candidates_descriptive", differentialCandidates(base.stems[r], fx.stems[r]).size()},
                {"onsets", onsets(fx, r)}};
        }
        for (std::size_t r = 0; r < kRoleCount; ++r) {
            row["roles"][kRoleNames[r]]["share_of_residual_energy"] = sum > 0.0 ? json(total[r] / sum) : json(nullptr);
        }
        const auto em = residualEnergy(base.mix, fx.mix, std::vector<bool>(frames, false));
        row["mix_residual_rel_mix_db"] = db(em.total, em.reference);
        out["part_a"].push_back(row);
    }
    control("C4", eventsSame && base.golden == "PASS", {{"same_events", eventsSame}, {"golden_pcm", base.golden}});

    { // C3: one Vettore NoteOff 10 ms early, slot 7, nearest to the middle of the piece
        const auto slots = slotsOf(base.applied, audio::MusicRole::Vettore);
        std::uint64_t id = 0, off = 0, best = ~0ULL;
        const std::uint64_t mid = content / 2;
        for (const auto& e : base.intents) {
            const auto it = slots.find(e.eventId);
            if (e.role != audio::MusicRole::Vettore || e.durationFrames <= 4800 || it == slots.end() || it->second != 7) continue;
            const std::uint64_t d = e.sampleFrame > mid ? e.sampleFrame - mid : mid - e.sampleFrame;
            if (d < best) {
                best = d;
                id = e.eventId;
                off = e.sampleFrame + e.durationFrames;
            }
        }
        auto edited = base.intents;
        for (auto& e : edited) {
            if (e.eventId == id && e.role == audio::MusicRole::Vettore) e.durationFrames -= 480;
        }
        const auto a = renderPlan(base.intents, base.noteOffs, cap, 2.0, 3, content);
        const auto b = renderPlan(edited, base.noteOffs, cap, 2.0, 3, content);
        const bool sameSlots = slotsOf(a.applied, audio::MusicRole::Vettore) == slotsOf(b.applied, audio::MusicRole::Vettore);
        const std::array<std::uint64_t, 1> start{off - 480};
        const auto e = residualEnergy(a.stem, b.stem, windowMask(start, 1440, a.stem.size() / 2));
        const double inside = e.total > 0.0 ? e.inside / e.total : 0.0;
        control("C3", best != ~0ULL && a.invalid.empty() && b.invalid.empty() && sameSlots && inside >= 0.999,
                {{"event_id", id}, {"note_off", off}, {"energy_inside", inside}, {"same_slots", sameSlots}});
    }

    // Part B
    std::map<std::uint64_t, std::size_t> dyad; // Corpo onset frame -> dyad index
    for (const auto& e : base.intents) {
        if (e.role == audio::MusicRole::Corpo) dyad.emplace(e.sampleFrame, 0);
    }
    std::size_t next = 0;
    for (auto& d : dyad) d.second = next++;
    std::array<std::vector<audio::MusicIntentEvent>, 2> half;
    for (const auto& e : base.intents) {
        if (e.role == audio::MusicRole::Pulso) {
            half[0].push_back(e);
            half[1].push_back(e);
        } else if (e.role == audio::MusicRole::Corpo) {
            half[dyad.at(e.sampleFrame) % 2].push_back(e);
        }
    }
    const std::vector<std::uint64_t>& cuts = offs[1].coincident;
    std::map<std::uint64_t, std::size_t> notesAt; // cut notes per cut frame
    for (const auto f : offs[1].all) ++notesAt[f];
    const auto flaggedNotes = [&](const std::vector<double>& m) {
        std::size_t n = 0;
        for (std::size_t i = 0; i < cuts.size(); ++i) {
            if (m[i] >= kFlagDb) n += notesAt[cuts[i]];
        }
        return n;
    };
    out["part_b"] = {{"cut_frames", cuts.size()}, {"cut_notes", offs[1].coincidentNotes}, {"dyads", dyad.size()}, {"rows", json::array()}};
    const auto end = static_cast<std::ptrdiff_t>(2 * content);
    const std::vector<float> prod(base.stems[1].begin(), base.stems[1].begin() + end);
    const std::vector<float> mix(base.mix.begin(), base.mix.begin() + end);
    const auto mixHi = highpass4k(mix);
    const std::vector<double> mixFull(mix.begin(), mix.end());
    const auto prodLufs = lufs(prod);
    std::vector<float> b0;
    bool c6 = true;
    json c6d = json::array();
    for (const double ms : {2.0, 15.0, 30.0, 50.0, 80.0}) {
        std::vector<float> s(2 * content, 0.0f);
        for (const auto& h : half) {
            const auto r = renderPlan(h, base.noteOffs, cap, ms, 1, content);
            if (!r.invalid.empty()) reasons.push_back("part B: " + r.invalid);
            c6 = c6 && r.conf.roleReleaseCuts[1] == 0 && r.conf.roleVoiceDrops[1] == 0;
            c6d.push_back({{"release_ms", ms}, {"cuts", r.conf.roleReleaseCuts[1]}, {"drops", r.conf.roleVoiceDrops[1]}});
            for (std::size_t i = 0; i < s.size() && i < r.stem.size(); ++i) s[i] += r.stem[i];
        }
        const bool isB0 = ms == 2.0;
        if (isB0) b0 = s;
        const auto len = isB0 ? std::uint64_t{96} : static_cast<std::uint64_t>((ms + 5.0) * 48.0);
        const auto e = residualEnergy(isB0 ? prod : b0, s, windowMask(isB0 ? cuts : offs[1].all, len, content));
        if (isB0) control("C5", e.maxAbsOutside <= 1e-6, e.maxAbsOutside);
        std::vector<float> d(s.size()); // production - B(ms)
        for (std::size_t i = 0; i < d.size(); ++i) d[i] = prod[i] - s[i];
        const auto hi = cutRatioDb(highpass4k(d), mixHi, cuts, kCutWindow);
        const std::vector<double> dFull(d.begin(), d.end());
        json mHi = summaryOf(hi);
        mHi["flagged_notes"] = flaggedNotes(hi);
        mHi["flagged_frames"] = std::count_if(hi.begin(), hi.end(), [](double x) { return x >= kFlagDb; });
        mHi["flagged_list"] = json::array();
        for (std::size_t i = 0; i < cuts.size(); ++i) {
            if (hi[i] >= kFlagDb) mHi["flagged_list"].push_back({{"frame", cuts[i]}, {"db", hi[i]}});
        }
        out["part_b"]["rows"].push_back({{"release_ms", ms},
                                         {"residual_against", isB0 ? "production" : "B0"},
                                         {"residual_rel_stem_db", db(e.total, e.reference)},
                                         {"energy_inside_windows", e.total > 0.0 ? json(e.inside / e.total) : json(nullptr)},
                                         {"stem_delta_lu_vs_production", delta(lufs(s), prodLufs)},
                                         {"m_hi_vs_production", mHi},
                                         {"m_full_descriptive", summaryOf(cutRatioDb(dFull, mixFull, cuts, kCutWindow))}});
    }
    control("C6", c6, c6d);

    { // C7: one-sample impulses at every cut
        const auto flaggedAt = [&](double amplitude) {
            std::vector<float> x(2 * content, 0.0f);
            for (const auto f : cuts) {
                if (f >= content) continue;
                x[2 * static_cast<std::size_t>(f)] = static_cast<float>(amplitude);
                x[2 * static_cast<std::size_t>(f) + 1] = static_cast<float>(amplitude);
            }
            return flaggedNotes(cutRatioDb(highpass4k(x), mixHi, cuts, kCutWindow));
        };
        bool ok = flaggedAt(0.0) == 0;
        std::size_t last = 0;
        json d = json::array();
        for (const double a : {-60.0, -40.0, -20.0, -10.0}) {
            const std::size_t n = flaggedAt(std::pow(10.0, a / 20.0));
            ok = ok && n >= last;
            last = n;
            d.push_back({{"dbfs", a}, {"flagged_notes", n}});
        }
        control("C7", ok && last == offs[1].coincidentNotes, d);
    }

    out["controls"] = controls;
    out["reasons"] = reasons;
    res.status = !reasons.empty() ? "INCONCLUSIVE" : (controlsOk ? "MEASURED" : "TOOL_NOT_VALIDATED");
    out["status"] = res.status;
    res.json = out.dump(2) + "\n";
    if (writeFiles) {
        std::error_code ec; // a failure shows up as manifestWriteSuccess false
        std::filesystem::create_directories(outputDir, ec);
        std::ofstream f(std::filesystem::path(outputDir) / "release_null_v1_manifest.json", std::ios::binary);
        f << res.json;
        f.close();
        res.manifestWriteSuccess = f.good();
    }
    return res;
}

} // namespace cymatica::cacp
