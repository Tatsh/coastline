#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace Tools::Common {

/**
 * Compute the SHA-256 digest of a buffer.
 *
 * @param data Bytes to digest.
 * @return The digest as 64 lower-case hexadecimal digits.
 */
std::string sha256Hex(std::span<const std::uint8_t> data);

} // namespace Tools::Common
