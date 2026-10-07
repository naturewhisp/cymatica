#include <catch2/catch_test_macros.hpp>

#include "run_record.h"

#include <cmath>
#include <limits>

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
        .tick = 180ULL,
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
    REQUIRE(restored.runtimeInterventions[0].tick == 180ULL);
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

    // 8. Inconsistent timing_profile (48000 / 120 / frames_per_tick=123) must throw (DIF-M1-23)
    const std::string inconsistentTiming = R"({
        "schema_version": 1,
        "rng_version": 1,
        "run_seed_u64": 10,
        "timing_profile": { "sample_rate": 48000, "simulation_hz": 120, "frames_per_tick": 123 }
    })";
    REQUIRE_THROWS_AS(RunRecord::fromJson(inconsistentTiming), std::runtime_error);

    // 9. Non-divisible timing_profile (48001 / 120 / frames_per_tick=400) must throw
    const std::string nonDivisibleTiming = R"({
        "schema_version": 1,
        "rng_version": 1,
        "run_seed_u64": 10,
        "timing_profile": { "sample_rate": 48001, "simulation_hz": 120, "frames_per_tick": 400 }
    })";
    REQUIRE_THROWS_AS(RunRecord::fromJson(nonDivisibleTiming), std::runtime_error);

    // 10. Malformed non-hex run_seed string must throw std::runtime_error
    const std::string malformedHex = R"({
        "schema_version": 1,
        "rng_version": 1,
        "run_seed": "zz_invalid_hex",
        "timing_profile": { "sample_rate": 48000, "simulation_hz": 120, "frames_per_tick": 400 }
    })";
    REQUIRE_THROWS_AS(RunRecord::fromJson(malformedHex), std::runtime_error);

    // 11. Non-numeric metric in final_metrics must throw std::runtime_error
    const std::string nonNumericMetric = R"({
        "schema_version": 1,
        "rng_version": 1,
        "run_seed_u64": 10,
        "timing_profile": { "sample_rate": 48000, "simulation_hz": 120, "frames_per_tick": 400 },
        "final_metrics": { "bad_val": "string_not_number" }
    })";
    REQUIRE_THROWS_AS(RunRecord::fromJson(nonNumericMetric), std::runtime_error);
}

TEST_CASE("RunRecord validate and toJson enforce non-finite float rejection and schema integrity", "[replay][validation]") {
    RunRecord r;
    r.runSeed = 12345;
    r.timingProfile = {48000, 120, 400};

    std::string err;
    REQUIRE(r.validate(&err));

    // Non-finite float in finalMetrics must be rejected by validate() and toJson() (§23.4, DIF-M1-23)
    r.finalMetrics["bad_metric"] = std::numeric_limits<double>::infinity();
    REQUIRE_FALSE(r.validate(&err));
    REQUIRE_THROWS_AS(r.toJson(), std::runtime_error);

    r.finalMetrics["bad_metric"] = std::numeric_limits<double>::quiet_NaN();
    REQUIRE_FALSE(r.validate(&err));
    REQUIRE_THROWS_AS(r.toJson(), std::runtime_error);

    // Inconsistent timing profile rejected by validate() and toJson()
    r.finalMetrics["bad_metric"] = 1.0;
    r.timingProfile.framesPerTick = 999;
    REQUIRE_FALSE(r.validate(&err));
    REQUIRE_THROWS_AS(r.toJson(), std::runtime_error);

    // Restoring consistent timing profile passes validate and toJson
    r.timingProfile.framesPerTick = 400;
    REQUIRE(r.validate(&err));
    REQUIRE_NOTHROW(r.toJson());
}

TEST_CASE("RunRecord recordIntervention records intervention and invalidates Pure Seed", "[replay][intervention]") {
    RunRecord r;
    r.runSeed = 42;
    REQUIRE(r.pureSeedEligible == true);
    REQUIRE(r.runtimeInterventions.empty());

    r.recordIntervention(8000ULL, "technical_suspension", 20ULL);
    REQUIRE(r.pureSeedEligible == false);
    REQUIRE(r.runtimeInterventions.size() == 1);
    REQUIRE(r.runtimeInterventions[0].sampleFrame == 8000ULL);
    REQUIRE(r.runtimeInterventions[0].tick == 20ULL);
    REQUIRE(r.runtimeInterventions[0].reason == "technical_suspension");
}

TEST_CASE("RunRecord requires the complete canonical seed syntax", "[replay][seed]") {
    const auto parse = [](const std::string& seed) {
        return RunRecord::fromJson(std::string(R"({"schema_version":1,"rng_version":1,"run_seed":")") + seed +
            R"(","timing_profile":{"sample_rate":48000,"simulation_hz":120,"frames_per_tick":400}})");
    };
    for (const auto* bad : {"0x000000000000000ajunk", "0x1", "000000000000000a", "+0x00000000000000a",
                            " 0x000000000000000a", "0X000000000000000a", "0x000000000000000g", "0x000000000000000a "}) {
        REQUIRE_THROWS_AS(parse(bad), std::runtime_error);
    }
    REQUIRE(parse("0x000000000000000a").runSeed == 10);
    REQUIRE(parse("0xFFFFFFFFFFFFFFFF").runSeed == UINT64_MAX);
    REQUIRE(parse("0x0000000000000000").runSeed == 0);
}

TEST_CASE("RunRecord rejects narrowing and noninteger version fields", "[replay][validation][overflow]") {
    const std::string prefix = R"({"run_seed_u64":10,"schema_version":)";
    const std::string tail = R"(,"rng_version":1,"timing_profile":{"sample_rate":48000,"simulation_hz":120,"frames_per_tick":400}})";
    for (const auto* invalid : {"4294967297", "-4294967295", "1.0", "true", "\"1\""}) {
        REQUIRE_THROWS_AS(RunRecord::fromJson(prefix + invalid + tail), std::runtime_error);
    }
    const auto valid = prefix + "1" + tail;
    auto badArchetype = valid;
    badArchetype.insert(badArchetype.size() - 1, R"(,"decisions":[{"archetype_id":4294967297}])");
    REQUIRE_THROWS_AS(RunRecord::fromJson(badArchetype), std::runtime_error);
    auto badPolicy = valid;
    badPolicy.insert(badPolicy.size() - 1, R"(,"policy_version_id":{"schema":4294967297})");
    REQUIRE_THROWS_AS(RunRecord::fromJson(badPolicy), std::runtime_error);

    for (const auto* field : {"rng_version", "simulation_hz", "frames_per_tick"}) {
        auto malformed = valid;
        const auto begin = malformed.find(std::string("\"") + field + "\":") + std::string(field).size() + 3;
        const auto end = malformed.find_first_of(",}", begin);
        malformed.replace(begin, end - begin, "4294967297");
        REQUIRE_THROWS_AS(RunRecord::fromJson(malformed), std::runtime_error);
    }
}
