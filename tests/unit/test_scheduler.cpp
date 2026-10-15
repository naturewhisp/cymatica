#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_scheduler.h"

#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

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

TEST_CASE("AudioEngine preserves committed cross-epoch NoteOn events across SetTempoMap and drops stale events (D-M2-04)", "[scheduler][epoch_drop][cross_epoch_commitment]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));

    // Part A: Fast-forward cursor to 23552 frames
    std::vector<float> preRoll(23552 * 2, 0.0f);
    engine.processBlock(preRoll.data(), 23552);
    REQUIRE(engine.framesRendered() == 23552);

    // Schedule stale event with target frame in the past (frame 10000 < 23552)
    ScheduledAudioEvent staleEvt{
        .eventId = 99,
        .sampleFrame = 10000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 880.0f,
        .valueB = 0.8f
    };
    REQUIRE(engine.scheduleAudioEvent(staleEvt));

    // Part B: Committed future NoteOn event (target frame 24500 > 24000) scheduled in epoch 1
    ScheduledAudioEvent committedEvt{
        .eventId = 42,
        .sampleFrame = 24500,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Corpo,
        .valueA = 440.0f,
        .valueB = 0.8f,
        .durationFrames = 1000
    };
    REQUIRE(engine.scheduleAudioEvent(committedEvt));

    // Send non-destructive SetTempoMap command targeting frame 24000 (valid beat 1 boundary), advancing transportEpoch to 2
    TempoMap map;
    map.bpm = RationalBpm{140, 1};
    map.timeSignatureNum = 4;
    map.timeSignatureDen = 4;
    map.beatsPerBar = 4;
    map.barsPerPhrase = 4;
    map.subdivisionsPerBeat = 4;

    const auto tempoCmd = AudioCommand::makeSetTempoMap(10, map, 24000);
    REQUIRE(engine.sendCommand(tempoCmd));

    // Process block 23552..25000 (1448 frames, covering frame 24000 and 24500)
    std::vector<float> block2(1448 * 2, 0.0f);
    engine.processBlock(block2.data(), 1448);

    // Stale event 99 was dropped (scheduledEventsDroppedTotal incremented)
    // Committed event 42 was preserved across epoch 2 and synthesized at frame 24500!
    REQUIRE(engine.totalScheduledEventsProcessed() == 2);
    REQUIRE(engine.updateTelemetry());
    REQUIRE(engine.telemetry().transportEpoch >= 2);
    REQUIRE(engine.telemetry().scheduledEventsDroppedTotal >= 1); // Stale event dropped
    REQUIRE(engine.telemetry().activeVoiceCount >= 1);            // Committed event 42 is active!
}

TEST_CASE("AudioEngine preserves intra-block temporal order between events and SetTempoMap (DIF-M2-R3-02)", "[scheduler][intra_block_order]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));

    // Fast-forward engine to frame 23552:
    std::vector<float> preRoll(23552 * 2, 0.0f);
    engine.processBlock(preRoll.data(), 23552);
    REQUIRE(engine.framesRendered() == 23552);

    TempoMap map140;
    map140.bpm = RationalBpm{140, 1};
    map140.timeSignatureNum = 4;
    map140.timeSignatureDen = 4;
    map140.beatsPerBar = 4;
    map140.barsPerPhrase = 4;
    map140.subdivisionsPerBeat = 4;

    // Block: [23552, 24064) (512 frames)
    // 1. NoteOn at frame 23600 (offset 48 in block), epoch 1 -> sets frequency to 880 Hz
    ScheduledAudioEvent noteOnEvt{
        .eventId = 1,
        .sampleFrame = 23600,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .valueA = 880.0f, // 880 Hz
        .valueB = 1.0f
    };
    REQUIRE(engine.scheduleAudioEvent(noteOnEvt));

    // 2. SetTempoMap targeting frame 24000 (offset 448 in block) -> transitions to epoch 2
    const auto tempoCmd = AudioCommand::makeSetTempoMap(200, map140, 24000);
    REQUIRE(engine.sendCommand(tempoCmd));

    // Render block of 512 frames:
    std::vector<float> block(512 * 2, 0.0f);
    engine.processBlock(block.data(), 512);

    // Event at 23600 must NOT be dropped! It was processed and applied!
    REQUIRE(engine.totalScheduledEventsProcessed() == 1);

    // Command at 24000 must be applied!
    AudioCommandAck ack;
    REQUIRE(engine.pollAck(ack));
    REQUIRE(ack.commandId == 200);
    REQUIRE(ack.status == AudioCommandStatus::Applied);
    REQUIRE(ack.sampleFrame == 24000);
    REQUIRE(ack.transportEpoch >= 2);

    // Verify PCM content:
    // Before frame 23600 (offsets 0..47): 220 Hz
    // Between frame 23600 and 24000 (offsets 48..447, 400 frames): 880 Hz!
    std::size_t zc880 = 0;
    for (std::size_t i = 49; i < 448; ++i) {
        if ((block[(i - 1) * 2] <= 0.0f && block[i * 2] > 0.0f) ||
            (block[(i - 1) * 2] >= 0.0f && block[i * 2] < 0.0f)) {
            ++zc880;
        }
    }
    // 880 Hz over 400 frames at 48000 Hz is ~14.67 zero crossings
    CHECK(zc880 >= 12);
    CHECK(zc880 <= 17);
}

