#pragma once

#include "acao02_calibration.h"
#include "audio_probe.h"
#include "onset_parity.h"

#include <cstdint>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario full_m2_suite (ADR-0006 5e-02, updated in "5e.4a"): every M2 scenario in sequence, in one process.
// Local gate on the owner's hardware, Release; never run in CI. Exit 0 only when every binding scenario is PASS and
// every measurement and pack is valid; exploratory outcomes (H1/H2/R2, M-NORM, NULL_DIVERSITY, calibration
// generalisation, specificity) are reported, never a gate.

struct SuiteStep {
    std::string name{};
    std::string category{};      // binding, measurement, pack
    std::string verdict{"NOT_RUN"};
    bool valid{false};           // PASS for binding, valid measure or generated pack otherwise
    double seconds{0.0};
    std::string manifest{};
    std::vector<std::string> notes{};   // exploratory outcomes and reasons, never a gate
};

struct SuiteResult {
    std::string status{"NOT_RUN"};   // PASS or FAIL
    std::vector<SuiteStep> steps{};
    std::uint64_t packSeed{0};
    bool defaultPackSeed{false};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string toJson() const;
};

// Validity rules of the steps whose runner can report success without its manifest or without gated timings
// (DIF-5e.4a-01/02); pure, testable without running the suite
[[nodiscard]] bool onsetParityStepValid(const OnsetParityResult& r) noexcept;   // completed, valid input, manifest written
[[nodiscard]] bool acao02StepValid(const Acao02Result& r) noexcept;             // MEASURED and manifest written
// full_m2_suite is a Release gate: profile_test counts only when its timings were gated (a Debug build fails the step)
[[nodiscard]] bool profileStepValid(const audio::ProfileProbeResult& r) noexcept;
// PASS only when there is at least one step and every step is valid
[[nodiscard]] std::string suiteStatus(const std::vector<SuiteStep>& steps);

[[nodiscard]] SuiteResult runFullM2Suite(std::uint64_t packSeed, const std::string& outputDir = "artifacts/cacp",
                                         const std::string& sealedRoot = "artifacts/sealed");

} // namespace cymatica::cacp
