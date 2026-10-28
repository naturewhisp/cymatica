#include "loudness.h"

#include <algorithm>
#include <cmath>

namespace cymatica::cacp {

namespace {

// Direct-form-I biquad: y = b0 x + b1 x1 + b2 x2 - a1 y1 - a2 y2 (BS.1770 Figure 3)
struct Biquad {
    double b0, b1, b2, a1, a2;
    double x1{0.0}, x2{0.0}, y1{0.0}, y2{0.0};
    double process(double x) noexcept {
        const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = x;
        y2 = y1; y1 = y;
        return y;
    }
};

// ITU-R BS.1770-5 Table 1 (stage 1, spherical head pre-filter) and Table 2 (stage 2, RLB high-pass), 48 kHz
Biquad stage1() { return {1.53512485958697, -2.69169618940638, 1.19839281085285, -1.69065929318241, 0.73248077421585}; }
Biquad stage2() { return {1.0, -2.0, 1.0, -1.99004745483398, 0.99007225036621}; }

std::optional<double> loudnessOf(double sumMeanSquares) noexcept {
    if (!(sumMeanSquares > 0.0)) return std::nullopt;
    const double l = -0.691 + 10.0 * std::log10(sumMeanSquares);
    if (!std::isfinite(l)) return std::nullopt;
    return l;
}

} // namespace

LoudnessResult measureLoudnessStereo(std::span<const float> interleavedStereo, std::uint32_t sampleRate,
                                     const LoudnessConfig& cfg) {
    LoudnessResult r;
    r.seriesStepS = cfg.seriesStepS;
    if (sampleRate != 48000) { // coefficients are specified at 48 kHz only
        r.status = AnalysisStatus::InvalidConfig;
        return r;
    }
    r.supported = true;
    if (!allFinite(interleavedStereo)) {
        r.status = AnalysisStatus::NonFiniteInput;
        return r;
    }

    const std::size_t frames = interleavedStereo.size() / 2;
    if (frames == 0) return r;

    // K-weighted squares, summed over L and R (G = 1.0), as a prefix sum over frames
    Biquad l1 = stage1(), l2 = stage2(), r1 = stage1(), r2 = stage2();
    std::vector<double> prefix(frames + 1, 0.0);
    for (std::size_t i = 0; i < frames; ++i) {
        const double yl = l2.process(l1.process(static_cast<double>(interleavedStereo[2 * i])));
        const double yr = r2.process(r1.process(static_cast<double>(interleavedStereo[2 * i + 1])));
        prefix[i + 1] = prefix[i] + yl * yl + yr * yr;
    }
    auto meanSquareSum = [&](std::size_t start, std::size_t length) {
        return (prefix[start + length] - prefix[start]) / static_cast<double>(length);
    };

    const double fs = static_cast<double>(sampleRate);
    const auto step = static_cast<std::size_t>(std::llround(cfg.seriesStepS * fs));
    auto series = [&](double windowS, std::vector<std::optional<double>>& out, std::optional<double>& maxOut) {
        const auto window = static_cast<std::size_t>(std::llround(windowS * fs));
        if (window == 0 || step == 0 || frames < window) return;
        for (std::size_t end = window; end <= frames; end += step) {
            const auto v = loudnessOf(meanSquareSum(end - window, window));
            out.push_back(v);
            if (v && (!maxOut || *v > *maxOut)) maxOut = v;
        }
    };
    series(cfg.momentaryWindowS, r.momentaryLufs, r.maxMomentaryLufs);
    series(cfg.shortTermWindowS, r.shortTermLufs, r.maxShortTermLufs);

    // Gated integrated loudness (BS.1770-5 §2.8): blocks of T_g with step T_g (1 - overlap)
    const auto block = static_cast<std::size_t>(std::llround(cfg.blockS * fs));
    const auto blockStep = static_cast<std::size_t>(std::llround(cfg.blockS * (1.0 - cfg.blockOverlap) * fs));
    if (block == 0 || blockStep == 0 || frames < block) return r;
    std::vector<double> z; // per-block sum of channel mean squares
    for (std::size_t start = 0; start + block <= frames; start += blockStep) {
        z.push_back(meanSquareSum(start, block));
    }
    r.blocks = z.size();

    double absSum = 0.0;
    std::size_t absCount = 0;
    for (const double zj : z) {
        const auto lj = loudnessOf(zj);
        if (lj && *lj > cfg.absoluteGateLufs) { absSum += zj; ++absCount; }
    }
    r.blocksAboveAbsoluteGate = absCount;
    if (absCount == 0) return r;
    const auto absLoudness = loudnessOf(absSum / static_cast<double>(absCount));
    if (!absLoudness) return r;
    const double relativeGate = *absLoudness + cfg.relativeGateLu;

    double relSum = 0.0;
    std::size_t relCount = 0;
    for (const double zj : z) {
        const auto lj = loudnessOf(zj);
        if (lj && *lj > cfg.absoluteGateLufs && *lj > relativeGate) { relSum += zj; ++relCount; }
    }
    r.blocksAboveRelativeGate = relCount;
    if (relCount > 0) r.integratedLufs = loudnessOf(relSum / static_cast<double>(relCount));
    return r;
}

} // namespace cymatica::cacp
