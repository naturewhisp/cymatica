#include "sha256.h"

#include <bit>
#include <fstream>
#include <vector>

namespace cymatica::cacp {

namespace {

// FIPS 180-4 §4.2.2: first 32 bits of the fractional parts of the cube roots of the first 64 primes
constexpr std::array<std::uint32_t, 64> kK{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

constexpr std::uint32_t rotr(std::uint32_t x, int n) noexcept { return std::rotr(x, n); }

} // namespace

Sha256::Sha256() noexcept
    // FIPS 180-4 §5.3.3: initial hash value H(0)
    : h_{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19} {}

void Sha256::compress(const std::uint8_t* block) noexcept {
    std::array<std::uint32_t, 64> w{};
    for (int t = 0; t < 16; ++t) {
        w[t] = (static_cast<std::uint32_t>(block[4 * t]) << 24) | (static_cast<std::uint32_t>(block[4 * t + 1]) << 16) |
               (static_cast<std::uint32_t>(block[4 * t + 2]) << 8) | static_cast<std::uint32_t>(block[4 * t + 3]);
    }
    for (int t = 16; t < 64; ++t) {
        const std::uint32_t s0 = rotr(w[t - 15], 7) ^ rotr(w[t - 15], 18) ^ (w[t - 15] >> 3);
        const std::uint32_t s1 = rotr(w[t - 2], 17) ^ rotr(w[t - 2], 19) ^ (w[t - 2] >> 10);
        w[t] = w[t - 16] + s0 + w[t - 7] + s1;
    }
    std::uint32_t a = h_[0], b = h_[1], c = h_[2], d = h_[3], e = h_[4], f = h_[5], g = h_[6], h = h_[7];
    for (int t = 0; t < 64; ++t) {
        const std::uint32_t bigS1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        const std::uint32_t ch = (e & f) ^ (~e & g);
        const std::uint32_t t1 = h + bigS1 + ch + kK[static_cast<std::size_t>(t)] + w[static_cast<std::size_t>(t)];
        const std::uint32_t bigS0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        const std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const std::uint32_t t2 = bigS0 + maj;
        h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
    }
    h_[0] += a; h_[1] += b; h_[2] += c; h_[3] += d; h_[4] += e; h_[5] += f; h_[6] += g; h_[7] += h;
}

void Sha256::update(std::span<const std::uint8_t> bytes) noexcept {
    totalBytes_ += bytes.size();
    std::size_t i = 0;
    if (buffered_ > 0) {
        while (buffered_ < 64 && i < bytes.size()) buffer_[buffered_++] = bytes[i++];
        if (buffered_ < 64) return;
        compress(buffer_.data());
        buffered_ = 0;
    }
    for (; i + 64 <= bytes.size(); i += 64) compress(bytes.data() + i);
    while (i < bytes.size()) buffer_[buffered_++] = bytes[i++];
}

std::array<std::uint8_t, 32> Sha256::finish() noexcept {
    // FIPS 180-4 §5.1.1: append 1 bit, zeros, then the 64-bit big-endian message length in bits
    const std::uint64_t bitLength = totalBytes_ * 8;
    buffer_[buffered_++] = 0x80;
    if (buffered_ > 56) {
        while (buffered_ < 64) buffer_[buffered_++] = 0;
        compress(buffer_.data());
        buffered_ = 0;
    }
    while (buffered_ < 56) buffer_[buffered_++] = 0;
    for (int k = 7; k >= 0; --k) buffer_[buffered_++] = static_cast<std::uint8_t>(bitLength >> (8 * k));
    compress(buffer_.data());

    std::array<std::uint8_t, 32> out{};
    for (std::size_t i = 0; i < 8; ++i) {
        out[4 * i] = static_cast<std::uint8_t>(h_[i] >> 24);
        out[4 * i + 1] = static_cast<std::uint8_t>(h_[i] >> 16);
        out[4 * i + 2] = static_cast<std::uint8_t>(h_[i] >> 8);
        out[4 * i + 3] = static_cast<std::uint8_t>(h_[i]);
    }
    return out;
}

std::array<std::uint8_t, 32> sha256(std::span<const std::uint8_t> bytes) noexcept {
    Sha256 s;
    s.update(bytes);
    return s.finish();
}

std::string toHex(std::span<const std::uint8_t> bytes) {
    static constexpr char kDigits[] = "0123456789abcdef";
    std::string out;
    out.reserve(bytes.size() * 2);
    for (const auto b : bytes) {
        out.push_back(kDigits[b >> 4]);
        out.push_back(kDigits[b & 0x0F]);
    }
    return out;
}

std::string sha256Hex(std::span<const std::uint8_t> bytes) {
    const auto d = sha256(bytes);
    return toHex(d);
}

std::string sha256Hex(std::string_view text) {
    return sha256Hex(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(text.data()), text.size()));
}

std::optional<std::string> sha256FileHex(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return std::nullopt;
    Sha256 s;
    std::vector<char> buf(1 << 16);
    while (f) {
        f.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        const auto got = static_cast<std::size_t>(f.gcount());
        if (got > 0) s.update(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(buf.data()), got));
    }
    if (f.bad()) return std::nullopt;
    const auto d = s.finish();
    return toHex(d);
}

} // namespace cymatica::cacp
