#include <catch2/catch_test_macros.hpp>

#include "music_clock.h"

using namespace cymatica::audio;

TEST_CASE("MusicClock 120 BPM beat boundaries are exact", "[clock][timing]") {
    TempoMap map;
    map.bpm = RationalBpm{120, 1}; // 120 BPM
    MusicClock clock(48000, map);

    REQUIRE(clock.frameAtBeat(0) == 0);
    REQUIRE(clock.frameAtBeat(1) == 24000); // 0.5s at 48kHz
    REQUIRE(clock.frameAtBeat(2) == 48000); // 1.0s
    REQUIRE(clock.frameAtBar(1) == 96000);  // 4 beats = 2.0s
    REQUIRE(clock.frameAtPhrase(1) == 384000); // 4 bars = 8.0s
}

TEST_CASE("MusicClock handles non-integer BPM over 1 virtual hour with zero drift", "[clock][timing]") {
    // 127.5 BPM: 1275 / 10
    TempoMap map;
    map.bpm = RationalBpm{1275, 10};
    MusicClock clock(48000, map);

    // In 1 hour (3600 seconds), there are 3600 * 127.5 / 60 = 7650 beats.
    constexpr std::uint64_t kOneHourFrames = 48000ULL * 3600ULL; // 172,800,000 frames
    const std::uint64_t frameAt1Hour = clock.frameAtBeat(7650);

    REQUIRE(frameAt1Hour == kOneHourFrames); // EXACT zero drift on 1 hour!
}

TEST_CASE("MusicClock 127.5 BPM beat boundaries are exact for b in [0, 68]", "[clock][timing][boundary]") {
    // 127.5 BPM: 255 / 2 (or 1275 / 10)
    TempoMap map;
    map.bpm = RationalBpm{255, 2};
    map.beatsPerBar = 4;
    map.barsPerPhrase = 4;
    map.subdivisionsPerBeat = 4;
    MusicClock clock(48000, map);

    for (std::uint64_t b = 0; b <= 68; ++b) {
        const std::uint64_t boundaryFrame = clock.frameAtBeat(b);
        const auto pos = clock.positionAtFrame(boundaryFrame);

        const std::uint64_t expectedTotalBeats = b;
        const std::uint32_t expectedBeatInBar = static_cast<std::uint32_t>(expectedTotalBeats % 4);
        const std::uint64_t totalBars = expectedTotalBeats / 4;
        const std::uint32_t expectedBarInPhrase = static_cast<std::uint32_t>(totalBars % 4);
        const std::uint32_t expectedPhrase = static_cast<std::uint32_t>(totalBars / 4);

        REQUIRE(pos.sampleFrame == boundaryFrame);
        REQUIRE(pos.phrase == expectedPhrase);
        REQUIRE(pos.bar == expectedBarInPhrase);
        REQUIRE(pos.beat == expectedBeatInBar);
        REQUIRE(pos.subdivision == 0);
        REQUIRE(pos.beatPhase == 0.0f);

        // Frame immediately prior to boundary belongs to preceding beat (b - 1)
        if (b > 0) {
            const auto prevPos = clock.positionAtFrame(boundaryFrame - 1);
            const std::uint64_t prevTotalBeats = b - 1;
            const std::uint32_t prevBeatInBar = static_cast<std::uint32_t>(prevTotalBeats % 4);
            const std::uint64_t prevTotalBars = prevTotalBeats / 4;
            const std::uint32_t prevBarInPhrase = static_cast<std::uint32_t>(prevTotalBars % 4);
            const std::uint32_t prevPhrase = static_cast<std::uint32_t>(prevTotalBars / 4);

            REQUIRE(prevPos.phrase == prevPhrase);
            REQUIRE(prevPos.bar == prevBarInPhrase);
            REQUIRE(prevPos.beat == prevBeatInBar);
            REQUIRE(prevPos.subdivision == 3);
            REQUIRE(prevPos.beatPhase < 1.0f);
            REQUIRE(prevPos.beatPhase >= 0.99f);
        }
    }
}

TEST_CASE("MusicClock setTempoMap validates defensive defaults on zero values", "[clock][validation]") {
    MusicClock clock(48000);

    TempoMap invalidMap;
    invalidMap.bpm = RationalBpm{0, 0};
    invalidMap.beatsPerBar = 0;
    invalidMap.barsPerPhrase = 0;
    invalidMap.subdivisionsPerBeat = 0;

    clock.setTempoMap(invalidMap);

    const auto& sanitized = clock.tempoMap();
    REQUIRE(sanitized.bpm.numerator == 120);
    REQUIRE(sanitized.bpm.denominator == 1);
    REQUIRE(sanitized.beatsPerBar == 4);
    REQUIRE(sanitized.barsPerPhrase == 4);
    REQUIRE(sanitized.subdivisionsPerBeat == 4);

    // positionAtFrame still executes deterministically without division by zero
    const auto pos = clock.positionAtFrame(24000);
    REQUIRE(pos.beat == 1);
}

TEST_CASE("MusicPosition derives structured musical coordinates correctly", "[clock][position]") {
    TempoMap map;
    map.bpm = RationalBpm{120, 1};
    map.beatsPerBar = 4;
    map.barsPerPhrase = 4;
    map.subdivisionsPerBeat = 4;
    MusicClock clock(48000, map);

    // Frame 0: phrase 0, bar 0, beat 0, sub 0, phase 0.0
    const auto pos0 = clock.positionAtFrame(0, 42);
    REQUIRE(pos0.sampleFrame == 0);
    REQUIRE(pos0.transportEpoch == 42);
    REQUIRE(pos0.phrase == 0);
    REQUIRE(pos0.bar == 0);
    REQUIRE(pos0.beat == 0);
    REQUIRE(pos0.subdivision == 0);
    REQUIRE(pos0.beatPhase == 0.0f);

    // Frame 12,000: midway through beat 0 (beatPhase = 0.5, subdivision 2)
    const auto posMid = clock.positionAtFrame(12000, 42);
    REQUIRE(posMid.beat == 0);
    REQUIRE(posMid.subdivision == 2);
    REQUIRE(posMid.beatPhase >= 0.49f);
    REQUIRE(posMid.beatPhase <= 0.51f);

    // Frame 96,000: start of bar 1 (beat 0 of bar 1)
    const auto posBar1 = clock.positionAtFrame(96000, 42);
    REQUIRE(posBar1.phrase == 0);
    REQUIRE(posBar1.bar == 1);
    REQUIRE(posBar1.beat == 0);
}

TEST_CASE("Sample rate conversion 44.1 kHz <-> 48 kHz uses exact rational 160/147 factor", "[clock][resample]") {
    MusicClock clock(48000);

    // 147 device frames at 44.1 kHz = exactly 160 internal frames at 48 kHz
    REQUIRE(clock.deviceToInternalFrames(147, 44100) == 160);
    REQUIRE(clock.internalToDeviceFrames(160, 44100) == 147);

    // 1 second of audio at 44.1 kHz = 44,100 frames -> 48,000 frames
    REQUIRE(clock.deviceToInternalFrames(44100, 44100) == 48000);
    REQUIRE(clock.internalToDeviceFrames(48000, 44100) == 44100);
}
