#include "paths.h"

#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iterator>
#include <set>
#include <string>
#include <system_error>

#include "assetstorage.h"
#include "importversion.h"

#ifndef COASTLINE_SYSTEM_DATA_DIR
#define COASTLINE_SYSTEM_DATA_DIR ""
#endif
#ifndef COASTLINE_PORTABLE
#define COASTLINE_PORTABLE 1
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
    bool canLink = true;
#ifdef _WIN32
    // Clearing a cached hard link's read-only attribute also changes its imported source.
    canLink = (std::filesystem::status(from, error).permissions() &
               std::filesystem::perms::owner_write) != std::filesystem::perms::none;
#endif
    if (canLink) {
        std::filesystem::create_hard_link(from, to, error);
    } else {
        return std::filesystem::copy_file(from, to, error);
    }
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
    RemoveAssetTree(expanded, error);
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
    const std::filesystem::path packPath(std::u8string(pack.begin(), pack.end()));
    const auto directory = data / "assets" / packPath;
    std::error_code error;
    return fallback.empty() || std::filesystem::is_directory(directory, error) ?
               directory :
               fallback / "assets" / packPath;
}

std::optional<std::filesystem::path> PackAssets(const std::filesystem::path &data,
                                                const std::filesystem::path &cache,
                                                std::string_view pack,
                                                const std::filesystem::path &fallback,
                                                std::string_view region) {
    auto directory = PackDirectory(data, pack, fallback);
    std::error_code error;
    if (pack.empty() || !std::filesystem::is_directory(directory, error)) {
        return std::nullopt;
    }
    auto base = ExpandAliases(DefaultAssets(data), DefaultAssets(data), cache);
    if (!base) {
        return std::nullopt;
    }
    auto layer = [&](const std::filesystem::path &source, const std::filesystem::path &layered) {
        auto selected = ExpandAliases(source, DefaultAssets(data), cache);
        if (!selected) {
            return false;
        }
        const auto stampPath = layered / LAYER_STAMP;
        auto stamp = LayerStamp(*base, source);
        if (!stamp || ReadStamp(stampPath) != *stamp) {
            RemoveAssetTree(layered, error);
            if (error || !LayerDirectory(*base, *selected, layered)) {
                return false;
            }
            if (stamp) {
                std::ofstream file(stampPath);
                file << *stamp << '\n';
                file.close();
                if (!file) {
                    return false;
                }
            }
        }
        base = layered;
        return true;
    };
    if (!region.empty()) {
        if ((region != "usa" && region != "eur" && region != "jpn") ||
            !std::filesystem::is_directory(PackDirectory(data, region, fallback), error) ||
            !layer(PackDirectory(data, region, fallback), cache / "assets" / "regions" / region)) {
            return std::nullopt;
        }
    }
    if (pack != DEFAULT_PACK && pack != region &&
        !layer(directory,
               region.empty() ? cache / "assets" / std::u8string(pack.begin(), pack.end()) :
                                cache / "assets" / "selected" / region /
                                    std::u8string(pack.begin(), pack.end()))) {
        return std::nullopt;
    }
    return base;
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

std::expected<GamePaths, std::string>
ResolvePortablePaths(const GamePaths &fallback, const std::filesystem::path &executable) {
    GamePaths paths = fallback;
#if COASTLINE_PORTABLE
    if (executable.empty()) {
        return paths;
    }
    std::error_code error;
    paths.portableConfig = std::filesystem::is_regular_file(executable / "BossRally.ini", error);
    paths.portableData = std::filesystem::is_directory(executable / "assets", error);
    if (!paths.portableConfig && !paths.portableData) {
        return paths;
    }
    constexpr auto writeBits = std::filesystem::perms::owner_write |
                               std::filesystem::perms::group_write |
                               std::filesystem::perms::others_write;
    auto writable = [&](const std::filesystem::path &path) {
        const auto permissions = std::filesystem::status(path, error).permissions();
        return !error && (permissions & writeBits) != std::filesystem::perms::none;
    };
    if (!writable(executable) ||
        (paths.portableConfig && !writable(executable / "BossRally.ini")) ||
        (paths.portableData && !writable(executable / "assets"))) {
        return std::unexpected("Portable files cannot be written in " + executable.string() + ".");
    }
    const auto probe =
        executable / (".coastline-write-check-" +
                      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!std::filesystem::create_directory(probe, error)) {
        return std::unexpected("Cannot write portable directory " + executable.string() + ". " +
                               error.message());
    }
    std::filesystem::remove(probe, error);
    if (paths.portableConfig && !std::ofstream(executable / "BossRally.ini", std::ios::app)) {
        return std::unexpected("Portable configuration cannot be written in " +
                               executable.string() + ".");
    }
    if (paths.portableConfig) {
        paths.config = executable;
    }
    if (paths.portableData) {
        paths.data = executable;
        paths.state = executable / "state";
        paths.cache = executable / "cache";
    }
#else
    (void)executable;
#endif
    return paths;
}

std::filesystem::path StorePortablePath(const std::filesystem::path &path,
                                        const std::filesystem::path &root,
                                        const std::filesystem::path &home) {
    if (path.empty() || path.is_relative()) {
        return path;
    }
    const auto relative = path.lexically_normal().lexically_relative(root.lexically_normal());
    if (!relative.empty() && *relative.begin() != "..") {
        return relative;
    }
    if (!home.empty()) {
        const auto fromHome = path.lexically_normal().lexically_relative(home.lexically_normal());
        if (!fromHome.empty() && *fromHome.begin() != "..") {
            return std::filesystem::path("~") / fromHome;
        }
    }
    return relative.empty() ? path : relative;
}

std::filesystem::path RestorePortablePath(const std::filesystem::path &path,
                                          const std::filesystem::path &root,
                                          const std::filesystem::path &home) {
    if (path.empty() || path.is_absolute()) {
        return path;
    }
    if (*path.begin() == "~") {
        if (home.empty()) {
            return path;
        }
        auto rebuilt = home;
        for (auto component = std::next(path.begin()); component != path.end(); ++component) {
            rebuilt /= *component;
        }
        return rebuilt.lexically_normal();
    }
    return (root / path).lexically_normal();
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
    directories.emplace_back("/Library/Application Support/Coastline");
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

std::optional<std::string> ResetGamePaths(const GamePaths &paths, bool factoryReset) {
    for (const auto &directory : {paths.config, paths.data, paths.state, paths.cache}) {
        const auto normalized = directory.lexically_normal();
        if (!normalized.is_absolute() || normalized == normalized.root_path()) {
            return "Cannot reset an empty, relative, or root directory.";
        }
    }
    std::vector<std::filesystem::path> directories;
    if (factoryReset) {
        if (paths.portableConfig) {
            directories = {paths.config / "BossRally.ini", paths.config / "controller-pak"};
        } else {
            directories.push_back(paths.config);
        }
        if (paths.portableData) {
            for (const auto *entry :
                 {"assets", "controller-pak", "screenshots", "ghost-frames.bin"}) {
                directories.push_back(paths.data / entry);
            }
        } else {
            directories.push_back(paths.data);
        }
        directories.push_back(paths.state);
        directories.push_back(paths.cache);
    } else {
        for (const auto *pack : {"default", "boss", "eur", "jpn"}) {
            directories.push_back(paths.data / "assets" / pack);
        }
        for (const auto *cache : {"assets", "asset-sources", "asset-search"}) {
            directories.push_back(paths.cache / cache);
        }
    }
    for (const auto &directory : directories) {
        std::error_code error;
        RemoveAssetTree(directory, error);
        if (error) {
            return "Cannot delete " + directory.string() + ". " + error.message();
        }
    }
    return std::nullopt;
}

} // namespace coastline
