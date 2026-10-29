#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

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
    return data / "assets" / "defaults";
}

} // namespace coastline
