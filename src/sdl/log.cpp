#include "log.h"

#include <cctype>
#include <chrono>
#include <fstream>
#include <memory>
#include <mutex>
#include <vector>

#include <SDL3/SDL.h>
#include <fmt/chrono.h>
#include <fmt/format.h>

namespace coastline {

std::string AbbreviateLogPaths(std::string_view message, std::string_view home) {
    while (home.size() > 1 && (home.back() == '/' || home.back() == '\\')) {
        home.remove_suffix(1);
    }
    if (home.size() <= 1) {
        return std::string(message);
    }
    const bool windows =
        (home.size() > 2 && home[1] == ':') || home.starts_with("\\\\") || home.starts_with("//");
    auto separator = [windows](char value) { return value == '/' || (windows && value == '\\'); };
    auto equal = [&](char left, char right) {
        return (separator(left) && separator(right)) ||
               (windows ? std::tolower(static_cast<unsigned char>(left)) ==
                              std::tolower(static_cast<unsigned char>(right)) :
                          left == right);
    };
    auto space = [](char value) { return std::isspace(static_cast<unsigned char>(value)) != 0; };
    std::string result;
    for (std::size_t at = 0; at < message.size();) {
        const bool start =
            at == 0 || space(message[at - 1]) ||
            std::string_view("\"'([{=:").find(message[at - 1]) != std::string_view::npos;
        bool matches = start && message.size() - at >= home.size();
        for (std::size_t i = 0; matches && i < home.size(); ++i) {
            matches = equal(message[at + i], home[i]);
        }
        if (matches) {
            const auto end = at + home.size();
            matches =
                end == message.size() || separator(message[end]) || space(message[end]) ||
                std::string_view("\"')]} ,;").find(message[end]) != std::string_view::npos ||
                (message[end] == '.' && (end + 1 == message.size() || space(message[end + 1])));
        }
        if (matches) {
            result += '~';
            at += home.size();
        } else {
            result += message[at++];
        }
    }
    return result;
}

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
    std::vector<std::string> homeDirectories;

    explicit State(const std::filesystem::path &path) {
        if (const char *home = SDL_GetUserFolder(SDL_FOLDER_HOME)) {
            homeDirectories.emplace_back(home);
        }
        for (const char *variable : {"HOME", "USERPROFILE"}) {
            if (const char *home = SDL_getenv(variable); home && *home) {
                homeDirectories.emplace_back(home);
            }
        }
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
        std::string formatted(message);
        for (const auto &home : state->homeDirectories) {
            formatted = AbbreviateLogPaths(formatted, home);
        }
        {
            std::scoped_lock lock(state->mutex);
            if (state->file) {
                auto now =
                    std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
                state->file << fmt::format(
                    "{:%F %T} [{}] {}\n", now, PriorityName(priority), formatted);
                state->file.flush();
            }
        }
        if (state->defaultOutput != nullptr) {
            state->defaultOutput(state->defaultUserData, category, priority, formatted.c_str());
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
