#pragma once

#include "onset.h"
#include "stem_analysis.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario acao02_v1 (ADR-0006 D-M2-13/14/16, Inc-5 phase 5c): per-role onset evaluation on the offline stems,
// with the calibration / verification separation of R6. Order of the procedure, fixed in advance:
//   a. intrinsic latency of the detector on synthetic per-role notes rendered by the real synth (set A);
//   b. calibration of delta and constant offset per role on fixture-m2-4-v1, seed 1337 only;
//   c. comparison of the calibrated profile with the FROZEN profile below (versioned, hashed, set before (d));
//   d. verification with the frozen profile, never re-tuned, on: (1) fixture seeds whose musical content
//      fingerprint differs from seed 1337 (declared scan 1..32), (2) planner_probe_v1, (3) synthetic per-role
//      notes with timings, densities and pitches different from set A (set B);
//   e. bus-mix reconstruction from the stems and stem presence measured on the PCM (OSS-5b-01).
// Everything is MEASURED; thresholds are only proposed (D-M2-16) and ratified at the M2.5 gate.

// Calibration grid and selection rule (declared before the measurement)
inline constexpr std::array<double, 8> kCalibrationDeltas{0.02, 0.03, 0.05, 0.07, 0.10, 0.15, 0.20, 0.30};
// Selection per role: maximum F at +-50 ms after subtracting the median signed error (nearest detection within
// +-100 ms) measured at the same delta; ties (|dF| < 1e-9) go to the larger delta (fewer false positives).

// Frozen profile stem_onset_profile_v1 (per role: delta, offset in seconds). Empty until the first calibration
// run has been reviewed and the values copied here; the verification runs only with a frozen profile.
struct FrozenStemProfile {
    std::array<RoleOnsetProfile, kRoleCount> roles{};
    const char* version{"stem_onset_profile_v1"};
};
[[nodiscard]] std::optional<FrozenStemProfile> frozenStemProfileV1() noexcept;
// SHA-256 of the canonical text of a profile ("role delta offset_us" per line)
[[nodiscard]] std::string stemProfileSha256(const std::array<RoleOnsetProfile, kRoleCount>& roles);

struct CalibrationGridPoint {
    double delta{0.0};
    double offsetS{0.0};
    std::size_t detections{0};
    std::size_t pairs{0};
    double precision{0.0}, recall{0.0}, f{0.0};
};

struct RoleSetEvaluation {
    std::size_t references{0};
    std::size_t detections{0};
    double precision50{0.0}, recall50{0.0}, f50{0.0};
    double precision25{0.0}, recall25{0.0}, f25{0.0};
    double errMedianMs{0.0}, errP05Ms{0.0}, errP95Ms{0.0};
    std::size_t unmatched{0};
    std::size_t releaseDetections{0};
    double precisionExclReleases{0.0};
    double unmatchedLagMedianMs{0.0}, unmatchedLagP05Ms{0.0}, unmatchedLagP95Ms{0.0};
    std::string status{"NOT_RUN"};
};

struct VerificationSet {
    std::string name;
    std::string status{"NOT_RUN"};        // MEASURED, INCONCLUSIVE (with reason) or NOT_RUN
    std::string note{};
    std::array<RoleSetEvaluation, kRoleCount> roles{};
};

struct Acao02Result {
    bool completed{false};
    std::string status{"NOT_RUN"};
    // a. intrinsic latency (set A), default delta 0.10, no offset
    std::array<RoleSetEvaluation, kRoleCount> intrinsic{};
    // b. calibration on seed 1337
    bool fixturePassed{false};
    std::array<std::vector<CalibrationGridPoint>, kRoleCount> grid{};
    std::array<RoleOnsetProfile, kRoleCount> calibrated{};
    std::string calibratedSha256{};
    std::array<RoleSetEvaluation, kRoleCount> calibrationScores{};
    // c. frozen profile
    bool profileFrozen{false};
    std::string frozenSha256{};
    bool calibratedEqualsFrozen{false};
    // d. verification
    std::vector<std::uint64_t> scannedSeeds{};
    std::vector<std::uint64_t> distinctSeeds{};      // fingerprint != seed 1337
    std::size_t distinctFingerprints{0};             // among the scanned seeds, 1337 included
    std::vector<VerificationSet> verification{};
    // e. reconstruction and presence
    std::size_t stemFrames{0};
    std::uint64_t stemOverflowFrames{0};
    std::size_t stemBytes{0};
    std::size_t reconstructionMismatches{0};
    double reconstructionMaxAbsError{0.0};
    std::array<std::string, 8> tramaPresenceA{};     // Trama stem presence per phrase of section A (0..7)
    std::array<double, 8> tramaPeakDbfsA{};
    // proposed thresholds (D-M2-16)
    std::array<double, kRoleCount> proposedRecall{};
    std::array<double, kRoleCount> proposedPrecision{};
    std::string failureReason{};
    bool manifestWriteSuccess{false};
    std::string manifestFilePath{};

    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] Acao02Result runAcao02V1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
