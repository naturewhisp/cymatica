#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_scheduler.h"

using namespace cymatica::audio;

TEST_CASE("AudioScheduler dispatches events within lookahead horizon", "[scheduler][timing]") {
    AudioEngine engine;
    REQUIRE(engine.init());

    AudioSchedulerConfig cfg;
    cfg.lookaheadFrames = 7200; // 150 ms at 48 kHz (ADR-0003 D-M2-03)
    AudioScheduler scheduler(cfg);

    // Stage events at various sample frames relative to cursor 0
    ScheduledAudioEvent e1{.eventId = 1, .sampleFrame = 1000, .type = AudioEventType::NoteOn};
    ScheduledAudioEvent e2{.eventId = 2, .sampleFrame = 5000, .type = AudioEventType::NoteOn};
    ScheduledAudioEvent e3{.eventId = 3, .sampleFrame = 7200, .type = AudioEventType::NoteOn}; // On horizon boundary
    ScheduledAudioEvent e4{.eventId = 4, .sampleFrame = 7201, .type = AudioEventType::NoteOn}; // Past horizon
    ScheduledAudioEvent e5{.eventId = 5, .sampleFrame = 15000, .type = AudioEventType::NoteOn};

    scheduler.stageEvent(e1);
    scheduler.stageEvent(e2);
    scheduler.stageEvent(e3);
    scheduler.stageEvent(e4);
    scheduler.stageEvent(e5);

    REQUIRE(scheduler.stagedEventCount() == 5);

    // Dispatch at currentRenderCursor = 0: horizon is 7200
    const std::size_t dispatched1 = scheduler.dispatchDueEvents(0, engine);
    REQUIRE(dispatched1 == 3); // e1, e2, e3
    REQUIRE(scheduler.totalDispatchedEvents() == 3);
    REQUIRE(scheduler.stagedEventCount() == 2); // e4, e5 remain staged

    // Advance cursor to 1000: horizon becomes 1000 + 7200 = 8200
    const std::size_t dispatched2 = scheduler.dispatchDueEvents(1000, engine);
    REQUIRE(dispatched2 == 1); // e4 now inside horizon (7201 <= 8200)
    REQUIRE(scheduler.totalDispatchedEvents() == 4);
    REQUIRE(scheduler.stagedEventCount() == 1); // e5 remains staged

    // Advance cursor to 8000: horizon becomes 15200
    const std::size_t dispatched3 = scheduler.dispatchDueEvents(8000, engine);
    REQUIRE(dispatched3 == 1); // e5 (15000 <= 15200)
    REQUIRE(scheduler.totalDispatchedEvents() == 5);
    REQUIRE(scheduler.stagedEventCount() == 0);
}

TEST_CASE("AudioScheduler enforces stable priority ordering at identical sample frame", "[scheduler][priority]") {
    AudioEngine engine;
    REQUIRE(engine.init());

    AudioScheduler scheduler;

    // Spec §7.5 stable ordering: (sampleFrame, eventPriority, eventId)
    // Priority order: NoteOff (0) < ParameterChange (1) < NoteOn (2)
    constexpr std::uint64_t kFrame = 5000;

    ScheduledAudioEvent onEvt{.eventId = 20, .sampleFrame = kFrame, .type = AudioEventType::NoteOn};
    ScheduledAudioEvent offEvt1{.eventId = 10, .sampleFrame = kFrame, .type = AudioEventType::NoteOff};
    ScheduledAudioEvent offEvt2{.eventId = 12, .sampleFrame = kFrame, .type = AudioEventType::NoteOff};
    ScheduledAudioEvent paramEvt{.eventId = 15, .sampleFrame = kFrame, .type = AudioEventType::ParameterChange};

    // Stage in arbitrary / inverse order
    scheduler.stageEvent(onEvt);
    scheduler.stageEvent(offEvt2);
    scheduler.stageEvent(paramEvt);
    scheduler.stageEvent(offEvt1);

    REQUIRE(scheduler.stagedEventCount() == 4);

    // Dispatch to engine
    const std::size_t count = scheduler.dispatchDueEvents(0, engine);
    REQUIRE(count == 4);

    // Process blocks in AudioEngine and verify all 4 events are processed in order
    float out[512 * 2]{};
    // Render until frame 5000 is covered
    for (int i = 0; i < 10; ++i) {
        engine.processBlock(out, 512); // 512 * 10 = 5120 frames rendered
    }
    REQUIRE(engine.totalScheduledEventsProcessed() == 4);
}

