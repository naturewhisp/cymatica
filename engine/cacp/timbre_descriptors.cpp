#include "timbre_descriptors.h"

#include "acao02_calibration.h"
#include "release_ab.h"
#include "stem_analysis.h"
#include "stem_export.h"
#include "stft.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr double kPi = 3.14159265358979323846;
constexpr double kNoteS = 0.300;
constexpr double kTailS = 0.050;
constexpr double kSpacingS = 1.0;
constexpr double kFirstS = 0.5;

std::vector<double> powerFrames(const MagnitudeSpectrogram& m, std::size_t frame) {
    std::vector<double> p(m.bins);
    for (std::size_t b = 0; b < m.bins; ++b) {
        const double v = m.at(frame, b);
        p[b] = v * v;
    }
    return p;
}

// Synthetic note with the same envelope for every waveform: 15 ms linear attack, sustain, 2 ms release
std::vector<float> syntheticNote(double f0, int kind) {
    const std::size_t n = static_cast<std::size_t>((kNoteS + kTailS) * kFs);
    std::vector<float> x(n, 0.0f);
    for (std::size_t i = 0; i < n; ++i) {
        const double t = static_cast<double>(i) / kFs;
        double env = t < 0.015 ? t / 0.015 : 1.0;
        if (t > kNoteS) env = (std::max)(0.0, 1.0 - (t - kNoteS) / 0.002);
        double v = 0.0;
        if (kind == 0) {
            v = std::sin(2.0 * kPi * f0 * t);
        } else if (kind == 1) {
            for (int k = 1; k <= 40 && k * f0 < 20000.0; ++k) v += std::sin(2.0 * kPi * k * f0 * t) / k;   // band-limited saw
            v *= 0.6;
        } else {
            v = std::sin(2.0 * kPi * f0 * t + 3.0 * std::sin(2.0 * kPi * 1.88 * f0 * t));               // FM 1.88, index 3
        }
        x[i] = static_cast<float>(0.25 * env * v);
    }
    return x;
}

json vecJson(const TimbreVector& v) {
    json j = json::object();
    for (std::size_t i = 0; i < kTimbreDescriptors; ++i) j[kTimbreDescriptorNames[i]] = v[i];
    return j;
}

} // namespace

std::array<double, 2> centroidSpreadHz(std::span<const float> mono, std::uint32_t sampleRate) {
    const MagnitudeSpectrogram m = stftMagnitude(mono);
    const double binHz = static_cast<double>(sampleRate) / static_cast<double>(kDescriptorsV1Stft.fftSize);
    double wSum = 0.0, cSum = 0.0, sSum = 0.0;
    for (std::size_t f = 0; f < m.frames; ++f) {
        const auto p = powerFrames(m, f);
        double e = 0.0, c = 0.0;
        for (std::size_t b = 0; b < p.size(); ++b) {
            e += p[b];
            c += p[b] * b * binHz;
        }
        if (!(e > 0.0)) continue;
        c /= e;
        double s = 0.0;
        for (std::size_t b = 0; b < p.size(); ++b) s += p[b] * (b * binHz - c) * (b * binHz - c);
        s = std::sqrt(s / e);
        wSum += e;
        cSum += e * c;
        sSum += e * s;
    }
    if (!(wSum > 0.0)) return {0.0, 0.0};
    return {cSum / wSum, sSum / wSum};
}

