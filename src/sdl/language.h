#pragma once

/**
 * @file
 * UTF-8 strings, language-specific fonts, and the selected cartridge assets.
 */

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

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

/**
 * @param release The converted cartridge directory with cartridge.ini.
 * @return usa, eur, or jpn from recorded metadata, or empty for absent or invalid metadata.
 */
std::string CartridgeRegion(const std::filesystem::path &release);

/**
 * @param dataRoots The data directories searched for default and regional packs.
 * @return Available regions without duplicates, sorted lexicographically.
 */
std::vector<std::string>
AvailableCartridgeRegions(std::span<const std::filesystem::path> dataRoots);

/**
 * @param choice regional, japanese, or international; another value is unavailable.
 * @param currentRegion The active cartridge region, assumed available.
 * @param available Other available cartridge regions.
 * @return The requested region, independent of language, or std::nullopt when unavailable.
 * International prefers the active USA or European region, then USA, then Europe.
 */
std::optional<std::string> TitleScreenRegion(std::string_view choice,
                                             std::string_view currentRegion,
                                             std::span<const std::string> available);

/**
 * @param assets The prepared asset directory.
 * @param language ja for Japanese fonts, en-gb for European fonts, or another value for USA fonts.
 * @return The selected fonts directory when fonts.json exists, or the imported base directory.
 */
std::filesystem::path FontDirectory(const std::filesystem::path &assets, std::string_view language);

} // namespace coastline
