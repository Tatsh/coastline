#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace coastline {

/**
 * Abbreviate home-directory paths in a log message without changing similarly prefixed paths.
 * Windows drive and UNC paths accept either separator and match ASCII letters without case
 * sensitivity.
 *
 * @param message The formatted message, possibly including several paths.
 * @param home The absolute home directory, with or without a trailing separator. Empty and
 * single-character directories disable abbreviation.
 * @return The message with recognised home-directory prefixes replaced by tildes.
 */
std::string AbbreviateLogPaths(std::string_view message, std::string_view home);

/**
 * Forward SDL messages to a file and the previous callback until destruction.
 * Destroy nested instances in reverse construction order to restore callbacks correctly.
 */
class GameLog {
public:
    /**
     * Open @p path, truncating an existing log. SDL messages still go to the previous callback
     * when the file cannot be opened.
     *
     * @param path The log file to create or truncate.
     */
    explicit GameLog(const std::filesystem::path &path);
    /** Close the log file and restore the SDL callback active before construction. */
    ~GameLog();
    /** Copying is disabled because this log registers a callback tied to its lifetime. */
    GameLog(const GameLog &) = delete;
    /** Assignment is disabled because callback restoration follows construction order. */
    GameLog &operator=(const GameLog &) = delete;

    /** @return Whether the log file is open. */
    bool IsOpen() const;

private:
    friend void StopGameLog();
    struct State;
    std::unique_ptr<State> m_state;
};

/** Close the active game log and restore its previous callback before fatal process exit. */
void StopGameLog();

} // namespace coastline
