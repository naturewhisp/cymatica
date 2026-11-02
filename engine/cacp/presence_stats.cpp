#include "presence_stats.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace cymatica::cacp {

double effectiveSampleSize(const std::vector<std::size_t>& clusterSizes) noexcept {
    double sum = 0.0, sumSq = 0.0;
    for (const auto m : clusterSizes) {
        sum += static_cast<double>(m);
        sumSq += static_cast<double>(m) * static_cast<double>(m);
    }
    return sumSq > 0.0 ? sum * sum / sumSq : 0.0;
}

std::pair<double, double> wilsonEffective(double successes, double trials) noexcept {
    const double nan = std::numeric_limits<double>::quiet_NaN();
    if (!(trials > 0.0) || !std::isfinite(trials) || !std::isfinite(successes) || successes < 0.0 || successes > trials) {
        return {nan, nan};
    }
    constexpr double z = 1.959963984540054;
    const double p = successes / trials;
    const double denom = 1.0 + z * z / trials;
    const double centre = (p + z * z / (2.0 * trials)) / denom;
    const double half = z * std::sqrt(p * (1.0 - p) / trials + z * z / (4.0 * trials * trials)) / denom;
    return {(std::max)(0.0, centre - half), (std::min)(1.0, centre + half)};
}

ClassStats classStats(const std::vector<const PresenceObservation*>& obs) {
    ClassStats c;
    if (obs.empty()) return c;
    std::map<std::string, std::size_t> perGroup;
    std::set<std::string> windows;
    std::set<int> phrases;
    for (const auto* o : obs) {
        ++c.claims;
        if (o->claimedPresent == o->truthPresent) ++c.correct;
        ++perGroup[o->group];
        windows.insert(o->window);
        if (o->phrase >= 0) phrases.insert(o->phrase);
    }
    c.windows = windows.size();
    c.phrases = phrases.size();
    c.groups = perGroup.size();
    std::vector<std::size_t> sizes;
    for (const auto& [g, m] : perGroup) sizes.push_back(m);
    c.nEff = effectiveSampleSize(sizes);
    c.rate = static_cast<double>(c.correct) / static_cast<double>(c.claims);
    if (c.groups < kPresenceStatsMinGroups) {
        c.status = "DESCRIPTIVE_ONLY";
    } else if (c.nEff < kPresenceStatsMinNeff) {
        c.status = "INSUFFICIENT_DATA";
    } else {
        c.status = "MEASURED";
        const auto [lo, hi] = wilsonEffective(c.rate * c.nEff, c.nEff);
        c.low = lo;
        c.high = hi;
    }
    return c;
}

PresenceStats computePresenceStats(const std::vector<PresenceObservation>& obs) {
    PresenceStats s;
    std::vector<const PresenceObservation*> pos, neg, natural, removal;
    std::map<std::string, std::vector<const PresenceObservation*>> negRole, posRole;
    for (const auto& o : obs) {
        if (o.truthPresent) {
            pos.push_back(&o);
            posRole[o.role].push_back(&o);
        } else {
            neg.push_back(&o);
            negRole[o.role].push_back(&o);
            (o.negativeKind == "removal" ? removal : natural).push_back(&o);
        }
    }
    s.positives = classStats(pos);
    s.negatives = classStats(neg);
    s.naturalNegatives = classStats(natural);
    s.removalNegatives = classStats(removal);
    for (const auto& [role, v] : negRole) s.negativesPerRole[role] = classStats(v);
    for (const auto& [role, v] : posRole) s.positivesPerRole[role] = classStats(v);
    if (s.positives.claims > 0 && s.negatives.claims > 0) {
        s.youdenHat = s.positives.rate + s.negatives.rate - 1.0;
        s.balancedAccuracy = (s.youdenHat + 1.0) / 2.0;
    }
    if (s.positives.status == "MEASURED" && s.negatives.status == "MEASURED") {
        s.youdenLow = s.positives.low + s.negatives.low - 1.0;
        s.aboveTrivialBaseline = s.youdenLow > 0.0;
    }
    return s;
}

} // namespace cymatica::cacp
