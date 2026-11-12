#pragma once

#include "diversity.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Offline ensemble and development tool (M2.5.1c, R-M25-7). Measures, never a gate; not part of the planner
// scoring. Everything here is fixed by the pre-registration docs/research/M2_5_1c_Ensemble_Tools_Preregistration.md
// (SHA-256 below), written before the implementation and the first run.
inline constexpr const char* kEnsemblePreregistration = "docs/research/M2_5_1c_Ensemble_Tools_Preregistration.md";
inline constexpr const char* kEnsemblePreregistrationSha256 =
    "30ac2cb52c488b2a3ba35badda4a0f116142d77dc3b61164237fe5f50113e0a7";
inline constexpr std::size_t kEnsemblePermutations = 1000;
inline constexpr std::uint64_t kEnsembleNullSeed = 0x52254D37ULL;      // permutation null
inline constexpr std::uint64_t kEnsembleFixtureSeed = 0x46495854ULL;   // adversarial fixtures
inline constexpr std::uint32_t kEnsembleStepsPerBar = 16;
inline constexpr std::uint32_t kEnsembleBarsPerPhrase = 4;
inline constexpr std::size_t kEnsembleSyntheticPhrases = 24;
inline constexpr const char* kEnsemblePositiveControlPath = "docs/research/experiments/m25_ensemble_tools/positive_control.json";

// Tokens of one phrase (all roles), onset in sixteenths from the phrase start
using PhraseTokens = std::vector<NoteToken>;

// Per-phrase tokens of a planned fixture (same grid and pitch rounding as planTokens)
[[nodiscard]] std::vector<PhraseTokens> fixturePhraseTokens(const FixturePlan& plan);

struct VariogramPoint {
    std::size_t lag{0};
    std::size_t pairs{0};
    double observed{0.0};
    double nullMean{0.0};
    double nullLow{0.0};    // 2.5 % quantile (nearest rank)
    double nullHigh{0.0};   // 97.5 % quantile (nearest rank)
    double pLower{1.0};     // (1 + #{null <= observed}) / (M + 1)
};

struct RoleVariogram {
    std::size_t role{0};
    std::size_t phrasesWithNotes{0};
    bool degenerate{false};                 // every distance is 0
    std::string classification{"NOT_RUN"};  // DEGENERATE, ORDERED, ORDER_FREE, ANTI_ORDERED, NO_DATA
    std::optional<double> spearmanRho{};    // k vs V(k) for k = 1..P/2; nullopt when undefined
    std::vector<VariogramPoint> points{};   // k = 1..P-1
};

// Normalised Levenshtein (M4) between phrases i and j of one role
[[nodiscard]] std::vector<std::vector<double>> roleDistanceMatrix(const std::vector<PhraseTokens>& phrases, std::size_t role);

// Variogram of one role with the permutation null (pre-registration §1)
[[nodiscard]] RoleVariogram roleVariogram(const std::vector<PhraseTokens>& phrases, std::size_t role,
                                          std::size_t permutations = kEnsemblePermutations, std::uint64_t seed = kEnsembleNullSeed);

// Spearman rho with average ranks for ties; nullopt when fewer than 3 points or a constant series
[[nodiscard]] std::optional<double> spearmanRho(const std::vector<double>& x, const std::vector<double>& y);

// Adversarial fixtures F1..F6 of the pre-registration §2 (Vettore only, D Dorian, 24 phrases of 4 bars)
enum class AdversarialFixture : std::uint8_t { Loop = 1, Noise, Development, ShuffledDevelopment, RandomTransposition, RandomOrnaments };
[[nodiscard]] std::vector<PhraseTokens> adversarialFixture(AdversarialFixture id, std::uint64_t seed = kEnsembleFixtureSeed);
[[nodiscard]] const char* adversarialFixtureName(AdversarialFixture id) noexcept;

// F7: positive control written by a person, JSON {"phrases": [[{"onset": 0, "midi": 62, "duration": 4}, ...], ...]};
// nullopt with the reason in `error` when the file is missing or invalid (the fixture is then NOT_RUN). Invalid also
// when fewer than 2 phrases carry notes, because no distance can be computed (DIF-M2.5.1c-STEP4-02; not a quality
// threshold: the request for about 24 phrases stays a documentary requirement).
[[nodiscard]] std::optional<std::vector<PhraseTokens>> loadPositiveControl(const std::string& path, std::string& error);

// Vertical statistics on the integer grid (pre-registration §5), per section of fixture-m2-4-v1
struct SectionVertical {
    std::uint32_t section{0};
    std::array<std::size_t, kRoleCount> onsets{};
    std::array<std::array<std::size_t, kRoleCount>, kRoleCount> coincidences{}; // [r][s], r < s, same sixteenth
    // Trama (index 0) and Vettore (index 1): onsets on a chord tone of the bar, on the downbeat and elsewhere
    std::array<std::size_t, 2> downbeatOnsets{};
    std::array<std::size_t, 2> downbeatChordTones{};
    std::array<std::size_t, 2> otherOnsets{};
    std::array<std::size_t, 2> otherChordTones{};
};
[[nodiscard]] std::vector<SectionVertical> fixtureVerticalStats(const FixturePlan& plan);

// Logical cost of the designs A, B, D, G (pre-registration §6), in step evaluations per phrase
struct DesignCost {
    std::string design;
    std::string formula;
    std::uint64_t stepEvaluations{0};
    bool hypotheticalParameters{false};
};
[[nodiscard]] std::vector<DesignCost> designCostEstimates();

struct EnsembleToolsResult {
    // MEASURED; INCONCLUSIVE when the pre-registration is missing or its hash differs (DIF-M2.5.1c-STEP4-01) or the
    // fixture cannot be planned. The manifest is still written for diagnosis.
    std::string status{"NOT_RUN"};
    std::string error{};
    std::string preregistrationPath{};
    std::string preregistrationSha256{};  // computed on the file, LF line endings; empty when unreadable
    bool preregistrationMatches{false};
    struct FixtureOutcome {
        std::string id;
        std::string name;
        std::string status{"NOT_RUN"};      // MEASURED, NOT_RUN (no file) or INCONCLUSIVE (variogram not computable)
        std::string reason{};
        std::string expectedClassification{};
        RoleVariogram vettore{};
    };
    std::vector<FixtureOutcome> adversarial{};
    bool horseCriterionMet{false};      // pre-registration §3: F3 ORDERED with rho > 0, F2 and F4 ORDER_FREE, F1 DEGENERATE
    std::vector<std::string> missedPredictions{};
    std::array<RoleVariogram, kRoleCount> fixtureRoles{};
    std::vector<SectionVertical> vertical{};
    std::vector<DesignCost> costs{};
    std::string manifestFilePath{};
    bool manifestWriteSuccess{false};

    [[nodiscard]] std::string toJson() const;
};

// preregistrationPath: the frozen pre-registration; relative paths resolve against the working directory
[[nodiscard]] EnsembleToolsResult runEnsembleToolsV1(const std::string& outputDir, bool writeFiles,
                                                     const std::string& preregistrationPath = kEnsemblePreregistration,
                                                     const std::string& positiveControlPath = kEnsemblePositiveControlPath);

} // namespace cymatica::cacp
