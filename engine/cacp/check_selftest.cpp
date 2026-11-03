#include "check_selftest.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <sstream>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

namespace fs = std::filesystem;

constexpr double kTol = 1e-12;
constexpr double kZ = 1.959963984540054;   // Wilson 95 %, restated here: the recomputation does not reuse presence_stats

enum class Fill { Truth, AllTrue, AllFalse };

std::optional<std::string> readFile(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return std::nullopt;
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
}

json resultBase(const TruthBundle& t) {
    json hashes = json::array();
    for (const auto& [name, h] : t.publicBlindFiles) hashes.push_back(h);
    return json{{"schema", "evaluation_result_v0"},
                {"claims_version", "claims_v0"},
                {"pack_mode", "blind"},
                {"evaluator", {{"type", "agent"}, {"model", "evaluation_check_selftest_v1"}, {"version", "1"}}},
                {"channel", {{"audio_channels", 2}, {"tokens_per_s", 0}}},
                {"access", {{"repository", false}, {"sealed", false}, {"diagnostic_pack", false}, {"code_execution", false}}},
                {"spectrogram_references_used", true},
                {"input_hashes", hashes},
                {"timestamp", "2026-11-03T00:00:00Z"},
                {"claims", json::array()}};
}

json presenceClaim(const WindowTruth& w, std::size_t role, bool value) {
    return json{{"window_id", w.id}, {"method", "listening"}, {"predicate", "role_signal_present"},
                {"args", {{"role", kRoleNames[role]}}}, {"claimed_value", value}, {"text", "selftest"}};
}

bool measurable(const WindowTruth& w, std::size_t r) { return w.roles[r].signal != SignalLevel::Unavailable; }
bool present(const WindowTruth& w, std::size_t r) { return w.roles[r].signal == SignalLevel::Present; }
bool inSample(const WindowTruth& w) { return w.kind == "test" || w.kind == "altered" || w.kind == "removal"; }

std::string groupOf(const WindowTruth& w) {
    if (!w.groupId.empty()) return w.groupId;
    return w.phrase >= 0 ? "p" + std::to_string(w.phrase) : "w:" + w.id;
}

// Independent recomputation of one class of presence_stats_v1 from the claims actually placed in the sample
struct Recomputed {
    std::size_t claims{0}, correct{0}, groups{0};
    double nEff{0.0};
    double rate{std::numeric_limits<double>::quiet_NaN()};
    double low{std::numeric_limits<double>::quiet_NaN()}, high{std::numeric_limits<double>::quiet_NaN()};
    std::string status{"NOT_RUN"};
};

Recomputed recompute(const std::vector<std::pair<std::string, bool>>& obs) {   // (group, correct)
    Recomputed r;
    if (obs.empty()) return r;
    std::map<std::string, double> m;
    for (const auto& [g, ok] : obs) {
        ++r.claims;
        r.correct += ok ? 1 : 0;
        m[g] += 1.0;
    }
    r.groups = m.size();
    double s = 0.0, s2 = 0.0;
    for (const auto& [g, v] : m) {
        s += v;
        s2 += v * v;
    }
    r.nEff = s * s / s2;
    r.rate = static_cast<double>(r.correct) / static_cast<double>(r.claims);
    if (r.groups < 4) {
        r.status = "DESCRIPTIVE_ONLY";
    } else if (r.nEff < 4.0) {
        r.status = "INSUFFICIENT_DATA";
    } else {
        r.status = "MEASURED";
        // Wilson in the (x + z^2/2) / (n + z^2) form, x = p * n_eff
        const double n = r.nEff, x = r.rate * n, z2 = kZ * kZ;
        const double centre = (x + z2 / 2.0) / (n + z2);
        const double half = kZ / (n + z2) * std::sqrt(x * (n - x) / n + z2 / 4.0);
        r.low = std::max(0.0, centre - half);
        r.high = std::min(1.0, centre + half);
    }
    return r;
}

bool near(double a, double b) {
    if (std::isnan(a) || std::isnan(b)) return std::isnan(a) && std::isnan(b);
    return std::abs(a - b) <= kTol;
}

void compareClass(const char* name, const ClassStats& got, const Recomputed& want, std::vector<std::string>& f) {
    auto fail = [&](const std::string& what) { f.push_back(std::string(name) + ": " + what); };
    if (got.status != want.status) fail("status " + got.status + " != " + want.status);
    if (got.claims != want.claims) fail("claims " + std::to_string(got.claims) + " != " + std::to_string(want.claims));
    if (got.correct != want.correct) fail("correct differs");
    if (got.groups != want.groups) fail("groups " + std::to_string(got.groups) + " != " + std::to_string(want.groups));
    if (!near(got.nEff, want.nEff)) fail("n_eff differs");
    if (want.claims > 0 && !near(got.rate, want.rate)) fail("rate differs");
    if (want.status == "MEASURED" && (!near(got.low, want.low) || !near(got.high, want.high))) fail("Wilson interval differs");
}

