#include "release_ab.h"

#include "audio_engine.h"
#include "audio_probe.h"
#include "audio_scheduler.h"
#include "evaluation_pack.h"
#include "music_planner.h"
#include "onset.h"
#include "stft.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::size_t kFixtureCaptureFrames = 8'900'000; // > 8,825,200 frames of fixture_probe_v1
constexpr double kToleranceS = 0.050;
constexpr double kClipFadeS = 0.010;

// 2 ms is the production release: no override at all, so the baseline is the shipped behaviour
std::uint32_t releaseFramesOf(double ms) {
    return ms == 2.0 ? 0U : static_cast<std::uint32_t>(std::llround(ms * kFs / 1000.0));
}

std::string msLabel(double ms) {
    std::ostringstream s;
    s << std::setw(2) << std::setfill('0') << static_cast<int>(ms) << "ms";
    return s.str();
}

json optNum(const std::optional<double>& v) { return (v && std::isfinite(*v)) ? json(*v) : json(nullptr); }

// FNV-1a over the identity and disposition of every record consumed by the engine
std::uint64_t eventsHash(const std::vector<AppliedEventRecord>& applied, std::uint64_t h = 1469598103934665603ULL) {
    auto mix = [&h](std::uint64_t v) {
        for (int i = 0; i < 8; ++i) {
            h ^= (v >> (8 * i)) & 0xFFU;
            h *= 1099511628211ULL;
        }
    };
    for (const auto& r : applied) {
        mix(static_cast<std::uint64_t>(r.id.type));
        mix(r.id.eventId);
        mix(static_cast<std::uint64_t>(r.id.role));
        mix(r.id.sampleFrame);
        mix(r.id.transportEpoch);
        mix(static_cast<std::uint64_t>(r.disposition));
    }
    return h;
}

ReleaseRoleMeasure measureRole(std::span<const float> stem, const std::vector<AppliedEventRecord>& applied, std::size_t role,
                               const RoleOnsetProfile& profile) {
    ReleaseRoleMeasure m;
    const auto mr = static_cast<audio::MusicRole>(role);
    const auto refs = roleReferenceOnsetsS(applied, mr, kFs);
    const auto rel = roleReleaseTimesS(applied, mr, kFs);
    const RoleOnsetEvaluation ev = evaluateRoleOnsets(stem, refs, profile, kFs, rel);
    m.references = ev.references;
    m.detections = ev.detections;
    if (ev.status != AnalysisStatus::Ok) {
        m.status = analysisStatusName(ev.status);
    } else if (ev.references == 0) {
        m.status = "NO_REFERENCES";
    } else {
        m.status = "MEASURED";
        m.matched = ev.at50.matches;
        m.releaseDetections = ev.releaseDetections;
        m.precision50 = ev.at50.precision;
        m.recall50 = ev.at50.recall;
        m.precisionExclReleases = ev.precisionExclReleases;
    }
    return m;
}

struct SynthRender {
    CaptureCheck check{};
    std::vector<AppliedEventRecord> applied{};
    audio::VoiceConfounders confounders{};
};

