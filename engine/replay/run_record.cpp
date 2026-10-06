#include "run_record.h"

#include <nlohmann/json.hpp>
#include <iomanip>
#include <sstream>

namespace cymatica::replay {

using json = nlohmann::json;

std::string RunRecord::toJson() const {
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
        intArray.push_back({
            {"sample_frame", in.sampleFrame},
            {"reason", in.reason}
        });
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

    RunRecord r;
    r.schemaVersion = j.value("schema_version", core::RUN_RECORD_SCHEMA_VERSION);
    r.buildId = j.value("build_id", "");
    r.policyVersion = j.value("policy_version", core::DEFAULT_POLICY_VERSION_STRING);

    if (j.contains("policy_version_id")) {
        const auto& pjid = j["policy_version_id"];
        r.policyVersionId = core::PolicyVersionId{
            pjid.value("schema", 1u),
            pjid.value("major", 0u),
            pjid.value("minor", 1u),
            pjid.value("patch", 0u)
        };
    }

    r.rngVersion = j.value("rng_version", core::RNG_VERSION);

    if (j.contains("run_seed_u64")) {
        r.runSeed = j["run_seed_u64"].get<std::uint64_t>();
    } else if (j.contains("run_seed")) {
        const std::string s = j["run_seed"].get<std::string>();
        r.runSeed = std::stoull(s, nullptr, 16);
    }

    r.mode = j.value("mode", "infinite");
    r.difficultyPolicy = j.value("difficulty_policy", "standard-adaptive");

    if (j.contains("decisions") && j["decisions"].is_array()) {
        for (const auto& elem : j["decisions"]) {
            r.decisions.push_back(DecisionRecord{
                .decisionId = elem.value("decision_id", 0ULL),
                .sampleFrame = elem.value("sample_frame", 0ULL),
                .archetypeId = elem.value("archetype_id", 0u),
                .patternTag = elem.value("pattern_tag", "")
            });
        }
    }

    if (j.contains("runtime_interventions") && j["runtime_interventions"].is_array()) {
        for (const auto& elem : j["runtime_interventions"]) {
            r.runtimeInterventions.push_back(RuntimeIntervention{
                .sampleFrame = elem.value("sample_frame", 0ULL),
                .reason = elem.value("reason", "")
            });
        }
    }

    if (j.contains("final_metrics") && j["final_metrics"].is_object()) {
        for (auto it = j["final_metrics"].begin(); it != j["final_metrics"].end(); ++it) {
            if (it.value().is_number()) {
                r.finalMetrics[it.key()] = it.value().get<double>();
            }
        }
    }

    return r;
}

} // namespace cymatica::replay
