#include <catch2/catch_test_macros.hpp>

#include "acao02_calibration.h"
#include "audio_engine.h"
#include "audio_probe.h"
#include "audio_scheduler.h"
#include "event_chain_log.h"
#include "stem_analysis.h"

#include <cmath>
#include <limits>
#include <string>
#include <vector>

using namespace cymatica::cacp;
using cymatica::audio::AudioCommand;
using cymatica::audio::AudioCommandAck;
using cymatica::audio::AudioCommandType;
using cymatica::audio::AudioEngine;
using cymatica::audio::AudioScheduler;
using cymatica::audio::MusicIntentEvent;
using cymatica::audio::MusicRole;

namespace {

// Renders notes of one role offline with the stem sink installed; returns the rendered frame count
std::uint64_t renderRoleNotes(MusicRole role, const std::vector<std::pair<double, double>>& onsetsAndDurations,
                              StemCapture& cap, std::vector<AppliedEventRecord>& applied, bool pauseHalfway = false) {
    AudioEngine engine;
    REQUIRE(engine.initOffline());
    AppliedEventLog log(1024);
    REQUIRE(engine.setAppliedEventLog(&log));
    REQUIRE(engine.setRoleStemSink(cap.sink()));
    AudioScheduler scheduler({.lookaheadFrames = 7200, .sampleRate = 48000});
    engine.sendCommand(AudioCommand{.commandId = 1, .type = AudioCommandType::Start, .targetSampleFrame = 0});
    std::uint64_t id = 1;
    for (const auto& [t, d] : onsetsAndDurations) {
        scheduler.scheduleIntent(MusicIntentEvent{.eventId = id++, .sampleFrame = static_cast<std::uint64_t>(t * 48000),
                                                  .transportEpoch = 1, .role = role, .pitchHz = 220.0f, .velocity = 0.8f,
                                                  .durationFrames = static_cast<std::uint32_t>(d * 48000), .timbreParam = 0.5f},
                                 1);
    }
    std::vector<float> block(400 * 2);
    const std::uint64_t ticks = 360; // 3 s
    for (std::uint64_t t = 0; t < ticks; ++t) {
        if (pauseHalfway && t == ticks / 2) {
            engine.sendCommand(AudioCommand{.commandId = 2, .type = AudioCommandType::Pause, .targetSampleFrame = t * 400});
        }
        (void)scheduler.dispatchDueEvents(t * 400, engine);
        engine.processBlock(block.data(), 400);
        AudioCommandAck ack{};
        while (engine.pollAck(ack)) {}
    }
    REQUIRE(engine.setRoleStemSink(nullptr));
    REQUIRE(engine.setAppliedEventLog(nullptr));
    applied.assign(log.records().begin(), log.records().end());
    return ticks * 400;
}

} // namespace

TEST_CASE("Stem tap: frame-aligned, bit-exact bus-mix reconstruction and role isolation (D-M2-13)", "[stem_analysis][stem_tap]") {
    StemCapture cap(200000);
    std::vector<AppliedEventRecord> applied;
    const auto rendered = renderRoleNotes(MusicRole::Corpo, {{0.10, 0.30}, {0.60, 0.30}, {1.10, 0.20}}, cap, applied);

    // Every output frame went through exactly one renderBlock call
    CHECK(cap.frames() == rendered);
    CHECK(cap.overflowFrames() == 0);

    // Sum of the four stems in bus order == bus mix, bit-exact
    const auto rec = reconstructBusMix(cap);
    CHECK(rec.status == AnalysisStatus::Ok);
    CHECK(rec.mismatchedSamples == 0);
    CHECK(rec.maxAbsError == 0.0);

    // Only the Corpo stem carries signal; the others are exactly zero
    const auto corpo = measureStemPresence(cap.stem(1), 0, cap.frames());
    CHECK(corpo.status == AnalysisStatus::Ok);
    CHECK(corpo.peakDbfs > -40.0);
    for (const std::size_t r : {0u, 2u, 3u}) {
        const auto p = measureStemPresence(cap.stem(r), 0, cap.frames());
        CHECK(p.status == AnalysisStatus::ExactSilence);
        CHECK(p.exactSilence);
    }
}

TEST_CASE("Stem tap stays aligned through frozen pause silence and counts overflow (D-M2-13)", "[stem_analysis][stem_tap]") {
    StemCapture cap(200000);
    std::vector<AppliedEventRecord> applied;
    const auto rendered = renderRoleNotes(MusicRole::Vettore, {{0.10, 0.40}}, cap, applied, true);
    CHECK(cap.frames() == rendered); // frozen-silence frames are written as zeros
    const auto tail = measureStemPresence(cap.stem(3), cap.frames() - 4800, cap.frames());
    CHECK(tail.status == AnalysisStatus::ExactSilence);

    StemCapture small(1000);
    std::vector<AppliedEventRecord> applied2;
    const auto rendered2 = renderRoleNotes(MusicRole::Pulso, {{0.01, 0.05}}, small, applied2);
    CHECK(small.frames() == 1000);
    CHECK(small.overflowFrames() == rendered2 - 1000);
}

