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
#include <utility>

#include "assetstorage.h"
#include "config.h"
#include "controllerpak.h"
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

void CopyPortableEntry(const std::filesystem::path &from,
                       const std::filesystem::path &to,
                       std::error_code &error) {
    const auto status = std::filesystem::symlink_status(from, error);
    if (error) {
        return;
    }
    if (std::filesystem::is_symlink(status)) {
        std::filesystem::copy_symlink(from, to, error);
    } else if (std::filesystem::is_directory(status)) {
        if (!std::filesystem::create_directory(to, error)) {
            if (!error) {
                error = std::make_error_code(std::errc::file_exists);
            }
            return;
        }
        std::filesystem::directory_iterator entry(from, error);
        for (; !error && entry != std::filesystem::directory_iterator(); entry.increment(error)) {
            CopyPortableEntry(entry->path(), to / entry->path().filename(), error);
        }
        if (!error) {
            // Imported directories are read-only; protect the copy only after writing its children.
            std::filesystem::permissions(to, status.permissions(), error);
        }
    } else {
        std::filesystem::copy_file(from, to, error);
    }
}

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

bool PortablePathsEnabled() {
    return COASTLINE_PORTABLE != 0;
}

std::filesystem::path PortableRoot(const std::filesystem::path &executable,
                                   const std::filesystem::path &appImage) {
    if (appImage.is_absolute()) {
        return appImage.parent_path() / "CoastlineData";
    }
    for (auto directory = executable.lexically_normal(); !directory.empty();) {
        if (directory.filename() == "Contents" && directory.parent_path().extension() == ".app") {
            return directory.parent_path().parent_path() / "CoastlineData";
        }
        const auto parent = directory.parent_path();
        if (parent == directory) {
            break;
        }
        directory = parent;
    }
    auto directory = executable.lexically_normal();
    if (directory.filename().empty()) {
        directory = directory.parent_path();
    }
    return directory.filename() == "bin" ? directory.parent_path() : executable;
}

