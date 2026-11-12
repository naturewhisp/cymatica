#include "ensemble_tools.h"

#include "sha256.h"
#include "stable_random.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <tuple>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::size_t kVettore = 3;
constexpr std::size_t kTrama = 2;
constexpr std::uint32_t kPhraseSteps = kEnsembleStepsPerBar * kEnsembleBarsPerPhrase;

// Deterministic counter-based stream (stable mixer, ADR-0002); independent of every runtime stream
struct Stream {
    std::uint64_t seed;
    std::uint64_t tag;
    std::uint64_t counter{0};
    std::uint64_t next() noexcept { return core::stableMix3(seed, tag, counter++); }
    std::uint64_t below(std::uint64_t n) noexcept { return n == 0 ? 0 : next() % n; }
    std::int64_t between(std::int64_t lo, std::int64_t hi) noexcept {
        return lo + static_cast<std::int64_t>(below(static_cast<std::uint64_t>(hi - lo + 1)));
    }
};

void sortPhrase(PhraseTokens& p) {
    std::sort(p.begin(), p.end(), [](const NoteToken& a, const NoteToken& b) {
        return std::tie(a.role, a.onset, a.midi, a.durationClass) < std::tie(b.role, b.onset, b.midi, b.durationClass);
    });
}

// D Dorian degree to MIDI (62 = D4), degrees may be negative
int dorianMidi(std::int64_t degree) {
    static constexpr std::array<int, 7> kSteps{0, 2, 3, 5, 7, 9, 10};
    const std::int64_t octave = degree >= 0 ? degree / 7 : -((-degree + 6) / 7);
    const auto step = static_cast<std::size_t>(degree - octave * 7);
    return 62 + static_cast<int>(12 * octave) + kSteps[step];
}

struct SyntheticNote {
    std::int64_t onset;
    std::int64_t degree;
    std::uint32_t duration;
};

PhraseTokens toTokens(const std::vector<SyntheticNote>& notes) {
    PhraseTokens p;
    for (const auto& n : notes) p.push_back({kVettore, n.onset, dorianMidi(n.degree), n.duration});
    sortPhrase(p);
    return p;
}

std::vector<SyntheticNote> baseMotif() {
    return {{0, 0, 4}, {4, 2, 4}, {8, 4, 4}, {12, 3, 4}, {16, 4, 8}, {24, 2, 8}, {32, 1, 8}, {40, 0, 8}};
}

bool onsetUsed(const std::vector<SyntheticNote>& notes, std::int64_t onset) {
    return std::any_of(notes.begin(), notes.end(), [&](const SyntheticNote& n) { return n.onset == onset; });
}

// One elementary development operation on a copy of the previous stage (pre-registration §2, F3)
std::vector<SyntheticNote> developOnce(std::vector<SyntheticNote> notes, Stream& rng) {
    std::sort(notes.begin(), notes.end(), [](const SyntheticNote& a, const SyntheticNote& b) { return a.onset < b.onset; });
    const auto op = rng.below(5);
    const auto pick = static_cast<std::size_t>(rng.below(notes.size()));
    const std::int64_t sign = rng.below(2) == 0 ? -1 : 1;
    if (op == 0) {
        notes[pick].degree += sign;
    } else if (op == 1) {
        const std::int64_t target = notes[pick].onset + sign;
        const std::int64_t lo = pick == 0 ? -1 : notes[pick - 1].onset;
        const std::int64_t hi = pick + 1 == notes.size() ? static_cast<std::int64_t>(kPhraseSteps) : notes[pick + 1].onset;
        if (target > lo && target < hi) notes[pick].onset = target;
        else notes[pick].degree += sign; // the shift would cross a neighbour: pitch step instead
    } else if (op == 2) {
        const std::int64_t d = static_cast<std::int64_t>(notes[pick].duration) + sign;
        notes[pick].duration = static_cast<std::uint32_t>(std::clamp<std::int64_t>(d, 1, 16));
    } else if (op == 3 || notes.size() <= 4) {
        for (std::uint64_t attempt = 0; attempt < kPhraseSteps; ++attempt) {
            const auto onset = static_cast<std::int64_t>(rng.below(kPhraseSteps));
            if (onsetUsed(notes, onset)) continue;
            notes.push_back({onset, rng.between(0, 7), 2});
            break;
        }
    } else {
        notes.erase(notes.begin() + static_cast<std::ptrdiff_t>(pick));
    }
    return notes;
}