// Isolated notes of one role through the real engine (offline), with the release override and the confounders
SynthRender renderSynthetic(audio::MusicRole role, const std::vector<SynthNote>& notes, StemCapture& cap, std::uint32_t releaseFrames) {
    SynthRender out;
    cap.reset();
    audio::AudioEngine engine;
    if (!engine.initOffline({kFs, 2, 220.0f})) {
        out.check.probeFailure = "offline engine initialisation failed";
        return out;
    }
    AppliedEventLog log(audio::kEventChainLogCapacity);
    (void)engine.setAppliedEventLog(&log);
    (void)engine.setRoleStemSink(cap.sink());
    (void)engine.setNoteReleaseFrames(releaseFrames);
    audio::AudioScheduler scheduler({.lookaheadFrames = 7200, .sampleRate = kFs});
    engine.sendCommand(audio::AudioCommand{.commandId = 1, .type = audio::AudioCommandType::Start, .targetSampleFrame = 0});
    std::uint64_t id = 1;
    double endS = 0.0;
    for (const auto& n : notes) {
        const audio::MusicIntentEvent e{.eventId = id++,
                                        .sampleFrame = static_cast<std::uint64_t>(std::llround(n.timeS * kFs)),
                                        .tick = 0,
                                        .transportEpoch = 1,
                                        .role = role,
                                        .pitchHz = n.pitchHz,
                                        .velocity = n.velocity,
                                        .durationFrames = static_cast<std::uint32_t>(std::llround(n.durationS * kFs)),
                                        .timbreParam = n.timbre};
        scheduler.scheduleIntent(e, 1);
        endS = (std::max)(endS, n.timeS + n.durationS);
    }
    constexpr std::uint32_t kTick = 400;
    const auto ticks = static_cast<std::uint64_t>(std::ceil((endS + 1.0) * kFs / kTick)); // 1 s covers the 80 ms release
    std::vector<float> block(kTick * 2);
    for (std::uint64_t t = 0; t < ticks; ++t) {
        (void)scheduler.dispatchDueEvents(t * kTick, engine);
        engine.processBlock(block.data(), kTick);
        audio::AudioCommandAck ack{};
        while (engine.pollAck(ack)) {}
    }
    (void)engine.setRoleStemSink(nullptr);
    (void)engine.setAppliedEventLog(nullptr);
    const auto& va = engine.voiceAllocator();
    out.confounders = {va.voiceDropCount(), va.releaseCutCount(), va.releaseOverlapCount()};
    out.applied.assign(log.records().begin(), log.records().end());
    engine.shutdown();
    std::size_t appliedNoteOns = 0;
    for (const auto& r : out.applied) {
        if (r.id.type == audio::AudioEventType::NoteOn && r.id.role == role && r.disposition == audio::EventDisposition::Applied) ++appliedNoteOns;
    }
    out.check.rendered = true;
    out.check.probePassed = appliedNoteOns == notes.size();
    if (!out.check.probePassed) out.check.probeFailure = std::to_string(appliedNoteOns) + " of " + std::to_string(notes.size()) + " NoteOn applied";
    out.check.expectedFrames = static_cast<std::size_t>(ticks * kTick);
    out.check.stemFrames = cap.frames();
    out.check.stemOverflowFrames = cap.overflowFrames();
    out.check.appliedOverflow = log.overflow();
    return out;
}

// Unmatched references of one role after the frozen-profile detection and the +-50 ms matching
std::vector<std::size_t> unmatchedReferences(std::span<const float> stem, const std::vector<double>& refs, const RoleOnsetProfile& profile) {
    OnsetParams p{};
    p.delta = profile.delta;
    const OnsetDetection det = detectOnsetsSeconds(monoFromStereo(stem), p, kFs);
    std::vector<std::size_t> out;
    if (det.status != AnalysisStatus::Ok) return out;
    std::vector<double> est = det.timesS;
    for (auto& t : est) t -= profile.offsetS;
    const OnsetScore sc = matchOnsetsMaxCardinality(refs, est, kToleranceS); // refs already sorted
    std::vector<bool> matched(refs.size(), false);
    for (const auto& pr : sc.pairs) matched[pr.reference] = true;
    for (std::size_t i = 0; i < refs.size(); ++i) {
        if (!matched[i]) out.push_back(i);
    }
    return out;
}

std::vector<float> fadedSlice(const std::vector<float>& stereo, std::uint64_t start, std::uint64_t end) {
    std::vector<float> out;
    if (end <= start || 2 * end > stereo.size()) return out;
    const auto n = static_cast<std::size_t>(end - start);
    out.assign(stereo.begin() + static_cast<std::ptrdiff_t>(2 * start), stereo.begin() + static_cast<std::ptrdiff_t>(2 * end));
    const auto fade = static_cast<std::size_t>(std::llround(kClipFadeS * kFs));
    constexpr double kPi = 3.14159265358979323846;
    for (std::size_t k = 0; k < fade && k < n; ++k) {
        const auto g = static_cast<float>(0.5 * (1.0 - std::cos(kPi * (static_cast<double>(k) + 0.5) / static_cast<double>(fade))));
        for (std::size_t ch = 0; ch < 2; ++ch) {
            out[2 * k + ch] *= g;
            out[2 * (n - 1 - k) + ch] *= g;
        }
    }
    return out;
}

} // namespace

