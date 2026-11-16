#include "kick_level_ab.h"

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "evaluation_pack.h"
#include "loudness.h"
#include "music_planner.h"
#include "stem_analysis.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::size_t kFixtureCaptureFrames = 8'900'000; // > 8,825,200 frames of fixture_probe_v1
constexpr double kClipFadeS = 0.010;
constexpr std::array<const char*, 3> kSectionNames{"A", "B", "Aprime"};

constexpr std::uint64_t kFnvOffset = 1469598103934665603ULL;

void fnvMix(std::uint64_t& h, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) {
        h ^= (v >> (8 * i)) & 0xFFU;
        h *= 1099511628211ULL;
    }
}

// FNV-1a over identity, consumed frame, disposition and slot of every record consumed by the engine
std::uint64_t eventsHash(const std::vector<AppliedEventRecord>& applied) {
    std::uint64_t h = kFnvOffset;
    for (const auto& r : applied) {
        fnvMix(h, static_cast<std::uint64_t>(r.id.type));
        fnvMix(h, r.id.eventId);
        fnvMix(h, static_cast<std::uint64_t>(r.id.role));
        fnvMix(h, r.id.sampleFrame);
        fnvMix(h, r.id.transportEpoch);
        fnvMix(h, r.consumedFrame);
        fnvMix(h, static_cast<std::uint64_t>(r.disposition));
        fnvMix(h, static_cast<std::uint64_t>(static_cast<std::int64_t>(r.slot)));
    }
    return h;
}

std::string offsetLabel(double db) {
    std::ostringstream s;
    s << '+' << std::setw(2) << std::setfill('0') << static_cast<int>(db) << "dB";
    return s.str();
}

json optNum(const std::optional<double>& v) { return (v && std::isfinite(*v)) ? json(*v) : json(nullptr); }

std::optional<double> integrated(std::span<const float> pcm) {
    const LoudnessResult r = measureLoudnessStereo(pcm, kFs);
    if (r.status != AnalysisStatus::Ok) return std::nullopt;
    return r.integratedLufs;
}

std::span<const float> frameRange(std::span<const float> pcm, std::uint64_t start, std::uint64_t end) {
    if (end <= start || 2 * end > pcm.size()) return {};
    return pcm.subspan(static_cast<std::size_t>(2 * start), static_cast<std::size_t>(2 * (end - start)));
}

std::vector<float> fadedSlice(std::span<const float> stereo, std::uint64_t start, std::uint64_t end) {
    const auto r = frameRange(stereo, start, end);
    std::vector<float> out(r.begin(), r.end());
    const std::size_t n = out.size() / 2;
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

struct SectionRange {
    std::uint32_t section{0};
    std::uint64_t start{0};
    std::uint64_t end{0};
};

struct ClipRange {
    std::uint32_t section{0};
    std::uint64_t start{0};
    std::uint64_t end{0};
};

// Section ranges (A, B, A') from the fixture phrases, the last one ending at the content end
std::vector<SectionRange> sectionRanges(const std::array<audio::PhrasePlan, 24>& phrases, std::uint64_t contentEnd) {
    std::map<std::uint32_t, std::uint64_t> starts;
    for (const auto& p : phrases) {
        const auto it = starts.find(p.sectionIndex);
        if (it == starts.end() || p.startFrame < it->second) starts[p.sectionIndex] = p.startFrame;
    }
    std::vector<SectionRange> out;
    for (auto it = starts.begin(); it != starts.end(); ++it) {
        const auto next = std::next(it);
        out.push_back({it->first, it->second, next != starts.end() ? next->second : contentEnd});
    }
    return out;
}

// Pre-registered clips: phrases 3 and 4 of each section (bars 9-16 of the section)
std::vector<ClipRange> clipRanges(const std::array<audio::PhrasePlan, 24>& phrases) {
    std::map<std::uint32_t, std::vector<const audio::PhrasePlan*>> bySection;
    for (const auto& p : phrases) bySection[p.sectionIndex].push_back(&p);
    std::vector<ClipRange> out;
    for (auto& [section, list] : bySection) {
        std::sort(list.begin(), list.end(), [](const auto* a, const auto* b) { return a->startFrame < b->startFrame; });
        if (list.size() >= 4) out.push_back({section, list[2]->startFrame, list[3]->endFrame});
    }
    return out;
}

bool writeWav(const std::filesystem::path& path, std::span<const float> pcm) {
    const auto bytes = encodeWavStereo16(pcm, kFs);
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return out.good() && !pcm.empty();
}

} // namespace