double quantileNearestRank(const std::vector<double>& sorted, double q) {
    if (sorted.empty()) return 0.0;
    const auto rank = static_cast<std::size_t>(std::ceil(q * static_cast<double>(sorted.size())));
    return sorted[(std::min)(sorted.size() - 1, rank == 0 ? 0 : rank - 1)];
}

std::vector<double> averageRanks(const std::vector<double>& v) {
    std::vector<std::size_t> idx(v.size());
    std::iota(idx.begin(), idx.end(), 0);
    std::sort(idx.begin(), idx.end(), [&](std::size_t a, std::size_t b) { return v[a] < v[b]; });
    std::vector<double> ranks(v.size());
    for (std::size_t i = 0; i < idx.size();) {
        std::size_t j = i;
        while (j + 1 < idx.size() && v[idx[j + 1]] == v[idx[i]]) ++j;
        const double r = 0.5 * static_cast<double>(i + j) + 1.0;
        for (std::size_t t = i; t <= j; ++t) ranks[idx[t]] = r;
        i = j + 1;
    }
    return ranks;
}

json variogramJson(const RoleVariogram& v) {
    json pts = json::array();
    for (const auto& p : v.points) {
        pts.push_back({{"lag", p.lag}, {"pairs", p.pairs}, {"observed", p.observed}, {"null_mean", p.nullMean},
                       {"null_q025", p.nullLow}, {"null_q975", p.nullHigh}, {"p_lower", p.pLower}});
    }
    return {{"classification", v.classification}, {"degenerate", v.degenerate}, {"phrases_with_notes", v.phrasesWithNotes},
            {"spearman_rho_k_1_to_half", v.spearmanRho ? json(*v.spearmanRho) : json(nullptr)}, {"points", pts}};
}

} // namespace

std::vector<PhraseTokens> fixturePhraseTokens(const FixturePlan& plan) {
    std::map<std::pair<std::uint64_t, int>, std::uint64_t> release;
    for (const auto& o : plan.noteOffs) release[{o.eventId, static_cast<int>(o.role)}] = o.sampleFrame;
    std::vector<PhraseTokens> out;
    for (std::size_t i = 0; i < plan.phrases.size() && i < plan.intentsPerPhrase.size(); ++i) {
        const auto& p = plan.phrases[i];
        PhraseTokens tokens;
        if (p.barCount > 0 && p.endFrame > p.startFrame) {
            const double sixteenth = static_cast<double>(p.endFrame - p.startFrame) / (static_cast<double>(p.barCount) * kDiversityGridPerBar);
            for (const auto& e : plan.intentsPerPhrase[i]) {
                const auto it = release.find({e.eventId, static_cast<int>(e.role)});
                const std::uint64_t end = it != release.end() ? it->second : e.sampleFrame + e.durationFrames;
                const double dur = end > e.sampleFrame ? static_cast<double>(end - e.sampleFrame) : 0.0;
                NoteToken t;
                t.role = static_cast<std::size_t>(e.role);
                t.onset = std::llround((static_cast<double>(e.sampleFrame) - static_cast<double>(p.startFrame)) / sixteenth);
                t.midi = (e.pitchHz > 0.0f && std::isfinite(e.pitchHz))
                             ? static_cast<int>(std::lround(69.0 + 12.0 * std::log2(static_cast<double>(e.pitchHz) / 440.0)))
                             : -1;
                t.durationClass = static_cast<std::uint32_t>(
                    (std::min<long long>)(kDiversityMaxDurationClass, (std::max)(1LL, std::llround(dur / sixteenth))));
                tokens.push_back(t);
            }
        }
        sortPhrase(tokens);
        out.push_back(std::move(tokens));
    }
    return out;
}

