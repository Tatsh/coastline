#include <array>
#include <cassert>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "assetimport.h"
#include "assetpack.h"
#include "importversion.h"
#include "paths.h"

namespace {
using namespace coastline;

std::vector<RomRelease> converted;
std::optional<RomRelease> failConversion;
constexpr std::array<const char *, 3> REGIONS{"usa", "jpn", "eur"};

void Write(const std::filesystem::path &path, const std::string &text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
    assert(file.good());
}

std::map<std::filesystem::path, std::string> Snapshot(const std::filesystem::path &directory) {
    std::map<std::filesystem::path, std::string> result;
    for (const auto &entry : std::filesystem::recursive_directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            std::ifstream file(entry.path(), std::ios::binary);
            result[entry.path().lexically_relative(directory)] =
                std::string(std::istreambuf_iterator<char>(file), {});
        }
    }
    return result;
}

void CheckNoStagingDirectory(const std::filesystem::path &data) {
    for (const auto &entry : std::filesystem::directory_iterator(data)) {
        assert(!entry.path().filename().string().starts_with(".coastline-import-"));
    }
}
} // namespace

namespace coastline {

std::expected<RomRelease, std::string> IdentifyRomFile(const std::filesystem::path &path) {
    for (std::size_t index = 0; index < REGIONS.size(); ++index) {
        if (path.stem() == REGIONS[index]) {
            return static_cast<RomRelease>(index);
        }
    }
    return std::unexpected("Invalid ROM image.");
}

std::optional<std::string> CheckDisc(const std::filesystem::path &disc) {
    return disc == "bad.iso" ? std::optional<std::string>("Invalid disc image.") : std::nullopt;
}

std::expected<RomRelease, std::string> ImportRom(const std::filesystem::path &path,
                                                 const std::filesystem::path &defaults,
                                                 const ImportProgress &progress) {
    auto release = IdentifyRomFile(path);
    assert(release);
    converted.push_back(*release);
    if (failConversion == *release) {
        return std::unexpected("Conversion failed.");
    }
    if (progress && !progress("Converting the ROM image", 0.5)) {
        return std::unexpected("Cancelled.");
    }
    const auto name = REGIONS[static_cast<std::size_t>(*release)];
    Write(defaults / "n64" / name / "segment.bin", "segment " + std::string(name));
    Write(defaults / "n64" / name / "fonts" / "fonts.json", "font " + std::string(name));
    Write(defaults / "n64" / name / "common.dat", "common cartridge");
    Write(defaults / "n64" / name / "regional.dat", "regional " + std::string(name));
    if (!std::filesystem::exists(defaults / "music" / "menu.xm")) {
        Write(defaults / "music" / "menu.xm", name);
    }
    return release;
}

std::expected<std::size_t, std::string> ImportDisc(const std::filesystem::path &,
                                                   const std::filesystem::path &defaults,
                                                   const ImportProgress &progress) {
    if (progress && !progress("Converting the disc", 0.5)) {
        return std::unexpected("Cancelled.");
    }
    Write(defaults / "cars" / "model.dat", "disc");
    return 1;
}

std::optional<std::string> CurateAssetPacks(const std::filesystem::path &defaults) {
    Write(defaults / "pack.json", "{\"version\":1}");
    Write(defaults.parent_path() / "boss" / "original.png", "unbranded");
    return std::nullopt;
}

} // namespace coastline

