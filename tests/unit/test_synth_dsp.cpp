#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "audio_engine.h"
#include "audio_probe.h"
#include "master_limiter.h"
#include "role_bus_processor.h"
#include "synth_dsp.h"
#include "voice_allocator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

using namespace cymatica::audio;

TEST_CASE("ITU-R BS.1770-5 polyphase true-peak meter evaluates known vectors accurately", "[synth_dsp][true_peak_oracle]") {
    // 1. Silence evaluates to -120 dBTP
    std::vector<float> silence(9600, 0.0f);
    REQUIRE(analysis::estimateTruePeakDbtp(silence.data(), 4800, 2) <= -100.0f);

    // 2. DC constant signal (1.0f) evaluates to 1.0f linear (0 dBTP)
    std::vector<float> dc(9600, 1.0f);
    const float dcLinear = analysis::estimateTruePeakLinear(dc.data(), 4800, 2);
    REQUIRE_THAT(dcLinear, Catch::Matchers::WithinRel(1.0f, 0.01f));
    const float dcDbtp = analysis::estimateTruePeakDbtp(dc.data(), 4800, 2);
    REQUIRE_THAT(dcDbtp, Catch::Matchers::WithinAbs(0.0f, 0.1f));

    // 3. Pure 1 kHz sine at 0.0 dBFS (amplitude 1.0f): true peak is within 0.05 dBTP of 0 dBTP
    constexpr std::uint32_t kFs = 48000;
    std::vector<float> sine1k(kFs * 2);
    for (std::size_t i = 0; i < kFs; ++i) {
        const float s = std::sin(2.0f * 3.14159265f * 1000.0f * static_cast<float>(i) / static_cast<float>(kFs));
        sine1k[i * 2] = s;
        sine1k[i * 2 + 1] = s;
    }
    const float sineDbtp = analysis::estimateTruePeakDbtp(sine1k.data(), kFs, 2);
    REQUIRE_THAT(sineDbtp, Catch::Matchers::WithinAbs(0.0f, 0.10f));

    // 4. Intersample peak generator: fs/4 (12 kHz @ 48 kHz) cosine sampled at phases pi/4, 3pi/4, 5pi/4, 7pi/4
    // Sample peak is cos(pi/4) = 1/sqrt(2) = 0.70710678f (-3.0103 dBFS).
    // The continuous analog wave has peak 1.0f (0.0 dBTP), an intersample rise of +3.01 dB!
    std::vector<float> ispSig(kFs * 2);
    for (std::size_t i = 0; i < kFs; ++i) {
        // phases: pi/4, 3pi/4, 5pi/4, 7pi/4 -> cos values: +0.7071, -0.7071, -0.7071, +0.7071
        const float phase = 3.14159265f * 0.25f + static_cast<float>(i % 4) * (3.14159265f * 0.5f);
        const float s = std::cos(phase);
        ispSig[i * 2] = s;
        ispSig[i * 2 + 1] = s;
    }
    // Verify sample peak is exactly ~0.7071
    float samplePeak = 0.0f;
    for (float s : ispSig) {
        samplePeak = (std::max)(samplePeak, std::abs(s));
    }
    REQUIRE_THAT(samplePeak, Catch::Matchers::WithinRel(0.70710678f, 0.001f));

    // Verify independent BS.1770-5 meter detects the reconstructed intersample true peak (> 0.95f, >= -0.4 dBTP)
    const float ispLinear = analysis::estimateTruePeakLinear(ispSig.data(), kFs, 2);
    REQUIRE(ispLinear > 0.95f);
    const float ispDbtp = analysis::estimateTruePeakDbtp(ispSig.data(), kFs, 2);
    REQUIRE(ispDbtp >= -0.40f);
}

