#pragma once

#include <cstdint>
#include <filesystem>
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
 * Append a code point to UTF-8 text.
 *
 * @param text The UTF-8 text.
 * @param c The code point.
 */
void AppendUtf8(std::string &text, char32_t c);

/**
 * Decode the code point of UTF-8 text at a position, and step past it. A sequence cut short by the
 * end of the text decodes from the bytes it has.
 *
 * @param text The UTF-8 text.
 * @param position The byte of the code point, moved to the byte after it.
 * @return The code point.
 */
char32_t NextCodePoint(std::string_view text, std::size_t &position);

/**
 * Read the strings of a language and of en-us, each a JSON object of strings by key, from
 * strings/LANGUAGE.json and strings/en-us.json under @p assets, for the race and the menus. A key
 * the language lacks takes its en-us string.
 *
 * @param assets The assets of the pack.
 * @param language The language, such as en-us, en-gb, or ja.
 */
void LoadStrings(const std::filesystem::path &assets, std::string_view language);

/**
 * @param key The key of a string, such as 1 or n64.carSelect.rumblePakNote.
 * @return The string of LoadStrings() in UTF-8, or nothing.
 */
const std::string *FindString(std::string_view key);

/**
 * @param key The key of a string, such as n64.carSelect.rumblePakNote.
 * @return The string of LoadStrings() in UTF-8, the string of the selected language or else of
 * en-us, or @p key when neither file has it, logged the first time.
 */
std::string_view GameString(std::string_view key);

} // namespace coastline
