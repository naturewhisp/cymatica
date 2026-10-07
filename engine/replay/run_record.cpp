#include "run_record.h"

#include <cmath>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace cymatica::replay {

using json = nlohmann::json;

bool RunRecord::validate(std::string* error) const {
    if (schemaVersion != core::RUN_RECORD_SCHEMA_VERSION) {
        if (error) *error = "unsupported schema_version " + std::to_string(schemaVersion);
        return false;
    }
    if (rngVersion != core::RNG_VERSION) {
        if (error) *error = "unsupported rng_version " + std::to_string(rngVersion);
        return false;
    }
    if (timingProfile.sampleRate == 0 || timingProfile.simulationHz == 0 || timingProfile.framesPerTick == 0) {
        if (error) *error = "non-positive timing_profile values";
        return false;
    }
    if (timingProfile.sampleRate % timingProfile.simulationHz != 0 ||
        timingProfile.framesPerTick != timingProfile.sampleRate / timingProfile.simulationHz) {
        if (error) {
            *error = "timing_profile frames_per_tick (" + std::to_string(timingProfile.framesPerTick) +
                     ") inconsistent with sample_rate / simulation_hz (" +
                     std::to_string(timingProfile.sampleRate / timingProfile.simulationHz) + ")";
        }
        return false;
    }
    for (const auto& [k, v] : finalMetrics) {
        if (!std::isfinite(v)) {
            if (error) *error = "non-finite float in final_metrics for key " + k;
            return false;
        }
    }
    return true;
}

void RunRecord::recordIntervention(std::uint64_t sampleFrame, std::string_view reason, std::uint64_t tick) {
    runtimeInterventions.push_back(RuntimeIntervention{
        .sampleFrame = sampleFrame,
        .tick = tick,
        .reason = std::string(reason),
    });
    pureSeedEligible = false; // DIF-M1-21 / Spec §15: technical intervention invalidates Pure Seed
}

std::string RunRecord::toJson() const {
    std::string err;
    if (!validate(&err)) {
        throw std::runtime_error("RunRecord::toJson validation failed: " + err);
    }

    json j;
    j["schema_version"] = schemaVersion;
    j["build_id"] = buildId;
    j["policy_version"] = policyVersion;
    j["policy_version_id"] = {
        {"schema", policyVersionId.schema},
        {"major", policyVersionId.major},
        {"minor", policyVersionId.minor},
        {"patch", policyVersionId.patch},
        {"u32", policyVersionId.toU32()}
    };
    j["rng_version"] = rngVersion;

    std::ostringstream ss;
    ss << "0x" << std::hex << std::setfill('0') << std::setw(16) << runSeed;
    j["run_seed"] = ss.str();
    j["run_seed_u64"] = runSeed;

    j["mode"] = mode;
    j["difficulty_policy"] = difficultyPolicy;
    j["pure_seed_eligible"] = pureSeedEligible;

    j["timing_profile"] = {
        {"sample_rate", timingProfile.sampleRate},
        {"simulation_hz", timingProfile.simulationHz},
        {"frames_per_tick", timingProfile.framesPerTick}
    };

    json decArray = json::array();
    for (const auto& d : decisions) {
        decArray.push_back({
            {"decision_id", d.decisionId},
            {"sample_frame", d.sampleFrame},
            {"archetype_id", d.archetypeId},
            {"pattern_tag", d.patternTag}
        });
    }
    j["decisions"] = decArray;

    json intArray = json::array();
    for (const auto& in : runtimeInterventions) {
        json item = {
            {"sample_frame", in.sampleFrame},
            {"reason", in.reason}
        };
        if (in.tick > 0) {
            item["tick"] = in.tick;
        }
        intArray.push_back(item);
    }
    j["runtime_interventions"] = intArray;

    json metricsObj = json::object();
    for (const auto& [k, v] : finalMetrics) {
        metricsObj[k] = v;
    }
    j["final_metrics"] = metricsObj;

    return j.dump(2);
}