TimbreVector timbreDescriptors(std::span<const float> mono, double f0, std::uint32_t sampleRate) {
    TimbreVector d{};
    const double fs = static_cast<double>(sampleRate);
    const auto cs = centroidSpreadHz(mono, sampleRate);
    d[0] = cs[0] / f0;
    d[1] = cs[1] / f0;
    // Odd/even: long-term power around the first 10 harmonics, +-f0/4
    const MagnitudeSpectrogram m = stftMagnitude(mono);
    const double binHz = fs / static_cast<double>(kDescriptorsV1Stft.fftSize);
    std::vector<double> lt(m.bins, 0.0);
    for (std::size_t f = 0; f < m.frames; ++f) {
        for (std::size_t b = 0; b < m.bins; ++b) {
            const double v = m.at(f, b);
            lt[b] += v * v;
        }
    }
    double odd = 0.0, even = 0.0;
    for (int k = 1; k <= 10; ++k) {
        const double lo = (k - 0.25) * f0, hi = (k + 0.25) * f0;
        double e = 0.0;
        for (std::size_t b = 0; b < m.bins; ++b) {
            const double fb = b * binHz;
            if (fb >= lo && fb <= hi) e += lt[b];
        }
        (k % 2 == 1 ? odd : even) += e;
    }
    d[2] = (odd > 0.0 && even > 0.0) ? 10.0 * std::log10(odd / even) : 0.0;
    // Attack time: 10 % -> 90 % of the maximum of the 1 ms RMS envelope
    {
        const std::size_t blk = sampleRate / 1000;
        std::vector<double> env;
        for (std::size_t i = 0; i + blk <= mono.size(); i += blk) {
            double s = 0.0;
            for (std::size_t k = 0; k < blk; ++k) s += static_cast<double>(mono[i + k]) * mono[i + k];
            env.push_back(std::sqrt(s / static_cast<double>(blk)));
        }
        const double mx = env.empty() ? 0.0 : *std::max_element(env.begin(), env.end());
        std::size_t i10 = 0, i90 = 0;
        bool f10 = false;
        for (std::size_t i = 0; i < env.size(); ++i) {
            if (!f10 && env[i] >= 0.1 * mx) { i10 = i; f10 = true; }
            if (env[i] >= 0.9 * mx) { i90 = i; break; }
        }
        d[3] = static_cast<double>(i90 - i10);   // ms (1 ms blocks)
    }
    // Centroid slope over the first 200 ms (frames above 1e-6 of the maximum frame energy), per second
    {
        std::vector<double> t, c;
        double eMax = 0.0;
        std::vector<double> es(m.frames, 0.0), cents(m.frames, 0.0);
        for (std::size_t f = 0; f < m.frames; ++f) {
            const auto p = powerFrames(m, f);
            double e = 0.0, cc = 0.0;
            for (std::size_t b = 0; b < p.size(); ++b) { e += p[b]; cc += p[b] * b * binHz; }
            es[f] = e;
            cents[f] = e > 0.0 ? cc / e / f0 : 0.0;
            eMax = (std::max)(eMax, e);
        }
        for (std::size_t f = 0; f < m.frames; ++f) {
            const double tc = frameCentreSeconds(f, kDescriptorsV1Stft, sampleRate);
            if (tc > 0.200 || !(es[f] > 1e-6 * eMax)) continue;
            t.push_back(tc);
            c.push_back(cents[f]);
        }
        if (t.size() >= 2) {
            double mt = 0.0, mc = 0.0;
            for (std::size_t i = 0; i < t.size(); ++i) { mt += t[i]; mc += c[i]; }
            mt /= static_cast<double>(t.size());
            mc /= static_cast<double>(t.size());
            double num = 0.0, den = 0.0;
            for (std::size_t i = 0; i < t.size(); ++i) { num += (t[i] - mt) * (c[i] - mc); den += (t[i] - mt) * (t[i] - mt); }
            d[4] = den > 0.0 ? num / den : 0.0;
        }
    }
    // Spectral flux: mean over frames of the half-wave rectified magnitude increase, normalised by the magnitude sum
    {
        double acc = 0.0;
        std::size_t n = 0;
        for (std::size_t f = 1; f < m.frames; ++f) {
            double up = 0.0, tot = 0.0;
            for (std::size_t b = 0; b < m.bins; ++b) {
                const double diff = static_cast<double>(m.at(f, b)) - m.at(f - 1, b);
                if (diff > 0.0) up += diff;
                tot += m.at(f, b);
            }
            if (tot > 0.0) { acc += up / tot; ++n; }
        }
        d[5] = n > 0 ? acc / static_cast<double>(n) : 0.0;
    }
    return d;
}

namespace {
void zscore(std::vector<TimbreVector*>& all) {
    for (std::size_t k = 0; k < kTimbreDescriptors; ++k) {
        double m = 0.0;
        for (const auto* v : all) m += (*v)[k];
        m /= static_cast<double>(all.size());
        double s = 0.0;
        for (const auto* v : all) s += ((*v)[k] - m) * ((*v)[k] - m);
        s = std::sqrt(s / static_cast<double>(all.size()));
        for (auto* v : all) (*v)[k] = s > 0.0 ? ((*v)[k] - m) / s : 0.0;
    }
}
double dist(const TimbreVector& a, const TimbreVector& b) {
    double s = 0.0;
    for (std::size_t k = 0; k < kTimbreDescriptors; ++k) s += (a[k] - b[k]) * (a[k] - b[k]);
    return std::sqrt(s);
}
TimbreVector meanOf(const std::vector<TimbreVector>& g, std::size_t skip = static_cast<std::size_t>(-1)) {
    TimbreVector m{};
    std::size_t n = 0;
    for (std::size_t i = 0; i < g.size(); ++i) {
        if (i == skip) continue;
        for (std::size_t k = 0; k < kTimbreDescriptors; ++k) m[k] += g[i][k];
        ++n;
    }
    for (auto& v : m) v /= static_cast<double>(n);
    return m;
}
} // namespace

