#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "importer.h"

namespace coastline {

/**
 * Import a disc and every supplied ROM as one batch. USA is the base when supplied; otherwise
 * the first ROM is the base. Failed or cancelled conversions preserve installed packs. A
 * successful batch replaces generated packs and preserves mod packs.
 * Each additional cartridge produces a regional difference pack over the base. Language fonts
 * and strings remain available independently of the selected region.
 *
 * @param data The data root where generated packs are installed under assets.
 * @param disc A Boss Rally disc image or mounted disc directory.
 * @param roms Top Gear Rally ROM images, with at most one image per region.
 * @param progress Report batch progress, or empty. Return false to cancel before installation.
 * @param sources Receives original input files and searched directories on success, or null.
 * @return An error, or nothing after installation and inventory validation.
 */
std::optional<std::string> ImportAssetImages(const std::filesystem::path &data,
                                             const std::filesystem::path &disc,
                                             std::span<const std::filesystem::path> roms,
                                             const ImportProgress &progress = {},
                                             std::vector<std::filesystem::path> *sources = nullptr);

/**
 * Write a GCC-style dependency file atomically for a completed asset import.
 * Paths become absolute UTF-8 paths with sorted, unique dependencies. Line breaks and tabs
 * cannot be represented; literal backslashes are also rejected on POSIX.
 *
 * @param file The dependency output file.
 * @param target The build output depending on imported sources.
 * @param sources Original input files and searched directories.
 * @return An error, or an empty optional after writing the dependency file.
 */
std::optional<std::string> WriteImportDependencies(const std::filesystem::path &file,
                                                   const std::filesystem::path &target,
                                                   std::span<const std::filesystem::path> sources);

} // namespace coastline
