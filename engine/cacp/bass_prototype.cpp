#include "bass_prototype.h"

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "evaluation_pack.h"
#include "loudness.h"
#include "release_ab.h"
#include "stem_analysis.h"

#include <nlohmann/json.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::size_t kCaptureFrames = 8'900'000; // > 8,825,200 frames of fixture_probe_v1
constexpr std::array<const char*, 4> kStemNames{"pulso", "corpo", "trama", "vettore"};

bool writeWav(const std::filesystem::path& path, std::span<const float> pcm) {
    const auto bytes = encodeWavStereo16(pcm, kFs);
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return out.good() && !pcm.empty();
}

std::uint64_t hashFloats(std::span<const float> x) {
    std::uint64_t h = 1469598103934665603ULL;
    const auto* bytes = reinterpret_cast<const unsigned char*>(x.data());
    for (std::size_t i = 0; i < x.size_bytes(); ++i) {
        h ^= bytes[i];
        h *= 1099511628211ULL;
    }
    return h;
}

// One note per line: time_s,pitch_hz,velocity,duration_s,timbre. Empty on any malformed line (fail-closed).
std::vector<SynthNote> readNotes(const std::filesystem::path& path) {
    std::vector<SynthNote> notes;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::istringstream ss(line);
        std::array<double, 5> v{};
        for (std::size_t k = 0; k < v.size(); ++k) {
            char comma = 0;
            if ((k > 0 && (!(ss >> comma) || comma != ',')) || !(ss >> v[k]) || !std::isfinite(v[k])) return {};
        }
        if (v[0] < 0.0 || v[0] > 200.0 || v[1] < 20.0 || v[1] > 400.0 || v[3] <= 0.0 || v[3] > 10.0) return {};
        notes.push_back({v[0], static_cast<float>(v[1]), static_cast<float>(v[2]), v[3], static_cast<float>(v[4])});
    }
    return notes;
}

} // namespace

std::string runBassPrototypeV1(const std::string& outputDir) {
    namespace fs = std::filesystem;
    const fs::path dir = fs::path(outputDir) / "bass_prototype_v1";
    std::error_code ec;
    fs::create_directories(dir, ec);
    json out{{"scenario", "bass_prototype_v1"}, {"renders", json::array()}, {"bass_lines", json::array()}, {"reasons", json::array()}};
    StemCapture cap(kCaptureFrames);
    for (const bool candidateKick : {false, true}) {
        const std::string kick = candidateKick ? "c2" : "now";
        std::array<std::uint64_t, 4> base{};
        for (const bool octaveDown : {false, true}) {
            const std::string tag = kick + (octaveDown ? "_v1" : "_v0");
            cap.reset();
            std::vector<float> stereo;
            audio::ProbeAnalysisCapture pc{};
            pc.stems = cap.sink();
            pc.stereo = &stereo;
            pc.pulsoFixedMidi = candidateKick ? 36 : 0;
            pc.pulsoClickScale = candidateKick ? static_cast<float>(std::pow(10.0, 3.0 / 20.0)) : 1.0f;
            pc.corpoBassOctaveDown = octaveDown;
            const auto probe = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);
            if (!probe.passed || cap.overflowFrames() > 0) out["reasons"].push_back(tag + ": " + probe.failureReason);
            const std::string expected = (candidateKick || octaveDown) ? "NOT_APPLICABLE" : "PASS";
            if (probe.goldenPcmStatus != expected) out["reasons"].push_back(tag + ": golden PCM " + probe.goldenPcmStatus);
            std::array<std::uint64_t, 4> h{};
            bool ok = writeWav(dir / (tag + "_mix.wav"), stereo);
            for (std::size_t r = 0; r < 4; ++r) {
                h[r] = hashFloats(cap.stem(r));
                ok = ok && writeWav(dir / (tag + "_" + kStemNames[r] + ".wav"), cap.stem(r));
            }
            if (!ok) out["reasons"].push_back(tag + ": cannot write the stems");
            if (!octaveDown) base = h;
            const LoudnessResult mix = measureLoudnessStereo(stereo, kFs);
            out["renders"].push_back({{"tag", tag},
                                      {"golden_pcm_status", probe.goldenPcmStatus},
                                      {"same_pulso", h[0] == base[0]},
                                      {"same_corpo", h[1] == base[1]},
                                      {"same_trama", h[2] == base[2]},
                                      {"same_vettore", h[3] == base[3]},
                                      {"mix_integrated_lufs", mix.integratedLufs ? json(*mix.integratedLufs) : json(nullptr)},
                                      {"true_peak_dbtp", probe.truePeakDbtp},
                                      {"max_gain_reduction_db", probe.maxGainReductionDb}});
        }
        for (const std::string sep : {"s0", "s1"}) {
            const std::string name = "bass_" + kick + "_" + sep;
            const auto notes = readNotes(dir / (name + ".csv"));
            if (notes.empty()) {
                out["reasons"].push_back(name + ": no valid notes");
                continue;
            }
            const SyntheticRender sr = renderSyntheticWithRelease(audio::MusicRole::Corpo, notes, cap, 0);
            if (!sr.check.probePassed) out["reasons"].push_back(name + ": " + sr.check.probeFailure);
            if (!writeWav(dir / (name + ".wav"), cap.stem(1))) out["reasons"].push_back(name + ": cannot write");
            out["bass_lines"].push_back({{"name", name}, {"notes", notes.size()}});
        }
    }
    out["status"] = out["reasons"].empty() ? "MEASURED" : "INCONCLUSIVE";
    std::ofstream f(dir / "bass_prototype_v1_manifest.json");
    f << out.dump(2);
    f.close();
    return f.good() ? out.dump(2) : std::string{};
}

} // namespace cymatica::cacp
