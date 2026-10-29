#pragma once

#include "analysis_status.h"
#include "event_chain_log.h"
#include "onset.h"
#include "role_bus_processor.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace cymatica::cacp {

using audio::AppliedEventLog;
using audio::AppliedEventRecord;
using audio::EventDisposition;

// Per-role stem analysis for AC-AO-02 (ADR-0006 D-M2-13/14/16, Inc-5 phase 5c). Offline only.

inline constexpr std::size_t kRoleCount = 4;
inline constexpr std::array<const char*, kRoleCount> kRoleNames{"pulso", "corpo", "trama", "vettore"};

// Owner of the stem buffers handed to the engine through RoleStemSink
class StemCapture {
public:
    explicit StemCapture(std::size_t capacityFrames);
    [[nodiscard]] audio::RoleStemSink* sink() noexcept { return &sink_; }
    [[nodiscard]] std::size_t frames() const noexcept { return sink_.writtenFrames; }
    [[nodiscard]] std::uint64_t overflowFrames() const noexcept { return sink_.overflowFrames; }
    [[nodiscard]] std::span<const float> stem(std::size_t role) const noexcept {
        return {stems_[role].data(), sink_.writtenFrames * 2};
    }
    [[nodiscard]] std::span<const float> busMix() const noexcept { return {busMix_.data(), sink_.writtenFrames * 2}; }
    [[nodiscard]] std::size_t bytesAllocated() const noexcept;
    void reset() noexcept { sink_.writtenFrames = 0; sink_.overflowFrames = 0; }

private:
    std::array<std::vector<float>, kRoleCount> stems_{};
    std::vector<float> busMix_{};
    audio::RoleStemSink sink_{};
};

// Reconstruction of the bus mix from the stems (D-M2-13): the four stems are summed in the order of
// RoleBusProcessor (Pulso + Corpo + Trama + Vettore, float), which must reproduce busMix bit-exactly.
struct ReconstructionResult {
    AnalysisStatus status{AnalysisStatus::Ok};
    std::size_t frames{0};
    std::size_t mismatchedSamples{0};   // samples where the float sum differs from busMix
    double maxAbsError{0.0};
};
[[nodiscard]] ReconstructionResult reconstructBusMix(const StemCapture& capture) noexcept;

// Physical presence of a role measured on the stem PCM over [startFrame, endFrame) (OSS-5b-01): never inferred
// from a spectrogram status. ExactSilence when every sample is 0; otherwise Ok with peak and RMS in dBFS.
struct StemPresence {
    AnalysisStatus status{AnalysisStatus::Ok};
    double peakDbfs{-1.0e9};
    double rmsDbfs{-1.0e9};
    bool exactSilence{false};
};
[[nodiscard]] StemPresence measureStemPresence(std::span<const float> interleavedStereo, std::size_t startFrame,
                                               std::size_t endFrame) noexcept;

// Ground truth of a role (D-M2-14, dossier §4.3): frames of the Applied NoteOn records of the role, sorted,
// NoteOns closer than mergeS fused into the first one; returned in seconds.
[[nodiscard]] std::vector<double> roleReferenceOnsetsS(std::span<const AppliedEventRecord> applied, audio::MusicRole role,
                                                       std::uint32_t sampleRate = 48000, double mergeS = 0.030);

// Release times of a role: frames of its Applied NoteOff records (where the 2 ms release ramp starts), sorted
[[nodiscard]] std::vector<double> roleReleaseTimesS(std::span<const AppliedEventRecord> applied, audio::MusicRole role,
                                                    std::uint32_t sampleRate = 48000);

// Per-role detector profile (descriptors_v1 core, D-M2-14): delta and a constant offset subtracted from the
// detections (latency correction). Other parameters stay at the descriptors_v1 values.
struct RoleOnsetProfile {
    double delta{0.10};
    double offsetS{0.0};
};

// Signed error statistics of the nearest detection within +-windowS of each reference (used for calibration)
struct SignedErrorStats {
    std::size_t pairs{0};
    double medianS{0.0}, p05S{0.0}, p95S{0.0};
};
[[nodiscard]] SignedErrorStats nearestSignedErrors(std::span<const double> referenceS, std::span<const double> estimateS,
                                                   double windowS = 0.100);

struct RoleOnsetEvaluation {
    AnalysisStatus status{AnalysisStatus::Ok};
    std::size_t references{0};
    std::size_t detections{0};
    OnsetScore at50{};          // primary, +-50 ms, maximum cardinality, offset-corrected detections
    OnsetScore at25{};          // diagnostic, +-25 ms
    SignedErrorStats errors{};  // of the offset-corrected detections
    // Detections left unmatched at +-50 ms: lag after the preceding reference onset (diagnosis of extra detections)
    SignedErrorStats unmatchedLag{};
    // Diagnostic (does not change at50): unmatched detections within +-50 ms of a release of the role, and the
    // precision obtained when those are not counted as false positives
    std::size_t releaseDetections{0};
    double precisionExclReleases{0.0};
};
[[nodiscard]] RoleOnsetEvaluation evaluateRoleOnsets(std::span<const float> stemInterleaved, std::span<const double> referenceS,
                                                     const RoleOnsetProfile& profile, std::uint32_t sampleRate = 48000,
                                                     std::span<const double> releaseS = {});

[[nodiscard]] double percentileLinear(std::vector<double> v, double q);

} // namespace cymatica::cacp