double looNearestCentroidAccuracy(const std::vector<TimbreVector>& a0, const std::vector<TimbreVector>& b0) {
    std::vector<TimbreVector> a = a0, b = b0;
    std::vector<TimbreVector*> all;
    for (auto& v : a) all.push_back(&v);
    for (auto& v : b) all.push_back(&v);
    zscore(all);
    std::size_t correct = 0;
    for (std::size_t i = 0; i < a.size(); ++i) correct += dist(a[i], meanOf(a, i)) < dist(a[i], meanOf(b)) ? 1 : 0;
    for (std::size_t i = 0; i < b.size(); ++i) correct += dist(b[i], meanOf(b, i)) < dist(b[i], meanOf(a)) ? 1 : 0;
    return static_cast<double>(correct) / static_cast<double>(a.size() + b.size());
}

double standardizedMeanDistance(const std::vector<std::vector<TimbreVector>>& groups0, std::size_t i, std::size_t j) {
    auto groups = groups0;
    std::vector<TimbreVector*> all;
    for (auto& g : groups) for (auto& v : g) all.push_back(&v);
    zscore(all);
    return dist(meanOf(groups[i]), meanOf(groups[j]));
}

TimbreScenarioResult runTimbreDescriptorsV1(const std::string& outputDir, bool writeFiles) {
    namespace fs = std::filesystem;
    auto once = [&](TimbreScenarioResult& r, bool write) {
        fs::path dir;
        if (write) {
            std::error_code ec;
            dir = fs::path(outputDir) / "timbre_descriptors_v1";
            fs::create_directories(dir, ec);
        }
        const std::array<audio::MusicRole, 3> roles{audio::MusicRole::Corpo, audio::MusicRole::Trama, audio::MusicRole::Vettore};
        const std::array<float, 3> timbres{0.40f, 0.50f, 0.60f};   // planner values (music_planner.cpp)
        const std::array<const char*, 3> names{"corpo", "trama", "vettore"};
        StemCapture cap(kFs * 10);
        for (std::size_t ri = 0; ri < roles.size(); ++ri) {
            std::vector<SynthNote> notes;
            for (std::size_t p = 0; p < kTimbrePitchesHz.size(); ++p) {
                notes.push_back(SynthNote{kFirstS + kSpacingS * static_cast<double>(p), kTimbrePitchesHz[p], 0.8f, kNoteS, timbres[ri]});
            }
            const SyntheticRender sr = renderSyntheticWithRelease(roles[ri], notes, cap, 0);
            if (!captureInvalidReason(sr.check).empty()) {
                r.reasons.push_back(std::string(names[ri]) + ": invalid capture: " + captureInvalidReason(sr.check));
                continue;
            }
            const auto stem = cap.stem(static_cast<std::size_t>(roles[ri]));
            for (std::size_t p = 0; p < notes.size(); ++p) {
                const auto s0 = static_cast<std::size_t>(std::llround(notes[p].timeS * kFs));
                const auto s1 = s0 + static_cast<std::size_t>(std::llround((kNoteS + kTailS) * kFs));
                std::vector<float> mono(s1 - s0);
                for (std::size_t i = s0; i < s1 && 2 * i + 1 < stem.size(); ++i) mono[i - s0] = 0.5f * (stem[2 * i] + stem[2 * i + 1]);
                r.roleNotes[ri].push_back(timbreDescriptors(mono, notes[p].pitchHz));
                if (write) {
                    std::vector<float> st(2 * mono.size());
                    for (std::size_t i = 0; i < mono.size(); ++i) { st[2 * i] = mono[i]; st[2 * i + 1] = mono[i]; }
                    const auto bytes = encodeWavStereoFloat32(st, kFs);
                    std::ofstream o(dir / (std::string(names[ri]) + "_" + std::to_string(p) + ".wav"), std::ios::binary);
                    o.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
                }
            }
        }
        if (!r.reasons.empty()) return;
        // D1: Corpo split by alternate pitches
        {
            std::vector<TimbreVector> g0, g1;
            for (std::size_t p = 0; p < r.roleNotes[0].size(); ++p) (p % 2 == 0 ? g0 : g1).push_back(r.roleNotes[0][p]);
            r.d1CorpoSplitAccuracy = looNearestCentroidAccuracy(g0, g1);
        }
        // D2, D3: synthetic waveforms with the same envelope
        {
            std::array<std::vector<TimbreVector>, 3> syn{};
            for (const float f0 : kTimbrePitchesHz) {
                for (int kind = 0; kind < 3; ++kind) syn[static_cast<std::size_t>(kind)].push_back(timbreDescriptors(syntheticNote(f0, kind), f0));
            }
            r.d2SineSawAccuracy = looNearestCentroidAccuracy(syn[0], syn[1]);
            r.d3Fm188SawAccuracy = looNearestCentroidAccuracy(syn[2], syn[1]);
        }
        r.accTramaCorpo = looNearestCentroidAccuracy(r.roleNotes[1], r.roleNotes[0]);
        r.accTramaVettore = looNearestCentroidAccuracy(r.roleNotes[1], r.roleNotes[2]);
        r.accCorpoVettore = looNearestCentroidAccuracy(r.roleNotes[0], r.roleNotes[2]);
        const std::vector<std::vector<TimbreVector>> groups{r.roleNotes[0], r.roleNotes[1], r.roleNotes[2]};
        r.distTramaCorpo = standardizedMeanDistance(groups, 1, 0);
        r.distTramaVettore = standardizedMeanDistance(groups, 1, 2);
        r.distCorpoVettore = standardizedMeanDistance(groups, 0, 2);
    };
    TimbreScenarioResult r;
    once(r, writeFiles);
    TimbreScenarioResult again;
    once(again, false);
    r.d5RepeatIdentical = again.measuresJson() == r.measuresJson();
    if (!r.d5RepeatIdentical) r.reasons.emplace_back("D5: repeat run differs");
    r.status = r.reasons.empty() ? "MEASURED" : "INCONCLUSIVE";
    if (writeFiles) {
        r.manifestFilePath = (fs::path(outputDir) / "timbre_descriptors_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(r.manifestFilePath);
        r.manifestWriteSuccess = mf.is_open();
        if (r.manifestWriteSuccess) {
            mf << r.toJson();
            r.manifestWriteSuccess = mf.good();
        }
    }
    return r;
}

std::string TimbreScenarioResult::measuresJson() const {
    json notes = json::object();
    const std::array<const char*, 3> names{"corpo", "trama", "vettore"};
    for (std::size_t ri = 0; ri < 3; ++ri) {
        json arr = json::array();
        for (const auto& v : roleNotes[ri]) arr.push_back(vecJson(v));
        notes[names[ri]] = arr;
    }
    json j = {{"pitches_hz", kTimbrePitchesHz},
              {"notes", notes},
              {"D1_corpo_split_accuracy", d1CorpoSplitAccuracy},
              {"D2_sine_saw_accuracy", d2SineSawAccuracy},
              {"D3_fm188_saw_accuracy", d3Fm188SawAccuracy},
              {"accuracy", {{"trama_corpo", accTramaCorpo}, {"trama_vettore", accTramaVettore}, {"corpo_vettore", accCorpoVettore}}},
              {"standardized_distance", {{"trama_corpo", distTramaCorpo}, {"trama_vettore", distTramaVettore}, {"corpo_vettore", distCorpoVettore}}}};
    return j.dump(2);
}

std::string TimbreScenarioResult::toJson() const {
    json j;
    j["scenario"] = "timbre_descriptors_v1";
    j["status"] = status;
    j["reasons"] = reasons;
    j["preregistration"] = "docs/research/M2_5_Pkg5_Timbre_Descriptors_Preregistration.md";
    j["descriptors"] = kTimbreDescriptorNames;
    j["measures"] = json::parse(measuresJson());
    j["D5_repeat_identical"] = d5RepeatIdentical;
    j["oracle_wavs"] = "timbre_descriptors_v1/<role>_<pitch index>.wav";
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
