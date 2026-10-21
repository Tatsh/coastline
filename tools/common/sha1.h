#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace Tools::Common {

/**
 * Compute the SHA-1 digest of a buffer.
 *
 * @param data Bytes to digest.
 * @return The digest as 40 lower-case hexadecimal digits.
 */
std::string sha1Hex(std::span<const std::uint8_t> data);

} // namespace Tools::Common