float kickGainScale(double offsetDb) noexcept {
    if (offsetDb == 0.0) return 1.0f;
    return static_cast<float>(std::pow(10.0, offsetDb / 20.0));
}

std::optional<double> loudnessAlignGain(std::optional<double> clipLufs, std::optional<double> targetLufs) noexcept {
    if (!clipLufs || !targetLufs || !std::isfinite(*clipLufs) || !std::isfinite(*targetLufs)) return std::nullopt;
    return std::pow(10.0, (*targetLufs - *clipLufs) / 20.0);
}

KickNullTest kickNullTest(float scale, const std::array<std::span<const float>, 4>& stems0, std::span<const float> bus0,
                          const std::array<std::span<const float>, 4>& stems, std::span<const float> bus) {
    KickNullTest t;
    const std::size_t n = bus0.size();
    if (bus.size() != n) return t;
    for (std::size_t r = 0; r < 4; ++r) {
        if (stems0[r].size() != n || stems[r].size() != n) return t;
    }
    const double k = static_cast<double>(scale);
    for (std::size_t r = 0; r < 4; ++r) {
        const double factor = (r == 0) ? k : 1.0;
        for (std::size_t i = 0; i < n; ++i) {
            const double d = std::abs(static_cast<double>(stems[r][i]) - factor * static_cast<double>(stems0[r][i]));
            if (d > t.stemMaxAbsResidual[r]) {
                t.stemMaxAbsResidual[r] = d;
                t.stemWorstSample[r] = i;
            }
        }
    }
    for (std::size_t i = 0; i < n; ++i) {
        const double expected = static_cast<double>(bus0[i]) + (k - 1.0) * static_cast<double>(stems0[0][i]);
        const double d = std::abs(static_cast<double>(bus[i]) - expected);
        if (d > t.busMaxAbsResidual) {
            t.busMaxAbsResidual = d;
            t.busWorstSample = i;
        }
    }
    t.computed = true;
    return t;
}

KickOutcome evaluateKickP1(const std::vector<KickVariantMeasure>& measures) {
    if (measures.empty() || measures.front().offsetDb != 0.0) return {"INCONCLUSIVE", "offset-0 variant missing"};
    const auto& m = measures.front();
    if (m.goldenPcmStatus == "PASS") return {"SUPPORTED", "offset 0: PCM content hash equals the golden"};
    std::ostringstream s;
    s << "offset 0: golden status " << m.goldenPcmStatus << ", hash " << m.pcmContentHash << " (golden " << kFixtureGoldenPcm << ")";
    return {"NOT_SUPPORTED", s.str()};
}

KickOutcome evaluateKickP2(const std::vector<KickVariantMeasure>& measures) {
    if (measures.empty() || measures.front().offsetDb != 0.0) return {"INCONCLUSIVE", "offset-0 variant missing"};
    std::vector<std::string> differ;
    for (const auto& m : measures) {
        if (m.eventsHash != measures.front().eventsHash) differ.push_back(offsetLabel(m.offsetDb));
    }
    if (differ.empty()) return {"SUPPORTED", "same applied events in every variant"};
    std::string d = "events differ from offset 0:";
    for (const auto& x : differ) d += " " + x;
    return {"NOT_SUPPORTED", d};
}

KickOutcome evaluateKickP3(const KickVariantMeasure& m) {
    if (!m.captureInvalid.empty() || !m.nullTest.computed) return {"INCONCLUSIVE", "null test not computed"};
    std::ostringstream s;
    s << std::scientific << std::setprecision(3);
    bool ok = true;
    for (std::size_t r = 0; r < 4; ++r) {
        s << kRoleNames[r] << " " << m.nullTest.stemMaxAbsResidual[r] << "; ";
        if (!(m.nullTest.stemMaxAbsResidual[r] <= kP3StemTolerance)) {
            ok = false;
            s << "(worst sample " << m.nullTest.stemWorstSample[r] << ") ";
        }
    }
    s << "bus " << m.nullTest.busMaxAbsResidual;
    if (!(m.nullTest.busMaxAbsResidual <= kP3BusTolerance)) {
        ok = false;
        s << " (worst sample " << m.nullTest.busWorstSample << ")";
    }
    return {ok ? "SUPPORTED" : "NOT_SUPPORTED", s.str()};
}

