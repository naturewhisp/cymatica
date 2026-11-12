// CP-1, early part of R-M25-3 (pre-registration docs/research/CP1_RM25_3_Early_Preregistration.md): offline prototypes
// that measure cost and aliasing of saturation, supersaw, LFO on the SVF cutoff and denormal tails. Nothing here enters
// the engine. Tagged [perf]: excluded from the headless presets and the CI; results go to artifacts/rm25_3/results.json.
#include <catch2/catch_test_macros.hpp>

#include "fft.h"
#include "synth_dsp.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <complex>
#include <filesystem>
#include <fstream>
#include <functional>
#include <vector>
#include <xmmintrin.h>
#include <pmmintrin.h>

using namespace cymatica::audio;

namespace {

constexpr double kFs = 48000.0;
constexpr std::size_t kTimingSamples = 480000; // 10 s
constexpr int kRepetitions = 5;
volatile float gSink = 0.0f;

// Median over the repetitions of ns per call of `fn` (which processes `samples` samples)
double medianNsPerSample(const std::function<float(std::size_t)>& fn, std::size_t samples = kTimingSamples) {
    std::vector<double> runs;
    for (int r = 0; r < kRepetitions; ++r) {
        const auto t0 = std::chrono::steady_clock::now();
        const float s = fn(samples);
        const auto t1 = std::chrono::steady_clock::now();
        gSink = gSink + s;
        runs.push_back(std::chrono::duration<double, std::nano>(t1 - t0).count() / static_cast<double>(samples));
    }
    std::sort(runs.begin(), runs.end());
    return runs[runs.size() / 2];
}

// ---- A. saturation tanh(drive * x) ----
constexpr double kDrive = 4.0;
double logCosh(double z) {
    const double a = std::abs(z);
    return a + std::log1p(std::exp(-2.0 * a)) - std::log(2.0);
}
// First-order antiderivative antialiasing of tanh(kDrive x) (Parker et al. 2016; Bilbao et al. 2017)
struct Adaa1 {
    double prev{0.0};
    double operator()(double x) {
        const double dx = x - prev;
        double y;
        if (std::abs(dx) < 1e-6) {
            y = std::tanh(kDrive * 0.5 * (x + prev));
        } else {
            y = (logCosh(kDrive * x) - logCosh(kDrive * prev)) / (kDrive * dx);
        }
        prev = x;
        return y;
    }
};

// Windowed-sinc lowpass (Blackman), cutoff `fc` as a fraction of the sample rate, odd length
std::vector<double> lowpass(std::size_t taps, double fc) {
    std::vector<double> h(taps);
    const double m = static_cast<double>(taps - 1);
    double sum = 0.0;
    for (std::size_t n = 0; n < taps; ++n) {
        const double k = static_cast<double>(n) - m / 2.0;
        const double sinc = k == 0.0 ? 2.0 * fc : std::sin(2.0 * 3.141592653589793 * fc * k) / (3.141592653589793 * k);
        const double w = 0.42 - 0.5 * std::cos(2.0 * 3.141592653589793 * n / m) + 0.08 * std::cos(4.0 * 3.141592653589793 * n / m);
        h[n] = sinc * w;
        sum += h[n];
    }
    for (auto& v : h) v /= sum;
    return h;
}

// Oversampled tanh: zero-stuff by M, lowpass, nonlinearity, lowpass, decimate (direct-form FIR: an upper bound on cost)
std::vector<double> oversampledTanh(const std::vector<double>& x, int M) {
    const auto h = lowpass(static_cast<std::size_t>(64 * M - 1), 0.5 / M * 0.9);
    const std::size_t L = h.size();
    std::vector<double> up(x.size() * M, 0.0);
    for (std::size_t i = 0; i < x.size(); ++i) up[i * M] = x[i] * M;
    std::vector<double> y(up.size(), 0.0);
    for (std::size_t n = 0; n < up.size(); ++n) {
        double acc = 0.0;
        for (std::size_t k = 0; k < L && k <= n; ++k) acc += h[k] * up[n - k];
        y[n] = std::tanh(kDrive * acc);
    }
    std::vector<double> out(x.size(), 0.0);
    for (std::size_t i = 0; i < x.size(); ++i) {
        const std::size_t n = i * M;
        double acc = 0.0;
        for (std::size_t k = 0; k < L && k <= n; ++k) acc += h[k] * y[n - k];
        out[i] = acc;
    }
    return out;
}

// Alias-to-signal ratio in dB of a saturated sine at f0: power within +-3 bins of the true harmonics below Nyquist
// against everything else except DC (Blackman-Harris window, N points, after a warm-up)
double aliasToSignalDb(const std::vector<double>& y, double f0, long halfBand) {
    constexpr std::size_t N = 65536;
    const std::size_t start = y.size() - N;
    std::vector<std::complex<double>> X(N);
    for (std::size_t n = 0; n < N; ++n) {
        const double t = 2.0 * 3.141592653589793 * n / (N - 1);
        const double w = 0.35875 - 0.48829 * std::cos(t) + 0.14128 * std::cos(2 * t) - 0.01168 * std::cos(3 * t);
        X[n] = y[start + n] * w;
    }
    cymatica::cacp::FftPlan(N).transform(X);
    std::vector<bool> harmonic(N / 2 + 1, false);
    for (double f = f0; f < kFs / 2.0; f += f0) {
        const auto b = static_cast<long>(std::lround(f / kFs * N));
        for (long d = -halfBand; d <= halfBand; ++d) {
            if (b + d > 0 && b + d <= static_cast<long>(N / 2)) harmonic[static_cast<std::size_t>(b + d)] = true;
        }
    }
    double sig = 0.0, alias = 0.0;
    for (std::size_t k = 4; k <= N / 2; ++k) (harmonic[k] ? sig : alias) += std::norm(X[k]);
    return 10.0 * std::log10(alias / sig);
}

// TPT SVF lowpass without the subnormal flush (copy of the engine filter for the denormal measure)
struct SvfNoFlush {
    float s1{0.0f}, s2{0.0f};
    float process(float x, float cutoffHz, float q) {
        const float R = 1.0f / (2.0f * q);
        const float g = std::tan(kPi * cutoffHz / static_cast<float>(kFs));
        const float a1 = 1.0f / (1.0f + g * (g + 2.0f * R));
        const float a2 = g * a1, a3 = g * a2;
        const float v3 = x - s2;
        const float v1 = a1 * s1 + a2 * v3;
        const float v2 = s2 + a2 * s1 + a3 * v3;
        s1 = 2.0f * v1 - s1;
        s2 = 2.0f * v2 - s2;
        return v2;
    }
};

} // namespace

