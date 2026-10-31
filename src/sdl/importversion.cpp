#include "importversion.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <string_view>
#include <system_error>

#include "config.h"

namespace coastline {

namespace {

constexpr std::string_view SECTION = "Import";

struct SourceInfo {
    ImportSource source;
    std::string_view key;       // The key of import.ini.
    std::string_view directory; // A directory of the pack only this source writes.
    std::string_view name;      // The source in a sentence.
    int version;
};

constexpr std::array<SourceInfo, 4> SOURCES{{
    {ImportSource::Disc, "Disc", "cars", "the disc", DISC_CONVERTER_VERSION},
    {ImportSource::Usa, "Usa", "n64/usa", "the USA cartridge", CARTRIDGE_CONVERTER_VERSION},
    {ImportSource::Jpn, "Jpn", "n64/jpn", "the Japanese cartridge", CARTRIDGE_CONVERTER_VERSION},
    {ImportSource::Eur, "Eur", "n64/eur", "the European cartridge", CARTRIDGE_CONVERTER_VERSION},
}};

const SourceInfo &Info(ImportSource source) {
    return SOURCES[static_cast<std::size_t>(source)];
}

std::filesystem::path RecordPath(const std::filesystem::path &defaults) {
    return defaults / "import.ini";
}

} // namespace

bool RecordImport(const std::filesystem::path &defaults, ImportSource source) {
    auto record = GameConfig::Load(RecordPath(defaults));
    const SourceInfo &info = Info(source);
    record.Set(SECTION, info.key, std::to_string(info.version));
    return record.Save(RecordPath(defaults));
}

std::vector<ImportSource> StaleImports(const std::filesystem::path &defaults) {
    auto record = GameConfig::Load(RecordPath(defaults));
    std::vector<ImportSource> stale;
    for (const SourceInfo &info : SOURCES) {
        std::error_code error;
        bool imported = std::filesystem::is_directory(defaults / info.directory, error);
        if (imported && record.GetInteger(SECTION, info.key, 0) < info.version) {
            stale.push_back(info.source);
        }
    }
    return stale;
}

bool DefaultPackUsable(const std::filesystem::path &defaults) {
    auto record = GameConfig::Load(RecordPath(defaults));
    auto current = [&](ImportSource source) {
        const SourceInfo &info = Info(source);
        return record.GetInteger(SECTION, info.key, 0) == info.version;
    };
    if (!current(ImportSource::Disc) ||
        !(current(ImportSource::Usa) || current(ImportSource::Jpn) || current(ImportSource::Eur))) {
        return false;
    }
    std::ifstream files(defaults / "import-files.txt", std::ios::binary);
    std::size_t count = 0;
    std::string relative;
    std::uintmax_t size = 0;
    while (files >> size >> std::quoted(relative)) {
        if (relative.empty()) {
            files >> std::ws;
            return files.eof() && count > 0;
        }
        const std::filesystem::path path(std::u8string(relative.begin(), relative.end()));
        if (path.is_absolute() || path != path.lexically_normal() ||
            (path.begin() != path.end() && *path.begin() == "..")) {
            return false;
        }
        std::error_code error;
        if (!std::filesystem::is_regular_file(defaults.parent_path() / path, error) ||
            std::filesystem::file_size(defaults.parent_path() / path, error) != size || error) {
            return false;
        }
        ++count;
    }
    return false;
}

bool RecordImportFiles(const std::filesystem::path &defaults) {
    const auto manifest = defaults / "import-files.txt";
    std::error_code error;
    std::filesystem::remove(manifest, error);
    std::ofstream files(manifest, std::ios::binary | std::ios::trunc);
    for (const char *pack : {"default", "boss", "eur", "jpn"}) {
        const auto directory = defaults.parent_path() / pack;
        if (!std::filesystem::is_directory(directory, error)) {
            continue;
        }
        std::filesystem::recursive_directory_iterator entry(directory, error), end;
        while (!error && entry != end) {
            if (entry->is_regular_file(error) && entry->path() != manifest) {
                const auto relative =
                    entry->path().lexically_relative(defaults.parent_path()).u8string();
                const std::string text(relative.begin(), relative.end());
                files << entry->file_size(error) << ' ' << std::quoted(text) << '\n';
            }
            entry.increment(error);
        }
        if (error) {
            files.close();
            std::filesystem::remove(manifest, error);
            return false;
        }
    }
    files << "0 \"\"\n";
    files.close();
    return static_cast<bool>(files);
}

std::string ReimportMessage(std::span<const ImportSource> sources) {
    std::string message = "Import ";
    for (std::size_t i = 0; i < sources.size(); ++i) {
        if (i > 0 && sources.size() > 2) {
            message += ", ";
        } else if (i > 0) {
            message += ' ';
        }
        if (i > 0 && i + 1 == sources.size()) {
            message += "and ";
        }
        message += Info(sources[i]).name;
    }
    return message + " again.";
}

} // namespace coastline
