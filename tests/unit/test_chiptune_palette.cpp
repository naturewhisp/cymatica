#include "acao02_calibration.h"
#include "audio_engine.h"
#include "release_ab.h"
#include "stem_analysis.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace cymatica::audio;
using namespace cymatica::cacp;

// Spike R-M25-9 (docs/research/M2_5_Chiptune_Palette_Spike_Preregistration.md), criterion X2
TEST_CASE("Chiptune palette: offline only, explicit, and the production path is unchanged", "[chiptune_palette]") {
    // Offline rule shared with the other offline overrides: a started device engine refuses it
    STATIC_REQUIRE_FALSE(AudioEngine::noteReleaseOverrideAllowed(true, true));
    STATIC_REQUIRE_FALSE(AudioEngine::noteReleaseOverrideAllowed(false, true));
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    REQUIRE(engine.setTimbrePalette(TimbrePalette::Chiptune8Bit));
    REQUIRE(engine.setTimbrePalette(TimbrePalette::Production));
    REQUIRE_FALSE(engine.setTimbrePalette(static_cast<TimbrePalette>(7)));

    // The same notes: an explicit Production palette renders exactly the default; the chiptune palette differs and
    // stays finite, in every role
    const std::vector<SynthNote> notes{{0.2, 220.0f, 0.8f, 0.3, 0.5f}, {0.8, 110.0f, 0.8f, 0.3, 0.5f}, {1.4, 440.0f, 0.8f, 0.3, 0.5f}};
    for (const MusicRole role : {MusicRole::Pulso, MusicRole::Corpo, MusicRole::Trama, MusicRole::Vettore}) {
        INFO("role " << static_cast<int>(role));
        StemCapture capDefault(48000 * 3), capProd(48000 * 3), capChip(48000 * 3);
        (void)renderSyntheticWithRelease(role, notes, capDefault, 0);
        (void)renderSyntheticWithRelease(role, notes, capProd, 0, TimbrePalette::Production);
        (void)renderSyntheticWithRelease(role, notes, capChip, 0, TimbrePalette::Chiptune8Bit);
        const auto r = static_cast<std::size_t>(role);
        const auto d = capDefault.stem(r), p = capProd.stem(r), c = capChip.stem(r);
        REQUIRE(d.size() == p.size());
        CHECK(std::equal(d.begin(), d.end(), p.begin()));
        REQUIRE(c.size() == d.size());
        CHECK_FALSE(std::equal(d.begin(), d.end(), c.begin()));
        CHECK(std::all_of(c.begin(), c.end(), [](float x) { return std::isfinite(x); }));
        CHECK(std::any_of(c.begin(), c.end(), [](float x) { return x != 0.0f; }));
    }
}
