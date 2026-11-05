// Meter change feasibility (ADR-0007, M2.5.1b, P-M25-01). Spike, not an integration: it changes nothing in the
// engine or in fixture-m2-4-v1. The form below exists only here, to give CP-1 facts about what already works.
#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_scheduler.h"
#include "music_clock.h"
#include "music_planner.h"
#include "plan_canonical_hash.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

using namespace cymatica::audio;

namespace {

constexpr std::uint32_t kBarsPerSection = 8;
constexpr std::uint32_t kPhrasesPerSection = 2;
constexpr std::uint32_t kBlockFrames = 480;

TempoMap fourFour() {
    return TempoMap{.bpm = RationalBpm{120, 1}, .timeSignatureNum = 4, .timeSignatureDen = 4,
                    .subdivisionsPerBeat = 4, .beatsPerBar = 4, .barsPerPhrase = 4};
}

TempoMap meter(std::uint32_t num, std::uint32_t den, std::uint32_t subdivisions) {
    return TempoMap{.bpm = RationalBpm{120, 1}, .timeSignatureNum = num, .timeSignatureDen = den,
                    .subdivisionsPerBeat = subdivisions, .beatsPerBar = num, .barsPerPhrase = 4};
}

// Dorian base motif rewritten for a bar of 12 steps (the stock one has a note on step 12)
Motif twelveStepMotif() {
    Motif m{};
    m.motifId = 301;
    m.targetRole = MusicRole::Vettore;
    m.lengthSteps = 12;
    m.noteCount = 3;
    m.notes[0] = MotifNote{.scaleDegree = 0, .stepOffset = 0, .durationSteps = 4, .velocity = 0.85f, .isAnchor = true};
    m.notes[1] = MotifNote{.scaleDegree = 2, .stepOffset = 4, .durationSteps = 4, .velocity = 0.75f, .isAnchor = false};
    m.notes[2] = MotifNote{.scaleDegree = 4, .stepOffset = 8, .durationSteps = 4, .velocity = 0.80f, .isAnchor = true};
    return m;
}

FormPlan formWithMiddleMeter(const TempoMap& middle, const Motif& middleMotif) {
    FormPlan form{};
    form.sectionCount = 3;
    const ModalScale dorian{.rootMidiNote = 62, .mode = ScaleMode::Dorian};
    for (std::uint32_t s = 0; s < 3; ++s) {
        form.sections[s] = SectionPlan{.sectionIndex = s, .startBar = s * kBarsPerSection, .barCount = kBarsPerSection,
                                       .tempoMap = (s == 1) ? middle : fourFour(), .scale = dorian, .baseTension = 0.4f,
                                       .sectionMotif = (s == 1) ? middleMotif : MusicPlanner::makeDorianBaseMotif()};
    }
    return form;
}

MusicClock clockOf(const FormPlan& form) {
    auto clock = MusicClock::create(48000, form.sections[0].tempoMap);
    REQUIRE(clock.has_value());
    REQUIRE(clock->setTempoMap(form.sections[1].tempoMap, clock->frameAtBar(form.sections[1].startBar)));
    REQUIRE(clock->setTempoMap(form.sections[2].tempoMap, clock->frameAtBar(form.sections[2].startBar)));
    return *clock;
}

std::vector<PhrasePlan> phrasesOf(const FormPlan& form, const MusicClock& clock) {
    const std::array<ChordSpec, 4> chords{
        ChordSpec{0, 2, 4, HarmonicFunction::Stability, false},
        ChordSpec{3, 5, 0, HarmonicFunction::Preparation, false},
        ChordSpec{4, 6, 1, HarmonicFunction::Tension, false},
        ChordSpec{0, 2, 4, HarmonicFunction::Resolution, false}};
    std::vector<PhrasePlan> phrases;
    for (std::uint32_t s = 0; s < 3; ++s) {
        for (std::uint32_t p = 0; p < kPhrasesPerSection; ++p) {
            phrases.push_back(MusicPlanner::makePhrasePlan(clock, form.sections[s], p, chords, CadenceGoal::Sustain));
        }
    }
    return phrases;
}

struct PlannedForm {
    std::vector<std::vector<MusicIntentEvent>> intents;
    std::vector<PhraseDecisionTrace> traces;
    std::vector<PlannedNoteOff> noteOffs;
};

PlannedForm plan(const FormPlan& form, const MusicClock& clock, std::uint64_t seed = 1) {
    const auto phrases = phrasesOf(form, clock);
    MusicPlanner planner({.runSeed = seed, .candidateCount = 8, .sampleRate = 48000});
    planner.setFormPlan(form);
    PlannedForm out{};
    for (const PhrasePlan& phrase : phrases) {
        std::vector<MusicIntentEvent> intents;
        planner.planPhrase(phrase, clock, phrase.startFrame, phrase.sectionIndex + 1, intents);
        out.intents.push_back(intents);
    }
    out.traces = planner.decisionTraces();
    out.noteOffs = planner.plannedNoteOffs();
    return out;
}

} // namespace