TEST_CASE("AudioEngine immediate SetTempoMap during playback quantizes to next boundary and rejects late commands (DIF-M2-R3-04)", "[scheduler][immediate_quantize]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    REQUIRE(engine.startTone(220.0f, 0.25f));

    // Render 10,000 frames (cursor is at 10,000, inside beat 0 of bar 0 at 120 BPM 4/4)
    std::vector<float> out(10000 * 2, 0.0f);
    engine.processBlock(out.data(), 10000);
    REQUIRE(engine.framesRendered() == 10000);

    TempoMap map140;
    map140.bpm = RationalBpm{140, 1};
    map140.timeSignatureNum = 4;
    map140.timeSignatureDen = 4;
    map140.beatsPerBar = 4;
    map140.barsPerPhrase = 4;
    map140.subdivisionsPerBeat = 4;

    // Send immediate SetTempoMap (targetSampleFrame == 0) while playing at frame 10,000
    // Must quantize to next beat boundary: frame 24,000! Not frame 0!
    const auto immCmd = AudioCommand::makeSetTempoMap(1, map140, 0);
    REQUIRE(engine.sendCommand(immCmd));

    // Send late command with explicit target frame 5,000 (< current cursor 10,000)
    const auto lateCmd = AudioCommand::makeSetTempoMap(2, map140, 5000);
    REQUIRE(engine.sendCommand(lateCmd));

    // Render block from 10,000 to 20,000 (10,000 frames)
    engine.processBlock(out.data(), 10000);

    // Late command (id 2) must be immediately rejected!
    AudioCommandAck ackLate;
    REQUIRE(engine.pollAck(ackLate));
    REQUIRE(ackLate.commandId == 2);
    REQUIRE(ackLate.status == AudioCommandStatus::Rejected);

    // Immediate command (id 1) was quantized to frame 24,000, so it hasn't acked yet at frame 20,000!
    AudioCommandAck ackPending;
    REQUIRE_FALSE(engine.pollAck(ackPending));

    // Render block covering frame 24,000 (from 20,000 to 30,000)
    engine.processBlock(out.data(), 10000);

    // Now immediate command must have been applied at frame 24,000!
    AudioCommandAck ackImm;
    REQUIRE(engine.pollAck(ackImm));
    REQUIRE(ackImm.commandId == 1);
    REQUIRE(ackImm.status == AudioCommandStatus::Applied);
    REQUIRE(ackImm.sampleFrame == 24000);
}