std::vector<std::vector<double>> roleDistanceMatrix(const std::vector<PhraseTokens>& phrases, std::size_t role) {
    const std::size_t n = phrases.size();
    std::vector<std::vector<double>> d(n, std::vector<double>(n, 0.0));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) d[i][j] = d[j][i] = m4EditDistance(phrases[i], phrases[j], role);
    }
    return d;
}

std::optional<double> spearmanRho(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.size() < 3) return std::nullopt;
    const auto rx = averageRanks(x), ry = averageRanks(y);
    const double n = static_cast<double>(x.size());
    const double mx = std::accumulate(rx.begin(), rx.end(), 0.0) / n, my = std::accumulate(ry.begin(), ry.end(), 0.0) / n;
    double sxy = 0.0, sxx = 0.0, syy = 0.0;
    for (std::size_t i = 0; i < rx.size(); ++i) {
        sxy += (rx[i] - mx) * (ry[i] - my);
        sxx += (rx[i] - mx) * (rx[i] - mx);
        syy += (ry[i] - my) * (ry[i] - my);
    }
    if (sxx <= 0.0 || syy <= 0.0) return std::nullopt;
    return sxy / std::sqrt(sxx * syy);
}

RoleVariogram roleVariogram(const std::vector<PhraseTokens>& phrases, std::size_t role, std::size_t permutations, std::uint64_t seed) {
    RoleVariogram v;
    v.role = role;
    const std::size_t n = phrases.size();
    for (const auto& p : phrases) {
        if (std::any_of(p.begin(), p.end(), [&](const NoteToken& t) { return t.role == role; })) ++v.phrasesWithNotes;
    }
    if (n < 2 || v.phrasesWithNotes == 0 || permutations == 0) {
        v.classification = "NO_DATA";
        return v;
    }
    const auto d = roleDistanceMatrix(phrases, role);
    v.degenerate = true;
    for (std::size_t i = 0; i < n && v.degenerate; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            if (d[i][j] != 0.0) {
                v.degenerate = false;
                break;
            }
        }
    }
    const auto variogram = [&](const std::vector<std::size_t>& order, std::size_t k) {
        double sum = 0.0;
        for (std::size_t i = 0; i + k < n; ++i) sum += d[order[i]][order[i + k]];
        return sum / static_cast<double>(n - k);
    };
    std::vector<std::size_t> identity(n);
    std::iota(identity.begin(), identity.end(), 0);
    std::vector<std::vector<double>> nulls(n);
    Stream rng{seed, static_cast<std::uint64_t>(role) + 1};
    std::vector<std::size_t> order(n);
    for (std::size_t m = 0; m < permutations; ++m) {
        order = identity;
        for (std::size_t i = n - 1; i > 0; --i) std::swap(order[i], order[static_cast<std::size_t>(rng.below(i + 1))]);
        for (std::size_t k = 1; k < n; ++k) nulls[k].push_back(variogram(order, k));
    }
    std::vector<double> lags, values;
    for (std::size_t k = 1; k < n; ++k) {
        VariogramPoint p;
        p.lag = k;
        p.pairs = n - k;
        p.observed = variogram(identity, k);
        auto& nl = nulls[k];
        std::sort(nl.begin(), nl.end());
        p.nullMean = std::accumulate(nl.begin(), nl.end(), 0.0) / static_cast<double>(nl.size());
        p.nullLow = quantileNearestRank(nl, 0.025);
        p.nullHigh = quantileNearestRank(nl, 0.975);
        const auto le = static_cast<double>(std::upper_bound(nl.begin(), nl.end(), p.observed) - nl.begin());
        p.pLower = (1.0 + le) / (static_cast<double>(nl.size()) + 1.0);
        v.points.push_back(p);
        if (k <= n / 2) {
            lags.push_back(static_cast<double>(k));
            values.push_back(p.observed);
        }
    }
    v.spearmanRho = spearmanRho(lags, values);
    const auto& first = v.points.front();
    if (v.degenerate) v.classification = "DEGENERATE";
    else if (first.observed < first.nullLow) v.classification = "ORDERED";
    else if (first.observed > first.nullHigh) v.classification = "ANTI_ORDERED";
    else v.classification = "ORDER_FREE";
    return v;
}

