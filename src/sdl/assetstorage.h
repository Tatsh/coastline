#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace coastline {

inline constexpr const char *ASSET_ALIASES_FILE = "asset-aliases.txt";

/** A logical file backed by a physical file in the same pack or the default pack. */
struct AssetFileAlias {
    std::filesystem::path path;
    std::filesystem::path target;
    bool fromDefault = false;
};

/** The number and total byte size of files removed from freshly converted generated packs. */
struct AssetStorageSavings {
    std::uintmax_t files = 0;
    std::uintmax_t bytes = 0;
};

/**
 * Replace byte-identical generated files with aliases. Files shared by multiple non-default
 * packs move into default/shared. Mod packs are excluded. Call before recording the inventory.
 *
 * @param defaults The freshly converted default pack, without an alias table.
 * @return Storage savings, or an error. Discard staged packs after a failure.
 */
std::expected<AssetStorageSavings, std::string>
DeduplicateAssetFiles(const std::filesystem::path &defaults);

/**
 * Read and validate an alias table, including its physical targets. Targets never refer to
 * aliases or to another optional pack.
 *
 * @param pack The physical pack directory.
 * @param defaults The physical default pack directory.
 * @return Aliases, an empty vector without a table, or an error for invalid metadata.
 */
std::expected<std::vector<AssetFileAlias>, std::string>
ReadAssetFileAliases(const std::filesystem::path &pack, const std::filesystem::path &defaults);

} // namespace coastline
