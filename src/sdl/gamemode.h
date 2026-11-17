#ifndef COASTLINE_GAMEMODE_H
#define COASTLINE_GAMEMODE_H

/**
 * @file
 * The request for Feral GameMode on Linux.
 */

#include <SDL3/SDL_loadso.h>

namespace coastline {

/** Optional Linux GameMode registration for the lifetime of an interactive game process. */
class GameModeSession {
public:
    /**
     * Load the Linux client and request GameMode immediately when enabled.
     * Missing libraries, incomplete clients, and rejected requests are logged without stopping
     * the game. Other platforms do not load a library or request GameMode.
     * @param enabled Whether to request GameMode. An enabled session requires SDL to remain
     * initialised until destruction.
     */
    explicit GameModeSession(bool enabled);
    /** Attempt to end a successful registration, then unload the client before SDL shuts down. */
    ~GameModeSession();
    /** Prevent duplicate ownership of a GameMode registration. */
    GameModeSession(const GameModeSession &) = delete;
    /**
     * Prevent replacement of an active GameMode registration.
     *
     * @return This object. The operator is deleted and is never called.
     */
    GameModeSession &operator=(const GameModeSession &) = delete;

private:
#if defined(__linux__)
    SDL_SharedObject *m_library = nullptr; /*!< The loaded GameMode client library. */
    int (*m_end)() = nullptr; /*!< real_gamemode_request_end() of the client, 0 on success. */
    const char *(*m_error)() = nullptr; /*!< real_gamemode_error_string() of the client. */
    bool m_active = false;              /*!< Whether the start request succeeded. */
#endif
};

} // namespace coastline

#endif
