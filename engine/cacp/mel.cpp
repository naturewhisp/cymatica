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
    if (mag.bins != fb.bins || fb.bands == 0 || !(dbRange > 0.0)) return out;
    out.frames = mag.frames;
    out.bands = fb.bands;
    std::vector<double> db(out.frames * out.bands);
    double maxDb = -std::numeric_limits<double>::infinity();
    for (std::size_t k = 0; k < mag.frames; ++k) {
        for (std::size_t m = 0; m < fb.bands; ++m) {
            double acc = 0.0;
            const double* w = &fb.weights[m * fb.bins];
            for (std::size_t b = 0; b < fb.bins; ++b) {
                if (w[b] != 0.0) acc += w[b] * static_cast<double>(mag.at(k, b));
            }
            const double v = 20.0 * std::log10((std::max)(acc, 1e-10));
            db[k * out.bands + m] = v;
            maxDb = (std::max)(maxDb, v);
        }
    }
    out.normalized.resize(db.size());
    for (std::size_t i = 0; i < db.size(); ++i) {
        out.normalized[i] = static_cast<float>(std::clamp((db[i] - (maxDb - dbRange)) / dbRange, 0.0, 1.0));
    }
    return out;
}

} // namespace cymatica::cacp
