#include "m2_suite.h"

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "check_selftest.h"
#include "evaluation_pack.h"
#include "onset_parity.h"
#include "procedural_scenario.h"
#include "release_ab.h"
#include "trama_diagnosis.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <sstream>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

std::string fmt3(double v) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(3) << v;
    return s.str();
}

std::string path(const std::string& dir, const char* file) {
    return (std::filesystem::path(dir) / file).lexically_normal().string();
}

void timed(SuiteResult& r, const char* name, const char* category, const std::function<void(SuiteStep&)>& body) {
    SuiteStep s;
    s.name = name;
    s.category = category;
    const auto t0 = std::chrono::steady_clock::now();
    body(s);
    s.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("[full_m2_suite] %-30s %-12s %-12s %s (%.1f s)\n", name, category, s.verdict.c_str(), s.valid ? "ok" : "NOT OK",
                s.seconds);
    std::fflush(stdout);
    r.steps.push_back(std::move(s));
}

// The pack is generated inside the timed body, so that its duration is measured
void packStep(SuiteResult& r, const char* name, const std::function<EvaluationPackRun()>& generate) {
    timed(r, name, "pack", [&](SuiteStep& s) {
        const EvaluationPackRun run = generate();
        s.verdict = run.status;
        s.valid = run.ok && run.status == "GENERATED" && run.repeatIdentical && run.manifestWriteSuccess;
        s.manifest = run.manifestFilePath;
        if (!run.repeatIdentical) s.notes.emplace_back("repeat not byte-identical");
        for (const auto& e : run.errors) s.notes.push_back(e);
        if (run.musicalGroups > 0) {
            s.notes.push_back("musical groups " + std::to_string(run.musicalGroups) + ", expected n_eff positives " +
                              fmt3(run.expectedPositiveNeff) + ", negatives " + fmt3(run.expectedNegativeNeff));
            // Exploratory (OSS-5e.4a-01): a class under the n_eff threshold cannot be MEASURED with this pack
            for (const auto& [cls, neff] : {std::pair{"sensitivity", run.expectedPositiveNeff}, std::pair{"specificity", run.expectedNegativeNeff}}) {
                if (neff < 4.0) s.notes.push_back(std::string("exploratory: ") + cls + " INSUFFICIENT_DATA expected (n_eff " + fmt3(neff) + " < 4)");
            }
        }
    });
}

} // namespace

bool onsetParityStepValid(const OnsetParityResult& r) noexcept { return r.completed && r.inputValid && r.manifestWriteSuccess; }

bool acao02StepValid(const Acao02Result& r) noexcept { return r.completed && r.status == "MEASURED" && r.manifestWriteSuccess; }

bool profileStepValid(const audio::ProfileProbeResult& r) noexcept { return r.passed && r.timingGated; }

std::string suiteStatus(const std::vector<SuiteStep>& steps) {
    bool ok = !steps.empty();
    for (const auto& s : steps) ok = ok && s.valid;
    return ok ? "PASS" : "FAIL";
}