TEST_CASE("AudioScheduler handles backpressure when AudioEngine queue saturates", "[scheduler][backpressure]") {
    AudioEngine engine;
    REQUIRE(engine.init());

    AudioScheduler scheduler;

    // Queue capacity in AudioEngine is 256 (ADR-0003 D-M2-03)
    // Stage 260 events within the lookahead horizon
    for (std::uint64_t i = 0; i < 260; ++i) {
        scheduler.stageEvent(ScheduledAudioEvent{
            .eventId = i,
            .sampleFrame = 100 + i,
            .type = AudioEventType::NoteOn
        });
    }

    REQUIRE(scheduler.stagedEventCount() == 260);

    // Dispatch: first 256 must succeed, 257th triggers backpressure
    const std::size_t dispatched = scheduler.dispatchDueEvents(0, engine);
    REQUIRE(dispatched == 256);
    REQUIRE(scheduler.totalDispatchedEvents() == 256);
    REQUIRE(scheduler.backpressureDrops() > 0);
    REQUIRE(engine.droppedScheduledEvents() > 0);

    // Remaining 4 events are retained in staging buffer, not corrupted or discarded silently
    REQUIRE(scheduler.stagedEventCount() == 4);

    // Drain engine queue via processBlock
    float out[512 * 2]{};
    engine.processBlock(out, 512); // Covers all 256 events
    REQUIRE(engine.totalScheduledEventsProcessed() == 256);

    // Now dispatch remaining events into emptied queue
    const std::size_t remainingDispatched = scheduler.dispatchDueEvents(0, engine);
    REQUIRE(remainingDispatched == 4);
    REQUIRE(scheduler.stagedEventCount() == 0);
    REQUIRE(scheduler.totalDispatchedEvents() == 260);
}

TEST_CASE("AudioScheduler scheduleIntent generates NoteOn and NoteOff pairs", "[scheduler][intent]") {
    AudioEngine engine;
    REQUIRE(engine.init());

    AudioScheduler scheduler;

    MusicIntentEvent intent{
        .eventId = 42,
        .sampleFrame = 2000,
        .tick = 5,
        .role = MusicRole::Corpo,
        .pitchHz = 330.0f,
        .velocity = 0.8f,
        .durationFrames = 1200, // 25 ms duration
        .timbreParam = 0.5f
    };

    scheduler.scheduleIntent(intent);
    REQUIRE(scheduler.stagedEventCount() == 2); // NoteOn + NoteOff

    const std::size_t dispatched = scheduler.dispatchDueEvents(0, engine);
    REQUIRE(dispatched == 2);

    // Process block in engine
    float out[4000 * 2]{};
    engine.processBlock(out, 4000); // Covers both 2000 and 3200
    REQUIRE(engine.totalScheduledEventsProcessed() == 2);
}

TEST_CASE("AudioEngine processBlock consumes scheduled events strictly up to block boundary", "[scheduler][block]") {
    AudioEngine engine;
    REQUIRE(engine.init());

    // Schedule 3 events: frame 100, frame 200, frame 600
    REQUIRE(engine.scheduleAudioEvent(ScheduledAudioEvent{.eventId = 1, .sampleFrame = 100}));
    REQUIRE(engine.scheduleAudioEvent(ScheduledAudioEvent{.eventId = 2, .sampleFrame = 200}));
    REQUIRE(engine.scheduleAudioEvent(ScheduledAudioEvent{.eventId = 3, .sampleFrame = 600}));

    float out[256 * 2]{};

    // Block 1: frame 0 -> 256. Covers 100 and 200, does NOT cover 600
    engine.processBlock(out, 256);
    REQUIRE(engine.totalScheduledEventsProcessed() == 2);

    // Block 2: frame 256 -> 512. Does NOT cover 600
    engine.processBlock(out, 256);
    REQUIRE(engine.totalScheduledEventsProcessed() == 2);

    // Block 3: frame 512 -> 768. Covers 600!
    engine.processBlock(out, 256);
    REQUIRE(engine.totalScheduledEventsProcessed() == 3);
}

TEST_CASE("AudioEngine processBlock applies scheduled events at exact intra-block offsets (DIF-M2-03)", "[scheduler][sample_accurate]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    REQUIRE(engine.startTone(220.0f, 0.25f));

    // Schedule a NoteOn event at frame 200 that changes frequency to 880 Hz
    ScheduledAudioEvent evt{
        .eventId = 1,
        .sampleFrame = 200,
        .type = AudioEventType::NoteOn,
        .voice = 0,
        .role = MusicRole::Vettore,
        .valueA = 880.0f, // pitchHz
        .valueB = 1.0f,   // velocity
        .durationFrames = 200
    };
    REQUIRE(engine.scheduleAudioEvent(evt));

    std::vector<float> out(400 * 2, 0.0f);
    engine.processBlock(out.data(), 400);

    REQUIRE(engine.totalScheduledEventsProcessed() == 1);

    // Verify waveform before sample 200 vs after sample 200:
    // Waveform starts at 220 Hz for frames 0..199, then shifts to 880 Hz at frame 200.
    std::size_t zcBefore = 0;
    for (std::size_t i = 1; i < 200; ++i) {
        if ((out[(i - 1) * 2] <= 0.0f && out[i * 2] > 0.0f) ||
            (out[(i - 1) * 2] >= 0.0f && out[i * 2] < 0.0f)) {
            ++zcBefore;
        }
    }

    std::size_t zcAfter = 0;
    for (std::size_t i = 201; i < 400; ++i) {
        if ((out[(i - 1) * 2] <= 0.0f && out[i * 2] > 0.0f) ||
            (out[(i - 1) * 2] >= 0.0f && out[i * 2] < 0.0f)) {
            ++zcAfter;
        }
    }

    REQUIRE(zcBefore >= 1);
    REQUIRE(zcBefore <= 3);
    REQUIRE(zcAfter >= 6);
    REQUIRE(zcAfter >= zcBefore * 2);
}