struct Run {
    CheckReport rep;
    Recomputed pos, neg;
};

// Writes the result, runs the file-level checker (with the sealed integrity check) and recomputes the sample
std::optional<Run> runCase(const TruthBundle& t, const json& claims, const std::string& sealedDir, const fs::path& work,
                           const std::string& tag, std::vector<std::string>& f) {
    json r = resultBase(t);
    r["claims"] = claims;
    std::error_code ec;
    fs::create_directories(work, ec);
    const fs::path resultPath = work / (tag + "_result.json");
    const fs::path reportPath = work / (tag + "_report.json");
    {
        std::ofstream out(resultPath, std::ios::binary);
        out << r.dump(2);
        if (!out.good()) {
            f.push_back("cannot write " + resultPath.string());
            return std::nullopt;
        }
    }
    Run run;
    run.rep = runEvaluationCheck(resultPath.string(), sealedDir, reportPath.string());
    if (!run.rep.checked) {
        f.push_back("checker did not run: " + run.rep.error);
        return std::nullopt;
    }
    std::map<std::string, const WindowTruth*> byId;
    for (const auto& w : t.blind) byId[w.id] = &w;
    std::vector<std::pair<std::string, bool>> pos, neg;
    for (const auto& c : claims) {
        const auto it = byId.find(c.at("window_id").get<std::string>());
        if (it == byId.end() || !inSample(*it->second)) continue;
        const WindowTruth& w = *it->second;
        std::size_t role = 0;
        while (role < kRoleCount && c["args"]["role"] != kRoleNames[role]) ++role;
        if (role >= kRoleCount || !measurable(w, role)) continue;
        const bool truth = present(w, role);
        (truth ? pos : neg).emplace_back(groupOf(w), c.at("claimed_value").get<bool>() == truth);
    }
    run.pos = recompute(pos);
    run.neg = recompute(neg);
    return run;
}

std::string overallByRule(const std::string& ps, const std::string& ns) {
    if (ps == "NOT_RUN" && ns == "NOT_RUN") return "NOT_RUN";
    if (ps == "MEASURED" && ns == "MEASURED") return "MEASURED";
    if (ps == "DESCRIPTIVE_ONLY" || ns == "DESCRIPTIVE_ONLY" || ps == "NOT_RUN" || ns == "NOT_RUN") return "DESCRIPTIVE_ONLY";
    return "INSUFFICIENT_DATA";
}

json fullClaims(const TruthBundle& t, Fill fill) {
    json a = json::array();
    for (const auto& w : t.blind) {
        if (w.kind == "duplicate") continue;
        for (std::size_t r = 0; r < kRoleCount; ++r) {
            if (!measurable(w, r)) continue;
            const bool v = fill == Fill::AllTrue ? true : fill == Fill::AllFalse ? false : (w.kind == "catch" ? false : present(w, r));
            a.push_back(presenceClaim(w, r, v));
        }
    }
    return a;
}

// Test windows grouped by musical group, in group order, with their present roles
std::map<std::string, std::vector<std::pair<const WindowTruth*, std::vector<std::size_t>>>> testGroups(const TruthBundle& t) {
    std::map<std::string, std::vector<std::pair<const WindowTruth*, std::vector<std::size_t>>>> g;
    for (const auto& w : t.blind) {
        if (w.kind != "test") continue;
        std::vector<std::size_t> roles;
        for (std::size_t r = 0; r < kRoleCount; ++r) {
            if (measurable(w, r) && present(w, r)) roles.push_back(r);
        }
        if (!roles.empty()) g[groupOf(w)].emplace_back(&w, roles);
    }
    return g;
}

SelftestCase caseOf(const std::string& version, const char* name) {
    SelftestCase c;
    c.version = version;
    c.name = name;
    return c;
}

} // namespace

