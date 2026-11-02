#pragma once

#include "acao02_calibration.h"
#include "evaluation_pack.h"
#include "stem_analysis.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cymatica::audio {
struct FixtureProbeResult;
}

namespace cymatica::cacp {

// Scenario procedural_v1 (ADR-0006 precisions 5e-01, 5e-05, 5e-06; Inc-5 phase 5e.1): fixture-m2-4-v1, seed
// 1337, about 183 s and three sections, rendered once with stems and event logs, with the whole analysis chain
// in one manifest. Binding (PASS/FAIL): O3 chain, determinism and goldens, PCM integrity and the approved
// limiter invariants, capture validity, exact bus-mix reconstruction. MEASURED, never PASS: true peak against
// the -1 dBTP trial value (an excess is a finding, DIF-5e.0-01), LUFS, per-role onsets with the frozen profile,
// per-phrase presence. Seed diversity belongs to phase 5e.2 and is NOT_RUN here.

// Binding verdict of the render: every check of fixture_probe_v1 except its true-peak engineering check
// (ADR-0005 D-M2-09), which procedural_v1 reports as a measure. Empty when every binding check passes.
[[nodiscard]] std::vector<std::string> proceduralBindingFailures(const audio::FixtureProbeResult& fixture);

struct SectionMeasures {
    std::uint32_t section{0};
    std::uint64_t startFrame{0};
    std::uint64_t endFrame{0};
    std::string loudnessStatus{"NOT_RUN"};
    std::optional<double> integratedLufs{};
    std::optional<double> maxMomentaryLufs{};
    std::optional<double> maxShortTermLufs{};
    double truePeakDbtp{0.0};
};

struct RoleOnsetSummary {
    std::string status{"NOT_RUN"};
    std::size_t references{0};
    std::size_t detections{0};
    std::size_t matched{0};
    std::size_t releaseDetections{0};
    std::optional<double> precision50{};
    std::optional<double> recall50{};
    std::optional<double> precisionExclReleases{};
};

struct ProceduralResult {
    std::string status{"NOT_RUN"};               // PASS or FAIL (binding checks only)
    std::vector<std::string> failures{};
    std::vector<std::string> findings{};         // measured facts worth attention, never failures
    bool fixtureProbePassed{false};              // verdict of fixture_probe_v1, true-peak check included
    std::uint64_t pcmContentHash{0};
    std::uint64_t canonicalPlanHash{0};
    std::uint64_t contentFingerprint{0};
    std::uint64_t renderedFrames{0};
    std::uint64_t contentEndFrame{0};
    CaptureCheck capture{};
    std::size_t reconstructionMismatches{0};
    std::string reconstructionStatus{"NOT_RUN"};
    // Measures (MEASURED)
    double truePeakDbtp{0.0};
    double maxGainReductionDb{0.0};
    std::string loudnessStatus{"NOT_RUN"};
    std::optional<double> integratedLufs{};
    std::optional<double> maxMomentaryLufs{};
    std::optional<double> maxShortTermLufs{};
    std::vector<SectionMeasures> sections{};
    std::array<RoleOnsetSummary, kRoleCount> onsets{};
    std::string onsetProfileSha256{};
    std::vector<WindowTruth> phrasePresence{};
    std::vector<std::string> staffViews{};
    std::string staffDir{};
    std::string wavFilePath{};
    bool wavWriteSuccess{false};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};

    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] ProceduralResult runProceduralV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