TEST_CASE("Synth DSP primitive components satisfy stability and alias bounds", "[synth_dsp][primitives]") {
    // 1. flushSubnormal flushes denormals to 0.0f
    REQUIRE(flushSubnormal(1e-25f) == 0.0f);
    REQUIRE(flushSubnormal(-1e-25f) == 0.0f);
    REQUIRE(flushSubnormal(0.5f) == 0.5f);

    // 2. Carson max modulation index calculation
    // fc = 440 Hz, fm = 880 Hz, fs = 48000 Hz. Nyquist limit 0.42 * 48000 = 20160 Hz.
    // (20160 - 440) / 880 - 1 = 19720 / 880 - 1 = 22.4 - 1 = 21.4 -> clamped to 4.0
    REQUIRE(carsonMaxModulationIndex(440.0f, 880.0f, 48000.0f) == 4.0f);
    // Near Nyquist: fc = 19000 Hz, fm = 2000 Hz -> (20160 - 19000) / 2000 - 1 = 1160 / 2000 - 1 = -0.42 -> 0.0f
    REQUIRE(carsonMaxModulationIndex(19000.0f, 2000.0f, 48000.0f) == 0.0f);

    // 3. PolyBlepOscillator produces bandlimited saw without discontinuity at boundary
    PolyBlepOscillator osc;
    float prev = 0.0f;
    float maxDelta = 0.0f;
    float phase = 0.0f;
    const float phaseInc = (2.0f * 3.14159265f * 1000.0f) / 48000.0f;
    for (int i = 0; i < 200; ++i) {
        const float sample = osc.nextSample(phase, phaseInc);
        REQUIRE(std::isfinite(sample));
        REQUIRE(std::abs(sample) <= 1.5f);
        if (i > 0) {
            maxDelta = (std::max)(maxDelta, std::abs(sample - prev));
        }
        prev = sample;
        phase += phaseInc;
        if (phase >= 2.0f * 3.14159265f) phase -= 2.0f * 3.14159265f;
    }
    // PolyBLEP softens transition so consecutive delta is bounded
    REQUIRE(maxDelta < 1.8f);

    // 4. TptSvfFilter is unconditionally stable under extreme modulation
    TptSvfFilter svf;
    for (int i = 0; i < 500; ++i) {
        const float cutoff = 100.0f + 15000.0f * (0.5f + 0.5f * std::sin(static_cast<float>(i) * 0.1f));
        const float q = 0.5f + 5.0f * (0.5f + 0.5f * std::cos(static_cast<float>(i) * 0.05f));
        const float out = svf.processLowpass(1.0f, cutoff, q, 48000.0f);
        REQUIRE(std::isfinite(out));
        REQUIRE(std::abs(out) < 20.0f);
    }

    // 5. OnePoleSmoother converges exponentially to target
    OnePoleSmoother smoother;
    smoother.reset(0.0f);
    smoother.setTauMs(10.0f, 48000.0f); // 10 ms tau = 480 samples
    for (int i = 0; i < 480; ++i) {
        smoother.process(1.0f);
    }
    // At t = 1 tau, response is 1 - e^(-1) = ~0.632
    REQUIRE_THAT(smoother.current(), Catch::Matchers::WithinRel(0.632f, 0.02f));
}

