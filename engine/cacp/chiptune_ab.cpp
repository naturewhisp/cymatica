#include "chiptune_ab.h"

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "evaluation_pack.h"
#include "loudness.h"
#include "music_planner.h"
#include "release_ab.h"
#include "stem_analysis.h"
#include "timbre_descriptors.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <span>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;
constexpr std::size_t kFixtureCaptureFrames = 8'900'000; // > 8,825,200 frames of fixture_probe_v1
constexpr double kClipFadeS = 0.010;
constexpr double kLoudnessToleranceLu = 1.0;               // X4
constexpr double kMinRoleAccuracy = 0.75;                  // X5
constexpr std::array<const char*, 3> kSectionNames{"A", "B", "Aprime"};
// Isolated notes of measurement package 5 (timbre_descriptors.cpp): same times, pitches and planner timbres
constexpr double kNoteS = 0.300;
constexpr double kTailS = 0.050;
constexpr double kSpacingS = 1.0;
constexpr double kFirstS = 0.5;

json optNum(const std::optional<double>& v) { return (v && std::isfinite(*v)) ? json(*v) : json(nullptr); }

std::optional<double> integrated(std::span<const float> pcm) {
    const LoudnessResult r = measureLoudnessStereo(pcm, kFs);
    if (r.status != AnalysisStatus::Ok) return std::nullopt;
    return r.integratedLufs;
}

std::span<const float> frameRange(std::span<const float> pcm, std::uint64_t start, std::uint64_t end) {
    if (end <= start || 2 * end > pcm.size()) return {};
    return pcm.subspan(static_cast<std::size_t>(2 * start), static_cast<std::size_t>(2 * (end - start)));
}

std::vector<float> fadedSlice(std::span<const float> stereo, std::uint64_t start, std::uint64_t end) {
    const auto r = frameRange(stereo, start, end);
    std::vector<float> out(r.begin(), r.end());
    const std::size_t n = out.size() / 2;
    const auto fade = static_cast<std::size_t>(std::llround(kClipFadeS * kFs));
    constexpr double kPi = 3.14159265358979323846;
    for (std::size_t k = 0; k < fade && k < n; ++k) {
        const auto g = static_cast<float>(0.5 * (1.0 - std::cos(kPi * (static_cast<double>(k) + 0.5) / static_cast<double>(fade))));
        for (std::size_t ch = 0; ch < 2; ++ch) {
            out[2 * k + ch] *= g;
            out[2 * (n - 1 - k) + ch] *= g;
        }
    }
    return out;
}

struct ClipRange {
    std::uint32_t section{0};
    std::uint64_t start{0};
    std::uint64_t end{0};
};

// Phrases 3 and 4 of each section (bars 9-16 of the section), as in kick_level_ab_v1
std::vector<ClipRange> clipRanges(const std::array<audio::PhrasePlan, 24>& phrases) {
    std::map<std::uint32_t, std::vector<const audio::PhrasePlan*>> bySection;
    for (const auto& p : phrases) bySection[p.sectionIndex].push_back(&p);
    std::vector<ClipRange> out;
    for (auto& [section, list] : bySection) {
        std::sort(list.begin(), list.end(), [](const auto* a, const auto* b) { return a->startFrame < b->startFrame; });
        if (list.size() >= 4) out.push_back({section, list[2]->startFrame, list[3]->endFrame});
    }
    return out;
}

bool writeWav(const std::filesystem::path& path, std::span<const float> pcm) {
    const auto bytes = encodeWavStereo16(pcm, kFs);
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return out.good() && !pcm.empty();
}

ChiptuneRender renderFixture(audio::TimbrePalette palette, StemCapture& cap, std::vector<float>& stereo, const std::string& outputDir) {
    ChiptuneRender m;
    cap.reset();
    std::vector<AppliedEventRecord> applied;
    std::uint64_t appliedOverflow = 0;
    audio::ProbeAnalysisCapture pc{};
    pc.stems = cap.sink();
    pc.applied = &applied;
    pc.appliedOverflow = &appliedOverflow;
    pc.stereo = &stereo;
    pc.timbrePalette = palette;
    const auto probe = audio::runFixtureProbeV1Offline(outputDir, false, audio::FixtureInjectedFault::None, &pc);
    const CaptureCheck c{true, probe.passed, probe.failureReason, static_cast<std::size_t>(probe.requestedSampleFrames),
                         cap.frames(), cap.overflowFrames(), appliedOverflow};
    m.captureInvalid = captureInvalidReason(c);
    m.goldenPcmStatus = probe.goldenPcmStatus;
    m.pcmContentHash = probe.pcmContentHash;
    m.truePeakDbtp = probe.truePeakDbtp;
    m.maxGainReductionDb = probe.maxGainReductionDb;
    m.finite = !stereo.empty() && std::all_of(stereo.begin(), stereo.end(), [](float x) { return std::isfinite(x); });
    if (m.captureInvalid.empty()) {
        m.mixLufs = integrated(stereo);
        for (std::size_t role = 0; role < 4; ++role) m.stemLufs[role] = integrated(cap.stem(role));
    }
    return m;
}

