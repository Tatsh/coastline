#include "assetstorage.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <system_error>

namespace coastline {

namespace {

constexpr std::array<const char *, 4> GENERATED_PACKS{"default", "boss", "eur", "jpn"};

std::string Utf8(const std::filesystem::path &path) {
    auto text = path.generic_u8string();
    return std::string(text.begin(), text.end());
}

std::filesystem::path FromUtf8(const std::string &text) {
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

bool RelativeFile(const std::filesystem::path &path) {
    if (path.empty() || path.has_root_path() || path != path.lexically_normal()) {
        return false;
    }
    for (const auto &part : path) {
        if (part == ".." || part == "." || part.empty()) {
            return false;
        }
    }
    return Utf8(path).find('\0') == std::string::npos;
}

std::expected<std::string, std::string> ReadFile(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::unexpected("Cannot read " + Utf8(path) + ".");
    }
    std::string bytes(std::istreambuf_iterator<char>(file), {});
    if (file.bad()) {
        return std::unexpected("Cannot finish reading " + Utf8(path) + ".");
    }
    return bytes;
}

struct StoredFile {
    std::filesystem::path pack;
    std::filesystem::path path;
};

struct PendingAlias {
    std::filesystem::path path;
    std::shared_ptr<StoredFile> target;
};

void MakeRemovable(const std::filesystem::path &path, std::error_code &error) {
    const auto status = std::filesystem::symlink_status(path, error);
    if (status.type() == std::filesystem::file_type::not_found) {
        error.clear();
        return;
    }
    if (error || std::filesystem::is_symlink(status)) {
        return;
    }
    const bool directory = std::filesystem::is_directory(status);
    bool writable = directory;
#ifdef _WIN32
    writable |= std::filesystem::is_regular_file(status);
#endif
    if (writable) {
        std::filesystem::permissions(
            path, std::filesystem::perms::owner_write, std::filesystem::perm_options::add, error);
        if (error) {
            return;
        }
    }
    if (directory) {
        std::filesystem::directory_iterator entry(path, error), end;
        for (; !error && entry != end; entry.increment(error)) {
            MakeRemovable(entry->path(), error);
            if (error) {
                return;
            }
        }
    }
}

} // namespace

void RemoveAssetTree(const std::filesystem::path &path, std::error_code &error) {
    error.clear();
    MakeRemovable(path, error);
    if (!error) {
        std::filesystem::remove_all(path, error);
    }
}

std::optional<std::string> FinalizeGeneratedAssetPacks(const std::filesystem::path &defaults) {
    constexpr auto writes = std::filesystem::perms::owner_write |
                            std::filesystem::perms::group_write |
                            std::filesystem::perms::others_write;
    try {
        for (const auto *name : GENERATED_PACKS) {
            const auto pack = defaults.parent_path() / name;
            if (!std::filesystem::exists(pack)) {
                continue;
            }
            if (std::filesystem::is_symlink(pack)) {
                return "Generated assets must not include symbolic links.";
            }
            std::vector<std::filesystem::path> directories{pack};
            for (const auto &entry : std::filesystem::recursive_directory_iterator(pack)) {
                if (entry.is_symlink()) {
                    return "Generated assets must not include symbolic links.";
                }
                if (entry.is_directory()) {
                    directories.push_back(entry.path());
                } else {
                    std::filesystem::permissions(
                        entry.path(), writes, std::filesystem::perm_options::remove);
                }
            }
            for (auto directory = directories.rbegin(); directory != directories.rend();
                 ++directory) {
                if (*directory != pack && std::filesystem::is_empty(*directory)) {
                    std::filesystem::remove(*directory);
                } else {
                    std::filesystem::permissions(
                        *directory, writes, std::filesystem::perm_options::remove);
                }
            }
        }
    } catch (const std::filesystem::filesystem_error &error) {
        return "Cannot finalize generated assets. " + std::string(error.what());
    }
    return std::nullopt;
}

std::expected<std::vector<AssetFileAlias>, std::string>
ReadAssetFileAliases(const std::filesystem::path &pack, const std::filesystem::path &defaults) {
    const auto table = pack / ASSET_ALIASES_FILE;
    std::error_code error;
    if (!std::filesystem::exists(table, error) && !error) {
        return std::vector<AssetFileAlias>{};
    }
    std::ifstream file(table, std::ios::binary);
    auto invalid = [&]() {
        return std::unexpected("Invalid asset aliases in " + Utf8(table) + ".");
    };
    std::string word;
    int version = 0;
    if (!(file >> word >> version) || word != "coastline-asset-aliases" || version != 1) {
        return invalid();
    }
    std::set<std::filesystem::path> paths;
    std::vector<AssetFileAlias> aliases;
    while (file >> word) {
        if (word == "end") {
            file >> std::ws;
            for (const auto &alias : aliases) {
                for (auto parent = alias.path.parent_path(); !parent.empty();
                     parent = parent.parent_path()) {
                    if (paths.contains(parent)) {
                        return invalid();
                    }
                }
            }
            return file.eof() ?
                       std::expected<std::vector<AssetFileAlias>, std::string>(std::move(aliases)) :
                       invalid();
        }
        std::string path, target;
        if ((word != "self" && word != "default") ||
            !(file >> std::quoted(path) >> std::quoted(target))) {
            return invalid();
        }
        AssetFileAlias alias{FromUtf8(path), FromUtf8(target), word == "default"};
        if (!RelativeFile(alias.path) || !RelativeFile(alias.target) ||
            alias.path == ASSET_ALIASES_FILE || alias.path == ".layer-stamp" ||
            !paths.insert(alias.path).second || std::filesystem::exists(pack / alias.path, error) ||
            error ||
            !std::filesystem::is_regular_file((alias.fromDefault ? defaults : pack) / alias.target,
                                              error) ||
            error) {
            return invalid();
        }
        for (auto parent = alias.path.parent_path(); !parent.empty();
             parent = parent.parent_path()) {
            if (paths.contains(parent)) {
                return invalid();
            }
        }
        aliases.push_back(std::move(alias));
    }
    return invalid();
}

std::expected<AssetStorageSavings, std::string>
DeduplicateAssetFiles(const std::filesystem::path &defaults) {
    std::map<std::filesystem::path, std::vector<std::filesystem::path>> files;
    std::map<std::filesystem::path, std::vector<PendingAlias>> aliases;
    for (const char *name : GENERATED_PACKS) {
        const auto pack = defaults.parent_path() / name;
        std::error_code error;
        if (!std::filesystem::is_directory(pack, error)) {
            continue;
        }
        if (std::filesystem::exists(pack / ASSET_ALIASES_FILE, error)) {
            return std::unexpected("Asset files have already been deduplicated.");
        }
        std::filesystem::recursive_directory_iterator entry(pack, error), end;
        for (; !error && entry != end; entry.increment(error)) {
            if (entry->is_symlink(error)) {
                return std::unexpected("Generated assets must not include symbolic links.");
            }
            const auto relative = entry->path().lexically_relative(pack);
            if (entry->is_regular_file(error) && relative != "pack.json" &&
                relative != "import.ini" && relative != "import-files.txt" &&
                relative != "n64/base/cartridge.ini") {
                files[pack].push_back(entry->path());
            }
        }
        if (error) {
            return std::unexpected("Cannot list generated assets. " + error.message());
        }
        std::ranges::sort(files[pack]);
    }
    using Key = std::pair<std::size_t, std::size_t>;
    std::map<Key, std::vector<std::shared_ptr<StoredFile>>> stored;
    AssetStorageSavings savings;
    std::size_t sharedIndex = 0;
    for (const char *name : GENERATED_PACKS) {
        const auto pack = defaults.parent_path() / name;
        for (const auto &path : files[pack]) {
            auto bytes = ReadFile(path);
            if (!bytes) {
                return std::unexpected(bytes.error());
            }
            auto &candidates = stored[{bytes->size(), std::hash<std::string>{}(*bytes)}];
            std::shared_ptr<StoredFile> same;
            for (const auto &candidate : candidates) {
                auto existing = ReadFile(candidate->path);
                if (!existing) {
                    return std::unexpected(existing.error());
                }
                // Hash collisions never establish equality.
                if (*existing == *bytes) {
                    same = candidate;
                    break;
                }
            }
            if (!same) {
                candidates.push_back(std::make_shared<StoredFile>(pack, path));
                continue;
            }
            std::error_code error;
            if (same->pack != pack && same->pack != defaults) {
                auto shared = defaults / "shared";
                std::filesystem::create_directories(shared, error);
                if (error) {
                    return std::unexpected("Cannot create shared asset storage. " +
                                           error.message());
                }
                do {
                    shared = defaults / "shared" /
                             (std::to_string(sharedIndex++) + same->path.extension().string());
                } while (std::filesystem::exists(shared, error) && !error);
                auto originalPack = same->pack;
                auto originalPath = same->path.lexically_relative(originalPack);
                std::filesystem::rename(same->path, shared, error);
                if (error) {
                    return std::unexpected("Cannot share an asset file. " + error.message());
                }
                same->path = shared;
                same->pack = defaults;
                aliases[originalPack].push_back({originalPath, same});
            }
            aliases[pack].push_back({path.lexically_relative(pack), same});
            if (!std::filesystem::remove(path, error) || error) {
                return std::unexpected("Cannot remove a duplicate asset. " + error.message());
            }
            ++savings.files;
            savings.bytes += bytes->size();
        }
    }
    for (const auto &[pack, list] : aliases) {
        std::ofstream table(pack / ASSET_ALIASES_FILE, std::ios::binary | std::ios::trunc);
        table << "coastline-asset-aliases 1\n";
        for (const auto &alias : list) {
            table << (alias.target->pack == pack ? "self" : "default") << ' '
                  << std::quoted(Utf8(alias.path)) << ' '
                  << std::quoted(Utf8(alias.target->path.lexically_relative(alias.target->pack)))
                  << '\n';
        }
        table << "end\n";
        table.close();
        if (!table) {
            return std::unexpected("Cannot record asset aliases.");
        }
    }
    return savings;
}

} // namespace coastline
