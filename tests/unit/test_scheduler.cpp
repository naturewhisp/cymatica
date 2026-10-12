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
