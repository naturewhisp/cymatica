#include "development_measures.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <map>
#include <fstream>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::size_t kCorpoRole = 1;
constexpr std::size_t kVettoreRole = 3;

struct Step {
    int interval{0};
    std::int64_t ioi{1};
};

std::vector<Step> stepsOf(const PhraseTokens& p, std::size_t role) {
    std::vector<const NoteToken*> notes;
    for (const auto& t : p) {
        if (t.role == role && t.midi >= 0) notes.push_back(&t);
    }
    std::stable_sort(notes.begin(), notes.end(), [](const NoteToken* a, const NoteToken* b) { return a->onset < b->onset; });
    std::vector<Step> out;
    for (std::size_t i = 1; i < notes.size(); ++i) {
        out.push_back({notes[i]->midi - notes[i - 1]->midi, (std::max<std::int64_t>)(1, notes[i]->onset - notes[i - 1]->onset)});
    }
    return out;
}

double substitution(const Step& a, const Step& b, bool withRhythm) {
    const double di = (std::min)(std::abs(a.interval - b.interval), 12) / 12.0;
    if (!withRhythm) return di;
    const double dr = (std::min)(std::abs(std::log2(static_cast<double>(a.ioi) / static_cast<double>(b.ioi))), 2.0) / 2.0;
    return 0.5 * di + 0.5 * dr;
}

json validationJson(const MeasureValidation& m) {
    json adv = json::array();
    for (const auto& [id, c] : m.adversarial) adv.push_back({{"id", id}, {"classification", c}});
    return {{"measure", m.measure},
            {"V1_adversarial", adv},
            {"V1_horse", m.v1Horse},
            {"V2_lineage_ranks", m.lineageRanks},
            {"V2_hits_rank_1", m.lineageHits},
            {"V2_mrr", m.lineageMrr},
            {"V2_lineage", m.v2Lineage},
            {"inversion_distances_6_from_1_and_14_from_6", m.inversionDistances},
            {"V3_f9_top5_peaks_1_based", m.f9Peaks},
            {"V3_f9_boundaries_hit", m.f9BoundariesHit},
            {"V3_sections_f9", m.v3SectionsF9},
            {"V4_corpo_top2_peaks_1_based", m.corpoPeaks},
            {"V4_sections_fixture", m.v4SectionsFixture},
            {"F8_classification_descriptive", m.f8Classification},
            {"admitted", m.admitted}};
}

} // namespace

double gradedDistance(const PhraseTokens& a, const PhraseTokens& b, std::size_t role, bool withRhythm) {
    const auto x = stepsOf(a, role), y = stepsOf(b, role);
    const bool ea = std::none_of(a.begin(), a.end(), [&](const NoteToken& t) { return t.role == role; });
    const bool eb = std::none_of(b.begin(), b.end(), [&](const NoteToken& t) { return t.role == role; });
    if (ea && eb) return 0.0;
    if (ea != eb) return 1.0;
    if (x.empty() && y.empty()) return 0.0;
    std::vector<double> prev(y.size() + 1), cur(y.size() + 1);
    for (std::size_t j = 0; j <= y.size(); ++j) prev[j] = static_cast<double>(j);
    for (std::size_t i = 1; i <= x.size(); ++i) {
        cur[0] = static_cast<double>(i);
        for (std::size_t j = 1; j <= y.size(); ++j) {
            cur[j] = (std::min)({prev[j] + 1.0, cur[j - 1] + 1.0, prev[j - 1] + substitution(x[i - 1], y[j - 1], withRhythm)});
        }
        std::swap(prev, cur);
    }
    return prev[y.size()] / static_cast<double>((std::max)(x.size(), y.size()));
}

std::vector<std::vector<double>> developmentDistanceMatrix(const std::vector<PhraseTokens>& phrases, std::size_t role, DevelopmentMeasure m) {
    if (m == DevelopmentMeasure::M4) return roleDistanceMatrix(phrases, role);
    const std::size_t n = phrases.size();
    std::vector<std::vector<double>> d(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) d[i][j] = d[j][i] = gradedDistance(phrases[i], phrases[j], role, m == DevelopmentMeasure::M5r);
    }
    return d;
}

