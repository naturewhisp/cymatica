#include "onset_parity.h"

#include "audio_probe.h"
#include "music_planner.h"
#include "onset.h"
#include "stft.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace cymatica::cacp {

namespace {

constexpr std::uint32_t kSampleRate = 48000;
constexpr double kToleranceS = 0.050;
constexpr std::array<double, 3> kDeltas{0.05, 0.10, 0.20};
// E1 (onset_report_2026-10-21.json): detected, precision, recall per delta
constexpr std::array<std::size_t, 3> kE1Detected{991, 730, 464};
constexpr std::array<double, 3> kE1Precision{0.920, 1.000, 1.000};
constexpr std::array<double, 3> kE1Recall{0.905, 0.724, 0.460};

double round3(double v) { return std::round(v * 1000.0) / 1000.0; }

// numpy.percentile default ("linear") on sorted data
double percentileLinear(std::vector<double> v, double q) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    const double pos = q / 100.0 * static_cast<double>(v.size() - 1);
    const auto lo = static_cast<std::size_t>(std::floor(pos));
    const auto hi = (std::min)(lo + 1, v.size() - 1);
    return v[lo] + (v[hi] - v[lo]) * (pos - static_cast<double>(lo));
}

std::string num(double v) {
    if (!std::isfinite(v)) return "null";
    std::ostringstream s;
    s << std::setprecision(10) << v;
    return s.str();
}

} // namespace

