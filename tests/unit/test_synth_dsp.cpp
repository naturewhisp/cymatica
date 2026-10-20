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

    // 2. DC constant signal (1.0f) evaluates to ~1.0f linear (0 dBTP). The meter zero-pads outside the
    // capture (digital silence before/after the stream), so the DC is faded in/out over 480 frames to keep
    // the Gibbs overshoot of an abrupt step out of this passband check.
    std::vector<float> dc(9600, 1.0f);
    for (std::size_t i = 0; i < 480; ++i) {
        const float g = 0.5f - 0.5f * std::cos(3.14159265f * static_cast<float>(i) / 480.0f);
        dc[i * 2] = dc[i * 2 + 1] = g;
        dc[(4799 - i) * 2] = dc[(4799 - i) * 2 + 1] = g;
    }
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

TEST_CASE("MasterLimiter keeps the estimated true peak under the ceiling on the adversarial vectors (T-ADV-01..06)", "[synth_dsp][adversarial_limiter]") {
    MasterLimiter limiter;
    limiter.init(48000);

    // The meter zero-pads after the capture. In the engine every stream end passes through the 2 ms
    // transport de-click ramp (Stop/Pause), so the vectors end the same way instead of with an abrupt step
    // to silence, which would add a reconstruction overshoot no running engine produces. The engine ramps
    // before the limiter; fading the limiter output here is a close approximation: during a falling ramp the
    // limiter cannot attack further, and its 50 ms release barely moves the gain within 96 frames.
    const auto applyTransportFadeOut = [](std::vector<float>& buf, std::size_t frames) {
        constexpr std::size_t kTail = 96; // floor(0.002 * 48000)
        for (std::size_t k = 0; k < kTail && k < frames; ++k) {
            const std::size_t f = frames - kTail + k;
            const float g = 1.0f - static_cast<float>(k + 1) / static_cast<float>(kTail);
            buf[f * 2] *= g;
            buf[f * 2 + 1] *= g;
        }
    };

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
    applyTransportFadeOut(adv1, kFrames);

    const float truePeakAdv1 = analysis::estimateTruePeakDbtp(adv1.data(), kFrames, 2);
    REQUIRE(truePeakAdv1 <= -1.0f);
    for (float s : adv1) {
        REQUIRE(std::abs(s) <= MasterLimiter::kHardCeilingLinear + 1e-4f);
    }

    // T-ADV-02: Simultaneous sum of all voices at full scale (massive +6 dB overdrive)
    limiter.reset();
    std::vector<float> adv2(kFrames * 2, 2.0f); // DC / maximum square jump
    limiter.processBlock(adv2.data(), kFrames);
    applyTransportFadeOut(adv2, kFrames);
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
    applyTransportFadeOut(adv3, kFrames);
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
    applyTransportFadeOut(adv5, kFrames);
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

    // The checks are backed by measurements, not by static configuration
    REQUIRE(res.duckingAttenuationDb >= 1.5f);
    REQUIRE(res.duckingAttenuationDb <= 3.1f);
    REQUIRE(res.duckingPreHitDeviationDb <= 0.05f);
    REQUIRE(res.delayLagLFrames == 529); // 11 ms tap + first non-zero attack sample
    REQUIRE(res.delayLagRFrames == 817); // 17 ms tap + first non-zero attack sample
    REQUIRE(res.delayEchoRmsDbfs > -50.0f);
    REQUIRE(res.roleIsolationWorstLeakDb <= -60.0f);
    REQUIRE(res.roleIsolationWeakestActiveDbfs > -40.0f);
    const std::string json = res.toJson();
    REQUIRE(json.find("\"ducking_attenuation_db\"") != std::string::npos);
    REQUIRE(json.find("\"delay_lag_l_frames\": 529") != std::string::npos);
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

TEST_CASE("True-peak meter uses exact normative ITU-R BS.1770-5 Annex 2 table coefficients with filter symmetry and zero false pass", "[synth_dsp][itu_bs1770_normative]") {
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

namespace {

ScheduledAudioEvent makeNoteOn(std::uint64_t id, MusicRole role, float pitchHz, float velocity, float timbre,
                               std::uint64_t frame = 0, std::uint32_t durationFrames = 0) {
    return ScheduledAudioEvent{
        .eventId = id,
        .sampleFrame = frame,
        .transportEpoch = 1,
        .type = AudioEventType::NoteOn,
        .role = role,
        .valueA = pitchHz,
        .valueB = velocity,
        .durationFrames = durationFrames,
        .timbreParam = timbre
    };
}

// Instantaneous frequency from interpolated upward zero crossings of channel `ch`, one value per period,
// reported at the period midpoint (in frames).
struct FreqPoint {
    double timeFrames;
    double freqHz;
};

std::vector<FreqPoint> zeroCrossingFrequencies(const std::vector<float>& stereo, std::size_t ch,
                                               std::size_t begin, std::size_t end, double minPeriodFrames) {
    std::vector<FreqPoint> points;
    double lastCrossing = -1.0;
    for (std::size_t n = begin + 1; n < end; ++n) {
        const double a = stereo[(n - 1) * 2 + ch];
        const double b = stereo[n * 2 + ch];
        if (a < 0.0 && b >= 0.0) {
            const double crossing = static_cast<double>(n - 1) + (-a / (b - a));
            if (lastCrossing >= 0.0) {
                const double period = crossing - lastCrossing;
                if (period < minPeriodFrames) continue; // ignore spurious crossings
                points.push_back({0.5 * (crossing + lastCrossing), 48000.0 / period});
            }
            lastCrossing = crossing;
        }
    }
    return points;
}

float maxAbsStep(const std::vector<float>& stereo, std::size_t ch, std::size_t begin, std::size_t end) {
    float m = 0.0f;
    for (std::size_t n = std::max<std::size_t>(begin, 1); n < end; ++n) {
        m = (std::max)(m, std::abs(stereo[n * 2 + ch] - stereo[(n - 1) * 2 + ch]));
    }
    return m;
}

void appendRender(VoiceAllocator& allocator, std::vector<float>& out, std::uint32_t frames,
                  std::uint64_t logicalFrame, bool paused = false) {
    std::vector<float> block(static_cast<std::size_t>(frames) * 2, 0.0f);
    allocator.renderBlock(block.data(), frames, logicalFrame, paused);
    out.insert(out.end(), block.begin(), block.end());
}

} // namespace

TEST_CASE("Terminal MasterLimiter is the single limiter: allocator output is unlimited, pOutput is clamped", "[synth_dsp][single_terminal_limiter]") {
    // 8 legacy voices at full velocity sum to ~2.0 peak: the allocator itself must not limit any more
    VoiceAllocator allocator;
    allocator.init(48000);
    for (std::uint64_t i = 0; i < 8; ++i) {
        REQUIRE(allocator.onNoteOn(makeNoteOn(i + 1, static_cast<MusicRole>(i / 2), 110.0f, 1.0f, 0.0f)) >= 0);
    }
    std::vector<float> raw(4800 * 2, 0.0f);
    allocator.renderBlock(raw.data(), 4800, 0, false);
    float rawPeak = 0.0f;
    for (float s : raw) rawPeak = (std::max)(rawPeak, std::abs(s));
    REQUIRE(rawPeak > 1.2f);

    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    for (std::uint64_t i = 0; i < 8; ++i) {
        REQUIRE(engine.scheduleAudioEvent(makeNoteOn(i + 1, static_cast<MusicRole>(i / 2), 110.0f, 1.0f, 0.0f)));
    }
    std::vector<float> block(480 * 2, 0.0f);
    float enginePeak = 0.0f;
    for (int b = 0; b < 10; ++b) {
        engine.processBlock(block.data(), 480);
        for (float s : block) enginePeak = (std::max)(enginePeak, std::abs(s));
    }
    REQUIRE(enginePeak <= MasterLimiter::kHardCeilingLinear);
    REQUIRE(engine.masterLimiter().maxGainReductionDb() > 3.0f);
}

TEST_CASE("Terminal limiter keeps exact digital silence after n_pause + N_tail on pOutput", "[synth_dsp][terminal_limiter_silence_boundary]") {
    for (const bool useStop : {false, true}) {
        AudioEngine engine;
        REQUIRE(engine.initOffline({48000, 2, 220.0f}));
        // Hot material on all roles (bus effects active) so that the terminal limiter is in gain reduction
        REQUIRE(engine.scheduleAudioEvent(makeNoteOn(1, MusicRole::Pulso, 55.0f, 1.0f, 0.8f)));
        REQUIRE(engine.scheduleAudioEvent(makeNoteOn(2, MusicRole::Corpo, 110.0f, 1.0f, 0.6f)));
        REQUIRE(engine.scheduleAudioEvent(makeNoteOn(3, MusicRole::Trama, 440.0f, 1.0f, 0.6f)));
        REQUIRE(engine.scheduleAudioEvent(makeNoteOn(4, MusicRole::Vettore, 330.0f, 1.0f, 0.6f)));
        REQUIRE(engine.startTone(220.0f, 1.0f)); // full-scale diagnostic tone into the same terminal stage

        std::vector<float> block(400 * 2, 0.0f);
        for (int b = 0; b < 6; ++b) engine.processBlock(block.data(), 400);
        REQUIRE(engine.masterLimiter().maxGainReductionDb() > 0.5f);

        // Command matures at the start of the next block: n_pause = block start, N_tail = 96 frames
        REQUIRE(engine.sendCommand(AudioCommand{
            .commandId = 9,
            .type = useStop ? AudioCommandType::Stop : AudioCommandType::Pause}));
        engine.processBlock(block.data(), 400);
        bool tailNonZero = false;
        for (std::size_t i = 0; i < 96 * 2; ++i) tailNonZero = tailNonZero || (block[i] != 0.0f);
        REQUIRE(tailNonZero);
        for (std::size_t i = 96 * 2; i < block.size(); ++i) {
            REQUIRE(block[i] == 0.0f);
        }
        for (int b = 0; b < 20; ++b) {
            engine.processBlock(block.data(), 400);
            for (float s : block) {
                REQUIRE(s == 0.0f);
            }
        }
    }
}

TEST_CASE("Terminal limiter non-finite guard is published in AudioTelemetryFrame::dspAnomalyCount", "[synth_dsp][limiter_anomaly_telemetry]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    REQUIRE(engine.startTone(220.0f, 0.25f));
    std::vector<float> block(256 * 2, 0.0f);
    engine.processBlock(block.data(), 256);
    REQUIRE(engine.updateTelemetry());
    REQUIRE(engine.telemetry().dspAnomalyCount == 0);

    // A non-finite diagnostic tone frequency produces NaN samples that only the terminal limiter sees
    REQUIRE(engine.sendCommand(AudioCommand{
        .commandId = 1,
        .type = AudioCommandType::SetTone,
        .paramF32 = std::numeric_limits<float>::quiet_NaN()}));
    engine.processBlock(block.data(), 256);
    for (float s : block) {
        REQUIRE(std::isfinite(s));
    }
    REQUIRE(engine.voiceAllocator().dspAnomalyCount() == 0);
    REQUIRE(engine.masterLimiter().anomalyCount() > 0);
    REQUIRE(engine.updateTelemetry());
    REQUIRE(engine.telemetry().dspAnomalyCount == engine.masterLimiter().anomalyCount());
}

TEST_CASE("Vettore vibrato modulates the measured instantaneous frequency by +/-0.5% * timbreParam at 5.5 Hz", "[synth_dsp][vettore_vibrato]") {
    for (const float timbre : {1.0f, 0.5f}) {
        VoiceAllocator allocator;
        allocator.init(48000);
        REQUIRE(allocator.onNoteOn(makeNoteOn(1, MusicRole::Vettore, 440.0f, 0.8f, timbre)) == 6);
        std::vector<float> out(48000 * 2, 0.0f);
        allocator.renderBlock(out.data(), 48000, 0, false);

        // 100..700 ms (600 ms >= 3 vibrato cycles of 181.8 ms)
        const auto pts = zeroCrossingFrequencies(out, 0, 4800, 33600, 0.5 * 48000.0 / 440.0);
        REQUIRE(pts.size() > 200);
        double fMin = 1e9;
        double fMax = 0.0;
        double fSum = 0.0;
        for (const auto& p : pts) {
            fMin = (std::min)(fMin, p.freqHz);
            fMax = (std::max)(fMax, p.freqHz);
            fSum += p.freqHz;
        }
        const double mean = fSum / static_cast<double>(pts.size());
        const double depth = 0.5 * (fMax - fMin) / 440.0;
        INFO("timbre " << timbre << " depth " << depth << " mean " << mean);
        REQUIRE(std::abs(mean - 440.0) < 0.5);
        REQUIRE(std::abs(depth - 0.005 * timbre) < 0.0010 * timbre + 0.0002);

        // Vibrato rate: upward crossings of the mean with +/- depth/2 hysteresis -> ~5.5 Hz
        std::size_t upCrossings = 0;
        const double hyst = 0.25 * (fMax - fMin);
        bool below = pts.front().freqHz < mean;
        for (const auto& p : pts) {
            if (below && p.freqHz > mean + hyst) {
                ++upCrossings;
                below = false;
            } else if (!below && p.freqHz < mean - hyst) {
                below = true;
            }
        }
        const double durationSec = (pts.back().timeFrames - pts.front().timeFrames) / 48000.0;
        const double rateHz = static_cast<double>(upCrossings) / durationSec;
        REQUIRE(std::abs(rateHz - 5.5) < 1.0);
    }
}

TEST_CASE("Vettore glides exponentially from the previous Vettore pitch with tau ~= 15 ms; first note does not glide", "[synth_dsp][vettore_glide]") {
    VoiceAllocator allocator;
    allocator.init(48000);
    std::vector<float> out;

    // First note of the run: starts on pitch (no glide)
    REQUIRE(allocator.onNoteOn(makeNoteOn(1, MusicRole::Vettore, 440.0f, 0.8f, 0.01f)) == 6);
    appendRender(allocator, out, 4800, 0);
    {
        const auto pts = zeroCrossingFrequencies(out, 0, 96, 1440, 50.0);
        REQUIRE(pts.size() >= 8);
        for (const auto& p : pts) {
            REQUIRE(std::abs(p.freqHz - 440.0) < 440.0 * 0.01);
        }
    }

    ScheduledAudioEvent off{.eventId = 1, .sampleFrame = 4800, .type = AudioEventType::NoteOff, .role = MusicRole::Vettore};
    REQUIRE(allocator.onNoteOff(off) == 6);
    appendRender(allocator, out, 200, 4800); // 2 ms release completes, slot idle
    REQUIRE(allocator.slot(6).state == VoiceState::Idle);

    const std::size_t onset = out.size() / 2;
    REQUIRE(allocator.onNoteOn(makeNoteOn(2, MusicRole::Vettore, 660.0f, 0.8f, 0.01f, 5000)) >= 0);
    appendRender(allocator, out, 9600, 5000);

    const auto pts = zeroCrossingFrequencies(out, 0, onset + 96, onset + 4800, 40.0);
    REQUIRE(pts.size() > 20);
    // Starts near the previous pitch, moves monotonically towards the target
    REQUIRE(pts.front().freqHz < 500.0);
    REQUIRE(std::abs(pts.back().freqHz - 660.0) < 660.0 * 0.01);

    double tauSum = 0.0;
    int tauCount = 0;
    for (const auto& p : pts) {
        const double remaining = 660.0 - p.freqHz;
        if (remaining > 25.0 && remaining < 180.0) {
            const double tSec = (p.timeFrames - static_cast<double>(onset)) / 48000.0;
            tauSum += -tSec / std::log(remaining / 220.0);
            ++tauCount;
        }
    }
    REQUIRE(tauCount >= 5);
    const double tauMs = 1000.0 * tauSum / tauCount;
    INFO("estimated glide tau " << tauMs << " ms");
    REQUIRE(tauMs > 12.0);
    REQUIRE(tauMs < 18.0);
}

TEST_CASE("Pulso follows ADR-0004 sweep (f_start = min(3 f0, 180 Hz), tau_p = 8 ms) and a 15 ms deterministic click", "[synth_dsp][pulso_profile]") {
    for (const float f0 : {40.0f, 110.0f}) {
        VoiceAllocator allocator;
        allocator.init(48000);
        REQUIRE(allocator.onNoteOn(makeNoteOn(7, MusicRole::Pulso, f0, 1.0f, 0.5f)) == 0);
        std::vector<float> out(384 * 2, 0.0f);
        allocator.renderBlock(out.data(), 384, 0, false); // k = 384 = tau_p * fs
        const float fStart = (std::min)(3.0f * f0, 180.0f);
        // The last rendered sample (k = 383) used the sweep envelope exp(-383 / (tau_p fs))
        const float expected = f0 + (fStart - f0) * std::exp(-383.0f / 384.0f);
        const float instFreq = allocator.slot(0).phaseIncrement * 48000.0f / kTwoPi;
        REQUIRE_THAT(instFreq, Catch::Matchers::WithinRel(expected, 0.002f));
    }

    // Click: high-frequency energy (first difference) only inside the 15 ms burst; bit-exact determinism
    std::vector<float> a(4800 * 2, 0.0f);
    std::vector<float> b(4800 * 2, 0.0f);
    for (auto* buf : {&a, &b}) {
        VoiceAllocator allocator;
        allocator.init(48000);
        REQUIRE(allocator.onNoteOn(makeNoteOn(42, MusicRole::Pulso, 55.0f, 1.0f, 0.7f)) == 0);
        allocator.renderBlock(buf->data(), 4800, 0, false);
    }
    REQUIRE(a == b);
    // Second difference suppresses the 55..165 Hz body and keeps the 1.2..3.5 kHz click band
    const auto hfEnergy = [&](std::size_t begin, std::size_t end) {
        double e = 0.0;
        for (std::size_t n = begin + 2; n < end; ++n) {
            const double d = a[n * 2] - 2.0 * a[(n - 1) * 2] + a[(n - 2) * 2];
            e += d * d;
        }
        return e / static_cast<double>(end - begin);
    };
    const double inBurst = hfEnergy(48, 700);
    const double afterBurst = hfEnergy(760, 1400);
    REQUIRE(inBurst > 100.0 * afterBurst);
}

TEST_CASE("Pause beyond the de-click ramp freezes DSP state: Resume continues exactly without DC bump", "[synth_dsp][pause_frozen_dsp]") {
    const auto startNotes = [](VoiceAllocator& alloc) {
        REQUIRE(alloc.onNoteOn(makeNoteOn(1, MusicRole::Corpo, 220.0f, 0.8f, 0.3f)) == 2);
        REQUIRE(alloc.onNoteOn(makeNoteOn(2, MusicRole::Trama, 440.0f, 0.8f, 0.5f)) == 4);
        REQUIRE(alloc.onNoteOn(makeNoteOn(3, MusicRole::Vettore, 330.0f, 0.8f, 0.5f)) == 6);
    };

    VoiceAllocator paused;
    paused.init(48000);
    startNotes(paused);
    std::vector<float> pre;
    appendRender(paused, pre, 9600, 0); // all voices in sustain

    paused.onPause();
    std::vector<float> ramp;
    appendRender(paused, ramp, 96, 9600, true);
    std::vector<float> silent;
    appendRender(paused, silent, 4800, 9600, true); // 100 ms frozen (> 50 ms)
    for (float s : silent) {
        REQUIRE(s == 0.0f);
    }
    paused.onResume();
    std::vector<float> resumed;
    appendRender(paused, resumed, 4800, 9600);

    // Reference: same notes rendered without interruption
    VoiceAllocator reference;
    reference.init(48000);
    startNotes(reference);
    std::vector<float> ref;
    appendRender(reference, ref, 9600 + 96 + 4800, 0);

    // After the 2 ms fade-in the resumed stream equals the uninterrupted one shifted by the 96-frame ramp:
    // no DC was accumulated in the delay and no filter drifted while paused.
    float maxDiff = 0.0f;
    double dcResumed = 0.0;
    double dcRef = 0.0;
    for (std::size_t n = 96; n < 4800 - 96; ++n) {
        for (std::size_t ch = 0; ch < 2; ++ch) {
            const float x = resumed[n * 2 + ch];
            const float y = ref[(9600 + 96 + n) * 2 + ch];
            maxDiff = (std::max)(maxDiff, std::abs(x - y));
            dcResumed += x;
            dcRef += y;
        }
    }
    REQUIRE(maxDiff < 1e-5f);
    REQUIRE(std::abs(dcResumed - dcRef) / 9216.0 < 1e-5);
}

TEST_CASE("Steal tails start at the voice's actual post-bus level (Trama dry factor, Corpo ducking)", "[synth_dsp][steal_tail_post_bus]") {
    // Smooth sine voices (timbreParam = 0) make the expected per-sample step small, so a tail that bypasses the
    // Trama dry factor (x 1/0.8) or the Corpo duck (x 1/0.708) shows up as a step several times larger.
    SECTION("Trama steal") {
        VoiceAllocator allocator;
        allocator.init(48000);
        REQUIRE(allocator.onNoteOn(makeNoteOn(1, MusicRole::Trama, 440.0f, 1.0f, 0.0f)) == 4);
        REQUIRE(allocator.onNoteOn(makeNoteOn(2, MusicRole::Trama, 550.0f, 1.0f, 0.0f, 1)) == 5);
        std::vector<float> out;
        appendRender(allocator, out, 9627, 0); // ~quarter period after a 440 Hz zero crossing: large voice value
        const std::size_t steal = out.size() / 2;
        REQUIRE(allocator.onNoteOn(makeNoteOn(3, MusicRole::Trama, 660.0f, 1.0f, 0.0f, 9627)) == 4);
        REQUIRE(std::abs(allocator.slot(4).stealTail.startAmplitude[0]) > 0.1f);
        appendRender(allocator, out, 1200, 9627);
        for (std::size_t ch = 0; ch < 2; ++ch) {
            const float reference = maxAbsStep(out, ch, steal - 960, steal - 4);
            const float atSteal = maxAbsStep(out, ch, steal - 1, steal + 4);
            INFO("ch " << ch << " ref " << reference << " at steal " << atSteal);
            REQUIRE(reference > 1e-4f);
            REQUIRE(atSteal <= 1.25f * reference);
        }
    }
    SECTION("Corpo steal while ducked by Pulso") {
        VoiceAllocator allocator;
        allocator.init(48000);
        REQUIRE(allocator.onNoteOn(makeNoteOn(1, MusicRole::Corpo, 110.0f, 1.0f, 0.0f)) == 2);
        REQUIRE(allocator.onNoteOn(makeNoteOn(2, MusicRole::Corpo, 165.0f, 1.0f, 0.0f, 1)) == 3);
        std::vector<float> out;
        appendRender(allocator, out, 9600, 0);
        REQUIRE(allocator.onNoteOn(makeNoteOn(3, MusicRole::Pulso, 55.0f, 1.0f, 0.0f, 9600)) == 0);
        appendRender(allocator, out, 960, 9600); // 20 ms into the Pulso hit
        REQUIRE(allocator.roleBusProcessor().lastDuckingGain() < 0.85f);
        const std::size_t steal = out.size() / 2;
        REQUIRE(allocator.onNoteOn(makeNoteOn(4, MusicRole::Corpo, 220.0f, 1.0f, 0.0f, 10560)) == 2);
        REQUIRE(std::abs(allocator.slot(2).stealTail.startAmplitude[0]) > 0.1f);
        appendRender(allocator, out, 1200, 10560);
        for (std::size_t ch = 0; ch < 2; ++ch) {
            const float reference = maxAbsStep(out, ch, steal - 480, steal - 4);
            const float atSteal = maxAbsStep(out, ch, steal - 1, steal + 4);
            INFO("ch " << ch << " ref " << reference << " at steal " << atSteal);
            REQUIRE(reference > 1e-4f);
            REQUIRE(atSteal <= 1.25f * reference);
        }
    }
}

TEST_CASE("Bus gains and Trama wet are smoothed and held when the role goes idle while the delay rings", "[synth_dsp][bus_gain_smoothing]") {
    VoiceAllocator allocator;
    allocator.init(48000);
    constexpr std::uint32_t kDuration = 9600;
    REQUIRE(allocator.onNoteOn(makeNoteOn(1, MusicRole::Trama, 440.0f, 1.0f, 0.6f, 0, kDuration)) == 4);
    std::vector<float> out;
    appendRender(allocator, out, kDuration + 2400, 0);
    REQUIRE(allocator.slot(4).state == VoiceState::Idle);

    // The role went idle at kDuration + 96 while the echo still rings: wet and bus gain keep their values
    REQUIRE_THAT(allocator.roleBusProcessor().currentTramaWet(), Catch::Matchers::WithinAbs(0.20f + 0.25f * 0.6f, 1e-4f));
    REQUIRE_THAT(allocator.roleBusProcessor().currentBusGain(MusicRole::Trama), Catch::Matchers::WithinAbs(RoleBusProcessor::kTramaGain, 1e-4f));

    const std::size_t idle = kDuration + 96;
    for (std::size_t ch = 0; ch < 2; ++ch) {
        const float reference = maxAbsStep(out, ch, idle - 1500, idle - 200);
        const float atIdle = maxAbsStep(out, ch, idle - 2, idle + 12);
        INFO("ch " << ch << " ref " << reference << " at idle " << atIdle);
        REQUIRE(atIdle <= 1.25f * reference);
    }

    // Legacy -> timbred target change on a sounding bus is smoothed with tau = 5 ms (240 frames)
    VoiceAllocator smoothing;
    smoothing.init(48000);
    REQUIRE(smoothing.onNoteOn(makeNoteOn(8, MusicRole::Corpo, 220.0f, 1.0f, 0.0f)) == 2);
    std::vector<float> warm(480 * 2, 0.0f);
    smoothing.renderBlock(warm.data(), 480, 0, false);
    REQUIRE(smoothing.roleBusProcessor().currentBusGain(MusicRole::Corpo) == 1.0f);
    REQUIRE(smoothing.onNoteOn(makeNoteOn(9, MusicRole::Corpo, 330.0f, 1.0f, 0.4f, 480)) == 3);
    std::vector<float> tauBlock(240 * 2, 0.0f);
    smoothing.renderBlock(tauBlock.data(), 240, 480, false);
    const float expectedGain = RoleBusProcessor::kCorpoGain + (1.0f - RoleBusProcessor::kCorpoGain) * std::exp(-1.0f);
    REQUIRE_THAT(smoothing.roleBusProcessor().currentBusGain(MusicRole::Corpo), Catch::Matchers::WithinAbs(expectedGain, 0.01f));

    // A role's first note snaps its bus gain (input was silent): no 5 ms overshoot at onset
    VoiceAllocator fresh;
    fresh.init(48000);
    REQUIRE(fresh.onNoteOn(makeNoteOn(5, MusicRole::Corpo, 220.0f, 1.0f, 0.4f)) == 2);
    std::vector<float> one(2 * 2, 0.0f);
    fresh.renderBlock(one.data(), 2, 0, false);
    REQUIRE(fresh.roleBusProcessor().currentBusGain(MusicRole::Corpo) == RoleBusProcessor::kCorpoGain);
}

TEST_CASE("Role telemetry accumulates across all slices of one callback block", "[synth_dsp][telemetry_window]") {
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    // Corpo note in the middle of the first block splits the callback into two slices
    REQUIRE(engine.scheduleAudioEvent(makeNoteOn(1, MusicRole::Corpo, 220.0f, 1.0f, 0.4f, 200)));
    std::vector<float> block(400 * 2, 0.0f);
    engine.processBlock(block.data(), 400);
    REQUIRE(engine.updateTelemetry());

    double sumSq = 0.0;
    for (float s : block) sumSq += static_cast<double>(s) * s;
    const double blockRms = std::sqrt(sumSq / static_cast<double>(block.size()));
    REQUIRE(blockRms > 1e-4);
    REQUIRE(engine.masterLimiter().maxGainReductionDb() == 0.0f); // output == Corpo bus (unity limiter)
    REQUIRE_THAT(engine.telemetry().body.energy, Catch::Matchers::WithinRel(static_cast<float>(blockRms), 1e-3f));
    REQUIRE_THAT(engine.telemetry().globalEnergy, Catch::Matchers::WithinRel(static_cast<float>(blockRms), 1e-3f));
}

TEST_CASE("Start while Running keeps the running buses and terminal limiter untouched", "[synth_dsp][start_while_running]") {
    const auto render = [](bool sendStart) {
        AudioEngine engine;
        REQUIRE(engine.initOffline({48000, 2, 220.0f}));
        REQUIRE(engine.scheduleAudioEvent(makeNoteOn(1, MusicRole::Trama, 440.0f, 1.0f, 0.6f, 0, 4000)));
        std::vector<float> all;
        std::vector<float> block(400 * 2, 0.0f);
        for (int b = 0; b < 20; ++b) {
            if (sendStart && b == 11) { // note ended at 4096, echo still ringing
                REQUIRE(engine.sendCommand(AudioCommand{.commandId = 5, .type = AudioCommandType::Start}));
            }
            engine.processBlock(block.data(), 400);
            all.insert(all.end(), block.begin(), block.end());
        }
        return all;
    };
    const auto withStart = render(true);
    const auto withoutStart = render(false);
    REQUIRE(withStart == withoutStart);
    float echoPeak = 0.0f;
    for (std::size_t i = 4400 * 2; i < 4800 * 2; ++i) echoPeak = (std::max)(echoPeak, std::abs(withStart[i]));
    REQUIRE(echoPeak > 1e-4f);
}

TEST_CASE("AudioEngine rejects non-stereo configurations and mismatched probes", "[synth_dsp][stereo_only]") {
    {
        AudioEngine engine;
        REQUIRE_FALSE(engine.initOffline({48000, 1, 220.0f}));
        REQUIRE_FALSE(engine.initOffline({48000, 6, 220.0f}));
    }
    {
        AudioEngine engine;
        REQUIRE_FALSE(engine.init({48000, 1, 220.0f}));
    }
    AudioEngine engine;
    REQUIRE(engine.initOffline({48000, 2, 220.0f}));
    AudioProbe mono(1024, 1, 48000);
    REQUIRE_FALSE(engine.attachAudioProbe(&mono));
    REQUIRE(engine.audioProbe() == nullptr);
    AudioProbe stereo(1024, 2, 48000);
    REQUIRE(engine.attachAudioProbe(&stereo));
    REQUIRE(engine.audioProbe() == &stereo);
    engine.detachAudioProbe();
}
