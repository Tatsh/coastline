#pragma once

/**
 * @file
 * The background writer of the configuration file.
 */

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
    /** Start the writing thread. */
    ConfigSaver();
    /** Write the copy that waits, if any, and stop the thread. */
    ~ConfigSaver();
    /** Prevent copying the writing thread. */
    ConfigSaver(const ConfigSaver &) = delete;
    /**
     * Prevent assignment of the writing thread.
     *
     * @return This saver.
     */
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

    /**
     * Report the result of the last finished write without waiting for queued copies.
     *
     * @return Whether the last finished write failed.
     */
    bool Failed();

private:
    // Write each queued copy outside the lock and log a failure, until the saver stops with no
    // copy queued.
    void Run();

    std::mutex m_mutex;             // Guards the queued copy and the flags.
    std::condition_variable m_wake; // Signalled when a copy is queued or the saver stops.
    std::condition_variable m_idle; // Signalled after each write finishes.
    // The copy and the INI file waiting for the thread, replaced by each Save.
    std::optional<std::pair<GameConfig, std::filesystem::path>> m_pending;
    bool m_writing = false;
    bool m_failed = false; // Whether the last finished write failed.
    bool m_stop = false;   // Set by the destructor to end the thread.
    std::thread m_thread;  // Runs Run.
};

} // namespace coastline
