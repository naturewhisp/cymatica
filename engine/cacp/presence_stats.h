#pragma once

#include <cstddef>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace cymatica::cacp {

// presence_stats_v1 (ADR-0006 precisions 5e-09 and 5e-11, DIF-5e.0-02; dossier R5 §6.1). Statistics of the
// role_signal_present claims of an evaluator against the measured truth, with the musical group as the unit of
// independence. Never a gate. Fixed before any measurement:
//   - unit: musical group (same source phrase, removal variants and duplicate included, or same per-phrase
//     content fingerprint); within a group rho = 1, between groups independence is assumed;
//   - per class (positives, negatives): n_eff = (sum m_c)^2 / sum m_c^2, x_eff = p * n_eff, Wilson 95 %;
//   - DESCRIPTIVE_ONLY with fewer than 4 groups in a class; INSUFFICIENT_DATA with n_eff < 4; otherwise MEASURED;
//   - baseline: J_L = L_se + L_sp - 1 > 0 only when both classes are MEASURED;
//   - intervals are labelled model_based_unverified.
inline constexpr const char* kPresenceStatsProfile = "presence_stats_v1";
inline constexpr std::size_t kPresenceStatsMinGroups = 4;
inline constexpr double kPresenceStatsMinNeff = 4.0;
inline constexpr double kPresenceStatsZ = 1.959963984540054;   // Wilson 95 %
inline constexpr double kPresenceStatsRhoAssumed = 1.0;        // within a group

struct PresenceObservation {
    std::string group{};
    std::string window{};
    int phrase{-1};
    std::string role{};
    bool truthPresent{false};
    std::string negativeKind{};   // "natural" or "removal" for a negative, empty for a positive
    bool claimedPresent{false};
};

struct ClassStats {
    std::string status{"NOT_RUN"};   // MEASURED, INSUFFICIENT_DATA, DESCRIPTIVE_ONLY or NOT_RUN (no claim)
    std::size_t claims{0};
    std::size_t correct{0};
    std::size_t windows{0};
    std::size_t phrases{0};
    std::size_t groups{0};
    double nEff{0.0};
    double rate{std::numeric_limits<double>::quiet_NaN()};   // p, descriptive in every status with claims
    double low{std::numeric_limits<double>::quiet_NaN()};    // Wilson on n_eff, MEASURED only
    double high{std::numeric_limits<double>::quiet_NaN()};
};

struct PresenceStats {
    std::string profile{kPresenceStatsProfile};
    std::string coverage{"model_based_unverified"};
    ClassStats positives{};
    ClassStats negatives{};
    ClassStats naturalNegatives{};   // reported apart (different mechanism)
    ClassStats removalNegatives{};
    std::map<std::string, ClassStats> negativesPerRole{};   // balance per role
    std::map<std::string, ClassStats> positivesPerRole{};
    double youdenHat{std::numeric_limits<double>::quiet_NaN()};
    double youdenLow{std::numeric_limits<double>::quiet_NaN()};   // J_L, both classes MEASURED only
    double balancedAccuracy{std::numeric_limits<double>::quiet_NaN()};
    std::optional<bool> aboveTrivialBaseline{};                   // nullopt unless both classes MEASURED
    // Secondary count (dossier R5 §6.1): windows of the sample whose claims are all correct
    std::size_t windowsInSample{0};
    std::size_t windowsAllCorrect{0};
};

// Kish effective sample size of a cluster partition: (sum m)^2 / sum m^2; 0 for no observation
[[nodiscard]] double effectiveSampleSize(const std::vector<std::size_t>& clusterSizes) noexcept;
// Wilson 95 % interval for a possibly fractional count (x_eff successes out of n_eff)
[[nodiscard]] std::pair<double, double> wilsonEffective(double successes, double trials) noexcept;
[[nodiscard]] ClassStats classStats(const std::vector<const PresenceObservation*>& obs);
[[nodiscard]] PresenceStats computePresenceStats(const std::vector<PresenceObservation>& obs);

} // namespace cymatica::cacp
