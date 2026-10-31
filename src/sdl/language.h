#pragma once

/**
 * @file
 * The language of the text of the game: the strings it reads and the release of the cartridge the
 * menus draw.
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

/**
 * Select the language of the game.
 *
 * @param language The language, such as en-us, en-gb, or ja.
 */
void SelectLanguage(std::string_view language);

/** @return The language of SelectLanguage(), else DEFAULT_LANGUAGE. */
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
 * @param language The language, such as en-us, en-gb, or ja.
 * @return The import of the cartridge the menus of the language draw, n64/jpn for ja, n64/eur for
 * en-gb, and n64/usa for every other language, or n64/usa when the release of the language was not
 * imported.
 */
std::filesystem::path ReleaseDirectory(const std::filesystem::path &assets,
                                       std::string_view language);

} // namespace coastline