std::expected<GamePaths, std::string>
ResolvePortablePaths(const GamePaths &fallback,
                     const std::filesystem::path &executable,
                     const std::filesystem::path &workingDirectory) {
    GamePaths paths = fallback;
#if COASTLINE_PORTABLE
    for (const auto &directory :
         {executable, workingDirectory / "CoastlineData", workingDirectory}) {
        if (directory.empty() || !directory.is_absolute()) {
            continue;
        }
        std::error_code error;
        if (!paths.portableConfig &&
            std::filesystem::is_regular_file(directory / "BossRally.ini", error)) {
            paths.config = directory;
            paths.portableConfig = true;
        }
        if (!paths.portableData && std::filesystem::is_directory(directory / "assets", error)) {
            paths.data = directory;
            paths.portableData = true;
        }
    }
    if (!paths.portableConfig && !paths.portableData) {
        return paths;
    }
    const auto local = paths.portableData ? paths.data : paths.config;
    paths.state = local / "state";
    paths.cache = local / "cache";
    std::error_code error;
    constexpr auto writeBits = std::filesystem::perms::owner_write |
                               std::filesystem::perms::group_write |
                               std::filesystem::perms::others_write;
    auto writable = [&](const std::filesystem::path &path) {
        const auto permissions = std::filesystem::status(path, error).permissions();
        return !error && (permissions & writeBits) != std::filesystem::perms::none;
    };
    std::set<std::filesystem::path> directories{local};
    if (paths.portableConfig) {
        directories.insert(paths.config);
    }
    for (const auto &directory : directories) {
        if (!writable(directory) ||
            (paths.portableConfig && directory == paths.config &&
             !writable(directory / "BossRally.ini")) ||
            (paths.portableData && directory == paths.data && !writable(directory / "assets"))) {
            return std::unexpected("Portable files cannot be written in " + directory.string() +
                                   ".");
        }
        const auto probe =
            directory /
            (".coastline-write-check-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        if (!std::filesystem::create_directory(probe, error)) {
            return std::unexpected("Cannot write portable directory " + directory.string() + ". " +
                                   error.message());
        }
        std::filesystem::remove(probe, error);
    }
    if (paths.portableConfig && !std::ofstream(paths.config / "BossRally.ini", std::ios::app)) {
        return std::unexpected("Portable configuration cannot be written in " +
                               paths.config.string() + ".");
    }
#else
    (void)executable;
    (void)workingDirectory;
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

namespace {

std::optional<std::string>
TransferGamePaths(const GamePaths &source,
                  const GamePaths &destination,
                  bool copyOnly,
                  const std::function<std::optional<std::string>()> &publish = {}) {
    if (!source.portableConfig && !source.portableData) {
        return "No portable installation found.";
    }
    using Entry = std::pair<std::filesystem::path, std::filesystem::path>;
    std::vector<Entry> entries;
    if (source.portableConfig) {
        entries.emplace_back(source.config / "BossRally.ini", destination.config / "BossRally.ini");
    }
    if (source.portableData) {
        for (const auto *entry : {"assets", "screenshots", "ghost-frames.bin"}) {
            entries.emplace_back(source.data / entry, destination.data / entry);
        }
    }
    entries.emplace_back(source.state, destination.state);
    std::error_code error;
    std::set<std::filesystem::path> pakDirectories;
    if (source.portableConfig) {
        pakDirectories.insert(source.config / "controller-pak");
    }
    if (source.portableData) {
        pakDirectories.insert(source.data / "controller-pak");
    }
    for (const auto &directory : pakDirectories) {
        std::filesystem::directory_iterator entry(directory, error);
        if (error == std::errc::no_such_file_or_directory) {
            error.clear();
            continue;
        }
        for (; !error && entry != std::filesystem::directory_iterator(); entry.increment(error)) {
            const auto name = entry->path().filename();
            if (name.extension() != ".lock") {
                const auto &root =
                    name == PAK_SETTINGS_NOTE ? destination.config : destination.data;
                entries.emplace_back(entry->path(), root / "controller-pak" / name);
            }
        }
        if (error) {
            return "Cannot inspect Controller Pak data. " + error.message();
        }
    }
    auto nested = [](const std::filesystem::path &child, const std::filesystem::path &parent) {
        auto component = child.begin();
        for (const auto &part : parent) {
            if (component == child.end()) {
                return false;
            }
#ifdef _WIN32
            if (CompareStringOrdinal(component->c_str(), -1, part.c_str(), -1, TRUE) !=
                CSTR_EQUAL) {
                return false;
            }
#else
            if (*component != part) {
                return false;
            }
#endif
            ++component;
        }
        return true;
    };
    std::vector<std::filesystem::path> roots{source.state, source.cache};
    if (source.portableConfig) {
        roots.push_back(source.config);
    }
    if (source.portableData) {
        roots.push_back(source.data);
    }
    for (const auto &from : roots) {
        for (const auto &to :
             {destination.config, destination.data, destination.state, destination.cache}) {
            const auto origin = std::filesystem::weakly_canonical(from, error);
            if (error || !origin.is_absolute() || origin == origin.root_path()) {
                return "Cannot resolve portable migration source.";
            }
            const auto target = std::filesystem::weakly_canonical(to, error);
            if (error || !target.is_absolute() || target == target.root_path() ||
                nested(origin, target) || nested(target, origin)) {
                return "Portable and platform directories must not overlap.";
            }
        }
    }
    std::vector<Entry> existing;
    std::set<std::filesystem::path> destinations;
    for (const auto &[from, to] : entries) {
        const auto status = std::filesystem::symlink_status(from, error);
        if (error == std::errc::no_such_file_or_directory) {
            error.clear();
            continue;
        }
        if (error) {
            return "Cannot inspect portable data. " + error.message();
        }
        if (status.type() == std::filesystem::file_type::not_found) {
            continue;
        }
        if (!destinations.insert(to).second) {
            return "Multiple source files have the same destination. " + to.string();
        }
        const auto target = std::filesystem::symlink_status(to, error);
        if (error && error != std::errc::no_such_file_or_directory) {
            return "Cannot inspect migration destination. " + error.message();
        }
        error.clear();
        if (target.type() != std::filesystem::file_type::not_found) {
            return "Migration destination " + to.string() + " already exists.";
        }
        existing.emplace_back(from, to);
    }
    std::vector<std::filesystem::path> copied;
    auto rollback = [&] {
        for (const auto &path : copied) {
            std::error_code cleanup;
            RemoveAssetTree(path, cleanup);
        }
    };
    for (const auto &[from, to] : existing) {
        std::filesystem::create_directories(to.parent_path(), error);
        if (!error) {
            copied.push_back(to);
            CopyPortableEntry(from, to, error);
        }
        if (error) {
            const auto message = "Cannot copy portable data. " + error.message();
            rollback();
            return message;
        }
    }
    const auto ini = destination.config / "BossRally.ini";
    if (source.portableConfig && std::filesystem::is_regular_file(ini, error)) {
        auto config = GameConfig::Load(ini);
        for (const auto *key : {"LastDirectory",
                                "DiscImage",
                                "RomImage1",
                                "RomImage2",
                                "RomImage3",
                                "SearchDirectory"}) {
            if (const auto value = config.Get("SetupWizard", key); value && !value->empty()) {
                const auto path =
                    std::filesystem::path(std::u8string(value->begin(), value->end()));
                if (path.is_relative() && *path.begin() != "~") {
                    const auto absolute = (source.config / path).lexically_normal().u8string();
                    config.Set("SetupWizard", key, std::string(absolute.begin(), absolute.end()));
                }
            }
        }
        if (!config.Save(ini)) {
            rollback();
            return "Cannot save migrated settings.";
        }
    }
    if (publish) {
        if (const auto failure = publish()) {
            rollback();
            return failure;
        }
    }
    if (copyOnly) {
        return std::nullopt;
    }
    for (const auto &[from, to] : existing) {
        (void)to;
        RemoveAssetTree(from, error);
        if (error) {
            return "Data copied, but cannot remove portable source. " + error.message();
        }
    }
    for (const auto &directory : pakDirectories) {
        RemoveAssetTree(directory, error);
        if (error) {
            return "Data copied, but cannot remove Controller Pak source. " + error.message();
        }
    }
    RemoveAssetTree(source.cache, error);
    return error ?
               std::optional("Data copied, but cannot remove portable cache. " + error.message()) :
               std::nullopt;
}

} // namespace

std::optional<std::string>
MigratePortablePaths(const GamePaths &source, const GamePaths &destination, bool copyOnly) {
    return TransferGamePaths(source, destination, copyOnly);
}

std::optional<std::string>
MakePortablePaths(const GamePaths &source, const std::filesystem::path &directory, bool copyOnly) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(source.config / "BossRally.ini", error) ||
        !std::filesystem::is_directory(source.data / "assets", error)) {
        return "No non-portable installation found.";
    }
    auto selected = source;
    selected.portableConfig = true;
    selected.portableData = true;
    const GamePaths destination{
        directory, directory, directory / "state", directory / "cache", true, true};
    return MigratePortablePaths(selected, destination, copyOnly);
}

