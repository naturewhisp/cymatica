#include "cumulative_listening.h"

#include "audio_probe.h"
#include "evaluation_pack.h"
#include "loudness.h"
#include "stem_analysis.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <span>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::uint64_t kReferenceSeed = 1337;
constexpr std::size_t kStemCaptureFrames = 8'900'000; // > 8,825,200 frames of fixture_probe_v1

json variantJson(const audio::FixtureVariant& v) {
    return json{{"a_trama_entry", v.aTramaEntry},   {"b_displacement_start", v.bDisplacementStart},
                {"a_prime_kick_rest", v.aPrimeKickRest}, {"aura_motif_a", v.auraMotifA},
                {"aura_motif_b", v.auraMotifB},     {"b_harmony", v.bHarmony}};
}

json pieceJson(const ListeningPiece& p) {
    return json{{"file", p.file},
                {"seed", p.seed},
                {"variant", variantJson(p.variant)},
                {"probe_passed", p.probePassed},
                {"failure_reason", p.failureReason},
                {"golden_pcm_status", p.goldenPcmStatus},
                {"pcm_content_hash", std::to_string(p.pcmContentHash)},
                {"integrated_lufs", p.integratedLufs ? json(*p.integratedLufs) : json(nullptr)},
                {"true_peak_dbtp", p.truePeakDbtp},
                {"duration_s", p.durationS}};
}

ListeningPiece renderPiece(std::uint64_t seed, const audio::FixtureVariant& variant, const std::string& outputDir,
                           std::vector<float>& stereo, StemCapture& stems) {
    ListeningPiece p;
    p.seed = seed;
    p.variant = variant;
    stereo.clear();
    audio::ProbeAnalysisCapture pc{};
    pc.fixtureSeed = seed;
    pc.stereo = &stereo;
    pc.fixtureVariant = variant;
    stems.reset();
    pc.stems = stems.sink(); // role stems for the form measures of the expert production analysis (S1)
    const auto probe = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);
    p.probePassed = probe.passed;
    p.failureReason = probe.failureReason;
    p.goldenPcmStatus = probe.goldenPcmStatus;
    p.pcmContentHash = probe.pcmContentHash;
    p.truePeakDbtp = probe.truePeakDbtp;
    p.durationS = static_cast<double>(stereo.size() / 2) / kFs;
    const bool finite = !stereo.empty() && std::all_of(stereo.begin(), stereo.end(), [](float x) { return std::isfinite(x); });
    if (finite) {
        const LoudnessResult r = measureLoudnessStereo(stereo, kFs);
        if (r.status == AnalysisStatus::Ok) p.integratedLufs = r.integratedLufs;
    }
    return p;
}

bool writeWav(const std::filesystem::path& path, std::span<const float> pcm) {
    const auto bytes = encodeWavStereo16(pcm, kFs);
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return out.good() && !pcm.empty();
}

} // namespace

std::optional<std::uint64_t> secondListeningSeed(std::uint64_t first, std::uint64_t last) {
    for (std::uint64_t s = first; s <= last; ++s) {
        const auto v = audio::MusicPlanner::drawFixtureVariant(s, audio::kPlannerPolicyVersion);
        if (v.bHarmony == 2 && v.auraMotifA && v.auraMotifB && v.aTramaEntry != 4 && v.bDisplacementStart != 12) return s;
    }
    return std::nullopt;
}

std::string CumulativeListeningResult::toJson() const {
    return json{{"scenario", "cumulative_listening_v1"},
                {"status", status},
                {"reasons", reasons},
                {"design", "docs/research/M2_5_Cumulative_Listening_Hypotheses.md (D-M2-50)"},
                {"piece_1_reference", pieceJson(reference)},
                {"piece_2_second_path", pieceJson(second)},
                {"section_start_s", {{"B", sectionStartS[0]}, {"Aprime", sectionStartS[1]}}},
                {"manifest", manifestFilePath}}
        .dump(2);
}

CumulativeListeningResult runCumulativeListeningV1(const std::string& outputDir, bool writeFiles) {
    namespace fs = std::filesystem;
    CumulativeListeningResult r;
    const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();
    if (clock) {
        const auto form = audio::MusicPlanner::makeFixtureM24V1Form();
        r.sectionStartS = {static_cast<double>(clock->frameAtBar(form.sections[1].startBar)) / kFs,
                           static_cast<double>(clock->frameAtBar(form.sections[2].startBar)) / kFs};
    } else {
        r.reasons.push_back("fixture clock unavailable");
    }
    const auto seed2 = secondListeningSeed();
    if (!seed2) {
        r.status = "INCONCLUSIVE";
        r.reasons.push_back("no seed in [1, 64] satisfies the declared rule");
        return r;
    }
    const fs::path dir = fs::path(outputDir) / "cumulative_listening_v1";
    std::error_code ec;
    if (writeFiles) fs::create_directories(dir, ec);

    std::vector<float> stereo;
    StemCapture stems(kStemCaptureFrames);
    static constexpr std::array<const char*, 4> kStemRoleNames{"pulso", "corpo", "trama", "vettore"};
    const auto writeStems = [&](const std::string& prefix) {
        if (stems.overflowFrames() > 0) r.reasons.push_back(prefix + ": stem capture overflow");
        for (std::size_t role = 0; role < kStemRoleNames.size(); ++role) {
            const std::string name = prefix + "_" + kStemRoleNames[role] + ".wav";
            if (!writeWav(dir / name, stems.stem(role))) r.reasons.push_back("cannot write " + name);
        }
    };
    r.reference = renderPiece(kReferenceSeed, audio::kFixtureReferenceVariant, outputDir, stereo, stems);
    r.reference.file = "brano_riferimento.wav";
    if (!r.reference.probePassed) r.reasons.push_back("reference render failed: " + r.reference.failureReason);
    if (r.reference.goldenPcmStatus != "PASS") r.reasons.push_back("reference PCM golden " + r.reference.goldenPcmStatus);
    if (writeFiles && !writeWav(dir / r.reference.file, stereo)) r.reasons.push_back("cannot write " + r.reference.file);
    if (writeFiles) writeStems("stem_riferimento");

    const auto variant2 = audio::MusicPlanner::drawFixtureVariant(*seed2, audio::kPlannerPolicyVersion);
    r.second = renderPiece(*seed2, variant2, outputDir, stereo, stems);
    r.second.file = "brano_seed_" + std::to_string(*seed2) + ".wav";
    if (!r.second.probePassed) r.reasons.push_back("second render failed: " + r.second.failureReason);
    if (r.second.goldenPcmStatus != "NOT_APPLICABLE") r.reasons.push_back("second PCM golden " + r.second.goldenPcmStatus);
    if (writeFiles && !writeWav(dir / r.second.file, stereo)) r.reasons.push_back("cannot write " + r.second.file);
    if (writeFiles) writeStems("stem_seed_" + std::to_string(*seed2));

    r.status = r.reasons.empty() ? "MEASURED" : "INCONCLUSIVE";
    if (writeFiles) {
        r.manifestFilePath = (dir / "cumulative_listening_v1_manifest.json").string();
        std::ofstream f(r.manifestFilePath);
        f << r.toJson();
        r.manifestWriteSuccess = f.good();
    }
    return r;
}

} // namespace cymatica::cacp
