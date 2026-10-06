#include <catch2/catch_test_macros.hpp>

#include "run_record.h"

using namespace cymatica::replay;

TEST_CASE("RunRecord serializes to JSON and round-trips correctly", "[replay][json]") {
    RunRecord original;
    original.schemaVersion = 1;
    original.buildId = "git:test_commit";
    original.policyVersion = "cie-policy-0.1.0";
    original.policyVersionId = cymatica::core::PolicyVersionId{1, 0, 1, 0};
    original.rngVersion = 1;
    original.runSeed = 0x6e51a0b7d44c1f23ULL;
    original.mode = "infinite";
    original.difficultyPolicy = "standard-adaptive";

    original.decisions.push_back(DecisionRecord{
        .decisionId = 1001ULL,
        .sampleFrame = 48000ULL,
        .archetypeId = 1,
        .patternTag = "synthetic_matrix_grid"
    });
    original.decisions.push_back(DecisionRecord{
        .decisionId = 1002ULL,
        .sampleFrame = 96000ULL,
        .archetypeId = 2,
        .patternTag = "organic_concentric_spiral"
    });

    original.runtimeInterventions.push_back(RuntimeIntervention{
        .sampleFrame = 72000ULL,
        .reason = "technical_suspension_reconciled"
    });

    original.finalMetrics["survival_time_seconds"] = 142.5;
    original.finalMetrics["total_score"] = 9850.0;
    original.finalMetrics["accuracy_ratio"] = 0.945;

    const std::string jsonOutput = original.toJson();
    REQUIRE_FALSE(jsonOutput.empty());

    const RunRecord restored = RunRecord::fromJson(jsonOutput);
    REQUIRE(restored == original);
}
