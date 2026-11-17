#include "stem_export.h"

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "music_planner.h"
#include "stem_analysis.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>

namespace cymatica::cacp {

using nlohmann::json;

namespace {
constexpr std::uint32_t kFs = 48000;
constexpr std::size_t kFixtureCaptureFrames = 8'900'000;
constexpr const char* kSectionNames[] = {"A", "B", "Aprime"};

void put32(std::vector<std::uint8_t>& b, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xFFU));
}
void put16(std::vector<std::uint8_t>& b, std::uint16_t v) {
    b.push_back(static_cast<std::uint8_t>(v & 0xFFU));
    b.push_back(static_cast<std::uint8_t>(v >> 8));
}
} // namespace

std::vector<std::uint8_t> encodeWavStereoFloat32(std::span<const float> interleaved, std::uint32_t sampleRate) {
    std::vector<std::uint8_t> b;
    const auto dataBytes = static_cast<std::uint32_t>(interleaved.size() * sizeof(float));
    b.insert(b.end(), {'R', 'I', 'F', 'F'});
    put32(b, 36 + dataBytes);
    b.insert(b.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    put32(b, 16);
    put16(b, 3);            // IEEE float
    put16(b, 2);
    put32(b, sampleRate);
    put32(b, sampleRate * 2 * 4);
    put16(b, 8);
    put16(b, 32);
    b.insert(b.end(), {'d', 'a', 't', 'a'});
    put32(b, dataBytes);
    const std::size_t o = b.size();
    b.resize(o + dataBytes);
    std::memcpy(b.data() + o, interleaved.data(), dataBytes);
    return b;
}

StemExportResult runStemExportV1(const std::string& outputDir) {
    namespace fs = std::filesystem;
    StemExportResult r;
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
        r.status = "INCONCLUSIVE";
        r.reasons.push_back("invalid capture: " + captureInvalidReason(c));
        return r;
    }
    const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();
    if (!clock) {
        r.status = "INCONCLUSIVE";
        r.reasons.emplace_back("fixture clock unavailable");
        return r;
    }
    std::map<std::uint32_t, std::vector<const audio::PhrasePlan*>> bySection;
    const auto phrases = audio::MusicPlanner::makeFixtureM24V1Phrases(*clock);
    for (const auto& p : phrases) bySection[p.sectionIndex].push_back(&p);
    std::error_code ec;
    const fs::path dir = fs::path(outputDir) / "stem_export_v1";
    fs::create_directories(dir, ec);
    const auto write = [&](const std::string& name, const std::vector<float>& pcm) {
        const auto bytes = encodeWavStereoFloat32(pcm, kFs);
        std::ofstream o(dir / name, std::ios::binary);
        o.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (o.good()) r.files.push_back(name);
        else r.reasons.push_back("cannot write " + name);
    };
    for (auto& [section, list] : bySection) {
        std::sort(list.begin(), list.end(), [](const auto* a, const auto* b) { return a->startFrame < b->startFrame; });
        if (list.size() < 4 || section > 2) continue;
        const std::size_t s0 = static_cast<std::size_t>(list[2]->startFrame), s1 = static_cast<std::size_t>(list[3]->endFrame);
        const auto slice = [&](std::span<const float> pcm) { return std::vector<float>(pcm.begin() + 2 * s0, pcm.begin() + 2 * s1); };
        const std::string base = std::string(kSectionNames[section]) + "_";
        const auto bus = slice(cap.busMix());
        write(base + "mix.wav", bus);
        for (std::size_t role = 0; role < 4; ++role) {
            const auto stem = slice(cap.stem(role));
            write(base + kRoleNames[role] + ".wav", stem);
            std::vector<float> without = bus;
            for (std::size_t i = 0; i < without.size(); ++i) without[i] -= stem[i];
            write(base + "mix_without_" + kRoleNames[role] + ".wav", without);
        }
    }
    r.status = (r.reasons.empty() && r.files.size() == 27) ? "MEASURED" : "INCONCLUSIVE";
    r.manifestFilePath = (fs::path(outputDir) / "stem_export_v1_manifest.json").lexically_normal().string();
    std::ofstream mf(r.manifestFilePath);
    r.manifestWriteSuccess = mf.is_open();
    if (r.manifestWriteSuccess) {
        mf << r.toJson();
        r.manifestWriteSuccess = mf.good();
    }
    return r;
}

std::string StemExportResult::toJson() const {
    json j;
    j["scenario"] = "stem_export_v1";
    j["status"] = status;
    j["reasons"] = reasons;
    j["preregistration"] = "docs/research/M2_5_Pkg4_Specific_Loudness_Masking_Preregistration.md";
    j["dir"] = "stem_export_v1";
    j["files"] = files;
    j["golden_pcm_status"] = goldenPcmStatus;
    j["note"] = "bus mix before the master; mix_without_<role> = bus mix - stem (the sum of the other stems up to float rounding)";
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
