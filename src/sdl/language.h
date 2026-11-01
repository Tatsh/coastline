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

/** @return The recorded region of a converted cartridge, or empty for invalid metadata. */
std::string CartridgeRegion(const std::filesystem::path &release);

/** @return Regions available from imported default and regional packs across the data roots. */
std::vector<std::string>
AvailableCartridgeRegions(std::span<const std::filesystem::path> dataRoots);

/** @return The requested title region, independent of language, or empty when unavailable. */
std::optional<std::string> TitleScreenRegion(std::string_view choice,
                                             std::string_view currentRegion,
                                             std::span<const std::string> available);

/** @return Fonts for @p language, or the imported base fonts when unavailable. */
std::filesystem::path FontDirectory(const std::filesystem::path &assets, std::string_view language);

} // namespace coastline
