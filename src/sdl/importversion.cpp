#include "importversion.h"

#include <array>
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
