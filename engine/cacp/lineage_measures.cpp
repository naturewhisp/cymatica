#include "lineage_measures.h"

#include "development_measures.h"
#include "ensemble_tools.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace cymatica::cacp {

namespace {
constexpr std::size_t kVettoreRole = 3;
constexpr std::size_t kPhrasesPerSection = 8;
} // namespace

std::string runLineageMeasuresV1(const std::string& outputDir) {
    const FixturePlan plan = planFixtureM24V1(1337);
    if (!plan.ok || plan.traces.size() != plan.intentsPerPhrase.size()) return {};
    const auto tokens = fixturePhraseTokens(plan);
    const auto d = developmentDistanceMatrix(tokens, kVettoreRole, DevelopmentMeasure::M5r);
    const std::size_t n = tokens.size();

    // Vettore groups: phrases at M5r distance 0 share a group (first phrase of the group as id)
    std::vector<std::size_t> group(n, 0);
    for (std::size_t i = 0; i < n; ++i) {
        group[i] = i;
        for (std::size_t j = 0; j < i; ++j) {
            if (d[i][j] == 0.0) { group[i] = group[j]; break; }
        }
    }
    std::vector<std::size_t> distinct(group.begin(), group.end());
    std::sort(distinct.begin(), distinct.end());
    distinct.erase(std::unique(distinct.begin(), distinct.end()), distinct.end());

    std::ostringstream o;
    o << "{\n  \"scenario\": \"lineage_measures_v1\",\n  \"measure\": \"M5r on Vettore\",\n  \"phrases\": [\n";
    for (std::size_t i = 0; i < n; ++i) {
        const auto& t = plan.traces[i];
        o << "    {\"phrase\": " << i << ", \"base_source\": " << t.baseSourcePhrase << ", \"lineage_reset\": "
          << (t.lineageReset ? "true" : "false") << ", \"vettore_group\": " << group[i] << "}" << (i + 1 < n ? "," : "") << "\n";
    }
    o << "  ],\n  \"K3a_vettore_groups\": " << distinct.size() << ",\n";

    // K3b: per section, a group first heard in a derived phrase and heard again later in the same section
    o << "  \"K3b_sections_with_new_reused_material\": [";
    std::size_t k3bSections = 0;
    bool firstOut = true;
    for (std::size_t s = 0; s * kPhrasesPerSection < n; ++s) {
        const std::size_t a = s * kPhrasesPerSection;
        const std::size_t b = std::min(n, a + kPhrasesPerSection);
        bool hit = false;
        for (std::size_t i = a; i < b && !hit; ++i) {
            const bool derived = plan.traces[i].baseSourcePhrase >= 0 &&
                                 static_cast<std::size_t>(plan.traces[i].baseSourcePhrase) + 1 == i;
            if (!derived) continue;
            bool seenBefore = false;
            for (std::size_t j = a; j < i; ++j) seenBefore = seenBefore || group[j] == group[i];
            if (seenBefore) continue;
            for (std::size_t j = i + 1; j < b; ++j) hit = hit || group[j] == group[i];
        }
        if (hit) {
            ++k3bSections;
            o << (firstOut ? "" : ", ") << s;
            firstOut = false;
        }
    }
    o << "],\n  \"K3b_count\": " << k3bSections << ",\n";

    // K4: derived phrases closer to the declared parent than the median of the other earlier phrases of the section
    std::size_t k4Total = 0;
    std::size_t k4Hits = 0;
    o << "  \"K4_derived\": [";
    firstOut = true;
    for (std::size_t i = 0; i < n; ++i) {
        const auto& t = plan.traces[i];
        if (t.lineageReset || t.baseSourcePhrase < 0 || static_cast<std::size_t>(t.baseSourcePhrase) + 1 != i) continue;
        const std::size_t parent = static_cast<std::size_t>(t.baseSourcePhrase);
        const std::size_t a = (i / kPhrasesPerSection) * kPhrasesPerSection;
        std::vector<double> others;
        for (std::size_t j = a; j < i; ++j) if (j != parent) others.push_back(d[i][j]);
        if (others.empty()) continue;
        std::sort(others.begin(), others.end());
        const std::size_t m = others.size();
        const double median = (m % 2 == 1) ? others[m / 2] : 0.5 * (others[m / 2 - 1] + others[m / 2]);
        const bool hit = d[i][parent] <= median;
        ++k4Total;
        k4Hits += hit ? 1 : 0;
        o << (firstOut ? "" : ", ") << "{\"phrase\": " << i << ", \"d_parent\": " << d[i][parent] << ", \"median_others\": " << median
          << ", \"hit\": " << (hit ? "true" : "false") << "}";
        firstOut = false;
    }
    o << "],\n  \"K4_hits\": " << k4Hits << ",\n  \"K4_total\": " << k4Total << ",\n";

    // K5: phrase 16 recalls the opening of A, and phrase 0 is the nearest of phrases 1..15 (ties admitted)
    bool k5 = false;
    if (n > 16) {
        double nearestOther = 1e9;
        for (std::size_t j = 1; j < 16; ++j) nearestOther = std::min(nearestOther, d[16][j]);
        k5 = plan.traces[16].baseSourcePhrase == 0 && d[16][0] <= nearestOther;
        o << "  \"K5_d16_0\": " << d[16][0] << ",\n  \"K5_nearest_other_1_15\": " << nearestOther << ",\n";
    }
    o << "  \"K5_returns\": " << (k5 ? "true" : "false") << "\n}\n";

    const std::string json = o.str();
    std::error_code ec;
    std::filesystem::create_directories(outputDir, ec);
    std::ofstream f(std::filesystem::path(outputDir) / "lineage_measures_v1_manifest.json");
    f << json;
    return f.good() ? json : std::string{};
}

} // namespace cymatica::cacp
