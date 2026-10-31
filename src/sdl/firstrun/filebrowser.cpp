#include "filebrowser.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <system_error>
#include <tuple>

namespace coastline {

namespace {

std::string AsciiLowercase(std::string text) {
    std::ranges::transform(
        text, text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

void SortByName(std::vector<BrowserEntry> &entries) {
    std::ranges::sort(entries, [](const BrowserEntry &a, const BrowserEntry &b) {
        return std::tuple(AsciiLowercase(a.name), a.name) <
               std::tuple(AsciiLowercase(b.name), b.name);
    });
}

} // namespace

std::string Utf8FromPath(const std::filesystem::path &path) {
    auto native = path;
    std::u8string text = native.make_preferred().u8string();
    return {text.begin(), text.end()};
}

std::filesystem::path PathFromUtf8(std::string_view text) {
    return std::filesystem::path(std::u8string(text.begin(), text.end())).make_preferred();
}

std::string LowercaseExtension(const std::filesystem::path &path) {
    return AsciiLowercase(Utf8FromPath(path.extension()));
}

std::optional<std::vector<BrowserEntry>>
ListBrowserDirectory(const std::filesystem::path &directory,
                     std::span<const std::string_view> extensions,
                     bool directoriesOnly) {
    std::error_code error;
    std::filesystem::directory_iterator iterator(directory, error);
    if (error) {
        return std::nullopt;
    }
    std::vector<BrowserEntry> directories;
    std::vector<BrowserEntry> files;
    const std::filesystem::directory_iterator end;
    while (iterator != end) {
        const auto entry = *iterator;
        iterator.increment(error);
        if (error) {
            return std::nullopt;
        }
        std::string name = Utf8FromPath(entry.path().filename());
        if (name.empty() || name.front() == '.') {
            continue;
        }
        std::error_code statusError;
        if (entry.is_directory(statusError)) {
            directories.push_back(
                {BrowserEntry::Kind::Directory, entry.path(), name, "[" + name + "]"});
        } else if (!directoriesOnly && !statusError && entry.is_regular_file(statusError) &&
                   (extensions.empty() ||
                    std::ranges::contains(extensions, LowercaseExtension(entry.path())))) {
            files.push_back({BrowserEntry::Kind::File, entry.path(), name, name});
        }
    }
    SortByName(directories);
    SortByName(files);
    std::vector<BrowserEntry> entries;
    // A trailing separator would make the directory its own parent.
    std::filesystem::path normal = directory.lexically_normal();
    if (!normal.has_filename() && normal.has_relative_path()) {
        normal = normal.parent_path();
    }
    if (normal.has_relative_path()) {
        entries.push_back({BrowserEntry::Kind::Parent, normal.parent_path(), "..", ".."});
    }
#ifdef _WIN32
    if (!normal.has_relative_path()) {
        for (char letter = 'A'; letter <= 'Z'; ++letter) {
            std::string root = std::string(1, letter) + ":\\";
            std::error_code driveError;
            if (std::filesystem::exists(root, driveError)) {
                entries.push_back({BrowserEntry::Kind::Drive, root, root, root});
            }
        }
    }
#endif
    std::ranges::move(directories, std::back_inserter(entries));
    std::ranges::move(files, std::back_inserter(entries));
    return entries;
}

std::filesystem::path ResolveUserPath(std::string_view text, const std::filesystem::path &home) {
    if (!home.empty() && (text == "~" || text.starts_with("~/") || text.starts_with("~\\"))) {
        auto resolved = text.size() == 1 ? home : home / PathFromUtf8(text.substr(2));
        return resolved.make_preferred();
    }
    return PathFromUtf8(text);
}

} // namespace coastline
