#include "meter_change.h"

#include "audio_engine.h"
#include "audio_scheduler.h"
#include "music_clock.h"
#include "music_planner.h"
#include "plan_canonical_hash.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>

namespace cymatica::cacp {

using nlohmann::json;
using namespace cymatica::audio;

namespace {

constexpr std::uint32_t kBarsPerSection = 8;
constexpr std::uint32_t kPhrasesPerSection = 2;
constexpr std::uint32_t kBlockFrames = 480;

TempoMap fourFour() {
    return TempoMap{.bpm = RationalBpm{120, 1}, .timeSignatureNum = 4, .timeSignatureDen = 4,
                    .subdivisionsPerBeat = 4, .beatsPerBar = 4, .barsPerPhrase = 4};
}

TempoMap threeFour() {
    return TempoMap{.bpm = RationalBpm{120, 1}, .timeSignatureNum = 3, .timeSignatureDen = 4,
                    .subdivisionsPerBeat = 4, .beatsPerBar = 3, .barsPerPhrase = 4};
}

// Dorian base motif rewritten for a bar of 12 steps (the stock one has a note on step 12): from the feasibility spike
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

FormPlan form() {
    FormPlan f{};
    f.sectionCount = 3;
    const ModalScale dorian{.rootMidiNote = 62, .mode = ScaleMode::Dorian};
    for (std::uint32_t s = 0; s < 3; ++s) {
        f.sections[s] = SectionPlan{.sectionIndex = s, .startBar = s * kBarsPerSection, .barCount = kBarsPerSection,
                                    .tempoMap = (s == 1) ? threeFour() : fourFour(), .scale = dorian, .baseTension = 0.4f,
                                    .sectionMotif = (s == 1) ? twelveStepMotif() : MusicPlanner::makeDorianBaseMotif()};
    }
    return f;
}

struct Run {
    MeterChangeResult r;
    std::vector<float> pcm;
};

Run runOnce() {
    Run run;
    MeterChangeResult& r = run.r;
    const FormPlan f = form();
    auto clockOpt = MusicClock::create(48000, f.sections[0].tempoMap);
    if (!clockOpt || !clockOpt->setTempoMap(f.sections[1].tempoMap, clockOpt->frameAtBar(8)) ||
        !clockOpt->setTempoMap(f.sections[2].tempoMap, clockOpt->frameAtBar(16))) {
        r.failures.emplace_back("clock setup failed");
        return run;
    }
    const MusicClock& clock = *clockOpt;
    r.bar8Frame = clock.frameAtBar(8);
    r.bar16Frame = clock.frameAtBar(16);
    r.bar24Frame = clock.frameAtBar(24);
    const std::array<ChordSpec, 4> chords{ChordSpec{0, 2, 4, HarmonicFunction::Stability, false},
                                          ChordSpec{3, 5, 0, HarmonicFunction::Preparation, false},
                                          ChordSpec{4, 6, 1, HarmonicFunction::Tension, false},
                                          ChordSpec{0, 2, 4, HarmonicFunction::Resolution, false}};
    std::vector<PhrasePlan> phrases;
    for (std::uint32_t s = 0; s < 3; ++s) {
        for (std::uint32_t p = 0; p < kPhrasesPerSection; ++p) phrases.push_back(MusicPlanner::makePhrasePlan(clock, f.sections[s], p, chords, CadenceGoal::Sustain));
    }
    MusicPlanner planner({.runSeed = 1, .candidateCount = 8, .sampleRate = 48000, .lookaheadFrames = AudioSchedulerConfig{}.lookaheadFrames});
    planner.setFormPlan(f);
    AudioScheduler scheduler;
    AudioEngine engine;
    if (!engine.initOffline({48000, 2, 220.0f})) {
        r.failures.emplace_back("offline engine initialisation failed");
        return run;
    }
    std::vector<std::vector<MusicIntentEvent>> intentsPerPhrase;
    std::size_t next = 0;
    const auto planNext = [&](std::uint64_t horizon) {
        std::vector<MusicIntentEvent> intents;
        const PhrasePlan& phrase = phrases[next++];
        planner.planPhrase(phrase, clock, horizon, phrase.sectionIndex + 1, intents, &scheduler);
        r.generated += intents.size();
        intentsPerPhrase.push_back(intents);
    };
    const std::array<std::uint64_t, 2> boundary{r.bar8Frame, r.bar16Frame};
    const std::array<std::uint64_t, 2> sendAt{clock.frameAtBar(7), clock.frameAtBar(15)};
    const std::uint64_t endFrame = r.bar24Frame;
    std::array<bool, 2> tempoSent{};
    bool stopSent = false;
    std::array<AudioCommandAck, 2> tempoAck{};
    AudioCommandAck stopAck{};
    planNext(0);
    planNext(0);
    (void)engine.sendCommand(AudioCommand{.commandId = 1, .type = AudioCommandType::Start, .targetSampleFrame = 0});
    std::vector<float> block(static_cast<std::size_t>(kBlockFrames) * 2);
    r.finite = true;
    for (std::uint64_t frame = 0; frame < endFrame + 48000; frame += kBlockFrames) {
        for (std::size_t i = 0; i < 2; ++i) {
            if (!tempoSent[i] && frame >= sendAt[i]) {
                (void)engine.sendCommand(AudioCommand::makeSetTempoMap(2 + i, f.sections[i + 1].tempoMap, boundary[i]));
                tempoSent[i] = true;
            }
        }
        if (!stopSent && frame >= clock.frameAtBar(23)) {
            (void)engine.sendCommand(AudioCommand{.commandId = 4, .type = AudioCommandType::Stop, .targetSampleFrame = endFrame});
            stopSent = true;
        }
        while (next < phrases.size() && frame >= phrases[next - 1].startFrame) planNext(frame + scheduler.lookaheadFrames());
        (void)scheduler.dispatchDueEvents(frame, engine);
        engine.processBlock(block.data(), kBlockFrames);
        const int section = frame < boundary[0] ? 0 : (frame < boundary[1] ? 1 : 2);
        for (const float v : block) {
            r.finite = r.finite && std::isfinite(v);
            const double e = static_cast<double>(v) * v;
            (section == 0 ? r.energyA : section == 1 ? r.energyB : r.energyA2) += e;
        }
        run.pcm.insert(run.pcm.end(), block.begin(), block.end());
        AudioCommandAck ack{};
        while (engine.pollAck(ack)) {
            if (ack.commandId == 2 || ack.commandId == 3) tempoAck[ack.commandId - 2] = ack;
            if (ack.commandId == 4) stopAck = ack;
        }
    }
    engine.shutdown();
    const auto status = [](const AudioCommandAck& a) { return a.status == AudioCommandStatus::Applied ? std::string("Applied") : std::string("NotApplied"); };
    r.tempoAck1 = status(tempoAck[0]);
    r.tempoAck2 = status(tempoAck[1]);
    r.stopAck = status(stopAck);
    r.tempoAck1Frame = tempoAck[0].sampleFrame;
    r.tempoAck2Frame = tempoAck[1].sampleFrame;
    r.stopAckFrame = stopAck.sampleFrame;
    r.droppedEvents = engine.droppedScheduledEvents();
    r.stagedAtEnd = scheduler.stagedEventCount();
    r.dispatched = scheduler.totalDispatchedEvents();
    r.phrasesPlanned = next;
    for (const auto& t : planner.decisionTraces()) {
        if (t.fallbackTriggered) ++r.fallbacks;
        if (t.candidatesValid != t.candidatesGenerated) ++r.invalidCandidates;
    }
    for (std::size_t p = 2; p < 4 && p < intentsPerPhrase.size(); ++p) {
        for (const auto& e : intentsPerPhrase[p]) {
            if ((e.sampleFrame - r.bar8Frame) % 6000 != 0) ++r.offGridMiddle;
        }
    }
    for (std::uint32_t bar = 8; bar < 16; ++bar) {
        const std::size_t p = 2 + (bar - 8) / 4;
        if (p >= intentsPerPhrase.size()) {
            ++r.corpoDownbeatsMissing;
            continue;
        }
        const std::uint64_t downbeat = clock.frameAtBar(bar);
        const auto& in = intentsPerPhrase[p];
        if (!std::any_of(in.begin(), in.end(), [&](const MusicIntentEvent& e) { return e.role == MusicRole::Corpo && e.sampleFrame == downbeat; })) {
            ++r.corpoDownbeatsMissing;
        }
    }
    r.planHash = canonicalPlanHash(intentsPerPhrase, planner.plannedNoteOffs(), planner.decisionTraces());
    std::uint64_t h = 1469598103934665603ULL;
    for (const float v : run.pcm) {
        const auto bits = std::bit_cast<std::uint32_t>(v);
        for (int k = 0; k < 4; ++k) {
            h ^= (bits >> (8 * k)) & 0xFFU;
            h *= 1099511628211ULL;
        }
    }
    r.pcmHash = h;
    return run;
}

} // namespace

MeterChangeResult runMeterChangeV1(const std::string& outputDir, bool writeFiles) {
    Run first = runOnce();
    const Run second = runOnce();
    MeterChangeResult r = first.r;
    r.repeatIdentical = r.planHash == second.r.planHash && r.pcmHash == second.r.pcmHash;
    auto& fail = r.failures;
    if (!(r.bar8Frame == 768000 && r.bar16Frame == 1344000 && r.bar24Frame == 2112000)) fail.emplace_back("M1: bar frames");
    if (!(r.tempoAck1 == "Applied" && r.tempoAck1Frame == 768000 && r.tempoAck2 == "Applied" && r.tempoAck2Frame == 1344000 &&
          r.stopAck == "Applied" && r.stopAckFrame == 2112000)) {
        fail.emplace_back("M2: tempo-map or stop acknowledgement");
    }
    if (!(r.droppedEvents == 0 && r.stagedAtEnd == 0 && r.dispatched == 2 * r.generated && r.phrasesPlanned == 6)) fail.emplace_back("M3: chain accounting");
    if (!(r.fallbacks == 0 && r.invalidCandidates == 0 && r.offGridMiddle == 0 && r.corpoDownbeatsMissing == 0)) fail.emplace_back("M4: 3/4 plan");
    if (!(r.finite && r.energyA > 0.0 && r.energyB > 0.0 && r.energyA2 > 0.0)) fail.emplace_back("M5: signal");
    if (!r.repeatIdentical) fail.emplace_back("M6: repeat differs");
    if (kMeterChangeGoldenPcmHash != 0) {
        r.goldenPcmStatus = r.pcmHash == kMeterChangeGoldenPcmHash ? "PASS" : "FAIL";
        if (r.goldenPcmStatus == "FAIL") fail.emplace_back("golden PCM differs");
    }
    r.verdict = fail.empty() ? "PASS" : "FAIL";
    if (writeFiles) {
        r.manifestFilePath = (std::filesystem::path(outputDir) / "meter_change_v1_manifest.json").lexically_normal().string();
        std::ofstream mf(r.manifestFilePath);
        r.manifestWriteSuccess = mf.is_open();
        if (r.manifestWriteSuccess) {
            mf << r.toJson();
            r.manifestWriteSuccess = mf.good();
        }
    }
    return r;
}

std::string MeterChangeResult::toJson() const {
    json j;
    j["scenario"] = "meter_change_v1";
    j["verdict"] = verdict;
    j["failures"] = failures;
    j["preregistration"] = "docs/research/M2_5_3_Meter_Change_Scenario_Preregistration.md";
    j["form"] = "24 bars: A 4/4, B 3/4 (12-step motif), A' 4/4; 120 BPM; seed 1; offline chain, 480-frame blocks";
    j["M1_bar_frames"] = {bar8Frame, bar16Frame, bar24Frame};
    j["M2_acks"] = {{"tempo_1", tempoAck1}, {"tempo_1_frame", tempoAck1Frame}, {"tempo_2", tempoAck2},
                    {"tempo_2_frame", tempoAck2Frame}, {"stop", stopAck}, {"stop_frame", stopAckFrame}};
    j["M3_chain"] = {{"dropped", droppedEvents}, {"staged_at_end", stagedAtEnd}, {"dispatched", dispatched},
                     {"generated_intents", generated}, {"phrases_planned", phrasesPlanned}};
    j["M4_plan_3_4"] = {{"fallbacks", fallbacks}, {"phrases_with_invalid_candidates", invalidCandidates},
                        {"middle_noteons_off_grid", offGridMiddle}, {"corpo_downbeats_missing", corpoDownbeatsMissing}};
    j["M5_signal"] = {{"finite", finite}, {"energy_a", energyA}, {"energy_b", energyB}, {"energy_a2", energyA2}};
    j["M6_repeat"] = {{"plan_hash", std::to_string(planHash)}, {"pcm_hash", std::to_string(pcmHash)}, {"identical", repeatIdentical}};
    j["golden_pcm_status"] = goldenPcmStatus;
    return j.dump(2) + "\n";
}

} // namespace cymatica::cacp