KickOutcome evaluateKickP4(const KickVariantMeasure& base, const KickVariantMeasure& m) {
    if (!base.stemLufs[0] || !m.stemLufs[0]) return {"INCONCLUSIVE", "Pulso loudness undefined"};
    const double rise = *m.stemLufs[0] - *base.stemLufs[0];
    std::ostringstream s;
    s << std::fixed << std::setprecision(3) << "Pulso rise " << rise << " LU, predicted " << m.offsetDb << " +/- " << kP4ToleranceLu;
    return {std::abs(rise - m.offsetDb) <= kP4ToleranceLu ? "SUPPORTED" : "NOT_SUPPORTED", s.str()};
}

KickOutcome evaluateKickP5(const KickVariantMeasure& base, const KickVariantMeasure& m, double predictedRiseLu) {
    if (!base.mixLufs || !m.mixLufs) return {"INCONCLUSIVE", "mix loudness undefined"};
    const double rise = *m.mixLufs - *base.mixLufs;
    std::ostringstream s;
    s << std::fixed << std::setprecision(3) << "mix rise " << rise << " LU, predicted " << predictedRiseLu << " +/- " << kP5ToleranceLu;
    return {std::abs(rise - predictedRiseLu) <= kP5ToleranceLu ? "SUPPORTED" : "NOT_SUPPORTED", s.str()};
}

KickOutcome evaluateKickP6(const KickVariantMeasure& m) {
    if (!m.captureInvalid.empty()) return {"INCONCLUSIVE", "invalid capture"};
    std::ostringstream s;
    s << std::fixed << std::setprecision(3) << "limiter reduction " << m.maxGainReductionDb << " dB";
    if (m.offsetDb > kP6MaxPredictedOffsetDb) return {"NO_PREDICTION", s.str()};
    return {m.maxGainReductionDb == 0.0 ? "SUPPORTED" : "NOT_SUPPORTED", s.str()};
}