SelftestResult runEvaluationCheckSelftestV1(const std::vector<std::string>& sealedDirs, const std::string& workDir,
                                           const std::string& outputDir, bool writeFiles) {
    SelftestResult res;
    const std::vector<std::string> versions{kEvaluationPackVersion, kEvaluationPackVersionV2, kEvaluationPackVersionV3};
    if (sealedDirs.size() != versions.size()) {
        res.status = "INCONCLUSIVE";
        res.reasons.emplace_back("three sealed directories expected (v1, v2, v3)");
        return res;
    }
    bool inconclusive = false;
    for (std::size_t vi = 0; vi < versions.size(); ++vi) {
        const std::string& version = versions[vi];
        const auto text = readFile(fs::path(sealedDirs[vi]) / "evaluation_truth.json");
        std::string err;
        const auto truth = text ? truthFromJson(*text, &err) : std::nullopt;
        if (!truth || truth->packVersion != version) {
            res.reasons.push_back(version + ": sealed truth missing or of another version " + err);
            inconclusive = true;
            continue;
        }
        const TruthBundle& t = *truth;
        const fs::path work = fs::path(workDir) / version;

        // S1 oracle
        {
            auto c = caseOf(version, "S1_oracle");
            if (const auto run = runCase(t, fullClaims(t, Fill::Truth), sealedDirs[vi], work, "s1", c.failures)) {
                const auto& rep = run->rep;
                if (!rep.resultAdmissible) c.failures.emplace_back("result not admissible");
                if (!rep.blindQualified) c.failures.emplace_back("result not blind qualified");
                if (rep.catchTrialFailed) c.failures.emplace_back("catch trial failed");
                for (const auto& o : rep.claims) {
                    if (o.outcome != "supported") c.failures.push_back("claim " + o.windowId + " " + o.outcome + ": " + o.reason);
                }
                compareClass("positives", rep.presence.positives, run->pos, c.failures);
                compareClass("negatives", rep.presence.negatives, run->neg, c.failures);
                if (rep.presenceStatus != overallByRule(run->pos.status, run->neg.status)) c.failures.emplace_back("overall status differs from the rule");
                if (run->pos.status == "MEASURED" && run->neg.status == "MEASURED") {
                    if (!near(rep.presence.youdenLow, run->pos.low + run->neg.low - 1.0)) c.failures.emplace_back("J_L differs");
                } else if (!std::isnan(rep.presence.youdenLow)) {
                    c.failures.emplace_back("J_L reported without both classes MEASURED");
                }
                c.presenceStatus = rep.presenceStatus;
            }
            c.passed = c.failures.empty();
            res.cases.push_back(c);
        }
        // S2 always present
        {
            auto c = caseOf(version, "S2_always_present");
            if (const auto run = runCase(t, fullClaims(t, Fill::AllTrue), sealedDirs[vi], work, "s2", c.failures)) {
                if (!run->rep.catchTrialFailed) c.failures.emplace_back("catch trial not failed");
                if (run->rep.presenceStatus != "EXCLUDED_BY_CATCH_RULE") c.failures.push_back("status " + run->rep.presenceStatus);
                if (run->rep.presence.aboveTrivialBaseline.has_value()) c.failures.emplace_back("baseline compared despite the catch rule");
                c.presenceStatus = run->rep.presenceStatus;
            }
            c.passed = c.failures.empty();
            res.cases.push_back(c);
        }
        // S3 always absent
        {
            auto c = caseOf(version, "S3_always_absent");
            if (const auto run = runCase(t, fullClaims(t, Fill::AllFalse), sealedDirs[vi], work, "s3", c.failures)) {
                const auto& rep = run->rep;
                if (rep.catchTrialFailed) c.failures.emplace_back("catch trial failed");
                if (rep.presence.positives.claims > 0 && rep.presence.positives.rate != 0.0) c.failures.emplace_back("positive rate not 0");
                if (rep.presence.negatives.claims > 0 && rep.presence.negatives.rate != 1.0) c.failures.emplace_back("negative rate not 1");
                compareClass("positives", rep.presence.positives, run->pos, c.failures);
                compareClass("negatives", rep.presence.negatives, run->neg, c.failures);
                if (rep.presence.aboveTrivialBaseline.value_or(false)) c.failures.emplace_back("constant classifier above the trivial baseline");
                if (rep.presence.aboveTrivialBaseline.has_value() && !(rep.presence.youdenLow <= 0.0)) c.failures.emplace_back("J_L > 0");
                c.presenceStatus = rep.presenceStatus;
            }
            c.passed = c.failures.empty();
            res.cases.push_back(c);
        }
        if (version == kEvaluationPackVersion) {
            auto c = caseOf(version, "S6_v1_compatibility");
            if (const auto run = runCase(t, fullClaims(t, Fill::Truth), sealedDirs[vi], work, "s6", c.failures)) {
                if (run->rep.groupingRule.find("source_phrase_only") == std::string::npos) c.failures.push_back("grouping rule " + run->rep.groupingRule);
            }
            c.passed = c.failures.empty();
            res.cases.push_back(c);
            continue;
        }
        const auto groups = testGroups(t);
        // S4 descriptive: positives on the test windows of exactly 3 groups
        {
            auto c = caseOf(version, "S4_descriptive_only");
            if (groups.size() < 3) {
                c.failures.emplace_back("fewer than 3 test groups with a present role");
                c.composable = false;
                inconclusive = true;
            } else {
                json claims = json::array();
                std::size_t taken = 0;
                for (const auto& [g, members] : groups) {
                    if (taken++ == 3) break;
                    for (const auto& [w, roles] : members) {
                        for (const auto r : roles) claims.push_back(presenceClaim(*w, r, true));
                    }
                }
                if (const auto run = runCase(t, claims, sealedDirs[vi], work, "s4", c.failures)) {
                    if (run->rep.presence.positives.status != "DESCRIPTIVE_ONLY") c.failures.push_back("positives " + run->rep.presence.positives.status);
                    if (run->rep.presence.negatives.status != "NOT_RUN") c.failures.push_back("negatives " + run->rep.presence.negatives.status);
                    if (run->rep.presenceStatus != "DESCRIPTIVE_ONLY") c.failures.push_back("status " + run->rep.presenceStatus);
                    compareClass("positives", run->rep.presence.positives, run->pos, c.failures);
                    c.presenceStatus = run->rep.presenceStatus;
                }
            }
            c.passed = c.failures.empty();
            res.cases.push_back(c);
        }
        // S5 insufficient: group A with k >= 2 present roles on its test window, B, C, D one role each
        {
            auto c = caseOf(version, "S5_insufficient_data");
            const std::pair<const WindowTruth*, std::vector<std::size_t>>* a = nullptr;
            std::string aGroup;
            for (const auto& [g, members] : groups) {
                if (members.front().second.size() >= 2) {
                    a = &members.front();
                    aGroup = g;
                    break;
                }
            }
            std::vector<const std::pair<const WindowTruth*, std::vector<std::size_t>>*> others;
            for (const auto& [g, members] : groups) {
                if (g != aGroup && others.size() < 3) others.push_back(&members.front());
            }
            if (a == nullptr || others.size() < 3) {
                c.failures.emplace_back("composition offers no group with 2 present roles plus 3 other groups");
                c.composable = false;
                inconclusive = true;
            } else {
                json claims = json::array();
                for (const auto r : a->second) claims.push_back(presenceClaim(*a->first, r, true));
                for (const auto* o : others) claims.push_back(presenceClaim(*o->first, o->second.front(), true));
                if (const auto run = runCase(t, claims, sealedDirs[vi], work, "s5", c.failures)) {
                    const double k = static_cast<double>(a->second.size());
                    if (run->rep.presence.positives.status != "INSUFFICIENT_DATA") c.failures.push_back("positives " + run->rep.presence.positives.status);
                    if (!near(run->rep.presence.positives.nEff, (k + 3.0) * (k + 3.0) / (k * k + 3.0))) c.failures.emplace_back("n_eff differs from (k+3)^2/(k^2+3)");
                    if (run->rep.presenceStatus != "DESCRIPTIVE_ONLY") c.failures.push_back("status " + run->rep.presenceStatus);
                    compareClass("positives", run->rep.presence.positives, run->pos, c.failures);
                    c.presenceStatus = run->rep.presenceStatus;
                }
            }
            c.passed = c.failures.empty();
            res.cases.push_back(c);
        }
    }
    bool failed = false;
    for (const auto& c : res.cases) {
        failed = failed || (c.composable && !c.passed);
        if (!c.passed) res.reasons.push_back(c.version + " " + c.name + ": " + (c.failures.empty() ? std::string("failed") : c.failures.front()));
    }
    // A failed expectation always wins: INCONCLUSIVE never hides a FAIL
    res.status = failed ? "FAIL" : (inconclusive || res.cases.empty()) ? "INCONCLUSIVE" : "PASS";
    if (writeFiles) {
        res.manifestFilePath = (fs::path(outputDir) / "evaluation_check_selftest_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(res.manifestFilePath);
        res.manifestWriteSuccess = mf.is_open();
        if (res.manifestWriteSuccess) {
            mf << res.toJson();
            res.manifestWriteSuccess = mf.good();
        }
    }
    return res;
}

std::string SelftestResult::toJson() const {
    json cs = json::array();
    for (const auto& c : cases) {
        cs.push_back({{"version", c.version}, {"case", c.name}, {"passed", c.passed}, {"composable", c.composable}, {"presence_status", c.presenceStatus},
                      {"failures", c.failures}});
    }
    const json j{{"manifest_version", "1.0"},
                 {"scenario", "evaluation_check_selftest_v1"},
                 {"build_id", CYMATICA_BUILD_ID},
                 {"verdict", status},
                 {"gate", true},
                 {"reasons", reasons},
                 {"cases", cs},
                 {"rules", "ADR-0006 5e.4a: S1-S3 on v1, v2, v3; S4-S5 on v2, v3; S6 on v1; recomputation tolerance 1e-12"}};
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