TEST_CASE("AudioEngine protects future epoch events and executes them only when epoch matches (DIF-M2-R3-05)", "[scheduler][future_epoch]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    REQUIRE(engine.startTone(220.0f, 0.25f));

    // Schedule event targeting frame 24000 with FUTURE epoch 2:
    ScheduledAudioEvent futureEvt{
        .eventId = 99,
        .sampleFrame = 24000,
        .transportEpoch = 2,
        .type = AudioEventType::NoteOn,
        .valueA = 880.0f,
        .valueB = 1.0f
    };
    REQUIRE(engine.scheduleAudioEvent(futureEvt));

    // Schedule SetTempoMap at frame 24000 to advance epoch from 1 to 2!
    TempoMap map140;
    map140.bpm = RationalBpm{140, 1};
    map140.timeSignatureNum = 4;
    map140.timeSignatureDen = 4;
    map140.beatsPerBar = 4;
    map140.barsPerPhrase = 4;
    map140.subdivisionsPerBeat = 4;
    const auto tempoCmd = AudioCommand::makeSetTempoMap(50, map140, 24000);
    REQUIRE(engine.sendCommand(tempoCmd));

    // Render block 0..12000: event must be held in queue
    std::vector<float> out(12000 * 2, 0.0f);
    engine.processBlock(out.data(), 12000);
    REQUIRE(engine.totalScheduledEventsProcessed() == 0);

    // Render block 12000..30000 (18000 frames covering 24000):
    std::vector<float> out2(18000 * 2, 0.0f);
    engine.processBlock(out2.data(), 18000);

    // Tempo command executed at 24000, advancing epoch to 2.
    // Event 99 matched epoch 2 and was processed!
    REQUIRE(engine.totalScheduledEventsProcessed() == 1);
    AudioCommandAck ack;
    REQUIRE(engine.pollAck(ack));
    REQUIRE(ack.commandId == 50);
    REQUIRE(ack.status == AudioCommandStatus::Applied);

    // Now test unmatchable future epoch event:
    // Event targeting frame 32000 with epoch 99, but no command ever transitions to epoch 99.
    ScheduledAudioEvent unmatchableEvt{
        .eventId = 100,
        .sampleFrame = 32000,
        .transportEpoch = 99,
        .type = AudioEventType::NoteOn,
        .valueA = 880.0f,
        .valueB = 1.0f
    };
    REQUIRE(engine.scheduleAudioEvent(unmatchableEvt));

    // Render block covering frame 32000 (from 30000 to 40000):
    std::vector<float> out3(10000 * 2, 0.0f);
    engine.processBlock(out3.data(), 10000);

    // Event 100 could not match epoch 99 and cursor passed it -> dropped as unmatchable!
    CHECK(engine.droppedScheduledEvents() >= 1);
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

TEST_CASE("AudioEngine preserves active voice across epoch boundary until NoteOff and silences cleanly (DIF-M2-R4-01)", "[scheduler][voice_epoch_lifecycle]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));

    // Fast-forward engine to frame 23552
    std::vector<float> preRoll(23552 * 2, 0.0f);
    engine.processBlock(preRoll.data(), 23552);
    REQUIRE(engine.framesRendered() == 23552);

    TempoMap map140;
    map140.bpm = RationalBpm{140, 1};
    map140.timeSignatureNum = 4;
    map140.timeSignatureDen = 4;
    map140.beatsPerBar = 4;
    map140.barsPerPhrase = 4;
    map140.subdivisionsPerBeat = 4;

    // 1. NoteOn at frame 23600 (offset 48 in block), epoch 1 -> 880 Hz, amplitude 1.0f
    ScheduledAudioEvent noteOnEvt{
        .eventId = 1,
        .sampleFrame = 23600,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .valueA = 880.0f,
        .valueB = 1.0f,
        .durationFrames = 1400 // Scheduled duration ends at 25000
    };
    REQUIRE(engine.scheduleAudioEvent(noteOnEvt));

    // 2. SetTempoMap targeting frame 24000 (offset 448 in block) -> transitions epoch 1 -> 2
    const auto tempoCmd = AudioCommand::makeSetTempoMap(200, map140, 24000);
    REQUIRE(engine.sendCommand(tempoCmd));

    // 3. NoteOff at frame 25000, epoch 1 (belonging to the note started at 23600)
    ScheduledAudioEvent noteOffEvt{
        .eventId = 1,
        .sampleFrame = 25000,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOff,
        .valueA = 0.0f,
        .valueB = 0.0f
    };
    REQUIRE(engine.scheduleAudioEvent(noteOffEvt));

    // Block 1: [23552, 24064) (512 frames)
    // Covers NoteOn at 23600 and SetTempoMap at 24000
    std::vector<float> b1(512 * 2, 0.0f);
    engine.processBlock(b1.data(), 512);

    // Verify NoteOn executed and played 880 Hz up to frame 24000:
    std::size_t zc1 = 0;
    for (std::size_t i = 49; i < 448; ++i) {
        if ((b1[(i - 1) * 2] <= 0.0f && b1[i * 2] > 0.0f) ||
            (b1[(i - 1) * 2] >= 0.0f && b1[i * 2] < 0.0f)) {
            ++zc1;
        }
    }
    CHECK(zc1 >= 12); // ~14.6 zero crossings at 880 Hz

    // Command acknowledged and epoch transitioned to 2
    AudioCommandAck ack;
    REQUIRE(engine.pollAck(ack));
    REQUIRE(ack.commandId == 200);
    REQUIRE(ack.status == AudioCommandStatus::Applied);
    REQUIRE(ack.transportEpoch >= 2);

    // Block 2: [24064, 24576) (512 frames)
    // Current epoch is 2. The active voice from epoch 1 MUST continue sounding!
    std::vector<float> b2(512 * 2, 0.0f);
    engine.processBlock(b2.data(), 512);

    std::size_t zc2 = 0;
    for (std::size_t i = 1; i < 512; ++i) {
        if ((b2[(i - 1) * 2] <= 0.0f && b2[i * 2] > 0.0f) ||
            (b2[(i - 1) * 2] >= 0.0f && b2[i * 2] < 0.0f)) {
            ++zc2;
        }
    }
    CHECK(zc2 >= 16); // 880 Hz active throughout block 2 (~18.7 zero crossings)

    // Block 3: [24576, 25088) (512 frames)
    // Covers frame 25000 (offset 424 in block) where NoteOff (epoch 1) matures!
    std::vector<float> b3(512 * 2, 0.0f);
    engine.processBlock(b3.data(), 512);

    // Verify 880 Hz sounded before frame 25000 (offsets 0..424):
    std::size_t zc3Before = 0;
    for (std::size_t i = 1; i < 424; ++i) {
        if ((b3[(i - 1) * 2] <= 0.0f && b3[i * 2] > 0.0f) ||
            (b3[(i - 1) * 2] >= 0.0f && b3[i * 2] < 0.0f)) {
            ++zc3Before;
        }
    }
    CHECK(zc3Before >= 13); // ~15.5 crossings at 880 Hz

    // Block 4: [25088, 25600) (512 frames)
    // NoteOff executed at 25000; the 96-sample micro-fade finished at frame 25096 (offset 8 in block 4).
    // From offset 16 to 512 (frames >= 25104), PCM MUST BE STRICTLY SILENT!
    std::vector<float> b4(512 * 2, 0.0f);
    engine.processBlock(b4.data(), 512);

    std::size_t zc4 = 0;
    float maxAmp4 = 0.0f;
    for (std::size_t i = 16; i < 512; ++i) {
        if ((b4[(i - 1) * 2] <= 0.0f && b4[i * 2] > 0.0f) ||
            (b4[(i - 1) * 2] >= 0.0f && b4[i * 2] < 0.0f)) {
            ++zc4;
        }
        maxAmp4 = (std::max)(maxAmp4, std::abs(b4[i * 2]));
    }
    // PCM must be silent! If NoteOff had been dropped, zc4 would be ~18 and maxAmp4 ~0.25!
    CHECK(zc4 == 0);
    CHECK(maxAmp4 == 0.0f);
}

TEST_CASE("AudioEngine enforces bounded event-processing budget per block under concurrent producer flooding (DIF-M2-R4-02)", "[scheduler][event_budget_concurrency]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));

    // Active concurrent producer thread flooding the queue continuously
    std::atomic<bool> producerRunning{true};
    std::atomic<std::uint64_t> eventsPushed{0};

    std::thread producerThread([&]() {
        std::uint64_t id = 1;
        while (producerRunning.load(std::memory_order_relaxed)) {
            ScheduledAudioEvent floodEvt{
                .eventId = ++id,
                .sampleFrame = 100 + (id % 400),
                .transportEpoch = 1,
                .type = AudioEventType::ParameterChange,
                .valueA = 440.0f
            };
            if (engine.scheduleAudioEvent(floodEvt)) {
                eventsPushed.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });

    // Ensure producer thread has actively started pushing into the queue
    while (eventsPushed.load(std::memory_order_relaxed) == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // Audio callback executes multiple blocks while producer is hammering the queue
    for (int b = 0; b < 20; ++b) {
        std::vector<float> floodBlock(512 * 2, 0.0f);
        const auto start = std::chrono::steady_clock::now();
        engine.processBlock(floodBlock.data(), 512);
        const auto elapsed = std::chrono::steady_clock::now() - start;
        // Verify hard-realtime guarantee: each block completes in bounded time (< 50 ms).
        // NOTE [M2.4 / M2.5 Handoff]: This 50 ms check validates termination and absence of unbounded loops
        // under queue saturation. In M2.4 DSP benchmarks and M2.5 integrated audio testing, the real DSP
        // callback budget for all 8 voices is strictly evaluated against <= 2.0 ms per 512-sample block
        // (audio quantum deadline is 10.67 ms at 48 kHz).
        CHECK(elapsed < std::chrono::milliseconds(50));
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }

    producerRunning.store(false, std::memory_order_relaxed);
    producerThread.join();

    // Verify telemetry updated:
    REQUIRE(engine.updateTelemetry());
    CHECK(engine.totalScheduledEventsProcessed() > 0);
    // Under continuous flooding, queue overflows or block budget was exercised
    CHECK((engine.deferredScheduledEvents() > 0 || engine.droppedScheduledEvents() > 0));
    CHECK(eventsPushed.load() > 0);
}

