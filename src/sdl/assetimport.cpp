#include "assetimport.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iterator>
#include <memory>
#include <set>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "assetpack.h"
#include "assetstorage.h"
#include "config.h"
#include "importjobs.h"
#include "importversion.h"
#include "paths.h"
#include "savefile.h"

namespace coastline {

namespace {

constexpr std::array<std::string_view, 4> GENERATED_PACKS{"boss", "eur", "jpn", "default"};
constexpr std::array<std::string_view, 4> REGIONS{"usa", "jpn", "eur", "asia"};
constexpr std::array<ImportSource, 4> SOURCES{
    ImportSource::Usa, ImportSource::Jpn, ImportSource::Eur, ImportSource::Asia};

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
            RemoveAssetTree(path, error);
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
        std::vector<std::pair<std::string_view, std::filesystem::perms>> moved;
        std::vector<std::string_view> installed;
        auto restore = [&](const std::string &failure) -> std::optional<std::string> {
            bool restored = true;
            for (auto pack : installed) {
                RemoveAssetTree(destination / pack, error);
                restored = !error && restored;
            }
            for (const auto &[pack, permissions] : moved) {
                std::filesystem::rename(previous / pack, destination / pack, error);
                if (!error && permissions != std::filesystem::perms::unknown) {
                    std::filesystem::permissions(destination / pack, permissions, error);
                }
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
            const auto permissions = std::filesystem::is_directory(status) ?
                                         status.permissions() :
                                         std::filesystem::perms::unknown;
            // Moving a directory between parents updates its '..' entry and requires write access.
            if (permissions != std::filesystem::perms::unknown) {
                std::filesystem::permissions(destination / pack,
                                             std::filesystem::perms::owner_write,
                                             std::filesystem::perm_options::add,
                                             error);
                if (error) {
                    return restore("Cannot prepare generated packs. " + error.message());
                }
            }
            std::filesystem::rename(destination / pack, previous / pack, error);
            if (error) {
                const auto failure = "Cannot replace generated packs. " + error.message();
                if (permissions != std::filesystem::perms::unknown) {
                    std::filesystem::permissions(destination / pack, permissions, error);
                }
                return restore(failure);
            }
            moved.emplace_back(pack, permissions);
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
            return restore("Installed asset inventory is invalid.");
        }
        if (auto failure = FinalizeGeneratedAssetPacks(DefaultAssets(data))) {
            return restore(*failure);
        }
        return std::nullopt;
    }
};

std::optional<std::string> WriteMetadata(const std::filesystem::path &defaults, RomRelease base) {
    auto record = GameConfig::Load(defaults / "import.ini");
    record.Set("Import", "Layout", "1");
    const auto baseRegion = base == RomRelease::Asia ? std::string_view("usa") :
                                                       REGIONS[static_cast<std::size_t>(base)];
    record.Set("Import", "BaseRegion", std::string(baseRegion));
    if (!record.Save(defaults / "import.ini")) {
        return "Cannot record the regional asset layout.";
    }
    const auto metadata = defaults / "pack.json";
    std::ifstream input(metadata);
    auto settings = nlohmann::ordered_json::parse(input, nullptr, false);
    input.close();
    if (!settings.is_object()) {
        return "The imported pack metadata is invalid.";
    }
    settings["baseRegion"] = baseRegion;
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
    std::ifstream bossInput(boss / "pack.json");
    auto bossMetadata = nlohmann::ordered_json::parse(bossInput, nullptr, false);
    bossInput.close();
    if (!bossMetadata.is_object()) {
        bossMetadata = {{"version", PACK_SETTINGS_VERSION}};
    }
    std::ofstream bossSettings(boss / "pack.json", std::ios::binary | std::ios::trunc);
    bossSettings << bossMetadata.dump(2) << '\n';
    bossSettings.close();
    return bossSettings ? std::nullopt :
                          std::optional<std::string>("Cannot record the Boss Rally pack metadata.");
}

void NormalizeCartridge(const std::filesystem::path &candidate,
                        std::string_view region,
                        const std::filesystem::path &fonts) {
    const auto source = candidate / "n64" / region;
    const auto gameRegion = region == "asia" ? std::string_view("usa") : region;
    std::filesystem::create_directories(fonts);
    std::filesystem::copy(
        source / "fonts", fonts / gameRegion, std::filesystem::copy_options::recursive);
    std::filesystem::remove_all(source / "fonts");
    std::filesystem::rename(source, candidate / "n64" / "base");
    GameConfig metadata;
    metadata.Set("Cartridge", "Region", std::string(gameRegion));
    if (!metadata.Save(candidate / "n64" / "base" / "cartridge.ini")) {
        throw std::filesystem::filesystem_error("Cannot record the cartridge region",
                                                candidate,
                                                std::make_error_code(std::errc::io_error));
    }
}

void RemoveUnchangedFiles(const std::filesystem::path &candidate,
                          const std::filesystem::path &defaults) {
    for (const auto &entry : std::filesystem::recursive_directory_iterator(candidate)) {
        if (!entry.is_regular_file() || entry.path() == candidate / "pack.json") {
            continue;
        }
        auto base = defaults / entry.path().lexically_relative(candidate);
        if (!std::filesystem::is_regular_file(base) ||
            entry.file_size() != std::filesystem::file_size(base)) {
            continue;
        }
        const bool unchanged = [&] {
            std::ifstream first(entry.path(), std::ios::binary), second(base, std::ios::binary);
            const std::string bytes(std::istreambuf_iterator<char>(first), {});
            const std::string original(std::istreambuf_iterator<char>(second), {});
            return first.is_open() && second.is_open() && !first.bad() && !second.bad() &&
                   bytes == original;
        }();
        if (unchanged) {
            std::filesystem::remove(entry.path());
        }
    }
}

} // namespace