const char* adversarialFixtureName(AdversarialFixture id) noexcept {
    switch (id) {
        case AdversarialFixture::Loop: return "literal loop";
        case AdversarialFixture::Noise: return "in-scale noise";
        case AdversarialFixture::Development: return "real development";
        case AdversarialFixture::ShuffledDevelopment: return "shuffled development";
        case AdversarialFixture::RandomTransposition: return "random transposition per phrase";
        case AdversarialFixture::RandomOrnaments: return "random ornaments";
    }
    return "?";
}

std::vector<PhraseTokens> adversarialFixture(AdversarialFixture id, std::uint64_t seed) {
    Stream rng{seed, static_cast<std::uint64_t>(id)};
    std::vector<PhraseTokens> out;
    const auto base = baseMotif();
    if (id == AdversarialFixture::Development || id == AdversarialFixture::ShuffledDevelopment) {
        Stream dev{seed, static_cast<std::uint64_t>(AdversarialFixture::Development)};
        auto stage = base;
        for (std::size_t i = 0; i < kEnsembleSyntheticPhrases; ++i) {
            if (i > 0) stage = developOnce(stage, dev);
            out.push_back(toTokens(stage));
        }
        if (id == AdversarialFixture::ShuffledDevelopment) {
            for (std::size_t i = out.size() - 1; i > 0; --i) std::swap(out[i], out[static_cast<std::size_t>(rng.below(i + 1))]);
        }
        return out;
    }
    for (std::size_t i = 0; i < kEnsembleSyntheticPhrases; ++i) {
        std::vector<SyntheticNote> notes;
        switch (id) {
            case AdversarialFixture::Loop:
                notes = base;
                break;
            case AdversarialFixture::Noise: {
                std::set<std::int64_t> onsets;
                while (onsets.size() < 8) onsets.insert(static_cast<std::int64_t>(rng.below(kPhraseSteps)));
                for (const auto o : onsets) notes.push_back({o, rng.between(0, 9), static_cast<std::uint32_t>(rng.between(1, 4))});
                break;
            }
            case AdversarialFixture::RandomTransposition: {
                const std::int64_t shift = rng.between(-5, 5);
                notes = base;
                for (auto& n : notes) n.degree += shift;
                break;
            }
            case AdversarialFixture::RandomOrnaments: {
                notes = base;
                for (int added = 0; added < 2;) {
                    const auto onset = static_cast<std::int64_t>(rng.below(kPhraseSteps));
                    if (onsetUsed(notes, onset)) continue;
                    std::int64_t previous = notes.front().degree;
                    for (const auto& n : notes) {
                        if (n.onset < onset) previous = n.degree;
                    }
                    notes.push_back({onset, previous + (rng.below(2) == 0 ? -1 : 1), 1});
                    ++added;
                }
                break;
            }
            default:
                break;
        }
        out.push_back(toTokens(notes));
    }
    return out;
}

