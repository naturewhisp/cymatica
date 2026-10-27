#include <catch2/catch_test_macros.hpp>

#include "fft.h"
#include "loudness.h"
#include "mel.h"
#include "onset.h"
#include "png_writer.h"
#include "sha256.h"
#include "stereo.h"
#include "stft.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <functional>
#include <limits>
#include <numbers>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace cymatica::cacp;

namespace {

// Deterministic pseudo-random sequence (xorshift64*), independent of the engine RNG
struct TestRng {
    std::uint64_t s{0x9E3779B97F4A7C15ULL};
    double uniform() noexcept {
        s ^= s >> 12; s ^= s << 25; s ^= s >> 27;
        return static_cast<double>((s * 2685821657736338717ULL) >> 11) * (1.0 / 9007199254740992.0);
    }
};

std::vector<std::complex<double>> naiveDft(const std::vector<std::complex<double>>& x) {
    const std::size_t n = x.size();
    std::vector<std::complex<double>> out(n);
    for (std::size_t k = 0; k < n; ++k) {
        std::complex<double> acc{0.0, 0.0};
        for (std::size_t t = 0; t < n; ++t) {
            const double a = -2.0 * std::numbers::pi * static_cast<double>((k * t) % n) / static_cast<double>(n);
            acc += x[t] * std::complex<double>(std::cos(a), std::sin(a));
        }
        out[k] = acc;
    }
    return out;
}

// Stereo sine, in phase on L and R, with the given per-channel peak level (EBU Tech 3341 test signals)
void appendSine(std::vector<float>& stereo, double seconds, double peakDbfs, double hz = 1000.0, std::uint32_t fs = 48000) {
    const double a = std::pow(10.0, peakDbfs / 20.0);
    const auto n = static_cast<std::size_t>(std::llround(seconds * fs));
    for (std::size_t i = 0; i < n; ++i) {
        const auto v = static_cast<float>(a * std::sin(2.0 * std::numbers::pi * hz * static_cast<double>(i) / fs));
        stereo.push_back(v);
        stereo.push_back(v);
    }
}

void appendSilence(std::vector<float>& stereo, double seconds, std::uint32_t fs = 48000) {
    stereo.insert(stereo.end(), static_cast<std::size_t>(std::llround(seconds * fs)) * 2, 0.0f);
}

std::uint32_t readU32Be(const std::vector<std::uint8_t>& b, std::size_t at) {
    return (static_cast<std::uint32_t>(b[at]) << 24) | (static_cast<std::uint32_t>(b[at + 1]) << 16) |
           (static_cast<std::uint32_t>(b[at + 2]) << 8) | static_cast<std::uint32_t>(b[at + 3]);
}

// Independent PNG/zlib reader for stored-only streams: validates every structural field and returns the raw bytes
struct ParsedPng {
    bool ok{false};
    std::uint32_t width{0}, height{0};
    std::uint8_t bitDepth{0}, colourType{0}, interlace{0};
    std::vector<std::uint8_t> raw{};
    std::size_t storedBlocks{0};
};

ParsedPng parseStoredPng(const std::vector<std::uint8_t>& png) {
    ParsedPng p;
    const std::uint8_t sig[8]{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if (png.size() < 8 || !std::equal(sig, sig + 8, png.begin())) return p;
    std::size_t pos = 8;
    std::vector<std::uint8_t> idat;
    bool sawEnd = false;
    while (pos + 12 <= png.size()) {
        const std::uint32_t len = readU32Be(png, pos);
        const std::string type(png.begin() + static_cast<std::ptrdiff_t>(pos + 4), png.begin() + static_cast<std::ptrdiff_t>(pos + 8));
        if (pos + 12 + len > png.size()) return p;
        const std::uint32_t crcStored = readU32Be(png, pos + 8 + len);
        const std::uint32_t crcActual = crc32(std::span<const std::uint8_t>(png.data() + pos + 4, 4 + len));
        if (crcStored != crcActual) return p;
        const std::uint8_t* data = png.data() + pos + 8;
        if (type == "IHDR") {
            if (len != 13) return p;
            p.width = readU32Be(png, pos + 8);
            p.height = readU32Be(png, pos + 12);
            p.bitDepth = data[8];
            p.colourType = data[9];
            if (data[10] != 0 || data[11] != 0) return p; // compression 0 (deflate), filter method 0
            p.interlace = data[12];
        } else if (type == "IDAT") {
            idat.insert(idat.end(), data, data + len);
        } else if (type == "IEND") {
            sawEnd = (len == 0);
        } else {
            return p; // the writer emits no other chunk
        }
        pos += 12 + len;
    }
    if (!sawEnd || pos != png.size() || idat.size() < 6) return p;
    // zlib header (RFC 1950): deflate, 32 KiB window, FCHECK multiple of 31, no preset dictionary
    if (idat[0] != 0x78 || ((idat[0] * 256 + idat[1]) % 31) != 0 || (idat[1] & 0x20) != 0) return p;
    std::size_t z = 2;
    for (;;) {
        if (z + 5 > idat.size()) return p;
        const std::uint8_t hdr = idat[z];
        if ((hdr & 0x06) != 0) return p; // BTYPE must be 00 (stored)
        const auto len = static_cast<std::uint16_t>(idat[z + 1] | (idat[z + 2] << 8));
        const auto nlen = static_cast<std::uint16_t>(idat[z + 3] | (idat[z + 4] << 8));
        if (static_cast<std::uint16_t>(~len) != nlen || z + 5 + len > idat.size()) return p;
        p.raw.insert(p.raw.end(), idat.begin() + static_cast<std::ptrdiff_t>(z + 5),
                     idat.begin() + static_cast<std::ptrdiff_t>(z + 5 + len));
        ++p.storedBlocks;
        z += 5 + len;
        if (hdr & 0x01) break; // BFINAL
    }
    if (z + 4 != idat.size()) return p;
    if (readU32Be(idat, z) != adler32(p.raw)) return p;
    p.ok = true;
    return p;
}

std::size_t bruteForceMaxMatching(const std::vector<double>& ref, const std::vector<double>& est, double tol) {
    std::vector<bool> used(est.size(), false);
    std::function<std::size_t(std::size_t)> go = [&](std::size_t i) -> std::size_t {
        if (i == ref.size()) return 0;
        std::size_t best = go(i + 1); // leave ref i unmatched
        for (std::size_t j = 0; j < est.size(); ++j) {
            if (!used[j] && std::abs(est[j] - ref[i]) <= tol) {
                used[j] = true;
                best = (std::max)(best, 1 + go(i + 1));
                used[j] = false;
            }
        }
        return best;
    };
    return go(0);
}

} // namespace

TEST_CASE("FFT radix-2 matches a direct DFT and inverts (D-M2-14)", "[cacp_analysis][fft]") {
    TestRng rng;
    for (std::size_t n : {1u, 2u, 4u, 8u, 64u, 256u, 2048u}) {
        std::vector<std::complex<double>> x(n);
        for (auto& v : x) v = {rng.uniform() * 2.0 - 1.0, rng.uniform() * 2.0 - 1.0};
        const auto ref = naiveDft(x);
        auto y = x;
        REQUIRE(fftRadix2(y));
        double maxErr = 0.0;
        for (std::size_t k = 0; k < n; ++k) maxErr = (std::max)(maxErr, std::abs(y[k] - ref[k]));
        INFO("n " << n << " max error " << maxErr);
        CHECK(maxErr < 1e-9 * static_cast<double>(n));

        REQUIRE(fftRadix2(y, true));
        double roundTrip = 0.0;
        for (std::size_t k = 0; k < n; ++k) roundTrip = (std::max)(roundTrip, std::abs(y[k] / static_cast<double>(n) - x[k]));
        CHECK(roundTrip < 1e-12);
    }
    std::vector<std::complex<double>> bad(6);
    CHECK_FALSE(fftRadix2(bad));
    CHECK_FALSE(FftPlan(1000).valid());
}

TEST_CASE("STFT framing, symmetric Hann window and frame-centre convention", "[cacp_analysis][stft]") {
    const auto w = hannSymmetric(2048);
    CHECK(w.front() == 0.0);
    CHECK(std::abs(w.back()) < 1e-15);
    CHECK(std::abs(w[1023] - w[1024]) < 1e-12); // symmetric around (N - 1) / 2

    CHECK(stftFrameCount(2047, kDescriptorsV1Stft) == 0);
    CHECK(stftFrameCount(2048, kDescriptorsV1Stft) == 1);
    CHECK(stftFrameCount(2048 + 256, kDescriptorsV1Stft) == 2);
    CHECK(frameCentreSeconds(0, kDescriptorsV1Stft, 48000) == 1024.0 / 48000.0);
    CHECK(frameCentreSeconds(10, kDescriptorsV1Stft, 48000) == (2560.0 + 1024.0) / 48000.0);

    // A sine exactly on bin 100 (100 * 48000 / 2048 Hz) peaks at bin 100 in every frame
    std::vector<float> mono(48000);
    for (std::size_t i = 0; i < mono.size(); ++i) {
        mono[i] = static_cast<float>(std::sin(2.0 * std::numbers::pi * 100.0 * static_cast<double>(i) / 2048.0));
    }
    const auto mag = stftMagnitude(mono);
    REQUIRE(mag.frames == stftFrameCount(mono.size(), kDescriptorsV1Stft));
    REQUIRE(mag.bins == 1025);
    for (std::size_t k = 0; k < mag.frames; k += 17) {
        std::size_t peak = 0;
        for (std::size_t b = 1; b < mag.bins; ++b) if (mag.at(k, b) > mag.at(k, peak)) peak = b;
        CHECK(peak == 100);
    }

    const std::vector<float> stereo{1.0f, 0.0f, -0.5f, 0.5f};
    const auto m = monoFromStereo(stereo);
    REQUIRE(m.size() == 2);
    CHECK(m[0] == 0.5f);
    CHECK(m[1] == 0.0f);
}

TEST_CASE("Onset detection finds synthetic attacks at known frames (D-M2-14)", "[cacp_analysis][onset]") {
    constexpr std::uint32_t fs = 48000;
    const std::vector<double> onsets{0.50, 0.95, 1.40, 2.05, 2.60, 3.30};
    std::vector<float> mono(static_cast<std::size_t>(4.0 * fs), 0.0f);
    for (const double t0 : onsets) {
        const auto start = static_cast<std::size_t>(std::llround(t0 * fs));
        for (std::size_t i = 0; i < static_cast<std::size_t>(0.40 * fs) && start + i < mono.size(); ++i) {
            const double t = static_cast<double>(i) / fs;
            const double env = (std::min)(t / 0.001, 1.0) * std::exp(-t / 0.04); // 1 ms attack, 40 ms decay: ~-87 dB at the 0.4 s cut
            mono[start + i] += static_cast<float>(0.5 * env * std::sin(2.0 * std::numbers::pi * 660.0 * t));
        }
    }
    const auto est = detectOnsetsSeconds(mono, OnsetParams{}, fs);
    const auto score = matchOnsetsMaxCardinality(onsets, est, 0.025);
    std::string detectedList;
    for (const double t : est) detectedList += std::to_string(t) + " ";
    INFO("detected " << est.size() << ": " << detectedList);
    CHECK(score.matches == onsets.size());
    CHECK(score.precision == 1.0);
    for (const auto& pair : score.pairs) CHECK(std::abs(pair.errorS) <= 0.025);

    // Silence gives no onset and a zero novelty curve
    const std::vector<float> silence(fs, 0.0f);
    CHECK(detectOnsetsSeconds(silence, OnsetParams{}, fs).empty());

    // Two detections closer than the minimum separation are merged into the larger peak
    std::vector<double> nov(400, 0.0);
    nov[100] = 0.8; nov[103] = 1.0; // 3 hops = 16 ms < 30 ms
    nov[300] = 0.9;
    const auto frames = pickOnsetFrames(nov, OnsetParams{}, kDescriptorsV1Stft, fs);
    REQUIRE(frames.size() == 2);
    CHECK(frames[0] == 103);
    CHECK(frames[1] == 300);
}

TEST_CASE("Onset matching: maximum cardinality, inclusive tolerance, P/R/F (mir_eval convention)", "[cacp_analysis][onset_matching]") {
    // Greedy nearest assignment is sub-optimal here; maximum cardinality is not
    const std::vector<double> ref{0.00, 0.04};
    const std::vector<double> est{-0.04, 0.03};
    CHECK(matchOnsetsGreedyNearest(ref, est, 0.05).matches == 1);
    const auto best = matchOnsetsMaxCardinality(ref, est, 0.05);
    CHECK(best.matches == 2);
    CHECK(best.precision == 1.0);
    CHECK(best.recall == 1.0);
    CHECK(best.fMeasure == 1.0);

    // Inclusive edge with exactly representable values: |0.5625 - 0.5| == 0.0625
    CHECK(matchOnsetsMaxCardinality(std::vector<double>{0.5}, std::vector<double>{0.5625}, 0.0625).matches == 1);
    CHECK(matchOnsetsMaxCardinality(std::vector<double>{0.5}, std::vector<double>{0.5625}, 0.0624).matches == 0);

    // Empty inputs
    const auto empty = matchOnsetsMaxCardinality({}, std::vector<double>{0.1});
    CHECK(empty.matches == 0);
    CHECK(empty.precision == 0.0);
    CHECK(empty.fMeasure == 0.0);

    // Against an exhaustive search on random small instances
    TestRng rng;
    for (int trial = 0; trial < 400; ++trial) {
        std::vector<double> r, e;
        const int nr = 1 + static_cast<int>(rng.uniform() * 6);
        const int ne = 1 + static_cast<int>(rng.uniform() * 6);
        for (int i = 0; i < nr; ++i) r.push_back(rng.uniform() * 0.4);
        for (int i = 0; i < ne; ++i) e.push_back(rng.uniform() * 0.4);
        const auto fast = matchOnsetsMaxCardinality(r, e, 0.05);
        INFO("trial " << trial);
        CHECK(fast.matches == bruteForceMaxMatching(r, e, 0.05));
        for (const auto& pair : fast.pairs) CHECK(std::abs(pair.errorS) <= 0.05);
    }
}

TEST_CASE("Mel filterbank (HTK) and normalised mel spectrogram (D-M2-14)", "[cacp_analysis][mel]") {
    CHECK(std::abs(hzToMelHtk(700.0) - 2595.0 * std::log10(2.0)) < 1e-12);
    CHECK(std::abs(hzToMelHtk(0.0)) < 1e-12);
    for (double f : {30.0, 440.0, 1000.0, 16000.0}) CHECK(std::abs(melToHzHtk(hzToMelHtk(f)) - f) < 1e-9);

    const auto fb = melFilterbank(kDescriptorsV1Mel, kDescriptorsV1Stft, 48000);
    REQUIRE(fb.bands == 128);
    REQUIRE(fb.bins == 1025);
    REQUIRE(fb.centreHz.size() == 128);
    CHECK(fb.centreHz.front() > 30.0);
    CHECK(fb.centreHz.back() < 16000.0);
    for (std::size_t m = 1; m < fb.centreHz.size(); ++m) CHECK(fb.centreHz[m] > fb.centreHz[m - 1]);
    for (const double w : fb.weights) {
        CHECK(w >= 0.0);
        CHECK(w <= 1.0);
    }
    // No weight outside [fMin, fMax]
    for (std::size_t b = 0; b < fb.bins; ++b) {
        const double f = static_cast<double>(b) * 48000.0 / 2048.0;
        if (f < 30.0 || f > 16000.0) {
            for (std::size_t m = 0; m < fb.bands; ++m) CHECK(fb.weights[m * fb.bins + b] == 0.0);
        }
    }

    // A 1 kHz sine lights the band whose centre is nearest to 1 kHz; the matrix is normalised to [0, 1]
    std::vector<float> mono(24000);
    for (std::size_t i = 0; i < mono.size(); ++i) mono[i] = static_cast<float>(0.5 * std::sin(2.0 * std::numbers::pi * 1000.0 * static_cast<double>(i) / 48000.0));
    const auto mel = melSpectrogramNormalized(stftMagnitude(mono), fb, 80.0);
    REQUIRE(mel.frames > 0);
    REQUIRE(mel.bands == 128);
    std::size_t peak = 0;
    for (std::size_t m = 1; m < mel.bands; ++m) if (mel.normalized[m] > mel.normalized[peak]) peak = m;
    std::size_t nearest = 0;
    for (std::size_t m = 1; m < 128; ++m) {
        if (std::abs(fb.centreHz[m] - 1000.0) < std::abs(fb.centreHz[nearest] - 1000.0)) nearest = m;
    }
    CHECK((peak == nearest || peak + 1 == nearest || peak == nearest + 1));
    const auto [mn, mx] = std::minmax_element(mel.normalized.begin(), mel.normalized.end());
    CHECK(*mn >= 0.0f);
    CHECK(*mx == 1.0f);
}

TEST_CASE("Side/Mid descriptor on analytic signals (D-M2-14)", "[cacp_analysis][side_mid]") {
    const std::vector<float> leftOnly{1.0f, 0.0f, -1.0f, 0.0f};
    REQUIRE(sideMidDb(leftOnly).has_value());
    CHECK(std::abs(*sideMidDb(leftOnly)) < 1e-12);                        // side = mid
    const std::vector<float> lr{1.0f, 0.5f, -1.0f, -0.5f};
    CHECK(std::abs(*sideMidDb(lr) - 10.0 * std::log10(1.0 / 9.0)) < 1e-9); // side 0.25, mid 0.75
    const std::vector<float> same{0.3f, 0.3f, -0.2f, -0.2f};
    CHECK_FALSE(sideMidDb(same).has_value());                             // side = 0: -inf -> undefined
    const std::vector<float> opposite{0.3f, -0.3f};
    CHECK_FALSE(sideMidDb(opposite).has_value());                         // mid = 0: +inf -> undefined
    CHECK_FALSE(sideMidDb(std::vector<float>{}).has_value());
}

TEST_CASE("Loudness: EBU Tech 3341 minimum-requirement signals (BS.1770-5 gating)", "[cacp_analysis][loudness]") {
    auto near = [](const std::optional<double>& v, double target) { return v.has_value() && std::abs(*v - target) <= 0.1; };

    SECTION("case 1 and 2: stereo 1 kHz, -23 and -33 dBFS per channel, 20 s") {
        for (const double level : {-23.0, -33.0}) {
            std::vector<float> s;
            appendSine(s, 20.0, level);
            const auto r = measureLoudnessStereo(s, 48000);
            INFO("level " << level << " I " << r.integratedLufs.value_or(-999));
            REQUIRE(r.supported);
            CHECK(near(r.integratedLufs, level));
            CHECK(near(r.maxMomentaryLufs, level));
            CHECK(near(r.maxShortTermLufs, level));
            for (const auto& m : r.momentaryLufs) CHECK(near(m, level));
            for (const auto& st : r.shortTermLufs) CHECK(near(st, level));
        }
    }
    SECTION("cases 3 and 4: relative and absolute gates") {
        std::vector<float> c3;
        appendSine(c3, 10.0, -36.0);
        appendSine(c3, 60.0, -23.0);
        appendSine(c3, 10.0, -36.0);
        CHECK(near(measureLoudnessStereo(c3, 48000).integratedLufs, -23.0));

        std::vector<float> c4;
        appendSine(c4, 10.0, -72.0);
        appendSine(c4, 10.0, -36.0);
        appendSine(c4, 60.0, -23.0);
        appendSine(c4, 10.0, -36.0);
        appendSine(c4, 10.0, -72.0);
        CHECK(near(measureLoudnessStereo(c4, 48000).integratedLufs, -23.0));
    }
    SECTION("case 5: 20 s at -26, 20.1 s at -20, 20 s at -26") {
        std::vector<float> c5;
        appendSine(c5, 20.0, -26.0);
        appendSine(c5, 20.1, -20.0);
        appendSine(c5, 20.0, -26.0);
        CHECK(near(measureLoudnessStereo(c5, 48000).integratedLufs, -23.0));
    }
    SECTION("case 9: Short-term constant at -23 after 3 s") {
        std::vector<float> c9;
        for (int k = 0; k < 5; ++k) { appendSine(c9, 1.34, -20.0); appendSine(c9, 1.66, -30.0); }
        const auto r = measureLoudnessStereo(c9, 48000);
        REQUIRE_FALSE(r.shortTermLufs.empty());
        for (const auto& v : r.shortTermLufs) CHECK(near(v, -23.0));
    }
    SECTION("case 12: Momentary constant at -23 after 1 s") {
        std::vector<float> c12;
        for (int k = 0; k < 25; ++k) { appendSine(c12, 0.18, -20.0); appendSine(c12, 0.22, -30.0); }
        const auto r = measureLoudnessStereo(c12, 48000);
        REQUIRE_FALSE(r.momentaryLufs.empty());
        for (std::size_t i = 0; i < r.momentaryLufs.size(); ++i) {
            const double windowEndS = 0.4 + static_cast<double>(i) * r.seriesStepS;
            if (windowEndS >= 1.0) CHECK(near(r.momentaryLufs[i], -23.0));
        }
    }
    SECTION("undefined values are nullopt, never a number: silence, short signal, unsupported rate, NaN") {
        std::vector<float> silence;
        appendSilence(silence, 5.0);
        const auto rs = measureLoudnessStereo(silence, 48000);
        CHECK(rs.supported);
        CHECK_FALSE(rs.integratedLufs.has_value());
        CHECK(rs.blocksAboveAbsoluteGate == 0);
        CHECK_FALSE(rs.maxMomentaryLufs.has_value());
        for (const auto& m : rs.momentaryLufs) CHECK_FALSE(m.has_value());

        std::vector<float> shortSig;
        appendSine(shortSig, 0.3, -23.0);
        const auto rShort = measureLoudnessStereo(shortSig, 48000);
        CHECK_FALSE(rShort.integratedLufs.has_value());
        CHECK(rShort.momentaryLufs.empty());
        CHECK(rShort.blocks == 0);

        std::vector<float> s44;
        appendSine(s44, 5.0, -23.0, 1000.0, 44100);
        const auto r44 = measureLoudnessStereo(s44, 44100);
        CHECK_FALSE(r44.supported);
        CHECK_FALSE(r44.integratedLufs.has_value());

        std::vector<float> withNan;
        appendSine(withNan, 2.0, -23.0);
        withNan[1000] = std::numeric_limits<float>::quiet_NaN();
        const auto rNan = measureLoudnessStereo(withNan, 48000);
        CHECK_FALSE(rNan.integratedLufs.has_value());
    }
}

TEST_CASE("PNG writer: CRC-32, Adler-32, stored deflate and chunk structure (RFC 1950/1951, PNG)", "[cacp_analysis][png]") {
    // Check values of the reference implementations (zlib.crc32 / zlib.adler32)
    const std::string_view check = "123456789";
    CHECK(crc32(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(check.data()), check.size())) == 0xCBF43926U);
    const std::string_view wiki = "Wikipedia";
    CHECK(adler32(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(wiki.data()), wiki.size())) == 0x11E60398U);
    CHECK(adler32({}) == 1U);

    const std::vector<std::uint8_t> rgb{255, 0, 0, 0, 255, 0, 0, 0, 255, 10, 20, 30, 40, 50, 60, 70, 80, 90}; // 3 x 2
    const auto png = encodePngRgb8(3, 2, rgb);
    const auto parsed = parseStoredPng(png);
    REQUIRE(parsed.ok);
    CHECK(parsed.width == 3);
    CHECK(parsed.height == 2);
    CHECK(parsed.bitDepth == 8);
    CHECK(parsed.colourType == 2);
    CHECK(parsed.interlace == 0);
    CHECK(parsed.storedBlocks == 1);
    std::vector<std::uint8_t> expectedRaw{0};
    expectedRaw.insert(expectedRaw.end(), rgb.begin(), rgb.begin() + 9);
    expectedRaw.push_back(0);
    expectedRaw.insert(expectedRaw.end(), rgb.begin() + 9, rgb.end());
    CHECK(parsed.raw == expectedRaw);

    // More than 65,535 raw bytes: several stored blocks, last one final
    std::vector<std::uint8_t> big(static_cast<std::size_t>(200) * 200 * 3);
    for (std::size_t i = 0; i < big.size(); ++i) big[i] = static_cast<std::uint8_t>((i * 31) & 0xFF);
    const auto bigParsed = parseStoredPng(encodePngRgb8(200, 200, big));
    REQUIRE(bigParsed.ok);
    CHECK(bigParsed.storedBlocks == 2); // 200 * 601 = 120,200 bytes
    CHECK(bigParsed.raw.size() == 200U * 601U);

    // Deterministic output; invalid dimensions rejected
    CHECK(encodePngRgb8(3, 2, rgb) == png);
    CHECK(encodePngRgb8(0, 2, {}).empty());
    CHECK(encodePngRgb8(3, 2, std::span<const std::uint8_t>(rgb.data(), rgb.size() - 1)).empty());
}

