#pragma once

#include "version_ids.h"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace cymatica::replay {

struct DecisionRecord {
    std::uint64_t decisionId{0};
    std::uint64_t sampleFrame{0};
    std::uint32_t archetypeId{0};
    std::string patternTag;

    [[nodiscard]] bool operator==(const DecisionRecord& other) const noexcept = default;
};

struct RuntimeIntervention {
    std::uint64_t sampleFrame{0};
    std::string reason;

    [[nodiscard]] bool operator==(const RuntimeIntervention& other) const noexcept = default;
};

struct RunRecord {
    std::uint32_t schemaVersion{core::RUN_RECORD_SCHEMA_VERSION};
    std::string buildId{"cymatica-dev"};
    std::string policyVersion{core::DEFAULT_POLICY_VERSION_STRING};
    core::PolicyVersionId policyVersionId{core::DEFAULT_POLICY_VERSION_ID};
    std::uint32_t rngVersion{core::RNG_VERSION};
    std::uint64_t runSeed{0};
    std::string mode{"infinite"};
    std::string difficultyPolicy{"standard-adaptive"};
    std::vector<DecisionRecord> decisions;
    std::vector<RuntimeIntervention> runtimeInterventions;
    std::map<std::string, double> finalMetrics; // Spec §15.7 schema: "final_metrics": {} (DIF-M1-14)

    [[nodiscard]] std::string toJson() const;
    static RunRecord fromJson(std::string_view jsonStr);

    [[nodiscard]] bool operator==(const RunRecord& other) const noexcept = default;
};

} // namespace cymatica::replay