std::size_t parentRank(const std::vector<std::vector<double>>& d, std::size_t child, std::size_t parent) {
    if (child >= d.size() || parent >= child) return 0;
    std::size_t rank = 1;
    for (std::size_t k = 0; k < child; ++k) {
        if (k != parent && d[child][k] < d[child][parent]) ++rank;
    }
    return rank;
}

std::vector<double> footeNovelty(const std::vector<std::vector<double>>& d, std::size_t w) {
    const std::size_t n = d.size();
    std::vector<double> nov(n, 0.0);
    for (std::size_t i = w; i + w <= n; ++i) {
        double acc = 0.0;
        for (std::size_t a = i - w; a < i + w; ++a) {
            for (std::size_t b = i - w; b < i + w; ++b) {
                const bool sameSide = (a < i) == (b < i);
                acc += (sameSide ? 1.0 : -1.0) * (1.0 - d[a][b]);
            }
        }
        nov[i] = acc;
    }
    return nov;
}

std::vector<std::size_t> topPeaks(const std::vector<double>& nov, std::size_t k) {
    std::vector<std::size_t> peaks;
    for (std::size_t i = 1; i + 1 < nov.size(); ++i) {
        if (nov[i] > nov[i - 1] && nov[i] >= nov[i + 1]) peaks.push_back(i);
    }
    std::stable_sort(peaks.begin(), peaks.end(), [&](std::size_t a, std::size_t b) { return nov[a] > nov[b]; });
    if (peaks.size() > k) peaks.resize(k);
    return peaks;
}