TEST_CASE("R-M25-3 early: cost and aliasing of saturation, supersaw, LFO and denormals", "[perf][rm25_3]") {
    using nlohmann::json;
    json out;
    out["preregistration"] = "docs/research/CP1_RM25_3_Early_Preregistration.md";
    out["unit"] = "median ns per sample over 5 repetitions of 10 s at 48 kHz, Release, one thread";

    // ---- A. saturation ----
    const double f0 = 1244.5;
    std::vector<double> sine(65536 + 8192);
    for (std::size_t n = 0; n < sine.size(); ++n) sine[n] = std::sin(2.0 * 3.141592653589793 * f0 * n / kFs);
    std::vector<double> naive(sine.size()), adaa(sine.size());
    Adaa1 a1;
    for (std::size_t n = 0; n < sine.size(); ++n) {
        naive[n] = std::tanh(kDrive * sine[n]);
        adaa[n] = a1(sine[n]);
    }
    const auto os2 = oversampledTanh(sine, 2);
    const auto os4 = oversampledTanh(sine, 4);
    json sat;
    // +-3 bins as pre-registered; +-6 bins as declared deviation (the Blackman-Harris main lobe is +-4 bins wide)
    for (const long hb : {3L, 6L}) {
        sat["alias_to_signal_db_band_" + std::to_string(hb)] = {
            {"naive", aliasToSignalDb(naive, f0, hb)}, {"adaa1", aliasToSignalDb(adaa, f0, hb)},
            {"oversampled_2x", aliasToSignalDb(os2, f0, hb)}, {"oversampled_4x", aliasToSignalDb(os4, f0, hb)}};
    }
    sat["ns_per_sample"] = {
        {"naive", medianNsPerSample([](std::size_t n) {
             float acc = 0.0f;
             for (std::size_t i = 0; i < n; ++i) acc += std::tanh(4.0f * std::sin(0.163f * static_cast<float>(i)));
             return acc;
         })},
        {"adaa1", medianNsPerSample([](std::size_t n) {
             Adaa1 a;
             double acc = 0.0;
             for (std::size_t i = 0; i < n; ++i) acc += a(std::sin(0.163 * static_cast<double>(i)));
             return static_cast<float>(acc);
         })},
        // n input samples processed, time divided by n (fixed: the first version divided twice)
        {"oversampled_2x_direct_fir", medianNsPerSample([&](std::size_t n) {
             std::vector<double> x(n);
             for (std::size_t i = 0; i < x.size(); ++i) x[i] = std::sin(0.163 * static_cast<double>(i));
             return static_cast<float>(oversampledTanh(x, 2).back());
         }, kTimingSamples / 10)},
        {"oversampled_4x_direct_fir", medianNsPerSample([&](std::size_t n) {
             std::vector<double> x(n);
             for (std::size_t i = 0; i < x.size(); ++i) x[i] = std::sin(0.163 * static_cast<double>(i));
             return static_cast<float>(oversampledTanh(x, 4).back());
         }, kTimingSamples / 10)},
    };
    sat["note"] = "sine 1244.5 Hz, tanh(4x); oversampling with direct-form windowed-sinc FIR (63 or 255 taps), an upper bound "
                  "on cost: a polyphase or IIR halfband would be cheaper";
    out["A_saturation"] = sat;

    // ---- B. supersaw: N engine PolyBLEP oscillators per note, 8 voices ----
    json saw;
    for (const int N : {1, 3, 5, 7}) {
        const double ns = medianNsPerSample([N](std::size_t n) {
            std::vector<PolyBlepOscillator> osc(static_cast<std::size_t>(8 * N));
            for (std::size_t k = 0; k < osc.size(); ++k) osc[k].reset(static_cast<float>(k) * 0.137f - std::floor(static_cast<float>(k) * 0.137f));
            float acc = 0.0f;
            for (std::size_t i = 0; i < n; ++i) {
                for (std::size_t k = 0; k < osc.size(); ++k) {
                    acc += osc[k].processSaw(220.0f * (1.0f + 0.003f * static_cast<float>(k % 7)), 48000);
                }
            }
            return acc;
        });
        saw[std::to_string(N)] = {{"ns_per_frame_8_voices", ns}};
    }
    out["B_supersaw"] = saw;

    // ---- C. LFO on the engine SVF cutoff ----
    out["C_lfo_svf"] = {
        {"constant_cutoff", medianNsPerSample([](std::size_t n) {
             TptSvfFilter f;
             float acc = 0.0f;
             for (std::size_t i = 0; i < n; ++i) acc += f.process(std::sin(0.05f * static_cast<float>(i)), 1000.0f, 0.7f, 48000).lp;
             return acc;
         })},
        {"lfo_every_sample", medianNsPerSample([](std::size_t n) {
             TptSvfFilter f;
             float acc = 0.0f;
             for (std::size_t i = 0; i < n; ++i) {
                 const float cutoff = 1000.0f * (1.0f + 0.5f * std::sin(2.0f * kPi * 2.0f * static_cast<float>(i) / 48000.0f));
                 acc += f.process(std::sin(0.05f * static_cast<float>(i)), cutoff, 0.7f, 48000).lp;
             }
             return acc;
         })},
        {"lfo_every_32_samples", medianNsPerSample([](std::size_t n) {
             TptSvfFilter f;
             float acc = 0.0f, cutoff = 1000.0f;
             for (std::size_t i = 0; i < n; ++i) {
                 if (i % 32 == 0) cutoff = 1000.0f * (1.0f + 0.5f * std::sin(2.0f * kPi * 2.0f * static_cast<float>(i) / 48000.0f));
                 acc += f.process(std::sin(0.05f * static_cast<float>(i)), cutoff, 0.7f, 48000).lp;
             }
             return acc;
         })},
    };

    // ---- D. denormal tail of a lowpass after an impulse ----
    const auto tail = [](auto&& step) {
        return medianNsPerSample([&](std::size_t n) {
            float acc = 0.0f;
            for (std::size_t i = 0; i < n; ++i) acc += step(i == 0 ? 1.0f : 0.0f);
            return acc;
        });
    };
    const double withFlush = tail([f = TptSvfFilter{}](float x) mutable { return f.processLowpass(x, 50.0f, 0.7f, 48000.0f); });
    const double noFlush = tail([f = SvfNoFlush{}](float x) mutable { return f.process(x, 50.0f, 0.7f); });
    const unsigned int csr = _mm_getcsr();
    _mm_setcsr(csr | _MM_FLUSH_ZERO_ON | _MM_DENORMALS_ZERO_ON);
    const double noFlushFtz = tail([f = SvfNoFlush{}](float x) mutable { return f.process(x, 50.0f, 0.7f); });
    _mm_setcsr(csr);
    // How many samples of the unflushed tail are subnormal
    SvfNoFlush probe;
    std::size_t subnormal = 0;
    for (std::size_t i = 0; i < kTimingSamples; ++i) {
        const float y = probe.process(i == 0 ? 1.0f : 0.0f, 50.0f, 0.7f);
        if (std::fpclassify(probe.s1) == FP_SUBNORMAL || std::fpclassify(probe.s2) == FP_SUBNORMAL || std::fpclassify(y) == FP_SUBNORMAL) ++subnormal;
    }
    out["D_denormals"] = {{"engine_flush", withFlush}, {"no_flush", noFlush}, {"no_flush_ftz_daz", noFlushFtz},
                          {"subnormal_samples_unflushed_of_480000", subnormal}};

    std::filesystem::create_directories("artifacts/rm25_3");
    std::ofstream("artifacts/rm25_3/results.json") << out.dump(2) << "\n";
    WARN(out.dump(2));
    CHECK(std::isfinite(out["A_saturation"]["alias_to_signal_db_band_3"]["naive"].get<double>()));
}
