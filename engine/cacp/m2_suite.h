#pragma once

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

[[nodiscard]] SuiteResult runFullM2Suite(std::uint64_t packSeed, const std::string& outputDir = "artifacts/cacp",
                                         const std::string& sealedRoot = "artifacts/sealed");

} // namespace cymatica::cacp
