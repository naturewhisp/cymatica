// Offline ensemble and development tool (M2.5.1c, R-M25-7). The tests check the mechanics with hand-computed
// cases; the pre-registered predictions on the adversarial fixtures are evaluated by the scenario, not forced here.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ensemble_tools.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <tuple>

using namespace cymatica::cacp;
using Catch::Matchers::WithinAbs;

namespace {

PhraseTokens phraseOf(std::initializer_list<int> midis) {
    PhraseTokens p;
    std::int64_t onset = 0;
    for (const int m : midis) {
        p.push_back({3, onset, m, 4});
        onset += 4;
    }
    return p;
}

bool sameTokens(const NoteToken& a, const NoteToken& b) {
    return std::tie(a.role, a.onset, a.midi, a.durationClass) == std::tie(b.role, b.onset, b.midi, b.durationClass);
}

bool samePhrases(const std::vector<PhraseTokens>& x, const std::vector<PhraseTokens>& y) {
    if (x.size() != y.size()) return false;
    for (std::size_t i = 0; i < x.size(); ++i) {
        if (!std::equal(x[i].begin(), x[i].end(), y[i].begin(), y[i].end(), sameTokens)) return false;
    }
    return true;
}

} // namespace

TEST_CASE("Ensemble tools: Spearman rho with ties and degenerate inputs", "[ensemble_tools]") {
    CHECK_THAT(*spearmanRho({1, 2, 3, 4}, {10, 20, 30, 40}), WithinAbs(1.0, 1e-12));
    CHECK_THAT(*spearmanRho({1, 2, 3, 4}, {4, 3, 2, 1}), WithinAbs(-1.0, 1e-12));
    // Ties take the average rank: y ranks 1.5, 1.5, 3, 4 against 1, 2, 3, 4 -> rho = 0.9486833
    CHECK_THAT(*spearmanRho({1, 2, 3, 4}, {5, 5, 6, 7}), WithinAbs(0.9486832980505138, 1e-12));
    CHECK_FALSE(spearmanRho({1, 2, 3}, {5, 5, 5}).has_value()); // constant series
    CHECK_FALSE(spearmanRho({1, 2}, {1, 2}).has_value());       // fewer than 3 points
}

TEST_CASE("Ensemble tools: variogram observed values and null bookkeeping", "[ensemble_tools]") {
    // Phrases A A B B: lag 1 pairs (A,A) (A,B) (B,B) -> distances 0, d, 0; lag 2 pairs (A,B) (A,B) -> d, d
    const PhraseTokens a = phraseOf({60, 62, 64, 65});
    const PhraseTokens b = phraseOf({60, 59, 57, 55});
    const std::vector<PhraseTokens> phrases{a, a, b, b};
    const double d = m4EditDistance(a, b, 3);
    REQUIRE(d > 0.0);
    const RoleVariogram v = roleVariogram(phrases, 3, 200, 7);
    REQUIRE(v.points.size() == 3);
    CHECK(v.points[0].lag == 1);
    CHECK(v.points[0].pairs == 3);
    CHECK_THAT(v.points[0].observed, WithinAbs(d / 3.0, 1e-12));
    CHECK_THAT(v.points[1].observed, WithinAbs(d, 1e-12));
    CHECK(v.points[0].nullLow <= v.points[0].nullMean);
    CHECK(v.points[0].nullMean <= v.points[0].nullHigh);
    CHECK(v.points[0].pLower > 0.0);
    CHECK(v.points[0].pLower <= 1.0);
    CHECK_FALSE(v.degenerate);
    // Deterministic for a given seed
    const RoleVariogram again = roleVariogram(phrases, 3, 200, 7);
    CHECK(again.points[0].nullMean == v.points[0].nullMean);
    CHECK(again.points[0].pLower == v.points[0].pLower);
}

