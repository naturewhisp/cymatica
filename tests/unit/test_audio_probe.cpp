#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "audio_engine.h"
#include "audio_probe.h"
#include "input_dispatcher.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace cymatica::audio;

namespace {

constexpr float kPi = 3.14159265358979323846f;

std::vector<float> generateSineWave(float freqHz, float amplitude, std::uint32_t sampleRate, std::size_t sampleCount) {
    std::vector<float> samples(sampleCount);
    for (std::size_t i = 0; i < sampleCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        samples[i] = amplitude * std::sin(2.0f * kPi * freqHz * t);
    }
    return samples;
}

} // namespace

TEST_CASE("AudioProbe: ring buffer basic read and write", "[audio][probe]") {
    AudioProbe probe(1024, 2, 48000);
    REQUIRE(probe.capacityFrames() == 1024);
    REQUIRE(probe.channels() == 2);
    REQUIRE(probe.sampleRate() == 48000);
    REQUIRE(probe.availableFrames() == 0);
    REQUIRE(probe.droppedFrames() == 0);

    // Write 256 stereo frames
    std::vector<float> inData(256 * 2);
    for (std::size_t i = 0; i < inData.size(); ++i) {
        inData[i] = static_cast<float>(i) * 0.001f;
    }
    probe.writeFrames(inData.data(), 256);
    CHECK(probe.availableFrames() == 256);
    CHECK(probe.droppedFrames() == 0);

    // Read 256 stereo frames
    std::vector<float> outData(256 * 2, 0.0f);
    const std::uint32_t readCount = probe.readFrames(outData.data(), 256);
    CHECK(readCount == 256);
    CHECK(probe.availableFrames() == 0);

    for (std::size_t i = 0; i < inData.size(); ++i) {
        CHECK(outData[i] == inData[i]);
    }
}

TEST_CASE("AudioProbe: buffer wrap-around preserves sample integrity", "[audio][probe]") {
    // Capacity 500 frames
    AudioProbe probe(500, 2, 48000);

    // Write 400 frames
    std::vector<float> block1(400 * 2, 1.25f);
    probe.writeFrames(block1.data(), 400);

    // Read 300 frames (100 frames remaining, writeHead = 400, readHead = 300)
    std::vector<float> readBuf(500 * 2);
    CHECK(probe.readFrames(readBuf.data(), 300) == 300);
    CHECK(probe.availableFrames() == 100);

    // Write 300 frames -> this will wrap around the 500-frame boundary!
    std::vector<float> block2(300 * 2);
    for (std::size_t i = 0; i < block2.size(); ++i) {
        block2[i] = static_cast<float>(i + 1);
    }
    probe.writeFrames(block2.data(), 300);
    CHECK(probe.availableFrames() == 400);
    CHECK(probe.droppedFrames() == 0);

    // Read the 100 leftover frames
    CHECK(probe.readFrames(readBuf.data(), 100) == 100);
    for (std::size_t i = 0; i < 100 * 2; ++i) {
        CHECK(readBuf[i] == 1.25f);
    }

    // Read the 300 wrapped frames
    CHECK(probe.readFrames(readBuf.data(), 300) == 300);
    for (std::size_t i = 0; i < 300 * 2; ++i) {
        CHECK(readBuf[i] == block2[i]);
    }
    CHECK(probe.availableFrames() == 0);
}

TEST_CASE("AudioProbe: consumer stall triggers non-blocking drops without corruption", "[audio][probe][stall]") {
    // Capacity 1000 frames
    AudioProbe probe(1000, 2, 48000);

    // Fill the buffer to capacity
    std::vector<float> fill(1000 * 2, 0.5f);
    probe.writeFrames(fill.data(), 1000);
    CHECK(probe.availableFrames() == 1000);
    CHECK(probe.droppedFrames() == 0);

    // Write 500 more frames while consumer is stalled: must drop without blocking or crashing
    std::vector<float> overflow(500 * 2, 9.9f);
    probe.writeFrames(overflow.data(), 500);
    CHECK(probe.droppedFrames() == 500);
    CHECK(probe.availableFrames() == 1000); // existing frames intact

    // Existing buffer remains valid
    std::vector<float> out(1000 * 2);
    CHECK(probe.readFrames(out.data(), 1000) == 1000);
    for (std::size_t i = 0; i < 1000 * 2; ++i) {
        CHECK(out[i] == 0.5f);
    }
}

