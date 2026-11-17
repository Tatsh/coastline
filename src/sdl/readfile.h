#pragma once

/**
 * @file
 * Reading a whole file into memory.
 */

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace coastline {

/**
 * Reads a whole file with one read of its size.
 *
 * @param path The file.
 * @return The bytes of the file, or no value when it cannot be opened or read.
 */
std::optional<std::vector<std::uint8_t>> ReadWholeFile(const std::filesystem::path &path);

} // namespace coastline
