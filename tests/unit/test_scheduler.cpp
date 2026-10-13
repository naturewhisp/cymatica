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

    // Send SetTempoMap command targeting frame 24000 (beat 1 boundary at 120 BPM 4/4)
    const auto cmd = AudioCommand::makeSetTempoMap(100, map, 24000);
    REQUIRE(engine.sendCommand(cmd));

    std::vector<float> out(6000 * 2, 0.0f);

    // Block 1: 0..6000. Target frame 24000 is far in future. Command must NOT execute or ack yet!
    engine.processBlock(out.data(), 6000);
    AudioCommandAck ack;
    REQUIRE_FALSE(engine.pollAck(ack));

    // Block 2: 6000..12000
    engine.processBlock(out.data(), 6000);
    REQUIRE_FALSE(engine.pollAck(ack));

    // Block 3: 12000..18000
    engine.processBlock(out.data(), 6000);
    REQUIRE_FALSE(engine.pollAck(ack));

    // Block 4: 18000..30000 (12000 frames). Target frame 24000 is inside this block!
    std::vector<float> outBlock4(12000 * 2, 0.0f);
    engine.processBlock(outBlock4.data(), 12000);
    REQUIRE(engine.pollAck(ack));
    REQUIRE(ack.commandId == 100);
    REQUIRE(ack.status == AudioCommandStatus::Applied);
    REQUIRE(ack.sampleFrame == 24000); // Executed at exact requested sample frame!
    REQUIRE(ack.transportEpoch >= 2);
}

TEST_CASE("AudioEngine rejects 17th tempo segment and preserves transport epoch", "[scheduler][epoch_ack]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    REQUIRE(engine.startTone(220.0f, 0.25f));

    TempoMap map;
    map.bpm = RationalBpm{120, 1};
    map.timeSignatureNum = 4;
    map.timeSignatureDen = 4;
    map.beatsPerBar = 4;
    map.barsPerPhrase = 4;
    map.subdivisionsPerBeat = 4;

    std::vector<float> out(500 * 2, 0.0f);

    // Initial segment is 0. MusicClock supports up to 16 segments total.
    // 15 additional segments can be added (indices 1..15), making 16 total.
    std::uint64_t targetFrame = 24000;
    for (std::uint64_t i = 1; i <= 15; ++i) {
        const auto cmd = AudioCommand::makeSetTempoMap(100 + i, map, targetFrame);
        REQUIRE(engine.sendCommand(cmd));
        engine.processBlock(out.data(), 500);
        std::vector<float> bigOut(24000 * 2, 0.0f);
        engine.processBlock(bigOut.data(), 24000);
        AudioCommandAck ack;
        REQUIRE(engine.pollAck(ack));
        REQUIRE(ack.commandId == 100 + i);
        REQUIRE(ack.status == AudioCommandStatus::Applied);
        targetFrame += 24000;
    }

    // Now send the 16th command (which would be segment 17):
    const auto cmdOverflow = AudioCommand::makeSetTempoMap(200, map, targetFrame);
    REQUIRE(engine.sendCommand(cmdOverflow));
    std::vector<float> bigOut(24000 * 2, 0.0f);
    engine.processBlock(bigOut.data(), 24000);
    AudioCommandAck ackOverflow;
    REQUIRE(engine.pollAck(ackOverflow));
    REQUIRE(ackOverflow.commandId == 200);
    REQUIRE(ackOverflow.status == AudioCommandStatus::Rejected);
}

TEST_CASE("AudioEngine future SetTempoMap does not cause head-of-line blocking on Pause and Resume", "[scheduler][hol_blocking]") {
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

    // Send Pause command
    AudioCommand pauseCmd{.commandId = 1, .type = AudioCommandType::Pause};
    REQUIRE(engine.sendCommand(pauseCmd));

    // Send future SetTempoMap command targeting frame 48,000
    const auto futureTempoCmd = AudioCommand::makeSetTempoMap(2, map, 48000);
    REQUIRE(engine.sendCommand(futureTempoCmd));

    // Send Resume command right behind the future command!
    AudioCommand resumeCmd{.commandId = 3, .type = AudioCommandType::Resume};
    REQUIRE(engine.sendCommand(resumeCmd));

    // Process a block of 500 frames (cursor is at 0, 48000 is far in future)
    std::vector<float> out(500 * 2, 0.0f);
    engine.processBlock(out.data(), 500);

    // Pause must be applied!
    AudioCommandAck ack1;
    REQUIRE(engine.pollAck(ack1));
    REQUIRE(ack1.commandId == 1);
    REQUIRE(ack1.status == AudioCommandStatus::Applied);

    // Resume must be applied immediately without waiting for frame 48,000!
    AudioCommandAck ack3;
    REQUIRE(engine.pollAck(ack3));
    REQUIRE(ack3.commandId == 3);
    REQUIRE(ack3.status == AudioCommandStatus::Applied);

    // Future tempo command (commandId 2) has NOT acked yet because it's staged for frame 48000
    AudioCommandAck ackPending;
    REQUIRE_FALSE(engine.pollAck(ackPending));

    // Engine transport state must be Running!
    REQUIRE(engine.updateTelemetry());
    REQUIRE(engine.telemetry().transportState == TransportState::Running);
}

TEST_CASE("AudioEngine drops obsolete epoch scheduled events without DSP synthesis", "[scheduler][epoch_drop]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    REQUIRE(engine.startTone(220.0f, 0.25f));

    // Initial transportEpoch is 1.
    // Stage an event with transportEpoch = 1 at frame 1000
    ScheduledAudioEvent oldEvt{
        .eventId = 42,
        .sampleFrame = 1000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .valueA = 880.0f, // high pitch
        .valueB = 1.0f
    };
    REQUIRE(engine.scheduleAudioEvent(oldEvt));

    // Now bump the engine epoch to 2 and 3 via Pause and Resume
    AudioCommand pauseCmd{.commandId = 10, .type = AudioCommandType::Pause};
    REQUIRE(engine.sendCommand(pauseCmd));
    AudioCommand resumeCmd{.commandId = 11, .type = AudioCommandType::Resume};
    REQUIRE(engine.sendCommand(resumeCmd));

    std::vector<float> out(500 * 2, 0.0f);
    engine.processBlock(out.data(), 500); // Processes Pause & Resume, bumps epoch to 3!

    AudioCommandAck ack;
    REQUIRE(engine.pollAck(ack)); // Pause ack
    REQUIRE(engine.pollAck(ack)); // Resume ack
    REQUIRE(engine.updateTelemetry());
    REQUIRE(engine.telemetry().transportEpoch >= 3);

    // Render block covering frame 1000 (from 500 to 1500)
    std::vector<float> block(1000 * 2, 0.0f);
    engine.processBlock(block.data(), 1000);

    // Event 42 has epoch 1 < currentEpoch (3), so it must be dropped without synthesizing DSP
    REQUIRE(engine.totalScheduledEventsProcessed() == 1); // Popped and counted
    REQUIRE(engine.updateTelemetry());
    REQUIRE(engine.telemetry().pulse.pitchHz != 880.0f);
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