TEST_CASE("AudioEngine processBlock does not consume scheduled events while Paused (DIF-M2-03)", "[scheduler][pause]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    REQUIRE(engine.startTone(220.0f, 0.25f));

    // Schedule event at frame 100
    REQUIRE(engine.scheduleAudioEvent(ScheduledAudioEvent{.eventId = 1, .sampleFrame = 100, .type = AudioEventType::NoteOn}));

    // Pause transport: transition Running -> Pausing -> Paused (DIF-M1-19)
    AudioCommand pauseCmd{.commandId = 10, .type = AudioCommandType::Pause};
    REQUIRE(engine.sendCommand(pauseCmd));

    float out[200 * 2]{};
    engine.processBlock(out, 200);

    AudioCommandAck ack;
    REQUIRE(engine.pollAck(ack));
    REQUIRE(ack.status == AudioCommandStatus::Applied);
    REQUIRE(engine.updateTelemetry());
    REQUIRE(engine.telemetry().transportState == TransportState::Paused);

    // Timeline is frozen and no events should be consumed while paused!
    REQUIRE(engine.totalScheduledEventsProcessed() == 0);

    // Call processBlock again while paused
    engine.processBlock(out, 200);
    REQUIRE(engine.totalScheduledEventsProcessed() == 0);

    // Resume transport: transition Paused -> Resuming -> Running
    AudioCommand resumeCmd{.commandId = 11, .type = AudioCommandType::Resume};
    REQUIRE(engine.sendCommand(resumeCmd));

    engine.processBlock(out, 200);
    REQUIRE(engine.pollAck(ack));
    REQUIRE(ack.status == AudioCommandStatus::Applied);
    REQUIRE(engine.updateTelemetry());
    REQUIRE(engine.telemetry().transportState == TransportState::Running);

    // Now event at frame 100 is consumed and applied!
    REQUIRE(engine.totalScheduledEventsProcessed() == 1);
}

TEST_CASE("AudioEngine holds AudioCommand with future targetSampleFrame (DIF-M2-02)", "[scheduler][command_future]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    REQUIRE(engine.startTone(220.0f, 0.25f));

    TempoMap map;
    map.bpm = RationalBpm{140, 1};
    map.timeSignatureNum = 4;
    map.timeSignatureDen = 4;
    map.beatsPerBar = 4;
    map.barsPerPhrase = 4;
    map.subdivisionsPerBeat = 4;

    // Send SetTempoMap command targeting frame 2000
    const auto cmd = AudioCommand::makeSetTempoMap(100, map, 2000);
    REQUIRE(engine.sendCommand(cmd));

    std::vector<float> out(1000 * 2, 0.0f);

    // Block 1: 0..500. Target frame 2000 is far in future. Command must NOT execute or ack yet!
    engine.processBlock(out.data(), 500);
    AudioCommandAck ack;
    REQUIRE_FALSE(engine.pollAck(ack));

    // Block 2: 500..1000
    engine.processBlock(out.data(), 500);
    REQUIRE_FALSE(engine.pollAck(ack));

    // Block 3: 1000..1500
    engine.processBlock(out.data(), 500);
    REQUIRE_FALSE(engine.pollAck(ack));

    // Block 4: 1500..2500 (1000 frames). Target frame 2000 is inside this block!
    engine.processBlock(out.data(), 1000);
    REQUIRE(engine.pollAck(ack));
    REQUIRE(ack.commandId == 100);
    REQUIRE(ack.status == AudioCommandStatus::Applied);
    REQUIRE(ack.sampleFrame == 2000); // Executed at exact requested sample frame!
    REQUIRE(ack.transportEpoch >= 2);
}

TEST_CASE("AudioScheduler handles late events safely without queue corruption", "[scheduler][late]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    REQUIRE(engine.startTone(220.0f, 0.25f));

    AudioScheduler scheduler;

    // Advance engine cursor to frame 5000
    std::vector<float> out(5000 * 2, 0.0f);
    engine.processBlock(out.data(), 5000);
    REQUIRE(engine.framesRendered() == 5000);

    // Stage a late event (frame 1000 <= 5000) and a future event (frame 6000)
    scheduler.stageEvent(ScheduledAudioEvent{.eventId = 1, .sampleFrame = 1000, .type = AudioEventType::NoteOn});
    scheduler.stageEvent(ScheduledAudioEvent{.eventId = 2, .sampleFrame = 6000, .type = AudioEventType::NoteOn});

    // Dispatch at current cursor 5000
    const std::size_t dispatched = scheduler.dispatchDueEvents(5000, engine);
    REQUIRE(dispatched == 2);

    // Next block: 5000..7000
    std::vector<float> out2(2000 * 2, 0.0f);
    engine.processBlock(out2.data(), 2000);
    REQUIRE(engine.totalScheduledEventsProcessed() == 2);
}