OnsetParityResult runOnsetParityV1(const std::string& outputDir, bool writeFiles) {
    OnsetParityResult r;

    // Input: fixture rendered in-process, same PCM as fixture_probe_v1.wav
    std::vector<float> stereo;
    const auto fixture = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &stereo);
    r.fixturePassed = fixture.passed;
    r.pcmContentHash = fixture.pcmContentHash;
    const std::uint64_t contentEnd = fixture.contentEndFrame;
    if (contentEnd == 0 || stereo.size() < contentEnd * 2) {
        r.failureReason = "fixture render unavailable or shorter than the content";
        return r;
    }

    // 16-bit WAV quantisation as E1 read it: int16 = round(clamp(x) * 32767), value = int16 / 32768
    std::vector<float> mono(static_cast<std::size_t>(contentEnd));
    for (std::size_t i = 0; i < mono.size(); ++i) {
        double ch[2];
        for (int c = 0; c < 2; ++c) {
            const float raw = stereo[2 * i + static_cast<std::size_t>(c)];
            const float s = std::isfinite(raw) ? std::clamp(raw, -1.0f, 1.0f) : 0.0f;
            ch[c] = static_cast<double>(static_cast<std::int16_t>(std::round(s * 32767.0f))) / 32768.0;
        }
        mono[i] = static_cast<float>(0.5 * (ch[0] + ch[1]));
    }
    r.analysedFrames = contentEnd;

    // Reference: unique NoteOn frames of the offline plan (seed 1337, 8 candidates, horizon 0, epoch section + 1)
    const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();
    if (!clock.has_value()) {
        r.failureReason = "fixture clock unavailable";
        return r;
    }
    const auto phrases = audio::MusicPlanner::makeFixtureM24V1Phrases(*clock);
    audio::MusicPlanner planner({.runSeed = 1337, .candidateCount = 8, .sampleRate = kSampleRate});
    planner.setFormPlan(audio::MusicPlanner::makeFixtureM24V1Form());
    std::vector<std::uint64_t> allFrames;
    std::array<std::vector<std::uint64_t>, 4> roleFrames;
    for (const auto& p : phrases) {
        std::vector<audio::MusicIntentEvent> out;
        planner.planPhrase(p, *clock, 0, p.sectionIndex + 1ULL, out, nullptr);
        for (const auto& e : out) {
            allFrames.push_back(e.sampleFrame);
            roleFrames[static_cast<std::size_t>(e.role)].push_back(e.sampleFrame);
        }
    }
    r.plannedNoteOns = allFrames.size();
    auto uniqueSeconds = [](std::vector<std::uint64_t> frames) {
        std::sort(frames.begin(), frames.end());
        frames.erase(std::unique(frames.begin(), frames.end()), frames.end());
        std::vector<double> s;
        s.reserve(frames.size());
        for (const auto f : frames) s.push_back(static_cast<double>(f) / kSampleRate);
        return s;
    };
    const std::vector<double> refS = uniqueSeconds(allFrames);
    r.uniqueOnsetFrames = refS.size();
    std::array<std::vector<double>, 4> roleRefS;
    for (std::size_t k = 0; k < 4; ++k) {
        roleRefS[k] = uniqueSeconds(roleFrames[k]);
        r.roleUniqueOnsets[k] = roleRefS[k].size();
    }

    // Detection, timed as E1 (STFT and novelty separately)
    const auto t0 = std::chrono::steady_clock::now();
    const MagnitudeSpectrogram mag = stftMagnitude(mono);
    const auto t1 = std::chrono::steady_clock::now();
    const OnsetParams base{};
    const std::vector<double> nov = spectralFluxNovelty(mag, base.logGamma);
    const auto t2 = std::chrono::steady_clock::now();
    r.stftFrames = mag.frames;
    r.stftSeconds = std::chrono::duration<double>(t1 - t0).count();
    r.noveltySeconds = std::chrono::duration<double>(t2 - t1).count();

    r.allMatchE1 = true;
    for (std::size_t d = 0; d < kDeltas.size(); ++d) {
        OnsetParams p = base;
        p.delta = kDeltas[d];
        std::vector<double> est;
        for (const auto f : pickOnsetFrames(nov, p, kDescriptorsV1Stft, kSampleRate)) {
            est.push_back(frameCentreSeconds(f, kDescriptorsV1Stft, kSampleRate));
        }
        OnsetParityDelta& out = r.deltas[d];
        out.delta = kDeltas[d];
        out.detected = est.size();
        const OnsetScore greedy = matchOnsetsGreedyNearest(refS, est, kToleranceS);
        const OnsetScore maxc = matchOnsetsMaxCardinality(refS, est, kToleranceS);
        out.greedyPrecision = greedy.precision;
        out.greedyRecall = greedy.recall;
        out.greedyF = greedy.fMeasure;
        out.maxPrecision = maxc.precision;
        out.maxRecall = maxc.recall;
        out.maxF = maxc.fMeasure;
        std::vector<double> offsMs;
        for (const auto& m : greedy.pairs) offsMs.push_back(m.errorS * 1000.0);
        if (offsMs.empty()) offsMs.push_back(0.0);
        out.offsetMedianMs = percentileLinear(offsMs, 50.0);
        out.offsetP05Ms = percentileLinear(offsMs, 5.0);
        out.offsetP95Ms = percentileLinear(offsMs, 95.0);
        for (std::size_t k = 0; k < 4; ++k) {
            std::size_t hit = 0;
            for (const double t : roleRefS[k]) {
                const auto it = std::lower_bound(est.begin(), est.end(), t);
                const bool near = (it != est.end() && std::abs(*it - t) <= kToleranceS) ||
                                  (it != est.begin() && std::abs(*(it - 1) - t) <= kToleranceS);
                if (near) ++hit;
            }
            out.roleRecall[k] = roleRefS[k].empty() ? 0.0 : static_cast<double>(hit) / static_cast<double>(roleRefS[k].size());
        }
        out.e1Detected = kE1Detected[d];
        out.e1Precision = kE1Precision[d];
        out.e1Recall = kE1Recall[d];
        out.matchesE1 = out.detected == out.e1Detected && round3(out.greedyPrecision) == out.e1Precision &&
                        round3(out.greedyRecall) == out.e1Recall;
        r.allMatchE1 = r.allMatchE1 && out.matchesE1;
    }
    r.completed = true;

    if (writeFiles) {
        std::error_code ec;
        std::filesystem::create_directories(outputDir, ec);
        r.manifestFilePath = (std::filesystem::path(outputDir) / "onset_parity_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(r.manifestFilePath);
        r.manifestWriteSuccess = mf.is_open();
        if (r.manifestWriteSuccess) {
            mf << r.toJson();
            r.manifestWriteSuccess = mf.good();
        }
    }
    return r;
}