std::size_t ReleaseSetMeasure::releaseDetectionsTotal() const noexcept {
    std::size_t s = 0;
    for (const auto& r : roles) s += r.releaseDetections;
    return s;
}

std::vector<std::string> sameEventsViolations(const std::vector<ReleaseSetMeasure>& measures) {
    std::vector<std::string> out;
    for (const auto& m : measures) {
        const ReleaseSetMeasure* base = nullptr;
        for (const auto& o : measures) {
            if (o.set == m.set && o.releaseMs == 2.0) base = &o;
        }
        if (base == nullptr) out.push_back(m.set + " " + msLabel(m.releaseMs) + " (no 2 ms baseline)");
        else if (base->eventsHash != m.eventsHash) out.push_back(m.set + " " + msLabel(m.releaseMs));
    }
    return out;
}

HypothesisOutcome evaluateH1(const std::vector<ReleaseSetMeasure>& measures) {
    HypothesisOutcome h;
    std::ostringstream detail;
    bool all = true;
    for (const char* set : {"set_b", "planner_probe_v1", "fixture"}) {
        const ReleaseSetMeasure* base = nullptr;
        const ReleaseSetMeasure* at = nullptr;
        for (const auto& m : measures) {
            if (m.set != set) continue;
            if (m.releaseMs == 2.0) base = &m;
            if (m.releaseMs == kH1ReleaseMs) at = &m;
        }
        if (base == nullptr || at == nullptr || !base->captureInvalid.empty() || !at->captureInvalid.empty()) {
            h.outcome = "INCONCLUSIVE";
            h.detail = std::string("missing or invalid measure for ") + set;
            return h;
        }
        const auto b = base->releaseDetectionsTotal();
        if (b == 0) {
            h.outcome = "INCONCLUSIVE";
            h.detail = std::string("no release detection at 2 ms in ") + set;
            return h;
        }
        const double ratio = static_cast<double>(at->releaseDetectionsTotal()) / static_cast<double>(b);
        detail << set << ' ' << at->releaseDetectionsTotal() << '/' << b << " = " << std::fixed << std::setprecision(3) << ratio << "; ";
        all = all && ratio <= kH1MaxRatio;
    }
    h.outcome = all ? "SUPPORTED" : "NOT_SUPPORTED";
    h.detail = detail.str();
    return h;
}

HypothesisOutcome evaluateH2(const std::vector<ReleaseSetMeasure>& measures) {
    HypothesisOutcome h;
    for (const auto& m : measures) {
        if (m.set != "set_b" || m.releaseMs != kH1ReleaseMs) continue;
        const auto& trama = m.roles[2];
        if (!m.captureInvalid.empty() || trama.status != "MEASURED" || !trama.recall50) {
            h.outcome = "INCONCLUSIVE";
            h.detail = "Trama recall on set B at 30 ms not measured";
            return h;
        }
        std::ostringstream d;
        d << "Trama recall set B at 30 ms = " << std::fixed << std::setprecision(3) << *trama.recall50;
        h.outcome = *trama.recall50 >= kH2MinRecall ? "SUPPORTED" : "NOT_SUPPORTED";
        h.detail = d.str();
        return h;
    }
    h.outcome = "INCONCLUSIVE";
    h.detail = "no set B measure at 30 ms";
    return h;
}

HypothesisOutcome evaluateR2(const std::vector<std::size_t>& unmatched, const std::vector<double>& precedingIntervalS,
                             std::size_t totalReferences) {
    HypothesisOutcome h;
    if (totalReferences == 0 || unmatched.size() != precedingIntervalS.size()) {
        h.outcome = "INCONCLUSIVE";
        h.detail = "no Trama reference on set B";
        return h;
    }
    std::size_t shortPreceded = 0;
    for (const double v : precedingIntervalS) {
        if (std::abs(v - 0.18) < 1e-6 || std::abs(v - 0.22) < 1e-6) ++shortPreceded;
    }
    std::ostringstream d;
    d << unmatched.size() << " unmatched of " << totalReferences << ", " << shortPreceded << " preceded by a 0.18 s or 0.22 s interval";
    h.outcome = (unmatched.size() == 10 && shortPreceded == unmatched.size()) ? "SUPPORTED" : "NOT_SUPPORTED";
    h.detail = d.str();
    return h;
}

