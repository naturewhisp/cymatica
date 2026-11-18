#pragma once

#include "music_planner.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario cumulative_listening_v1 (D-M2-50; design and hypotheses docs/research/M2_5_Cumulative_Listening_Hypotheses.md):
// renders the two full pieces of the cumulative listening. Piece 1: the reference fixture (seed 1337). Piece 2: the
// path of fixture-variants-v1 drawn by the smallest seed in [1, 64] with bHarmony 2, the AURA cell in A and B, Trama
// entry != 4 and displacement start != 12, rendered offline with that seed (every golden NOT_APPLICABLE).

struct ListeningPiece {
    std::string file{};
    std::uint64_t seed{0};
    audio::FixtureVariant variant{};
    bool probePassed{false};
    std::string failureReason{};
    std::string goldenPcmStatus{"NOT_RUN"};
    std::uint64_t pcmContentHash{0};
    std::optional<double> integratedLufs{};
    double truePeakDbtp{-120.0};
    double durationS{0.0};
};

struct CumulativeListeningResult {
    std::string status{"NOT_RUN"};               // MEASURED or INCONCLUSIVE
    std::vector<std::string> reasons{};
    ListeningPiece reference{}, second{};
    std::array<double, 2> sectionStartS{};        // start of B and A' in seconds (same clock for both pieces)
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string toJson() const;
};

// The seed of piece 2 by the declared rule; nullopt when no seed in [first, last] satisfies it
[[nodiscard]] std::optional<std::uint64_t> secondListeningSeed(std::uint64_t first = 1, std::uint64_t last = 64);

[[nodiscard]] CumulativeListeningResult runCumulativeListeningV1(const std::string& outputDir = "artifacts/cacp",
                                                                 bool writeFiles = true);

} // namespace cymatica::cacp
