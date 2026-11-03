// Analysis tool (outside the repository): tie structure of the planner decisions on fixture-m2-4-v1 and planner_probe_v1.
#include "music_planner.h"

#include <algorithm>
#include <cstdio>
#include <vector>

using namespace cymatica::audio;

struct Run {
    std::vector<PhraseDecisionTrace> traces;
    std::vector<std::vector<MusicIntentEvent>> intents;
};

template <std::size_t N>
Run plan(std::uint64_t seed, const MusicClock& clock, const FormPlan& form, const std::array<PhrasePlan, N>& phrases, std::uint32_t cands) {
    MusicPlanner p({.runSeed = seed, .candidateCount = cands, .sampleRate = 48000, .lookaheadFrames = AudioSchedulerConfig{}.lookaheadFrames});
    p.setFormPlan(form);
    Run r;
    r.intents.resize(N);
    for (std::size_t i = 0; i < N; ++i) p.planPhrase(phrases[i], clock, 0, 1, r.intents[i], nullptr);
    r.traces = p.decisionTraces();
    return r;
}

template <std::size_t N>
void report(const char* name, const MusicClock& clock, const FormPlan& form, const std::array<PhrasePlan, N>& phrases, std::uint32_t cands) {
    const Run base = plan(1337, clock, form, phrases, cands);
    std::printf("== %s (candidateCount %u)\n", name, cands);
    std::printf("phrase sec valid tied winScore second gap winIdx kind param fallback | scores\n");
    std::size_t tiedDecisions = 0;
    for (std::size_t p = 0; p < base.traces.size(); ++p) {
        const auto& t = base.traces[p];
        std::vector<int> valid;
        for (std::size_t c = 0; c < t.candidatesGenerated; ++c) {
            if (t.candidates[c].violation == HardConstraintViolation::None) valid.push_back(t.candidates[c].score);
        }
        std::vector<int> sorted = valid;
        std::sort(sorted.rbegin(), sorted.rend());
        const int second = sorted.size() > 1 ? sorted[1] : -1;
        if (t.tiedTopCandidates > 1) ++tiedDecisions;
        std::printf("%2zu %u %u %u %6d %6d %6d %3llu %u %d %d |", p, t.sectionIndex, t.candidatesValid, t.tiedTopCandidates,
                    t.winningScore, second, t.winningScore - second, static_cast<unsigned long long>(t.winningCandidateId & 0xFF),
                    static_cast<unsigned>(t.winningTransformKind), t.winningTransformParam, t.fallbackTriggered ? 1 : 0);
        for (std::size_t c = 0; c < t.candidatesGenerated; ++c) {
            const auto& e = t.candidates[c];
            std::printf(" %d%s", e.score, e.violation == HardConstraintViolation::None ? "" : "x");
        }
        std::printf("\n");
    }
    std::printf("tied decisions: %zu of %zu\n", tiedDecisions, base.traces.size());
    std::size_t seedsDiffer = 0;
    for (std::uint64_t s = 1; s <= 64; ++s) {
        const Run r = plan(s, clock, form, phrases, cands);
        bool same = r.intents.size() == base.intents.size();
        for (std::size_t p = 0; same && p < r.intents.size(); ++p) {
            if (r.intents[p].size() != base.intents[p].size()) { same = false; break; }
            for (std::size_t k = 0; k < r.intents[p].size(); ++k) {
                const auto& a = r.intents[p][k];
                const auto& b = base.intents[p][k];
                if (a.sampleFrame != b.sampleFrame || a.role != b.role || a.pitchHz != b.pitchHz || a.velocity != b.velocity ||
                    a.durationFrames != b.durationFrames || a.timbreParam != b.timbreParam) { same = false; break; }
            }
        }
        if (!same) ++seedsDiffer;
    }
    std::printf("seeds 1..64 with event content different from seed 1337: %zu\n", seedsDiffer);
}

int main() {
    const auto fclock = MusicPlanner::makeFixtureM24V1Clock();
    if (!fclock) return 1;
    report("fixture-m2-4-v1", *fclock, MusicPlanner::makeFixtureM24V1Form(), MusicPlanner::makeFixtureM24V1Phrases(*fclock), 8);
    const auto pclock = MusicPlanner::makePlannerProbeV1Clock();
    if (!pclock) return 1;
    report("planner_probe_v1", *pclock, MusicPlanner::makePlannerProbeV1Form(), MusicPlanner::makePlannerProbeV1Phrases(*pclock), 6);
    report("planner_probe_v1 (8 candidates)", *pclock, MusicPlanner::makePlannerProbeV1Form(), MusicPlanner::makePlannerProbeV1Phrases(*pclock), 8);
    return 0;
}
