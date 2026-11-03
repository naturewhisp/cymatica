#pragma once

#include "evaluation_pack.h"

#include <string>
#include <vector>

namespace cymatica::cacp {

// Scenario evaluation_check_selftest_v1 (ADR-0006 5e-02, DIF-5e.0-03, 5e.4a): prefabricated evaluation results run
// through runEvaluationCheck against the sealed truths of evaluation_pack_v1, v2 and v3. Expected outcomes fixed in
// ADR-0006 "5e.4a" before implementing:
//   S1 oracle (true value on every measurable role of every blind window, catch roles false): admissible, blind
//      qualified, every claim supported, catch passed, presence statistics equal to an independent recomputation
//      (groups, counts, n_eff, rate, Wilson, J_L) within 1e-12, status by the 5e-11 rule on the recomputation;
//   S2 always present: catch failed, EXCLUDED_BY_CATCH_RULE, no baseline comparison;
//   S3 always absent: catch passed, positive rate 0, negative rate 1, recomputation equal; J_L <= 0 if MEASURED;
//   S4 (v2, v3) positives on the test windows of exactly 3 groups: DESCRIPTIVE_ONLY;
//   S5 (v2, v3) 4 test groups, k >= 2 present roles in A and 1 in B, C, D: positives INSUFFICIENT_DATA, overall
//      DESCRIPTIVE_ONLY (negatives NOT_RUN);
//   S6 v1 compatibility: grouping rule source_phrase_only.
// FAIL when any composable case misses an expectation; otherwise INCONCLUSIVE when a truth is missing or S4/S5 cannot
// be composed; PASS only when every case ran and every expectation holds.

struct SelftestCase {
    std::string version{};        // evaluation_pack_v1, v2 or v3
    std::string name{};           // S1..S6
    bool passed{false};
    bool composable{true};        // false when the truth offers no composition for S4/S5 (INCONCLUSIVE, not FAIL)
    std::vector<std::string> failures{};
    std::string presenceStatus{};
};

struct SelftestResult {
    std::string status{"NOT_RUN"};   // PASS, FAIL or INCONCLUSIVE
    std::vector<std::string> reasons{};
    std::vector<SelftestCase> cases{};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};
    [[nodiscard]] std::string toJson() const;
};

// sealedDirs: the sealed directories of v1, v2 and v3 (in this order); workDir receives the result and report files
[[nodiscard]] SelftestResult runEvaluationCheckSelftestV1(const std::vector<std::string>& sealedDirs, const std::string& workDir,
                                                         const std::string& outputDir = "artifacts/cacp", bool writeFiles = true);

} // namespace cymatica::cacp
