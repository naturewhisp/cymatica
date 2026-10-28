#include "mel.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace cymatica::cacp {

double hzToMelHtk(double hz) noexcept { return 2595.0 * std::log10(1.0 + hz / 700.0); }
double melToHzHtk(double mel) noexcept { return 700.0 * (std::pow(10.0, mel / 2595.0) - 1.0); }

MelFilterbank melFilterbank(const MelConfig& mel, const StftConfig& stft, std::uint32_t sampleRate) {
    MelFilterbank fb;
    if (mel.bands == 0 || stft.fftSize == 0 || sampleRate == 0 || !(mel.fMaxHz > mel.fMinHz)) return fb;
    fb.bands = mel.bands;
    fb.bins = stft.fftSize / 2 + 1;
    fb.weights.assign(fb.bands * fb.bins, 0.0);

    // bands + 2 edge frequencies equally spaced in mel
    const double mLo = hzToMelHtk(mel.fMinHz);
    const double mHi = hzToMelHtk(mel.fMaxHz);
    std::vector<double> edges(mel.bands + 2);
    for (std::size_t i = 0; i < edges.size(); ++i) {
        const double m = mLo + (mHi - mLo) * static_cast<double>(i) / static_cast<double>(edges.size() - 1);
        edges[i] = melToHzHtk(m);
    }
    fb.centreHz.assign(edges.begin() + 1, edges.end() - 1);

    for (std::size_t m = 0; m < mel.bands; ++m) {
        const double lo = edges[m], ce = edges[m + 1], hi = edges[m + 2];
        for (std::size_t b = 0; b < fb.bins; ++b) {
            const double f = static_cast<double>(b) * static_cast<double>(sampleRate) / static_cast<double>(stft.fftSize);
            const double w = (std::min)((f - lo) / (ce - lo), (hi - f) / (hi - ce));
            fb.weights[m * fb.bins + b] = (std::max)(w, 0.0);
        }
    }
    return fb;
}

MelSpectrogram melSpectrogramNormalized(const MagnitudeSpectrogram& mag, const MelFilterbank& fb, double dbRange) {
    MelSpectrogram out;
    if (mag.status != AnalysisStatus::Ok) {
        out.status = mag.status;
        return out;
    }
    if (mag.bins != fb.bins || fb.bands == 0 || !(dbRange > 0.0) || !std::isfinite(dbRange)) {
        out.status = AnalysisStatus::InvalidConfig;
        return out;
    }
    out.frames = mag.frames;
    out.bands = fb.bands;
    std::vector<double> acc(out.frames * out.bands, 0.0);
    double maxAcc = 0.0;
    bool nonFinite = false;
    for (std::size_t k = 0; k < mag.frames; ++k) {
        for (std::size_t m = 0; m < fb.bands; ++m) {
            double a = 0.0;
            const double* w = &fb.weights[m * fb.bins];
            for (std::size_t b = 0; b < fb.bins; ++b) {
                if (w[b] != 0.0) a += w[b] * static_cast<double>(mag.at(k, b));
            }
            acc[k * out.bands + m] = a;
            if (!std::isfinite(a)) nonFinite = true; // std::max would silently skip a NaN
            maxAcc = (std::max)(maxAcc, a);
        }
    }
    if (nonFinite || !std::isfinite(maxAcc)) {
        out.status = AnalysisStatus::NonFiniteInput;
        out.frames = 0;
        return out;
    }
    out.normalized.assign(acc.size(), 0.0f);
    if (maxAcc == 0.0) {
        out.status = AnalysisStatus::ExactSilence; // no energy: all cells stay 0, never the maximum
        return out;
    }
    if (maxAcc <= kMelMagnitudeFloor) {
        out.status = AnalysisStatus::BelowNumericalFloor; // indistinguishable from the floor of the dB scale
        return out;
    }
    const double maxDb = 20.0 * std::log10(maxAcc);
    out.maxDb = maxDb;
    for (std::size_t i = 0; i < acc.size(); ++i) {
        const double v = 20.0 * std::log10((std::max)(acc[i], kMelMagnitudeFloor));
        out.normalized[i] = static_cast<float>(std::clamp((v - (maxDb - dbRange)) / dbRange, 0.0, 1.0));
    }
    return out;
}

} // namespace cymatica::cacp
