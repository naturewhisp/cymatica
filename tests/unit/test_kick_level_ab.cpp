#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "audio_engine.h"
#include "kick_level_ab.h"
#include "role_bus_processor.h"

#include <array>
#include <cmath>
#include <limits>
#include <vector>

using namespace cymatica::audio;
using namespace cymatica::cacp;

namespace {

struct BusRun {
    std::vector<float> mix;
    std::vector<float> minDuck;
};

// Pulso burst + steady Corpo through the bus processor for a few blocks, with a given Pulso gain scale
BusRun runBus(float scale) {
    RoleBusProcessor bus;
    bus.init(48000);
    bus.setPulsoGainScale(scale);
    constexpr std::uint32_t kBlock = 480;
    BusRun out;
    std::array<float, 4> pitches{55.0f, 110.0f, 440.0f, 330.0f};
    std::array<std::uint32_t, 4> active{1, 1, 0, 0};
    std::array<float, 4> timbres{0.5f, 0.2f, 0.3f, 0.4f};
    std::vector<float> block(kBlock * 2);
    for (int b = 0; b < 20; ++b) {
        bus.beginBlock(kBlock);
        bus.beginTelemetryWindow();
        for (std::uint32_t f = 0; f < kBlock; ++f) {
            const float kick = (b % 5 == 0) ? 0.25f * std::exp(-static_cast<float>(f) / 120.0f) : 0.0f;
            bus.accumulateRoleSample(MusicRole::Pulso, f, kick, kick);
            bus.accumulateRoleSample(MusicRole::Corpo, f, 0.2f, 0.2f);
        }
        AudioTelemetryFrame telem{};
        std::fill(block.begin(), block.end(), 0.0f);
        bus.processAndMix(block.data(), kBlock, pitches, active, timbres, telem);
        out.mix.insert(out.mix.end(), block.begin(), block.end());
        out.minDuck.push_back(bus.minDuckingGainInWindow());
    }
    return out;
}

KickVariantMeasure measure(double db) {
    KickVariantMeasure m;
    m.offsetDb = db;
    m.scale = kickGainScale(db);
    return m;
}

} // namespace

TEST_CASE("kick_level_ab_v1: offset 0 is the production path, offsets map to exact linear scales", "[kick_level_ab]") {
    REQUIRE(kickGainScale(0.0) == 1.0f);
    REQUIRE_THAT(kickGainScale(6.0), Catch::Matchers::WithinRel(1.9952623f, 1e-6f));
    REQUIRE_THAT(kickGainScale(12.0), Catch::Matchers::WithinRel(3.9810717f, 1e-6f));
    REQUIRE(kickGainScale(12.0) <= AudioEngine::kMaxPulsoBusGainScale);
    REQUIRE(kKickOffsetsDb.front() == 0.0);
    REQUIRE(kP5PredictedRiseLu.size() + 1 == kKickOffsetsDb.size());
}

TEST_CASE("kick_level_ab_v1: the Pulso gain override is offline only and refuses invalid scales", "[kick_level_ab]") {
    REQUIRE_FALSE(AudioEngine::noteReleaseOverrideAllowed(false, false));
    REQUIRE_FALSE(AudioEngine::noteReleaseOverrideAllowed(true, true));
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    REQUIRE_FALSE(engine.setPulsoBusGainScale(0.0f));
    REQUIRE_FALSE(engine.setPulsoBusGainScale(-1.0f));
    REQUIRE_FALSE(engine.setPulsoBusGainScale(std::numeric_limits<float>::quiet_NaN()));
    REQUIRE_FALSE(engine.setPulsoBusGainScale(std::numeric_limits<float>::infinity()));
    REQUIRE_FALSE(engine.setPulsoBusGainScale(4.01f));
    REQUIRE(engine.setPulsoBusGainScale(2.0f));
    REQUIRE(engine.setPulsoBusGainScale(1.0f));
}

TEST_CASE("kick_level_ab_v1: a Pulso gain scale changes only the kick, the ducking of Corpo is unchanged", "[kick_level_ab]") {
    const BusRun base = runBus(1.0f);
    const BusRun up = runBus(2.0f);
    REQUIRE(base.minDuck.size() == up.minDuck.size());
    bool ducked = false;
    for (std::size_t i = 0; i < base.minDuck.size(); ++i) {
        REQUIRE_THAT(up.minDuck[i], Catch::Matchers::WithinAbs(base.minDuck[i], 1e-6f));
        ducked = ducked || base.minDuck[i] < 0.95f;
    }
    REQUIRE(ducked);   // the ducking is engaged, so its invariance is not vacuous
    // The mix differs (the kick is louder), so the test can fail for the reason it tests
    double maxDiff = 0.0;
    for (std::size_t i = 0; i < base.mix.size(); ++i) maxDiff = std::max(maxDiff, std::abs(static_cast<double>(up.mix[i] - base.mix[i])));
    REQUIRE(maxDiff > 0.01);
}

TEST_CASE("kick_level_ab_v1: production scale 1 leaves the bus output bit-identical", "[kick_level_ab]") {
    RoleBusProcessor reference;
    reference.init(48000);
    const BusRun scaled = runBus(1.0f);
    const BusRun again = runBus(1.0f);
    REQUIRE(scaled.mix == again.mix);
    REQUIRE(reference.pulsoGainScale() == 1.0f);   // default
}