TEST_CASE("Ensemble tools: degenerate and empty roles", "[ensemble_tools]") {
    const PhraseTokens a = phraseOf({60, 62, 64});
    const RoleVariogram same = roleVariogram({a, a, a, a}, 3, 50, 1);
    CHECK(same.degenerate);
    CHECK(same.classification == "DEGENERATE");
    CHECK(roleVariogram({a, a, a, a}, 0, 50, 1).classification == "NO_DATA"); // nothing in role 0
    CHECK(roleVariogram({a}, 3, 50, 1).classification == "NO_DATA");          // a single phrase
}

TEST_CASE("Ensemble tools: an ordered sequence is ORDERED, its permutation is not", "[ensemble_tools]") {
    // 24 phrases of 24 notes; phrase i shortens the first i notes. The edit distance between i and j is |i - j| / 24,
    // so neighbours are close and far phrases are far.
    std::vector<PhraseTokens> drift;
    for (std::uint32_t i = 0; i < 24; ++i) {
        PhraseTokens p;
        for (std::uint32_t k = 0; k < 24; ++k) p.push_back({3, static_cast<std::int64_t>(4 * k), 60, k < i ? 2U : 4U});
        drift.push_back(p);
    }
    CHECK_THAT(m4EditDistance(drift[3], drift[7], 3), WithinAbs(4.0 / 24.0, 1e-12));
    const RoleVariogram v = roleVariogram(drift, 3, 500, 3);
    CHECK(v.classification == "ORDERED");
    REQUIRE(v.spearmanRho.has_value());
    CHECK(*v.spearmanRho > 0.0);
    // The same phrases in a shuffled order are not ORDERED
    std::vector<PhraseTokens> shuffled = drift;
    for (std::size_t i = 0; i < shuffled.size(); ++i) std::swap(shuffled[i], shuffled[(i * 7 + 5) % shuffled.size()]);
    CHECK(roleVariogram(shuffled, 3, 500, 3).classification != "ORDERED");
}

TEST_CASE("Ensemble tools: M4 is categorical on intervals (property found while testing, M2.5.1c)", "[ensemble_tools]") {
    // A gradual pitch drift: every pair of distinct phrases is at the same distance, however far the drift.
    // The magnitude of an interval change is not graded, so M4 cannot see a gradual development of pitch.
    const PhraseTokens p1 = phraseOf({60, 61, 60, 62});
    const PhraseTokens p2 = phraseOf({60, 62, 60, 64});
    const PhraseTokens p9 = phraseOf({60, 69, 60, 78});
    CHECK(m4EditDistance(p1, p2, 3) == m4EditDistance(p1, p9, 3));
}

TEST_CASE("Ensemble tools: adversarial fixtures are deterministic and well formed", "[ensemble_tools]") {
    for (const auto id : {AdversarialFixture::Loop, AdversarialFixture::Noise, AdversarialFixture::Development,
                          AdversarialFixture::ShuffledDevelopment, AdversarialFixture::RandomTransposition,
                          AdversarialFixture::RandomOrnaments}) {
        const auto f = adversarialFixture(id);
        REQUIRE(f.size() == kEnsembleSyntheticPhrases);
        CHECK(samePhrases(adversarialFixture(id), f)); // same seed, same content
        for (const auto& p : f) {
            REQUIRE_FALSE(p.empty());
            for (const auto& t : p) {
                CHECK(t.role == 3);
                CHECK(t.onset >= 0);
                CHECK(t.onset < 64);
                CHECK(t.durationClass >= 1);
            }
        }
    }
    // The shuffled development holds the same phrases as the development, in another order
    auto dev = adversarialFixture(AdversarialFixture::Development);
    auto shuffled = adversarialFixture(AdversarialFixture::ShuffledDevelopment);
    CHECK_FALSE(samePhrases(dev, shuffled));
    const auto byContent = [](const PhraseTokens& x, const PhraseTokens& y) {
        return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end(), [](const NoteToken& a, const NoteToken& b) {
            return std::tie(a.onset, a.midi, a.durationClass) < std::tie(b.onset, b.midi, b.durationClass);
        });
    };
    std::sort(dev.begin(), dev.end(), byContent);
    std::sort(shuffled.begin(), shuffled.end(), byContent);
    CHECK(samePhrases(dev, shuffled));
}