int main() {
    using namespace coastline;
    const auto data = std::filesystem::temp_directory_path() / "coastline-assetimport-test";
    std::filesystem::remove_all(data);
    Write(data / "assets" / "default" / "old.txt", "old default");
    Write(data / "assets" / "boss" / "old.txt", "old PC");
    Write(data / "assets" / "eur" / "old.txt", "old EUR");
    Write(data / "assets" / "jpn" / "old.txt", "old JPN");
    Write(data / "assets" / "custom" / "model.glb", "custom model");
    const auto original = Snapshot(data);
    const std::array<std::filesystem::path, 2> roms{"jpn.rom", "usa.rom"};
    auto failure = ImportAssetImages(data, {}, roms);
    assert(failure && converted.empty() && Snapshot(data) == original);
    failure = ImportAssetImages(data, "disc.iso", {});
    assert(failure && converted.empty() && Snapshot(data) == original);
    failure = ImportAssetImages(data, "bad.iso", roms);
    assert(failure && converted.empty() && Snapshot(data) == original);
    const std::array<std::filesystem::path, 1> invalid{"bad.rom"};
    failure = ImportAssetImages(data, "disc.iso", invalid);
    assert(failure && converted.empty() && Snapshot(data) == original);
    const std::array<std::filesystem::path, 2> duplicate{"usa.rom", "usa.rom"};
    failure = ImportAssetImages(data, "disc.iso", duplicate);
    assert(failure && converted.empty() && Snapshot(data) == original);

    failConversion = RomRelease::Jpn;
    failure = ImportAssetImages(data, "disc.iso", roms);
    assert(failure && Snapshot(data) == original);
    assert((converted == std::vector{RomRelease::Usa, RomRelease::Jpn}));
    CheckNoStagingDirectory(data);
    failConversion.reset();
    converted.clear();
    failure = ImportAssetImages(data, "disc.iso", roms, [](const std::string &step, double) {
        return step != "Installing the asset packs";
    });
    assert(failure && Snapshot(data) == original);
    CheckNoStagingDirectory(data);

    failure = ImportAssetImages(data, "disc.iso", roms, [&](const std::string &step, double) {
        if (step == "Installing the asset packs") {
            for (const auto &entry : std::filesystem::directory_iterator(data)) {
                if (entry.path().filename().string().starts_with(".coastline-import-")) {
                    std::filesystem::remove_all(entry.path() / "assets" / "boss");
                }
            }
        }
        return true;
    });
    assert(failure && Snapshot(data) == original);
    CheckNoStagingDirectory(data);

    converted.clear();
    double previous = 0.0;
    failure = ImportAssetImages(data, "disc.iso", roms, [&](const std::string &, double fraction) {
        assert(fraction >= previous && fraction <= 1.0);
        previous = fraction;
        return true;
    });
    assert(!failure && previous == 1.0);
    assert((converted == std::vector{RomRelease::Usa, RomRelease::Jpn}));
    assert(DefaultPackUsable(DefaultAssets(data)));
    auto files = Snapshot(data);
    assert(files.at("assets/custom/model.glb") == "custom model");
    assert(files.at("assets/default/music/menu.xm") == "usa");
    assert(!std::filesystem::exists(data / "assets" / "eur"));
    assert(std::filesystem::is_directory(data / "assets" / "jpn"));
    assert(files.at("assets/default/n64/base/segment.bin") == "segment usa");
    assert(files.at("assets/jpn/n64/base/segment.bin") == "segment jpn");
    assert(!std::filesystem::exists(data / "assets/jpn/n64/base/common.dat"));
    assert(!std::filesystem::exists(data / "assets/jpn/cars/model.dat"));
    assert(!std::filesystem::exists(data / "assets/default/n64/usa"));
    assert(!std::filesystem::exists(data / "assets/default/n64/jpn"));
    std::ifstream metadata(DefaultAssets(data) / "pack.json");
    assert(nlohmann::json::parse(metadata).at("baseRegion") == "usa");
    CheckNoStagingDirectory(data);

    const std::array<std::filesystem::path, 2> nonUsa{"jpn.rom", "eur.rom"};
    converted.clear();
    failure = ImportAssetImages(data, "disc.iso", nonUsa);
    assert(!failure && (converted == std::vector{RomRelease::Jpn, RomRelease::Eur}));
    files = Snapshot(data);
    assert(files.at("assets/default/music/menu.xm") == "jpn");
    assert(!std::filesystem::exists(DefaultAssets(data) / "n64" / "usa"));
    assert(files.at("assets/default/n64/base/segment.bin") == "segment jpn");
    assert(files.at("assets/eur/n64/base/segment.bin") == "segment eur");
    assert(!std::filesystem::exists(data / "assets" / "jpn"));
    assert(files.at("assets/custom/model.glb") == "custom model");
    assert(DefaultPackUsable(DefaultAssets(data)));
    CheckNoStagingDirectory(data);
    std::filesystem::remove_all(data);
}
