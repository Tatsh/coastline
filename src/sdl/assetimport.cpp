#include "assetimport.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <memory>
#include <set>
#include <string_view>
#include <system_error>
#include <vector>

#include "assetpack.h"
#include "assetstorage.h"
#include "importversion.h"
#include "paths.h"

namespace coastline {

namespace {

constexpr std::array<std::string_view, 4> GENERATED_PACKS{"boss", "eur", "jpn", "default"};
constexpr std::array<std::string_view, 3> REGIONS{"usa", "jpn", "eur"};
constexpr std::array<ImportSource, 3> SOURCES{
    ImportSource::Usa, ImportSource::Jpn, ImportSource::Eur};

struct RomInput {
    std::filesystem::path path;
    RomRelease release;
};

class ImportStage {
public:
    std::filesystem::path path;
    bool preserve = false;

    ~ImportStage() {
        if (!preserve && !path.empty()) {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    }

    static std::expected<std::unique_ptr<ImportStage>, std::string>
    Create(const std::filesystem::path &data) {
        std::error_code error;
        std::filesystem::create_directories(data, error);
        if (error) {
            return std::unexpected("Cannot create the import directory. " + error.message());
        }
        auto stage = std::make_unique<ImportStage>();
        auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 100; ++attempt) {
            auto candidate = data / (".coastline-import-" + std::to_string(tick) + "-" +
                                     std::to_string(attempt));
            if (std::filesystem::create_directory(candidate, error)) {
                stage->path = std::move(candidate);
                return stage;
            }
            if (error) {
                return std::unexpected("Cannot stage the import. " + error.message());
            }
        }
        return std::unexpected("Cannot allocate a staging directory for the import.");
    }

