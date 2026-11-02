#pragma once

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "evaluation_pack.h"
#include "stem_analysis.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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
    std::optional<double> truePeakDbtp{};      // distinct from the loudness validity (DIF-5e.1-R2-01)
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

// Status of a measure (DIF-5e.1-02): MEASURED only when the analysis ran on valid data and produced its values;
// INCONCLUSIVE with the reason when it ran but could not produce them; NOT_RUN when it never ran.
struct MeasureStatus {
    std::string status{"NOT_RUN"};
    std::string reason{};
};

struct ProceduralResult {
    std::string status{"NOT_RUN"};               // PASS or FAIL (binding checks only)
    bool rendered{false};
    std::optional<audio::FixtureProbeResult> fixture{}; // binding evidence of the render (DIF-5e.1-03)
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
    bool truePeakMeasured{false};
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

// Derived statuses of the measures (DIF-5e.1-02), from the recorded results only
struct ProceduralMeasureStatuses {
    MeasureStatus truePeak{};
    MeasureStatus loudness{};
    std::vector<MeasureStatus> sections{};          // loudness of each section
    std::vector<MeasureStatus> sectionTruePeaks{};  // true peak of each section, independent of its loudness
    MeasureStatus onsets{};
    MeasureStatus presence{};
    MeasureStatus overall{};   // MEASURED if every measure is; NOT_RUN if none ran; INCONCLUSIVE otherwise
};
[[nodiscard]] ProceduralMeasureStatuses proceduralMeasureStatuses(const ProceduralResult& r);
// Effective status of one role's onsets (DIF-5e.1-R3-01), used both for the aggregate and for the role's own
// serialisation: MEASURED only with finite precision and recall; NO_REFERENCES kept (no evidence, not an error);
// NOT_RUN kept; anything else, or MEASURED without finite values, is INCONCLUSIVE with the reason.
[[nodiscard]] MeasureStatus roleOnsetMeasureStatus(const RoleOnsetSummary& s);
// MEASURED only with analysis status OK and a finite Integrated value; Momentary and Short-term maxima, when
// present, must be finite too (DIF-5e.1-R2-01). Values of a measure that is not MEASURED are serialised as null.
[[nodiscard]] MeasureStatus loudnessMeasureStatus(const std::string& analysisStatus, const std::optional<double>& integrated,
                                                  const std::optional<double>& maxMomentary = std::nullopt,
                                                  const std::optional<double>& maxShortTerm = std::nullopt);

[[nodiscard]] ProceduralResult runProceduralV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