// Package 5 role notes (Corpo, Trama, Vettore) with one palette; accuracies Trama-Corpo, Trama-Vettore, Corpo-Vettore
std::optional<std::array<double, 3>> roleAccuracies(audio::TimbrePalette palette, std::vector<std::string>& reasons) {
    const std::array<audio::MusicRole, 3> roles{audio::MusicRole::Corpo, audio::MusicRole::Trama, audio::MusicRole::Vettore};
    const std::array<float, 3> timbres{0.40f, 0.50f, 0.60f};
    std::array<std::vector<TimbreVector>, 3> notesOf{};
    StemCapture cap(kFs * 10);
    for (std::size_t ri = 0; ri < roles.size(); ++ri) {
        std::vector<SynthNote> notes;
        for (std::size_t p = 0; p < kTimbrePitchesHz.size(); ++p) {
            notes.push_back(SynthNote{kFirstS + kSpacingS * static_cast<double>(p), kTimbrePitchesHz[p], 0.8f, kNoteS, timbres[ri]});
        }
        const SyntheticRender sr = renderSyntheticWithRelease(roles[ri], notes, cap, 0, palette);
        if (!captureInvalidReason(sr.check).empty()) {
            reasons.push_back("timbre notes: invalid capture: " + captureInvalidReason(sr.check));
            return std::nullopt;
        }
        const auto stem = cap.stem(static_cast<std::size_t>(roles[ri]));
        for (std::size_t p = 0; p < notes.size(); ++p) {
            const auto s0 = static_cast<std::size_t>(std::llround(notes[p].timeS * kFs));
            const auto s1 = s0 + static_cast<std::size_t>(std::llround((kNoteS + kTailS) * kFs));
            std::vector<float> mono(s1 - s0);
            for (std::size_t i = s0; i < s1 && 2 * i + 1 < stem.size(); ++i) mono[i - s0] = 0.5f * (stem[2 * i] + stem[2 * i + 1]);
            notesOf[ri].push_back(timbreDescriptors(mono, notes[p].pitchHz));
        }
    }
    return std::array<double, 3>{looNearestCentroidAccuracy(notesOf[1], notesOf[0]),
                                 looNearestCentroidAccuracy(notesOf[1], notesOf[2]),
                                 looNearestCentroidAccuracy(notesOf[0], notesOf[2])};
}

} // namespace

std::string ChiptuneAbResult::toJson() const {
    const auto renderJson = [](const ChiptuneRender& m) {
        json stems = json::array();
        for (const auto& s : m.stemLufs) stems.push_back(optNum(s));
        return json{{"capture_invalid", m.captureInvalid}, {"golden_pcm_status", m.goldenPcmStatus},
                    {"pcm_content_hash", std::to_string(m.pcmContentHash)}, {"finite", m.finite}, {"mix_lufs", optNum(m.mixLufs)},
                    {"stem_lufs_pulso_corpo_trama_vettore", stems}, {"true_peak_dbtp", m.truePeakDbtp},
                    {"max_gain_reduction_db", m.maxGainReductionDb}};
    };
    json j{{"scenario", "chiptune_ab_v1"},
           {"preregistration", "docs/research/M2_5_Chiptune_Palette_Spike_Preregistration.md"},
           {"status", status},
           {"reasons", reasons},
           {"gate", false},
           {"production", renderJson(production)},
           {"chiptune", renderJson(chiptune)},
           {"x3_chiptune_repeat_identical", chiptuneRepeatIdentical},
           {"x4_stem_delta_lu", stemDeltaLu},
           {"x4_mix_delta_lu", mixDeltaLu},
           {"x4_loudness_matched", x4LoudnessMatched},
           {"calibration_suggested_gain_db", suggestedGainDb},
           {"calibration_current_gain_db", audio::kChiptuneRoleGainDb},
           {"x5_accuracy_production_tc_tv_cv", accProduction},
           {"x5_accuracy_chiptune_tc_tv_cv", accChiptune},
           {"x5_roles_distinct", x5RolesDistinct},
           {"clips", clips}};
    return j.dump(2);
}

