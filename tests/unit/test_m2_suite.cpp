#include <catch2/catch_test_macros.hpp>

#include "m2_suite.h"

#include <filesystem>
#include <fstream>
#include <vector>

using namespace cymatica::cacp;

namespace {

SuiteStep step(const char* name, bool valid) {
    SuiteStep s;
    s.name = name;
    s.valid = valid;
    return s;
}

} // namespace

TEST_CASE("full_m2_suite: a measure without its manifest is not valid (DIF-5e.4a-01)", "[m2_suite]") {
    OnsetParityResult op;
    op.completed = true;
    op.inputValid = true;
    op.status = "MEASURED";
    op.manifestWriteSuccess = true;
    CHECK(onsetParityStepValid(op));
    op.manifestWriteSuccess = false;
    CHECK_FALSE(onsetParityStepValid(op));

    Acao02Result ac;
    ac.completed = true;
    ac.status = "MEASURED";
    ac.manifestWriteSuccess = true;
    CHECK(acao02StepValid(ac));
    ac.manifestWriteSuccess = false;
    CHECK_FALSE(acao02StepValid(ac));
    ac.manifestWriteSuccess = true;
    ac.status = "INCONCLUSIVE";
    CHECK_FALSE(acao02StepValid(ac));
}

TEST_CASE("full_m2_suite: an unwritable manifest path makes the onset parity step invalid and the suite FAIL (DIF-5e.4a-01)",
          "[m2_suite][slow]") {
    // A directory path below an existing regular file can never be created or written
    const auto root = std::filesystem::temp_directory_path() / "cymatica_m2_suite_unwritable";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    const auto blocker = root / "not_a_directory";
    { std::ofstream(blocker) << "x"; }
    const auto res = runOnsetParityV1((blocker / "out").string(), true);
    CHECK(res.completed);
    CHECK_FALSE(res.manifestWriteSuccess);
    CHECK_FALSE(onsetParityStepValid(res));
    std::vector<SuiteStep> steps{step("tone_test", true), step("onset_parity_v1", onsetParityStepValid(res))};
    CHECK(suiteStatus(steps) == "FAIL");
    std::filesystem::remove_all(root, ec);
}

TEST_CASE("full_m2_suite is a Release gate: ungated profile timings fail the step and the suite (DIF-5e.4a-02)", "[m2_suite]") {
    cymatica::audio::ProfileProbeResult p;
    p.passed = true;
    p.timingGated = true;
    CHECK(profileStepValid(p));
    p.timingGated = false; // Debug build: timings reported, not gated
    CHECK_FALSE(profileStepValid(p));
    p.passed = false;
    p.timingGated = true;
    CHECK_FALSE(profileStepValid(p));

    std::vector<SuiteStep> steps{step("tone_test", true), step("profile_test", false), step("procedural_v1", true)};
    CHECK(suiteStatus(steps) == "FAIL");
    steps[1].valid = true;
    CHECK(suiteStatus(steps) == "PASS");
    CHECK(suiteStatus({}) == "FAIL");
}
