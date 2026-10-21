#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

#include "error.h"

namespace Tools::Common {

/**
 * Read a whole file.
 *
 * @param path File to read.
 * @return The file contents, or an `ErrorCode::Io` error.
 */
std::expected<std::vector<std::uint8_t>, Error> readFile(const std::filesystem::path &path);

/**
 * Replace a file with the given bytes.
 *
 * @param path File to write.
 * @param data New contents.
 * @return Nothing, or an `ErrorCode::Io` error.
 */
std::expected<void, Error> writeFile(const std::filesystem::path &path,
                                     std::span<const std::uint8_t> data);

/**
 * Replace a file with the given text, written as is.
 *
 * @param path File to write.
 * @param text New contents.
 * @return Nothing, or an `ErrorCode::Io` error.
 */
std::expected<void, Error> writeFile(const std::filesystem::path &path, std::string_view text);

} // namespace Tools::Common