TEST_CASE("Stem presence, references, releases and the release diagnostic (D-M2-14, OSS-5b-01)", "[stem_analysis][references]") {
    // Presence measured on PCM
    const std::vector<float> silent(2000, 0.0f);
    CHECK(measureStemPresence(silent, 0, 1000).status == AnalysisStatus::ExactSilence);
    std::vector<float> tiny(2000, 0.0f);
    tiny[100] = 1e-30f; // physically present, far below any spectrogram floor
    const auto pTiny = measureStemPresence(tiny, 0, 1000);
    CHECK(pTiny.status == AnalysisStatus::Ok);
    CHECK_FALSE(pTiny.exactSilence);
    CHECK(measureStemPresence(tiny, 10, 5).status == AnalysisStatus::InvalidConfig);
    tiny[50] = std::numeric_limits<float>::quiet_NaN();
    CHECK(measureStemPresence(tiny, 0, 1000).status == AnalysisStatus::NonFiniteInput);

    // References: Applied NoteOns of the role, merged within 30 ms keeping the first; releases from NoteOffs
    using cymatica::audio::AudioEventType;
    auto rec = [](AudioEventType type, MusicRole role, std::uint64_t frame, EventDisposition d) {
        return AppliedEventRecord{.id = {.eventId = frame, .sampleFrame = frame, .transportEpoch = 1, .type = type, .role = role},
                                  .consumedFrame = frame, .disposition = d};
    };
    const std::vector<AppliedEventRecord> applied{
        rec(AudioEventType::NoteOn, MusicRole::Corpo, 48000, EventDisposition::Applied),
        rec(AudioEventType::NoteOn, MusicRole::Corpo, 48000 + 960, EventDisposition::Applied),   // +20 ms: fused
        rec(AudioEventType::NoteOn, MusicRole::Corpo, 96000, EventDisposition::Applied),
        rec(AudioEventType::NoteOn, MusicRole::Corpo, 120000, EventDisposition::Dropped),       // not applied
        rec(AudioEventType::NoteOn, MusicRole::Trama, 50000, EventDisposition::Applied),        // other role
        rec(AudioEventType::NoteOff, MusicRole::Corpo, 72000, EventDisposition::Applied)};
    const auto refs = roleReferenceOnsetsS(applied, MusicRole::Corpo);
    REQUIRE(refs.size() == 2);
    CHECK(refs[0] == 1.0);
    CHECK(refs[1] == 2.0);
    const auto rel = roleReleaseTimesS(applied, MusicRole::Corpo);
    REQUIRE(rel.size() == 1);
    CHECK(rel[0] == 1.5);

    // On a real render every extra detection of a sustained note sits on its release (2 ms ramp)
    StemCapture cap(200000);
    std::vector<AppliedEventRecord> rendered;
    (void)renderRoleNotes(MusicRole::Vettore, {{0.25, 0.30}, {0.75, 0.30}, {1.25, 0.30}, {1.75, 0.30}}, cap, rendered);
    const auto vRefs = roleReferenceOnsetsS(rendered, MusicRole::Vettore);
    const auto vRel = roleReleaseTimesS(rendered, MusicRole::Vettore);
    REQUIRE(vRefs.size() == 4);
    REQUIRE(vRel.size() == 4);
    const auto ev = evaluateRoleOnsets(cap.stem(3), vRefs, RoleOnsetProfile{0.10, 0.0}, 48000, vRel);
    REQUIRE(ev.status == AnalysisStatus::Ok);
    CHECK(ev.at50.recall == 1.0);
    CHECK(ev.detections == ev.at50.matches + ev.releaseDetections);
    CHECK(ev.precisionExclReleases == 1.0);

    // Invalid offset is a configuration error
    CHECK(evaluateRoleOnsets(cap.stem(3), vRefs, RoleOnsetProfile{0.10, std::nan("")}).status == AnalysisStatus::InvalidConfig);
}

TEST_CASE("Frozen stem onset profile v1 is versioned and hashed (D-M2-16)", "[stem_analysis][frozen_profile]") {
    const auto frozen = frozenStemProfileV1();
    REQUIRE(frozen.has_value());
    CHECK(std::string(frozen->version) == "stem_onset_profile_v1");
    // Any change of the frozen values must change this hash and be recorded as a new profile version
    CHECK(stemProfileSha256(frozen->roles) == "1ad4410052efe85e0fb4d4ea00c7e77383da9a1cea05e4b49fc5fea93bd8b3cd");
}

TEST_CASE("planner_probe_v1 analysis capture: stems aligned with the PCM and applied events recorded", "[stem_analysis][probe_capture]") {
    StemCapture cap(800000);
    std::vector<float> stereo;
    std::vector<AppliedEventRecord> applied;
    cymatica::audio::ProbeAnalysisCapture pc{};
    pc.stems = cap.sink();
    pc.stereo = &stereo;
    pc.applied = &applied;
    const auto res = cymatica::audio::runPlannerProbeV1Offline("artifacts/cacp", false, &pc);
    INFO(res.failureReason);
    CHECK(res.passed);
    CHECK(cap.overflowFrames() == 0);
    CHECK(cap.frames() * 2 == stereo.size());
    CHECK(reconstructBusMix(cap).mismatchedSamples == 0);
    std::size_t noteOns = 0;
    for (const auto& r : applied) if (r.id.type == cymatica::audio::AudioEventType::NoteOn) ++noteOns;
    CHECK(noteOns == res.generatedEvents);
}
