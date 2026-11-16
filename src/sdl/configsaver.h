#pragma once

#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

#include "config.h"

namespace coastline {

/**
 * Writes copies of the configuration on a thread, away from the frames. A copy saved while
 * another waits replaces it, and the copies are written in the order they are saved.
 */
class ConfigSaver {
public:
    ConfigSaver();
    /** Write the copy that waits, if any, and stop the thread. */
    ~ConfigSaver();
    /** Prevent copying the writing thread. */
    ConfigSaver(const ConfigSaver &) = delete;
    /** Prevent assignment of the writing thread. */
    ConfigSaver &operator=(const ConfigSaver &) = delete;

    /**
     * Queue a copy of the configuration for writing.
     *
     * @param config The configuration to write.
     * @param path The INI file.
     */
    void Save(GameConfig config, std::filesystem::path path);

    /**
     * Wait until every queued copy is written.
     *
     * @return Whether the last write succeeded, or true when nothing was written.
     */
    bool Flush();

    /** @return Whether the last finished write failed, without waiting. */
    bool Failed();

private:
    void Run();

    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::condition_variable m_idle;
    std::optional<std::pair<GameConfig, std::filesystem::path>> m_pending;
    bool m_writing = false;
    bool m_failed = false;
    bool m_stop = false;
    std::thread m_thread;
};

} // namespace coastline