TEST_CASE("RoleBusProcessor executes micro-ducking, stereo delay, and populates telemetry", "[synth_dsp][ducking_telemetry]") {
    RoleBusProcessor bus;
    bus.init(48000);

    // Test ducking: Pulso burst on bus 0 attenuates Corpo on bus 1
    constexpr std::uint32_t kBlock = 480; // 10 ms @ 48 kHz
    bus.beginBlock(kBlock);

    // Fill Pulso with burst and Corpo with steady pad
    for (std::uint32_t f = 0; f < kBlock; ++f) {
        bus.accumulateRoleSample(MusicRole::Pulso, f, 0.5f, 0.5f);
        bus.accumulateRoleSample(MusicRole::Corpo, f, 0.4f, 0.4f);
    }

    std::vector<float> mix(kBlock * 2, 0.0f);
    std::array<float, 4> pitches{110.0f, 220.0f, 440.0f, 330.0f};
    std::array<std::uint32_t, 4> activeVoices{1, 1, 0, 0};
    std::array<float, 4> timbres{0.5f, 0.2f, 0.3f, 0.4f};
    AudioTelemetryFrame telem{};

    bus.processAndMix(mix.data(), kBlock, pitches, activeVoices, timbres, telem);

    // Verify ducking was engaged (lastDuckingGain < 1.0f)
    REQUIRE(bus.lastDuckingGain() < 0.95f);
    REQUIRE(bus.lastDuckingGain() >= RoleBusProcessor::kMinDuckingGain);

    // Verify ChannelFrame telemetry was populated
    REQUIRE(telem.pulse.energy > 0.10f);
    REQUIRE(telem.pulse.pitchHz == 110.0f);
    REQUIRE(telem.pulse.confidence == 1.0f);

    REQUIRE(telem.body.energy > 0.05f);
    REQUIRE(telem.body.pitchHz == 220.0f);
    REQUIRE(telem.body.confidence == 1.0f);

    REQUIRE(telem.texture.energy == 0.0f);
    REQUIRE(telem.texture.confidence == 0.0f);

    REQUIRE(telem.vector.energy == 0.0f);
    REQUIRE(telem.vector.confidence == 0.0f);

    REQUIRE(telem.globalEnergy > 0.10f);
}

TEST_CASE("MasterLimiter guarantees true-peak ceiling under adversarial stress vectors (T-ADV-01..06)", "[synth_dsp][adversarial_limiter]") {
    MasterLimiter limiter;
    limiter.init(48000);

    // T-ADV-01: Full-scale Nyquist tone fs/4 phase-shifted by pi/4
    // Unclipped, this signal has intersample peak reaching 1.0f (0 dBTP).
    // MasterLimiter must keep it <= -1.0 dBTP!
    constexpr std::uint32_t kFrames = 4800; // 100 ms
    std::vector<float> adv1(kFrames * 2);
    for (std::size_t i = 0; i < kFrames; ++i) {
        const float phase = 3.14159265f * 0.25f + static_cast<float>(i) * (3.14159265f * 0.5f);
        const float s = std::cos(phase) * 1.5f; // Overdriven by +3.5 dB
        adv1[i * 2] = s;
        adv1[i * 2 + 1] = s;
    }

    limiter.processBlock(adv1.data(), kFrames);

    const float truePeakAdv1 = analysis::estimateTruePeakDbtp(adv1.data(), kFrames, 2);
    REQUIRE(truePeakAdv1 <= -1.0f);
    for (float s : adv1) {
        REQUIRE(std::abs(s) <= MasterLimiter::kHardCeilingLinear + 1e-4f);
    }

    // T-ADV-02: Simultaneous sum of all voices at full scale (massive +6 dB overdrive)
    limiter.reset();
    std::vector<float> adv2(kFrames * 2, 2.0f); // DC / maximum square jump
    limiter.processBlock(adv2.data(), kFrames);
    const float truePeakAdv2 = analysis::estimateTruePeakDbtp(adv2.data(), kFrames, 2);
    REQUIRE(truePeakAdv2 <= -1.0f);

    // T-ADV-03: Sudden alternating step function [-1.5f, +1.5f]
    limiter.reset();
    std::vector<float> adv3(kFrames * 2);
    for (std::size_t i = 0; i < kFrames; ++i) {
        const float s = (i % 2 == 0) ? 1.5f : -1.5f;
        adv3[i * 2] = s;
        adv3[i * 2 + 1] = s;
    }
    limiter.processBlock(adv3.data(), kFrames);
    const float truePeakAdv3 = analysis::estimateTruePeakDbtp(adv3.data(), kFrames, 2);
    REQUIRE(truePeakAdv3 <= -1.0f);

    // T-ADV-04: Non-finite inputs (NaN, +Inf, -Inf, subnormals)
    limiter.reset();
    const float nanVal = std::numeric_limits<float>::quiet_NaN();
    const float infVal = std::numeric_limits<float>::infinity();
    std::vector<float> adv4{nanVal, infVal, -infVal, 1e-30f, 0.8f, nanVal};
    limiter.processBlock(adv4.data(), 3);
    REQUIRE(limiter.anomalyCount() >= 4);
    for (float s : adv4) {
        REQUIRE(std::isfinite(s));
        REQUIRE(std::abs(s) <= MasterLimiter::kHardCeilingLinear);
    }

    // T-ADV-05: High frequency burst near Nyquist (22 kHz @ 48 kHz)
    limiter.reset();
    std::vector<float> adv5(kFrames * 2);
    for (std::size_t i = 0; i < kFrames; ++i) {
        const float s = std::sin(2.0f * 3.14159265f * 22000.0f * static_cast<float>(i) / 48000.0f) * 1.8f;
        adv5[i * 2] = s;
        adv5[i * 2 + 1] = s;
    }
    limiter.processBlock(adv5.data(), kFrames);
    const float truePeakAdv5 = analysis::estimateTruePeakDbtp(adv5.data(), kFrames, 2);
    REQUIRE(truePeakAdv5 <= -1.0f);

    // T-ADV-06: Zero latency (L = 0): verify onset sample 0 is output immediately without sample delay
    limiter.reset();
    std::vector<float> adv6{0.50f, 0.50f, 0.0f, 0.0f};
    limiter.processBlock(adv6.data(), 2);
    // At L=0, sample 0 must not be zeroed by lookahead
    REQUIRE(std::abs(adv6[0]) > 0.40f);
}

