// Archived experiment for DIF-INC4-R2-01 (docs/research/M2_4_DIF-INC4-R2-01_Limiter_GR_Analysis.md).
// Not part of the build: to reproduce, copy it to tests/unit/, add it to tests/CMakeLists.txt and run
// cymatica_tests "[experiment_r2_01]" in Release. Hidden tag [.] keeps it out of normal runs.
#include <catch2/catch_test_macros.hpp>

#include "audio_engine.h"
#include "audio_probe.h"
#include "master_limiter.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <numeric>
#include <vector>

using namespace cymatica::audio;

namespace {

struct WStats {
    double meanUs, p50Us, p999Us, maxUs;
    float preLimiterPeak; // only valid when no GR (output == input)
    float outPeak;     // blocks after the 64 warm-up blocks (measured window)
    float outPeakAll;  // all blocks, warm-up included (initial 8-voice onset)
    float grDb;
    std::uint32_t steals;
    bool allActive;
};

double pct(std::vector<double> v, double p) {
    std::sort(v.begin(), v.end());
    const std::size_t idx = std::min(v.size() - 1, static_cast<std::size_t>(std::ceil(p / 100.0 * v.size())) - 1);
    return v[idx];
}

// Workload W as in runProfileWorkload, with optional diagnostic tone volume and optional limiter bypass via
// measuring time; per-block duration measured externally with steady_clock as well as internal histogram.
WStats runW(float toneVolume, std::uint32_t blocks = 10000) {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    engine.sendCommand(AudioCommand{.commandId = 1, .type = AudioCommandType::Start, .paramF32 = toneVolume, .targetSampleFrame = 0});
    const std::array<MusicRole, 4> roles{MusicRole::Pulso, MusicRole::Corpo, MusicRole::Trama, MusicRole::Vettore};
    std::uint64_t id = 1;
    auto on = [&](MusicRole r, std::uint64_t f, float hz) {
        return ScheduledAudioEvent{.eventId = id++, .sampleFrame = f, .transportEpoch = 1, .type = AudioEventType::NoteOn,
                                   .role = r, .valueA = hz, .valueB = 1.0f, .durationFrames = 0, .timbreParam = 0.5f};
    };
    for (std::size_t r = 0; r < 4; ++r)
        for (int v = 0; v < 2; ++v) REQUIRE(engine.scheduleAudioEvent(on(roles[r], 0, 220.0f + static_cast<float>(r * 50 + v * 25))));
    std::vector<float> buf(1024, 0.0f);
    std::vector<double> durs;
    durs.reserve(blocks);
    float peak = 0.0f;
    float peakAll = 0.0f;
    WStats s{};
    s.allActive = true;
    std::uint32_t stealsBefore = 0;
    for (std::uint32_t b = 0; b < 64 + blocks; ++b) {
        if (b == 64) {
            engine.resetCallbackProfileHistogram();
            engine.masterLimiter().resetTelemetry();
            (void)engine.updateTelemetry();
            stealsBefore = engine.telemetry().voiceDropCount;
        }
        REQUIRE(engine.scheduleAudioEvent(on(roles[b % 4], static_cast<std::uint64_t>(b) * 512, 220.0f + static_cast<float>((b % 4) * 50))));
        engine.processBlock(buf.data(), 512);
        for (float x : buf) peakAll = std::max(peakAll, std::abs(x));
        if (b >= 64) {
            durs.push_back(static_cast<double>(engine.callbackProfileHistogram().lastDurationNs) / 1000.0);
            for (float x : buf) peak = std::max(peak, std::abs(x));
            if (!engine.updateTelemetry() || engine.telemetry().activeVoiceCount != 8) s.allActive = false;
        }
    }
    (void)engine.updateTelemetry();
    s.steals = engine.telemetry().voiceDropCount - stealsBefore;
    s.grDb = engine.masterLimiter().maxGainReductionDb();
    s.outPeak = peak;
    s.outPeakAll = peakAll;
    s.preLimiterPeak = peak;
    s.meanUs = std::accumulate(durs.begin(), durs.end(), 0.0) / static_cast<double>(durs.size());
    s.p50Us = pct(durs, 50.0);
    s.p999Us = pct(durs, 99.9);
    s.maxUs = *std::max_element(durs.begin(), durs.end());
    engine.shutdown();
    return s;
}

// Limiter alone on 512-frame stereo blocks: returns mean ns per block over `blocks` blocks
double benchLimiter(float amplitude, int mode, std::uint32_t blocks, float& grOut) {
    MasterLimiter lim;
    lim.init(48000);
    std::vector<float> src(1024), work(1024);
    double phase = 0.0;
    const double w = 2.0 * 3.14159265358979323846 * 997.0 / 48000.0;
    std::uint64_t totalNs = 0;
    volatile float sink = 0.0f;
    for (std::uint32_t b = 0; b < blocks; ++b) {
        float a = amplitude;
        if (mode == 1) a = (b % 8 == 0) ? amplitude : 0.3f;              // bursts: attack/hold/release cycles
        if (mode == 2) a = amplitude * std::pow(0.5f, static_cast<float>(b % 200)); // decays to subnormal range
        if (mode == 3) a = amplitude * 1.0e-39f; // constant subnormal input (below FLT_MIN)
        for (std::size_t f = 0; f < 512; ++f) {
            const float x = static_cast<float>(std::sin(phase)) * a;
            src[2 * f] = x;
            src[2 * f + 1] = -x;
            phase += w;
        }
        std::copy(src.begin(), src.end(), work.begin());
        const auto t0 = std::chrono::steady_clock::now();
        lim.processBlock(work.data(), 512);
        const auto t1 = std::chrono::steady_clock::now();
        totalNs += static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        sink = sink + work[0];
    }
    grOut = lim.maxGainReductionDb();
    (void)sink;
    return static_cast<double>(totalNs) / blocks;
}

} // namespace

