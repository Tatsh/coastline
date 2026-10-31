#include "paths.h"

#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <set>
#include <string>
#include <system_error>

#include "assetstorage.h"
#include "importversion.h"

#ifndef COASTLINE_SYSTEM_DATA_DIR
#define COASTLINE_SYSTEM_DATA_DIR ""
#endif
#ifndef COASTLINE_RELATIVE_DATA_DIR
#define COASTLINE_RELATIVE_DATA_DIR "../share/coastline"
#endif

#ifdef _WIN32
#include <shlobj.h>
#endif

namespace coastline {

namespace {

constexpr const char *LAYER_STAMP = ".layer-stamp";

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
        if (name == LAYER_STAMP) {
            continue;
        }
        bool inPack = std::filesystem::exists(pack / name, error);
        if (!inPack && (name.extension() == ".gltf" || name.extension() == ".glb")) {
            auto alternative = name;
            alternative.replace_extension(name.extension() == ".gltf" ? ".glb" : ".gltf");
            if (std::filesystem::is_regular_file(pack / alternative, error)) {
                continue;
            }
        }
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

// A hash of the path and the write time of every entry of both trees, or nothing when a tree
// cannot be read. Adding, removing, or writing an entry changes it.
std::optional<std::string> LayerStamp(const std::filesystem::path &base,
                                      const std::filesystem::path &pack) {
    std::set<std::string> lines;
    for (const auto *tree : {&base, &pack}) {
        std::error_code error;
        std::filesystem::recursive_directory_iterator entry(*tree, error);
        for (; !error && entry != std::filesystem::recursive_directory_iterator();
             entry.increment(error)) {
            auto time = entry->last_write_time(error);
            if (error) {
                return std::nullopt;
            }
            lines.insert(entry->path().string() + '\t' +
                         std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                            time.time_since_epoch())
                                            .count()));
        }
        if (error) {
            return std::nullopt;
        }
    }
    std::string listing = "asset-aliases-1\n";
    for (const auto &line : lines) {
        listing += line + '\n';
    }
    return std::to_string(std::hash<std::string>{}(listing));
}

std::string ReadStamp(const std::filesystem::path &path) {
    std::ifstream in(path);
    std::string stamp;
    std::getline(in, stamp);
    return stamp;
}

std::optional<std::filesystem::path> ExpandAliases(const std::filesystem::path &pack,
                                                   const std::filesystem::path &defaults,
                                                   const std::filesystem::path &cache) {
    auto aliases = ReadAssetFileAliases(pack, defaults);
    if (!aliases) {
        return std::nullopt;
    }
    if (aliases->empty()) {
        return pack;
    }
    auto expanded = cache / "asset-sources" / pack.filename();
    auto stamp = LayerStamp(defaults, pack);
    if (stamp && ReadStamp(expanded / LAYER_STAMP) == *stamp) {
        return expanded;
    }
    std::error_code error;
    std::filesystem::remove_all(expanded, error);
    if (error) {
        return std::nullopt;
    }
    std::filesystem::create_directories(expanded, error);
    if (error) {
        return std::nullopt;
    }
    std::filesystem::recursive_directory_iterator entry(pack, error), end;
    for (; !error && entry != end; entry.increment(error)) {
        auto relative = entry->path().lexically_relative(pack);
        auto to = expanded / relative;
        if (entry->is_directory(error)) {
            // Alias destinations require real directories, never links into physical storage.
            std::filesystem::create_directories(to, error);
        } else if (relative != LAYER_STAMP && !LinkEntry(entry->path(), to)) {
            return std::nullopt;
        }
    }
    if (error) {
        return std::nullopt;
    }
    for (const auto &alias : *aliases) {
        auto to = expanded / alias.path;
        std::filesystem::create_directories(to.parent_path(), error);
        if (error || !LinkEntry((alias.fromDefault ? defaults : pack) / alias.target, to)) {
            return std::nullopt;
        }
    }
    if (stamp) {
        std::ofstream file(expanded / LAYER_STAMP);
        file << *stamp << '\n';
        file.close();
        if (!file) {
            return std::nullopt;
        }
    }
    return expanded;
}

} // namespace

std::filesystem::path PackDirectory(const std::filesystem::path &data,
                                    std::string_view pack,
                                    const std::filesystem::path &fallback) {
    const auto directory = data / "assets" / pack;
    std::error_code error;
    return fallback.empty() || std::filesystem::is_directory(directory, error) ?
               directory :
               fallback / "assets" / pack;
}

std::optional<std::filesystem::path> PackAssets(const std::filesystem::path &data,
                                                const std::filesystem::path &cache,
                                                std::string_view pack,
                                                const std::filesystem::path &fallback) {
    auto directory = PackDirectory(data, pack, fallback);
    std::error_code error;
    if (pack.empty() || !std::filesystem::is_directory(directory, error)) {
        return std::nullopt;
    }
    auto base = ExpandAliases(DefaultAssets(data), DefaultAssets(data), cache);
    if (!base) {
        return std::nullopt;
    }
    if (pack == DEFAULT_PACK) {
        return base;
    }
    auto selected = ExpandAliases(directory, DefaultAssets(data), cache);
    if (!selected) {
        return std::nullopt;
    }
    auto layered = cache / "assets" / pack;
    auto stampPath = layered / LAYER_STAMP;
    auto stamp = LayerStamp(DefaultAssets(data), directory);
    if (stamp && ReadStamp(stampPath) == *stamp) {
        return layered;
    }
    std::filesystem::remove_all(layered, error);
    if (!LayerDirectory(*base, *selected, layered)) {
        return std::nullopt;
    }
    if (stamp) {
        std::ofstream(stampPath) << *stamp << '\n';
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

std::vector<std::filesystem::path> SystemDataDirectories(const std::filesystem::path &executable) {
    std::vector<std::filesystem::path> directories;
    if (!executable.empty()) {
#ifdef __APPLE__
        directories.push_back(executable / "../Resources/coastline");
#endif
        directories.push_back(executable / COASTLINE_RELATIVE_DATA_DIR);
    }
    if (std::string_view(COASTLINE_SYSTEM_DATA_DIR).size() > 0) {
        directories.emplace_back(COASTLINE_SYSTEM_DATA_DIR);
    }
#ifdef __APPLE__
    directories.emplace_back("/Library/Application Support/coastline");
#endif
#ifndef _WIN32
    directories.emplace_back("/usr/local/share/coastline");
    directories.emplace_back("/usr/share/coastline");
#endif
    for (auto &directory : directories) {
        directory = directory.lexically_normal().make_preferred();
    }
    return directories;
}

std::optional<std::filesystem::path> FindAssetData(const std::filesystem::path &user,
                                                   std::span<const std::filesystem::path> system) {
    for (const auto &directory : system) {
        if (DefaultPackUsable(DefaultAssets(directory))) {
            return directory;
        }
    }
    return DefaultPackUsable(DefaultAssets(user)) ? std::optional(user) : std::nullopt;
}

std::optional<std::string> ResetGamePaths(const GamePaths &paths) {
    const std::array directories{paths.config, paths.data, paths.state, paths.cache};
    for (const auto &directory : directories) {
        const auto normalized = directory.lexically_normal();
        if (!normalized.is_absolute() || normalized == normalized.root_path()) {
            return "Cannot reset an empty, relative, or root directory.";
        }
    }
    for (const auto &directory : directories) {
        std::error_code error;
        std::filesystem::remove_all(directory, error);
        if (error) {
            return "Cannot delete " + directory.string() + ". " + error.message();
        }
    }
    return std::nullopt;
}

} // namespace coastline
