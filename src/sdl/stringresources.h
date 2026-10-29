#pragma once

#include <cstdint>
#include <map>
#include <span>
#include <string>
#include <string_view>

namespace coastline {

/**
 * Read the string resources of a Windows library.
 *
 * @param image The bytes of the library file.
 * @return The strings of the RT_STRING resources as UTF-8, by identifier. A library that is not a
 * PE image or has no string resources gives none.
 */
std::map<unsigned, std::string> ReadLibraryStrings(std::span<const std::uint8_t> image);

/**
 * Convert UTF-8 text to Windows-1252, as WideCharToMultiByte() converts the strings LoadStringA()
 * returns. A character Windows-1252 does not have becomes '?'.
 *
 * @param text The UTF-8 text.
 * @return The Windows-1252 text.
 */
std::string ToWindows1252(std::string_view text);

} // namespace coastline