std::optional<std::vector<PhraseTokens>> loadPositiveControl(const std::string& path, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "file not found: " + path + " (the positive control must be written by a person)";
        return std::nullopt;
    }
    try {
        const json j = json::parse(in);
        std::vector<PhraseTokens> out;
        for (const auto& phrase : j.at("phrases")) {
            PhraseTokens p;
            for (const auto& note : phrase) {
                const auto onset = note.at("onset").get<std::int64_t>();
                const auto midi = note.at("midi").get<int>();
                const auto duration = note.at("duration").get<std::int64_t>();
                if (onset < 0 || onset >= kPhraseSteps || midi < 0 || midi > 127 || duration < 1) {
                    error = "note out of range";
                    return std::nullopt;
                }
                p.push_back({kVettore, onset, midi, static_cast<std::uint32_t>((std::min<std::int64_t>)(duration, kDiversityMaxDurationClass))});
            }
            sortPhrase(p);
            out.push_back(std::move(p));
        }
        if (out.size() < 4) {
            error = "fewer than 4 phrases";
            return std::nullopt;
        }
        return out;
    } catch (const std::exception& e) {
        error = std::string("invalid JSON: ") + e.what();
        return std::nullopt;
    }
}

std::vector<SectionVertical> fixtureVerticalStats(const FixturePlan& plan) {
    std::vector<SectionVertical> out;
    const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();
    if (!clock) return out;
    const auto phrasePlans = audio::MusicPlanner::makeFixtureM24V1Phrases(*clock);
    const auto tokens = fixturePhraseTokens(plan);
    std::map<std::uint32_t, SectionVertical> bySection;
    for (std::size_t i = 0; i < tokens.size() && i < phrasePlans.size(); ++i) {
        const auto& pp = phrasePlans[i];
        SectionVertical& sv = bySection[pp.sectionIndex];
        sv.section = pp.sectionIndex;
        std::map<std::int64_t, std::set<std::size_t>> rolesAt;
        for (const auto& t : tokens[i]) {
            if (t.role >= kRoleCount) continue;
            ++sv.onsets[t.role];
            rolesAt[t.onset].insert(t.role);
            if (t.role != kTrama && t.role != kVettore) continue;
            const std::size_t slot = t.role == kTrama ? 0 : 1;
            const auto bar = static_cast<std::size_t>(std::clamp<std::int64_t>(t.onset / kEnsembleStepsPerBar, 0, 3));
            const audio::ChordSpec& chord = pp.chords[bar % 4];
            bool chordTone = false;
            for (const auto degree : {chord.rootDegree, chord.thirdDegree, chord.fifthDegree}) {
                const int pc = ((audio::degreeToMidiNote(pp.scale, degree) % 12) + 12) % 12;
                if (t.midi >= 0 && t.midi % 12 == pc) chordTone = true;
            }
            if (t.onset % kEnsembleStepsPerBar == 0) {
                ++sv.downbeatOnsets[slot];
                sv.downbeatChordTones[slot] += chordTone ? 1 : 0;
            } else {
                ++sv.otherOnsets[slot];
                sv.otherChordTones[slot] += chordTone ? 1 : 0;
            }
        }
        for (const auto& [onset, roles] : rolesAt) {
            for (auto a = roles.begin(); a != roles.end(); ++a) {
                for (auto b = std::next(a); b != roles.end(); ++b) ++sv.coincidences[*a][*b];
            }
        }
    }
    for (auto& [section, sv] : bySection) out.push_back(sv);
    return out;
}

std::vector<DesignCost> designCostEstimates() {
    constexpr std::uint64_t C = 8, R = 4, B = kEnsembleBarsPerPhrase, S = kEnsembleStepsPerBar, L = 3, K = 6, KG = 2;
    return {
        {"A (current): shared plan, independent roles", "C x B x S, C = 8, B = 4, S = 16", C * B * S, false},
        {"B: sequential with reading, fixed order", "C x sum over roles of (B x S), R = 4", C * R * B * S, false},
        {"D: per-role candidates, reduced beam", "L x K x R per step, x B x S; L = 3, K = 6", L * K * R * B * S, true},
        {"G: revision in K fixed passes", "K x R x C per step, x B x S; K = 2", KG * R * C * B * S, true},
    };
}

