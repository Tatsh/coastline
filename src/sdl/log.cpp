#include "log.h"

#include <chrono>
#include <fstream>
#include <memory>
#include <mutex>

#include <SDL3/SDL.h>
#include <fmt/chrono.h>
#include <fmt/format.h>

namespace coastline {

namespace {

const char *PriorityName(SDL_LogPriority priority) {
    switch (priority) {
    case SDL_LOG_PRIORITY_TRACE:
        return "trace";
    case SDL_LOG_PRIORITY_VERBOSE:
        return "verbose";
    case SDL_LOG_PRIORITY_DEBUG:
        return "debug";
    case SDL_LOG_PRIORITY_INFO:
        return "info";
    case SDL_LOG_PRIORITY_WARN:
        return "warning";
    case SDL_LOG_PRIORITY_ERROR:
        return "error";
    case SDL_LOG_PRIORITY_CRITICAL:
        return "critical";
    default:
        return "unknown";
    }
}

} // namespace

struct GameLog::State {
    std::mutex mutex;
    std::ofstream file;
    SDL_LogOutputFunction defaultOutput = nullptr;
    void *defaultUserData = nullptr;
    bool active = false;

    explicit State(const std::filesystem::path &path) {
        SDL_GetLogOutputFunction(&defaultOutput, &defaultUserData);
        file.open(path, std::ios::out | std::ios::trunc);
        SDL_SetLogOutputFunction(Output, this);
        active = true;
    }

    ~State() {
        Stop();
    }

    void Stop() {
        if (!active) {
            return;
        }
        SDL_SetLogOutputFunction(defaultOutput, defaultUserData);
        std::scoped_lock lock(mutex);
        file.close();
        active = false;
    }

    static void SDLCALL Output(void *userData,
                               int category,
                               SDL_LogPriority priority,
                               const char *message) {
        auto *state = static_cast<State *>(userData);
        {
            std::scoped_lock lock(state->mutex);
            if (state->file) {
                auto now =
                    std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
                state->file << fmt::format(
                    "{:%F %T} [{}] {}\n", now, PriorityName(priority), message);
                state->file.flush();
            }
        }
        if (state->defaultOutput != nullptr) {
            state->defaultOutput(state->defaultUserData, category, priority, message);
        }
    }
};

GameLog::GameLog(const std::filesystem::path &path) : m_state(std::make_unique<State>(path)) {
}

GameLog::~GameLog() = default;

bool GameLog::IsOpen() const {
    return m_state->file.is_open();
}

void StopGameLog() {
    SDL_LogOutputFunction callback = nullptr;
    void *userData = nullptr;
    SDL_GetLogOutputFunction(&callback, &userData);
    if (callback == GameLog::State::Output) {
        static_cast<GameLog::State *>(userData)->Stop();
    }
}

} // namespace coastline
