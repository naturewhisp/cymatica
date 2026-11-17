#include "stem_export.h"

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "music_planner.h"
#include "stem_analysis.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>
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

StemExportV2Result runStemExportV2(const std::string& outputDir) {
    namespace fs = std::filesystem;
    StemExportV2Result r;
    std::error_code ec;
    const fs::path dir = fs::path(outputDir) / "stem_export_v2";
    fs::create_directories(dir, ec);
    const auto write = [&](const std::string& name, const std::vector<float>& pcm) {
        const auto bytes = encodeWavStereoFloat32(pcm, kFs);
        std::ofstream o(dir / name, std::ios::binary);
        o.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (o.good()) r.files.push_back(name);
        else r.reasons.push_back("cannot write " + name);
    };
    const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();
    if (!clock) {
        r.status = "INCONCLUSIVE";
        r.reasons.emplace_back("fixture clock unavailable");
        return r;
    }
    std::map<std::uint32_t, std::vector<const audio::PhrasePlan*>> bySection;
    const auto phrases = audio::MusicPlanner::makeFixtureM24V1Phrases(*clock);
    for (const auto& p : phrases) bySection[p.sectionIndex].push_back(&p);
    std::array<std::pair<std::size_t, std::size_t>, 3> ranges{};
    for (auto& [section, list] : bySection) {
        std::sort(list.begin(), list.end(), [](const auto* a, const auto* b) { return a->startFrame < b->startFrame; });
        if (section > 2 || list.size() < 6) continue;
        const std::size_t first = section == 0 ? 4 : 2;   // A: phrases 5-6 (Trama sounds there); B, A': phrases 3-4
        ranges[section] = {static_cast<std::size_t>(list[first]->startFrame), static_cast<std::size_t>(list[first + 1]->endFrame)};
    }
    std::array<std::uint64_t, 3> withoutHash0{};
    bool k1 = true;
    for (const double db : {0.0, 3.0, 6.0, 9.0}) {
        StemCapture cap(kFixtureCaptureFrames);
        std::vector<AppliedEventRecord> applied;
        std::uint64_t overflow = 0;
        audio::ProbeAnalysisCapture pc{};
        pc.stems = cap.sink();
        pc.applied = &applied;
        pc.appliedOverflow = &overflow;
        pc.tramaBusGainScale = db == 0.0 ? 1.0f : static_cast<float>(std::pow(10.0, db / 20.0));
        const auto probe = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);
        const CaptureCheck c{true, probe.passed, probe.failureReason, static_cast<std::size_t>(probe.requestedSampleFrames),
                             cap.frames(), cap.overflowFrames(), overflow};
        if (!captureInvalidReason(c).empty()) {
            r.reasons.push_back("offset " + std::to_string(static_cast<int>(db)) + ": invalid capture: " + captureInvalidReason(c));
            continue;
        }
        if (db == 0.0) {
            if (probe.goldenPcmStatus != "PASS") r.reasons.push_back("offset 0: golden PCM " + probe.goldenPcmStatus);
            write("whole_mix.wav", std::vector<float>(cap.busMix().begin(), cap.busMix().end()));
            for (std::size_t role = 0; role < 4; ++role) {
                write(std::string("whole_") + kRoleNames[role] + ".wav", std::vector<float>(cap.stem(role).begin(), cap.stem(role).end()));
            }
        }
        const std::string tag = "_t" + std::to_string(static_cast<int>(db)) + "_";
        for (std::size_t sec = 0; sec < 3; ++sec) {
            const auto [s0, s1] = ranges[sec];
            if (s1 <= s0) {
                r.reasons.push_back(std::string("section ") + kSectionNames[sec] + ": excerpt unavailable");
                continue;
            }
            const auto slice = [&](std::span<const float> pcm) { return std::vector<float>(pcm.begin() + 2 * s0, pcm.begin() + 2 * s1); };
            write(std::string(kSectionNames[sec]) + tag + "mix.wav", slice(cap.busMix()));
            write(std::string(kSectionNames[sec]) + tag + "trama.wav", slice(cap.stem(2)));
            std::vector<float> without = slice(cap.stem(0));
            const auto cv = slice(cap.stem(1));
            const auto vv = slice(cap.stem(3));
            std::uint64_t h = 1469598103934665603ULL;
            for (std::size_t i = 0; i < without.size(); ++i) {
                without[i] = without[i] + cv[i] + vv[i];
                std::uint32_t bits = 0;
                std::memcpy(&bits, &without[i], 4);
                for (int k = 0; k < 4; ++k) {
                    h ^= (bits >> (8 * k)) & 0xFFU;
                    h *= 1099511628211ULL;
                }
            }
            if (db == 0.0) {
                withoutHash0[sec] = h;
                write(std::string(kSectionNames[sec]) + "_mix_without_trama.wav", without);
            } else if (h != withoutHash0[sec]) {
                k1 = false;
            }
        }
    }
    r.k1WithoutTramaIdentical = k1;
    r.status = r.reasons.empty() ? "MEASURED" : "INCONCLUSIVE";
    r.manifestFilePath = (fs::path(outputDir) / "stem_export_v2_manifest.json").lexically_normal().string();
    std::ofstream mf(r.manifestFilePath);
    r.manifestWriteSuccess = mf.is_open();
    if (r.manifestWriteSuccess) {
        mf << r.toJson();
        r.manifestWriteSuccess = mf.good();
    }
    return r;
}

std::string StemExportV2Result::toJson() const {
    json j;
    j["scenario"] = "stem_export_v2";
    j["status"] = status;
    j["reasons"] = reasons;
    j["preregistration"] = "docs/research/M2_5_Pkg4b_Slope_And_Trama_Sensitivity_Preregistration.md";
    j["dir"] = "stem_export_v2";
    j["files"] = files;
    j["K1_mix_without_trama_identical"] = k1WithoutTramaIdentical;
    j["excerpts"] = "A: phrases 5-6; B, A': phrases 3-4; offsets 0, +3, +6, +9 dB on the Trama bus (offline override)";
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