std::optional<std::string> MakePortableDistribution(const GamePaths &source,
                                                    const std::filesystem::path &executable,
                                                    const std::filesystem::path &directory,
                                                    bool copyOnly) {
    std::error_code error;
    auto destinationAbsent = [&] {
        const auto status = std::filesystem::symlink_status(directory, error);
        if (error == std::errc::no_such_file_or_directory) {
            error.clear();
            return true;
        }
        return !error && status.type() == std::filesystem::file_type::not_found;
    };
    if (!directory.is_absolute() || directory == directory.root_path() || !destinationAbsent()) {
        return "Portable distribution destination must be a new absolute directory.";
    }
    if (!std::filesystem::is_regular_file(source.config / "BossRally.ini", error) ||
        !std::filesystem::is_directory(source.data / "assets", error)) {
        return "No non-portable installation found.";
    }
    std::filesystem::path bundle;
    for (auto parent = executable.lexically_normal(); !parent.empty();) {
        if (parent.extension() == ".app") {
            bundle = parent;
            break;
        }
        const auto next = parent.parent_path();
        if (next == parent) {
            break;
        }
        parent = next;
    }
    std::vector<std::filesystem::path> programs;
    if (!bundle.empty()) {
        programs.push_back(bundle);
    } else {
        const auto root = PortableRoot(executable);
        if (!std::filesystem::is_directory(root / "bin", error)) {
            return "Cannot locate the installed bin directory.";
        }
        for (const auto *entry : {"bin",
                                  "lib",
                                  "libexec",
                                  "share",
                                  "Coastline.lnk",
                                  "LICENSE",
                                  "LICENSE.txt",
                                  "README.md"}) {
            if (std::filesystem::exists(root / entry, error)) {
                programs.push_back(root / entry);
            }
            if (error) {
                return "Cannot inspect installed program files. " + error.message();
            }
        }
    }
    for (const auto &program : programs) {
        auto parent = directory;
        while (!parent.empty()) {
            if (std::filesystem::equivalent(program, parent, error)) {
                return "Portable distribution destination must be outside installed program files.";
            }
            const auto next = parent.parent_path();
            if (next == parent) {
                break;
            }
            parent = next;
        }
    }
    error.clear();
    std::filesystem::create_directories(directory.parent_path(), error);
    if (error) {
        return "Cannot create portable distribution parent. " + error.message();
    }
    const auto staging =
        directory.parent_path() /
        (".coastline-portable-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!std::filesystem::create_directory(staging, error)) {
        return "Cannot create portable distribution staging directory. " + error.message();
    }
    auto cleanup = [&] {
        std::error_code failure;
        RemoveAssetTree(staging, failure);
    };
    for (const auto &program : programs) {
        CopyPortableEntry(program, staging / program.filename(), error);
        if (error) {
            const auto failure = "Cannot copy installed program files. " + error.message();
            cleanup();
            return failure;
        }
    }
    const auto data = bundle.empty() ? staging : staging / "CoastlineData";
    const GamePaths destination{data, data, data / "state", data / "cache", true, true};
    auto selected = source;
    selected.portableConfig = true;
    selected.portableData = true;
    const auto failure =
        TransferGamePaths(selected, destination, copyOnly, [&]() -> std::optional<std::string> {
            if (!destinationAbsent()) {
                return "Portable distribution destination already exists.";
            }
            std::filesystem::rename(staging, directory, error);
            return error ?
                       std::optional("Cannot publish portable distribution. " + error.message()) :
                       std::nullopt;
        });
    cleanup();
    return failure;
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