EnsembleToolsResult runEnsembleToolsV1(const std::string& outputDir, bool writeFiles) {
    EnsembleToolsResult r;
    // Pre-registration integrity: hash of the file with LF line endings
    {
        std::ifstream in(kEnsemblePreregistration, std::ios::binary);
        if (in) {
            std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
            r.preregistrationSha256 = sha256Hex(std::string_view(text));
            r.preregistrationMatches = r.preregistrationSha256 == kEnsemblePreregistrationSha256;
        }
    }
    // Adversarial fixtures and their pre-registered classification
    const std::array<std::pair<AdversarialFixture, const char*>, 6> expected{{
        {AdversarialFixture::Loop, "DEGENERATE"},
        {AdversarialFixture::Noise, "ORDER_FREE"},
        {AdversarialFixture::Development, "ORDERED"},
        {AdversarialFixture::ShuffledDevelopment, "ORDER_FREE"},
        {AdversarialFixture::RandomTransposition, "DEGENERATE"},
        {AdversarialFixture::RandomOrnaments, "ORDER_FREE"},
    }};
    for (const auto& [id, cls] : expected) {
        EnsembleToolsResult::FixtureOutcome f;
        f.id = "F" + std::to_string(static_cast<int>(id));
        f.name = adversarialFixtureName(id);
        f.expectedClassification = cls;
        f.vettore = roleVariogram(adversarialFixture(id), kVettore);
        f.status = "MEASURED";
        if (f.vettore.classification != cls) r.missedPredictions.push_back(f.id + ": expected " + cls + ", got " + f.vettore.classification);
        if (id == AdversarialFixture::Development && !(f.vettore.spearmanRho && *f.vettore.spearmanRho > 0.0)) {
            r.missedPredictions.push_back("F3: expected rho > 0");
        }
        r.adversarial.push_back(f);
    }
    {
        EnsembleToolsResult::FixtureOutcome f;
        f.id = "F7";
        f.name = "positive control written by a person";
        std::string error;
        if (const auto control = loadPositiveControl(kEnsemblePositiveControlPath, error)) {
            f.vettore = roleVariogram(*control, kVettore);
            f.status = "MEASURED";
        } else {
            f.reason = error;
        }
        r.adversarial.push_back(f);
    }
    const auto cls = [&](std::size_t i) { return r.adversarial[i].vettore.classification; };
    r.horseCriterionMet = cls(0) == "DEGENERATE" && cls(1) == "ORDER_FREE" && cls(2) == "ORDERED" && cls(3) == "ORDER_FREE" &&
                          r.adversarial[2].vettore.spearmanRho && *r.adversarial[2].vettore.spearmanRho > 0.0;
    // Real fixture, seed 1337 (descriptive)
    const FixturePlan plan = planFixtureM24V1(1337);
    if (!plan.ok) {
        r.status = "INCONCLUSIVE";
        r.error = plan.error;
    } else {
        const auto tokens = fixturePhraseTokens(plan);
        for (std::size_t role = 0; role < kRoleCount; ++role) r.fixtureRoles[role] = roleVariogram(tokens, role);
        r.vertical = fixtureVerticalStats(plan);
        r.status = "MEASURED";
    }
    r.costs = designCostEstimates();
    if (writeFiles) {
        namespace fs = std::filesystem;
        std::error_code ec;
        fs::create_directories(outputDir, ec);
        r.manifestFilePath = (fs::path(outputDir) / "ensemble_tools_v1_manifest.json").lexically_normal().string();
        std::ofstream out(r.manifestFilePath, std::ios::binary);
        out << r.toJson();
        r.manifestWriteSuccess = out.good();
    }
    return r;
}

