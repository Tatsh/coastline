#pragma once

#include <memory>
#include <string_view>

namespace coastline {

/** Optional Discord Rich Presence through the local desktop client's IPC endpoint. */
class DiscordPresence {
public:
    /**
     * Start asynchronous IPC when support is compiled in and an application ID is supplied.
     * Missing clients are retried without blocking game processing. Empty or invalid IDs disable
     * communication. No remote network connection or Discord registration is performed.
     * @param applicationId Nonzero unsigned 64-bit decimal Discord application ID without leading
     * zeros, or empty to disable presence.
     */
    explicit DiscordPresence(std::string_view applicationId);
    /** Stop the worker and close IPC before returning. */
    ~DiscordPresence();
    /** Prevent duplicate ownership of an IPC session. */
    DiscordPresence(const DiscordPresence &) = delete;
    /** Prevent replacement of an active IPC session. */
    DiscordPresence &operator=(const DiscordPresence &) = delete;

    /**
     * Publish the latest activity without waiting for Discord.
     * Repeated identical updates are ignored; pending changes are coalesced and rate-limited.
     * @param details Activity description in UTF-8, limited to 128 bytes without splitting
     * codepoints.
     * @param state Additional activity state in UTF-8, limited to 128 bytes, or empty.
     */
    void Update(std::string_view details, std::string_view state = {});

private:
    struct Worker;
    std::unique_ptr<Worker> m_worker;
};

} // namespace coastline
