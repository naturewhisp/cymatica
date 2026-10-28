#include "onset.h"

#include <algorithm>
#include <cmath>

namespace cymatica::cacp {

namespace {

OnsetScore finalize(OnsetScore s) {
    s.matches = s.pairs.size();
    s.precision = s.estimates ? static_cast<double>(s.matches) / static_cast<double>(s.estimates) : 0.0;
    s.recall = s.references ? static_cast<double>(s.matches) / static_cast<double>(s.references) : 0.0;
    s.fMeasure = (s.precision + s.recall) > 0.0 ? 2.0 * s.precision * s.recall / (s.precision + s.recall) : 0.0;
    return s;
}

OnsetScore invalidScore(std::span<const double> referenceS, std::span<const double> estimateS) {
    OnsetScore s;
    s.status = AnalysisStatus::NonFiniteInput;
    s.references = referenceS.size();
    s.estimates = estimateS.size();
    return s; // no pair; P = R = F = 0 and the status says why
}

std::vector<double> sorted(std::span<const double> v) {
    std::vector<double> out(v.begin(), v.end());
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace

NoveltyCurve spectralFluxNovelty(const MagnitudeSpectrogram& mag, double logGamma) {
    NoveltyCurve curve;
    curve.status = mag.status;
    if (mag.status != AnalysisStatus::Ok) return curve;
    if (!std::isfinite(logGamma)) {
        curve.status = AnalysisStatus::InvalidConfig;
        return curve;
    }
    std::vector<double>& nov = curve.values;
    nov.assign(mag.frames, 0.0);
    if (mag.frames == 0) return curve;
    std::vector<double> prev(mag.bins), cur(mag.bins);
    for (std::size_t b = 0; b < mag.bins; ++b) prev[b] = std::log1p(logGamma * static_cast<double>(mag.at(0, b)));
    double maxV = 0.0;
    for (std::size_t k = 1; k < mag.frames; ++k) {
        double sum = 0.0;
        for (std::size_t b = 0; b < mag.bins; ++b) {
            cur[b] = std::log1p(logGamma * static_cast<double>(mag.at(k, b)));
            const double d = cur[b] - prev[b];
            if (d > 0.0) sum += d;
        }
        nov[k] = sum;
        maxV = (std::max)(maxV, sum);
        std::swap(prev, cur);
    }
    const double norm = (std::max)(maxV, 1e-12);
    for (auto& v : nov) v /= norm;
    if (!allFinite(std::span<const double>(nov))) { // defensive: overflow of a finite but huge magnitude
        curve.status = AnalysisStatus::NonFiniteInput;
        curve.values.clear();
    }
    return curve;
}

OnsetFrames pickOnsetFrames(std::span<const double> nov, const OnsetParams& p, const StftConfig& cfg,
                            std::uint32_t sampleRate) {
    OnsetFrames result;
    std::vector<std::size_t>& out = result.frames;
    if (cfg.hop == 0 || sampleRate == 0 || !std::isfinite(p.delta) || !std::isfinite(p.thresholdHalfWindowS) ||
        !std::isfinite(p.minSeparationS)) {
        result.status = AnalysisStatus::InvalidConfig;
        return result;
    }
    if (!allFinite(nov)) {
        result.status = AnalysisStatus::NonFiniteInput; // never a false "no onset"
        return result;
    }
    const std::size_t n = nov.size();
    if (n < 3) return result;
    const double framesPerSecond = static_cast<double>(sampleRate) / static_cast<double>(cfg.hop);
    const auto w = static_cast<std::ptrdiff_t>(std::llround(p.thresholdHalfWindowS * framesPerSecond));
    const auto sep = static_cast<std::size_t>(std::llround(p.minSeparationS * framesPerSecond));
    const double kernelLen = static_cast<double>(2 * w + 1);

    // Zero-padded centred moving average via prefix sums (numpy.convolve mode 'same')
    std::vector<double> prefix(n + 1, 0.0);
    for (std::size_t i = 0; i < n; ++i) prefix[i + 1] = prefix[i] + nov[i];
    auto threshold = [&](std::size_t i) {
        const std::ptrdiff_t lo = static_cast<std::ptrdiff_t>(i) - w;
        const std::ptrdiff_t hi = static_cast<std::ptrdiff_t>(i) + w + 1;
        const auto a = static_cast<std::size_t>(lo > 0 ? lo : 0);
        const auto b = static_cast<std::size_t>(hi < static_cast<std::ptrdiff_t>(n) ? hi : static_cast<std::ptrdiff_t>(n));
        return (prefix[b] - prefix[a]) / kernelLen + p.delta;
    };

    for (std::size_t i = 1; i + 1 < n; ++i) {
        if (!(nov[i] > nov[i - 1] && nov[i] >= nov[i + 1] && nov[i] > threshold(i))) continue;
        if (!out.empty() && i - out.back() < sep) {
            if (nov[i] > nov[out.back()]) out.back() = i;
            continue;
        }
        out.push_back(i);
    }
    return result;
}

OnsetDetection detectOnsetsSeconds(std::span<const float> mono, const OnsetParams& p, std::uint32_t sampleRate,
                                   const StftConfig& cfg) {
    OnsetDetection out;
    const MagnitudeSpectrogram mag = stftMagnitude(mono, cfg);
    const NoveltyCurve nov = spectralFluxNovelty(mag, p.logGamma);
    if (nov.status != AnalysisStatus::Ok) {
        out.status = nov.status;
        return out;
    }
    const OnsetFrames frames = pickOnsetFrames(nov.values, p, cfg, sampleRate);
    out.status = frames.status;
    out.timesS.reserve(frames.frames.size());
    for (const auto f : frames.frames) out.timesS.push_back(frameCentreSeconds(f, cfg, sampleRate));
    return out;
}

OnsetScore matchOnsetsMaxCardinality(std::span<const double> referenceS, std::span<const double> estimateS, double toleranceS) {
    if (!allFinite(referenceS) || !allFinite(estimateS) || !std::isfinite(toleranceS)) return invalidScore(referenceS, estimateS);
    const std::vector<double> ref = sorted(referenceS);
    const std::vector<double> est = sorted(estimateS);
    OnsetScore s;
    s.references = ref.size();
    s.estimates = est.size();
    std::size_t j = 0;
    for (std::size_t i = 0; i < ref.size() && j < est.size(); ++i) {
        while (j < est.size() && est[j] < ref[i] - toleranceS) ++j; // too early for this and every later reference
        if (j < est.size() && est[j] <= ref[i] + toleranceS) {
            s.pairs.push_back({i, j, est[j] - ref[i]});
            ++j;
        }
    }
    return finalize(std::move(s));
}

OnsetScore matchOnsetsGreedyNearest(std::span<const double> referenceS, std::span<const double> estimateS, double toleranceS) {
    if (!allFinite(referenceS) || !allFinite(estimateS) || !std::isfinite(toleranceS)) return invalidScore(referenceS, estimateS);
    const std::vector<double> ref = sorted(referenceS);
    const std::vector<double> est = sorted(estimateS);
    OnsetScore s;
    s.references = ref.size();
    s.estimates = est.size();
    std::vector<bool> used(est.size(), false);
    std::size_t j0 = 0;
    for (std::size_t i = 0; i < ref.size(); ++i) {
        while (j0 < est.size() && est[j0] < ref[i] - toleranceS) ++j0;
        std::size_t best = est.size();
        double bestD = toleranceS + 1.0;
        for (std::size_t j = j0; j < est.size() && est[j] <= ref[i] + toleranceS; ++j) {
            const double d = std::abs(est[j] - ref[i]);
            if (!used[j] && d < bestD) { best = j; bestD = d; }
        }
        if (best < est.size()) {
            used[best] = true;
            s.pairs.push_back({i, best, est[best] - ref[i]});
        }
    }
    return finalize(std::move(s));
}

} // namespace cymatica::cacp