std::string EnsembleToolsResult::toJson() const {
    json j;
    j["manifest_version"] = "1.0";
    j["scenario"] = "ensemble_tools_v1";
    j["build_id"] = CYMATICA_BUILD_ID;
    j["status"] = status;
    if (!error.empty()) j["error"] = error;
    j["gate"] = false;
    j["scope"] = "offline symbolic measures of R-M25-7 (M2.5.1c): never a gate, never part of the planner scoring";
    j["preregistration"] = {{"path", kEnsemblePreregistration}, {"expected_sha256", kEnsemblePreregistrationSha256},
                            {"file_sha256_lf", preregistrationSha256.empty() ? json(nullptr) : json(preregistrationSha256)},
                            {"matches", preregistrationMatches}};
    j["method"] = {{"distance", "M4: normalised Levenshtein on (onset delta, interval, duration class) per role; transposition invariant"},
                   {"null", "phrase order permutations, Fisher-Yates on a stable counter stream"},
                   {"permutations", kEnsemblePermutations}, {"null_seed", kEnsembleNullSeed}, {"fixture_seed", kEnsembleFixtureSeed},
                   {"classification", "on lag 1 against the 2.5 and 97.5 % nearest-rank quantiles of the null"}};
    json adv = json::array();
    for (const auto& f : adversarial) {
        json e = {{"id", f.id}, {"name", f.name}, {"status", f.status}};
        if (!f.reason.empty()) e["reason"] = f.reason;
        if (!f.expectedClassification.empty()) e["expected_classification"] = f.expectedClassification;
        if (f.status == "MEASURED") e["vettore"] = variogramJson(f.vettore);
        adv.push_back(e);
    }
    j["adversarial_fixtures"] = adv;
    j["horse_criterion"] = {{"rule", "F1 DEGENERATE, F2 and F4 ORDER_FREE, F3 ORDERED with rho > 0 (pre-registration §3)"},
                            {"met", horseCriterionMet}, {"missed_predictions", missedPredictions}};
    json roles = json::object();
    for (std::size_t i = 0; i < kRoleCount; ++i) roles[kRoleNames[i]] = variogramJson(fixtureRoles[i]);
    j["fixture_m2_4_v1_seed_1337"] = {{"note", "descriptive, no prediction (pre-registration §4)"}, {"roles", roles}};
    json vert = json::array();
    for (const auto& s : vertical) {
        json coinc = json::object();
        for (std::size_t a = 0; a < kRoleCount; ++a) {
            for (std::size_t b = a + 1; b < kRoleCount; ++b) coinc[std::string(kRoleNames[a]) + "_" + kRoleNames[b]] = s.coincidences[a][b];
        }
        json onsetsJ = json::object();
        for (std::size_t a = 0; a < kRoleCount; ++a) onsetsJ[kRoleNames[a]] = s.onsets[a];
        json chordTones = json::object();
        for (std::size_t k = 0; k < 2; ++k) {
            chordTones[k == 0 ? "trama" : "vettore"] = {{"downbeat_onsets", s.downbeatOnsets[k]}, {"downbeat_chord_tones", s.downbeatChordTones[k]},
                                                        {"other_onsets", s.otherOnsets[k]}, {"other_chord_tones", s.otherChordTones[k]}};
        }
        vert.push_back({{"section", s.section}, {"onsets", onsetsJ}, {"onset_coincidences_same_sixteenth", coinc},
                        {"chord_tone_onsets", chordTones}});
    }
    j["vertical_statistics"] = {{"grid", "sixteenths of the phrase bar"}, {"sections", vert}};
    json costList = json::array();
    for (const auto& c : this->costs) {
        costList.push_back({{"design", c.design}, {"formula", c.formula}, {"step_evaluations_per_phrase", c.stepEvaluations},
                         {"hypothetical_parameters", c.hypotheticalParameters}});
    }
    j["design_cost_estimates"] = {{"unit", "step evaluations per phrase, logical work, not time"}, {"designs", costList}};
    j["manifest_file"] = manifestFilePath;
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
