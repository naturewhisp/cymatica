#pragma once

#include "release_ab.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario trama_miss_diagnosis_v1 (Inc-5 phase 5e.3b, ADR-0006 "5e.3b"): causal diagnosis of the Trama references
// of synthetic set B missed at 2 ms by the frozen detector. Offline, MEASURED, never a gate; DSP, descriptors_v1 and
// the frozen profile are unchanged and nothing is tuned. Predictions and rules fixed in ADR-0006 before measuring:
//   P1: at 2 ms the global maximum of the raw flux lies within +-50 ms of a Trama NoteOff and
//       max F(2 ms) >= 1.2 x max F(30 ms).
//   P2: renormalising the 2 ms flux by its maximum outside +-50 ms of the Trama NoteOffs gives recall >= 0.95;
//       control: the same recomputation at 30 ms keeps recall 1.000, otherwise INCONCLUSIVE.
//   P3: at least 8 of the 10 references missed at 2 ms are among the 12 lowest raw attack peaks of the 40; every raw
//       peak must be measured, finite and non-negative, otherwise INCONCLUSIVE (DIF-5e.3b-02).
//   P4 (alternative, minimum-separation merge): NOT_SUPPORTED when no missed reference has an above-threshold local
//       maximum in its window dropped by the merge; SUPPORTED when at least 5 have one; INCONCLUSIVE in between.
//   Causal (M-NORM): INCONCLUSIVE when any validity condition fails (capture, analysis, parity with
//       evaluateRoleOnsets, identical repeat; DIF-5e.3b-03). Otherwise SUPPORTED when P1, P2 and P3 are SUPPORTED and
//       P4 is NOT_SUPPORTED (DIF-5e.3b-01); NOT_SUPPORTED when P2 is NOT_SUPPORTED with a valid control; else INCONCLUSIVE.
// Strength: a retrospective mechanistic experiment on set B (the hypothesis came from 5e.3 data), not an independent
// confirmation on new data.

inline constexpr double kDiagWindowS = 0.050;          // matching and release windows (+-50 ms)
inline constexpr double kP1MinNormaliserRatio = 1.2;
inline constexpr double kP2MinRecall = 0.95;
inline constexpr std::size_t kP3LowestCount = 12;
inline constexpr std::size_t kP3MinMissedAmongLowest = 8;
inline constexpr std::size_t kP4MinMerged = 5;
inline constexpr std::size_t kExpectedMissed = 10;    // as measured in 5e.3 (release_ab_v1, set B, 2 ms)

struct TramaReferenceDiagnosis {
    std::size_t index{0};
    double timeS{0.0};
    float velocity{0.0f};
    float timbre{0.0f};
    std::optional<double> precedingIntervalS{};   // none for the first note
    std::optional<double> sincePrevNoteOffS{};     // none when no Trama NoteOff precedes the reference
    bool matched{false};                           // by the frozen detector, +-50 ms
    std::optional<double> peakNorm{};              // max of the normalised novelty in the window
    std::optional<double> peakRaw{};               // same frame, raw flux
    std::optional<double> thresholdAtPeak{};
    std::optional<double> marginAtPeak{};          // peakNorm - threshold
    bool peakIsLocalMax{false};
    bool mergedCandidate{false};                   // above-threshold local maximum in the window dropped by the merge
    bool counterfactualMatched{false};
};

struct TramaRenderDiagnosis {
    double releaseMs{0.0};
    std::uint32_t releaseFrames{0};
    std::string captureInvalid{};
    std::string analysisStatus{"NOT_RUN"};         // Ok when every step of the analysis was valid
    double normaliser{0.0};                        // max raw flux
    double globalMaxTimeS{0.0};                    // offset-corrected time of the global maximum
    bool globalMaxAtRelease{false};
    std::optional<double> globalMaxNearestReleaseS{};
    std::size_t references{0};
    std::size_t matched{0};
    std::optional<double> recall{};
    std::optional<double> recallFromEvaluator{};   // evaluateRoleOnsets on the same stem (consistency check)
    double counterfactualNormaliser{0.0};
    std::size_t counterfactualMatched{0};
    std::optional<double> counterfactualRecall{};
    std::vector<TramaReferenceDiagnosis> refs{};
    [[nodiscard]] bool valid() const noexcept { return captureInvalid.empty() && analysisStatus == "Ok"; }
};

struct PredictionOutcome {
    std::string outcome{"NOT_RUN"};                // SUPPORTED, NOT_SUPPORTED, INCONCLUSIVE
    std::string detail{};
};

// Decision rules (pure, testable)
[[nodiscard]] PredictionOutcome evaluateP1(const TramaRenderDiagnosis& at2, const TramaRenderDiagnosis& at30);
[[nodiscard]] PredictionOutcome evaluateP2(const TramaRenderDiagnosis& at2, const TramaRenderDiagnosis& at30);
[[nodiscard]] PredictionOutcome evaluateP3(const TramaRenderDiagnosis& at2);
[[nodiscard]] PredictionOutcome evaluateP4(const TramaRenderDiagnosis& at2);
// validityFailures: the reasons a mandatory validity condition failed (empty when the evidence is valid)
[[nodiscard]] PredictionOutcome evaluateCausal(const PredictionOutcome& p1, const PredictionOutcome& p2, const PredictionOutcome& p3,
                                               const PredictionOutcome& p4, const std::vector<std::string>& validityFailures);

struct TramaDiagnosisResult {
    std::string status{"NOT_RUN"};                 // MEASURED or INCONCLUSIVE with reasons
    std::vector<std::string> reasons{};
    TramaRenderDiagnosis at2{}, at30{};
    PredictionOutcome p1{}, p2{}, p3{}, p4{}, causal{};
    bool repeatIdentical{false};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    std::string curvesFilePath{};
    bool curvesWriteSuccess{false};
    [[nodiscard]] std::string measuresJson() const; // deterministic part (compared on repeat)
    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] TramaDiagnosisResult runTramaMissDiagnosisV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