ReleaseAbResult runReleaseAbV1(const std::string& outputDir, bool writeFiles) {
    auto runOnce = [&](bool withClips) {
        ReleaseAbResult r;
        const auto frozen = frozenStemProfileV1();
        if (!frozen) {
            r.status = "INCONCLUSIVE";
            r.reasons.emplace_back("frozen onset profile unavailable");
            return r;
        }
        StemCapture synthCap(48000 * 30);
        StemCapture fixtureCap(kFixtureCaptureFrames);
        std::vector<float> fixtureStereo;
        const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();
        for (const double ms : kReleaseVariantsMs) {
            const std::uint32_t frames = releaseFramesOf(ms);
            // Synthetic sets A and B: one render per role
            for (const char* setName : {"set_a", "set_b"}) {
                ReleaseSetMeasure m;
                m.set = setName;
                m.releaseMs = ms;
                m.releaseFrames = frames;
                std::uint64_t h = 1469598103934665603ULL;
                for (std::size_t role = 0; role < kRoleCount; ++role) {
                    const auto notes = std::string(setName) == "set_a" ? syntheticNotesSetA(role) : syntheticNotesSetB(role);
                    const SynthRender sr = renderSynthetic(static_cast<audio::MusicRole>(role), notes, synthCap, frames);
                    m.captureInvalid += captureInvalidReason(sr.check);
                    h = eventsHash(sr.applied, h);
                    m.voiceDrops += sr.confounders.voiceDrops;
                    m.releaseCuts += sr.confounders.releaseCuts;
                    m.releaseOverlaps += sr.confounders.releaseOverlaps;
                    if (captureInvalidReason(sr.check).empty()) {
                        m.roles[role] = measureRole(synthCap.stem(role), sr.applied, role, frozen->roles[role]);
                        if (role == 2 && std::string(setName) == "set_b" && ms == 2.0) {
                            const auto refs = roleReferenceOnsetsS(sr.applied, audio::MusicRole::Trama, kFs);
                            r.r2UnmatchedTrama = unmatchedReferences(synthCap.stem(role), refs, frozen->roles[role]);
                            for (const auto i : r.r2UnmatchedTrama) {
                                r.r2PrecedingIntervalS.push_back(i == 0 ? std::numeric_limits<double>::quiet_NaN()
                                                                        : notes[i].timeS - notes[i - 1].timeS);
                            }
                            r.r2 = evaluateR2(r.r2UnmatchedTrama, r.r2PrecedingIntervalS, refs.size());
                        }
                    }
                }
                m.eventsHash = h;
                r.measures.push_back(m);
            }
            // planner_probe_v1
            {
                ReleaseSetMeasure m;
                m.set = "planner_probe_v1";
                m.releaseMs = ms;
                m.releaseFrames = frames;
                StemCapture cap(800000);
                std::vector<AppliedEventRecord> applied;
                std::uint64_t appliedOverflow = 0;
                audio::VoiceConfounders conf{};
                audio::ProbeAnalysisCapture pc{};
                pc.stems = cap.sink();
                pc.applied = &applied;
                pc.appliedOverflow = &appliedOverflow;
                pc.noteReleaseFrames = frames;
                pc.confounders = &conf;
                const auto probe = audio::runPlannerProbeV1Offline(outputDir, false, &pc);
                CaptureCheck c{true, probe.passed, probe.failureReason, static_cast<std::size_t>(probe.totalCapturedFrames), cap.frames(),
                               cap.overflowFrames(), appliedOverflow};
                m.captureInvalid = captureInvalidReason(c);
                m.eventsHash = eventsHash(applied);
                m.voiceDrops = conf.voiceDrops;
                m.releaseCuts = conf.releaseCuts;
                m.releaseOverlaps = conf.releaseOverlaps;
                if (m.captureInvalid.empty()) {
                    for (std::size_t role = 0; role < kRoleCount; ++role) m.roles[role] = measureRole(cap.stem(role), applied, role, frozen->roles[role]);
                }
                r.measures.push_back(m);
            }
            // Fixture: measures on the stems and the A/B clips from the final PCM
            {
                ReleaseSetMeasure m;
                m.set = "fixture";
                m.releaseMs = ms;
                m.releaseFrames = frames;
                fixtureCap.reset();
                std::vector<AppliedEventRecord> applied;
                std::uint64_t appliedOverflow = 0;
                audio::VoiceConfounders conf{};
                audio::ProbeAnalysisCapture pc{};
                pc.stems = fixtureCap.sink();
                pc.applied = &applied;
                pc.appliedOverflow = &appliedOverflow;
                pc.stereo = &fixtureStereo;
                pc.noteReleaseFrames = frames;
                pc.confounders = &conf;
                const auto probe = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);
                CaptureCheck c{true, probe.passed, probe.failureReason, static_cast<std::size_t>(probe.requestedSampleFrames),
                               fixtureCap.frames(), fixtureCap.overflowFrames(), appliedOverflow};
                m.captureInvalid = captureInvalidReason(c);
                m.eventsHash = eventsHash(applied);
                m.voiceDrops = conf.voiceDrops;
                m.releaseCuts = conf.releaseCuts;
                m.releaseOverlaps = conf.releaseOverlaps;
                if (m.captureInvalid.empty()) {
                    for (std::size_t role = 0; role < kRoleCount; ++role) m.roles[role] = measureRole(fixtureCap.stem(role), applied, role, frozen->roles[role]);
                }
                if (withClips && clock && m.captureInvalid.empty()) {
                    const auto phrases = audio::MusicPlanner::makeFixtureM24V1Phrases(*clock);
                    namespace fs = std::filesystem;
                    std::error_code ec;
                    const fs::path dir = fs::path(outputDir) / "release_ab_v1";
                    fs::create_directories(dir, ec);
                    for (const auto pi : kReleaseClipPhrases) {
                        if (pi >= phrases.size()) continue;
                        const auto clip = fadedSlice(fixtureStereo, phrases[pi].startFrame, phrases[pi].endFrame);
                        std::ostringstream name;
                        name << "fixture_p" << std::setw(2) << std::setfill('0') << pi << "_release_" << msLabel(ms) << ".wav";
                        const auto bytes = encodeWavStereo16(clip, kFs);
                        std::ofstream out(dir / name.str(), std::ios::binary);
                        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
                        if (out.good() && !clip.empty()) r.clips.push_back(name.str());
                        else r.reasons.push_back("cannot write clip " + name.str());
                    }
                }
                r.measures.push_back(m);
            }
        }
        // Validity: every capture valid, every analysis valid, the same events in every variant of a set
        for (const auto& m : r.measures) {
            if (!m.captureInvalid.empty()) r.reasons.push_back(m.set + " " + msLabel(m.releaseMs) + ": invalid capture: " + m.captureInvalid);
            for (std::size_t role = 0; role < kRoleCount; ++role) {
                const auto& st = m.roles[role].status;
                if (m.captureInvalid.empty() && st != "MEASURED" && st != "NO_REFERENCES") {
                    r.reasons.push_back(m.set + " " + msLabel(m.releaseMs) + " " + kRoleNames[role] + ": analysis " + st);
                }
            }
        }
        r.sameEventsViolations = sameEventsViolations(r.measures);
        for (const auto& v : r.sameEventsViolations) r.reasons.push_back("events differ from the 2 ms variant: " + v);
        r.h1 = evaluateH1(r.measures);
        r.h2 = evaluateH2(r.measures);
        if (r.r2.outcome == "NOT_RUN") r.r2 = {"INCONCLUSIVE", "set B Trama at 2 ms not measured"};
        r.status = r.reasons.empty() ? "MEASURED" : "INCONCLUSIVE";
        return r;
    };

    ReleaseAbResult r = runOnce(writeFiles);
    const ReleaseAbResult again = runOnce(false);
    r.repeatIdentical = again.measuresJson() == r.measuresJson();
    if (!r.repeatIdentical) {
        r.reasons.emplace_back("repeat run differs");
        r.status = "INCONCLUSIVE";
    }
    if (writeFiles) {
        r.manifestFilePath = (std::filesystem::path(outputDir) / "release_ab_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(r.manifestFilePath);
        r.manifestWriteSuccess = mf.is_open();
        if (r.manifestWriteSuccess) {
            mf << r.toJson();
            r.manifestWriteSuccess = mf.good();
        }
    }
    return r;
}