TEST_CASE("Meter feasibility: the clock keeps exact bars and phrases across 4/4, 3/4, 4/4", "[meter_feasibility]") {
    const FormPlan form = formWithMiddleMeter(meter(3, 4, 4), twelveStepMotif());
    const MusicClock clock = clockOf(form);

    CHECK(clock.frameAtBar(8) == 768000);   // 8 bars of 96000 frames
    CHECK(clock.frameAtBar(16) == 1344000); // then 8 bars of 72000 frames
    CHECK(clock.frameAtBar(24) == 2112000);
    for (std::uint32_t phrase = 0; phrase < 6; ++phrase) {
        const MusicPosition position = clock.positionAtFrame(clock.frameAtPhrase(phrase));
        CHECK(position.phrase == phrase);
        CHECK(position.bar == 0);
        CHECK(position.beat == 0);
    }
    const MusicPosition lastBeatOfThreeFour = clock.positionAtFrame(clock.frameAtBar(9) + 2 * 24000);
    CHECK(lastBeatOfThreeFour.beat == 2);
    CHECK(lastBeatOfThreeFour.bar == 1);
}

TEST_CASE("Meter feasibility: which meters the stock planner accepts with the stock motif", "[meter_feasibility]") {
    struct Row {
        const char* name;
        TempoMap map;
        std::uint32_t expectedValid; // candidates of the first phrase of the middle section
    };
    const std::array<Row, 5> rows{{{"3/4", meter(3, 4, 4), 0},
                                   {"6/8", meter(6, 8, 2), 0},
                                   {"5/4", meter(5, 4, 4), 8},
                                   {"7/8", meter(7, 8, 2), 7},
                                   {"12/8", meter(12, 8, 2), 8}}};
    for (const Row& row : rows) {
        const FormPlan form = formWithMiddleMeter(row.map, MusicPlanner::makeDorianBaseMotif());
        const MusicClock clock = clockOf(form);
        const PlannedForm planned = plan(form, clock);
        const PhraseDecisionTrace& middle = planned.traces[2];
        INFO(row.name);
        CHECK(middle.candidatesValid == row.expectedValid);
        CHECK(middle.fallbackTriggered == (row.expectedValid == 0)); // no error reaches the caller: the plan just thins out
    }
}

TEST_CASE("Meter feasibility: the 12-step motif also fits 6/8", "[meter_feasibility]") {
    const FormPlan form = formWithMiddleMeter(meter(6, 8, 2), twelveStepMotif());
    const MusicClock clock = clockOf(form);
    const PlannedForm planned = plan(form, clock);
    CHECK(clock.frameAtBar(16) - clock.frameAtBar(8) == 8 * 72000); // the BPM counts quarter notes in every meter
    CHECK(planned.traces[2].candidatesValid == planned.traces[2].candidatesGenerated);
    CHECK_FALSE(planned.traces[2].fallbackTriggered);
}

TEST_CASE("Meter feasibility: a 12-step motif gives a full plan in 3/4 on the 3/4 grid", "[meter_feasibility]") {
    const FormPlan form = formWithMiddleMeter(meter(3, 4, 4), twelveStepMotif());
    const MusicClock clock = clockOf(form);
    const PlannedForm planned = plan(form, clock);

    REQUIRE(planned.traces.size() == 6);
    for (const PhraseDecisionTrace& trace : planned.traces) {
        CHECK_FALSE(trace.fallbackTriggered);
        CHECK(trace.candidatesValid == trace.candidatesGenerated);
    }
    const std::uint64_t sectionStart = clock.frameAtBar(8);
    for (std::size_t p = 2; p < 4; ++p) {
        REQUIRE_FALSE(planned.intents[p].empty());
        for (const MusicIntentEvent& e : planned.intents[p]) {
            CHECK((e.sampleFrame - sectionStart) % 6000 == 0); // sixteenth at 120 BPM
        }
    }
    for (std::uint32_t bar = 8; bar < 16; ++bar) {
        const auto& intents = planned.intents[2 + (bar - 8) / 4];
        const std::uint64_t downbeat = clock.frameAtBar(bar);
        CHECK(std::any_of(intents.begin(), intents.end(), [&](const MusicIntentEvent& e) {
            return e.role == MusicRole::Corpo && e.sampleFrame == downbeat;
        }));
    }
    CHECK(canonicalPlanHash(planned.intents, planned.noteOffs, planned.traces) ==
          [&] {
              const PlannedForm again = plan(form, clock);
              return canonicalPlanHash(again.intents, again.noteOffs, again.traces);
          }());
}

