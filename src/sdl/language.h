#pragma once

/**
 * @file
 * UTF-8 strings, language-specific fonts, and the selected cartridge assets.
 */

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace coastline {

/** The language unless the configuration or the command line selects another. */
inline constexpr std::string_view DEFAULT_LANGUAGE = "en-us";

/** Strings by key, in UTF-8. */
using StringTable = std::map<std::string, std::string, std::less<>>;

/** @return The active text catalogue's language, or DEFAULT_LANGUAGE without a catalogue. */
const std::string &SelectedLanguage();

/**
 * @param fallback The strings of DEFAULT_LANGUAGE.
 * @param language The strings of the selected language.
 * @return The strings of the language, with the string of @p fallback for each key the language
 * lacks.
 */
StringTable MergeStrings(StringTable fallback, const StringTable &language);

/**
 * @param assets The assets of the pack.
 * @return The selected cartridge directory, independent of language.
 */
std::filesystem::path ReleaseDirectory(const std::filesystem::path &assets);

/** @return The recorded region of a converted cartridge, or empty for invalid metadata. */
std::string CartridgeRegion(const std::filesystem::path &release);

/** @return Fonts for @p language, or the imported base fonts when unavailable. */
std::filesystem::path FontDirectory(const std::filesystem::path &assets, std::string_view language);

} // namespace coastline
