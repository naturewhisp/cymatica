#include "diversity.h"

#include "audio_scheduler.h"
#include "plan_content_fingerprint.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <tuple>
#include <unordered_map>

namespace cymatica::cacp {

using nlohmann::json;

namespace {

constexpr std::uint32_t kFs = 48000;

std::string hex16(std::uint64_t v) {
    std::ostringstream s;
    s << std::hex << std::setw(16) << std::setfill('0') << v;
    return s.str();
}

json summary(std::vector<double> v) {
    if (v.empty()) return json{{"n", 0}, {"mean", nullptr}, {"median", nullptr}, {"min", nullptr}, {"max", nullptr}};
    std::sort(v.begin(), v.end());
    double sum = 0.0;
    for (const double x : v) sum += x;
    const double median = v.size() % 2 ? v[v.size() / 2] : 0.5 * (v[v.size() / 2 - 1] + v[v.size() / 2]);
    return json{{"n", v.size()}, {"mean", sum / static_cast<double>(v.size())}, {"median", median}, {"min", v.front()}, {"max", v.back()}};
}

} // namespace

FixturePlan planFixtureM24V1(std::uint64_t seed) {
    FixturePlan plan;
    const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();
    if (!clock) {
        plan.error = "fixture clock unavailable";
        return plan;
    }
    const auto phrases = audio::MusicPlanner::makeFixtureM24V1Phrases(*clock);
    audio::MusicPlanner planner({.runSeed = seed, .candidateCount = 8, .sampleRate = kFs,
                                 .lookaheadFrames = audio::AudioSchedulerConfig{}.lookaheadFrames});
    planner.setFormPlan(audio::MusicPlanner::makeFixtureM24V1Form());
    for (const auto& p : phrases) {
        std::vector<audio::MusicIntentEvent> out;
        planner.planPhrase(p, *clock, 0, p.sectionIndex + 1ULL, out, nullptr); // epoch excluded from content
        plan.intentsPerPhrase.push_back(std::move(out));
        plan.phrases.push_back({static_cast<std::uint32_t>(plan.phrases.size()), p.sectionIndex, p.startBar, p.barCount,
                                p.startFrame, p.endFrame});
    }
    plan.noteOffs = planner.plannedNoteOffs();
    plan.ok = true;
    return plan;
}

std::optional<std::uint64_t> phraseContentFingerprint(const std::vector<audio::MusicIntentEvent>& phraseIntents,
                                                      const std::vector<audio::PlannedNoteOff>& noteOffs, std::uint64_t phraseStartFrame,
                                                      std::uint32_t sampleRate, std::optional<audio::MusicRole> role) {
    std::set<std::pair<std::uint64_t, int>> own;
    for (const auto& e : phraseIntents) own.insert({e.eventId, static_cast<int>(e.role)});
    std::vector<audio::PlannedNoteOff> bound;
    for (const auto& o : noteOffs) {
        if (own.contains({o.eventId, static_cast<int>(o.role)})) bound.push_back(o);
    }
    const auto fp = audio::musicalContentFingerprint({phraseIntents}, bound, sampleRate, role, phraseStartFrame);
    if (!fp.valid) return std::nullopt;
    return fp.hash;
}

std::vector<std::string> phraseGroupIds(const FixturePlan& plan, std::uint32_t sampleRate) {
    std::vector<std::string> ids;
    for (std::size_t i = 0; i < plan.phrases.size() && i < plan.intentsPerPhrase.size(); ++i) {
        const auto fp = phraseContentFingerprint(plan.intentsPerPhrase[i], plan.noteOffs, plan.phrases[i].startFrame, sampleRate);
        if (!fp) return {}; // fail-closed (DIF-5e.2-03)
        ids.push_back("g" + hex16(*fp));
    }
    return ids;
}

std::optional<std::uint64_t> symbolicPhraseFingerprint(const std::vector<audio::MusicIntentEvent>& phraseIntents,
                                                       const std::vector<audio::PlannedNoteOff>& noteOffs, std::uint64_t phraseStartFrame,
                                                       std::uint64_t phraseEndFrame, std::uint32_t barCount) {
    if (barCount == 0 || phraseEndFrame <= phraseStartFrame) return std::nullopt;
    const double unitsPerFrame = static_cast<double>(kSymbolicGridPerBar) * static_cast<double>(barCount) /
                                 static_cast<double>(phraseEndFrame - phraseStartFrame);
    const auto grid = [&](std::uint64_t frame) {
        const double rel = static_cast<double>(frame) - static_cast<double>(phraseStartFrame);
        return static_cast<std::int64_t>(std::llround(rel * unitsPerFrame));
    };
    // (role, q_on, cents, velocity x1000, timbre x1000, q_end of the duration NoteOff or -1, sorted explicit releases)
    using Note = std::tuple<int, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::int64_t, std::vector<std::int64_t>>;
    std::vector<Note> notes;
    for (const auto& e : phraseIntents) {
        if (!std::isfinite(e.pitchHz) || !std::isfinite(e.velocity) || !std::isfinite(e.timbreParam) || e.pitchHz < 0.0f) return std::nullopt;
        const std::int64_t cents = e.pitchHz > 0.0f
                                       ? std::llround(1200.0 * std::log2(static_cast<double>(e.pitchHz) / 440.0))
                                       : static_cast<std::int64_t>(INT32_MIN);
        std::vector<std::int64_t> releases;
        for (const auto& o : noteOffs) {
            if (o.eventId == e.eventId && o.role == e.role) releases.push_back(grid(o.sampleFrame));
        }
        std::sort(releases.begin(), releases.end());
        notes.emplace_back(static_cast<int>(e.role), grid(e.sampleFrame), cents, std::llround(1000.0 * e.velocity),
                           std::llround(1000.0 * e.timbreParam), e.durationFrames > 0 ? grid(e.sampleFrame + e.durationFrames) : -1,
                           std::move(releases));
    }
    std::sort(notes.begin(), notes.end());
    std::uint64_t h = 1469598103934665603ULL;
    auto mix = [&h](std::int64_t v) {
        const auto u = static_cast<std::uint64_t>(v);
        for (int i = 0; i < 8; ++i) {
            h ^= (u >> (8 * i)) & 0xFFU;
            h *= 1099511628211ULL;
        }
    };
    mix(static_cast<std::int64_t>(kSymbolicGridPerBar));
    mix(static_cast<std::int64_t>(barCount));
    mix(static_cast<std::int64_t>(notes.size()));
    for (const auto& [role, on, cents, vel, timbre, end, releases] : notes) {
        mix(role);
        mix(on);
        mix(cents);
        mix(vel);
        mix(timbre);
        mix(end);
        mix(static_cast<std::int64_t>(releases.size()));
        for (const auto r : releases) mix(r);
    }
    return h;
}

std::vector<std::string> symbolicPhraseGroupIds(const FixturePlan& plan) {
    std::vector<std::string> ids;
    for (std::size_t i = 0; i < plan.phrases.size() && i < plan.intentsPerPhrase.size(); ++i) {
        const auto& p = plan.phrases[i];
        const auto fp = symbolicPhraseFingerprint(plan.intentsPerPhrase[i], plan.noteOffs, p.startFrame, p.endFrame, p.barCount);
        if (!fp) return {}; // fail-closed
        ids.push_back("s" + hex16(*fp));
    }
    return ids;
}

std::vector<NoteToken> planTokens(const FixturePlan& plan) {
    std::map<std::pair<std::uint64_t, int>, std::uint64_t> release;
    for (const auto& o : plan.noteOffs) release[{o.eventId, static_cast<int>(o.role)}] = o.sampleFrame;
    std::vector<NoteToken> tokens;
    for (std::size_t i = 0; i < plan.phrases.size() && i < plan.intentsPerPhrase.size(); ++i) {
        const auto& p = plan.phrases[i];
        if (p.barCount == 0 || p.endFrame <= p.startFrame) continue;
        const double sixteenth = static_cast<double>(p.endFrame - p.startFrame) / (static_cast<double>(p.barCount) * kDiversityGridPerBar);
        for (const auto& e : plan.intentsPerPhrase[i]) {
            const auto it = release.find({e.eventId, static_cast<int>(e.role)});
            const std::uint64_t end = it != release.end() ? it->second : e.sampleFrame + e.durationFrames;
            const double dur = end > e.sampleFrame ? static_cast<double>(end - e.sampleFrame) : 0.0;
            NoteToken t;
            t.role = static_cast<std::size_t>(e.role);
            t.onset = static_cast<std::int64_t>(p.startBar) * kDiversityGridPerBar +
                      std::llround((static_cast<double>(e.sampleFrame) - static_cast<double>(p.startFrame)) / sixteenth);
            t.midi = (e.pitchHz > 0.0f && std::isfinite(e.pitchHz))
                         ? static_cast<int>(std::lround(69.0 + 12.0 * std::log2(static_cast<double>(e.pitchHz) / 440.0)))
                         : -1;
            t.durationClass = static_cast<std::uint32_t>((std::min<long long>)(kDiversityMaxDurationClass, (std::max)(1LL, std::llround(dur / sixteenth))));
            tokens.push_back(t);
        }
    }
    std::sort(tokens.begin(), tokens.end(), [](const NoteToken& a, const NoteToken& b) {
        return std::tie(a.role, a.onset, a.midi, a.durationClass) < std::tie(b.role, b.onset, b.midi, b.durationClass);
    });
    return tokens;
}

std::vector<double> m2Features(const std::vector<NoteToken>& tokens) {
    std::vector<double> f(12 + 18 + 3, 0.0);
    std::vector<std::int64_t> onsets;
    int lo = 1000, hi = -1000;
    std::size_t pitched = 0;
    for (const auto& t : tokens) {
        onsets.push_back(t.onset);
        if (t.midi < 0) continue;
        f[static_cast<std::size_t>(t.midi % 12)] += 1.0;
        lo = (std::min)(lo, t.midi);
        hi = (std::max)(hi, t.midi);
        ++pitched;
    }
    double entropy = 0.0;
    for (std::size_t i = 0; i < 12; ++i) {
        if (pitched == 0) break;
        f[i] /= static_cast<double>(pitched);
        if (f[i] > 0.0) entropy -= f[i] * std::log2(f[i]);
    }
    std::sort(onsets.begin(), onsets.end());
    onsets.erase(std::unique(onsets.begin(), onsets.end()), onsets.end());
    std::size_t iois = 0;
    for (std::size_t i = 1; i < onsets.size(); ++i) {
        const auto d = onsets[i] - onsets[i - 1];
        f[12 + static_cast<std::size_t>((std::min<std::int64_t>)(d, 17))] += 1.0;
        ++iois;
    }
    for (std::size_t i = 12; i < 30 && iois > 0; ++i) f[i] /= static_cast<double>(iois);
    f[30] = static_cast<double>(tokens.size()) / 1000.0;
    f[31] = pitched > 0 ? static_cast<double>(hi - lo) / 88.0 : 0.0;
    f[32] = entropy / std::log2(12.0);
    return f;
}

double m3CardinalityDistance(const std::vector<NoteToken>& a, const std::vector<NoteToken>& b, std::size_t role) {
    std::vector<std::pair<std::int64_t, int>> q, r;
    for (const auto& t : a) if (t.role == role) q.emplace_back(t.onset, t.midi);
    for (const auto& t : b) if (t.role == role) r.emplace_back(t.onset, t.midi);
    if (q.empty() && r.empty()) return 0.0;
    if (q.empty() || r.empty()) return 1.0;
    // Multiset intersection (DIF-5e.2-02): for a translation t, |Q ∩ (R + t)| = sum over distinct points p of Q of
    // min(count_Q(p), count_R(p - t)). Each distinct q pairs with exactly one distinct r for a given t, so the sum
    // accumulates min(a, b) over the pairs of distinct points; the result stays in [0, 1].
    std::map<std::pair<std::int64_t, int>, std::size_t> cq, cr;
    for (const auto& x : q) ++cq[x];
    for (const auto& y : r) ++cr[y];
    std::map<std::pair<std::int64_t, int>, std::size_t> score;
    std::size_t best = 0;
    for (const auto& [x, aCount] : cq) {
        for (const auto& [y, bCount] : cr) {
            const std::pair<std::int64_t, int> t{x.first - y.first, x.second - y.second};
            best = (std::max)(best, score[t] += (std::min)(aCount, bCount));
        }
    }
    return 1.0 - static_cast<double>(best) / static_cast<double>((std::max)(q.size(), r.size()));
}

double m4EditDistance(const std::vector<NoteToken>& a, const std::vector<NoteToken>& b, std::size_t role) {
    auto seq = [role](const std::vector<NoteToken>& v) {
        std::vector<std::tuple<std::int64_t, int, std::uint32_t>> s;
        std::int64_t prevOnset = 0;
        int prevMidi = 0;
        bool first = true;
        for (const auto& t : v) {
            if (t.role != role) continue;
            s.emplace_back(first ? 0 : t.onset - prevOnset, first ? 0 : t.midi - prevMidi, t.durationClass);
            prevOnset = t.onset;
            prevMidi = t.midi;
            first = false;
        }
        return s;
    };
    const auto x = seq(a), y = seq(b);
    if (x.empty() && y.empty()) return 0.0;
    std::vector<std::size_t> prev(y.size() + 1), cur(y.size() + 1);
    for (std::size_t j = 0; j <= y.size(); ++j) prev[j] = j;
    for (std::size_t i = 1; i <= x.size(); ++i) {
        cur[0] = i;
        for (std::size_t j = 1; j <= y.size(); ++j) {
            cur[j] = (std::min)({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (x[i - 1] == y[j - 1] ? 0U : 1U)});
        }
        std::swap(prev, cur);
    }
    return static_cast<double>(prev[y.size()]) / static_cast<double>((std::max)(x.size(), y.size()));
}

DiversityResult measureSeedDiversity() {
    DiversityResult d;
    struct SeedData {
        FixturePlan plan;
        std::uint64_t fp{0};
        std::vector<std::array<std::uint64_t, kRoleCount>> phraseRoleFp;
        std::vector<NoteToken> tokens;
    };
    std::vector<SeedData> data;
    for (std::uint64_t s = kDiversitySeedFirst; s <= kDiversitySeedLast; ++s) {
        SeedData sd;
        sd.plan = planFixtureM24V1(s);
        if (!sd.plan.ok) {
            d.status = "INCONCLUSIVE";
            d.error = "planning failed for seed " + std::to_string(s) + ": " + sd.plan.error;
            return d;
        }
        const auto fp = audio::musicalContentFingerprint(sd.plan.intentsPerPhrase, sd.plan.noteOffs, kFs);
        if (!fp.valid) {
            d.status = "INCONCLUSIVE";
            d.error = "invalid content fingerprint for seed " + std::to_string(s);
            return d;
        }
        sd.fp = fp.hash;
        for (std::size_t i = 0; i < sd.plan.phrases.size(); ++i) {
            std::array<std::uint64_t, kRoleCount> pr{};
            for (std::size_t r = 0; r < kRoleCount; ++r) {
                pr[r] = phraseContentFingerprint(sd.plan.intentsPerPhrase[i], sd.plan.noteOffs, sd.plan.phrases[i].startFrame, kFs,
                                                 static_cast<audio::MusicRole>(r))
                            .value_or(0);
            }
            sd.phraseRoleFp.push_back(pr);
        }
        sd.tokens = planTokens(sd.plan);
        d.seeds.push_back(s);
        d.fingerprints.push_back(sd.fp);
        data.push_back(std::move(sd));
    }
    d.distinctFingerprints = std::set<std::uint64_t>(d.fingerprints.begin(), d.fingerprints.end()).size();

    std::size_t identicalPairs = 0;
    std::array<double, kRoleCount> phraseShareSum{};
    for (std::size_t a = 0; a < data.size(); ++a) {
        for (std::size_t b = a + 1; b < data.size(); ++b) {
            ++d.pairs;
            const std::size_t phrases = (std::min)(data[a].phraseRoleFp.size(), data[b].phraseRoleFp.size());
            for (std::size_t r = 0; r < kRoleCount; ++r) {
                std::size_t same = 0;
                for (std::size_t i = 0; i < phrases; ++i) same += data[a].phraseRoleFp[i][r] == data[b].phraseRoleFp[i][r];
                phraseShareSum[r] += phrases ? static_cast<double>(same) / static_cast<double>(phrases) : 0.0;
            }
            if (data[a].fp == data[b].fp) {
                ++identicalPairs; // M2-M4 are 0 by identity: not recomputed
                continue;
            }
            ++d.distinctPairs;
            const auto fa = m2Features(data[a].tokens), fb = m2Features(data[b].tokens);
            double sq = 0.0;
            for (std::size_t i = 0; i < fa.size(); ++i) sq += (fa[i] - fb[i]) * (fa[i] - fb[i]);
            d.m2Distances.push_back(std::sqrt(sq));
            for (std::size_t r = 0; r < kRoleCount; ++r) {
                d.m3Distances[r].push_back(m3CardinalityDistance(data[a].tokens, data[b].tokens, r));
                d.m4Distances[r].push_back(m4EditDistance(data[a].tokens, data[b].tokens, r));
            }
        }
    }
    d.m1IdenticalPairShare = d.pairs ? static_cast<double>(identicalPairs) / static_cast<double>(d.pairs) : 0.0;
    for (std::size_t r = 0; r < kRoleCount; ++r) d.m1IdenticalPhraseShare[r] = d.pairs ? phraseShareSum[r] / static_cast<double>(d.pairs) : 0.0;

    const FixturePlan p1337 = planFixtureM24V1(1337);
    if (p1337.ok) {
        const auto groups = phraseGroupIds(p1337);
        d.phraseGroups1337 = std::set<std::string>(groups.begin(), groups.end()).size();
        const auto symbolic = symbolicPhraseGroupIds(p1337);
        d.phraseGroups1337Symbolic = std::set<std::string>(symbolic.begin(), symbolic.end()).size();
    }
    d.status = "MEASURED";
    if (d.distinctFingerprints <= 1) {
        d.outcome = "NULL_DIVERSITY";
        d.generalisation = "INCONCLUSIVE";
    } else if (d.distinctFingerprints < kDiversityMinDistinct) {
        d.outcome = "INSUFFICIENT_DISTINCT";
        d.generalisation = "INSUFFICIENT_SAMPLE";
    } else {
        d.outcome = "DISTRIBUTIONS";
        d.generalisation = "EVALUABLE";
    }
    return d;
}

std::string diversityJson(const DiversityResult& d) {
    json roles = json::object();
    for (std::size_t r = 0; r < kRoleCount; ++r) {
        roles[kRoleNames[r]] = {{"m1_identical_phrase_share", d.m1IdenticalPhraseShare[r]},
                                {"m3_cardinality_distance", summary(d.m3Distances[r])},
                                {"m4_edit_distance", summary(d.m4Distances[r])}};
    }
    json j{{"status", d.status},
           {"gate", false},
           {"outcome", d.outcome},
           {"generalisation_of_calibration", d.generalisation},
           {"seeds", {{"first", kDiversitySeedFirst}, {"last", kDiversitySeedLast}, {"n", d.seeds.size()}}},
           {"k_min_distinct", kDiversityMinDistinct},
           {"grid", "sixteenths of the phrase bar"},
           {"distinct_fingerprints", d.distinctFingerprints},
           {"pairs", d.pairs},
           {"pairs_with_distinct_content", d.distinctPairs},
           {"m1_identical_pair_share", d.m1IdenticalPairShare},
           {"m2_feature_distance", summary(d.m2Distances)},
           {"roles", roles},
           {"phrase_groups_seed_1337", d.phraseGroups1337},
           {"phrase_groups_seed_1337_symbolic", d.phraseGroups1337Symbolic},
           {"note", "identical pairs are 0 by identity for M2-M4 and are not recomputed; diversity is not a gate and is not "
                    "to be maximised (ADR-0006 D-M2-16)"}};
    if (!d.error.empty()) j["error"] = d.error;
    return j.dump();
}

} // namespace cymatica::cacp
