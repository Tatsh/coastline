#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace coastline {

/** The name of the pack of the PC images the default pack replaces with branded images. */
inline constexpr std::string_view BOSS_PACK = "boss";

/** The version of pack.json the import writes. */
inline constexpr int PACK_SETTINGS_VERSION = 1;

/** The gain of the music modules of the default pack, as they play quieter than the CD audio. */
inline constexpr float DEFAULT_XM_MUSIC_GAIN = 1.5f;

/**
 * Put the images of the cartridges that show the branding the PC release removed (Nintendo, Top
 * Gear, and Kemco) in place of their PC images in the default pack, and move the PC images to the
 * pack boss beside it. The images are listed in data/branded-textures.json. An image whose model
 * has another image of the PC release in the same file takes a file of its own, and the primitives
 * of the listed meshes sample it; the original model moves to boss. An image whose cartridge or
 * disc is not yet imported is skipped, and an image already in place is not written again. Each
 * file written is read back and compared. The default pack and boss also take their pack.json.
 *
 * @param defaults The directory of the default pack, assets/default of the data directory.
 * @return A message, or nothing.
 */
std::optional<std::string> CurateAssetPacks(const std::filesystem::path &defaults);

/**
 * The settings of a pack: the keys of pack.json of the default pack, with the keys of pack.json of
 * the pack in their place. A missing or malformed file gives no keys.
 *
 * @param data The data directory of the game.
 * @param pack The name of the pack.
 * @return The settings, an object.
 */
nlohmann::json PackSettings(const std::filesystem::path &data, std::string_view pack);

} // namespace coastline
