#include "png_writer.h"

#include <algorithm>
#include <array>
#include <fstream>

namespace cymatica::cacp {

namespace {

constexpr std::array<std::uint32_t, 256> makeCrcTable() noexcept {
    std::array<std::uint32_t, 256> t{};
    for (std::uint32_t n = 0; n < 256; ++n) {
        std::uint32_t c = n;
        for (int k = 0; k < 8; ++k) c = (c & 1U) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
        t[n] = c;
    }
    return t;
}

constexpr auto kCrcTable = makeCrcTable();

void putU32Be(std::vector<std::uint8_t>& out, std::uint32_t v) {
    out.push_back(static_cast<std::uint8_t>(v >> 24));
    out.push_back(static_cast<std::uint8_t>(v >> 16));
    out.push_back(static_cast<std::uint8_t>(v >> 8));
    out.push_back(static_cast<std::uint8_t>(v));
}

void putChunk(std::vector<std::uint8_t>& out, const char (&type)[5], std::span<const std::uint8_t> data) {
    putU32Be(out, static_cast<std::uint32_t>(data.size()));
    const std::size_t typeAt = out.size();
    out.insert(out.end(), type, type + 4);
    out.insert(out.end(), data.begin(), data.end());
    // CRC over chunk type and data (not the length)
    putU32Be(out, crc32(std::span<const std::uint8_t>(out.data() + typeAt, 4 + data.size())));
}

} // namespace

std::uint32_t crc32(std::span<const std::uint8_t> bytes, std::uint32_t previous) noexcept {
    std::uint32_t c = previous ^ 0xFFFFFFFFU;
    for (const auto b : bytes) c = kCrcTable[(c ^ b) & 0xFFU] ^ (c >> 8);
    return c ^ 0xFFFFFFFFU;
}

std::uint32_t adler32(std::span<const std::uint8_t> bytes) noexcept {
    constexpr std::uint32_t kMod = 65521U;
    std::uint32_t s1 = 1, s2 = 0;
    for (const auto b : bytes) {
        s1 = (s1 + b) % kMod;
        s2 = (s2 + s1) % kMod;
    }
    return (s2 << 16) | s1;
}

std::vector<std::uint8_t> encodePngRgb8(std::uint32_t width, std::uint32_t height, std::span<const std::uint8_t> rgb) {
    std::vector<std::uint8_t> out;
    const std::size_t rowBytes = static_cast<std::size_t>(width) * 3;
    if (width == 0 || height == 0 || width > 0x7FFFFFFFU || height > 0x7FFFFFFFU ||
        rgb.size() != rowBytes * height) {
        return out;
    }

    // Raw scanlines with filter type 0
    std::vector<std::uint8_t> raw;
    raw.reserve((rowBytes + 1) * height);
    for (std::uint32_t y = 0; y < height; ++y) {
        raw.push_back(0);
        raw.insert(raw.end(), rgb.begin() + static_cast<std::ptrdiff_t>(y * rowBytes),
                   rgb.begin() + static_cast<std::ptrdiff_t>((y + 1) * rowBytes));
    }

    // zlib: CMF 0x78 (deflate, 32 KiB window), FLG 0x01 (FCHECK: 0x7801 % 31 == 0, no dictionary, level 0)
    std::vector<std::uint8_t> z{0x78, 0x01};
    std::size_t pos = 0;
    do {
        const std::size_t len = (std::min)(raw.size() - pos, std::size_t{65535});
        const bool final = (pos + len == raw.size());
        z.push_back(final ? 0x01 : 0x00); // BFINAL, BTYPE = 00 (stored), byte aligned
        const auto len16 = static_cast<std::uint16_t>(len);
        const auto nlen16 = static_cast<std::uint16_t>(~len16);
        z.push_back(static_cast<std::uint8_t>(len16 & 0xFF));
        z.push_back(static_cast<std::uint8_t>(len16 >> 8));
        z.push_back(static_cast<std::uint8_t>(nlen16 & 0xFF));
        z.push_back(static_cast<std::uint8_t>(nlen16 >> 8));
        z.insert(z.end(), raw.begin() + static_cast<std::ptrdiff_t>(pos), raw.begin() + static_cast<std::ptrdiff_t>(pos + len));
        pos += len;
    } while (pos < raw.size());
    putU32Be(z, adler32(raw)); // big-endian (network order), RFC 1950

    static constexpr std::array<std::uint8_t, 8> kSignature{0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    out.insert(out.end(), kSignature.begin(), kSignature.end());

    std::vector<std::uint8_t> ihdr;
    putU32Be(ihdr, width);
    putU32Be(ihdr, height);
    ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0}); // bit depth 8, colour type 2 (RGB), deflate, filter 0, no interlace
    putChunk(out, "IHDR", ihdr);
    putChunk(out, "IDAT", z);
    putChunk(out, "IEND", {});
    return out;
}

bool writePngRgb8(const std::string& path, std::uint32_t width, std::uint32_t height, std::span<const std::uint8_t> rgb) {
    const std::vector<std::uint8_t> png = encodePngRgb8(width, height, rgb);
    if (png.empty()) return false;
    std::ofstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    f.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    return f.good();
}

} // namespace cymatica::cacp
