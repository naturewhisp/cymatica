#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_probe.h"
#include "music_planner.h"

#include <array>
#include <vector>

using namespace cymatica::audio;

namespace {

constexpr std::uint32_t kSampleRate = 48000;
constexpr std::uint32_t kBlockFrames = 512;
constexpr std::uint32_t kWarmupBlocks = 64;
constexpr std::uint32_t kWorkloadWBlocks = 10000;
constexpr std::size_t kRepetitions = 3;

void runWorkloadWIteration(bool cacpEnabled) {
    AudioEngine engine;
    REQUIRE(engine.initOffline({kSampleRate, 2, 220.0f}));

    AudioProbe probe(AudioProbe::kDefaultCapacityFrames, 2, kSampleRate);
    if (cacpEnabled) {
        engine.attachAudioProbe(&probe);
    }

    engine.sendCommand(AudioCommand{.commandId = 1, .type = AudioCommandType::Start, .targetSampleFrame = 0});

    std::vector<float> blockBuffer(kBlockFrames * 2, 0.0f);
    std::vector<float> probeDrain(kBlockFrames * 2, 0.0f);

    const std::array<MusicRole, 4> roles = {MusicRole::Pulso, MusicRole::Corpo, MusicRole::Trama, MusicRole::Vettore};

    // Initialize 8 voices (2 per role)
    std::uint64_t nextEventId = 1;
    for (int r = 0; r < 4; ++r) {
        for (int v = 0; v < 2; ++v) {
            ScheduledAudioEvent ev{};
            ev.eventId = nextEventId++;
            ev.sampleFrame = 0;
            ev.transportEpoch = 1;
            ev.type = AudioEventType::NoteOn;
            ev.role = roles[static_cast<std::size_t>(r)];
            ev.valueA = 220.0f + static_cast<float>(r * 50 + v * 25);
            ev.valueB = 1.0f; // full velocity to drive limiter into saturation
            ev.timbreParam = 0.5f; // engage DSP filters / synthesis / delays
            ev.durationFrames = 0; // open sustain
            REQUIRE(engine.scheduleAudioEvent(ev));
        }
    }

    // Warm-up blocks
    for (std::uint32_t b = 0; b < kWarmupBlocks; ++b) {
        const std::uint64_t currentFrame = static_cast<std::uint64_t>(b) * kBlockFrames;
        ScheduledAudioEvent ev{};
        ev.eventId = nextEventId++;
        ev.sampleFrame = currentFrame;
        ev.transportEpoch = 1;
        ev.type = AudioEventType::NoteOn;
        ev.role = roles[b % roles.size()];
        ev.valueA = 220.0f + static_cast<float>((b % 4) * 50);
        ev.valueB = 1.0f;
        ev.timbreParam = 0.5f;
        ev.durationFrames = 0;
        REQUIRE(engine.scheduleAudioEvent(ev));

        engine.processBlock(blockBuffer.data(), kBlockFrames);
        if (cacpEnabled) {
            probe.readFrames(probeDrain.data(), kBlockFrames);
        }
    }

    // Discard warm-up measurements
    engine.resetCallbackProfileHistogram();

    // Measured run: 10,000 blocks
    for (std::uint32_t b = 0; b < kWorkloadWBlocks; ++b) {
        const std::uint64_t currentFrame = static_cast<std::uint64_t>(kWarmupBlocks + b) * kBlockFrames;
        ScheduledAudioEvent ev{};
        ev.eventId = nextEventId++;
        ev.sampleFrame = currentFrame;
        ev.transportEpoch = 1;
        ev.type = AudioEventType::NoteOn;
        ev.role = roles[b % roles.size()];
        ev.valueA = 220.0f + static_cast<float>((b % 4) * 50);
        ev.valueB = 1.0f;
        ev.timbreParam = 0.5f;
        ev.durationFrames = 0;
        REQUIRE(engine.scheduleAudioEvent(ev));

        engine.processBlock(blockBuffer.data(), kBlockFrames);
        if (cacpEnabled) {
            probe.readFrames(probeDrain.data(), kBlockFrames);
        }

        REQUIRE(engine.updateTelemetry());
        // Assert all 8 voice slots remain active on every block (INC4-R07)
        REQUIRE(engine.telemetry().activeVoiceCount == 8);
    }

    const auto hist = engine.callbackProfileHistogram();
    CHECK(hist.totalBlocks == kWorkloadWBlocks);

    const double p999Ms = hist.percentileMs(99.9);
    const double p50Ms = hist.percentileMs(50.0);
    const double maxMs = hist.maxMs();

    INFO("Workload W (CACP=" << (cacpEnabled ? "ON" : "OFF")
         << ") p50=" << p50Ms << "ms, p99.9=" << p999Ms << "ms, max=" << maxMs
         << "ms, overruns=" << hist.blocksExceedingBlockPeriod);

#ifdef NDEBUG
    // Release mode hard realtime gate: p99.9 <= 2.0 ms, zero blocks > block period (10.667 ms)
    CHECK(p999Ms <= 2.0);
    CHECK(hist.blocksExceedingBlockPeriod == 0);
#endif

    if (cacpEnabled) {
        engine.detachAudioProbe();
    }
    engine.shutdown();
}

void runWorkloadFIteration(bool cacpEnabled) {
    const auto optClock = MusicPlanner::makeFixtureM24V1Clock();
    REQUIRE(optClock.has_value());
    const MusicClock& clock = *optClock;
    const FormPlan form = MusicPlanner::makeFixtureM24V1Form();
    const auto phrases = MusicPlanner::makeFixtureM24V1Phrases(clock);

    AudioEngine engine;
    REQUIRE(engine.initOffline({kSampleRate, 2, 220.0f}));

    AudioProbe probe(AudioProbe::kDefaultCapacityFrames, 2, kSampleRate);
    if (cacpEnabled) {
        engine.attachAudioProbe(&probe);
    }

    const std::uint64_t contentEndFrame = clock.frameAtBar(96);
    constexpr std::uint64_t kPostStopFrames = 48000;
    const std::uint64_t totalFrames = contentEndFrame + kPostStopFrames;
    const std::uint64_t totalBlocks = (totalFrames + kBlockFrames - 1) / kBlockFrames;

    MusicPlanner planner({.runSeed = 1337ULL, .candidateCount = 8, .sampleRate = kSampleRate, .lookaheadFrames = AudioSchedulerConfig{}.lookaheadFrames});
    planner.setFormPlan(form);
    AudioScheduler scheduler;
    std::size_t nextPhraseToPlan = 0;

    auto planNextPhrase = [&](std::uint64_t horizon) {
        if (nextPhraseToPlan >= phrases.size()) return;
        const auto& p = phrases[nextPhraseToPlan];
        std::uint64_t epoch = 1;
        if (p.sectionIndex == 1) epoch = 2;
        else if (p.sectionIndex == 2) epoch = 3;
        std::vector<MusicIntentEvent> out;
        planner.planPhrase(p, clock, horizon, epoch, out, &scheduler);
        ++nextPhraseToPlan;
    };

    planNextPhrase(0);
    planNextPhrase(0);

    engine.sendCommand(AudioCommand{.commandId = 1, .type = AudioCommandType::Start, .targetSampleFrame = 0});

    const std::uint64_t tempoChangeFrameB = 3072000ULL;
    const std::uint64_t tempoChangeFrameA1 = 5705142ULL;
    const std::uint64_t bar31Frame = clock.frameAtBar(31);
    const std::uint64_t bar63Frame = clock.frameAtBar(63);
    const std::uint64_t bar95Frame = clock.frameAtBar(95);
    const std::uint64_t stopFrame = contentEndFrame;

    bool sentTempoB = false;
    bool sentTempoA1 = false;
    bool sentStop = false;

    std::vector<float> blockBuffer(kBlockFrames * 2, 0.0f);
    std::vector<float> probeDrain(kBlockFrames * 2, 0.0f);

    // Warm-up 64 blocks
    for (std::uint32_t b = 0; b < kWarmupBlocks && b < totalBlocks; ++b) {
        const std::uint64_t currentFrame = static_cast<std::uint64_t>(b) * kBlockFrames;
        scheduler.dispatchDueEvents(currentFrame, engine);
        engine.processBlock(blockBuffer.data(), kBlockFrames);
        if (cacpEnabled) {
            probe.readFrames(probeDrain.data(), kBlockFrames);
        }
    }

    engine.resetCallbackProfileHistogram();

    for (std::uint64_t b = kWarmupBlocks; b < totalBlocks; ++b) {
        const std::uint64_t currentFrame = b * kBlockFrames;

        if (!sentTempoB && currentFrame >= bar31Frame) {
            engine.sendCommand(AudioCommand::makeSetTempoMap(2, form.sections[1].tempoMap, tempoChangeFrameB));
            sentTempoB = true;
        }
        if (!sentTempoA1 && currentFrame >= bar63Frame) {
            engine.sendCommand(AudioCommand::makeSetTempoMap(3, form.sections[2].tempoMap, tempoChangeFrameA1));
            sentTempoA1 = true;
        }
        if (!sentStop && currentFrame >= bar95Frame) {
            engine.sendCommand(AudioCommand{.commandId = 4, .type = AudioCommandType::Stop, .targetSampleFrame = stopFrame});
            sentStop = true;
        }

        while (nextPhraseToPlan < phrases.size() &&
               (nextPhraseToPlan <= 1 || currentFrame >= phrases[nextPhraseToPlan - 1].startFrame)) {
            const std::uint64_t horizon = currentFrame + scheduler.lookaheadFrames();
            planNextPhrase(horizon);
        }

        scheduler.dispatchDueEvents(currentFrame, engine);
        engine.processBlock(blockBuffer.data(), kBlockFrames);
        if (cacpEnabled) {
            probe.readFrames(probeDrain.data(), kBlockFrames);
        }

        AudioCommandAck ack{};
        while (engine.pollAck(ack)) {}
    }

    const auto hist = engine.callbackProfileHistogram();
    const double p999Ms = hist.percentileMs(99.9);
    const double p50Ms = hist.percentileMs(50.0);
    const double maxMs = hist.maxMs();

    INFO("Workload F (CACP=" << (cacpEnabled ? "ON" : "OFF")
         << ") p50=" << p50Ms << "ms, p99.9=" << p999Ms << "ms, max=" << maxMs
         << "ms, overruns=" << hist.blocksExceedingBlockPeriod);

#ifdef NDEBUG
    CHECK(p999Ms <= 2.0);
    CHECK(hist.blocksExceedingBlockPeriod == 0);
#endif

    if (cacpEnabled) {
        engine.detachAudioProbe();
    }
    engine.shutdown();
}

} // namespace

