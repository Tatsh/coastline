#ifndef COASTLINE_GAMEMODE_H
#define COASTLINE_GAMEMODE_H

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
    /** Prevent replacement of an active GameMode registration. */
    GameModeSession &operator=(const GameModeSession &) = delete;

private:
#if defined(__linux__)
    SDL_SharedObject *m_library = nullptr;
    int (*m_end)() = nullptr;
    const char *(*m_error)() = nullptr;
    bool m_active = false;
#endif
};

} // namespace coastline

#endif
