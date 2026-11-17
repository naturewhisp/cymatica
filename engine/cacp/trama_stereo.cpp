#include "trama_stereo.h"

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "loudness.h"
#include "music_planner.h"
#include "stem_analysis.h"
#include "stereo.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::size_t kFixtureCaptureFrames = 8'900'000; // > 8,825,200 frames of fixture_probe_v1
constexpr std::size_t kTrama = 2;

std::optional<double> integrated(std::span<const float> pcm) {
    const LoudnessResult r = measureLoudnessStereo(pcm, kFs);
    if (r.status != AnalysisStatus::Ok) return std::nullopt;
    return r.integratedLufs;
}

std::optional<double> ildOf(double eL, double eR) {
    if (!(eL > 0.0) || !(eR > 0.0)) return std::nullopt;
    return 10.0 * std::log10(eL / eR);
}

std::uint64_t eventsHash(const std::vector<AppliedEventRecord>& applied) {
    std::uint64_t h = 1469598103934665603ULL;
    const auto mix = [&h](std::uint64_t v) {
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
        mix(r.consumedFrame);
        mix(static_cast<std::uint64_t>(r.disposition));
    }
    return h;
}

json ildJson(const IldResult& r) {
    return {{"ild_db", r.db ? json(*r.db) : json(nullptr)}, {"energy_l", r.energyL}, {"energy_r", r.energyR}};
}

json opt(const std::optional<double>& v) { return (v && std::isfinite(*v)) ? json(*v) : json(nullptr); }

} // namespace

IldResult ildDb(std::span<const float> stereo, std::size_t startFrame, std::size_t endFrame) noexcept {
    IldResult r;
    const std::size_t frames = stereo.size() / 2;
    endFrame = (std::min)(endFrame, frames);
    for (std::size_t f = startFrame; f < endFrame; ++f) {
        r.energyL += static_cast<double>(stereo[2 * f]) * stereo[2 * f];
        r.energyR += static_cast<double>(stereo[2 * f + 1]) * stereo[2 * f + 1];
    }
    r.db = ildOf(r.energyL, r.energyR);
    return r;
}

IldResult ildOverWindows(std::span<const float> stereo, std::span<const std::size_t> onsetFrames, std::size_t windowFrames) noexcept {
    IldResult r;
    for (const std::size_t on : onsetFrames) {
        const IldResult w = ildDb(stereo, on, on + windowFrames);
        r.energyL += w.energyL;
        r.energyR += w.energyR;
    }
    r.db = ildOf(r.energyL, r.energyR);
    return r;
}

std::optional<double> lrCorrelation(std::span<const float> stereo) noexcept {
    const std::size_t n = stereo.size() / 2;
    if (n == 0) return std::nullopt;
    double sl = 0, sr = 0, sll = 0, srr = 0, slr = 0;
    for (std::size_t f = 0; f < n; ++f) {
        const double l = stereo[2 * f], rr = stereo[2 * f + 1];
        sl += l;
        sr += rr;
        sll += l * l;
        srr += rr * rr;
        slr += l * rr;
    }
    const double nn = static_cast<double>(n);
    const double vl = sll - sl * sl / nn, vr = srr - sr * sr / nn;
    if (!(vl > 0.0) || !(vr > 0.0)) return std::nullopt;
    return (slr - sl * sr / nn) / std::sqrt(vl * vr);
}