std::string ReleaseAbResult::measuresJson() const {
    json a = json::array();
    for (const auto& m : measures) {
        json roles = json::object();
        for (std::size_t r = 0; r < kRoleCount; ++r) {
            const auto& x = m.roles[r];
            const bool ok = x.status == "MEASURED";
            roles[kRoleNames[r]] = {{"status", x.status}, {"references", x.references}, {"detections", x.detections},
                                    {"matched_50ms", ok ? json(x.matched) : json(nullptr)},
                                    {"release_detections", ok ? json(x.releaseDetections) : json(nullptr)},
                                    {"precision_50", ok ? optNum(x.precision50) : json(nullptr)},
                                    {"recall_50", ok ? optNum(x.recall50) : json(nullptr)},
                                    {"precision_excl_releases_50", ok ? optNum(x.precisionExclReleases) : json(nullptr)}};
        }
        a.push_back({{"set", m.set}, {"release_ms", m.releaseMs}, {"release_frames", m.releaseFrames},
                     {"production_release", m.releaseFrames == 0}, {"capture_valid", m.captureInvalid.empty()},
                     {"events_hash", m.eventsHash}, {"release_detections_total", m.releaseDetectionsTotal()},
                     {"confounders", {{"voice_drops", m.voiceDrops}, {"release_cuts", m.releaseCuts}, {"release_overlaps", m.releaseOverlaps}}},
                     {"roles", roles}});
    }
    json unmatchedJson = json::array();
    for (std::size_t i = 0; i < r2UnmatchedTrama.size(); ++i) {
        unmatchedJson.push_back({{"reference_index", r2UnmatchedTrama[i]},
                      {"preceding_interval_s", i < r2PrecedingIntervalS.size() && std::isfinite(r2PrecedingIntervalS[i])
                                                   ? json(r2PrecedingIntervalS[i]) : json(nullptr)}});
    }
    json j{{"measures", a},
           {"hypotheses", {{"h1_release_detections", {{"outcome", h1.outcome}, {"detail", h1.detail}}},
                           {"h2_trama_recall_set_b", {{"outcome", h2.outcome}, {"detail", h2.detail}}},
                           {"r2_unmatched_trama_set_b_2ms", {{"outcome", r2.outcome}, {"detail", r2.detail}, {"unmatched", unmatchedJson}}}}},
           {"same_events_violations", sameEventsViolations}};
    return j.dump();
}