SuiteResult runFullM2Suite(std::uint64_t packSeed, const std::string& outputDir, const std::string& sealedRoot) {
    SuiteResult r;
    r.packSeed = packSeed;
    r.defaultPackSeed = packSeed == kDefaultPackSeed;

    timed(r, "tone_test", "binding", [&](SuiteStep& s) {
        const auto res = audio::runToneProbeV0Offline(outputDir, true);
        s.verdict = res.passed ? "PASS" : "FAIL";
        s.valid = res.passed;
        s.manifest = path(outputDir, "tone_probe_v0_manifest.json");
    });
    timed(r, "voice_test", "binding", [&](SuiteStep& s) {
        const auto res = audio::runVoiceProbeV1Offline(outputDir, true);
        s.verdict = res.passed ? "PASS" : "FAIL";
        s.valid = res.passed;
        s.manifest = path(outputDir, "voice_probe_v1_manifest.json");
    });
    timed(r, "timbre_test", "binding", [&](SuiteStep& s) {
        const auto res = audio::runTimbreBusProbeV1Offline(outputDir, true);
        s.verdict = res.passed ? "PASS" : "FAIL";
        s.valid = res.passed;
        s.manifest = path(outputDir, "timbre_bus_probe_v1_manifest.json");
    });
    timed(r, "planner_test", "binding", [&](SuiteStep& s) {
        const auto res = audio::runPlannerProbeV1Offline(outputDir, true);
        s.verdict = res.passed ? "PASS" : "FAIL";
        s.valid = res.passed;
        s.manifest = path(outputDir, "planner_probe_v1_manifest.json");
    });
    timed(r, "fixture_test", "binding", [&](SuiteStep& s) {
        const auto res = audio::runFixtureProbeV1Offline(outputDir, true);
        const bool golden = res.goldenPcmStatus == "PASS";
        s.verdict = res.passed && golden ? "PASS" : "FAIL";
        s.valid = res.passed && golden;
        s.manifest = path(outputDir, "fixture_probe_v1_manifest.json");
        s.notes.push_back("golden PCM " + res.goldenPcmStatus);
    });
    timed(r, "profile_test", "binding", [&](SuiteStep& s) {
        const auto res = audio::runProfileProbeV1Offline(outputDir, true);
        s.valid = profileStepValid(res);
        s.verdict = s.valid ? "PASS" : "FAIL";
        s.manifest = path(outputDir, "profile_probe_v1_manifest.json");
        if (!res.timingGated) {
            s.notes.emplace_back("timings not gated (not a Release build): full_m2_suite is a Release gate (DIF-5e.4a-02)");
        }
    });
    timed(r, "onset_parity_v1", "measurement", [&](SuiteStep& s) {
        const auto res = runOnsetParityV1(outputDir, true);
        s.verdict = res.status;
        s.valid = onsetParityStepValid(res);
        s.manifest = path(outputDir, "onset_parity_v1_manifest.json");
        if (!res.manifestWriteSuccess) s.notes.emplace_back("manifest not written (DIF-5e.4a-01)");
    });
    timed(r, "acao02_v1", "measurement", [&](SuiteStep& s) {
        const auto res = runAcao02V1(outputDir, true);
        s.verdict = res.status;
        s.valid = acao02StepValid(res);
        s.manifest = path(outputDir, "acao02_v1_manifest.json");
        if (!res.manifestWriteSuccess) s.notes.emplace_back("manifest not written (DIF-5e.4a-01)");
        for (const auto& why : res.inconclusiveReasons) s.notes.push_back(why);
        // Exploratory (OSS-5e.4a-01): the multi-seed generalisation of the calibration
        if (res.distinctFingerprints <= 1) {
            s.notes.push_back("exploratory: multi-seed generalisation of the calibration INCONCLUSIVE (" +
                              std::to_string(res.distinctFingerprints) + " distinct content among the scanned seeds)");
        }
    });
    const std::string v1Sealed = path(sealedRoot, "evaluation_pack_v1");
    const std::string v2Sealed = path(sealedRoot, "evaluation_pack_v2");
    const std::string v3Sealed = path(sealedRoot, "evaluation_pack_v3");
    packStep(r, "evaluation_pack_v1", [&] { return runEvaluationPackV1(path(outputDir, "evaluation_pack_v1"), v1Sealed, packSeed); });
    packStep(r, "evaluation_pack_v2", [&] { return runEvaluationPackV2(path(outputDir, "evaluation_pack_v2"), v2Sealed, packSeed); });
    packStep(r, "evaluation_pack_v3", [&] { return runEvaluationPackV3(path(outputDir, "evaluation_pack_v3"), v3Sealed, packSeed); });
    timed(r, "evaluation_check_selftest_v1", "binding", [&](SuiteStep& s) {
        const auto res = runEvaluationCheckSelftestV1({v1Sealed, v2Sealed, v3Sealed}, path(outputDir, "evaluation_check_selftest_v1"),
                                                      outputDir, true);
        s.verdict = res.status;
        s.valid = res.status == "PASS" && res.manifestWriteSuccess;
        s.manifest = res.manifestFilePath;
        for (const auto& why : res.reasons) s.notes.push_back(why);
    });
    timed(r, "release_ab_v1", "measurement", [&](SuiteStep& s) {
        const auto res = runReleaseAbV1(outputDir, true);
        s.verdict = res.status;
        s.valid = res.status == "MEASURED" && res.repeatIdentical && res.manifestWriteSuccess;
        s.manifest = res.manifestFilePath;
        s.notes.push_back("exploratory: H1 " + res.h1.outcome + ", H2 " + res.h2.outcome + ", R2 short-IOI " + res.r2ShortIoi.outcome);
    });
    timed(r, "trama_miss_diagnosis_v1", "measurement", [&](SuiteStep& s) {
        const auto res = runTramaMissDiagnosisV1(outputDir, true);
        s.verdict = res.status;
        s.valid = res.status == "MEASURED" && res.repeatIdentical && res.manifestWriteSuccess && res.curvesWriteSuccess;
        s.manifest = res.manifestFilePath;
        s.notes.push_back("exploratory: M-NORM " + res.causal.outcome);
    });
    timed(r, "procedural_v1", "binding", [&](SuiteStep& s) {
        const auto res = runProceduralV1(outputDir, true);
        s.verdict = res.status;
        s.valid = res.status == "PASS" && res.manifestWriteSuccess;
        s.manifest = path(outputDir, "procedural_v1_manifest.json");
        for (const auto& f : res.failures) s.notes.push_back(f);
        for (const auto& f : res.findings) s.notes.push_back("finding: " + f);
        // Exploratory (OSS-5e.4a-01): seed diversity outcome of the declared scan
        if (!res.diversityJson.empty()) {
            const json d = json::parse(res.diversityJson, nullptr, false);
            if (d.is_object() && d.contains("outcome") && d["outcome"].is_string()) {
                s.notes.push_back("exploratory: seed diversity " + d["outcome"].get<std::string>());
            }
        }
    });

    r.status = suiteStatus(r.steps);
    r.manifestFilePath = path(outputDir, "full_m2_suite_manifest.json");
    std::ofstream mf(r.manifestFilePath);
    r.manifestWriteSuccess = mf.is_open();
    if (r.manifestWriteSuccess) {
        mf << r.toJson();
        r.manifestWriteSuccess = mf.good();
    }
    return r;
}

