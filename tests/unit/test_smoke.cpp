#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "fixed_step.h"

#include <chrono>
#include <cstdint>
#include <thread>

TEST_CASE("Tick boundary formula is exact in integer arithmetic", "[timing]") {
    cymatica::core::FixedStepAccumulator accumulator({48000, 120, 4, 16});

    REQUIRE(accumulator.tickToSampleFrame(0) == 0);
    REQUIRE(accumulator.tickToSampleFrame(1) == 400);
    REQUIRE(accumulator.tickToSampleFrame(120) == 48000);
    REQUIRE(accumulator.tickToSampleFrame(120ULL * 3600ULL) == 48000ULL * 3600ULL); // one virtual hour, no drift
}

TEST_CASE("Audio engine is inert before init", "[audio]") {
    cymatica::audio::AudioEngine engine;
    REQUIRE_FALSE(engine.isRunning());
    REQUIRE(engine.framesRendered() == 0);
    REQUIRE(engine.sampleRate() == 0);
    REQUIRE_FALSE(engine.startTone(220.0f, 0.0f));
    engine.stopTone(); // no-op, must not crash
    engine.shutdown(); // no-op, must not crash
}

// Requires a real playback device; excluded from headless CI via the [device] tag.
TEST_CASE("Audio engine renders frames on the default device", "[audio][device]") {
    cymatica::audio::AudioEngine engine;
    REQUIRE(engine.init({48000, 2, 220.0f}));
    REQUIRE(engine.sampleRate() > 0);
    REQUIRE(engine.startTone(220.0f, 0.0f)); // silent
    REQUIRE(engine.isRunning());

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (engine.framesRendered() == 0 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    REQUIRE(engine.framesRendered() > 0);

    engine.setToneFrequency(440.0f);
    engine.stopTone();
    REQUIRE_FALSE(engine.isRunning());
    engine.shutdown();
    REQUIRE(engine.sampleRate() == 0);
}