RunRecord RunRecord::fromJson(std::string_view jsonStr) {
    const json j = json::parse(jsonStr);

    const auto readU32 = [](const json& value, const char* name) -> std::uint32_t {
        if (!value.is_number_unsigned() || value.get<std::uint64_t>() > UINT32_MAX) {
            throw std::runtime_error(std::string("RunRecord: invalid uint32 field ") + name);
        }
        return value.get<std::uint32_t>();
    };

    // 1. schema_version is mandatory; parser fail-fast on incompatible major (§23.4)
    if (!j.contains("schema_version")) {
        throw std::runtime_error("RunRecord: missing mandatory field 'schema_version'");
    }
    const auto schemaVer = readU32(j["schema_version"], "schema_version");
    if (schemaVer != core::RUN_RECORD_SCHEMA_VERSION) {
        throw std::runtime_error("RunRecord: unsupported schema_version " + std::to_string(schemaVer));
    }

    // 2. rng_version is mandatory; fail-fast on unknown rng_version (§23.4)
    if (!j.contains("rng_version")) {
        throw std::runtime_error("RunRecord: missing mandatory field 'rng_version'");
    }
    const auto rngVer = readU32(j["rng_version"], "rng_version");
    if (rngVer != core::RNG_VERSION) {
        throw std::runtime_error("RunRecord: unsupported rng_version " + std::to_string(rngVer));
    }

    // 3. Seed validation: run_seed (hex) and run_seed_u64 must be consistent (§23.4, DIF-M1-23)
    const bool hasSeedStr = j.contains("run_seed");
    const bool hasSeedU64 = j.contains("run_seed_u64");
    if (!hasSeedStr && !hasSeedU64) {
        throw std::runtime_error("RunRecord: missing mandatory run_seed");
    }
    std::uint64_t seedValue = 0;
    if (hasSeedU64) {
        if (!j["run_seed_u64"].is_number_unsigned()) {
            throw std::runtime_error("RunRecord: 'run_seed_u64' must be an unsigned integer");
        }
        seedValue = j["run_seed_u64"].get<std::uint64_t>();
    }
    if (hasSeedStr) {
        if (!j["run_seed"].is_string()) {
            throw std::runtime_error("RunRecord: 'run_seed' must be a hex string");
        }
        const std::string s = j["run_seed"].get<std::string>();
        std::uint64_t parsedSeed = 0;
        if (s.size() != 18 || s[0] != '0' || s[1] != 'x') {
            throw std::runtime_error("RunRecord: run_seed must be 0x plus 16 hexadecimal digits");
        }
        for (std::size_t i = 2; i < s.size(); ++i) {
            const char c = s[i];
            unsigned digit = 0;
            if (c >= '0' && c <= '9') digit = static_cast<unsigned>(c - '0');
            else if (c >= 'a' && c <= 'f') digit = static_cast<unsigned>(c - 'a') + 10;
            else if (c >= 'A' && c <= 'F') digit = static_cast<unsigned>(c - 'A') + 10;
            else throw std::runtime_error("RunRecord: invalid hexadecimal digit in run_seed");
            parsedSeed = (parsedSeed << 4) | digit;
        }
        if (hasSeedU64 && parsedSeed != seedValue) {
            throw std::runtime_error("RunRecord: inconsistent run_seed (" + s + ") and run_seed_u64 (" + std::to_string(seedValue) + ")");
        }
        seedValue = parsedSeed;
    }

    // 4. timing_profile is mandatory per Spec §7.5 / DIF-M1-23
    if (!j.contains("timing_profile") || !j["timing_profile"].is_object()) {
        throw std::runtime_error("RunRecord: missing mandatory 'timing_profile' object");
    }
    const auto& tp = j["timing_profile"];
    if (!tp.contains("sample_rate") || !tp["sample_rate"].is_number_unsigned() ||
        !tp.contains("simulation_hz") || !tp["simulation_hz"].is_number_unsigned() ||
        !tp.contains("frames_per_tick") || !tp["frames_per_tick"].is_number_unsigned()) {
        throw std::runtime_error("RunRecord: timing_profile fields must be non-negative integers");
    }
    TimingProfile timingProf;
    timingProf.sampleRate = tp.value("sample_rate", 0ULL);
    timingProf.simulationHz = readU32(tp["simulation_hz"], "simulation_hz");
    timingProf.framesPerTick = readU32(tp["frames_per_tick"], "frames_per_tick");
    if (timingProf.sampleRate == 0 || timingProf.simulationHz == 0 || timingProf.framesPerTick == 0) {
        throw std::runtime_error("RunRecord: invalid non-positive timing_profile values");
    }

    RunRecord r;
    r.schemaVersion = schemaVer;
    r.rngVersion = rngVer;
    r.runSeed = seedValue;
    r.timingProfile = timingProf;
    r.buildId = j.value("build_id", "");
    r.policyVersion = j.value("policy_version", core::DEFAULT_POLICY_VERSION_STRING);

    if (j.contains("policy_version_id")) {
        const auto& pjid = j["policy_version_id"];
        r.policyVersionId = core::PolicyVersionId{
            pjid.contains("schema") ? readU32(pjid["schema"], "policy_version_id.schema") : 1u,
            pjid.contains("major") ? readU32(pjid["major"], "policy_version_id.major") : 0u,
            pjid.contains("minor") ? readU32(pjid["minor"], "policy_version_id.minor") : 1u,
            pjid.contains("patch") ? readU32(pjid["patch"], "policy_version_id.patch") : 0u
        };
    }

    r.mode = j.value("mode", "infinite");
    r.difficultyPolicy = j.value("difficulty_policy", "standard-adaptive");
    r.pureSeedEligible = j.value("pure_seed_eligible", true);

    if (j.contains("decisions") && j["decisions"].is_array()) {
        for (const auto& elem : j["decisions"]) {
            r.decisions.push_back(DecisionRecord{
                .decisionId = elem.value("decision_id", 0ULL),
                .sampleFrame = elem.value("sample_frame", 0ULL),
                .archetypeId = elem.contains("archetype_id") ? readU32(elem["archetype_id"], "archetype_id") : 0u,
                .patternTag = elem.value("pattern_tag", "")
            });
        }
    }

    if (j.contains("runtime_interventions") && j["runtime_interventions"].is_array()) {
        for (const auto& elem : j["runtime_interventions"]) {
            r.runtimeInterventions.push_back(RuntimeIntervention{
                .sampleFrame = elem.value("sample_frame", 0ULL),
                .tick = elem.value("tick", 0ULL),
                .reason = elem.value("reason", "")
            });
        }
    }

    // 5. final_metrics: non-finite float values and non-numeric metrics are strictly forbidden (§23.4)
    if (j.contains("final_metrics") && j["final_metrics"].is_object()) {
        for (auto it = j["final_metrics"].begin(); it != j["final_metrics"].end(); ++it) {
            if (!it.value().is_number()) {
                throw std::runtime_error("RunRecord: non-numeric metric value in final_metrics for key " + it.key());
            }
            const double v = it.value().get<double>();
            if (!std::isfinite(v)) {
                throw std::runtime_error("RunRecord: non-finite float value in final_metrics for key " + it.key());
            }
            r.finalMetrics[it.key()] = v;
        }
    }

    // 6. Centralized invariant validation (DIF-M1-23)
    std::string err;
    if (!r.validate(&err)) {
        throw std::runtime_error("RunRecord: validation failed: " + err);
    }

    return r;
}

} // namespace cymatica::replay