TEST_CASE("Ensemble tools: the positive control is read only from a person's file", "[ensemble_tools]") {
    std::string error;
    CHECK_FALSE(loadPositiveControl("artifacts/no_such_dir/positive_control.json", error).has_value());
    CHECK(error.find("written by a person") != std::string::npos);

    namespace fs = std::filesystem;
    const fs::path dir = fs::path("artifacts") / "ensemble_tools_test";
    fs::create_directories(dir);
    const fs::path file = dir / "control.json";
    {
        std::ofstream out(file);
        out << R"({"phrases": [[{"onset": 0, "midi": 62, "duration": 4}], [{"onset": 4, "midi": 64, "duration": 2}],
                  [{"onset": 0, "midi": 65, "duration": 4}], [{"onset": 8, "midi": 67, "duration": 1}]]})";
    }
    const auto ok = loadPositiveControl(file.string(), error);
    REQUIRE(ok.has_value());
    CHECK(ok->size() == 4);
    CHECK((*ok)[1][0].onset == 4);
    {
        std::ofstream out(file);
        out << R"({"phrases": [[{"onset": 64, "midi": 62, "duration": 4}], [], [], []]})";
    }
    CHECK_FALSE(loadPositiveControl(file.string(), error).has_value()); // onset outside the phrase
    {
        std::ofstream out(file);
        out << "not json";
    }
    CHECK_FALSE(loadPositiveControl(file.string(), error).has_value());
}

TEST_CASE("Ensemble tools: cost estimates follow the declared formulas", "[ensemble_tools]") {
    const auto c = designCostEstimates();
    REQUIRE(c.size() == 4);
    CHECK(c[0].stepEvaluations == 8 * 4 * 16);         // A
    CHECK(c[1].stepEvaluations == 8 * 4 * 4 * 16);     // B
    CHECK(c[2].stepEvaluations == 3 * 6 * 4 * 4 * 16); // D
    CHECK(c[3].stepEvaluations == 2 * 4 * 8 * 4 * 16); // G
    CHECK_FALSE(c[0].hypotheticalParameters);
    CHECK(c[2].hypotheticalParameters);
    CHECK(c[3].hypotheticalParameters);
}

TEST_CASE("Ensemble tools: scenario outcome on the pre-registered fixtures (M2.5.1c)", "[ensemble_tools]") {
    const std::string prereg = std::string(CYMATICA_SOURCE_DIR) + "/" + kEnsemblePreregistration;
    // Controls absent on purpose: this case pins the fixtures F1-F6 only, wherever the tests run
    const EnsembleToolsResult r = runEnsembleToolsV1("artifacts/ensemble_tools_test", false, prereg, "no/such/f7.json", "no/such/controls");
    CHECK(r.status == "MEASURED");
    CHECK(r.preregistrationMatches);
    // Horse criterion of the pre-registration §3: met on the first run
    CHECK(r.horseCriterionMet);
    // F5 was predicted DEGENERATE (transposition invariance) and is not: the transposition is diatonic, so the
    // intervals in semitones change. The missed prediction is reported, never hidden by changing the fixture.
    REQUIRE(r.missedPredictions.size() == 1);
    CHECK(r.missedPredictions[0] == "F5: expected DEGENERATE, got ORDER_FREE");
    // F7 stays NOT_RUN until a person provides the positive control
    REQUIRE(r.adversarial.size() == 9);
    CHECK(r.adversarial[6].id == "F7");
    CHECK(r.adversarial[6].status == "NOT_RUN");
}

