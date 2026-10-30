#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

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
 * Create every directory of @p paths that does not exist.
 *
 * @param paths The directories to create.
 * @return Whether every directory exists afterwards.
 */
bool CreateGamePaths(const GamePaths &paths);

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
 * @return The directory of the pack.
 */
std::filesystem::path PackDirectory(const std::filesystem::path &data, std::string_view pack);

/**
 * The directory the game reads its assets from with a pack selected. The default pack is read in
 * place. Another pack is layered over the default pack in a directory under @p cache, made again
 * on each call: an entry of the pack replaces the entry of the same name in the default pack, and
 * a directory in both is layered in turn. Each entry is a link to the file or the directory it
 * comes from, or a copy where the file system has no links.
 *
 * @param data The data directory of the game.
 * @param cache The cache directory of the game.
 * @param pack The name of the pack.
 * @return The directory, or nothing when the pack does not exist or the directory cannot be made.
 */
std::optional<std::filesystem::path> PackAssets(const std::filesystem::path &data,
                                                const std::filesystem::path &cache,
                                                std::string_view pack);

} // namespace coastline
