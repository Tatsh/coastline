#include "sha1.h"

#include <array>
#include <bit>
#include <format>

namespace Tools::Common {

namespace {

constexpr std::size_t kBlockSize = 64;

void processBlock(std::array<std::uint32_t, 5> &state, std::span<const std::uint8_t> block) {
    std::array<std::uint32_t, 80> words{};
    for (std::size_t i = 0; i < 16; ++i) {
        words[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24) |
                   (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) |
                   static_cast<std::uint32_t>(block[i * 4 + 3]);
    }
    for (std::size_t i = 16; i < 80; ++i) {
        words[i] = std::rotl(words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1);
    }
    auto [a, b, c, d, e] = state;
    for (std::size_t i = 0; i < 80; ++i) {
        std::uint32_t f = 0;
        std::uint32_t k = 0;
        if (i < 20) {
            f = (b & c) | (~b & d);
            k = 0x5a827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ed9eba1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8f1bbcdc;
        } else {
            f = b ^ c ^ d;
            k = 0xca62c1d6;
        }
        const std::uint32_t t = std::rotl(a, 5) + f + e + k + words[i];
        e = d;
        d = c;
        c = std::rotl(b, 30);
        b = a;
        a = t;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

} // namespace

std::string sha1Hex(std::span<const std::uint8_t> data) {
    std::array<std::uint32_t, 5> state{0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
    std::size_t offset = 0;
    for (; offset + kBlockSize <= data.size(); offset += kBlockSize) {
        processBlock(state, data.subspan(offset, kBlockSize));
    }
    // The tail is padded with a 1 bit, zeros, and the big-endian bit length, in one or two blocks.
    std::array<std::uint8_t, kBlockSize * 2> tail{};
    const std::size_t rest = data.size() - offset;
    for (std::size_t i = 0; i < rest; ++i) {
        tail[i] = data[offset + i];
    }
    tail[rest] = 0x80;
    const std::size_t blocks = rest + 1 + 8 <= kBlockSize ? 1 : 2;
    const std::uint64_t bits = static_cast<std::uint64_t>(data.size()) * 8;
    for (std::size_t i = 0; i < 8; ++i) {
        tail[blocks * kBlockSize - 1 - i] = static_cast<std::uint8_t>(bits >> (8 * i));
    }
    for (std::size_t block = 0; block < blocks; ++block) {
        processBlock(state, std::span(tail).subspan(block * kBlockSize, kBlockSize));
    }
    std::string hex;
    for (const auto word : state) {
        hex += std::format("{:08x}", word);
    }
    return hex;
}

} // namespace Tools::Common