TEST_CASE("Ensemble tools: a missing or changed pre-registration makes the run INCONCLUSIVE (DIF-M2.5.1c-STEP4-01)", "[ensemble_tools]") {
    namespace fs = std::filesystem;
    const EnsembleToolsResult missing = runEnsembleToolsV1("artifacts/ensemble_tools_test", false, "no/such/preregistration.md");
    CHECK(missing.status == "INCONCLUSIVE");
    CHECK_FALSE(missing.preregistrationMatches);
    CHECK(missing.error.find("not readable") != std::string::npos);

    // A copy of the frozen file with one character changed: different hash
    const std::string source = std::string(CYMATICA_SOURCE_DIR) + "/" + kEnsemblePreregistration;
    std::ifstream in(source, std::ios::binary);
    REQUIRE(in.good());
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    REQUIRE_FALSE(text.empty());
    text[text.size() / 2] = text[text.size() / 2] == 'x' ? 'y' : 'x';
    fs::create_directories("artifacts/ensemble_tools_test");
    const std::string changed = "artifacts/ensemble_tools_test/changed_preregistration.md";
    {
        std::ofstream out(changed, std::ios::binary);
        out << text;
    }
    const EnsembleToolsResult differs = runEnsembleToolsV1("artifacts/ensemble_tools_test", false, changed);
    CHECK(differs.status == "INCONCLUSIVE");
    CHECK_FALSE(differs.preregistrationMatches);
    CHECK(differs.error.find("differs") != std::string::npos);
    // The measures are still computed for diagnosis
    CHECK(differs.adversarial.size() == 9);
}

TEST_CASE("Ensemble tools: a positive control without notes is rejected (DIF-M2.5.1c-STEP4-02)", "[ensemble_tools]") {
    namespace fs = std::filesystem;
    fs::create_directories("artifacts/ensemble_tools_test");
    const std::string file = "artifacts/ensemble_tools_test/empty_control.json";
    {
        std::ofstream out(file);
        out << R"({"phrases": [[], [], [], []]})";
    }
    std::string error;
    CHECK_FALSE(loadPositiveControl(file, error).has_value());
    CHECK(error.find("fewer than 2 phrases with notes") != std::string::npos);
    {
        std::ofstream out(file);
        out << R"({"phrases": [[{"onset": 0, "midi": 62, "duration": 4}], [], [], []]})";
    }
    CHECK_FALSE(loadPositiveControl(file, error).has_value()); // one phrase with notes is still not a distance

    // Through the scenario: F7 is never MEASURED on such a file
    const std::string prereg = std::string(CYMATICA_SOURCE_DIR) + "/" + kEnsemblePreregistration;
    const EnsembleToolsResult r = runEnsembleToolsV1("artifacts/ensemble_tools_test", false, prereg, file);
    CHECK(r.adversarial[6].id == "F7");
    CHECK(r.adversarial[6].status == "NOT_RUN");
}

TEST_CASE("Ensemble tools: human and model development is not seen by M4 (F8, F9, M2.5.1c)", "[ensemble_tools]") {
    const std::string source = CYMATICA_SOURCE_DIR;
    const EnsembleToolsResult r = runEnsembleToolsV1("artifacts/ensemble_tools_test", false, source + "/" + kEnsemblePreregistration,
                                                     "no/such/f7.json", source + "/" + kEnsembleControlsDir);
    REQUIRE(r.adversarial.size() == 9);
    const auto& f8 = r.adversarial[7];
    const auto& f9 = r.adversarial[8];
    CHECK(f8.id == "F8");
    CHECK(f9.id == "F9");
    CHECK(f8.human);
    CHECK_FALSE(f9.human);
    CHECK(f8.status == "MEASURED");
    CHECK(f9.status == "MEASURED");
    CHECK(f8.vettore.phrasesWithNotes == 17);
    CHECK(f9.vettore.phrasesWithNotes == 24);
    // First measurement (2026-11-12): both predictions of the addendum F8/F9 (ORDERED) are missed. M4 saturates on
    // real phrases (distances 0.8-0.95 at every lag), so it does not see a real development. Reported, not tuned.
    CHECK(f8.vettore.classification == "ORDER_FREE");
    CHECK(f9.vettore.classification == "ORDER_FREE");
    CHECK(r.missedPredictions.size() == 3);
    // The horse criterion on F1-F4 is unchanged
    CHECK(r.horseCriterionMet);
}
