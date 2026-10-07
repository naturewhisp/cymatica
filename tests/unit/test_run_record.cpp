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
    REQUIRE(restored.timingProfile.sampleRate == 48000);
    REQUIRE(restored.timingProfile.simulationHz == 120);
    REQUIRE(restored.timingProfile.framesPerTick == 400);
    REQUIRE(restored.pureSeedEligible == true);
}

TEST_CASE("RunRecord fromJson enforces Spec section 23.4 fail-fast validation", "[replay][json][validation]") {
    // 1. Missing schema_version must throw
    const std::string missingSchema = R"({
        "rng_version": 1,
        "run_seed_u64": 12345,
        "timing_profile": { "sample_rate": 48000, "simulation_hz": 120, "frames_per_tick": 400 }
    })";
    REQUIRE_THROWS_AS(RunRecord::fromJson(missingSchema), std::runtime_error);

    // 2. Incompatible schema_version must throw
    const std::string incompatibleSchema = R"({
        "schema_version": 99,
        "rng_version": 1,
        "run_seed_u64": 12345,
        "timing_profile": { "sample_rate": 48000, "simulation_hz": 120, "frames_per_tick": 400 }
    })";
    REQUIRE_THROWS_AS(RunRecord::fromJson(incompatibleSchema), std::runtime_error);

    // 3. Incompatible rng_version must throw
    const std::string incompatibleRng = R"({
        "schema_version": 1,
        "rng_version": 42,
        "run_seed_u64": 12345,
        "timing_profile": { "sample_rate": 48000, "simulation_hz": 120, "frames_per_tick": 400 }
    })";
    REQUIRE_THROWS_AS(RunRecord::fromJson(incompatibleRng), std::runtime_error);

    // 4. Inconsistent run_seed and run_seed_u64 must throw (§23.4 / DIF-M1-23)
    const std::string inconsistentSeed = R"({
        "schema_version": 1,
        "rng_version": 1,
        "run_seed": "0x000000000000000a",
        "run_seed_u64": 999,
        "timing_profile": { "sample_rate": 48000, "simulation_hz": 120, "frames_per_tick": 400 }
    })";
    REQUIRE_THROWS_AS(RunRecord::fromJson(inconsistentSeed), std::runtime_error);

    // 5. Missing timing_profile must throw (§7.5 / DIF-M1-23)
    const std::string missingTiming = R"({
        "schema_version": 1,
        "rng_version": 1,
        "run_seed_u64": 10
    })";
    REQUIRE_THROWS_AS(RunRecord::fromJson(missingTiming), std::runtime_error);

    // 6. Missing run_seed and run_seed_u64 must throw
    const std::string missingSeed = R"({
        "schema_version": 1,
        "rng_version": 1,
        "timing_profile": { "sample_rate": 48000, "simulation_hz": 120, "frames_per_tick": 400 }
    })";
    REQUIRE_THROWS_AS(RunRecord::fromJson(missingSeed), std::runtime_error);

    // 7. Non-positive timing_profile values must throw
    const std::string invalidTiming = R"({
        "schema_version": 1,
        "rng_version": 1,
        "run_seed_u64": 10,
        "timing_profile": { "sample_rate": 48000, "simulation_hz": 0, "frames_per_tick": 400 }
    })";
    REQUIRE_THROWS_AS(RunRecord::fromJson(invalidTiming), std::runtime_error);
}
