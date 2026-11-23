// CP-3 I1-I2 (D-M2-51): offline kick overrides, bounds and golden applicability
#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_probe.h"

#include <limits>

using namespace cymatica::audio;

TEST_CASE("Kick overrides are bounded and offline only", "[kick_overrides]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    CHECK(engine.setPulsoFixedMidi(24));
    CHECK(engine.setPulsoFixedMidi(48));
    CHECK_FALSE(engine.setPulsoFixedMidi(23));
    CHECK_FALSE(engine.setPulsoFixedMidi(49));
    CHECK(engine.setPulsoClickScale(2.0f));
    CHECK_FALSE(engine.setPulsoClickScale(0.0f));
    CHECK_FALSE(engine.setPulsoClickScale(std::numeric_limits<float>::quiet_NaN()));
}

TEST_CASE("A kick override never claims the PCM golden", "[kick_overrides]") {
    FixtureProbeResult r{};
    r.goldenApplicable = true;
    r.pcmContentHash = 42;
    CHECK(fixturePcmGoldenStatus(r, 42) == "PASS");
    r.pulsoFixedMidiOverride = 36;
    CHECK(fixturePcmGoldenStatus(r, 42) == "NOT_APPLICABLE");
    r.pulsoFixedMidiOverride = 0;
    r.pulsoClickScaleOverride = 2.0f;
    CHECK(fixturePcmGoldenStatus(r, 42) == "NOT_APPLICABLE");
}
