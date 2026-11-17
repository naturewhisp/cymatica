#pragma once

#include "stem_analysis.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Measurement package 2 (docs/research/M2_5_Pkg2_Discontinuity_Detector_Preregistration.md): discontinuity (click)
// detector A1 on the rendered PCM. Linear predictor of order 16 per 1024-sample block (Hann autocorrelation,
// Levinson-Durbin), residual with the true preceding samples, robust block scale 1.4826 * median|e| (floor 1e-6),
// candidate where |e| / scale > 10, candidates within 2 ms fused. Offline, MEASURED, never a gate.
inline constexpr std::size_t kLpcOrder = 16;
inline constexpr std::size_t kLpcBlock = 1024;
inline constexpr double kResidualThreshold = 10.0;
inline constexpr double kScaleFloor = 1e-6;
inline constexpr std::size_t kMergeFrames = 96;          // 2 ms at 48 kHz
inline constexpr std::size_t kAnnotateFrames = 240;      // +-5 ms at 48 kHz

// Candidate frames of one channel (deinterleaved mono buffer)
[[nodiscard]] std::vector<std::size_t> discontinuityCandidates(std::span<const float> channel);
// Candidates of an interleaved stereo buffer: union of the two channels, fused within 2 ms
[[nodiscard]] std::vector<std::size_t> discontinuityCandidatesStereo(std::span<const float> interleaved);
// A2: candidates of the difference B - A of two interleaved stereo renders of the same length (empty on mismatch)
[[nodiscard]] std::vector<std::size_t> differentialCandidates(std::span<const float> a, std::span<const float> b);

enum class CandidateCategory : std::uint8_t { Attack = 0, Release = 1, OtherEvent = 2, Unexplained = 3 };
inline constexpr std::array<const char*, 4> kCandidateCategoryNames{"attack", "release", "other_event", "unexplained"};

// Category of a candidate from the applied-event log: nearest record within +-5 ms (role filter when roleFilter >= 0)
[[nodiscard]] CandidateCategory annotateCandidate(std::size_t frame, std::span<const AppliedEventRecord> applied, int roleFilter);

struct DiscontinuityResult {
    std::string status{"NOT_RUN"};            // MEASURED, TOOL_NOT_VALIDATED (W1-W5 failed) or INCONCLUSIVE
    std::vector<std::string> reasons{};
    // descriptive table: [stem 0..3, mix 4][category]
    std::array<std::array<std::size_t, 4>, 5> table{};
    std::array<std::vector<std::size_t>, 5> unexplainedFrames{};   // first frames of unexplained candidates (up to 50)
    // validation
    std::array<double, 3> impulseRecall{};    // -60, -40, -20 dBFS
    std::array<double, 3> stepRecall{};
    std::size_t sineCandidates{0};            // W2
    bool w5IdenticalZero{false};
    bool w5InjectedFound{false};
    bool repeatIdentical{false};               // W4
    std::string goldenPcmStatus{};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string tableJson() const;
    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] DiscontinuityResult runDiscontinuityV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