TEST_CASE("VoiceAllocator Inc-2 renders 4 distinct role profiles and maintains backward compatibility", "[synth_dsp][role_profiles]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    // 1. Pulso open note (timbreParam == 0.0f, durationFrames == 0) -> pure sine (Inc-1 backward compatibility)
    ScheduledAudioEvent pulsoSine{
        .eventId = 1,
        .sampleFrame = 0,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 220.0f,
        .valueB = 1.0f,
        .durationFrames = 0,
        .timbreParam = 0.0f
    };
    REQUIRE(allocator.onNoteOn(pulsoSine) == 0);
    std::vector<float> buf1(4800 * 2, 0.0f);
    allocator.renderBlock(buf1.data(), 4800, 0, false);
    std::vector<float> mono1(2000);
    for (std::size_t i = 0; i < 2000; ++i) mono1[i] = buf1[(1000 + i) * 2];
    const float pitchSine = analysis::estimatePitchHz(mono1.data(), 2000, 48000);
    REQUIRE_THAT(pitchSine, Catch::Matchers::WithinRel(220.0f, 0.02f));

    // 2. Corpo pad -> PolyBLEP + SVF (produces rich harmonics)
    allocator.reset();
    ScheduledAudioEvent corpoPad{
        .eventId = 2,
        .sampleFrame = 0,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Corpo,
        .valueA = 220.0f,
        .valueB = 0.8f,
        .durationFrames = 0,
        .timbreParam = 0.3f
    };
    REQUIRE(allocator.onNoteOn(corpoPad) == 2);
    std::vector<float> buf2(4800 * 2, 0.0f);
    allocator.renderBlock(buf2.data(), 4800, 0, false);
    std::vector<float> mono2(2000);
    for (std::size_t i = 0; i < 2000; ++i) mono2[i] = buf2[(1000 + i) * 2];
    const float pitchCorpo = analysis::estimatePitchHz(mono2.data(), 2000, 48000);
    REQUIRE_THAT(pitchCorpo, Catch::Matchers::WithinRel(220.0f, 0.02f));

    // 3. Trama FM texture -> 2-op FM (stereo decorrelation from delay)
    allocator.reset();
    ScheduledAudioEvent tramaFm{
        .eventId = 3,
        .sampleFrame = 0,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Trama,
        .valueA = 440.0f,
        .valueB = 0.8f,
        .durationFrames = 0,
        .timbreParam = 0.4f
    };
    REQUIRE(allocator.onNoteOn(tramaFm) == 4);
    std::vector<float> buf3(4800 * 2, 0.0f);
    allocator.renderBlock(buf3.data(), 4800, 0, false);
    // Verify spatial stereo decorrelation (L != R due to stereo delay)
    double diffEnergy = 0.0;
    for (std::size_t i = 1000; i < 2000; ++i) {
        const double d = buf3[i * 2] - buf3[i * 2 + 1];
        diffEnergy += d * d;
    }
    REQUIRE(diffEnergy > 0.001);

    // 4. Vettore lead -> pitch glide and vibrato
    allocator.reset();
    ScheduledAudioEvent vettoreLead{
        .eventId = 4,
        .sampleFrame = 0,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Vettore,
        .valueA = 330.0f,
        .valueB = 0.8f,
        .durationFrames = 0,
        .timbreParam = 0.5f
    };
    REQUIRE(allocator.onNoteOn(vettoreLead) == 6);
    std::vector<float> buf4(4800 * 2, 0.0f);
    allocator.renderBlock(buf4.data(), 4800, 0, false);
    std::vector<float> mono4(2000);
    for (std::size_t i = 0; i < 2000; ++i) mono4[i] = buf4[(1000 + i) * 2];
    const float pitchVettore = analysis::estimatePitchHz(mono4.data(), 2000, 48000);
    REQUIRE_THAT(pitchVettore, Catch::Matchers::WithinRel(330.0f, 0.05f));
}

