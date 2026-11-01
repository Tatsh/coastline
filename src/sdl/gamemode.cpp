#include "gamemode.h"

#include <SDL3/SDL_log.h>

namespace coastline {

GameModeSession::GameModeSession(bool enabled) {
#if defined(__linux__)
    if (!enabled) {
        return;
    }
    m_library = SDL_LoadObject("libgamemode.so.0");
    if (!m_library) {
        m_library = SDL_LoadObject("libgamemode.so");
    }
    if (!m_library) {
        SDL_Log("GameMode client unavailable.");
        return;
    }
    auto start =
        reinterpret_cast<int (*)()>(SDL_LoadFunction(m_library, "real_gamemode_request_start"));
    m_end = reinterpret_cast<int (*)()>(SDL_LoadFunction(m_library, "real_gamemode_request_end"));
    m_error = reinterpret_cast<const char *(*)()>(
        SDL_LoadFunction(m_library, "real_gamemode_error_string"));
    if (!start || !m_end || !m_error) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "GameMode client API unavailable.");
        return;
    }
    m_active = start() == 0;
    if (m_active) {
        SDL_Log("GameMode registration active.");
    } else {
        const char *error = m_error();
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Cannot enable GameMode: %s",
                    error ? error : "Unknown error.");
    }
#else
    (void)enabled;
#endif
}

GameModeSession::~GameModeSession() {
#if defined(__linux__)
    if (m_active) {
        if (m_end() == 0) {
            SDL_Log("GameMode registration inactive.");
        } else {
            const char *error = m_error();
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Cannot disable GameMode: %s",
                        error ? error : "Unknown error.");
        }
    }
    if (m_library) {
        SDL_UnloadObject(m_library);
    }
#endif
}

} // namespace coastline