TEST_CASE("Quantitative audio analysis: pitch and RMS estimation", "[audio][analysis]") {
    constexpr std::uint32_t kSampleRate = 48000;
    constexpr std::size_t kWindowSamples = 12000; // 250 ms @ 48 kHz

    SECTION("220 Hz synthetic sine estimation") {
        const auto sine220 = generateSineWave(220.0f, 0.50f, kSampleRate, kWindowSamples);
        const float pitch = analysis::estimatePitchHz(sine220.data(), sine220.size(), kSampleRate);
        const float rms = analysis::computeRmsDbfs(sine220.data(), sine220.size());

        // Target: 220.0 Hz +/- 0.5 Hz
        CHECK_THAT(pitch, Catch::Matchers::WithinAbs(220.0f, 0.5f));
        // Sine with peak 0.5 has RMS 0.5 / sqrt(2) = 0.35355 => ~ -9.03 dBFS
        CHECK_THAT(rms, Catch::Matchers::WithinAbs(-9.03f, 1.0f));
    }

    SECTION("440 Hz synthetic sine estimation") {
        const auto sine440 = generateSineWave(440.0f, 0.20f, kSampleRate, kWindowSamples);
        const float pitch = analysis::estimatePitchHz(sine440.data(), sine440.size(), kSampleRate);
        const float rms = analysis::computeRmsDbfs(sine440.data(), sine440.size());

        // Target: 440.0 Hz +/- 0.5 Hz
        CHECK_THAT(pitch, Catch::Matchers::WithinAbs(440.0f, 0.5f));
        // Sine with peak 0.2 has RMS 0.2 / sqrt(2) = 0.1414 => ~ -16.99 dBFS
        CHECK_THAT(rms, Catch::Matchers::WithinAbs(-16.99f, 1.0f));
    }

    SECTION("Silence signal estimation") {
        std::vector<float> silence(kWindowSamples, 0.0f);
        const float pitch = analysis::estimatePitchHz(silence.data(), silence.size(), kSampleRate);
        const float rms = analysis::computeRmsDbfs(silence.data(), silence.size());

        CHECK(pitch == 0.0f);
        CHECK(rms <= -100.0f);
    }
}

TEST_CASE("WAV export: 16-bit stereo WAV generation", "[audio][wav]") {
    const std::string tmpPath = "artifacts/test_audio/test_export.wav";
    constexpr std::uint32_t kSampleRate = 48000;
    constexpr std::size_t kFrames = 4800; // 0.1 s

    std::vector<float> stereoData(kFrames * 2, 0.25f);
    const bool ok = analysis::writeWavStereo16(tmpPath, stereoData.data(), kFrames, kSampleRate);
    REQUIRE(ok);
    REQUIRE(std::filesystem::exists(tmpPath));

    const auto fileSize = std::filesystem::file_size(tmpPath);
    // 44-byte header + 4800 frames * 2 channels * 2 bytes = 44 + 19200 = 19244 bytes
    CHECK(fileSize == 44 + (kFrames * 2 * 2));

    std::ifstream in(tmpPath, std::ios::binary);
    REQUIRE(in.is_open());
    char tag[4];
    in.read(tag, 4);
    CHECK(std::string(tag, 4) == "RIFF");

    in.close();
    std::filesystem::remove(tmpPath);
}

TEST_CASE("AC-AO-01A: authoritative tone_probe_v0 execution and verification", "[cacp][probe][ac-ao-01a]") {
    const std::string outDir = "artifacts/cacp_test";
    const auto result = runToneProbeV0Offline(outDir, true);

    INFO("ToneProbeResult JSON:\n" << result.toJson());
    if (!result.passed) {
        FAIL("tone_probe_v0 failed: " << result.failureReason);
    }

    REQUIRE(result.passed);
    REQUIRE(result.droppedFrames == 0);
    REQUIRE(result.totalCapturedFrames >= 144000); // 3.0 s @ 48 kHz

    // Phase 1 Baseline: 220 Hz +/- 2 Hz, RMS > -40 dBFS
    CHECK(result.phase1Baseline.freqPassed);
    CHECK(result.phase1Baseline.rmsPassed);
    CHECK(result.phase1Baseline.passed);
    CHECK_THAT(result.phase1Baseline.estimatedFreqHz, Catch::Matchers::WithinAbs(220.0f, 2.0f));
    CHECK(result.phase1Baseline.rmsDbfs > -40.0f);

    // Phase 2 Hold: 440 Hz +/- 2 Hz, RMS > -40 dBFS
    CHECK(result.phase2Hold.freqPassed);
    CHECK(result.phase2Hold.rmsPassed);
    CHECK(result.phase2Hold.passed);
    CHECK_THAT(result.phase2Hold.estimatedFreqHz, Catch::Matchers::WithinAbs(440.0f, 2.0f));
    CHECK(result.phase2Hold.rmsDbfs > -40.0f);

    // Phase 3 Release: 220 Hz +/- 2 Hz, RMS > -40 dBFS
    CHECK(result.phase3Release.freqPassed);
    CHECK(result.phase3Release.rmsPassed);
    CHECK(result.phase3Release.passed);
    CHECK_THAT(result.phase3Release.estimatedFreqHz, Catch::Matchers::WithinAbs(220.0f, 2.0f));
    CHECK(result.phase3Release.rmsDbfs > -40.0f);

    // Verify artifacts were created on disk
    REQUIRE(std::filesystem::exists(result.wavFilePath));
    REQUIRE(std::filesystem::exists(result.manifestFilePath));
    CHECK(std::filesystem::file_size(result.wavFilePath) >= 144000 * 4); // 44 + 144000 * 4
    CHECK(std::filesystem::file_size(result.manifestFilePath) > 100);

    // Clean up temporary test artifacts
    std::filesystem::remove(result.wavFilePath);
    std::filesystem::remove(result.manifestFilePath);
}
