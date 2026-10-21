#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace Tools::Common {

/**
 * Read a big-endian 16-bit value.
 *
 * @param bytes Buffer to read.
 * @param offset Offset of the value's first byte.
 * @return The value.
 */
inline std::uint16_t readBig16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint16_t>((bytes[offset] << 8) | bytes[offset + 1]);
}

/**
 * Read a big-endian 32-bit value.
 *
 * @param bytes Buffer to read.
 * @param offset Offset of the value's first byte.
 * @return The value.
 */
inline std::uint32_t readBig32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return (static_cast<std::uint32_t>(bytes[offset]) << 24) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 16) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 8) |
           static_cast<std::uint32_t>(bytes[offset + 3]);
}

} // namespace Tools::Common