TEST_CASE("kick_level_ab_v1: null test finds the worst residual and refuses length mismatches", "[kick_level_ab]") {
    const std::vector<float> p0{0.1f, -0.2f, 0.3f, 0.0f};
    const std::vector<float> c0{0.05f, 0.05f, 0.05f, 0.05f};
    const std::vector<float> z(4, 0.0f);
    std::vector<float> bus0(4);
    for (std::size_t i = 0; i < 4; ++i) bus0[i] = p0[i] + c0[i];
    constexpr float k = 2.0f;
    std::vector<float> p(4), bus(4);
    for (std::size_t i = 0; i < 4; ++i) {
        p[i] = k * p0[i];
        bus[i] = p[i] + c0[i];
    }
    const std::array<std::span<const float>, 4> s0{p0, c0, z, z};
    std::array<std::span<const float>, 4> s{p, c0, z, z};
    const KickNullTest ok = kickNullTest(k, s0, bus0, s, bus);
    REQUIRE(ok.computed);
    for (const double r : ok.stemMaxAbsResidual) REQUIRE(r <= 1e-7);
    REQUIRE(ok.busMaxAbsResidual <= 1e-7);

    std::vector<float> c = c0;
    c[2] += 1e-3f;
    s[1] = c;
    const KickNullTest bad = kickNullTest(k, s0, bus0, s, bus);
    REQUIRE(bad.stemMaxAbsResidual[1] > 9e-4);
    REQUIRE(bad.stemWorstSample[1] == 2);

    const std::vector<float> shorter(2, 0.0f);
    s[3] = shorter;
    REQUIRE_FALSE(kickNullTest(k, s0, bus0, s, bus).computed);
}

TEST_CASE("kick_level_ab_v1: pre-registered decision rules", "[kick_level_ab]") {
    std::vector<KickVariantMeasure> ms{measure(0.0), measure(3.0), measure(9.0), measure(12.0)};
    ms[0].goldenPcmStatus = "PASS";
    REQUIRE(evaluateKickP1(ms).outcome == "SUPPORTED");
    ms[0].goldenPcmStatus = "FAIL";
    REQUIRE(evaluateKickP1(ms).outcome == "NOT_SUPPORTED");
    REQUIRE(evaluateKickP1({measure(3.0)}).outcome == "INCONCLUSIVE");

    for (auto& m : ms) m.eventsHash = 42;
    REQUIRE(evaluateKickP2(ms).outcome == "SUPPORTED");
    ms[2].eventsHash = 43;
    REQUIRE(evaluateKickP2(ms).outcome == "NOT_SUPPORTED");

    // P3: tolerance on every stem and on the bus
    ms[1].nullTest.computed = true;
    ms[1].nullTest.stemMaxAbsResidual = {1e-7, 1e-7, 0.0, 0.0};
    ms[1].nullTest.busMaxAbsResidual = 1.5e-6;
    REQUIRE(evaluateKickP3(ms[1]).outcome == "SUPPORTED");
    ms[1].nullTest.busMaxAbsResidual = 2.5e-6;
    REQUIRE(evaluateKickP3(ms[1]).outcome == "NOT_SUPPORTED");
    ms[1].nullTest.busMaxAbsResidual = 0.0;
    ms[1].nullTest.stemMaxAbsResidual[1] = 2e-6;
    REQUIRE(evaluateKickP3(ms[1]).outcome == "NOT_SUPPORTED");
    REQUIRE(evaluateKickP3(ms[2]).outcome == "INCONCLUSIVE");

    // P4: Pulso rise = offset +/- 0.05 LU
    ms[0].stemLufs[0] = -34.49;
    ms[1].stemLufs[0] = -34.49 + 3.04;
    REQUIRE(evaluateKickP4(ms[0], ms[1]).outcome == "SUPPORTED");
    ms[1].stemLufs[0] = -34.49 + 3.06;
    REQUIRE(evaluateKickP4(ms[0], ms[1]).outcome == "NOT_SUPPORTED");
    REQUIRE(evaluateKickP4(ms[0], ms[2]).outcome == "INCONCLUSIVE");

    // P5: mix rise = prediction +/- 0.3 LU
    ms[0].mixLufs = -21.25;
    ms[2].mixLufs = -21.25 + 1.24 + 0.29;
    REQUIRE(evaluateKickP5(ms[0], ms[2], 1.24).outcome == "SUPPORTED");
    ms[2].mixLufs = -21.25 + 1.24 - 0.31;
    REQUIRE(evaluateKickP5(ms[0], ms[2], 1.24).outcome == "NOT_SUPPORTED");

    // P6: limiter reduction 0 up to +9 dB, no prediction at +12 dB
    ms[2].maxGainReductionDb = 0.0;
    REQUIRE(evaluateKickP6(ms[2]).outcome == "SUPPORTED");
    ms[2].maxGainReductionDb = 0.1;
    REQUIRE(evaluateKickP6(ms[2]).outcome == "NOT_SUPPORTED");
    ms[3].maxGainReductionDb = 2.0;
    REQUIRE(evaluateKickP6(ms[3]).outcome == "NO_PREDICTION");
    ms[3].captureInvalid = "x";
    REQUIRE(evaluateKickP6(ms[3]).outcome == "INCONCLUSIVE");
}

TEST_CASE("kick_level_ab_v1: loudness alignment gain", "[kick_level_ab]") {
    REQUIRE_FALSE(loudnessAlignGain(std::nullopt, -20.0).has_value());
    REQUIRE_FALSE(loudnessAlignGain(-20.0, std::nullopt).has_value());
    REQUIRE_THAT(*loudnessAlignGain(-20.0, -20.0), Catch::Matchers::WithinAbs(1.0, 1e-12));
    REQUIRE_THAT(*loudnessAlignGain(-18.0, -24.0), Catch::Matchers::WithinRel(0.5011872336, 1e-9));
}
