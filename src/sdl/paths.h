#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace coastline {

/** The directories the game reads and writes, each ending in the application directory. */
struct GamePaths {
    std::filesystem::path config; /*!< BossRally.ini and the bindings of the controls. */
    std::filesystem::path data;   /*!< The game data copied from the disc and the cartridges. */
    std::filesystem::path state;  /*!< game.log and the saved games. */
    std::filesystem::path cache;  /*!< Data derived from the game data, safe to delete. */
};

/** Reads an environment variable, or nothing when it is unset or empty. */
using EnvironmentReader = std::function<std::optional<std::string>(const char *name)>;

/**
 * Resolve the directories with the XDG Base Directory variables and their defaults under the home
 * directory. A relative XDG value is ignored, as the specification requires.
 *
 * @param environment The source of the environment variables.
 * @param application The name of the directory created under each base directory.
 * @return The four directories, or nothing when neither HOME nor an XDG variable gives a base.
 */
std::optional<GamePaths> ResolveXdgPaths(const EnvironmentReader &environment,
                                         const std::string &application);

/**
 * Resolve the directories with the conventions of the running operating system: the XDG Base
 * Directory variables on Linux and the BSDs, the folders of the user's library on macOS, and the
 * roaming and local application data folders on Windows.
 *
 * @param application The name of the directory created under each base directory.
 * @return The four directories, or nothing when the operating system does not report a base.
 */
std::optional<GamePaths> ResolveGamePaths(const std::string &application);

/**
 * List system data roots, including the relocated installation and platform defaults.
 *
 * @param executable The directory of the executable, or empty to omit relative roots.
 * @return Data roots in lookup order.
 */
std::vector<std::filesystem::path> SystemDataDirectories(const std::filesystem::path &executable);

/**
 * Find usable assets in system roots first, then in the writable user data root.
 *
 * @param user The writable user data root.
 * @param system System data roots in lookup order.
 * @return The first root with usable default assets, or nothing.
 */
std::optional<std::filesystem::path> FindAssetData(const std::filesystem::path &user,
                                                   std::span<const std::filesystem::path> system);

/**
 * Create every directory of @p paths that does not exist.
 *
 * @param paths The directories to create.
 * @return Whether every directory exists afterwards.
 */
bool CreateGamePaths(const GamePaths &paths);

/** Delete all four application directories, or return an error message on failure. */
std::optional<std::string> ResetGamePaths(const GamePaths &paths);

/**
 * The directory the import writes the game data to, the default pack. A mod pack is a directory
 * beside it under assets.
 *
 * @param data The data directory of the game.
 * @return The directory of the default pack.
 */
inline std::filesystem::path DefaultAssets(const std::filesystem::path &data) {
    return data / "assets" / "default";
}

/** The name of the asset pack of the import, the directory assets/default. */
inline constexpr std::string_view DEFAULT_PACK = "default";

/**
 * The directory of a pack under assets of the data directory, named after the pack.
 *
 * @param data The data directory of the game.
 * @param pack The name of the pack.
 * @param fallback Another data root used when @p data lacks the pack directory, or empty.
 * @return The directory of the pack.
 */
std::filesystem::path PackDirectory(const std::filesystem::path &data,
                                    std::string_view pack,
                                    const std::filesystem::path &fallback = {});

/**
 * The directory the game reads its assets from with a pack selected. Duplicate-file aliases are
 * expanded under @p cache before layering. Without aliases, the default pack is read in place.
 * Another pack is layered over the default pack in a directory under @p cache. An entry of the
 * pack replaces the entry of the same name in the default pack, and a directory in both is
 * layered in turn. Each entry is a link to the file or the directory it comes from, or a copy where
 * the file system has no links. The directory is made again only when the path or the write time
 * of an entry of either pack differs from the stamp recorded in it, or the stamp is missing.
 *
 * @param data The data directory of the game.
 * @param cache The cache directory of the game.
 * @param pack The name of the pack.
 * @param fallback Another data root searched for the selected pack, or empty. Default assets
 * still come from @p data.
 * @param region A regional difference pack applied before @p pack, or empty for the base region.
 * @return The directory, or nothing when the pack does not exist or the directory cannot be made.
 */
std::optional<std::filesystem::path> PackAssets(const std::filesystem::path &data,
                                                const std::filesystem::path &cache,
                                                std::string_view pack,
                                                const std::filesystem::path &fallback = {},
                                                std::string_view region = {});

} // namespace coastline