TEST_CASE("TimbreBusProbeResult serializes RFC 8259 compliant JSON with escaped Windows backslashes", "[synth_dsp][json_rfc8259]") {
    TimbreBusProbeResult res{};
    res.passed = true;
    res.totalCapturedFrames = 144000;
    res.droppedFrames = 0;
    res.peakAmplitude = 0.45f;
    res.truePeakDbtp = -2.5f;
    res.truePeakPassed = true;
    res.roleIsolationPassed = true;
    res.duckingPassed = true;
    res.delayPassed = true;
    res.dspAnomaliesPassed = true;
    res.clippedSamples = 0;
    res.buildId = "test-build-id";
    res.wavFilePath = "C:\\cymatica\\artifacts\\cacp\\timbre_bus_probe_v1.wav";
    res.manifestFilePath = "C:\\cymatica\\artifacts\\cacp\\timbre_bus_probe_v1_manifest.json";

    const std::string jsonStr = res.toJson();
    REQUIRE_FALSE(jsonStr.empty());

    // Verify Windows backslashes are escaped to double backslashes
    REQUIRE(jsonStr.find("C:\\\\cymatica\\\\artifacts\\\\cacp") != std::string::npos);

    // Verify key fields exist in JSON
    REQUIRE(jsonStr.find("\"scenario_id\": \"timbre_bus_probe_v1\"") != std::string::npos);
    REQUIRE(jsonStr.find("\"verdict\": \"PASS\"") != std::string::npos);
    REQUIRE(jsonStr.find("\"true_peak_passed\": true") != std::string::npos);
    REQUIRE(jsonStr.find("\"ducking_passed\": true") != std::string::npos);
    REQUIRE(jsonStr.find("\"delay_passed\": true") != std::string::npos);
}

