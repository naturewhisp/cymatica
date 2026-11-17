#pragma once

#include "ensemble_tools.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace cymatica::cacp {

// Measurement package 6 (docs/research/M2_5_Pkg6_Development_Measures_Preregistration.md): graded development
// measures, validated on declared truths before any use. Offline, MEASURED, never a gate, never part of the planner score.
enum class DevelopmentMeasure : std::uint8_t { M4 = 4, M5 = 5, M5r = 6 };

// Graded distance between the notes of one role in two phrases (M5: intervals; M5r: intervals and onset intervals)
[[nodiscard]] double gradedDistance(const PhraseTokens& a, const PhraseTokens& b, std::size_t role, bool withRhythm);
[[nodiscard]] std::vector<std::vector<double>> developmentDistanceMatrix(const std::vector<PhraseTokens>& phrases, std::size_t role,
                                                                         DevelopmentMeasure m);

// Rank (1 = nearest) of the declared parent among all earlier phrases of the child (0-based indices); ties share the
// best rank; 0 when undefined
[[nodiscard]] std::size_t parentRank(const std::vector<std::vector<double>>& d, std::size_t child, std::size_t parent);
// Foote novelty on S = 1 - d with a checkerboard kernel of half-width w phrases: value at index i is the boundary
// between phrases i-1 and i (0 where the kernel does not fit)
[[nodiscard]] std::vector<double> footeNovelty(const std::vector<std::vector<double>>& d, std::size_t w = 2);
// Indices of the local maxima of a novelty curve, highest first (ties: earlier first), at most k
[[nodiscard]] std::vector<std::size_t> topPeaks(const std::vector<double>& novelty, std::size_t k);

struct MeasureValidation {
    std::string measure{};
    std::vector<std::pair<std::string, std::string>> adversarial{};   // id, classification
    bool v1Horse{false};
    std::vector<std::size_t> lineageRanks{};                           // the six declared derivations of F9
    std::size_t lineageHits{0};
    double lineageMrr{0.0};
    bool v2Lineage{false};
    std::vector<double> inversionDistances{};                          // 6 <- 1, 14 <- 6 (descriptive)
    std::vector<std::size_t> f9Peaks{};
    std::size_t f9BoundariesHit{0};
    bool v3SectionsF9{false};
    std::vector<std::size_t> corpoPeaks{};
    bool v4SectionsFixture{false};
    std::string f8Classification{};                                    // descriptive
    bool admitted{false};
};

struct DevelopmentMeasuresResult {
    std::string status{"NOT_RUN"};
    std::vector<std::string> reasons{};
    std::vector<MeasureValidation> measures{};   // M4 (reference), M5, M5r
    bool v5RepeatIdentical{false};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string measuresJson() const;
    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] DevelopmentMeasuresResult runDevelopmentMeasuresV1(const std::string& outputDir = "artifacts/cacp", bool writeFiles = true,
                                                                 const std::string& controlsDir = kEnsembleControlsDir);

} // namespace cymatica::cacp
