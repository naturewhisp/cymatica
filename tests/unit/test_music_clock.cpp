#include <catch2/catch_test_macros.hpp>

#include "music_clock.h"

using namespace cymatica::audio;

TEST_CASE("MusicClock 120 BPM beat boundaries are exact", "[clock][timing]") {
    TempoMap map;
    map.bpm = RationalBpm{120, 1}; // 120 BPM
    auto clockOpt = MusicClock::create(48000, map);
    REQUIRE(clockOpt.has_value());
    auto& clock = *clockOpt;

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
    auto clockOpt = MusicClock::create(48000, map);
    REQUIRE(clockOpt.has_value());
    auto& clock = *clockOpt;

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
    auto clockOpt = MusicClock::create(48000, map);
    REQUIRE(clockOpt.has_value());
    auto& clock = *clockOpt;

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

TEST_CASE("MusicClock setTempoMap and create reject invalid or unsupported configurations explicitly", "[clock][validation]") {
    MusicClock clock;

    // 1. Invalid zero values must fail explicitly per Spec §7.5 / DIF-M1-22
    TempoMap invalidMap;
    invalidMap.bpm = RationalBpm{0, 0};
    invalidMap.beatsPerBar = 0;
    invalidMap.barsPerPhrase = 0;
    invalidMap.subdivisionsPerBeat = 0;
    REQUIRE_FALSE(clock.setTempoMap(invalidMap));
    REQUIRE_FALSE(MusicClock::create(48000, invalidMap).has_value());
    REQUIRE_FALSE(MusicClock::create(0, clock.tempoMap()).has_value());

    // Initial valid defaults are preserved
    const auto& current = clock.tempoMap();
    REQUIRE(current.bpm.numerator == 120);
    REQUIRE(current.bpm.denominator == 1);
    REQUIRE(current.beatsPerBar == 4);
    REQUIRE(current.barsPerPhrase == 4);
    REQUIRE(current.subdivisionsPerBeat == 4);

    // 2. Inconsistent configuration (e.g., 3/8 declared with beatsPerBar=4) must fail explicitly
    TempoMap inconsistentMap;
    inconsistentMap.bpm = RationalBpm{120, 1};
    inconsistentMap.timeSignatureNum = 3;
    inconsistentMap.timeSignatureDen = 8;
    inconsistentMap.beatsPerBar = 4;
    inconsistentMap.barsPerPhrase = 4;
    inconsistentMap.subdivisionsPerBeat = 4;
    REQUIRE_FALSE(clock.setTempoMap(inconsistentMap));
    REQUIRE_FALSE(MusicClock::create(48000, inconsistentMap).has_value());

    // 3. Unsupported non-standard meter configuration (denominator 5) must fail explicitly
    TempoMap unsupportedMap;
    unsupportedMap.bpm = RationalBpm{120, 1};
    unsupportedMap.timeSignatureNum = 3;
    unsupportedMap.timeSignatureDen = 5; // Unsupported non-power-of-two denominator
    unsupportedMap.beatsPerBar = 3;
    unsupportedMap.barsPerPhrase = 4;
    unsupportedMap.subdivisionsPerBeat = 4;
    REQUIRE_FALSE(clock.setTempoMap(unsupportedMap));
    REQUIRE_FALSE(MusicClock::create(48000, unsupportedMap).has_value());

    // 3b. Supported non-4/4 configuration (3/4 time signature) under M2 scope succeeds
    TempoMap meter34Map;
    meter34Map.bpm = RationalBpm{120, 1};
    meter34Map.timeSignatureNum = 3;
    meter34Map.timeSignatureDen = 4;
    meter34Map.beatsPerBar = 3;
    meter34Map.barsPerPhrase = 4;
    meter34Map.subdivisionsPerBeat = 4;
    REQUIRE(clock.setTempoMap(meter34Map));
    REQUIRE(MusicClock::create(48000, meter34Map).has_value());
    REQUIRE(clock.frameAtBar(1) == 72000); // 3 beats at 120 BPM = 1.5s = 72,000 frames

    // 4. Valid supported configuration (4/4, 140 BPM) succeeds
    TempoMap validMap;
    validMap.bpm = RationalBpm{140, 1};
    validMap.timeSignatureNum = 4;
    validMap.timeSignatureDen = 4;
    validMap.beatsPerBar = 4;
    validMap.barsPerPhrase = 4;
    validMap.subdivisionsPerBeat = 4;
    REQUIRE(clock.setTempoMap(validMap));
    REQUIRE(clock.tempoMap().bpm.numerator == 140);

    auto validCreated = MusicClock::create(48000, validMap);
    REQUIRE(validCreated.has_value());
    REQUIRE(validCreated->tempoMap().bpm.numerator == 140);
}

TEST_CASE("MusicPosition derives structured musical coordinates correctly", "[clock][position]") {
    TempoMap map;
    map.bpm = RationalBpm{120, 1};
    map.beatsPerBar = 4;
    map.barsPerPhrase = 4;
    map.subdivisionsPerBeat = 4;
    auto clockOpt = MusicClock::create(48000, map);
    REQUIRE(clockOpt.has_value());
    auto& clock = *clockOpt;

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
    MusicClock clock;

    // 147 device frames at 44.1 kHz = exactly 160 internal frames at 48 kHz
    REQUIRE(clock.deviceToInternalFrames(147, 44100) == 160);
    REQUIRE(clock.internalToDeviceFrames(160, 44100) == 147);

    // 1 second of audio at 44.1 kHz = 44,100 frames -> 48,000 frames
    REQUIRE(clock.deviceToInternalFrames(44100, 44100) == 48000);
    REQUIRE(clock.internalToDeviceFrames(48000, 44100) == 44100);

    // DIF-M1-22: deviceRate == 0 must fail explicitly by returning 0, never identity
    REQUIRE(clock.deviceToInternalFrames(147, 0) == 0);
    REQUIRE(clock.internalToDeviceFrames(160, 0) == 0);
    REQUIRE(clock.deviceToInternalFrames(0, 0) == 0);
    REQUIRE(clock.internalToDeviceFrames(0, 0) == 0);
}

TEST_CASE("MusicClock limits saturate conversions and reject unsupported arithmetic profiles", "[clock][overflow]") {
    MusicClock clock;
    REQUIRE(clock.frameAtBeat(UINT64_MAX) == UINT64_MAX);
    REQUIRE(clock.frameAtBar(UINT64_MAX) == UINT64_MAX);
    REQUIRE(clock.frameAtPhrase(UINT64_MAX) == UINT64_MAX);
    REQUIRE(clock.deviceToInternalFrames(UINT64_MAX, 44100) == UINT64_MAX);
    REQUIRE(clock.internalToDeviceFrames(UINT64_MAX, 44100) == 16'947'946'117'720'650'546ULL);
    const auto pos = clock.positionAtFrame(UINT64_MAX);
    REQUIRE(pos.beat == 0);
    REQUIRE(pos.bar == 1);
    REQUIRE(pos.phrase == UINT32_MAX);
    REQUIRE(pos.beatPhase >= 0.0f);
    REQUIRE(pos.beatPhase < 1.0f);
    REQUIRE_FALSE(MusicClock::create(UINT64_MAX).has_value());
    TempoMap map;
    map.bpm = {UINT32_MAX, 1};
    REQUIRE_FALSE(MusicClock::create(48000, map).has_value());
    REQUIRE_FALSE(clock.setTempoMap(map));
    REQUIRE(clock.frameAtBeat(1) == 24000);
}

TEST_CASE("MusicClock calculates exact metric boundaries for 3/8, 6/8, 5/4, 2/2 (DIF-M2-04)", "[clock][metrics]") {
    // Spec §7.5: BPM measures quarter notes per minute.
    // At 120 BPM, 48 kHz: 1 quarter note = 24,000 frames.

    // 1. Meter 3/8: beat = eighth note (12,000 frames), 3 beats/bar = 36,000 frames/bar
    TempoMap meter38;
    meter38.bpm = RationalBpm{120, 1};
    meter38.timeSignatureNum = 3;
    meter38.timeSignatureDen = 8;
    meter38.beatsPerBar = 3;
    meter38.barsPerPhrase = 4;
    meter38.subdivisionsPerBeat = 2; // 16th notes (2 per 8th note)
    auto clock38 = MusicClock::create(48000, meter38);
    REQUIRE(clock38.has_value());
    REQUIRE(clock38->frameAtBeat(0) == 0);
    REQUIRE(clock38->frameAtBeat(1) == 12000);
    REQUIRE(clock38->frameAtBeat(2) == 24000);
    REQUIRE(clock38->frameAtBeat(3) == 36000);
    REQUIRE(clock38->frameAtBar(1) == 36000);  // Exactly 36,000 frames, NOT 72,000! (DIF-M2-04)
    REQUIRE(clock38->frameAtBar(2) == 72000);
    REQUIRE(clock38->frameAtPhrase(1) == 144000); // 4 bars = 144,000 frames

    const auto pos38_b1 = clock38->positionAtFrame(12000);
    REQUIRE(pos38_b1.beat == 1);
    REQUIRE(pos38_b1.bar == 0);
    REQUIRE(pos38_b1.beatPhase == 0.0f);

    const auto pos38_bar1 = clock38->positionAtFrame(36000);
    REQUIRE(pos38_bar1.beat == 0);
    REQUIRE(pos38_bar1.bar == 1);

    // 2. Meter 6/8: beat = eighth note (12,000 frames), 6 beats/bar = 72,000 frames/bar
    TempoMap meter68;
    meter68.bpm = RationalBpm{120, 1};
    meter68.timeSignatureNum = 6;
    meter68.timeSignatureDen = 8;
    meter68.beatsPerBar = 6;
    meter68.barsPerPhrase = 4;
    meter68.subdivisionsPerBeat = 2;
    auto clock68 = MusicClock::create(48000, meter68);
    REQUIRE(clock68.has_value());
    REQUIRE(clock68->frameAtBeat(1) == 12000);
    REQUIRE(clock68->frameAtBar(1) == 72000); // 6 beats = 72,000 frames
    REQUIRE(clock68->frameAtPhrase(1) == 288000);

    // 3. Meter 5/4: beat = quarter note (24,000 frames), 5 beats/bar = 120,000 frames/bar
    TempoMap meter54;
    meter54.bpm = RationalBpm{120, 1};
    meter54.timeSignatureNum = 5;
    meter54.timeSignatureDen = 4;
    meter54.beatsPerBar = 5;
    meter54.barsPerPhrase = 4;
    meter54.subdivisionsPerBeat = 4;
    auto clock54 = MusicClock::create(48000, meter54);
    REQUIRE(clock54.has_value());
    REQUIRE(clock54->frameAtBeat(1) == 24000);
    REQUIRE(clock54->frameAtBar(1) == 120000); // 5 beats = 120,000 frames

    // 4. Meter 2/2 (alla breve): beat = half note (48,000 frames), 2 beats/bar = 96,000 frames/bar
    TempoMap meter22;
    meter22.bpm = RationalBpm{120, 1};
    meter22.timeSignatureNum = 2;
    meter22.timeSignatureDen = 2;
    meter22.beatsPerBar = 2;
    meter22.barsPerPhrase = 4;
    meter22.subdivisionsPerBeat = 8;
    auto clock22 = MusicClock::create(48000, meter22);
    REQUIRE(clock22.has_value());
    REQUIRE(clock22->frameAtBeat(1) == 48000);
    REQUIRE(clock22->frameAtBar(1) == 96000); // 2 beats = 96,000 frames
}

TEST_CASE("MusicClock continuous tempo transition preserves phase and beat continuity (DIF-M2-02)", "[clock][anchors]") {
    // Start with 120 BPM 4/4 (24,000 frames per beat, 96,000 frames per bar)
    TempoMap map1;
    map1.bpm = RationalBpm{120, 1};
    map1.timeSignatureNum = 4;
    map1.timeSignatureDen = 4;
    map1.beatsPerBar = 4;
    map1.barsPerPhrase = 4;
    map1.subdivisionsPerBeat = 4;

    auto clockOpt = MusicClock::create(48000, map1);
    REQUIRE(clockOpt.has_value());
    auto& clock = *clockOpt;

    // Transition at bar 1 = 96,000 frames to 140 BPM 4/4
    TempoMap map2 = map1;
    map2.bpm = RationalBpm{140, 1}; // 140 BPM

    REQUIRE(clock.setTempoMap(map2, 96000));
    REQUIRE(clock.segmentCount() == 2);

    // Frame 95,999: belongs to segment 1, right before bar 1 boundary
    const auto posBefore = clock.positionAtFrame(95999);
    REQUIRE(posBefore.bar == 0);
    REQUIRE(posBefore.beat == 3);
    REQUIRE(posBefore.beatPhase >= 0.99f);

    // Frame 96,000: exact boundary of segment 2
    const auto posBoundary = clock.positionAtFrame(96000);
    REQUIRE(posBoundary.bar == 1);
    REQUIRE(posBoundary.beat == 0);
    REQUIRE(posBoundary.beatPhase == 0.0f);

    // In segment 2 (140 BPM, 48 kHz):
    // 1 beat = 60 * 48000 / 140 = 2,880,000 / 140 = 144,000 / 7 frames = 20,571 frames (with remainder)
    const std::uint64_t nextBeatFrame = clock.frameAtBeat(5); // beat 4 was at 96,000; beat 5 is 1st beat of new segment
    REQUIRE(nextBeatFrame == 96000 + (144000 / 7));

    const auto posNextBeat = clock.positionAtFrame(nextBeatFrame);
    REQUIRE(posNextBeat.beat == 1);
    REQUIRE(posNextBeat.bar == 1);
    REQUIRE(posNextBeat.beatPhase == 0.0f);
}

TEST_CASE("MusicClock intra-measure tempo transition preserves bar count and beat continuity (DIF-M2-10)", "[clock][anchors][intra_measure]") {
    // Start with 120 BPM 4/4 (24,000 frames per beat, 96,000 frames per bar)
    TempoMap map1;
    map1.bpm = RationalBpm{120, 1};
    map1.timeSignatureNum = 4;
    map1.timeSignatureDen = 4;
    map1.beatsPerBar = 4;
    map1.barsPerPhrase = 4;
    map1.subdivisionsPerBeat = 4;

    auto clockOpt = MusicClock::create(48000, map1);
    REQUIRE(clockOpt.has_value());
    auto& clock = *clockOpt;

    // Transition at beat 3 (intra-measure! frame 72,000) to 140 BPM 4/4
    // Bar 0 has beats 0, 1, 2, 3. Beat 3 is the 4th beat of bar 0 (beat index 3).
    const std::uint64_t transFrame = clock.frameAtBeat(3);
    REQUIRE(transFrame == 72000);

    TempoMap map2 = map1;
    map2.bpm = RationalBpm{140, 1}; // 140 BPM

    REQUIRE(clock.setTempoMap(map2, transFrame));
    REQUIRE(clock.segmentCount() == 2);

    // Frame 71,999: right before transition, in segment 1 (beat 2, bar 0)
    const auto posBefore = clock.positionAtFrame(71999);
    REQUIRE(posBefore.bar == 0);
    REQUIRE(posBefore.beat == 2);
    REQUIRE(posBefore.beatPhase >= 0.99f);

    // Frame 72,000: exact transition boundary (beat 3 of bar 0)
    const auto posBoundary = clock.positionAtFrame(72000);
    REQUIRE(posBoundary.bar == 0);
    REQUIRE(posBoundary.beat == 3);
    REQUIRE(posBoundary.beatPhase == 0.0f);

    // 1 beat elapsed in segment 2:
    // Beat 4 = 72000 + 144000/7. Since beat 3 was the last beat of bar 0, beat 4 MUST be bar 1, beat 0!
    const std::uint64_t beat4Frame = clock.frameAtBeat(4);
    REQUIRE(beat4Frame == 72000 + (144000 / 7));

    const auto posBeat4 = clock.positionAtFrame(beat4Frame);
    REQUIRE(posBeat4.bar == 1);
    REQUIRE(posBeat4.beat == 0);
    REQUIRE(posBeat4.beatPhase == 0.0f);

    // 2 beats elapsed in segment 2:
    // Beat 5 = bar 1, beat 1
    const std::uint64_t beat5Frame = clock.frameAtBeat(5);
    const auto posBeat5 = clock.positionAtFrame(beat5Frame);
    REQUIRE(posBeat5.bar == 1);
    REQUIRE(posBeat5.beat == 1);
    REQUIRE(posBeat5.beatPhase == 0.0f);

    // 5 beats elapsed in segment 2:
    // Beat 8 = bar 2, beat 0
    const std::uint64_t beat8Frame = clock.frameAtBeat(8);
    const auto posBeat8 = clock.positionAtFrame(beat8Frame);
    REQUIRE(posBeat8.bar == 2);
    REQUIRE(posBeat8.beat == 0);
    REQUIRE(posBeat8.beatPhase == 0.0f);

    // frameAtBar continuity:
    REQUIRE(clock.frameAtBar(0) == 0);
    REQUIRE(clock.frameAtBar(1) == beat4Frame);
    REQUIRE(clock.frameAtBar(2) == beat8Frame);
}
