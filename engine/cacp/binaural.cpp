#include "binaural.h"

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "evaluation_pack.h"
#include "stem_analysis.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <complex>
#include <filesystem>
#include <fstream>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr std::uint32_t kFs = 48000;
constexpr std::size_t kFixtureCaptureFrames = 8'900'000;

double erb(double f) { return 24.7 * (4.37e-3 * f + 1.0); }
double erbNumber(double f) { return 21.4 * std::log10(4.37e-3 * f + 1.0); }
double erbNumberInverse(double e) { return (std::pow(10.0, e / 21.4) - 1.0) / 4.37e-3; }

std::uint64_t splitmix(std::uint64_t& s) {
    std::uint64_t z = (s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

std::vector<float> noise(std::size_t n, std::uint64_t seed) {
    std::vector<float> x(n);
    for (auto& v : x) v = static_cast<float>(static_cast<double>(splitmix(seed) >> 11) * (1.0 / 9007199254740992.0) * 2.0 - 1.0) * 0.25f;
    return x;
}

std::vector<float> interleave(const std::vector<float>& l, const std::vector<float>& r) {
    std::vector<float> s(2 * l.size());
    for (std::size_t i = 0; i < l.size(); ++i) {
        s[2 * i] = l[i];
        s[2 * i + 1] = r[i];
    }
    return s;
}

void writeWav(const std::filesystem::path& p, std::span<const float> stereo) {
    const auto bytes = encodeWavStereo16(stereo, kFs);
    std::ofstream o(p, std::ios::binary);
    o.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

json summaryJson(const BinauralSummary& s) {
    json bands = json::array();
    for (const auto& b : s.bands) {
        bands.push_back({{"fc_hz", b.fcHz}, {"ild_db", b.ildDb}, {"itd_frames", b.itdFrames}, {"iacc", b.iacc},
                         {"energy_l", b.energyL}, {"energy_r", b.energyR}});
    }
    return {{"defined", s.defined}, {"ild_weighted_db", s.ildWeightedDb}, {"itd_low_frames", s.itdLowFrames},
            {"iacc_weighted", s.iaccWeighted}, {"ild_band_std_db", s.ildBandStdDb}, {"bands", bands}};
}

} // namespace

std::vector<double> erbSpacedCentres(std::size_t n, double lo, double hi) {
    std::vector<double> out;
    const double e0 = erbNumber(lo), e1 = erbNumber(hi);
    for (std::size_t i = 0; i < n; ++i) out.push_back(erbNumberInverse(e0 + (e1 - e0) * static_cast<double>(i) / static_cast<double>(n - 1)));
    return out;
}

std::vector<float> gammatoneBand(std::span<const float> x, double fcHz, std::uint32_t sampleRate) {
    std::vector<float> out(x.size());
    const double fs = static_cast<double>(sampleRate);
    const double a = 1.0 - std::exp(-2.0 * kPi * 1.019 * erb(fcHz) / fs);
    const std::complex<double> step = std::polar(1.0, -2.0 * kPi * fcHz / fs);
    std::complex<double> osc(1.0, 0.0), y1{}, y2{}, y3{}, y4{};
    for (std::size_t n = 0; n < x.size(); ++n) {
        const std::complex<double> z = static_cast<double>(x[n]) * osc;
        y1 += a * (z - y1);
        y2 += a * (y1 - y2);
        y3 += a * (y2 - y3);
        y4 += a * (y3 - y4);
        out[n] = static_cast<float>(2.0 * (y4 * std::conj(osc)).real());
        osc *= step;
        if ((n & 1023U) == 1023U) osc /= std::abs(osc); // keep the oscillator on the unit circle
    }
    return out;
}

BinauralSummary binauralOverWindows(std::span<const float> stereo, std::span<const std::size_t> starts, std::size_t windowFrames,
                                    std::uint32_t sampleRate) {
    BinauralSummary s;
    const std::size_t frames = stereo.size() / 2;
    std::vector<float> l(frames), r(frames);
    for (std::size_t i = 0; i < frames; ++i) {
        l[i] = stereo[2 * i];
        r[i] = stereo[2 * i + 1];
    }
    for (const double fc : erbSpacedCentres()) {
        const auto bl = gammatoneBand(l, fc, sampleRate);
        const auto br = gammatoneBand(r, fc, sampleRate);
        BandBinaural b;
        b.fcHz = fc;
        // Normalised cross-correlation: for every lag the products and both energies cover the same pairs of samples
        // (left in the window, right shifted by the lag), so |rho| <= 1 also on short, non-stationary windows
        std::vector<double> xc(2 * kMaxLagFrames + 1, 0.0), eLk(xc.size(), 0.0), eRk(xc.size(), 0.0);
        for (const std::size_t st : starts) {
            const std::size_t end = (std::min)(st + windowFrames, frames);
            for (std::size_t n = st; n < end; ++n) {
                const double lv = bl[n];
                b.energyL += lv * lv;
                b.energyR += static_cast<double>(br[n]) * br[n];
                for (int k = -kMaxLagFrames; k <= kMaxLagFrames; ++k) {
                    const long long m = static_cast<long long>(n) + k;
                    if (m < 0 || m >= static_cast<long long>(frames)) continue;
                    const double rv = br[static_cast<std::size_t>(m)];
                    const auto i = static_cast<std::size_t>(k + kMaxLagFrames);
                    xc[i] += lv * rv;
                    eLk[i] += lv * lv;
                    eRk[i] += rv * rv;
                }
            }
        }
        if (b.energyL > 0.0 && b.energyR > 0.0) {
            b.ildDb = 10.0 * std::log10(b.energyL / b.energyR);
            std::size_t best = 0;
            double bestRho = -2.0;
            for (std::size_t k = 0; k < xc.size(); ++k) {
                const double den = std::sqrt(eLk[k] * eRk[k]);
                const double rho = den > 0.0 ? xc[k] / den : 0.0;
                if (rho > bestRho) {
                    bestRho = rho;
                    best = k;
                }
            }
            b.itdFrames = static_cast<int>(best) - kMaxLagFrames;
            b.iacc = bestRho;
        }
        s.bands.push_back(b);
    }
    double eSum = 0.0, ild = 0.0, iacc = 0.0, eMax = 0.0;
    std::vector<int> lowItd;
    for (const auto& b : s.bands) {
        const double e = b.energyL + b.energyR;
        eMax = (std::max)(eMax, e);
        if (!(b.energyL > 0.0 && b.energyR > 0.0)) continue;
        eSum += e;
        ild += e * b.ildDb;
        iacc += e * b.iacc;
        if (b.fcHz < kItdLowBandsHz) lowItd.push_back(b.itdFrames);
    }
    if (eSum > 0.0) {
        s.defined = true;
        s.ildWeightedDb = ild / eSum;
        s.iaccWeighted = iacc / eSum;
        if (!lowItd.empty()) {
            std::sort(lowItd.begin(), lowItd.end());
            s.itdLowFrames = lowItd[lowItd.size() / 2];
        }
        std::vector<double> sel;
        for (const auto& b : s.bands) {
            if (b.energyL + b.energyR >= 0.1 * eMax && b.energyL > 0.0 && b.energyR > 0.0) sel.push_back(b.ildDb);
        }
        if (sel.size() > 1) {
            double m = 0.0;
            for (const double v : sel) m += v;
            m /= static_cast<double>(sel.size());
            double var = 0.0;
            for (const double v : sel) var += (v - m) * (v - m);
            s.ildBandStdDb = std::sqrt(var / static_cast<double>(sel.size() - 1));
        }
    }
    return s;
}

BinauralScenarioResult runBinauralV1(const std::string& outputDir, bool writeFiles) {
    namespace fs = std::filesystem;
    auto once = [&](BinauralScenarioResult& r, bool write) {
        const std::size_t n = 2 * kFs;
        const std::array<std::size_t, 1> whole{0};
        fs::path dir;
        if (write) {
            std::error_code ec;
            dir = fs::path(outputDir) / "binaural_v1";
            fs::create_directories(dir, ec);
        }
        // C1: amplitude pan
        for (const double pan : {-6.0, -3.0, 0.0, 3.0, 6.0}) {
            const auto x = noise(n, 11);
            std::vector<float> rr(n);
            const float g = static_cast<float>(std::pow(10.0, -pan / 20.0));
            for (std::size_t i = 0; i < n; ++i) rr[i] = x[i] * g;
            const auto st = interleave(x, rr);
            r.c1PanDb.push_back(pan);
            r.c1MeasuredDb.push_back(binauralOverWindows(st, whole, n).ildWeightedDb);
            if (write) writeWav(dir / ("c1_pan_" + std::to_string(static_cast<int>(pan)) + "dB.wav"), st);
        }
        // C2: interaural delay (positive = right channel lags)
        for (const int d : {-29, -14, 14, 29}) {
            const auto x = noise(n + 64, 12);
            std::vector<float> ll(n), rr(n);
            for (std::size_t i = 0; i < n; ++i) {
                ll[i] = x[i + 32];
                rr[i] = x[static_cast<std::size_t>(static_cast<long long>(i) + 32 - d)];
            }
            const auto st = interleave(ll, rr);
            r.c2DelayFrames.push_back(d);
            r.c2MeasuredFrames.push_back(binauralOverWindows(st, whole, n).itdLowFrames);
            if (write) writeWav(dir / ("c2_itd_" + std::to_string(d) + ".wav"), st);
        }
        // C3: decorrelated centred noise
        {
            const auto st = interleave(noise(n, 13), noise(n, 14));
            const auto s = binauralOverWindows(st, whole, n);
            r.c3IldDb = s.ildWeightedDb;
            r.c3Iacc = s.iaccWeighted;
            if (write) writeWav(dir / "c3_decorrelated.wav", st);
        }
        // C4: precedence. Lead burst (3 ms, R +6 dB) at 0.5 s; lag copy at +8 ms with L +10 dB
        {
            std::vector<float> ll(n, 0.0f), rr(n, 0.0f);
            const auto b = noise(144, 15);
            const std::size_t t0 = kFs / 2, lag = 384;
            const float g6 = static_cast<float>(std::pow(10.0, 6.0 / 20.0)), g10 = static_cast<float>(std::pow(10.0, 10.0 / 20.0));
            for (std::size_t i = 0; i < b.size(); ++i) {
                ll[t0 + i] += b[i];
                rr[t0 + i] += b[i] * g6;
                ll[t0 + lag + i] += b[i] * g6 * g10;
                rr[t0 + lag + i] += b[i] * g6;
            }
            const auto st = interleave(ll, rr);
            const std::array<std::size_t, 1> at{t0};
            r.c4FirstWavefrontIldDb = binauralOverWindows(st, at, 240).ildWeightedDb;
            r.c4NoteIldDb = binauralOverWindows(st, at, 4800).ildWeightedDb;
        }
        // Fixture: Trama stem, first wavefront and note windows from the applied NoteOns
        StemCapture cap(kFixtureCaptureFrames);
        std::vector<AppliedEventRecord> applied;
        std::uint64_t overflow = 0;
        audio::ProbeAnalysisCapture pc{};
        pc.stems = cap.sink();
        pc.applied = &applied;
        pc.appliedOverflow = &overflow;
        const auto probe = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);
        r.goldenPcmStatus = probe.goldenPcmStatus;
        const CaptureCheck c{true, probe.passed, probe.failureReason, static_cast<std::size_t>(probe.requestedSampleFrames),
                             cap.frames(), cap.overflowFrames(), overflow};
        if (!captureInvalidReason(c).empty()) {
            r.reasons.push_back("invalid capture: " + captureInvalidReason(c));
            return;
        }
        std::vector<std::size_t> onsets;
        for (const double s : roleReferenceOnsetsS(applied, audio::MusicRole::Trama, kFs)) onsets.push_back(static_cast<std::size_t>(std::llround(s * kFs)));
        r.tramaFirstWavefront = binauralOverWindows(cap.stem(2), onsets, 240);
        r.tramaNote = binauralOverWindows(cap.stem(2), onsets, 4800);
    };
    BinauralScenarioResult r;
    once(r, writeFiles);
    BinauralScenarioResult again;
    once(again, false);
    r.c6RepeatIdentical = again.measuresJson() == r.measuresJson();
    if (!r.c6RepeatIdentical) r.reasons.emplace_back("C6: repeat run differs");
    r.status = r.reasons.empty() && r.tramaNote.defined ? "MEASURED" : "INCONCLUSIVE";
    if (writeFiles) {
        r.manifestFilePath = (fs::path(outputDir) / "binaural_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(r.manifestFilePath);
        r.manifestWriteSuccess = mf.is_open();
        if (r.manifestWriteSuccess) {
            mf << r.toJson();
            r.manifestWriteSuccess = mf.good();
        }
    }
    return r;
}

std::string BinauralScenarioResult::measuresJson() const {
    json j = {{"C1", {{"pan_db", c1PanDb}, {"measured_ild_db", c1MeasuredDb}}},
              {"C2", {{"delay_frames", c2DelayFrames}, {"measured_itd_frames", c2MeasuredFrames}}},
              {"C3", {{"ild_db", c3IldDb}, {"iacc", c3Iacc}}},
              {"C4", {{"first_wavefront_ild_db", c4FirstWavefrontIldDb}, {"note_ild_db", c4NoteIldDb}}},
              {"trama_first_wavefront", summaryJson(tramaFirstWavefront)},
              {"trama_note", summaryJson(tramaNote)}};
    return j.dump(2);
}

std::string BinauralScenarioResult::toJson() const {
    json j;
    j["scenario"] = "binaural_v1";
    j["status"] = status;
    j["reasons"] = reasons;
    j["preregistration"] = "docs/research/M2_5_Pkg3_Binaural_Bands_Preregistration.md";
    j["filterbank"] = {{"bands", kBinauralBands}, {"lo_hz", kBinauralLoHz}, {"hi_hz", kBinauralHiHz}, {"max_lag_frames", kMaxLagFrames}};
    j["measures"] = json::parse(measuresJson());
    j["C6_repeat_identical"] = c6RepeatIdentical;
    j["golden_pcm_status"] = goldenPcmStatus;
    j["oracle_wavs"] = "binaural_v1/ (C1-C3)";
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