TEST_CASE("Realtime Profile: Workload W 10000 blocks stress", "[perf][realtime_profile][workload_w]") {
    SECTION("CACP OFF") {
        for (std::size_t rep = 0; rep < kRepetitions; ++rep) {
            runWorkloadWIteration(false);
        }
    }
    SECTION("CACP ON") {
        for (std::size_t rep = 0; rep < kRepetitions; ++rep) {
            runWorkloadWIteration(true);
        }
    }
}

TEST_CASE("Realtime Profile: Workload F full 96-bar fixture", "[perf][realtime_profile][workload_f]") {
    SECTION("CACP OFF") {
        for (std::size_t rep = 0; rep < kRepetitions; ++rep) {
            runWorkloadFIteration(false);
        }
    }
    SECTION("CACP ON") {
        for (std::size_t rep = 0; rep < kRepetitions; ++rep) {
            runWorkloadFIteration(true);
        }
    }
}

TEST_CASE("CallbackProfileHistogram calculation and percentiles (DIF-INC4-01)", "[audio_engine][profile_histogram]") {
    CallbackProfileHistogram hist{};
    hist.reset();
    REQUIRE(hist.totalBlocks == 0);
    CHECK(hist.percentileMs(50.0) == 0.0);
    CHECK(hist.meanMs() == 0.0);
    CHECK(hist.maxMs() == 0.0);

    // 500 blocks of 99 us -> bin 9 (upper bound 100 us = 0.10 ms)
    for (int i = 0; i < 500; ++i) hist.record(99000ULL, 512, 48000);
    // 400 blocks of 199 us -> bin 19 (upper bound 200 us = 0.20 ms)
    for (int i = 0; i < 400; ++i) hist.record(199000ULL, 512, 48000);
    // 90 blocks of 499 us -> bin 49 (upper bound 500 us = 0.50 ms)
    for (int i = 0; i < 90; ++i) hist.record(499000ULL, 512, 48000);
    // 9 blocks of 999 us -> bin 99 (upper bound 1000 us = 1.00 ms)
    for (int i = 0; i < 9; ++i) hist.record(999000ULL, 512, 48000);
    // 1 block of 3000 us (3.00 ms)
    hist.record(3000000ULL, 512, 48000);

    CHECK(hist.totalBlocks == 1000);
    CHECK(hist.maxMs() == 3.0);
    CHECK(hist.blocksExceeding2Ms == 1);
    CHECK(hist.blocksExceedingBlockPeriod == 0);

    CHECK(hist.percentileMs(50.0) == 0.10);
    CHECK(hist.percentileMs(0.50) == 0.10);
    CHECK(hist.percentileMs(90.0) == 0.20);
    CHECK(hist.percentileMs(99.0) == 0.50);
    CHECK(hist.percentileMs(99.9) == 1.00);
    CHECK(hist.percentileMs(100.0) == 3.00);

    CHECK(hist.percentileMs(50.0) <= hist.percentileMs(90.0));
    CHECK(hist.percentileMs(90.0) <= hist.percentileMs(99.0));
    CHECK(hist.percentileMs(99.0) <= hist.percentileMs(99.9));
    CHECK(hist.percentileMs(99.9) <= hist.maxMs());
}
