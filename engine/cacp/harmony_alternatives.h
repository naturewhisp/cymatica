#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario harmony_alternatives_v1 (pacchetto 6c, D-M2-49; pre-registration
// docs/research/M2_5_3_C6c_Harmony_Alternatives_Preregistration.md): paired comparison of the three harmony
// alternatives of section B of fixture-variants-v1. For every seed the drawn path is planned three times, with bHarmony
// forced to 0, 1 and 2. Symbolic measures on the plan tokens (sixteenths), section B only. MEASURED, never a gate.

struct HarmonyAlternativeStats {
    std::uint8_t alternative{0};
    std::size_t plansOk{0};
    std::size_t falseRelations{0};          // H-FR: notes on the natural pitch class of an altered third, in its bars
    std::size_t vettoreCorpoClashes{0};     // H-IC: Vettore onsets in interval class 1 or 6 with a sounding Corpo note
    std::size_t changeOnsets{0};            // H-CT: Vettore onsets on step 0 of a chord change (planner definition)
    std::size_t changeChordTones{0};
    std::size_t realChangeOnsets{0};        // descriptive: chord changes against the bar actually preceding
    std::size_t realChangeChordTones{0};
    std::size_t adaptedNotes{0};            // H-AD: Vettore notes moved by the chord reading, phrases of B
    std::size_t repairedBars{0};            // H-RP: Corpo bars repaired by C1c, phrases of B
    std::size_t registerUnmet{0};           // phrases of B with vettoreRegisterUnmet
    std::size_t distinctCorpoGroups{0};     // H-GR: distinct Corpo phrase fingerprints of B over the seeds
    std::array<std::size_t, 4> leadingToneOnsets{}; // H-LT: onsets on the altered third, per role (Pulso..Vettore)
};

struct HarmonyAlternativesResult {
    std::string status{"NOT_RUN"};          // MEASURED or INCONCLUSIVE
    std::vector<std::string> reasons{};
    std::uint64_t seedFirst{1}, seedLast{64};
    std::array<HarmonyAlternativeStats, 3> alternatives{};
    std::array<std::size_t, 3> drawnCount{}; // bHarmony drawn by drawFixtureVariant over the seeds
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string toJson() const;
};

[[nodiscard]] HarmonyAlternativesResult runHarmonyAlternativesV1(const std::string& outputDir = "artifacts/cacp",
                                                                 bool writeFiles = true, std::uint64_t seedFirst = 1,
                                                                 std::uint64_t seedLast = 64);

} // namespace cymatica::cacp
