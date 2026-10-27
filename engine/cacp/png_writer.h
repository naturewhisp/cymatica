#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cymatica::cacp {

// Minimal PNG encoder of descriptors_v1 (ADR-0006 D-M2-14, R5 option T1): 8-bit RGB, no interlace, filter type 0
// on every scanline, zlib stream (RFC 1950) made of deflate stored blocks (RFC 1951 BTYPE = 00, at most 65,535
// bytes each), Adler-32 trailer, CRC-32 per chunk (PNG, W3C Third Edition). No compression, no ancillary or text
// chunks: the output depends only on the pixels.

// CRC-32 of PNG / zlib (reflected polynomial 0xEDB88320, initial and final XOR 0xFFFFFFFF)
[[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> bytes, std::uint32_t previous = 0) noexcept;
// Adler-32 of RFC 1950 (modulo 65521, s1 = 1 and s2 = 0 initially)
[[nodiscard]] std::uint32_t adler32(std::span<const std::uint8_t> bytes) noexcept;

// rgb holds width * height * 3 bytes, rows top to bottom. Returns an empty vector on invalid dimensions.
[[nodiscard]] std::vector<std::uint8_t> encodePngRgb8(std::uint32_t width, std::uint32_t height,
                                                      std::span<const std::uint8_t> rgb);

bool writePngRgb8(const std::string& path, std::uint32_t width, std::uint32_t height, std::span<const std::uint8_t> rgb);

} // namespace cymatica::cacp