TEST_CASE("EXPERIMENT R2-01 W levels and tone sweep", "[.][experiment_r2_01]") {
    for (int rep = 0; rep < 5; ++rep) {
        for (float tv : {0.0f, 0.4f, 0.6f}) {
            const WStats s = runW(tv);
            std::printf("W rep=%d tone=%.2f outPeakAll=%.4f (%.2f dBFS) outPeak=%.4f (%.2f dBFS) GR=%.2f dB steals=%u allActive=%d mean=%.2fus p50=%.2fus p99.9=%.2fus max=%.2fus\n",
                        rep, tv, s.outPeakAll, 20.0 * std::log10(std::max(1e-9f, s.outPeakAll)), s.outPeak, 20.0 * std::log10(std::max(1e-9f, s.outPeak)), s.grDb, s.steals, s.allActive ? 1 : 0,
                        s.meanUs, s.p50Us, s.p999Us, s.maxUs);
        }
    }
}

TEST_CASE("EXPERIMENT R2-01 limiter microbenchmark", "[.][experiment_r2_01]") {
    struct Cond { const char* name; float amp; int mode; };
    const std::array<Cond, 8> conds{{{"below_thr_0.5", 0.5f, 0},
                                     {"decay_no_gr_0.5", 0.5f, 2},
                                     {"subnormal_const", 1.0f, 3},
                                     {"above_thr_1.5", 1.5f, 0},
                                     {"far_above_8.0", 8.0f, 0},
                                     {"bursts_1.5", 1.5f, 1},
                                     {"silence_0", 0.0f, 0},
                                     {"decay_subnormal", 1.0f, 2}}};
    for (int rep = 0; rep < 7; ++rep) {
        for (const auto& c : conds) {
            float gr = 0.0f;
            const double ns = benchLimiter(c.amp, c.mode, 100000, gr);
            std::printf("LIM rep=%d cond=%s ns_per_block=%.1f ns_per_frame=%.3f GR=%.2f dB\n", rep, c.name, ns, ns / 512.0, gr);
        }
    }
}
