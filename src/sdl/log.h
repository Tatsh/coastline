#pragma once

#include <filesystem>

namespace coastline {

/**
 * Send every SDL log message to @p path as well as to the default output. The file is truncated
 * when the game starts.
 *
 * @param path The log file.
 * @return Whether the file opened. Messages still go to the default output when it did not.
 */
bool StartGameLog(const std::filesystem::path &path);

/** Flush and close the log file, and restore the default output. */
void StopGameLog();

} // namespace coastline