KickLevelAbResult runKickLevelAbV1(const std::string& outputDir, bool writeFiles) {
    namespace fs = std::filesystem;
    const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();

    auto runOnce = [&](bool withClips) {
        KickLevelAbResult r;
        if (!clock) {
            r.status = "INCONCLUSIVE";
            r.reasons.emplace_back("fixture clock unavailable");
            return r;
        }
        const auto phrases = audio::MusicPlanner::makeFixtureM24V1Phrases(*clock);
        const auto clipsAt = clipRanges(phrases);
        if (clipsAt.size() != 3) r.reasons.emplace_back("expected 3 sections with at least 4 phrases");

        StemCapture cap(kFixtureCaptureFrames);
        std::array<std::vector<float>, 4> stems0{};
        std::vector<float> bus0;
        std::vector<float> stereo;
        std::vector<std::optional<double>> clipLufs0(clipsAt.size());
        fs::path dir;
        if (withClips) {
            std::error_code ec;
            dir = fs::path(outputDir) / "kick_level_ab_v1";
            fs::create_directories(dir, ec);
        }

        for (const double db : kKickOffsetsDb) {
            KickVariantMeasure m;
            m.offsetDb = db;
            m.scale = kickGainScale(db);
            cap.reset();
            std::vector<AppliedEventRecord> applied;
            std::uint64_t appliedOverflow = 0;
            audio::ProbeAnalysisCapture pc{};
            pc.stems = cap.sink();
            pc.applied = &applied;
            pc.appliedOverflow = &appliedOverflow;
            pc.stereo = &stereo;
            pc.pulsoBusGainScale = m.scale;
            const auto probe = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);
            const CaptureCheck c{true, probe.passed, probe.failureReason, static_cast<std::size_t>(probe.requestedSampleFrames),
                                 cap.frames(), cap.overflowFrames(), appliedOverflow};
            m.captureInvalid = captureInvalidReason(c);
            m.goldenPcmStatus = probe.goldenPcmStatus;
            m.pcmContentHash = probe.pcmContentHash;
            m.eventsHash = eventsHash(applied);
            m.truePeakDbtp = probe.truePeakDbtp;
            m.maxGainReductionDb = probe.maxGainReductionDb;
            if (m.captureInvalid.empty()) {
                m.mixLufs = integrated(stereo);
                m.busLufs = integrated(cap.busMix());
                for (std::size_t role = 0; role < 4; ++role) m.stemLufs[role] = integrated(cap.stem(role));
                for (const auto& sec : sectionRanges(phrases, probe.contentEndFrame)) {
                    m.sectionMixLufs.push_back(integrated(frameRange(stereo, sec.start, sec.end)));
                    m.sectionPulsoLufs.push_back(integrated(frameRange(cap.stem(0), sec.start, sec.end)));
                }
                if (db == 0.0) {
                    for (std::size_t role = 0; role < 4; ++role) stems0[role].assign(cap.stem(role).begin(), cap.stem(role).end());
                    bus0.assign(cap.busMix().begin(), cap.busMix().end());
                } else if (!bus0.empty()) {
                    const std::array<std::span<const float>, 4> s0{stems0[0], stems0[1], stems0[2], stems0[3]};
                    const std::array<std::span<const float>, 4> s{cap.stem(0), cap.stem(1), cap.stem(2), cap.stem(3)};
                    m.nullTest = kickNullTest(m.scale, s0, bus0, s, cap.busMix());
                }
                if (withClips) {
                    for (std::size_t i = 0; i < clipsAt.size(); ++i) {
                        const auto& cr = clipsAt[i];
                        const auto raw = fadedSlice(stereo, cr.start, cr.end);
                        const auto lufs = integrated(raw);
                        if (db == 0.0) clipLufs0[i] = lufs;
                        const std::string base = std::string("f4a_") +
                                                 (cr.section < kSectionNames.size() ? kSectionNames[cr.section] : "S") + "_" + offsetLabel(db);
                        if (writeWav(dir / (base + "_raw.wav"), raw)) r.clips.push_back(base + "_raw.wav");
                        else r.reasons.push_back("cannot write clip " + base + "_raw.wav");
                        const auto g = loudnessAlignGain(lufs, clipLufs0[i]);
                        if (!g) {
                            r.reasons.push_back("cannot align clip " + base);
                            continue;
                        }
                        std::vector<float> aligned = raw;
                        for (auto& x : aligned) x = static_cast<float>(static_cast<double>(x) * *g);
                        if (writeWav(dir / (base + "_aligned.wav"), aligned)) r.clips.push_back(base + "_aligned.wav");
                        else r.reasons.push_back("cannot write clip " + base + "_aligned.wav");
                    }
                }
            }
            r.measures.push_back(m);
        }

        for (const auto& m : r.measures) {
            if (!m.captureInvalid.empty()) r.reasons.push_back(offsetLabel(m.offsetDb) + ": invalid capture: " + m.captureInvalid);
        }
        r.p1 = evaluateKickP1(r.measures);
        r.p2 = evaluateKickP2(r.measures);
        for (std::size_t i = 1; i < r.measures.size(); ++i) {
            const auto& m = r.measures[i];
            r.p3.push_back(evaluateKickP3(m));
            r.p4.push_back(evaluateKickP4(r.measures.front(), m));
            r.p5.push_back(evaluateKickP5(r.measures.front(), m, kP5PredictedRiseLu[i - 1]));
            r.p6.push_back(evaluateKickP6(m));
            if (m.maxGainReductionDb > 0.0) r.limiterConfounded.push_back(offsetLabel(m.offsetDb));
        }
        if (r.p1.outcome != "SUPPORTED" || r.p2.outcome != "SUPPORTED") {
            r.status = "INVALID";
        } else {
            r.status = r.reasons.empty() ? "MEASURED" : "INCONCLUSIVE";
        }
        return r;
    };

    KickLevelAbResult r = runOnce(writeFiles);
    const KickLevelAbResult again = runOnce(false);
    r.repeatIdentical = again.measuresJson() == r.measuresJson();
    r.p7 = r.repeatIdentical ? KickOutcome{"SUPPORTED", "repeat run identical"} : KickOutcome{"NOT_SUPPORTED", "repeat run differs"};
    if (!r.repeatIdentical && r.status == "MEASURED") {
        r.reasons.emplace_back("repeat run differs");
        r.status = "INCONCLUSIVE";
    }
    if (writeFiles) {
        r.manifestFilePath = (fs::path(outputDir) / "kick_level_ab_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(r.manifestFilePath);
        r.manifestWriteSuccess = mf.is_open();
        if (r.manifestWriteSuccess) {
            mf << r.toJson();
            r.manifestWriteSuccess = mf.good();
        }
    }
    return r;
}