TramaStereoResult runTramaStereoV1(const std::string& outputDir, bool writeFiles) {
    TramaStereoResult r;
    StemCapture cap(kFixtureCaptureFrames);
    std::vector<AppliedEventRecord> applied;
    std::uint64_t appliedOverflow = 0;
    std::vector<float> stereo;
    audio::ProbeAnalysisCapture pc{};
    pc.stems = cap.sink();
    pc.applied = &applied;
    pc.appliedOverflow = &appliedOverflow;
    pc.stereo = &stereo;
    const auto probe = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);
    const CaptureCheck c{true, probe.passed, probe.failureReason, static_cast<std::size_t>(probe.requestedSampleFrames),
                         cap.frames(), cap.overflowFrames(), appliedOverflow};
    const std::string invalid = captureInvalidReason(c);
    r.goldenPcmStatus = probe.goldenPcmStatus;
    r.goldenTracePassed = probe.goldenTracePassed;
    r.goldenPlanPassed = probe.goldenPlanPassed;
    r.pcmContentHash = probe.pcmContentHash;
    r.eventsHash = eventsHash(applied);
    r.truePeakDbtp = probe.truePeakDbtp;
    r.maxGainReductionDb = probe.maxGainReductionDb;
    // The capture is analysed even when only the golden PCM differs (expected before a reviewed golden update)
    if (!invalid.empty() && !(probe.goldenPcmStatus == "FAIL" && cap.frames() == probe.requestedSampleFrames && appliedOverflow == 0 &&
                              cap.overflowFrames() == 0)) {
        r.status = "INCONCLUSIVE";
        r.reasons.push_back("invalid capture: " + invalid);
    } else {
        if (!invalid.empty()) r.reasons.push_back("note: golden PCM differs (measured anyway): " + invalid);
        const auto trama = cap.stem(kTrama);
        std::vector<std::size_t> onsets;
        for (const double s : roleReferenceOnsetsS(applied, audio::MusicRole::Trama, kFs)) {
            onsets.push_back(static_cast<std::size_t>(std::llround(s * kFs)));
        }
        r.tramaAttacks = onsets.size();
        r.firstWavefront = ildOverWindows(trama, onsets, static_cast<std::size_t>(std::llround(kFirstWavefrontS * kFs)));
        r.whole = ildDb(trama, 0, trama.size() / 2);
        if (const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock()) {
            std::map<std::uint32_t, std::uint64_t> starts;
            for (const auto& p : audio::MusicPlanner::makeFixtureM24V1Phrases(*clock)) {
                const auto it = starts.find(p.sectionIndex);
                if (it == starts.end() || p.startFrame < it->second) starts[p.sectionIndex] = p.startFrame;
            }
            for (auto it = starts.begin(); it != starts.end(); ++it) {
                const auto next = std::next(it);
                const std::uint64_t end = next != starts.end() ? next->second : probe.contentEndFrame;
                r.sections.push_back(ildDb(trama, static_cast<std::size_t>(it->second), static_cast<std::size_t>(end)));
            }
        }
        r.sideMidDb = sideMidDb(trama);
        r.correlation = lrCorrelation(trama);
        r.tramaLufs = integrated(trama);
        r.mixLufs = integrated(stereo);
        for (std::size_t role = 0; role < 4; ++role) r.stemLufs[role] = integrated(cap.stem(role));
        if (r.tramaAttacks == 0) r.reasons.push_back("no Trama attacks");
        r.status = (r.tramaAttacks > 0 && r.firstWavefront.db && r.whole.db) ? "MEASURED" : "INCONCLUSIVE";
    }
    if (writeFiles) {
        r.manifestFilePath = (std::filesystem::path(outputDir) / "trama_stereo_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(r.manifestFilePath);
        r.manifestWriteSuccess = mf.is_open();
        if (r.manifestWriteSuccess) {
            mf << r.toJson();
            r.manifestWriteSuccess = mf.good();
        }
    }
    return r;
}

std::string TramaStereoResult::toJson() const {
    json secs = json::array();
    for (const auto& s : sections) secs.push_back(ildJson(s));
    json stems = json::object();
    for (std::size_t role = 0; role < 4; ++role) stems[kRoleNames[role]] = opt(stemLufs[role]);
    json j = {{"scenario", "trama_stereo_v1"},
              {"status", status},
              {"reasons", reasons},
              {"preregistration", "docs/research/M2_5_2_F2_Trama_Stereo_Preregistration.md"},
              {"trama_attacks", tramaAttacks},
              {"first_wavefront_window_s", kFirstWavefrontS},
              {"first_wavefront", ildJson(firstWavefront)},
              {"whole", ildJson(whole)},
              {"sections", secs},
              {"side_mid_db", opt(sideMidDb)},
              {"lr_correlation", opt(correlation)},
              {"trama_integrated_lufs", opt(tramaLufs)},
              {"mix_integrated_lufs", opt(mixLufs)},
              {"stem_integrated_lufs", stems},
              {"true_peak_dbtp", truePeakDbtp},
              {"max_gain_reduction_db", maxGainReductionDb},
              {"golden_pcm_status", goldenPcmStatus},
              {"golden_trace_passed", goldenTracePassed},
              {"golden_plan_passed", goldenPlanPassed},
              {"pcm_content_hash", std::to_string(pcmContentHash)},
              {"events_hash", std::to_string(eventsHash)}};
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