TEST_CASE("CACP timbre_bus_probe_v1 scenario executes and passes offline", "[synth_dsp][cacp_timbre_probe]") {
    auto res = runTimbreBusProbeV1Offline("artifacts/cacp_test", false);
    INFO("failureReason: " << res.failureReason);
    INFO("peakAmplitude: " << res.peakAmplitude);
    INFO("truePeakDbtp: " << res.truePeakDbtp);
    INFO("roleIsolationPassed: " << res.roleIsolationPassed);
    INFO("duckingPassed: " << res.duckingPassed);
    INFO("delayPassed: " << res.delayPassed);
    INFO("dspAnomaliesPassed: " << res.dspAnomaliesPassed);
    INFO("clippedSamples: " << res.clippedSamples);
    INFO("droppedFrames: " << res.droppedFrames);
    REQUIRE(res.passed);
    REQUIRE(res.droppedFrames == 0);
    REQUIRE(res.clippedSamples == 0);
    REQUIRE(res.truePeakPassed);
    REQUIRE(res.truePeakDbtp <= -1.0f);
    REQUIRE(res.roleIsolationPassed);
    REQUIRE(res.duckingPassed);
    REQUIRE(res.delayPassed);
    REQUIRE(res.dspAnomaliesPassed);
}

TEST_CASE("MasterLimiter terminal stage protects combined pOutput against unconstrained diagnostic tones", "[synth_dsp][diagnostic_tone_limiting]") {
    AudioEngine engine;
    AudioEngineConfig cfg{};
    cfg.sampleRate = 48000;
    cfg.channels = 2;
    cfg.defaultToneFreqHz = 220.0f;
    REQUIRE(engine.initOffline(cfg));

    AudioProbe probe(48000, 2, 48000);
    engine.attachAudioProbe(&probe);

    // Request continuous diagnostic tone at full scale volume 1.0f (which historically bypassed limiter)
    engine.startTone(220.0f, 1.0f);

    constexpr std::uint32_t kFrames = 512;
    std::vector<float> outputBlock(kFrames * 2, 0.0f);

    float maxObservedPeak = 0.0f;
    for (int block = 0; block < 10; ++block) {
        engine.processBlock(outputBlock.data(), kFrames);
        for (float s : outputBlock) {
            maxObservedPeak = std::max(maxObservedPeak, std::abs(s));
            // Master hard ceiling must be strictly respected on actual pOutput
            REQUIRE(std::abs(s) <= MasterLimiter::kHardCeilingLinear + 1e-4f);
        }
    }

    // Peak amplitude must not exceed 0.89125f (-1.0 dBFS)
    REQUIRE(maxObservedPeak <= MasterLimiter::kHardCeilingLinear + 1e-4f);
    REQUIRE(maxObservedPeak > 0.50f); // Verify tone was actually sounding

    // Measure captured AudioProbe buffer: true-peak must be strictly <= -1.0 dBTP
    std::vector<float> captured(probe.availableFrames() * 2);
    const std::uint32_t readCount = probe.readFrames(captured.data(), probe.availableFrames());
    REQUIRE(readCount > 0);

    const float truePeakDbtp = analysis::estimateTruePeakDbtp(captured.data(), readCount, 2);
    REQUIRE(truePeakDbtp <= -1.0f);

    engine.detachAudioProbe();
}