std::string OnsetParityResult::toJson() const {
    static constexpr std::array<const char*, 4> kRoles{"pulso", "corpo", "trama", "vettore"};
    std::ostringstream s;
    s << "{\n";
    s << "  \"manifest_version\": \"1.0\",\n";
    s << "  \"scenario\": \"onset_parity_v1\",\n";
    s << "  \"build_id\": \"" << CYMATICA_BUILD_ID << "\",\n";
    s << "  \"verdict\": \"" << (completed ? "MEASURED" : "NOT_RUN") << "\",\n";
    s << "  \"gate\": false,\n";
    s << "  \"descriptors_version\": \"descriptors_v1\",\n";
    s << "  \"reference_experiment\": \"E1, docs/research/experiments/cacp_methods/onset_report_2026-10-21.json\",\n";
    s << "  \"fixture_passed\": " << (fixturePassed ? "true" : "false") << ",\n";
    s << "  \"pcm_content_hash\": " << pcmContentHash << ",\n";
    s << "  \"analysed_frames\": " << analysedFrames << ",\n";
    s << "  \"planned_noteons\": " << plannedNoteOns << ",\n";
    s << "  \"unique_onset_frames\": " << uniqueOnsetFrames << ",\n";
    s << "  \"role_unique_onsets\": {";
    for (std::size_t k = 0; k < 4; ++k) s << "\"" << kRoles[k] << "\": " << roleUniqueOnsets[k] << (k < 3 ? ", " : "");
    s << "},\n";
    s << "  \"stft_frames\": " << stftFrames << ",\n";
    s << "  \"timing_s\": {\"stft\": " << num(stftSeconds) << ", \"novelty\": " << num(noveltySeconds) << "},\n";
    s << "  \"deltas\": [\n";
    for (std::size_t d = 0; d < deltas.size(); ++d) {
        const auto& x = deltas[d];
        s << "    {\"delta\": " << num(x.delta) << ", \"detected\": " << x.detected
          << ", \"greedy\": {\"precision\": " << num(x.greedyPrecision) << ", \"recall\": " << num(x.greedyRecall)
          << ", \"f\": " << num(x.greedyF) << "}"
          << ", \"max_cardinality\": {\"precision\": " << num(x.maxPrecision) << ", \"recall\": " << num(x.maxRecall)
          << ", \"f\": " << num(x.maxF) << "}"
          << ", \"offset_ms\": {\"median\": " << num(x.offsetMedianMs) << ", \"p05\": " << num(x.offsetP05Ms)
          << ", \"p95\": " << num(x.offsetP95Ms) << "}, \"role_recall\": {";
        for (std::size_t k = 0; k < 4; ++k) s << "\"" << kRoles[k] << "\": " << num(x.roleRecall[k]) << (k < 3 ? ", " : "");
        s << "}, \"e1\": {\"detected\": " << x.e1Detected << ", \"precision\": " << num(x.e1Precision)
          << ", \"recall\": " << num(x.e1Recall) << "}, \"matches_e1\": " << (x.matchesE1 ? "true" : "false") << "}"
          << (d + 1 < deltas.size() ? ",\n" : "\n");
    }
    s << "  ],\n";
    s << "  \"all_match_e1\": " << (allMatchE1 ? "true" : "false");
    if (!failureReason.empty()) s << ",\n  \"failure_reason\": \"" << failureReason << "\"";
    s << "\n}\n";
    return s.str();
}

} // namespace cymatica::cacp