namespace {
json outcomeJson(const KickOutcome& o) { return {{"outcome", o.outcome}, {"detail", o.detail}}; }
} // namespace

std::string KickLevelAbResult::measuresJson() const {
    json arr = json::array();
    for (const auto& m : measures) {
        json stems = json::object();
        for (std::size_t r = 0; r < 4; ++r) stems[kRoleNames[r]] = optNum(m.stemLufs[r]);
        json secMix = json::array(), secPulso = json::array();
        for (const auto& v : m.sectionMixLufs) secMix.push_back(optNum(v));
        for (const auto& v : m.sectionPulsoLufs) secPulso.push_back(optNum(v));
        json nt = nullptr;
        if (m.nullTest.computed) {
            json st = json::object();
            for (std::size_t r = 0; r < 4; ++r) {
                st[kRoleNames[r]] = {{"max_abs_residual", m.nullTest.stemMaxAbsResidual[r]}, {"worst_sample", m.nullTest.stemWorstSample[r]}};
            }
            nt = {{"stems", st}, {"bus_max_abs_residual", m.nullTest.busMaxAbsResidual}, {"bus_worst_sample", m.nullTest.busWorstSample}};
        }
        arr.push_back({{"offset_db", m.offsetDb},
                       {"scale", m.scale},
                       {"capture_invalid", m.captureInvalid},
                       {"golden_pcm_status", m.goldenPcmStatus},
                       {"pcm_content_hash", std::to_string(m.pcmContentHash)},
                       {"events_hash", std::to_string(m.eventsHash)},
                       {"mix_integrated_lufs", optNum(m.mixLufs)},
                       {"bus_integrated_lufs", optNum(m.busLufs)},
                       {"stem_integrated_lufs", stems},
                       {"section_mix_lufs", secMix},
                       {"section_pulso_lufs", secPulso},
                       {"true_peak_dbtp", m.truePeakDbtp},
                       {"max_gain_reduction_db", m.maxGainReductionDb},
                       {"null_test", nt}});
    }
    json j3 = json::array(), j4 = json::array(), j5 = json::array(), j6 = json::array();
    for (const auto& o : p3) j3.push_back(outcomeJson(o));
    for (const auto& o : p4) j4.push_back(outcomeJson(o));
    for (const auto& o : p5) j5.push_back(outcomeJson(o));
    for (const auto& o : p6) j6.push_back(outcomeJson(o));
    json j = {{"measures", arr},
              {"predictions", {{"P1", outcomeJson(p1)}, {"P2", outcomeJson(p2)}, {"P3", j3}, {"P4", j4}, {"P5", j5}, {"P6", j6}}},
              {"limiter_confounded", limiterConfounded}};
    return j.dump(2);
}

std::string KickLevelAbResult::toJson() const {
    json j;
    j["scenario"] = "kick_level_ab_v1";
    j["status"] = status;
    j["reasons"] = reasons;
    j["preregistration"] = "docs/research/M2_5_2_F4_Kick_Level_Preregistration.md";
    j["offsets_db"] = kKickOffsetsDb;
    j["deterministic"] = json::parse(measuresJson());
    j["P7"] = outcomeJson(p7);
    j["repeat_identical"] = repeatIdentical;
    j["clips"] = {{"dir", "kick_level_ab_v1"}, {"files", clips},
                  {"selection", "phrases 3-4 of each section (bars 9-16), 10 ms fades; raw and loudness-aligned to offset 0"}};
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
