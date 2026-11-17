#include "discontinuity.h"

#include "acao02_calibration.h"
#include "audio_probe.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::size_t kFixtureCaptureFrames = 8'900'000;
constexpr std::array<double, 3> kInjectDbfs{-60.0, -40.0, -20.0};
constexpr std::size_t kInjections = 50;
constexpr std::uint64_t kInjectionSeed = 0xC11C5EEDULL;
constexpr std::size_t kStepFrames = 480; // 10 ms

std::uint64_t splitmix(std::uint64_t& s) {
    std::uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// Levinson-Durbin on autocorrelation r[0..p]: predictor a[1..p] with xhat[n] = sum a[k] x[n-k]
std::array<double, kLpcOrder + 1> levinson(const std::array<double, kLpcOrder + 1>& r) {
    std::array<double, kLpcOrder + 1> a{};
    if (!(r[0] > 0.0)) return a;
    std::array<double, kLpcOrder + 1> tmp{};
    double err = r[0];
    for (std::size_t i = 1; i <= kLpcOrder; ++i) {
        double acc = r[i];
        for (std::size_t j = 1; j < i; ++j) acc -= a[j] * r[i - j];
        const double k = acc / err;
        tmp = a;
        a[i] = k;
        for (std::size_t j = 1; j < i; ++j) a[j] = tmp[j] - k * tmp[i - j];
        err *= (1.0 - k * k);
        if (!(err > 0.0)) break;
    }
    return a;
}

std::vector<std::size_t> fuse(std::vector<std::size_t> frames) {
    std::sort(frames.begin(), frames.end());
    std::vector<std::size_t> out;
    std::size_t last = 0;
    bool have = false;
    for (const auto f : frames) {
        if (have && f - last <= kMergeFrames) {
            last = f; // chain: a cluster continues while candidates stay within 2 ms of each other
            continue;
        }
        out.push_back(f);
        last = f;
        have = true;
    }
    return out;
}

std::vector<float> channelOf(std::span<const float> interleaved, std::size_t ch) {
    std::vector<float> out(interleaved.size() / 2);
    for (std::size_t i = 0; i < out.size(); ++i) out[i] = interleaved[2 * i + ch];
    return out;
}

double recallNear(const std::vector<std::size_t>& candidates, const std::vector<std::size_t>& injected) {
    std::size_t found = 0;
    for (const auto f : injected) {
        const auto it = std::lower_bound(candidates.begin(), candidates.end(), f > kMergeFrames ? f - kMergeFrames : 0);
        if (it != candidates.end() && *it <= f + kMergeFrames) ++found;
    }
    return injected.empty() ? 0.0 : static_cast<double>(found) / static_cast<double>(injected.size());
}

} // namespace

std::vector<std::size_t> discontinuityCandidates(std::span<const float> x) {
    std::vector<std::size_t> cand;
    const std::size_t n = x.size();
    std::array<double, kLpcBlock> w{};
    constexpr double kPi = 3.14159265358979323846;
    for (std::size_t i = 0; i < kLpcBlock; ++i) w[i] = 0.5 - 0.5 * std::cos(2.0 * kPi * (static_cast<double>(i) + 0.5) / kLpcBlock);
    std::vector<double> e(kLpcBlock), absE(kLpcBlock), xw(kLpcBlock);
    for (std::size_t b0 = 0; b0 < n; b0 += kLpcBlock) {
        const std::size_t len = (std::min)(kLpcBlock, n - b0);
        for (std::size_t i = 0; i < kLpcBlock; ++i) xw[i] = i < len ? w[i] * static_cast<double>(x[b0 + i]) : 0.0;
        std::array<double, kLpcOrder + 1> r{};
        for (std::size_t k = 0; k <= kLpcOrder; ++k) {
            double acc = 0.0;
            for (std::size_t i = k; i < kLpcBlock; ++i) acc += xw[i] * xw[i - k];
            r[k] = acc;
        }
        const auto a = levinson(r);
        for (std::size_t i = 0; i < len; ++i) {
            const std::size_t t = b0 + i;
            double pred = 0.0;
            for (std::size_t k = 1; k <= kLpcOrder && k <= t; ++k) pred += a[k] * static_cast<double>(x[t - k]);
            e[i] = static_cast<double>(x[t]) - pred;
            absE[i] = std::abs(e[i]);
        }
        std::vector<double> sorted(absE.begin(), absE.begin() + static_cast<std::ptrdiff_t>(len));
        std::nth_element(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(len / 2), sorted.end());
        const double scale = (std::max)(1.4826 * sorted[len / 2], kScaleFloor);
        for (std::size_t i = 0; i < len; ++i) {
            // The first kLpcOrder samples of a buffer have no full history: no residual is defined there
            if (b0 + i >= kLpcOrder && absE[i] / scale > kResidualThreshold) cand.push_back(b0 + i);
        }
    }
    return fuse(std::move(cand));
}

