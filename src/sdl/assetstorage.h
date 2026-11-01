#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <string>
#include <vector>

namespace coastline {

/** The alias-table filename for logical paths and their physical backing files. */
inline constexpr const char *ASSET_ALIASES_FILE = "asset-aliases.txt";

/** A logical file backed by a physical file in the same pack or the default pack. */
struct AssetFileAlias {
    std::filesystem::path path;   /*!< The logical path relative to its pack. */
    std::filesystem::path target; /*!< The physical path relative to its backing pack. */
    bool fromDefault = false;     /*!< Whether the backing file is in the default pack. */
};

/** The number and total byte size of files removed from freshly converted generated packs. */
struct AssetStorageSavings {
    std::uintmax_t files = 0; /*!< The number of redundant physical files removed. */
    std::uintmax_t bytes = 0; /*!< The sum of the removed file sizes, in bytes. */
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
 * @return Aliases, an empty vector without a table, or an error for unreadable or invalid metadata.
 */
std::expected<std::vector<AssetFileAlias>, std::string>
ReadAssetFileAliases(const std::filesystem::path &pack, const std::filesystem::path &defaults);

} // namespace coastline
