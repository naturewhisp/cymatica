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
struct OnsetParityDelta {
    double delta{0.0};
    std::size_t detected{0};
    double greedyPrecision{0.0}, greedyRecall{0.0}, greedyF{0.0};
    double maxPrecision{0.0}, maxRecall{0.0}, maxF{0.0};
    double offsetMedianMs{0.0}, offsetP05Ms{0.0}, offsetP95Ms{0.0}; // greedy pairs, as E1
    std::array<double, 4> roleRecall{};  // Pulso, Corpo, Trama, Vettore: any detection within the tolerance (E1)
    // E1 reference values (onset_report_2026-10-21.json)
    std::size_t e1Detected{0};
    double e1Precision{0.0}, e1Recall{0.0};
    bool matchesE1{false};               // detected count equal and P, R equal at E1's 3-decimal rounding
};

struct OnsetParityResult {
    bool completed{false};
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