std::vector<std::size_t> discontinuityCandidatesStereo(std::span<const float> interleaved) {
    auto l = discontinuityCandidates(channelOf(interleaved, 0));
    const auto r = discontinuityCandidates(channelOf(interleaved, 1));
    l.insert(l.end(), r.begin(), r.end());
    return fuse(std::move(l));
}

std::vector<std::size_t> differentialCandidates(std::span<const float> a, std::span<const float> b) {
    if (a.size() != b.size()) return {};
    std::vector<float> d(a.size());
    for (std::size_t i = 0; i < a.size(); ++i) d[i] = b[i] - a[i];
    return discontinuityCandidatesStereo(d);
}

CandidateCategory annotateCandidate(std::size_t frame, std::span<const AppliedEventRecord> applied, int roleFilter) {
    std::size_t bestDist = std::numeric_limits<std::size_t>::max();
    CandidateCategory cat = CandidateCategory::Unexplained;
    for (const auto& rec : applied) {
        if (rec.disposition != EventDisposition::Applied) continue;
        if (roleFilter >= 0 && static_cast<int>(rec.id.role) != roleFilter) continue;
        const std::size_t f = static_cast<std::size_t>(rec.id.sampleFrame);
        const std::size_t d = f > frame ? f - frame : frame - f;
        if (d > kAnnotateFrames || d >= bestDist) continue;
        bestDist = d;
        cat = rec.id.type == audio::AudioEventType::NoteOn    ? CandidateCategory::Attack
              : rec.id.type == audio::AudioEventType::NoteOff ? CandidateCategory::Release
                                                               : CandidateCategory::OtherEvent;
    }
    return cat;
}

