#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace cymatica::cacp {

// Scenario onset_parity_v1 (ADR-0006 D-M2-14, Inc-5 phase 5b): numerical parity of the C++ onset descriptor with
// experiment E1 of the CACP methods dossier §3.2 (numpy, 2026-10-21) on the same input. Documented, not a gate.
// Input: fixture_probe_v1 rendered in-process, quantised as the 16-bit WAV E1 read (round(x * 32767) / 32768),
// content frames only, mono (L + R) / 2. Reference: unique NoteOn frames of the offline plan (seed 1337).
// Reports E1's greedy matching (for parity) and the maximum-cardinality matching adopted by descriptors_v1.
//
// Parity (DIF-5b-03) compares every value E1 published, each within half a unit of E1's printed precision:
// detections and per-section counts exactly; P, R, F, per-section P/R/F and per-role recall at 3 decimals
// (tolerance 0.0005); offset median and p05/p95 at 2 decimals (tolerance 0.005 ms).
struct OnsetParitySection {
    double precision{0.0}, recall{0.0}, f{0.0};
    std::size_t references{0}, estimates{0};
};

struct OnsetParityDelta {
    double delta{0.0};
    std::size_t detected{0};
    double greedyPrecision{0.0}, greedyRecall{0.0}, greedyF{0.0};
    double maxPrecision{0.0}, maxRecall{0.0}, maxF{0.0};
    double offsetMedianMs{0.0}, offsetP05Ms{0.0}, offsetP95Ms{0.0}; // greedy pairs, as E1
    std::array<double, 4> roleRecall{};  // Pulso, Corpo, Trama, Vettore: any detection within the tolerance (E1)
    std::array<OnsetParitySection, 3> sections{}; // A, B, A' (greedy matching inside each section, as E1)
    bool matchesE1{false};
    std::string mismatchedFields{};      // empty when every compared field is within tolerance
};

struct OnsetParityResult {
    bool completed{false};               // the analysis ran to the end
    bool inputValid{false};              // the fixture render passed its own verdict and the PCM is finite
    std::string status{"NOT_RUN"};       // MEASURED only when completed and inputValid; otherwise INCONCLUSIVE
    bool fixturePassed{false};
    std::uint64_t pcmContentHash{0};
    std::uint64_t analysedFrames{0};
    std::size_t plannedNoteOns{0};
    std::size_t uniqueOnsetFrames{0};
    std::array<std::size_t, 4> roleUniqueOnsets{};
    std::size_t stftFrames{0};
    double stftSeconds{0.0};
    double noveltySeconds{0.0};
    std::array<OnsetParityDelta, 3> deltas{};
    bool allMatchE1{false};
    bool manifestWriteSuccess{false};
    std::string manifestFilePath{};
    std::string failureReason{};

    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] OnsetParityResult runOnsetParityV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
