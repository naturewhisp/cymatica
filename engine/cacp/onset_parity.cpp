#include "onset_parity.h"

#include "analysis_status.h"
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
constexpr std::array<std::uint64_t, 4> kSectionBounds{0, 3072000, 5705142, 8777142};
constexpr std::array<const char*, 4> kRoles{"pulso", "corpo", "trama", "vettore"};

// Every value published by E1 (docs/research/experiments/cacp_methods/onset_report_2026-10-21.json)
struct E1Section {
    double precision, recall, f;
    std::size_t references, estimates;
};
struct E1Delta {
    std::size_t detected;
    double precision, recall, f;
    double offsetMedianMs, offsetP05Ms, offsetP95Ms;
    std::array<double, 4> roleRecall;
    std::array<E1Section, 3> sections;
};
constexpr std::array<E1Delta, 3> kE1{{
    {991, 0.920, 0.905, 0.912, -7.33, -11.12, 10.33, {1.0, 0.99, 0.762, 0.997},
     {{{0.841, 0.99, 0.909, 304, 358}, {1.0, 0.733, 0.846, 352, 258}, {0.939, 1.0, 0.968, 352, 375}}}},
    {730, 1.000, 0.724, 0.840, -8.26, -11.23, -2.67, {1.0, 0.99, 0.31, 0.984},
     {{{0.996, 0.734, 0.845, 304, 224}, {1.0, 0.71, 0.831, 352, 250}, {1.0, 0.727, 0.842, 352, 256}}}},
    {464, 1.000, 0.460, 0.630, -9.12, -11.60, -6.38, {1.0, 0.417, 0.022, 0.292},
     {{{1.0, 0.655, 0.791, 304, 199}, {1.0, 0.366, 0.536, 352, 129}, {1.0, 0.386, 0.557, 352, 136}}}},
}};
constexpr double kTol3 = 0.0005 + 1e-9; // half a unit of 3 decimals
constexpr double kTol2 = 0.005 + 1e-9;  // half a unit of 2 decimals

// numpy.percentile default ("linear")
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

std::vector<double> inRange(const std::vector<double>& v, double lo, double hi) {
    std::vector<double> out;
    for (const double t : v) if (t >= lo && t < hi) out.push_back(t);
    return out;
}

} // namespace

