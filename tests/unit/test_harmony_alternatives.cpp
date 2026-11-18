// Pacchetto 6c (D-M2-49): harmony alternatives of section B with the declared leading tone
// (docs/research/M2_5_3_C6c_Harmony_Alternatives_Preregistration.md)
#include <catch2/catch_test_macros.hpp>

#include "harmony_alternatives.h"
#include "music_planner.h"

using namespace cymatica;

TEST_CASE("chordToneMidi alters the third in every octave, and only the third", "[harmony_alternatives]") {
    const audio::ModalScale gAeolian{.rootMidiNote = 55, .mode = audio::ScaleMode::Aeolian};
    const audio::ChordSpec dMinor{4, 6, 1, audio::HarmonicFunction::Tension, false};
    const audio::ChordSpec dMajor{4, 6, 1, audio::HarmonicFunction::Tension, false, 1};
    CHECK(audio::chordToneMidi(gAeolian, dMinor, 6) == 65);       // F4
    CHECK(audio::chordToneMidi(gAeolian, dMajor, 6) == 66);       // F#4
    CHECK(audio::chordToneMidi(gAeolian, dMajor, 6 + 7) == 78);   // F#5
    CHECK(audio::chordToneMidi(gAeolian, dMajor, 6 - 7) == 54);   // F#3
    CHECK(audio::chordToneMidi(gAeolian, dMajor, 4 - 7) == 50);   // D3, root unchanged
    CHECK(audio::chordToneMidi(gAeolian, dMajor, 1) == 57);       // A3, fifth unchanged
    CHECK(audio::naturalThirdPitchClass(gAeolian, dMinor) == -1);
    CHECK(audio::naturalThirdPitchClass(gAeolian, dMajor) == 5);  // F
}

TEST_CASE("The reference keeps the authored harmony of B; the alternatives follow the declared table", "[harmony_alternatives]") {
    CHECK(audio::kFixtureReferenceVariant.bHarmony == 0);
    const auto clock = audio::MusicPlanner::makeFixtureM24V1Clock();
    REQUIRE(clock.has_value());
    const auto roots = [](const audio::PhrasePlan& p) {
        return std::array<int, 4>{p.chords[0].rootDegree, p.chords[1].rootDegree, p.chords[2].rootDegree, p.chords[3].rootDegree};
    };
    const auto alterations = [](const audio::PhrasePlan& p) {
        return std::array<int, 4>{p.chords[0].thirdChromaticOffset, p.chords[1].thirdChromaticOffset,
                                  p.chords[2].thirdChromaticOffset, p.chords[3].thirdChromaticOffset};
    };
    const auto reference = audio::MusicPlanner::makeFixtureM24V1Phrases(*clock);
    for (std::size_t i = 8; i < 16; ++i) {
        CHECK(roots(reference[i]) == std::array<int, 4>{0, 5, 6, 0});
        CHECK(alterations(reference[i]) == std::array<int, 4>{0, 0, 0, 0});
    }
    for (const std::uint8_t alt : {std::uint8_t{1}, std::uint8_t{2}}) {
        audio::FixtureVariant v{};
        v.bHarmony = alt;
        const auto phrases = audio::MusicPlanner::makeFixtureM24V1Phrases(*clock, v);
        const int lt = (alt == 2) ? 1 : 0;
        for (std::size_t i = 8; i < 15; ++i) {
            CHECK(roots(phrases[i]) == std::array<int, 4>{0, 5, 3, 4});
            CHECK(alterations(phrases[i]) == std::array<int, 4>{0, 0, 0, lt});
            CHECK(audio::melodySequenceOffset(phrases[i], 2) == 3);
            CHECK(audio::melodySequenceOffset(phrases[i], 3) == -3);
        }
        CHECK(roots(phrases[15]) == std::array<int, 4>{0, 3, 4, 0});
        CHECK(alterations(phrases[15]) == std::array<int, 4>{0, 0, lt, 0});
        CHECK(phrases[15].chords[3].openPadSustain); // the pad into A' stays G minor
        // A and A' never change
        for (std::size_t i = 0; i < 24; ++i) {
            if (i >= 8 && i < 16) continue;
            CHECK(roots(phrases[i]) == roots(reference[i]));
            CHECK(alterations(phrases[i]) == std::array<int, 4>{0, 0, 0, 0});
        }
    }
}

TEST_CASE("Harmony alternatives: no false relation and the contract of points 1-4 on paired seeds", "[harmony_alternatives]") {
    const auto r = cacp::runHarmonyAlternativesV1("", false, 1, 8);
    REQUIRE(r.status == "MEASURED");
    for (const auto& a : r.alternatives) {
        INFO("alternative " << static_cast<int>(a.alternative));
        CHECK(a.plansOk == 8);
        CHECK(a.falseRelations == 0);                  // H2
        CHECK(a.vettoreCorpoClashes == 0);             // H3
        CHECK(a.changeOnsets > 0);
        CHECK(a.changeChordTones == a.changeOnsets);   // H3
        CHECK(a.registerUnmet == 0);                   // H3
    }
    CHECK(r.alternatives[0].leadingToneOnsets == std::array<std::size_t, 4>{});
    CHECK(r.alternatives[1].leadingToneOnsets == std::array<std::size_t, 4>{});
    CHECK(r.alternatives[2].leadingToneOnsets[2] > 0); // the leading tone sounds in Trama
    // The draw over the seeds is deterministic
    const auto again = cacp::runHarmonyAlternativesV1("", false, 1, 8);
    CHECK(again.drawnCount == r.drawnCount);
    CHECK(again.alternatives[2].adaptedNotes == r.alternatives[2].adaptedNotes);
}
