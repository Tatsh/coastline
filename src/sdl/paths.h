#pragma once

#include <expected>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace coastline {

/**
 * @param path A native filesystem path.
 * @return The path in UTF-8, with native directory separators.
 */
inline std::string Utf8FromPath(const std::filesystem::path &path) {
    auto native = path;
    const auto text = native.make_preferred().u8string();
    return {text.begin(), text.end()};
}

/** The directories the game reads and writes, each ending in the application directory. */
struct GamePaths {
    std::filesystem::path config; /*!< BossRally.ini and the bindings of the controls. */
    std::filesystem::path data;   /*!< The game data copied from the disc and the cartridges. */
    std::filesystem::path state;  /*!< game.log and the saved games. */
    std::filesystem::path cache;  /*!< Data derived from the game data, safe to delete. */
    bool portableConfig = false;  /*!< Whether configuration uses a local portable directory. */
    bool portableData = false;    /*!< Whether data uses a local portable directory. */
};

/** @return Whether this build supports adjacent portable configuration and assets. */
bool PortablePathsEnabled();

/**
 * Locate portable files beside an application bundle, AppImage, or ordinary executable.
 * @param executable Absolute base directory reported by SDL.
 * @param appImage Absolute AppImage filename, or empty for other distributions.
 * @return CoastlineData beside a bundle or AppImage, the distribution root above bin,
 * or @p executable for other layouts.
 */
std::filesystem::path PortableRoot(const std::filesystem::path &executable,
                                   const std::filesystem::path &appImage = {});

/**
 * Prefer adjacent configuration and assets independently when portable lookup is enabled.
 *
 * @param fallback Platform directories used for absent adjacent files.
 * @param executable Absolute portable directory of the application distribution.
 * @param workingDirectory Launch directory checked after the distribution, including its
 * CoastlineData child, or empty to omit it.
 * @return Selected directories, or an error when an adjacent location cannot be written.
 */
std::expected<GamePaths, std::string>
ResolvePortablePaths(const GamePaths &fallback,
                     const std::filesystem::path &executable,
                     const std::filesystem::path &workingDirectory = {});

/**
 * Store an input path relative to the portable directory, using '~' for external home paths.
 *
 * @param path Input path; empty and relative paths remain unchanged.
 * @param root Portable directory rebuilt from the executable location on startup.
 * @param home Home directory, or empty to disable home shorthand.
 * @return Relative path, home shorthand, or the original path when no relative path exists.
 */
std::filesystem::path StorePortablePath(const std::filesystem::path &path,
                                        const std::filesystem::path &root,
                                        const std::filesystem::path &home);

/**
 * Rebuild a stored portable input path at its current executable or home location.
 *
 * @param path Stored path, possibly relative or starting with a '~' component.
 * @param root Current portable directory.
 * @param home Current home directory, or empty to preserve home shorthand.
 * @return Rebuilt path, with absolute input paths preserved.
 */
std::filesystem::path RestorePortablePath(const std::filesystem::path &path,
                                          const std::filesystem::path &root,
                                          const std::filesystem::path &home);

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

/**
 * Move portable assets, settings, saves, screenshots, and state to platform directories.
 * Derived cache is not copied and is removed only when moving the installation.
 * Existing destination entries are never overwritten.
 * @param source Selected portable directories; unrelated neighbouring files are preserved.
 * @param destination Normal platform directories, outside the portable source trees.
 * @param copyOnly Whether to preserve the portable installation after copying.
 * @return An error on failure, or an empty optional on success.
 * Copy failures preserve source data; cleanup failures preserve the completed destination.
 */
std::optional<std::string>
MigratePortablePaths(const GamePaths &source, const GamePaths &destination, bool copyOnly = false);

/**
 * Export platform data as a portable installation, without copying the executable.
 * Existing destination entries are never overwritten. Derived cache is regenerated.
 * @param source Normal platform directories with imported assets and BossRally.ini.
 * @param directory Absolute destination directory for assets, settings, and state.
 * @param copyOnly Whether to preserve platform data after copying.
 * @return An error on failure, or an empty optional on success.
 */
std::optional<std::string> MakePortablePaths(const GamePaths &source,
                                             const std::filesystem::path &directory,
                                             bool copyOnly = false);

/**
 * Export installed program files and platform data into a complete portable distribution.
 * Bundles use CoastlineData beside the copied bundle; bin packages store data at their root.
 * Source program files are preserved. Data is removed only after publishing the completed copy.
 * @param source Normal platform directories with imported assets and BossRally.ini.
 * @param executable Absolute SDL base directory within an installed bundle or bin directory.
 * @param directory New absolute parent directory for the complete portable distribution.
 * @param copyOnly Whether to preserve platform data after copying.
 * @return An error on failure, or an empty optional on success.
 */
std::optional<std::string> MakePortableDistribution(const GamePaths &source,
                                                    const std::filesystem::path &executable,
                                                    const std::filesystem::path &directory,
                                                    bool copyOnly = false);

/**
 * Clear imported assets and asset caches, or all application data for a factory reset.
 *
 * @param paths Absolute, non-root application directories whose generated data is removed.
 * @param factoryReset Whether to remove all four directories, including settings and saves.
 * @return An error message on failure, or std::nullopt on success.
 */
std::optional<std::string> ResetGamePaths(const GamePaths &paths, bool factoryReset = false);

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
