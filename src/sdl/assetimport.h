#pragma once

#include <filesystem>
#include <optional>
#include <span>
#include <string>

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
 * @param disc A Boss Rally disc image.
 * @param roms Top Gear Rally ROM images, with at most one image per region.
 * @param progress Report batch progress, or empty. Return false to cancel before installation.
 * @return An error, or nothing after installation and inventory validation.
 */
std::optional<std::string> ImportAssetImages(const std::filesystem::path &data,
                                             const std::filesystem::path &disc,
                                             std::span<const std::filesystem::path> roms,
                                             const ImportProgress &progress = {});

} // namespace coastline
