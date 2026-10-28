#include "paths.h"

#include <cstdlib>
#include <system_error>

#ifdef _WIN32
#include <shlobj.h>
#endif

namespace coastline {

namespace {

std::optional<std::filesystem::path> AbsoluteVariable(const EnvironmentReader &environment,
                                                      const char *name) {
    auto value = environment(name);
    if (!value) {
        return std::nullopt;
    }
    std::filesystem::path path(*value);
    if (!path.is_absolute()) {
        return std::nullopt;
    }
    return path;
}

#ifdef _WIN32
// SHGetFolderPathW is the earliest documented call that reports these folders, and it is present
// from Windows 2000.
std::optional<std::filesystem::path> KnownFolder(int folder) {
    wchar_t buffer[MAX_PATH];
    if (FAILED(SHGetFolderPathW(
            nullptr, folder | CSIDL_FLAG_CREATE, nullptr, SHGFP_TYPE_CURRENT, buffer))) {
        return std::nullopt;
    }
    return std::filesystem::path(buffer);
}
#endif

} // namespace

std::optional<GamePaths> ResolveXdgPaths(const EnvironmentReader &environment,
                                         const std::string &application) {
    auto home = AbsoluteVariable(environment, "HOME");
    auto base = [&](const char *name,
                    const char *fallback) -> std::optional<std::filesystem::path> {
        if (auto value = AbsoluteVariable(environment, name)) {
            return *value / application;
        }
        if (home) {
            return *home / fallback / application;
        }
        return std::nullopt;
    };
    auto config = base("XDG_CONFIG_HOME", ".config");
    auto data = base("XDG_DATA_HOME", ".local/share");
    auto state = base("XDG_STATE_HOME", ".local/state");
    auto cache = base("XDG_CACHE_HOME", ".cache");
    if (!config || !data || !state || !cache) {
        return std::nullopt;
    }
    return GamePaths{*config, *data, *state, *cache};
}

#ifdef __APPLE__
// Implemented in paths-apple.mm with NSFileManager.
std::optional<GamePaths> ResolveApplePaths(const std::string &application);
#endif

std::optional<GamePaths> ResolveGamePaths(const std::string &application) {
#if defined(_WIN32)
    auto roaming = KnownFolder(CSIDL_APPDATA);
    auto local = KnownFolder(CSIDL_LOCAL_APPDATA);
    if (!roaming || !local) {
        return std::nullopt;
    }
    return GamePaths{*roaming / application,
                     *roaming / application / "data",
                     *local / application / "state",
                     *local / application / "cache"};
#elif defined(__APPLE__)
    return ResolveApplePaths(application);
#else
    return ResolveXdgPaths(
        [](const char *name) -> std::optional<std::string> {
            const char *value = std::getenv(name);
            if (value == nullptr || *value == '\0') {
                return std::nullopt;
            }
            return std::string(value);
        },
        application);
#endif
}

bool CreateGamePaths(const GamePaths &paths) {
    for (const auto *path : {&paths.config, &paths.data, &paths.state, &paths.cache}) {
        std::error_code error;
        std::filesystem::create_directories(*path, error);
        if (!std::filesystem::is_directory(*path, error)) {
            return false;
        }
    }
    return true;
}

} // namespace coastline
