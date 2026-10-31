#pragma once

#include <filesystem>
#include <memory>

namespace coastline {

/**
 * Forward SDL messages to a file and the previous callback until destruction. Nested instances
 * restore callbacks in reverse construction order.
 */
class GameLog {
public:
    /**
     * Open @p path, truncating an existing log. SDL messages still go to the previous callback
     * when the file cannot be opened.
     */
    explicit GameLog(const std::filesystem::path &path);
    ~GameLog();
    GameLog(const GameLog &) = delete;
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
