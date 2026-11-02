#pragma once

#include <cstdint>
#include <istream>
#include <optional>
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

/**
 * Compute SHA-1 from the current stream position through EOF with bounded memory.
 *
 * @param data Binary input stream.
 * @return Lowercase digest, or an empty optional when reading fails before EOF.
 */
std::optional<std::string> sha1Hex(std::istream &data);

} // namespace Tools::Common
