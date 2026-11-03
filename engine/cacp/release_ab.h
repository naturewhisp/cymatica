#pragma once

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "stem_analysis.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario release_ab_v1 (ADR-0006 precision 5e-08, Inc-5 phase 5e.3): controlled experiment on the release of
// ordinary NoteOffs. Only the release varies (offline override); seeds, events and rendering conditions are the
// same in every variant and verified; production keeps the 2 ms de-click. MEASURED, never a gate; no production
// release is adopted in Inc-5 (decision at M2.5). Decision rules fixed here, before any measurement:
//   H1 (release detections): SUPPORTED when at 30 ms the release detections are <= 20 % of those at 2 ms in each
//      of set B, planner_probe_v1 and the fixture (sum over the MEASURED roles; NO_REFERENCES adds nothing; any
//      other role status, an invalid capture, a missing measure or a 2 ms baseline of 0 give INCONCLUSIVE).
//   H2 (Trama recall, set B): SUPPORTED when the Trama recall at 30 ms is >= 0.90 AND greater than at 2 ms, both
//      MEASURED on valid captures (DIF-5e.3-04); otherwise NOT_SUPPORTED, or INCONCLUSIVE without both measures.
//   R2 short-IOI sub-hypothesis (2 ms diagnostic, DIF-5e.3-05): SUPPORTED when the unmatched Trama references of
//      set B are exactly the notes preceded by the 0.18 s and 0.22 s intervals (10 of 40). It tests only that
//      sub-hypothesis of the 5c explanation; the causal explanation as a whole stays INCONCLUSIVE (phase 5e.3b).
// H1 and H2 are distinct hypotheses with their own measure and outcome; neither is assumed before measuring.

inline constexpr std::array<double, 5> kReleaseVariantsMs{2.0, 15.0, 30.0, 50.0, 80.0};
inline constexpr double kH1ReleaseMs = 30.0;
inline constexpr double kH1MaxRatio = 0.20;
inline constexpr double kH2MinRecall = 0.90;
inline constexpr std::array<std::size_t, 2> kReleaseClipPhrases{5, 10};   // fixture phrases for the A/B clips

struct ReleaseRoleMeasure {
    std::string status{"NOT_RUN"};
    std::size_t references{0};
    std::size_t detections{0};
    std::size_t matched{0};
    std::size_t releaseDetections{0};
    std::optional<double> precision50{};
    std::optional<double> recall50{};
    std::optional<double> precisionExclReleases{};
};

struct ReleaseSetMeasure {
    std::string set{};                       // set_a, set_b, planner_probe_v1, fixture
    double releaseMs{0.0};
    std::uint32_t releaseFrames{0};          // 0 = production (2 ms de-click)
    std::string captureInvalid{};            // empty when the capture is valid
    std::uint64_t eventsHash{0};             // identity, consumed frame and disposition of the applied events
    std::uint64_t pcmHash{0};                // fixture: FNV-1a of the final PCM (repeat check); 0 for the other sets
    std::array<ReleaseRoleMeasure, kRoleCount> roles{};
    audio::VoiceConfounders confounders{};   // totals and per role (DIF-5e.3-03)
    // Release detections summed over the MEASURED roles only (DIF-5e.3-06)
    [[nodiscard]] std::size_t releaseDetectionsTotal() const noexcept;
    // Valid capture and every role MEASURED or NO_REFERENCES
    [[nodiscard]] bool analysesValid() const noexcept;
};

struct HypothesisOutcome {
    std::string outcome{"NOT_RUN"};          // SUPPORTED, NOT_SUPPORTED, INCONCLUSIVE
    std::string detail{};
};

struct ReleaseAbResult {
    std::string status{"NOT_RUN"};           // MEASURED (valid data, same events) or INCONCLUSIVE with reasons
    std::vector<std::string> reasons{};
    std::vector<ReleaseSetMeasure> measures{};
    std::vector<std::string> sameEventsViolations{};
    HypothesisOutcome h1{}, h2{}, r2ShortIoi{};
    HypothesisOutcome r2Causal{"INCONCLUSIVE", "causal explanation of the unmatched Trama references not tested here "
                                               "(velocity, timbre, NoteOff distance, flux normalisation: phase 5e.3b)"};
    std::vector<std::size_t> r2UnmatchedTrama{};     // indices of the unmatched Trama references, set B, 2 ms
    std::vector<double> r2PrecedingIntervalS{};      // interval before each of them
    std::vector<std::string> clips{};
    bool repeatIdentical{false};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string measuresJson() const;  // deterministic part (compared on repeat)
    [[nodiscard]] std::string toJson() const;
};

// Same-events check: every variant of a set has the events hash of its 2 ms variant (a missing baseline is a violation)
[[nodiscard]] std::vector<std::string> sameEventsViolations(const std::vector<ReleaseSetMeasure>& measures);

// Decision rules on recorded measures (pure, testable)
[[nodiscard]] HypothesisOutcome evaluateH1(const std::vector<ReleaseSetMeasure>& measures);
[[nodiscard]] HypothesisOutcome evaluateH2(const std::vector<ReleaseSetMeasure>& measures);
[[nodiscard]] HypothesisOutcome evaluateR2ShortIoi(const std::vector<std::size_t>& unmatched,
                                                   const std::vector<double>& precedingIntervalS, std::size_t totalReferences);

[[nodiscard]] ReleaseAbResult runReleaseAbV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