ChiptuneAbResult runChiptuneAbV1(const std::string& outputDir, bool writeFiles) {
    namespace fs = std::filesystem;
    ChiptuneAbResult r;
    const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();
    if (!clock) {
        r.status = "INCONCLUSIVE";
        r.reasons.emplace_back("fixture clock unavailable");
        return r;
    }
    const auto phrases = audio::MusicPlanner::makeFixtureM24V1Phrases(*clock);
    StemCapture cap(kFixtureCaptureFrames);
    std::vector<float> stereoProd, stereoChip, stereoRepeat;
    r.production = renderFixture(audio::TimbrePalette::Production, cap, stereoProd, outputDir);
    r.chiptune = renderFixture(audio::TimbrePalette::Chiptune8Bit, cap, stereoChip, outputDir);
    const ChiptuneRender repeat = renderFixture(audio::TimbrePalette::Chiptune8Bit, cap, stereoRepeat, outputDir);
    r.chiptuneRepeatIdentical = repeat.pcmContentHash == r.chiptune.pcmContentHash && stereoRepeat == stereoChip;
    stereoRepeat.clear();
    stereoRepeat.shrink_to_fit();
    for (const auto* m : {&r.production, &r.chiptune}) {
        if (!m->captureInvalid.empty()) r.reasons.push_back("invalid capture: " + m->captureInvalid);
    }

    bool matched = r.production.mixLufs && r.chiptune.mixLufs;
    if (matched) {
        r.mixDeltaLu = *r.chiptune.mixLufs - *r.production.mixLufs;
        matched = std::fabs(r.mixDeltaLu) <= kLoudnessToleranceLu;
    }
    for (std::size_t role = 0; role < 4; ++role) {
        if (!r.production.stemLufs[role] || !r.chiptune.stemLufs[role]) {
            matched = false;
            continue;
        }
        r.stemDeltaLu[role] = *r.chiptune.stemLufs[role] - *r.production.stemLufs[role];
        r.suggestedGainDb[role] = std::round((audio::kChiptuneRoleGainDb[role] - r.stemDeltaLu[role]) * 2.0) / 2.0;
        matched = matched && std::fabs(r.stemDeltaLu[role]) <= kLoudnessToleranceLu;
    }
    r.x4LoudnessMatched = matched;

    const auto accProd = roleAccuracies(audio::TimbrePalette::Production, r.reasons);
    const auto accChip = roleAccuracies(audio::TimbrePalette::Chiptune8Bit, r.reasons);
    if (accProd) r.accProduction = *accProd;
    if (accChip) {
        r.accChiptune = *accChip;
        r.x5RolesDistinct = std::all_of(accChip->begin(), accChip->end(), [](double a) { return a >= kMinRoleAccuracy; });
    }

    if (writeFiles && r.production.captureInvalid.empty() && r.chiptune.captureInvalid.empty()) {
        std::error_code ec;
        const fs::path dir = fs::path(outputDir) / "chiptune_ab_v1";
        fs::create_directories(dir, ec);
        for (const auto& cr : clipRanges(phrases)) {
            const std::string sec = cr.section < kSectionNames.size() ? kSectionNames[cr.section] : "S";
            const auto prod = fadedSlice(stereoProd, cr.start, cr.end);
            auto chip = fadedSlice(stereoChip, cr.start, cr.end);
            const auto lp = integrated(prod);
            const auto lc = integrated(chip);
            if (lp && lc) {
                const double g = std::pow(10.0, (*lp - *lc) / 20.0);   // chiptune clip aligned to the production clip
                for (auto& x : chip) x = static_cast<float>(static_cast<double>(x) * g);
            }
            const std::string a = "chip_" + sec + "_production.wav";
            const std::string b = "chip_" + sec + "_chiptune_aligned.wav";
            if (writeWav(dir / a, prod)) r.clips.push_back(a);
            if (writeWav(dir / b, chip)) r.clips.push_back(b);
        }
        if (writeWav(dir / "chip_full_chiptune.wav", stereoChip)) r.clips.push_back("chip_full_chiptune.wav");
    }

    r.status = r.reasons.empty() ? "MEASURED" : "INCONCLUSIVE";
    if (writeFiles) {
        std::error_code ec;
        fs::create_directories(outputDir, ec);
        r.manifestFilePath = (fs::path(outputDir) / "chiptune_ab_v1_manifest.json").string();
        std::ofstream f(r.manifestFilePath);
        f << r.toJson();
        r.manifestWriteSuccess = f.good();
    }
    return r;
}

} // namespace cymatica::cacp
