#include "paths.h"

#include <cstdlib>
#include <set>
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

// A link to a file or a directory: a hard link or a symbolic link, or else a copy, of each file of
// a directory in turn.
bool LinkEntry(const std::filesystem::path &from, const std::filesystem::path &to) {
    std::error_code error;
    if (std::filesystem::is_directory(from, error)) {
        std::filesystem::create_directory_symlink(from, to, error);
        if (!error) {
            return true;
        }
        std::filesystem::create_directories(to, error);
        for (const auto &entry : std::filesystem::directory_iterator(from, error)) {
            if (!LinkEntry(entry.path(), to / entry.path().filename())) {
                return false;
            }
        }
        return !error;
    }
    std::filesystem::create_hard_link(from, to, error);
    if (!error) {
        return true;
    }
    error.clear();
    std::filesystem::create_symlink(from, to, error);
    if (!error) {
        return true;
    }
    error.clear();
    return std::filesystem::copy_file(from, to, error);
}

// The entries of base and of pack in to, an entry of pack in place of the entry of base of the
// same name, and a directory of both layered in turn.
bool LayerDirectory(const std::filesystem::path &base,
                    const std::filesystem::path &pack,
                    const std::filesystem::path &to) {
    std::error_code error;
    std::filesystem::create_directories(to, error);
    if (error) {
        return false;
    }
    std::set<std::filesystem::path> names;
    for (const auto *directory : {&base, &pack}) {
        for (const auto &entry : std::filesystem::directory_iterator(*directory, error)) {
            names.insert(entry.path().filename());
        }
    }
    for (const auto &name : names) {
        bool inPack = std::filesystem::exists(pack / name, error);
        bool layered = inPack && std::filesystem::is_directory(pack / name, error) &&
                       std::filesystem::is_directory(base / name, error);
        bool linked = layered ? LayerDirectory(base / name, pack / name, to / name) :
                                LinkEntry((inPack ? pack : base) / name, to / name);
        if (!linked) {
            return false;
        }
    }
    return true;
}

} // namespace

std::filesystem::path PackDirectory(const std::filesystem::path &data, std::string_view pack) {
    return data / "assets" / pack;
}

std::optional<std::filesystem::path> PackAssets(const std::filesystem::path &data,
                                                const std::filesystem::path &cache,
                                                std::string_view pack) {
    auto directory = PackDirectory(data, pack);
    std::error_code error;
    if (pack.empty() || !std::filesystem::is_directory(directory, error)) {
        return std::nullopt;
    }
    if (pack == DEFAULT_PACK) {
        return directory;
    }
    // ponytail: the layers are linked again on every start, a few milliseconds for a pack beside
    // whole directories of the default pack; a stamp of both packs would skip an unchanged layer.
    auto layered = cache / "assets" / pack;
    std::filesystem::remove_all(layered, error);
    if (!LayerDirectory(DefaultAssets(data), directory, layered)) {
        return std::nullopt;
    }
    return layered;
}

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
