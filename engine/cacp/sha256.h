#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace cymatica::cacp {

// SHA-256 of FIPS 180-4 (NIST, 2015, DOI 10.6028/NIST.FIPS.180-4) for the hashes of the evaluation pack files
// (ADR-0006 D-M2-14/15). Streaming interface plus one-shot helpers.
class Sha256 {
public:
    Sha256() noexcept;
    void update(std::span<const std::uint8_t> bytes) noexcept;
    [[nodiscard]] std::array<std::uint8_t, 32> finish() noexcept; // the object must not be reused afterwards

private:
    void compress(const std::uint8_t* block) noexcept;

    std::array<std::uint32_t, 8> h_{};
    std::array<std::uint8_t, 64> buffer_{};
    std::size_t buffered_{0};
    std::uint64_t totalBytes_{0};
};

[[nodiscard]] std::array<std::uint8_t, 32> sha256(std::span<const std::uint8_t> bytes) noexcept;
[[nodiscard]] std::string sha256Hex(std::span<const std::uint8_t> bytes);
[[nodiscard]] std::string sha256Hex(std::string_view text);
[[nodiscard]] std::string toHex(std::span<const std::uint8_t> bytes);
// nullopt when the file cannot be read
[[nodiscard]] std::optional<std::string> sha256FileHex(const std::string& path);

} // namespace cymatica::cacp