TEST_CASE("SHA-256: FIPS 180-4 examples and padding boundaries (D-M2-14)", "[cacp_analysis][sha256]") {
    // NIST CSRC example values for SHA-256 (one-block "abc", two-block 448-bit message)
    CHECK(sha256Hex(std::string_view("abc")) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    CHECK(sha256Hex(std::string_view("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq")) ==
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");

    // Padding boundaries checked against an independent implementation (Python hashlib / OpenSSL)
    const std::vector<std::pair<std::size_t, std::string>> vectors{
        {0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
        {55, "9f4390f8d30c2dd92ec9f095b65e2b9ae9b0a925a5258e241c9f1e910f734318"},
        {56, "b35439a4ac6f0948b6d6f9e3c6af0f5f590ce20f1bde7090ef7970686ec6738a"},
        {63, "7d3e74a05d7db15bce4ad9ec0658ea98e3f06eeecf16b4c6fff2da457ddc2f34"},
        {64, "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb"},
        {1000000, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"}};
    for (const auto& [n, hex] : vectors) {
        const std::string msg(n, 'a');
        INFO("length " << n);
        CHECK(sha256Hex(std::string_view(msg)) == hex);

        // Streaming in uneven chunks gives the same digest
        Sha256 s;
        std::size_t pos = 0, step = 1;
        while (pos < msg.size()) {
            const std::size_t len = (std::min)(step, msg.size() - pos);
            s.update(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(msg.data() + pos), len));
            pos += len;
            step = step * 3 + 1;
        }
        const auto d = s.finish();
        CHECK(toHex(d) == hex);
    }
}