OnsetParityResult runOnsetParityV1(const std::string& outputDir, bool writeFiles) {
    OnsetParityResult r;
    auto finish = [&]() -> OnsetParityResult {
        r.status = (r.completed && r.inputValid) ? "MEASURED" : "INCONCLUSIVE";
        if (writeFiles) {
            std::error_code ec;
            std::filesystem::create_directories(outputDir, ec);
            r.manifestFilePath =
                (std::filesystem::path(outputDir) / "onset_parity_v1_manifest.json").lexically_normal().string();
            std::ofstream mf(r.manifestFilePath);
            r.manifestWriteSuccess = mf.is_open();
            if (r.manifestWriteSuccess) {
                mf << r.toJson();
                r.manifestWriteSuccess = mf.good();
            }
        }
        return r;
    };

    // Input: fixture rendered in-process, same PCM as fixture_probe_v1.wav
    std::vector<float> stereo;
    audio::ProbeAnalysisCapture capture{};
    capture.stereo = &stereo;
    const auto fixture = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &capture);
    r.fixturePassed = fixture.passed;
    r.pcmContentHash = fixture.pcmContentHash;
    const std::uint64_t contentEnd = fixture.contentEndFrame;
    if (contentEnd == 0 || stereo.size() < contentEnd * 2) {
        r.failureReason = "fixture render unavailable or shorter than the content";
        return finish();
    }
    // The parity is meaningful only on the validated fixture render (DIF-5b-03)
    r.inputValid = r.fixturePassed && allFinite(std::span<const float>(stereo.data(), static_cast<std::size_t>(contentEnd) * 2));
    if (!r.inputValid) r.failureReason = "fixture_probe_v1 did not pass or its PCM is not finite: input not valid for parity";

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
        return finish();
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
    const NoveltyCurve nov = spectralFluxNovelty(mag, base.logGamma);
    const auto t2 = std::chrono::steady_clock::now();
    r.stftFrames = mag.frames;
    r.stftSeconds = std::chrono::duration<double>(t1 - t0).count();
    r.noveltySeconds = std::chrono::duration<double>(t2 - t1).count();
    if (nov.status != AnalysisStatus::Ok) {
        r.failureReason = std::string("novelty status ") + analysisStatusName(nov.status);
        return finish();
    }

    r.allMatchE1 = true;
    for (std::size_t d = 0; d < kDeltas.size(); ++d) {
        OnsetParams p = base;
        p.delta = kDeltas[d];
        const OnsetFrames frames = pickOnsetFrames(nov.values, p, kDescriptorsV1Stft, kSampleRate);
        if (frames.status != AnalysisStatus::Ok) {
            r.failureReason = std::string("peak picking status ") + analysisStatusName(frames.status);
            r.allMatchE1 = false;
            return finish();
        }
        std::vector<double> est;
        for (const auto f : frames.frames) est.push_back(frameCentreSeconds(f, kDescriptorsV1Stft, kSampleRate));

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
        for (std::size_t s = 0; s < 3; ++s) {
            const double lo = static_cast<double>(kSectionBounds[s]) / kSampleRate;
            const double hi = static_cast<double>(kSectionBounds[s + 1]) / kSampleRate;
            const std::vector<double> rs = inRange(refS, lo, hi);
            const std::vector<double> es = inRange(est, lo, hi);
            const OnsetScore sc = matchOnsetsGreedyNearest(rs, es, kToleranceS);
            out.sections[s] = {sc.precision, sc.recall, sc.fMeasure, rs.size(), es.size()};
        }

        // Field-by-field comparison with E1 (DIF-5b-03)
        const E1Delta& e1 = kE1[d];
        std::ostringstream bad;
        auto check = [&](const char* name, double ours, double ref, double tol) {
            if (!(std::abs(ours - ref) <= tol)) bad << name << " " << num(ours) << " vs " << num(ref) << "; ";
        };
        auto checkCount = [&](const std::string& name, std::size_t ours, std::size_t ref) {
            if (ours != ref) bad << name << " " << ours << " vs " << ref << "; ";
        };
        checkCount("detected", out.detected, e1.detected);
        check("precision", out.greedyPrecision, e1.precision, kTol3);
        check("recall", out.greedyRecall, e1.recall, kTol3);
        check("f", out.greedyF, e1.f, kTol3);
        check("offset_median_ms", out.offsetMedianMs, e1.offsetMedianMs, kTol2);
        check("offset_p05_ms", out.offsetP05Ms, e1.offsetP05Ms, kTol2);
        check("offset_p95_ms", out.offsetP95Ms, e1.offsetP95Ms, kTol2);
        for (std::size_t k = 0; k < 4; ++k) {
            check((std::string("recall_") + kRoles[k]).c_str(), out.roleRecall[k], e1.roleRecall[k], kTol3);
        }
        for (std::size_t s = 0; s < 3; ++s) {
            const std::string tag = "section" + std::to_string(s) + "_";
            check((tag + "precision").c_str(), out.sections[s].precision, e1.sections[s].precision, kTol3);
            check((tag + "recall").c_str(), out.sections[s].recall, e1.sections[s].recall, kTol3);
            check((tag + "f").c_str(), out.sections[s].f, e1.sections[s].f, kTol3);
            checkCount(tag + "references", out.sections[s].references, e1.sections[s].references);
            checkCount(tag + "estimates", out.sections[s].estimates, e1.sections[s].estimates);
        }
        out.mismatchedFields = bad.str();
        out.matchesE1 = out.mismatchedFields.empty();
        r.allMatchE1 = r.allMatchE1 && out.matchesE1;
    }
    r.completed = true;
    return finish();
}

std::string OnsetParityResult::toJson() const {
    std::ostringstream s;
    s << "{\n";
    s << "  \"manifest_version\": \"1.1\",\n";
    s << "  \"scenario\": \"onset_parity_v1\",\n";
    s << "  \"build_id\": \"" << CYMATICA_BUILD_ID << "\",\n";
    s << "  \"verdict\": \"" << status << "\",\n";
    s << "  \"gate\": false,\n";
    s << "  \"descriptors_version\": \"descriptors_v1\",\n";
    s << "  \"reference_experiment\": \"E1, docs/research/experiments/cacp_methods/onset_report_2026-10-21.json\",\n";
    s << "  \"parity_tolerances\": {\"counts\": \"exact\", \"three_decimals\": 0.0005, \"offsets_ms_two_decimals\": 0.005},\n";
    s << "  \"completed\": " << (completed ? "true" : "false") << ",\n";
    s << "  \"input_valid\": " << (inputValid ? "true" : "false") << ",\n";
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
        s << "}, \"sections\": [";
        for (std::size_t sec = 0; sec < 3; ++sec) {
            const auto& q = x.sections[sec];
            s << "{\"precision\": " << num(q.precision) << ", \"recall\": " << num(q.recall) << ", \"f\": " << num(q.f)
              << ", \"references\": " << q.references << ", \"estimates\": " << q.estimates << "}" << (sec < 2 ? ", " : "");
        }
        s << "], \"matches_e1\": " << (x.matchesE1 ? "true" : "false") << ", \"mismatched_fields\": \""
          << x.mismatchedFields << "\"}" << (d + 1 < deltas.size() ? ",\n" : "\n");
    }
    s << "  ],\n";
    s << "  \"all_match_e1\": " << (allMatchE1 ? "true" : "false");
    if (!failureReason.empty()) s << ",\n  \"failure_reason\": \"" << failureReason << "\"";
    s << "\n}\n";
    return s.str();
}

} // namespace cymatica::cacp
