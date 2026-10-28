#pragma once

#include <filesystem>
#include <optional>

namespace coastline {

/**
 * Set the directories the file routines of the PC release read from and write to.
 *
 * @param data The data copied from the disc and the cartridges.
 * @param state The saved games and settings the game writes.
 */
void SetGameDirectories(const std::filesystem::path &data, const std::filesystem::path &state);

/**
 * Find a file of the game by the path the PC release gives it: backslashes or slashes, in any
 * case, with or without a drive. The state directory is searched before the data directory.
 *
 * @param path The path of the PC release.
 * @return The file, or nothing when it is in neither directory.
 */
std::optional<std::filesystem::path> FindGameFile(const char *path);

/**
 * The file a path of the PC release writes to: the existing file of the state directory, or a new
 * one there, whose directories are created.
 *
 * @param path The path of the PC release.
 * @return The file to write.
 */
std::filesystem::path WritableGameFile(const char *path);

} // namespace coastline