DevelopmentMeasuresResult runDevelopmentMeasuresV1(const std::string& outputDir, bool writeFiles, const std::string& controlsDir) {
    namespace fs = std::filesystem;
    auto once = [&](DevelopmentMeasuresResult& r) {
        std::string error;
        const auto f9 = loadPositiveControl((fs::path(controlsDir) / "F9_subagent.json").string(), error);
        if (!f9) {
            r.reasons.push_back("F9: " + error);
            return;
        }
        const auto f8 = loadPositiveControl((fs::path(controlsDir) / "F8_bach.json").string(), error);
        const auto corpoPlan = fixturePhraseTokens(planFixtureM24V1(1337));
        // Declared truths (pre-registration §1, 0-based indices)
        const std::vector<std::pair<std::size_t, std::size_t>> derivations{{2, 0}, {4, 0}, {12, 0}, {14, 0}, {20, 0}, {21, 1}};
        const std::vector<std::size_t> f9Starts{6, 10, 14, 18, 20};
        const std::vector<std::size_t> corpoStarts{8, 16};
        for (const DevelopmentMeasure m : {DevelopmentMeasure::M4, DevelopmentMeasure::M5, DevelopmentMeasure::M5r}) {
            MeasureValidation v;
            v.measure = m == DevelopmentMeasure::M4 ? "M4" : (m == DevelopmentMeasure::M5 ? "M5" : "M5r");
            std::map<std::string, RoleVariogram> adv;
            for (const auto id : {AdversarialFixture::Loop, AdversarialFixture::Noise, AdversarialFixture::Development,
                                  AdversarialFixture::ShuffledDevelopment, AdversarialFixture::RandomTransposition,
                                  AdversarialFixture::RandomOrnaments}) {
                const auto ph = adversarialFixture(id);
                std::size_t withNotes = 0;
                for (const auto& p : ph) withNotes += std::any_of(p.begin(), p.end(), [](const NoteToken& t) { return t.role == kVettoreRole; }) ? 1 : 0;
                const std::string key = "F" + std::to_string(static_cast<int>(id));
                adv[key] = variogramOfMatrix(developmentDistanceMatrix(ph, kVettoreRole, m), kVettoreRole, withNotes);
                v.adversarial.emplace_back(key, adv[key].classification);
            }
            v.v1Horse = adv["F1"].classification == "DEGENERATE" && adv["F3"].classification == "ORDERED" && adv["F3"].spearmanRho &&
                        *adv["F3"].spearmanRho > 0.0 && adv["F2"].classification == "ORDER_FREE" && adv["F4"].classification == "ORDER_FREE";
            const auto d9 = developmentDistanceMatrix(*f9, kVettoreRole, m);
            double mrr = 0.0;
            for (const auto& [child, parent] : derivations) {
                const std::size_t rank = parentRank(d9, child, parent);
                v.lineageRanks.push_back(rank);
                if (rank == 1) ++v.lineageHits;
                mrr += rank > 0 ? 1.0 / static_cast<double>(rank) : 0.0;
            }
            v.lineageMrr = mrr / static_cast<double>(derivations.size());
            v.v2Lineage = v.lineageHits >= 4 && v.lineageMrr >= 0.6;
            v.inversionDistances = {d9[5][0], d9[13][5]};
            for (const auto p : topPeaks(footeNovelty(d9), 5)) v.f9Peaks.push_back(p + 1);
            for (const auto s : f9Starts) {
                if (std::any_of(v.f9Peaks.begin(), v.f9Peaks.end(), [&](std::size_t p1) {
                        const long diff = static_cast<long>(p1 - 1) - static_cast<long>(s);
                        return diff >= -1 && diff <= 1;
                    })) {
                    ++v.f9BoundariesHit;
                }
            }
            v.v3SectionsF9 = v.f9BoundariesHit >= 3;
            const auto dc = developmentDistanceMatrix(corpoPlan, kCorpoRole, m);
            for (const auto p : topPeaks(footeNovelty(dc), 2)) v.corpoPeaks.push_back(p + 1);
            v.v4SectionsFixture = v.corpoPeaks.size() == 2;
            for (const auto s : corpoStarts) {
                v.v4SectionsFixture = v.v4SectionsFixture && std::any_of(v.corpoPeaks.begin(), v.corpoPeaks.end(), [&](std::size_t p1) {
                                          const long diff = static_cast<long>(p1 - 1) - static_cast<long>(s);
                                          return diff >= -1 && diff <= 1;
                                      });
            }
            if (f8) {
                std::size_t withNotes = 0;
                for (const auto& p : *f8) withNotes += p.empty() ? 0 : 1;
                v.f8Classification = variogramOfMatrix(developmentDistanceMatrix(*f8, kVettoreRole, m), kVettoreRole, withNotes).classification;
            } else {
                v.f8Classification = "NOT_RUN";
            }
            r.measures.push_back(v);
        }
    };
    DevelopmentMeasuresResult r;
    once(r);
    DevelopmentMeasuresResult again;
    once(again);
    r.v5RepeatIdentical = again.measuresJson() == r.measuresJson();
    for (auto& m : r.measures) m.admitted = m.measure != "M4" && m.v1Horse && m.v2Lineage && m.v3SectionsF9 && m.v4SectionsFixture && r.v5RepeatIdentical;
    if (!r.v5RepeatIdentical) r.reasons.emplace_back("V5: repeat differs");
    r.status = r.reasons.empty() && r.measures.size() == 3 ? "MEASURED" : "INCONCLUSIVE";
    if (writeFiles) {
        r.manifestFilePath = (fs::path(outputDir) / "development_measures_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(r.manifestFilePath);
        r.manifestWriteSuccess = mf.is_open();
        if (r.manifestWriteSuccess) {
            mf << r.toJson();
            r.manifestWriteSuccess = mf.good();
        }
    }
    return r;
}

std::string DevelopmentMeasuresResult::measuresJson() const {
    json arr = json::array();
    for (const auto& m : measures) {
        auto j = validationJson(m);
        j.erase("admitted");
        arr.push_back(j);
    }
    return arr.dump(2);
}

std::string DevelopmentMeasuresResult::toJson() const {
    json arr = json::array();
    for (const auto& m : measures) arr.push_back(validationJson(m));
    json j;
    j["scenario"] = "development_measures_v1";
    j["status"] = status;
    j["reasons"] = reasons;
    j["preregistration"] = "docs/research/M2_5_Pkg6_Development_Measures_Preregistration.md";
    j["truth"] = "F9 intent note: derivations 3<-1, 5<-1, 13<-1, 15<-1, 21<-1, 22<-2; section starts 7, 11, 15, 19, 21 (1-based)";
    j["measures"] = arr;
    j["V5_repeat_identical"] = v5RepeatIdentical;
    j["admission_rule"] = "V1-V5 all passed; M4 is the reference and is never admitted here";
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
