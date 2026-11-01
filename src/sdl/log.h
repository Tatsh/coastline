#pragma once

#include <filesystem>
#include <memory>

namespace coastline {

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
