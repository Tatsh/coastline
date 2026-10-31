#pragma once

/**
 * @file
 * The listing of a directory for the file browser of the setup wizard.
 */

#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace coastline {

/** An entry of a listing of the file browser. */
struct BrowserEntry {
    /** What an entry opens or chooses. */
    enum class Kind {
        Parent,    /*!< The directory above, `..`. */
        Drive,     /*!< A drive, at the root of a drive on Windows. */
        Directory, /*!< A directory. */
        File,      /*!< A file. */
    };
    Kind kind = Kind::File;     /*!< What the entry opens or chooses. */
    std::filesystem::path path; /*!< The path the entry opens or chooses. */
    std::string name;           /*!< The name of the entry in UTF-8, without brackets. */
    std::string label;          /*!< The text of the entry in UTF-8: .., [name], C:\, or name. */
};

/**
 * List a directory for the file browser: `..` first, or on Windows the drives at the root of a
 * drive, then the directories, then the files with one of @p extensions. The directories and the
 * files are each sorted by name without regard to case. An entry whose name starts with a full stop
 * is hidden and not listed.
 *
 * @param directory The directory, absolute.
 * @param extensions The extensions of the files listed, lowercase with the full stop, such as
 * .cue, or empty for every file.
 * @return The entries, or nothing when the directory cannot be read.
 */
std::optional<std::vector<BrowserEntry>>
ListBrowserDirectory(const std::filesystem::path &directory,
                     std::span<const std::string_view> extensions);

/**
 * @param path A path.
 * @return The path in UTF-8.
 */
std::string Utf8FromPath(const std::filesystem::path &path);

/**
 * @param text A path in UTF-8.
 * @return The path.
 */
std::filesystem::path PathFromUtf8(std::string_view text);

/**
 * @param path A path.
 * @return The extension of the path in ASCII lowercase, with the full stop, or empty.
 */
std::string LowercaseExtension(const std::filesystem::path &path);

} // namespace coastline