    std::optional<std::string> Install(const std::filesystem::path &data) {
        const auto destination = data / "assets";
        const auto previous = path / "previous";
        std::error_code error;
        std::filesystem::create_directories(destination, error);
        if (error) {
            return "Cannot create the assets directory. " + error.message();
        }
        std::filesystem::create_directory(previous, error);
        if (error) {
            return "Cannot back up generated packs. " + error.message();
        }
        std::vector<std::string_view> moved;
        std::vector<std::string_view> installed;
        auto restore = [&](const std::string &failure) -> std::optional<std::string> {
            bool restored = true;
            for (auto pack : installed) {
                std::filesystem::remove_all(destination / pack, error);
                restored = !error && restored;
            }
            for (auto pack : moved) {
                std::filesystem::rename(previous / pack, destination / pack, error);
                restored = !error && restored;
            }
            if (!restored) {
                preserve = true;
                return failure + " Restore previous packs from " + previous.string() + ".";
            }
            return failure;
        };
        for (auto pack : GENERATED_PACKS) {
            auto status = std::filesystem::symlink_status(destination / pack, error);
            if (status.type() == std::filesystem::file_type::not_found) {
                continue;
            }
            if (error) {
                return restore("Cannot inspect generated packs. " + error.message());
            }
            std::filesystem::rename(destination / pack, previous / pack, error);
            if (error) {
                return restore("Cannot replace generated packs. " + error.message());
            }
            moved.push_back(pack);
        }
        for (auto pack : GENERATED_PACKS) {
            if (!std::filesystem::is_directory(path / "assets" / pack, error)) {
                continue;
            }
            // Default is installed last, after every file its inventory records is in place.
            std::filesystem::rename(path / "assets" / pack, destination / pack, error);
            if (error) {
                return restore("Cannot install generated packs. " + error.message());
            }
            installed.push_back(pack);
        }
        if (!DefaultPackUsable(DefaultAssets(data))) {
            return restore("The installed assets failed their inventory check.");
        }
        return std::nullopt;
    }
};

std::optional<std::string> WriteMetadata(const std::filesystem::path &defaults, RomRelease base) {
    const auto metadata = defaults / "pack.json";
    std::ifstream input(metadata);
    auto settings = nlohmann::ordered_json::parse(input, nullptr, false);
    input.close();
    if (!settings.is_object()) {
        return "The imported pack metadata is invalid.";
    }
    settings["baseRegion"] = REGIONS[static_cast<std::size_t>(base)];
    std::ofstream output(metadata, std::ios::binary | std::ios::trunc);
    output << settings.dump(2) << '\n';
    output.close();
    if (!output) {
        return "Cannot record the imported base region.";
    }
    const auto boss = defaults.parent_path() / BOSS_PACK;
    std::error_code error;
    std::filesystem::create_directories(boss, error);
    if (error) {
        return "Cannot create the Boss Rally pack. " + error.message();
    }
    std::ofstream bossSettings(boss / "pack.json", std::ios::binary | std::ios::trunc);
    bossSettings << nlohmann::ordered_json{{"version", PACK_SETTINGS_VERSION}}.dump(2) << '\n';
    bossSettings.close();
    return bossSettings ? std::nullopt :
                          std::optional<std::string>("Cannot record the Boss Rally pack metadata.");
}

} // namespace

std::optional<std::string> ImportAssetImages(const std::filesystem::path &data,
                                             const std::filesystem::path &disc,
                                             std::span<const std::filesystem::path> roms,
                                             const ImportProgress &progress) {
    if (disc.empty() || roms.empty()) {
        return "Import requires a Boss Rally disc image and at least one Top Gear Rally ROM "
               "image. Use coastline import --cd DISC --rom ROM.";
    }
    if (auto failure = CheckDisc(disc)) {
        return failure;
    }
    std::vector<RomInput> inputs;
    std::set<RomRelease> releases;
    for (const auto &rom : roms) {
        auto release = IdentifyRomFile(rom);
        if (!release) {
            return release.error();
        }
        if (!releases.insert(*release).second) {
            return "Supply only one ROM image for each region.";
        }
        inputs.push_back({rom, *release});
    }
    std::stable_partition(inputs.begin(), inputs.end(), [](const RomInput &input) {
        return input.release == RomRelease::Usa;
    });
    auto stage = ImportStage::Create(data);
    if (!stage) {
        return stage.error();
    }
    auto defaults = DefaultAssets((*stage)->path);
    constexpr double DISC_SHARE = 4.0;
    const double total = static_cast<double>(inputs.size()) + DISC_SHARE + 1.0;
    double done = 0.0;
    double reported = 0.0;
    auto report = [&](const std::string &step, double fraction) {
        reported = std::max(reported, std::clamp(fraction, 0.0, 1.0));
        return !progress || progress(step, reported);
    };
    for (const auto &input : inputs) {
        auto imported =
            ImportRom(input.path, defaults, [&](const std::string &step, double fraction) {
                return report(step, (done + fraction) / total);
            });
        if (!imported) {
            return imported.error();
        }
        if (*imported != input.release) {
            return "The ROM image changed during import.";
        }
        if (!RecordImport(defaults, SOURCES[static_cast<std::size_t>(*imported)])) {
            return "Cannot record the imported ROM version.";
        }
        done += 1.0;
    }
    auto imported = ImportDisc(disc, defaults, [&](const std::string &step, double fraction) {
        return report(step, (done + fraction * DISC_SHARE) / total);
    });
    if (!imported) {
        return imported.error();
    }
    if (!RecordImport(defaults, ImportSource::Disc)) {
        return "Cannot record the imported disc version.";
    }
    if (!report("Preparing the asset packs", (done + DISC_SHARE) / total)) {
        return "The import was cancelled.";
    }
    if (auto failure = CurateAssetPacks(defaults)) {
        return failure;
    }
    if (auto failure = WriteMetadata(defaults, inputs.front().release)) {
        return failure;
    }
    auto storage = DeduplicateAssetFiles(defaults);
    if (!storage) {
        return storage.error();
    }
    if (!RecordImportFiles(defaults) || !DefaultPackUsable(defaults)) {
        return "The staged assets failed their inventory check.";
    }
    if (!report("Installing the asset packs", 1.0)) {
        return "The import was cancelled.";
    }
    return (*stage)->Install(data);
}

} // namespace coastline