std::string SuiteResult::toJson() const {
    json steps_ = json::array();
    double total = 0.0;
    for (const auto& s : steps) {
        total += s.seconds;
        steps_.push_back({{"name", s.name}, {"category", s.category}, {"verdict", s.verdict}, {"valid", s.valid},
                          {"seconds", s.seconds}, {"manifest", s.manifest}, {"notes", s.notes}});
    }
    const json j{{"manifest_version", "1.0"},
                 {"scenario", "full_m2_suite"},
                 {"build_id", CYMATICA_BUILD_ID},
                 {"verdict", status},
                 {"local_gate", true},
                 {"ci", false},
                 {"pack_seed_commitment_sha256", packSeedCommitment(packSeed)},
                 {"default_pack_seed", defaultPackSeed},
                 {"blind_session_valid_packs", false},
                 {"total_seconds", total},
                 {"steps", steps_},
                 {"rules", {{"source", "ADR-0006 5e-02, 5e.4a"},
                            {"exit_zero", "every binding scenario PASS and every measurement and pack valid"},
                            {"exploratory", "H1/H2/R2, M-NORM, NULL_DIVERSITY, calibration generalisation and specificity "
                                            "are reported, never a gate"},
                            {"packs", "the blind session is deferred to M2.5: these packs are never handed to a blind evaluator"}}}};
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