std::string ReleaseAbResult::toJson() const {
    json j = json::parse(measuresJson());
    j["manifest_version"] = "1.0";
    j["scenario"] = "release_ab_v1";
    j["build_id"] = CYMATICA_BUILD_ID;
    j["verdict"] = status;
    j["gate"] = false;
    j["reasons"] = reasons;
    j["repeat_identical"] = repeatIdentical;
    j["rules"] = {{"variants_ms", kReleaseVariantsMs},
                  {"h1", "SUPPORTED when release detections at 30 ms <= 20 % of 2 ms in set B, planner_probe_v1 and fixture"},
                  {"h2", "SUPPORTED when the Trama recall on set B at 30 ms >= 0.90"},
                  {"r2", "SUPPORTED when the unmatched Trama references of set B at 2 ms are exactly the 10 notes after 0.18 s / 0.22 s"},
                  {"note", "only the release of ordinary NoteOffs varies; steal tails, Pause and Stop keep the 2 ms de-click; the "
                           "2 ms variant is the production release without override; no production release is adopted in Inc-5"}};
    j["clips"] = {{"dir", "release_ab_v1"}, {"files", clips},
                  {"source", "fixture_probe_v1 final PCM, whole phrases, 10 ms raised-cosine fades; for the expert's A/B listening"}};
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