TEST_CASE("Meter feasibility: the offline chain renders 4/4, 3/4, 4/4 without losing events", "[meter_feasibility]") {
    const FormPlan form = formWithMiddleMeter(meter(3, 4, 4), twelveStepMotif());
    const MusicClock clock = clockOf(form);
    const auto phrases = phrasesOf(form, clock);
    MusicPlanner planner({.runSeed = 1, .candidateCount = 8, .sampleRate = 48000,
                          .lookaheadFrames = AudioSchedulerConfig{}.lookaheadFrames});
    planner.setFormPlan(form);
    AudioScheduler scheduler;
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));

    std::size_t next = 0;
    std::size_t generated = 0;
    const auto planNext = [&](std::uint64_t horizon) {
        std::vector<MusicIntentEvent> intents;
        const PhrasePlan& phrase = phrases[next++];
        planner.planPhrase(phrase, clock, horizon, phrase.sectionIndex + 1, intents, &scheduler);
        generated += intents.size();
    };
    const std::array<std::uint64_t, 2> boundary{clock.frameAtBar(8), clock.frameAtBar(16)};
    const std::array<std::uint64_t, 2> sendAt{clock.frameAtBar(7), clock.frameAtBar(15)};
    const std::uint64_t endFrame = clock.frameAtBar(24);
    std::array<bool, 2> tempoSent{};
    bool stopSent = false;
    std::array<AudioCommandAck, 2> tempoAck{};
    AudioCommandAck stopAck{};

    planNext(0);
    planNext(0);
    REQUIRE(engine.sendCommand(AudioCommand{.commandId = 1, .type = AudioCommandType::Start, .targetSampleFrame = 0}));

    std::vector<float> block(static_cast<std::size_t>(kBlockFrames) * 2);
    std::array<double, 3> energy{};
    float peak = 0.0f;
    bool finite = true;
    for (std::uint64_t frame = 0; frame < endFrame + 48000; frame += kBlockFrames) {
        for (std::size_t i = 0; i < 2; ++i) {
            if (!tempoSent[i] && frame >= sendAt[i]) {
                REQUIRE(engine.sendCommand(AudioCommand::makeSetTempoMap(2 + i, form.sections[i + 1].tempoMap, boundary[i])));
                tempoSent[i] = true;
            }
        }
        if (!stopSent && frame >= clock.frameAtBar(23)) {
            REQUIRE(engine.sendCommand(AudioCommand{.commandId = 4, .type = AudioCommandType::Stop, .targetSampleFrame = endFrame}));
            stopSent = true;
        }
        while (next < phrases.size() && frame >= phrases[next - 1].startFrame) {
            planNext(frame + scheduler.lookaheadFrames());
        }
        (void)scheduler.dispatchDueEvents(frame, engine);
        engine.processBlock(block.data(), kBlockFrames);
        const std::size_t section = frame < boundary[0] ? 0 : (frame < boundary[1] ? 1 : 2);
        for (const float sample : block) {
            finite = finite && std::isfinite(sample);
            peak = std::max(peak, std::fabs(sample));
            energy[section] += static_cast<double>(sample) * sample;
        }
        AudioCommandAck ack{};
        while (engine.pollAck(ack)) {
            if (ack.commandId == 2 || ack.commandId == 3) tempoAck[ack.commandId - 2] = ack;
            if (ack.commandId == 4) stopAck = ack;
        }
    }
    engine.shutdown();

    CHECK(tempoAck[0].status == AudioCommandStatus::Applied);
    CHECK(tempoAck[0].sampleFrame == boundary[0]);
    CHECK(tempoAck[1].status == AudioCommandStatus::Applied);
    CHECK(tempoAck[1].sampleFrame == boundary[1]);
    CHECK(stopAck.status == AudioCommandStatus::Applied);
    CHECK(stopAck.sampleFrame == endFrame);
    CHECK(next == phrases.size());
    CHECK(engine.droppedScheduledEvents() == 0);
    CHECK(scheduler.stagedEventCount() == 0);
    CHECK(scheduler.totalDispatchedEvents() == 2 * generated);
    CHECK(finite);
    CHECK(peak > 0.0f);
    CHECK(energy[1] > 0.0);
    CHECK(energy[2] > 0.0);
}