std::optional<std::string> ImportAssetImages(const std::filesystem::path &data,
                                             const std::filesystem::path &disc,
                                             std::span<const std::filesystem::path> roms,
                                             const ImportProgress &progress,
                                             std::vector<std::filesystem::path> *sources) {
    if (disc.empty() || roms.empty()) {
        return "Import requires a Boss Rally disc source and at least one Top Gear Rally ROM "
               "image. Use coastline import --cd DISC --rom ROM.";
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
    if (releases.contains(RomRelease::Usa)) {
        std::erase_if(inputs,
                      [](const RomInput &input) { return input.release == RomRelease::Asia; });
    }
    std::stable_partition(inputs.begin(), inputs.end(), [](const RomInput &input) {
        return input.release == RomRelease::Usa || input.release == RomRelease::Asia;
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
    try {
        const auto pristine = (*stage)->path / "disc";
        std::vector<std::filesystem::path> discSources;
        auto importedDisc = ImportDisc(
            disc,
            pristine,
            [&](const std::string &step, double fraction) {
                return report(step, fraction * DISC_SHARE / total);
            },
            sources ? &discSources : nullptr);
        if (!importedDisc) {
            return importedDisc.error();
        }
        done += DISC_SHARE;
        std::vector<ImportJob> romJobs;
        for (const auto &input : inputs) {
            romJobs.emplace_back(
                [&, input](const ImportProgress &romProgress) -> std::optional<std::string> {
                    const auto region = REGIONS[static_cast<std::size_t>(input.release)];
                    const auto candidate = DefaultAssets((*stage)->path / "candidates" / region);
                    std::filesystem::create_directories(candidate.parent_path());
                    std::filesystem::copy(
                        pristine, candidate, std::filesystem::copy_options::recursive);
                    auto imported = ImportRom(input.path, candidate, romProgress);
                    if (!imported) {
                        return imported.error();
                    }
                    if (*imported != input.release) {
                        return "ROM image differs from the validated input.";
                    }
                    return std::nullopt;
                });
        }
        if (auto failure = RunImportJobs(romJobs, [&](const std::string &step, double fraction) {
                return report(step, (done + fraction * static_cast<double>(inputs.size())) / total);
            })) {
            return failure;
        }
        for (std::size_t index = 0; index < inputs.size(); ++index) {
            const auto &input = inputs[index];
            const auto region = REGIONS[static_cast<std::size_t>(input.release)];
            const auto workspace = (*stage)->path / "candidates" / region;
            const auto candidate = DefaultAssets(workspace);
            if (auto failure = CurateAssetPacks(candidate)) {
                return failure;
            }
            const auto candidateBoss = candidate.parent_path() / BOSS_PACK;
            const auto boss = defaults.parent_path() / BOSS_PACK;
            if (std::filesystem::is_directory(candidateBoss)) {
                std::filesystem::create_directories(boss);
                std::filesystem::copy(candidateBoss,
                                      boss,
                                      std::filesystem::copy_options::recursive |
                                          std::filesystem::copy_options::overwrite_existing);
            }
            const auto fonts = (*stage)->path / "fonts";
            NormalizeCartridge(candidate, region, fonts);
            if (index == 0) {
                std::filesystem::create_directories(defaults.parent_path());
                std::filesystem::rename(candidate, defaults);
                std::filesystem::copy(fonts / (region == "asia" ? "usa" : region),
                                      fonts / "base",
                                      std::filesystem::copy_options::recursive);
            } else {
                const auto strings = candidate / "strings";
                if (std::filesystem::is_directory(strings)) {
                    std::filesystem::create_directories(defaults / "strings");
                    std::filesystem::copy(strings,
                                          defaults / "strings",
                                          std::filesystem::copy_options::recursive |
                                              std::filesystem::copy_options::skip_existing);
                    std::filesystem::remove_all(strings);
                }
                RemoveUnchangedFiles(candidate, defaults);
                std::filesystem::rename(candidate, defaults.parent_path() / region);
            }
            done += 1.0;
        }
        std::filesystem::rename((*stage)->path / "fonts", defaults / "fonts");
        for (const auto &input : inputs) {
            if (!RecordImport(defaults, SOURCES[static_cast<std::size_t>(input.release)])) {
                return "Cannot record the imported ROM version.";
            }
        }
        if (!RecordImport(defaults, ImportSource::Disc)) {
            return "Cannot record the imported disc version.";
        }
        if (!report("Preparing the asset packs", done / total)) {
            return "Import cancelled.";
        }
        if (auto failure = WriteMetadata(defaults, inputs.front().release)) {
            return failure;
        }
        for (const auto &entry :
             std::filesystem::recursive_directory_iterator(defaults.parent_path())) {
            const auto filename = entry.path().filename();
            if (filename == "import.ini.lock" || filename == "cartridge.ini.lock") {
                std::filesystem::remove(entry.path());
            }
        }
        auto storage = DeduplicateAssetFiles(defaults);
        if (!storage) {
            return storage.error();
        }
        if (!RecordImportFiles(defaults) || !DefaultPackUsable(defaults)) {
            return "Staged asset inventory is invalid.";
        }
        if (!report("Installing the asset packs", 1.0)) {
            return "Import cancelled.";
        }
        if (auto failure = (*stage)->Install(data)) {
            return failure;
        }
        if (sources) {
            discSources.insert(discSources.end(), roms.begin(), roms.end());
            *sources = std::move(discSources);
        }
        return std::nullopt;
    } catch (const std::filesystem::filesystem_error &error) {
        return "Cannot prepare regional asset packs. " + std::string(error.what());
    }
}

std::optional<std::string> WriteImportDependencies(const std::filesystem::path &file,
                                                   const std::filesystem::path &target,
                                                   std::span<const std::filesystem::path> sources) {
    auto escape = [](const std::filesystem::path &path) -> std::optional<std::string> {
        if (path.empty()) {
            return std::nullopt;
        }
        std::error_code error;
        const auto absolute =
            std::filesystem::absolute(path, error).lexically_normal().generic_u8string();
        if (error) {
            return std::nullopt;
        }
        std::string escaped;
        for (const auto character : absolute) {
            if (character == u8'\n' || character == u8'\r' || character == u8'\t'
#ifndef _WIN32
                || character == u8'\\'
#endif
            ) {
                return std::nullopt;
            }
            if (character == u8' ' || character == u8'#') {
                escaped += '\\';
            } else if (character == u8'$') {
                escaped += '$';
            }
            escaped += static_cast<char>(character);
        }
        return escaped;
    };
    const auto output = escape(target);
    if (!output) {
        return "Cannot represent the import target in a dependency file.";
    }
    std::set<std::string> dependencies;
    for (const auto &source : sources) {
        auto dependency = escape(source);
        if (!dependency) {
            return "Cannot represent an import source in a dependency file.";
        }
        dependencies.insert(std::move(*dependency));
    }
    std::string text = *output + ':';
    for (const auto &dependency : dependencies) {
        text += ' ';
        text += dependency;
    }
    text += '\n';
    if (!WriteSaveFile(file, text)) {
        return "Cannot write the import dependency file.";
    }
    return std::nullopt;
}

} // namespace coastline
