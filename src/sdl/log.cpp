#include "log.h"

#include <chrono>
#include <format>
#include <fstream>
#include <memory>
#include <mutex>

#include <SDL3/SDL.h>

namespace coastline {

namespace {

struct LogState {
    std::mutex mutex;
    std::ofstream file;
    SDL_LogOutputFunction defaultOutput = nullptr;
    void *defaultUserData = nullptr;
};

std::unique_ptr<LogState> g_pLog;

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

void SDLCALL WriteLogMessage(void *userData,
                             int category,
                             SDL_LogPriority priority,
                             const char *message) {
    auto *state = static_cast<LogState *>(userData);
    {
        std::scoped_lock lock(state->mutex);
        if (state->file) {
            auto now =
                std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
            state->file << std::format("{:%F %T} [{}] {}\n", now, PriorityName(priority), message);
            state->file.flush();
        }
    }
    if (state->defaultOutput != nullptr) {
        state->defaultOutput(state->defaultUserData, category, priority, message);
    }
}

} // namespace

bool StartGameLog(const std::filesystem::path &path) {
    g_pLog = std::make_unique<LogState>();
    SDL_GetLogOutputFunction(&g_pLog->defaultOutput, &g_pLog->defaultUserData);
    g_pLog->file.open(path, std::ios::out | std::ios::trunc);
    SDL_SetLogOutputFunction(WriteLogMessage, g_pLog.get());
    return g_pLog->file.is_open();
}

void StopGameLog() {
    if (!g_pLog) {
        return;
    }
    SDL_SetLogOutputFunction(g_pLog->defaultOutput, g_pLog->defaultUserData);
    g_pLog.reset();
}

} // namespace coastline