DiscontinuityResult runDiscontinuityV1(const std::string& outputDir, bool writeFiles) {
    auto analyse = [&](DiscontinuityResult& r, bool withValidation) {
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
        r.goldenPcmStatus = probe.goldenPcmStatus;
        const CaptureCheck c{true, probe.passed, probe.failureReason, static_cast<std::size_t>(probe.requestedSampleFrames),
                             cap.frames(), cap.overflowFrames(), appliedOverflow};
        const std::string invalid = captureInvalidReason(c);
        if (!invalid.empty()) {
            r.status = "INCONCLUSIVE";
            r.reasons.push_back("invalid capture: " + invalid);
            return;
        }
        std::sort(applied.begin(), applied.end(), [](const auto& x, const auto& y) { return x.id.sampleFrame < y.id.sampleFrame; });
        for (std::size_t s = 0; s < 5; ++s) {
            const std::span<const float> buf = s < 4 ? cap.stem(s) : std::span<const float>(stereo);
            for (const auto f : discontinuityCandidatesStereo(buf)) {
                const auto cat = annotateCandidate(f, applied, s < 4 ? static_cast<int>(s) : -1);
                ++r.table[s][static_cast<std::size_t>(cat)];
                if (cat == CandidateCategory::Unexplained && r.unexplainedFrames[s].size() < 50) r.unexplainedFrames[s].push_back(f);
            }
        }
        if (!withValidation) {
            r.status = "MEASURED";
            return;
        }
        // W1: impulses and 10 ms steps injected into a copy of the mix, at seeded positions 50 ms apart
        const std::size_t frames = stereo.size() / 2;
        std::vector<std::size_t> positions;
        std::uint64_t seed = kInjectionSeed;
        while (positions.size() < kInjections && frames > 3 * kFs) {
            const std::size_t f = kFs + static_cast<std::size_t>(splitmix(seed) % (frames - 2 * kFs));
            bool far = true;
            for (const auto p : positions) far = far && (f > p ? f - p : p - f) > 2400;
            if (far) positions.push_back(f);
        }
        std::sort(positions.begin(), positions.end());
        for (std::size_t a = 0; a < kInjectDbfs.size(); ++a) {
            const float amp = static_cast<float>(std::pow(10.0, kInjectDbfs[a] / 20.0));
            std::vector<float> imp = stereo, step = stereo;
            for (const auto p : positions) {
                imp[2 * p] += amp;
                imp[2 * p + 1] += amp;
                for (std::size_t k = 0; k < kStepFrames && p + k < frames; ++k) {
                    step[2 * (p + k)] += amp;
                    step[2 * (p + k) + 1] += amp;
                }
            }
            r.impulseRecall[a] = recallNear(discontinuityCandidatesStereo(imp), positions);
            r.stepRecall[a] = recallNear(discontinuityCandidatesStereo(step), positions);
        }
        // W2: 440 Hz sine at -6 dBFS, 10 s, 50 ms ramps
        {
            std::vector<float> sine(2 * 10 * kFs);
            constexpr double kPi = 3.14159265358979323846;
            const std::size_t ramp = kFs / 20, total = 10 * kFs;
            for (std::size_t i = 0; i < total; ++i) {
                double g = 0.5;
                if (i < ramp) g *= static_cast<double>(i) / ramp;
                if (i >= total - ramp) g *= static_cast<double>(total - i) / ramp;
                const float v = static_cast<float>(g * std::sin(2.0 * kPi * 440.0 * static_cast<double>(i) / kFs));
                sine[2 * i] = v;
                sine[2 * i + 1] = v;
            }
            r.sineCandidates = discontinuityCandidatesStereo(sine).size();
        }
        // W5: A2 on identical renders and with one -40 dBFS impulse at frame 1,000,000
        {
            r.w5IdenticalZero = differentialCandidates(stereo, stereo).empty();
            std::vector<float> b = stereo;
            b[2 * 1'000'000] += 0.01f;
            const auto d = differentialCandidates(stereo, b);
            r.w5InjectedFound = d.size() == 1 && d[0] == 1'000'000;
        }
        r.status = "MEASURED";
    };

    DiscontinuityResult r;
    analyse(r, true);
    DiscontinuityResult again;
    analyse(again, false);
    r.repeatIdentical = again.tableJson() == r.tableJson();
    if (!r.repeatIdentical) {
        r.reasons.emplace_back("repeat run differs");
        r.status = "INCONCLUSIVE";
    }
    // Pre-registered validation of the A1 tool (W1-W5): a tool that fails it never yields a MEASURED table
    if (r.status == "MEASURED") {
        if (!(r.impulseRecall[2] == 1.0 && r.stepRecall[2] == 1.0)) r.reasons.emplace_back("W1: recall at -20 dBFS below 1.0");
        if (!(r.impulseRecall[1] >= 0.9 && r.stepRecall[1] >= 0.9)) r.reasons.emplace_back("W1: recall at -40 dBFS below 0.9");
        if (!(r.impulseRecall[0] <= r.impulseRecall[1] && r.impulseRecall[1] <= r.impulseRecall[2] &&
              r.stepRecall[0] <= r.stepRecall[1] && r.stepRecall[1] <= r.stepRecall[2])) {
            r.reasons.emplace_back("W1: recall not monotone in amplitude");
        }
        if (r.sineCandidates != 0) r.reasons.emplace_back("W2: candidates on a pure sine");
        if (r.table[0][static_cast<std::size_t>(CandidateCategory::Unexplained)] != 0) r.reasons.emplace_back("W3: unexplained candidates on the Pulso stem");
        if (!(r.w5IdenticalZero && r.w5InjectedFound)) r.reasons.emplace_back("W5: differential check failed");
        if (!r.reasons.empty()) r.status = "TOOL_NOT_VALIDATED";
    }
    if (writeFiles) {
        r.manifestFilePath = (std::filesystem::path(outputDir) / "discontinuity_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(r.manifestFilePath);
        r.manifestWriteSuccess = mf.is_open();
        if (r.manifestWriteSuccess) {
            mf << r.toJson();
            r.manifestWriteSuccess = mf.good();
        }
    }
    return r;
}

std::string DiscontinuityResult::tableJson() const {
    json t = json::object();
    const std::array<const char*, 5> names{"pulso", "corpo", "trama", "vettore", "mix"};
    for (std::size_t s = 0; s < 5; ++s) {
        json row = json::object();
        for (std::size_t c = 0; c < 4; ++c) row[kCandidateCategoryNames[c]] = table[s][c];
        row["unexplained_frames_first50"] = unexplainedFrames[s];
        t[names[s]] = row;
    }
    return t.dump(2);
}

std::string DiscontinuityResult::toJson() const {
    json j;
    j["scenario"] = "discontinuity_v1";
    j["status"] = status;
    j["reasons"] = reasons;
    j["preregistration"] = "docs/research/M2_5_Pkg2_Discontinuity_Detector_Preregistration.md";
    j["detector"] = {{"lpc_order", kLpcOrder}, {"block", kLpcBlock}, {"threshold", kResidualThreshold}, {"merge_ms", 2},
                     {"annotate_ms", 5}};
    j["table"] = json::parse(tableJson());
    j["validation"] = {{"W1_inject_dbfs", kInjectDbfs},
                       {"W1_impulse_recall", impulseRecall},
                       {"W1_step_recall", stepRecall},
                       {"W2_sine_candidates", sineCandidates},
                       {"W4_repeat_identical", repeatIdentical},
                       {"W5_identical_zero", w5IdenticalZero},
                       {"W5_injected_found", w5InjectedFound}};
    j["golden_pcm_status"] = goldenPcmStatus;
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
