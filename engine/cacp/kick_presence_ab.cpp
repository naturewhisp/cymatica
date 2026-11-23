#include "kick_presence_ab.h"

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "evaluation_pack.h"
#include "loudness.h"
#include "stem_analysis.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::size_t kCaptureFrames = 8'900'000; // > 8,825,200 frames of fixture_probe_v1

struct Variant {
    std::string tag{};
    std::int32_t fixedMidi{0}; // 0 = production pitch
    double clickDb{0.0};
};

std::uint64_t hashFloats(std::span<const float> x) {
    std::uint64_t h = 1469598103934665603ULL;
    const auto* bytes = reinterpret_cast<const unsigned char*>(x.data());
    for (std::size_t i = 0; i < x.size_bytes(); ++i) {
        h ^= bytes[i];
        h *= 1099511628211ULL;
    }
    return h;
}

bool writeWav(const std::filesystem::path& path, std::span<const float> pcm) {
    const auto bytes = encodeWavStereo16(pcm, kFs);
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return out.good() && !pcm.empty();
}

} // namespace

std::string runKickPresenceAbV1(const std::string& outputDir) {
    namespace fs = std::filesystem;
    const fs::path dir = fs::path(outputDir) / "kick_presence_ab_v1";
    std::error_code ec;
    fs::create_directories(dir, ec);

    std::vector<Variant> variants{{"base", 0, 0.0}};
    for (const std::int32_t m : {31, 33, 35, 36, 38}) variants.push_back({"pitch" + std::to_string(m), m, 0.0});
    for (const int d : {3, 6, 9}) variants.push_back({"click" + std::to_string(d), 0, static_cast<double>(d)});
    variants.push_back({"combo36click3", 36, 3.0}); // combination pre-registered on 2026-11-23

    json out{{"scenario", "kick_presence_ab_v1"}, {"variants", json::array()}, {"reasons", json::array()}};
    std::uint64_t events0 = 0, trama0 = 0, vettore0 = 0;
    bool sameEvents = true, sameOthers = true;
    StemCapture cap(kCaptureFrames);
    for (const auto& v : variants) {
        cap.reset();
        std::vector<AppliedEventRecord> applied;
        std::uint64_t overflow = 0;
        std::vector<float> stereo;
        audio::ProbeAnalysisCapture pc{};
        pc.stems = cap.sink();
        pc.applied = &applied;
        pc.appliedOverflow = &overflow;
        pc.stereo = &stereo;
        pc.pulsoFixedMidi = v.fixedMidi;
        pc.pulsoClickScale = (v.clickDb == 0.0) ? 1.0f : static_cast<float>(std::pow(10.0, v.clickDb / 20.0));
        const auto probe = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);
        const CaptureCheck c{true, probe.passed, probe.failureReason, static_cast<std::size_t>(probe.requestedSampleFrames),
                             cap.frames(), cap.overflowFrames(), overflow};
        const std::string invalid = captureInvalidReason(c);
        if (!invalid.empty()) {
            out["reasons"].push_back(v.tag + ": invalid capture: " + invalid);
            continue;
        }
        std::uint64_t ev = 1469598103934665603ULL;
        for (const auto& rec : applied) {
            for (const std::uint64_t x : {rec.id.eventId, rec.id.sampleFrame, rec.consumedFrame, static_cast<std::uint64_t>(rec.disposition)}) {
                ev ^= x;
                ev *= 1099511628211ULL;
            }
        }
        const std::uint64_t trama = hashFloats(cap.stem(2)), vettore = hashFloats(cap.stem(3));
        const bool base = v.tag == "base";
        if (base) {
            events0 = ev;
            trama0 = trama;
            vettore0 = vettore;
            if (probe.goldenPcmStatus != "PASS") out["reasons"].push_back("base: golden PCM " + probe.goldenPcmStatus);
        } else {
            sameEvents = sameEvents && ev == events0;
            sameOthers = sameOthers && trama == trama0 && vettore == vettore0;
            if (probe.goldenPcmStatus != "NOT_APPLICABLE") out["reasons"].push_back(v.tag + ": golden PCM " + probe.goldenPcmStatus);
        }
        const LoudnessResult mix = measureLoudnessStereo(stereo, kFs);
        out["variants"].push_back({{"tag", v.tag},
                                   {"fixed_midi", v.fixedMidi},
                                   {"click_db", v.clickDb},
                                   {"golden_pcm_status", probe.goldenPcmStatus},
                                   {"mix_integrated_lufs", mix.integratedLufs ? json(*mix.integratedLufs) : json(nullptr)},
                                   {"true_peak_dbtp", probe.truePeakDbtp},
                                   {"max_gain_reduction_db", probe.maxGainReductionDb},
                                   {"pulso_voice_drops_total", probe.voiceDropCount}});
        bool ok = writeWav(dir / (v.tag + "_pulso.wav"), cap.stem(0)) && writeWav(dir / (v.tag + "_corpo.wav"), cap.stem(1)) &&
                  writeWav(dir / (v.tag + "_mix.wav"), stereo);
        if (base) ok = ok && writeWav(dir / "base_trama.wav", cap.stem(2)) && writeWav(dir / "base_vettore.wav", cap.stem(3));
        if (!ok) out["reasons"].push_back(v.tag + ": cannot write the clips");
    }
    out["k5_events_identical"] = sameEvents;
    out["k5_trama_vettore_identical"] = sameOthers;
    out["status"] = out["reasons"].empty() ? "MEASURED" : "INCONCLUSIVE";
    std::ofstream f(dir / "kick_presence_ab_v1_manifest.json");
    f << out.dump(2);
    f.close();
    return f.good() ? out.dump(2) : std::string{};
}

} // namespace cymatica::cacp
