#include <algorithm>
#include <array>
#include <cctype>
#include <system_error>

#include "../discformats.h"
#include "setupwizard.h"

namespace coastline {

WizardSearchResult SearchWizardDirectory(const std::filesystem::path &directory,
                                         const WizardServices &services,
                                         const WizardSearchProgress &progress) {
    WizardSearchResult result;
    auto report = [&](const std::string &step, double fraction) {
        return !progress || progress(result, step, fraction);
    };
    constexpr std::array<std::string_view, 4> extensions{".z64", ".n64", ".v64", ".zip"};
    std::vector<std::filesystem::path> candidates;
    std::error_code error;
    std::filesystem::recursive_directory_iterator entry(
        directory, std::filesystem::directory_options::skip_permission_denied, error),
        end;
    if (error) {
        result.failure = "The directory cannot be read.";
        return result;
    }
    while (entry != end) {
        if (!report("Looking in " + Utf8FromPath(entry->path().parent_path()), 0.0)) {
            return result;
        }
        if (entry->is_regular_file(error)) {
            const auto extension = LowercaseExtension(entry->path());
            if (IsDiscImageExtension(extension) || std::ranges::contains(extensions, extension)) {
                candidates.push_back(entry->path());
            } else if (LowercaseExtension(entry->path()) == ".pod") {
                std::string filename = entry->path().filename().string();
                std::ranges::transform(filename, filename.begin(), [](unsigned char c) {
                    return static_cast<char>(std::tolower(c));
                });
                if (filename == "bossrally.pod") {
                    candidates.push_back(entry->path().parent_path());
                }
            }
        }
        entry.increment(error);
        if (error) {
            result.failure = "Part of the directory could not be read.";
            break;
        }
    }
    auto rank = [](const std::filesystem::path &path) {
        auto extension = LowercaseExtension(path);
        return extension == ".cue" || extension == ".ccd" || extension == ".mds" ? 0 :
               extension == ".iso" || extension == ".nrg"                        ? 1 :
                                                                                   2;
    };
    std::ranges::sort(candidates, [&](const auto &left, const auto &right) {
        return std::pair(rank(left), left) < std::pair(rank(right), right);
    });
    result.candidates = candidates.size();
    for (const auto &path : candidates) {
        const double fraction = static_cast<double>(result.examined) /
                                static_cast<double>(std::max<std::size_t>(result.candidates, 1));
        if (!report("Checking " + Utf8FromPath(path), fraction)) {
            return result;
        }
        const auto extension = LowercaseExtension(path);
        if (extension == ".zip") {
            if (services.scanArchive) {
                services.scanArchive(
                    path,
                    [&](WizardCartridge release, const std::filesystem::path &cached) {
                        auto &found = result.roms[static_cast<std::size_t>(release)];
                        if (found.empty()) {
                            found = cached;
                        }
                        return report("Found ROM image in " + Utf8FromPath(path), fraction);
                    },
                    [&](const std::string &member, double part) {
                        return report(
                            "Checking " + Utf8FromPath(path) + ": " + member,
                            (static_cast<double>(result.examined) + part) /
                                static_cast<double>(std::max<std::size_t>(result.candidates, 1)));
                    });
            }
        } else if (std::filesystem::is_directory(path, error)) {
            if (result.disc.empty() && !services.checkDisc(path)) {
                result.disc = path;
            }
        } else if (IsDiscImageExtension(extension)) {
            if (result.disc.empty() && !services.checkDisc(path)) {
                result.disc = path;
            }
        }
        if (extension != ".zip" && (extension == ".bin" || !IsDiscImageExtension(extension)) &&
            result.disc != path) {
            const auto size = std::filesystem::file_size(path, error);
            if (!error && size <= 32 * 1024 * 1024) {
                if (auto release = services.identifyCartridge(path)) {
                    auto &found = result.roms[static_cast<std::size_t>(*release)];
                    if (found.empty()) {
                        found = path;
                    }
                }
            }
        }
        ++result.examined;
        if (!report("Checked " + Utf8FromPath(path),
                    static_cast<double>(result.examined) /
                        static_cast<double>(std::max<std::size_t>(result.candidates, 1)))) {
            return result;
        }
        if (!result.disc.empty() &&
            std::ranges::all_of(result.roms, [](const auto &rom) { return !rom.empty(); })) {
            break;
        }
    }
    report("Search complete", 1.0);
    return result;
}

} // namespace coastline
