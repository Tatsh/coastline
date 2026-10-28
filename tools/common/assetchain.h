#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <vector>

#include "error.h"

namespace Tools::Common {

/** The bytes of an asset header: the total length and the decompressed length, big-endian. */
inline constexpr std::uint32_t kAssetHeaderSize = 8;

/** The bytes of the big-endian length before each zlib chunk of an asset. */
inline constexpr std::uint32_t kAssetChunkHeaderSize = 4;

/**
 * Round a chunk length up to the even length the cartridge stores.
 *
 * @param length Chunk length.
 * @return The length rounded up to a multiple of two.
 */
constexpr std::uint32_t padToEven(std::uint32_t length) {
    return (length + 1) & ~std::uint32_t{1};
}

/**
 * Check for a zlib header: a deflate method byte and a flags byte whose 16-bit value is a multiple
 * of 31.
 *
 * @param bytes Buffer to check.
 * @param offset Offset of the header.
 * @return Whether a zlib header starts at @p offset.
 */
bool hasZlibHeader(std::span<const std::uint8_t> bytes, std::size_t offset);

/**
 * Decompress an asset of the cartridge: its header, then a chain of zlib chunks, each after its
 * length.
 *
 * @param blob The asset, from its header.
 * @return The decompressed data, or an error when the chain is not well formed.
 */
std::expected<std::vector<std::uint8_t>, Error> decompress(std::span<const std::uint8_t> blob);

} // namespace Tools::Common