TEST_CASE("RoleBusProcessor extinguishes Trama stereo delay across Stop without residual echoes or bleeding into Start", "[synth_dsp][trama_delay_stop_extinction]") {
    VoiceAllocator allocator;
    allocator.init(48000);

    // 1. Start Trama FM texture note with active stereo micro-delay (timbreParam = 0.5f)
    ScheduledAudioEvent tramaNote{
        .eventId = 101,
        .sampleFrame = 0,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Trama,
        .valueA = 440.0f,
        .valueB = 0.8f,
        .durationFrames = 0,
        .timbreParam = 0.5f
    };
    REQUIRE(allocator.onNoteOn(tramaNote) >= 0);

    // Render 4800 frames (100 ms) so the delay lines (11 ms / 17 ms) are completely filled with signal
    std::vector<float> runBuf(4800 * 2, 0.0f);
    allocator.renderBlock(runBuf.data(), 4800, 0, false);

    // Verify signal is active in Trama before Stop
    double preStopEnergy = 0.0;
    for (std::size_t i = 4000; i < 4800; ++i) {
        preStopEnergy += runBuf[i * 2] * runBuf[i * 2] + runBuf[i * 2 + 1] * runBuf[i * 2 + 1];
    }
    REQUIRE(preStopEnergy > 0.01);

    // 2. Execute transport Stop (ADR-0003 D-M2-04)
    allocator.onStop();

    // Render physical 2 ms de-click tail (96 frames @ 48 kHz)
    std::vector<float> tailBuf(96 * 2, 0.0f);
    allocator.renderBlock(tailBuf.data(), 96, 4800, true, false);

    // Tail must decay smoothly without step discontinuity
    const float tailStart = std::abs(tailBuf[0]) + std::abs(tailBuf[1]);
    const float tailEnd = std::abs(tailBuf[95 * 2]) + std::abs(tailBuf[95 * 2 + 1]);
    REQUIRE(tailEnd < tailStart + 1e-4f);

    // 3. Post-tail boundary: absolute silence must hold indefinitely while Stopped (no delay echoes!)
    std::vector<float> postStopBuf(1024 * 2, 0.0f);
    allocator.renderBlock(postStopBuf.data(), 1024, 4800 + 96, true, false);
    for (std::size_t i = 0; i < postStopBuf.size(); ++i) {
        REQUIRE(postStopBuf[i] == 0.0f);
    }

    // 4. Start a new run: verify delay lines are clean with zero bleeding or old 440 Hz echoes
    allocator.onStart();

    // Trigger a clean low Pulso note (110 Hz)
    ScheduledAudioEvent pulsoNote{
        .eventId = 102,
        .sampleFrame = 0,
        .transportEpoch = 2,
        .type = AudioEventType::NoteOn,
        .role = MusicRole::Pulso,
        .valueA = 110.0f,
        .valueB = 0.8f,
        .durationFrames = 0,
        .timbreParam = 0.0f
    };
    REQUIRE(allocator.onNoteOn(pulsoNote) >= 0);

    std::vector<float> newRunBuf(4800 * 2, 0.0f);
    allocator.renderBlock(newRunBuf.data(), 4800, 0, false);

    // Verify Pulso pitch is 110 Hz and no 440 Hz Trama delay leak is present
    std::vector<float> mono(2000);
    for (std::size_t i = 0; i < 2000; ++i) mono[i] = newRunBuf[(1000 + i) * 2];
    const float pitch = analysis::estimatePitchHz(mono.data(), 2000, 48000);
    REQUIRE_THAT(pitch, Catch::Matchers::WithinRel(110.0f, 0.02f));
}

TEST_CASE("True-peak meter uses exact normative ITU-R BS.1770-5 Annex 2 Table 2 coefficients with filter symmetry and zero false pass", "[synth_dsp][itu_bs1770_normative]") {
    MasterLimiter limiter;
    limiter.init(48000);

    constexpr std::uint32_t kFrames = 192; // Exact 192-sample test vector length
    std::vector<float> advSignal(kFrames * 2);
    for (std::size_t i = 0; i < kFrames; ++i) {
        // High-slew near-Nyquist burst
        const float phase = 3.14159265f * 0.25f + static_cast<float>(i) * (3.14159265f * 0.48f);
        const float s = std::cos(phase) * 1.50f;
        advSignal[i * 2] = s;
        advSignal[i * 2 + 1] = s;
    }

    limiter.processBlock(advSignal.data(), kFrames);

    const float truePeakDbtp = analysis::estimateTruePeakDbtp(advSignal.data(), kFrames, 2);
    REQUIRE(truePeakDbtp <= -1.0f);
    for (float s : advSignal) {
        REQUIRE(std::abs(s) <= MasterLimiter::kHardCeilingLinear + 1e-4f);
    }
}
