// Host check of the XDG directories, the layers of the asset packs, the record of the versions of
// the import, the BossRally.ini reader, the icon file reader, the volume levels, and the reader of
// the resident memory of the SDL game.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "config.h"
#include "consolestrings.h"
#include "controller.h"
#include "iconfile.h"
#include "importversion.h"
#include "language.h"
#include "paths.h"
#include "performance.h"
#include "runscript.h"
#include "trackcatalog.h"
#include "volumelevels.h"

namespace {

int g_failures = 0;

void Check(bool condition, const char *description) {
    if (!condition) {
        std::cerr << "FAIL: " << description << '\n';
        ++g_failures;
    }
}

coastline::EnvironmentReader Environment(std::map<std::string, std::string> values) {
    return [values](const char *name) -> std::optional<std::string> {
        auto found = values.find(name);
        if (found == values.end() || found->second.empty()) {
            return std::nullopt;
        }
        return found->second;
    };
}

void Put16(std::vector<std::uint8_t> &bytes, std::size_t at, std::uint32_t value) {
    bytes[at] = static_cast<std::uint8_t>(value);
    bytes[at + 1] = static_cast<std::uint8_t>(value >> 8);
}

void Put32(std::vector<std::uint8_t> &bytes, std::size_t at, std::uint32_t value) {
    Put16(bytes, at, value);
    Put16(bytes, at + 2, value >> 16);
}

// A 2x2 icon of 4 bits a pixel: the colours of indices 2 and 1 on the top row and 1 and 2 on the
// bottom, where the AND mask makes the second pixel transparent.
std::vector<std::uint8_t> IconBitmap() {
    std::vector<std::uint8_t> dib(120);
    Put32(dib, 0, 40);
    Put32(dib, 4, 2);
    Put32(dib, 8, 4);
    Put16(dib, 12, 1);
    Put16(dib, 14, 4);
    Put32(dib, 40 + 4, 0x00102030);
    Put32(dib, 40 + 8, 0x00FF0080);
    dib[104] = 0x12;
    dib[108] = 0x21;
    dib[112] = 0x40;
    return dib;
}

// An icon file of the bitmap whose entry records 0 planes and 0 bits per pixel, as Boss.ico does.
std::vector<std::uint8_t> IconFile(const std::vector<std::uint8_t> &dib) {
    std::vector<std::uint8_t> icon(22);
    Put16(icon, 2, 1);
    Put16(icon, 4, 1);
    icon[6] = 2;
    icon[7] = 2;
    icon[8] = 16;
    Put32(icon, 14, static_cast<std::uint32_t>(dib.size()));
    Put32(icon, 18, 22);
    icon.insert(icon.end(), dib.begin(), dib.end());
    return icon;
}

void CheckVolumeLevels() {
    using coastline::MAX_RACE_VOLUME_LEVEL;
    using coastline::MAX_VOLUME_LEVEL;
    using coastline::MenuVolumeLevel;
    using coastline::RaceVolumeLevel;
    Check(RaceVolumeLevel(0) == 0 && RaceVolumeLevel(MAX_VOLUME_LEVEL) == MAX_RACE_VOLUME_LEVEL,
          "silence and the loudest level map to the race engine");
    for (int level = 0; level <= MAX_VOLUME_LEVEL; ++level) {
        Check(MenuVolumeLevel(RaceVolumeLevel(level), level) == level,
              "an unchanged race level retains the level of the options");
    }
    for (int raceLevel = 0; raceLevel <= MAX_RACE_VOLUME_LEVEL; ++raceLevel) {
        for (int level = 0; level <= MAX_VOLUME_LEVEL; ++level) {
            int back = MenuVolumeLevel(raceLevel, level);
            Check(back >= 0 && back <= MAX_VOLUME_LEVEL && RaceVolumeLevel(back) == raceLevel,
                  "a changed race level maps to a level of the options that gives it");
        }
    }
    Check(MenuVolumeLevel(RaceVolumeLevel(MAX_VOLUME_LEVEL) - 1, MAX_VOLUME_LEVEL) == 9,
          "a step down in the pause menu is a step down in the options");
    Check(MenuVolumeLevel(5, 5) == 5 && MenuVolumeLevel(5, 6) == 6,
          "both levels of the options that give the same race level are retained");
    Check(MenuVolumeLevel(MAX_RACE_VOLUME_LEVEL + 3, 0) == MAX_VOLUME_LEVEL,
          "a race level past the loudest is clamped");
}

void CheckIconFile() {
    auto dib = IconBitmap();
    auto iconFile = IconFile(dib);
    auto entries = coastline::ListIconImages(iconFile);
    Check(entries.size() == 1 && entries[0].bitCount == 4 &&
              std::equal(dib.begin(), dib.end(), entries[0].image.begin(), entries[0].image.end()),
          "the icon file lists its image");
    auto pixels = coastline::DecodeIconBitmap(dib);
    const std::vector<std::uint8_t> rgba{0xFF,
                                         0x00,
                                         0x80,
                                         0xFF,
                                         0x10,
                                         0x20,
                                         0x30,
                                         0xFF,
                                         0x10,
                                         0x20,
                                         0x30,
                                         0xFF,
                                         0xFF,
                                         0x00,
                                         0x80,
                                         0x00};
    Check(pixels && pixels->width == 2 && pixels->height == 2 && pixels->rgba == rgba,
          "the bitmap decodes from the bottom row with the AND mask as alpha");
    iconFile.resize(iconFile.size() - 1);
    Check(coastline::ListIconImages(iconFile).empty(), "an image cut short gives no images");
    Check(!coastline::DecodeIconBitmap(std::span(dib).first(110)), "a mask cut short is refused");
    Check(coastline::ParseIconName("32x32x4.png") == coastline::IconSize{32, 32, 4},
          "an icon name gives its size");
    Check(coastline::IconName({256, 256, 32}) == "256x256x32.png", "a size gives its icon name");
    Check(!coastline::ParseIconName("coastline.ico") && !coastline::ParseIconName("32x32.png"),
          "other names give no size");
}

void WriteText(const std::filesystem::path &path, const std::string &text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path) << text;
}

std::string ReadText(const std::filesystem::path &path) {
    std::ifstream in(path);
    return std::string(std::istreambuf_iterator<char>(in), {});
}

void CheckImportVersions() {
    using coastline::ImportSource;
    auto defaults = std::filesystem::temp_directory_path() / "coastline-sdl-test-imports";
    std::filesystem::remove_all(defaults);
    Check(coastline::StaleImports(defaults).empty(), "an empty pack has no old source");
    std::filesystem::create_directories(defaults / "cars");
    std::filesystem::create_directories(defaults / "n64" / "usa");
    std::filesystem::create_directories(defaults / "n64" / "eur");
    Check(coastline::StaleImports(defaults) ==
              std::vector{ImportSource::Disc, ImportSource::Usa, ImportSource::Eur},
          "every source imported without a record is old");
    Check(coastline::RecordImport(defaults, ImportSource::Usa), "the record is written");
    Check(coastline::StaleImports(defaults) == std::vector{ImportSource::Disc, ImportSource::Eur},
          "a recorded source is current");
    std::string cartridge = std::to_string(coastline::CARTRIDGE_CONVERTER_VERSION);
    WriteText(defaults / "import.ini",
              "[Import]\nDisc=0\nUsa=" + cartridge + "\nEur=" + cartridge + "\n");
    Check(coastline::StaleImports(defaults) == std::vector{ImportSource::Disc},
          "an older version is old");
    WriteText(defaults / "import.ini", "[Import]\nDisc=3\nUsa=4\nEur=" + cartridge + "\n");
    Check(coastline::StaleImports(defaults) == std::vector{ImportSource::Disc, ImportSource::Usa},
          "pre-release versions require a new import");
    Check(coastline::RecordImport(defaults, ImportSource::Usa), "the cartridge record is updated");
    Check(coastline::RecordImport(defaults, ImportSource::Disc), "the record is updated");
    Check(coastline::StaleImports(defaults).empty(), "every source is current");
    Check(!coastline::DefaultPackUsable(defaults), "version records alone are insufficient");
    Check(coastline::GameConfig::Load(defaults / "import.ini").GetInteger("Import", "Eur", 0) ==
              coastline::CARTRIDGE_CONVERTER_VERSION,
          "an update retains the other sources");
    std::array disc{ImportSource::Disc};
    Check(coastline::ReimportMessage(disc) == "Import the disc again.", "one source");
    std::array two{ImportSource::Disc, ImportSource::Jpn};
    Check(coastline::ReimportMessage(two) == "Import the disc and the Japanese cartridge again.",
          "two sources");
    std::array three{ImportSource::Usa, ImportSource::Jpn, ImportSource::Eur};
    Check(coastline::ReimportMessage(three) == "Import the USA cartridge, the Japanese cartridge, "
                                               "and the European cartridge again.",
          "three sources");
    std::filesystem::remove_all(defaults);
}

void CheckImportInventory() {
    const auto root = std::filesystem::temp_directory_path() / "coastline-import-inventory";
    const auto defaults = root / "default";
    std::filesystem::remove_all(root);
    WriteText(defaults / "cars" / "model.gltf", "model");
    for (const auto *file : {"BossRally/pod.json",
                             "RallyIntro1.json",
                             "RallyIntro2.json",
                             "RallyCredits.json",
                             "RallyOutro.json"}) {
        WriteText(defaults / file, "{}");
    }
    WriteText(defaults / "n64" / "base" / "segment.bin", "segment");
    WriteText(defaults / "n64/base/cartridge.ini", "[Cartridge]\nRegion=eur\n");
    WriteText(defaults / "import.ini", "[Import]\nLayout=1\nBaseRegion=eur\n");
    Check(coastline::RecordImport(defaults, coastline::ImportSource::Disc), "record the disc");
    Check(coastline::RecordImport(defaults, coastline::ImportSource::Eur), "record Europe");
    Check(!coastline::DefaultPackUsable(defaults), "an unfinished import must be refused");
    Check(coastline::RecordImportFiles(defaults), "record imported files");
    Check(coastline::DefaultPackUsable(defaults), "a complete import must be usable");
    const auto inventory = ReadText(defaults / "import-files.txt");
    WriteText(defaults / "import-files.txt", inventory.substr(0, inventory.size() - 5));
    Check(!coastline::DefaultPackUsable(defaults), "a truncated inventory must be refused");
    auto modified = inventory;
    modified[modified.find('\n') + 1] = '9';
    WriteText(defaults / "import-files.txt", modified);
    Check(!coastline::DefaultPackUsable(defaults), "an altered inventory must be refused");
    modified = inventory;
    modified[modified.find(' ') + 1] = '9';
    WriteText(defaults / "import-files.txt", modified);
    Check(!coastline::DefaultPackUsable(defaults), "an incorrect file count must be refused");
    WriteText(defaults / "import-files.txt", inventory);
    const auto originalRecord = ReadText(defaults / "import.ini");
    auto record = coastline::GameConfig::Load(defaults / "import.ini");
    record.Set("Import", "Layout", "0");
    record.Save(defaults / "import.ini");
    coastline::RecordImportFiles(defaults);
    Check(!coastline::DefaultPackUsable(defaults), "legacy layouts require a new import");
    record.Set("Import", "Layout", "1");
    record.Set("Import", "Jpn", "2");
    record.Save(defaults / "import.ini");
    coastline::RecordImportFiles(defaults);
    Check(!coastline::DefaultPackUsable(defaults), "every recorded converter must be current");
    record.Set("Import", "Jpn", "1");
    record.Save(defaults / "import.ini");
    coastline::RecordImportFiles(defaults);
    Check(!coastline::DefaultPackUsable(defaults), "a recorded region needs its difference pack");
    WriteText(defaults / "import.ini", originalRecord);
    coastline::RecordImportFiles(defaults);
    Check(coastline::DefaultPackUsable(defaults), "restored metadata is usable");
    std::filesystem::remove(defaults / "RallyIntro1.json");
    coastline::RecordImportFiles(defaults);
    Check(!coastline::DefaultPackUsable(defaults),
          "a complete inventory without a required converted recording must be refused");
    WriteText(defaults / "RallyIntro1.json", "{}");
    coastline::RecordImportFiles(defaults);
    WriteText(defaults / "cars" / "model.gltf", "truncated");
    Check(!coastline::DefaultPackUsable(defaults), "a changed file size must be refused");
    WriteText(defaults / "cars" / "model.gltf", "model");
    std::filesystem::remove(defaults / "cars" / "model.gltf");
    Check(!coastline::DefaultPackUsable(defaults), "a missing imported file must be refused");
    WriteText(defaults / "import-files.txt", "1 \"../outside\"\n0 \"\"\n");
    Check(!coastline::DefaultPackUsable(defaults), "inventory paths must stay inside assets");
    WriteText(defaults / "import-files.txt", "0 \"\"\n");
    Check(!coastline::DefaultPackUsable(defaults), "an empty inventory must be refused");
    std::filesystem::remove_all(root);
}

void CheckPackAssets() {
    auto root = std::filesystem::temp_directory_path() / "coastline-sdl-test-packs";
    std::filesystem::remove_all(root);
    auto data = root / "data";
    auto cache = root / "cache";
    auto defaults = coastline::DefaultAssets(data);
    WriteText(defaults / "tracks" / "a.png", "default a");
    WriteText(defaults / "tracks" / "b.png", "default b");
    WriteText(defaults / "music" / "menu.xm", "menu");
    WriteText(data / "assets" / "boss" / "tracks" / "a.png", "boss a");
    WriteText(data / "assets" / "boss" / "extra.txt", "extra");

    Check(coastline::PackAssets(data, cache, "default") == defaults,
          "the default pack is read in place");
    Check(!coastline::PackAssets(data, cache, "missing"), "a missing pack gives nothing");
    auto boss = coastline::PackAssets(data, cache, "boss");
    Check(boss && ReadText(*boss / "tracks" / "a.png") == "boss a", "a file of the pack replaces");
    Check(boss && ReadText(*boss / "tracks" / "b.png") == "default b",
          "a directory of both packs is layered");
    Check(boss && ReadText(*boss / "music" / "menu.xm") == "menu",
          "a directory of the default pack alone is read");
    Check(boss && ReadText(*boss / "extra.txt") == "extra", "a file of the pack alone is read");
    std::filesystem::remove(data / "assets" / "boss" / "extra.txt");
    boss = coastline::PackAssets(data, cache, "boss");
    Check(boss && !std::filesystem::exists(*boss / "extra.txt"), "the layers are made again");
    Check(ReadText(defaults / "tracks" / "a.png") == "default a",
          "the default pack is not written");

    // A file of the cache alone shows whether a call reuses the layers or makes them again.
    auto marker = *boss / "marker";
    WriteText(marker, "");
    Check(coastline::PackAssets(data, cache, "boss") == boss && std::filesystem::exists(marker),
          "unchanged packs reuse the layers");
    WriteText(defaults / "music" / "race.xm", "race");
    boss = coastline::PackAssets(data, cache, "boss");
    Check(boss && !std::filesystem::exists(marker) &&
              ReadText(*boss / "music" / "race.xm") == "race",
          "a file added to the default pack makes the layers again");
    WriteText(marker, "");
    auto bossFile = data / "assets" / "boss" / "tracks" / "a.png";
    std::filesystem::last_write_time(
        bossFile, std::filesystem::last_write_time(bossFile) + std::chrono::hours(1));
    coastline::PackAssets(data, cache, "boss");
    Check(!std::filesystem::exists(marker), "a file of the pack written makes the layers again");
    WriteText(marker, "");
    std::filesystem::remove(*boss / ".layer-stamp");
    coastline::PackAssets(data, cache, "boss");
    Check(!std::filesystem::exists(marker), "a missing stamp makes the layers again");
    WriteText(defaults / "cars" / "model.gltf", "default glTF");
    WriteText(defaults / "cars" / "other.glb", "default GLB");
    WriteText(data / "assets" / "boss" / "cars" / "model.glb", "pack GLB");
    WriteText(data / "assets" / "boss" / "cars" / "other.gltf", "pack glTF");
    boss = coastline::PackAssets(data, cache, "boss");
    Check(boss && ReadText(*boss / "cars" / "model.glb") == "pack GLB" &&
              !std::filesystem::exists(*boss / "cars" / "model.gltf"),
          "a GLB replacement suppresses the inherited glTF model");
    Check(boss && ReadText(*boss / "cars" / "other.gltf") == "pack glTF" &&
              !std::filesystem::exists(*boss / "cars" / "other.glb"),
          "a glTF replacement suppresses the inherited GLB model");
    WriteText(data / "assets" / "boss" / "cars" / "model.gltf", "pack glTF");
    boss = coastline::PackAssets(data, cache, "boss");
    Check(boss && ReadText(*boss / "cars" / "model.gltf") == "pack glTF" &&
              ReadText(*boss / "cars" / "model.glb") == "pack GLB",
          "both formats in the selected pack remain available for glTF precedence");
    const std::filesystem::path unicodePack(u8"日本");
    WriteText(data / "assets" / unicodePack / "cars/model.glb", "Unicode GLB");
    auto unicode = coastline::PackAssets(data, cache, "日本");
    Check(coastline::PackDirectory(data, "日本") == data / "assets" / unicodePack &&
              unicode == cache / "assets" / unicodePack &&
              ReadText(*unicode / "cars/model.glb") == "Unicode GLB",
          "UTF-8 pack identifiers select native paths and cache directories");
    WriteText(data / "assets/jpn/tracks/a.png", "Japanese a");
    WriteText(data / "assets/jpn/tracks/b.png", "Japanese b");
    WriteText(data / "assets/jpn/cars/model.glb", "Japanese GLB");
    auto japanese = coastline::PackAssets(data, cache, "default", {}, "jpn");
    Check(japanese && ReadText(*japanese / "tracks/a.png") == "Japanese a" &&
              ReadText(*japanese / "music/menu.xm") == "menu",
          "regional assets override default assets and fall back directly to default");
    Check(japanese && !std::filesystem::exists(*japanese / "cars/model.gltf") &&
              ReadText(*japanese / "cars/model.glb") == "Japanese GLB",
          "a regional GLB suppresses the default glTF");
    auto japaneseBoss = coastline::PackAssets(data, cache, "boss", {}, "jpn");
    Check(japaneseBoss && ReadText(*japaneseBoss / "tracks/a.png") == "boss a" &&
              ReadText(*japaneseBoss / "tracks/b.png") == "Japanese b" &&
              ReadText(*japaneseBoss / "cars/model.gltf") == "pack glTF",
          "selected pack overrides the region before falling back to default");
    WriteText(data / "assets/jpn/tracks/b.png", "revised Japanese b");
    japaneseBoss = coastline::PackAssets(data, cache, "boss", {}, "jpn");
    Check(japaneseBoss && ReadText(*japaneseBoss / "tracks/b.png") == "revised Japanese b",
          "regional changes invalidate the final pack cache");
    boss = coastline::PackAssets(data, cache, "boss");
    Check(boss && ReadText(*boss / "tracks/b.png") == "default b",
          "switching to the base region does not inherit a previous region");
    Check(!coastline::PackAssets(data, cache, "default", {}, "../jpn") &&
              !coastline::PackAssets(data, cache, "default", {}, "eur"),
          "unsafe and unavailable regions are refused");
    const auto user = root / "user";
    WriteText(user / "assets/eur/tracks/a.png", "user European a");
    auto european = coastline::PackAssets(data, cache, "default", user, "eur");
    Check(european && ReadText(*european / "tracks/a.png") == "user European a" &&
              ReadText(*european / "tracks/b.png") == "default b",
          "regional packs fall back to the user root above system defaults");
    std::filesystem::remove_all(root);
}

void CheckLanguage() {
    auto strings = coastline::MergeStrings({{"1", "Paused"}, {"2", "Quit"}},
                                           {{"2", "やめる"}, {"n64/802a90a8", "振動パック"}});
    Check(strings.at("1") == "Paused", "a key the language lacks takes the en-us string");
    Check(strings.at("2") == "やめる", "a key of the language replaces the en-us string");
    Check(strings.at("n64/802a90a8") == "振動パック", "a key of the language alone is read");
    Check(strings.size() == 3, "the strings are the keys of both");

    auto assets = std::filesystem::temp_directory_path() / "coastline-sdl-test-language";
    std::filesystem::remove_all(assets);
    Check(coastline::ReleaseDirectory(assets) == assets / "n64" / "base",
          "cartridge assets use a stable path independent of language");
    Check(coastline::CartridgeRegion(coastline::ReleaseDirectory(assets)).empty(),
          "missing region metadata does not provide a region");
    WriteText(assets / "n64/base/cartridge.ini", "[Cartridge]\nRegion=jpn\n");
    Check(coastline::CartridgeRegion(coastline::ReleaseDirectory(assets)) == "jpn",
          "cartridge identity comes from converted metadata");
    WriteText(assets / "fonts/base/fonts.json", "base fonts");
    WriteText(assets / "fonts/jpn/fonts.json", "Japanese fonts");
    Check(coastline::FontDirectory(assets, "ja") == assets / "fonts/jpn",
          "Japanese text selects Japanese fonts");
    Check(coastline::FontDirectory(assets, "en-us") == assets / "fonts/base",
          "missing fonts fall back to base fonts");
    WriteText(assets / "fonts/usa/fonts.json", "USA fonts");
    Check(coastline::FontDirectory(assets, "en-us") == assets / "fonts/usa" &&
              coastline::CartridgeRegion(coastline::ReleaseDirectory(assets)) == "jpn",
          "English fonts do not change the Japanese cartridge");
    const std::array roots{assets / "system", assets / "user"};
    WriteText(roots[0] / "assets/default/n64/base/cartridge.ini", "[Cartridge]\nRegion=usa\n");
    WriteText(roots[1] / "assets/jpn/n64/base/cartridge.ini", "[Cartridge]\nRegion=jpn\n");
    WriteText(roots[1] / "assets/eur/n64/base/cartridge.ini", "[Cartridge]\nRegion=jpn\n");
    WriteText(roots[1] / "assets/custom/n64/base/cartridge.ini", "[Cartridge]\nRegion=eur\n");
    const auto regions = coastline::AvailableCartridgeRegions(roots);
    Check(regions == std::vector<std::string>{"jpn", "usa"},
          "region discovery spans roots and rejects mismatched and custom pack metadata");
    Check(coastline::TitleScreenRegion("regional", "usa", regions) == "usa" &&
              coastline::TitleScreenRegion("regional", "jpn", regions) == "jpn",
          "the default title follows the selected converted region");
    Check(coastline::TitleScreenRegion("japanese", "usa", regions) == "jpn" &&
              coastline::TitleScreenRegion("international", "jpn", regions) == "usa",
          "title choices can differ from the selected converted region");
    Check(coastline::TitleScreenRegion("international", "eur", regions) == "eur",
          "an active European release retains its international title");
    const std::vector<std::string> europeanOnly{"eur"};
    Check(coastline::TitleScreenRegion("international", "jpn", europeanOnly) == "eur",
          "European assets can supply the international title without USA assets");
    Check(!coastline::TitleScreenRegion("japanese", "usa", europeanOnly) &&
              !coastline::TitleScreenRegion("international", "jpn", {}) &&
              !coastline::TitleScreenRegion("ja", "usa", regions) &&
              !coastline::TitleScreenRegion("../jpn", "usa", regions),
          "unavailable and invalid title choices are rejected");
    std::filesystem::remove_all(assets);
}

void CheckConsoleStrings() {
    std::set<std::string_view> keys;
    for (const auto &message : coastline::CONSOLE_STRINGS) {
        Check(keys.insert(message.key).second, "each key of the console strings is unique");
        Check(message.key.starts_with("n64.") || message.key.starts_with("race."),
              "each key of the console strings starts with n64. or race.");
        Check(!message.usa.empty() || !message.eur.empty() || !message.jpn.empty(),
              "each console string has the lines of a release");
        for (const auto *lines : {&message.usa, &message.eur, &message.jpn}) {
            Check(std::ranges::all_of(*lines,
                                      [](std::uint32_t address) {
                                          return address >= 0x80200000 && address < 0x802AE0F0 &&
                                                 address % 4 == 0;
                                      }),
                  "each line of a console string is a word of a main segment");
        }
    }
    for (const auto &message : coastline::SAVE_FOLDER_STRINGS) {
        Check(keys.insert(message.key).second, "each key of the save folder strings is unique");
        Check(message.key.starts_with("sdl."),
              "each key of the save folder strings starts with sdl.");
        Check(!message.text.empty() && !message.text.ends_with('\n'),
              "each save folder string has text and no newline at its end");
    }
    // Every number of BRString.dll has a key of its own. A race. key is the key of a message of
    // the console too, and a pc. key the PC release's alone.
    std::set<std::string_view> libraryKeys;
    for (auto key : coastline::LIBRARY_STRING_KEYS) {
        Check(!key.empty(), "each string of the library has a key");
        Check(libraryKeys.insert(key).second, "each key of the library strings is unique");
        if (key.starts_with("race.")) {
            Check(keys.contains(key), "each race. key of the library is a key of the console");
        } else {
            Check(key.starts_with("pc.") && !keys.contains(key),
                  "each other key of the library starts with pc. and is the library's alone");
        }
    }
    for (auto key : keys) {
        if (key.starts_with("race.")) {
            Check(libraryKeys.contains(key), "each race. key of the console is a library key");
        }
    }
}

void CheckPerformanceParsing() {
    Check(coastline::StatmResidentPages("53287 12345 4321 1 0 9876 0\n") == 12345u,
          "the second number of statm is the resident pages");
    Check(coastline::StatmResidentPages("  7 8") == 8u, "statm may open with spaces");
    Check(!coastline::StatmResidentPages("53287"), "statm without a second number gives nothing");
    Check(!coastline::StatmResidentPages(""), "an empty statm gives nothing");
    constexpr std::string_view SMAPS =
        "5600a000-5600b000 r-xp 00000000 08:01 42   /opt/coastline/bin/coastline\n"
        "Pss:                 100 kB\n"
        "Pss_Dirty:            90 kB\n"
        "5600c000-5600d000 rw-p 00000000 00:00 0    [heap]\n"
        "Pss:                1000 kB\n"
        "7f000000-7f001000 rw-p 00000000 00:00 0 \n"
        "Pss:                  20 kB\n"
        "7f001000-7f002000 rw-p 00000000 00:00 0    [anon:pool]\n"
        "Pss:                   3 kB\n"
        "7f002000-7f003000 r-xp 00000000 08:01 7    /usr/lib64/libdriver.so.615\n"
        "Pss:               13000 kB\n"
        "7f003000-7f004000 rw-s 00000000 00:06 9    /dev/gpu0\n"
        "Pss:               14000 kB\n"
        "7ffd0000-7ffd1000 rw-p 00000000 00:00 0    [stack]\n"
        "Pss:                   8 kB\n"
        "7ffd2000-7ffd3000 r-xp 00000000 00:00 0    [vdso]\n"
        "Pss:                   1 kB\n";
    Check(coastline::SmapsGamePssBytes(SMAPS, "/opt/coastline/bin/coastline") == 1131ull * 1024,
          "the game's memory counts the program, the heap, the stack, and anonymous memory");
    Check(coastline::SmapsGamePssBytes(SMAPS, "/elsewhere/coastline") == 1031ull * 1024,
          "a program at another path is a library");
    Check(!coastline::SmapsGamePssBytes("", "/opt/coastline/bin/coastline"),
          "an empty smaps gives nothing");
    Check(!coastline::SmapsGamePssBytes("5600c000-5600d000 rw-p 00000000 00:00 0 [heap]\nPss: x\n",
                                        "/opt/coastline/bin/coastline"),
          "a Pss line without a number gives nothing");
    Check(!coastline::StatmResidentPages("1 x"), "a second word that is not a number is nothing");
}

// The line and the start of the message of the error of a script.
bool ScriptFails(std::string_view text, int line, std::string_view message) {
    auto script = coastline::ParseScript(text);
    return !script && script.error().line == line && script.error().message.starts_with(message);
}

void CheckRunScript() {
    auto script = coastline::ParseScript("# a comment\n"
                                         "\n"
                                         "autopilot\r\n"
                                         "screenshot-every 60\n"
                                         "  120 press a\n"
                                         "200 press start 12 2   # FRAMES, then PORT\n"
                                         "300\tkey F9\n"
                                         "310 key Return 3\n"
                                         "400 type HELLO there\n"
                                         "450 type <backspace>\n"
                                         "500 mouse 100 80 left 4\n"
                                         "510 mouse 10 20\n"
                                         "600 screenshot\n"
                                         "900 quit\n"
                                         "800 quit");
    Check(script.has_value(), "a script of every command is read");
    if (!script) {
        return;
    }
    Check(script->autopilot, "autopilot applies from the start");
    Check(script->screenshotEvery == 60, "screenshot-every takes its number");
    Check(script->presses.size() == 2 && script->presses[0].frame == 120 &&
              script->presses[0].button == coastline::BUTTON_A &&
              script->presses[0].frames == coastline::SCRIPT_PRESS_FRAMES &&
              script->presses[0].port == 0,
          "a press lasts the default frames on the first controller");
    Check(script->presses[1].button == coastline::BUTTON_START && script->presses[1].frames == 12 &&
              script->presses[1].port == 1,
          "a press takes its frames and its controller");
    Check(script->keys.size() == 2 && script->keys[0].name == "F9" &&
              script->keys[0].frames == coastline::SCRIPT_PRESS_FRAMES &&
              script->keys[0].line == 7 && script->keys[1].frames == 3,
          "a key takes its name, its frames, and its line");
    Check(script->typing.size() == 2 && script->typing[0].text == "HELLO there" &&
              script->typing[1].text == "<backspace>",
          "type takes the rest of the line");
    Check(script->mouseMoves.size() == 2 && script->mouseMoves[0].x == 100 &&
              script->mouseMoves[0].y == 80 &&
              script->mouseMoves[0].button == coastline::ScriptMouseButton::Left &&
              script->mouseMoves[0].frames == 4 &&
              script->mouseMoves[1].button == coastline::ScriptMouseButton::None &&
              script->mouseMoves[1].frames == 1,
          "a move of the mouse takes its place, its button, and its frames");
    Check(script->screenshots == std::vector<int>{600}, "screenshot saves its frame");
    Check(script->quitFrame == 800, "the earliest quit applies");
    Check(coastline::ParseScript("").has_value(), "an empty script is read");

    Check(ScriptFails("autopilot\njump\n", 2, "jump is not a command"), "an unknown command");
    Check(ScriptFails("12x press a", 1, "12x is not a command"), "a frame with letters");
    Check(ScriptFails("0 press a", 1, "0 is not a frame"), "a frame of 0");
    Check(ScriptFails("10 press a six", 1, "six is not a number"), "a bad number of frames");
    Check(ScriptFails("10 press a 0", 1, "0 is not a number"), "a press of no frames");
    Check(ScriptFails("10 press a 6 5", 1, "5 is not a controller"), "a fifth controller");
    Check(ScriptFails("10 press x", 1, "x is not one of"), "an unknown button");
    Check(ScriptFails("10 press", 1, "press takes"), "a press without a button");
    Check(ScriptFails("10 key", 1, "key takes"), "a key without a name");
    Check(ScriptFails("10 type   # nothing", 1, "type takes"), "a typing without text");
    Check(ScriptFails("10 mouse 5", 1, "mouse takes"), "a move without a place");
    Check(ScriptFails("10 mouse 5 y", 1, "y is not a place"), "a move to a bad place");
    Check(ScriptFails("10 mouse 5 6 middle", 1, "middle is not one of"), "an unknown mouse button");
    Check(ScriptFails("press a", 1, "press needs a frame"), "a press without a frame");
    Check(ScriptFails("10 autopilot", 1, "autopilot applies"), "autopilot at a frame");
    Check(ScriptFails("screenshot-every", 1, "screenshot-every takes"), "screenshot-every alone");
    Check(ScriptFails("screenshot-every -1", 1, "-1 is not a number"), "a negative period");
    Check(ScriptFails("10 quit now", 1, "quit takes no arguments"), "quit with an argument");
    Check(ScriptFails("10", 1, "The frame has no command"), "a frame alone");
}

void CheckReset() {
    const auto root = std::filesystem::temp_directory_path() /
                      ("coastline-reset-" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const coastline::GamePaths paths{
        root / "config", root / "config/data", root / "state", root / "cache"};
    Check(coastline::CreateGamePaths(paths), "reset fixture directories are created");
    for (const auto *pack : {"default", "boss", "eur", "jpn"}) {
        WriteText(paths.data / "assets" / pack / "asset", "asset");
    }
    for (const auto *cache : {"assets", "asset-sources", "asset-search"}) {
        WriteText(paths.cache / cache / "stale", "cache");
    }
    const std::array preserved{paths.config / "settings",
                               paths.state / "save",
                               paths.data / "saves/decal",
                               paths.data / "assets/custom/mod",
                               paths.cache / "decals/paint",
                               paths.cache / "paint-shop/decal"};
    for (const auto &path : preserved) {
        WriteText(path, "preserve");
    }
    WriteText(root / "unrelated", "preserve");
    std::filesystem::create_directory_symlink(root, paths.data / "outside");
    Check(!coastline::ResetGamePaths(paths), "imported assets and asset caches are reset");
    for (const auto &path : preserved) {
        Check(ReadText(path) == "preserve", "reset preserves saves, decals, settings, and mods");
    }
    for (const auto *pack : {"default", "boss", "eur", "jpn"}) {
        Check(!std::filesystem::exists(paths.data / "assets" / pack),
              "reset removes each generated asset pack");
    }
    for (const auto *cache : {"assets", "asset-sources", "asset-search"}) {
        Check(!std::filesystem::exists(paths.cache / cache), "reset removes stale asset caches");
    }
    Check(!coastline::ResetGamePaths(paths), "reset tolerates already-cleared assets");
    Check(!coastline::ResetGamePaths(paths, true), "factory reset deletes all application data");
    for (const auto *path : {&paths.config, &paths.data, &paths.state, &paths.cache}) {
        Check(!std::filesystem::exists(*path), "factory reset removes each directory");
    }
    Check(ReadText(root / "unrelated") == "preserve", "reset preserves unrelated files");
    Check(!coastline::ResetGamePaths(paths, true),
          "absent directories permit another factory reset");
    Check(coastline::CreateGamePaths(paths), "directories can be recreated after reset");
    auto invalid = paths;
    invalid.cache = root.root_path();
    Check(coastline::ResetGamePaths(invalid).has_value(), "reset refuses a filesystem root");
    Check(std::filesystem::exists(paths.config), "all paths are checked before deletion");
    std::filesystem::remove_all(root);
}

void CheckAssetData() {
    const auto root = std::filesystem::temp_directory_path() / "coastline-system-assets-test";
    std::filesystem::remove_all(root);
    const auto user = root / "user";
    const std::array system{root / "missing", root / "system"};
    auto make = [](const std::filesystem::path &data) {
        const auto assets = coastline::DefaultAssets(data);
        WriteText(assets / "cars" / "model.gltf", "model");
        for (const auto *file : {"BossRally/pod.json",
                                 "RallyIntro1.json",
                                 "RallyIntro2.json",
                                 "RallyCredits.json",
                                 "RallyOutro.json"}) {
            WriteText(assets / file, "{}");
        }
        WriteText(assets / "n64" / "base" / "segment.bin", "segment");
        WriteText(assets / "n64/base/cartridge.ini", "[Cartridge]\nRegion=eur\n");
        WriteText(assets / "import.ini", "[Import]\nLayout=1\nBaseRegion=eur\n");
        coastline::RecordImport(assets, coastline::ImportSource::Disc);
        coastline::RecordImport(assets, coastline::ImportSource::Eur);
        coastline::RecordImportFiles(assets);
    };
    Check(!coastline::FindAssetData(user, system), "missing assets have no data root");
    make(user);
    Check(coastline::FindAssetData(user, system) == user, "user assets are the fallback");
    make(system[1]);
    Check(coastline::FindAssetData(user, system) == system[1], "system assets have priority");
    WriteText(user / "assets/mod/extra.txt", "user mod");
    const auto mod = coastline::PackAssets(system[1], root / "cache", "mod", user);
    Check(mod && ReadText(*mod / "extra.txt") == "user mod" &&
              ReadText(*mod / "cars/model.gltf") == "model",
          "user mods layer over system defaults in the writable cache");
    std::filesystem::remove(coastline::DefaultAssets(system[1]) / "import-files.txt");
    Check(coastline::FindAssetData(user, system) == user,
          "incomplete system assets fall back to usable user assets");
    const auto directories = coastline::SystemDataDirectories(root / "bin");
    Check(std::ranges::find(directories, root / "share/coastline") != directories.end(),
          "relocated prefixes have a data root beside bin");
    std::filesystem::remove_all(root);
}

void CheckPortablePaths() {
    const auto root = std::filesystem::temp_directory_path() / "coastline-portable-paths-test";
    std::filesystem::remove_all(root);
    const auto executable = root / "portable";
    const coastline::GamePaths fallback{
        root / "config", root / "data", root / "state", root / "cache"};
    std::filesystem::create_directories(executable);
    const auto home = root / "home";
    const auto stored =
        coastline::StorePortablePath(executable / "inputs/usa.z64", executable, home);
    Check(stored == "inputs/usa.z64", "portable input paths are stored relative to the executable");
    Check(coastline::RestorePortablePath(stored, root / "moved", home) ==
              root / "moved/inputs/usa.z64",
          "portable input paths follow a relocated executable");
    const auto downloads = coastline::StorePortablePath(home / "Downloads", executable, home);
    Check(downloads == "~/Downloads" &&
              coastline::RestorePortablePath(downloads, root / "moved", home) == home / "Downloads",
          "external home paths retain home shorthand across portable moves");
    Check(coastline::RestorePortablePath({}, executable, home).empty(),
          "empty portable paths remain empty");
    Check(coastline::RestorePortablePath(root / "external", executable, home) == root / "external",
          "absolute legacy paths remain usable");
    auto paths = coastline::ResolvePortablePaths(fallback, executable);
    Check(paths && paths->config == fallback.config && paths->data == fallback.data,
          "absent adjacent files use platform directories");
    WriteText(executable / "BossRally.ini", "[Game]\n");
    paths = coastline::ResolvePortablePaths(fallback, executable);
#if !defined(COASTLINE_PORTABLE) || COASTLINE_PORTABLE
    Check(paths && paths->portableConfig && paths->config == executable &&
              paths->data == fallback.data && paths->cache == fallback.cache,
          "adjacent configuration wins without changing absent portable data");
    std::filesystem::remove(executable / "BossRally.ini");
    std::filesystem::create_directories(executable / "assets");
    paths = coastline::ResolvePortablePaths(fallback, executable);
    Check(paths && !paths->portableConfig && paths->config == fallback.config &&
              paths->portableData && paths->data == executable &&
              paths->state == executable / "state" && paths->cache == executable / "cache",
          "adjacent assets select portable data with platform configuration fallback");
    WriteText(executable / "BossRally.ini", "[Game]\n");
    WriteText(executable / "coastline", "executable");
    WriteText(executable / "unrelated.txt", "preserved");
    WriteText(executable / "assets/default/test", "asset");
    paths = coastline::ResolvePortablePaths(fallback, executable);
    Check(paths && paths->portableConfig && paths->portableData,
          "adjacent configuration and data are selected together");
    Check(paths && !coastline::ResetGamePaths(*paths, true), "portable factory reset succeeds");
    Check(ReadText(executable / "coastline") == "executable" &&
              ReadText(executable / "unrelated.txt") == "preserved" &&
              !std::filesystem::exists(executable / "assets") &&
              !std::filesystem::exists(executable / "BossRally.ini"),
          "portable factory reset preserves executable and unrelated files");
    WriteText(executable / "BossRally.ini", "[Game]\n");
    std::filesystem::permissions(executable / "BossRally.ini", std::filesystem::perms::owner_read);
    Check(!coastline::ResolvePortablePaths(fallback, executable),
          "read-only adjacent configuration fails without platform fallback");
    std::filesystem::permissions(executable / "BossRally.ini", std::filesystem::perms::owner_all);
#else
    std::filesystem::create_directories(executable / "assets");
    paths = coastline::ResolvePortablePaths(fallback, executable);
    Check(paths && !paths->portableConfig && !paths->portableData &&
              paths->config == fallback.config && paths->data == fallback.data &&
              paths->state == fallback.state && paths->cache == fallback.cache,
          "disabled portable builds ignore all adjacent files");
#endif
    std::filesystem::remove_all(root);
}

} // namespace

int main() {
    CheckPortablePaths();
    const std::array<int, 10> nativeTracks{0, 1, 2, 3, 4, 6, 7, 8, 9, 10};
    std::set<int> engineTracks;
    for (int track = 0; track < coastline::MENU_TRACK_COUNT; ++track) {
        int engine = coastline::EngineTrackForMenu(track);
        engineTracks.insert(engine);
        Check(coastline::MenuTrackForEngine(engine) == track,
              "each track maps back to its menu index");
        if (track < coastline::CONSOLE_TRACK_COUNT) {
            Check(engine == nativeTracks[static_cast<std::size_t>(track)],
                  "the original track indices retain their engine mapping");
        }
    }
    Check(engineTracks.size() == 15 && *engineTracks.begin() == 0 && *engineTracks.rbegin() == 14,
          "every engine track appears once, including both PC tracks and their mirrors");
    Check(coastline::EngineTrackForMenu(10) == 5 && coastline::EngineTrackForMenu(11) == 11 &&
              coastline::EngineTrackForMenu(12) == 14 && coastline::EngineTrackForMenu(13) == 13 &&
              coastline::EngineTrackForMenu(14) == 12,
          "PC-only tracks and Gamewin do not reuse action or championship indices");
    Check(coastline::EngineTrackForMenu(-1) == -1 &&
              coastline::EngineTrackForMenu(coastline::MENU_TRACK_COUNT) == -1 &&
              coastline::MenuTrackForEngine(-1) == -1 && coastline::MenuTrackForEngine(15) == -1,
          "invalid tracks and menu actions cannot become engine tracks");
    const std::array<std::string, 2> gpuNames{"NVIDIA adapter 0", "NVIDIA adapter 1"};
    coastline::FrameTimings timings;
    for (std::uint64_t frame = 0; frame <= 60; ++frame) {
        timings.Record(2000000, frame * 1000000000 / 60);
    }
    Check(timings.Latest() && std::abs(timings.Latest()->fps - 60.0) < 0.0001 &&
              std::abs(timings.Latest()->intervalMs - 1000.0 / 60.0) < 0.0001 &&
              timings.Latest()->workMs == 2.0,
          "timing averages must distinguish CPU work from a paced 60 FPS interval");
    timings.Record(50000000, 1250000000);
    Check(timings.Latest()->workMs == 2.0 && timings.Latest()->fps == 60.0,
          "displayed timings must remain stable between one-second samples");
    for (std::uint64_t frame = 2; frame <= 4; ++frame) {
        timings.Record(2000000, 1000000000 + frame * 250000000);
    }
    Check(timings.Latest()->fps == 4.0 && timings.Latest()->worstFps == 4.0 &&
              timings.Latest()->workMs == 14.0 && timings.Latest()->worstWorkMs == 50.0 &&
              timings.Latest()->intervalMs == 250.0 && timings.Latest()->worstIntervalMs == 250.0,
          "slow frames must update timing averages and worst values");
    for (std::uint64_t frame = 1; frame <= 60; ++frame) {
        timings.Record(1000000, 2000000000 + frame * 1000000000 / 60);
    }
    Check(timings.Latest()->fps == 60.0 && timings.Latest()->worstFps == 4.0 &&
              timings.Latest()->worstWorkMs == 50.0 && timings.Latest()->worstIntervalMs == 250.0,
          "worst values must persist when frame timing improves");
    Check(coastline::UniqueGpuIndex("NVIDIA adapter 1", gpuNames) == 1,
          "GPU statistics must match the selected adapter instead of the first device");
    Check(!coastline::UniqueGpuIndex("Intel integrated adapter", gpuNames),
          "an integrated adapter must not match an NVIDIA statistics device");
    const std::array<std::string, 2> duplicateNames{"NVIDIA adapter", "NVIDIA adapter"};
    Check(!coastline::UniqueGpuIndex("NVIDIA adapter", duplicateNames),
          "ambiguous adapter identities must disable GPU utilization sampling");
    CheckAssetData();
    CheckReset();
    CheckImportInventory();
    CheckRunScript();
    CheckPerformanceParsing();
    CheckIconFile();
    CheckVolumeLevels();
    CheckPackAssets();
    CheckImportVersions();
    CheckLanguage();
    CheckConsoleStrings();

    auto defaults =
        coastline::ResolveXdgPaths(Environment({{"HOME", "/home/player"}}), "coastline");
    Check(defaults.has_value(), "HOME alone resolves the directories");
    Check(defaults->config == "/home/player/.config/coastline", "default configuration directory");
    Check(defaults->data == "/home/player/.local/share/coastline", "default data directory");
    Check(defaults->state == "/home/player/.local/state/coastline", "default state directory");
    Check(defaults->cache == "/home/player/.cache/coastline", "default cache directory");

    auto overridden =
        coastline::ResolveXdgPaths(Environment({{"HOME", "/home/player"},
                                                {"XDG_CONFIG_HOME", "/etc/xdg-user"},
                                                {"XDG_CACHE_HOME", "relative/cache"}}),
                                   "coastline");
    Check(overridden->config == "/etc/xdg-user/coastline", "XDG_CONFIG_HOME is used");
    Check(overridden->cache == "/home/player/.cache/coastline", "a relative XDG value is ignored");
    Check(!coastline::ResolveXdgPaths(Environment({}), "coastline"), "no base gives nothing");

    auto config = coastline::GameConfig::Parse("; comment\r\n[Video]\r\nDriver=D3D\r\n"
                                               "Width = 1920\r\n\r\n[game]\r\nPlayMusic=0\r\n"
                                               "PlaySFX=yes\r\nWidth=wide\r\n");
    Check(config.Get("video", "DRIVER") == "D3D", "names compare without case");
    Check(config.GetInteger("Video", "Width", 0) == 1920, "values are trimmed");
    Check(!config.GetBoolean("Game", "PlayMusic", true), "0 is false");
    Check(config.GetBoolean("Game", "PlaySFX", false), "yes is true");
    Check(config.GetInteger("Game", "Width", 7) == 7, "a malformed number gives the fallback");
    Check(!coastline::GameConfig::Parse("[Video]\nWidth=1920\nbroken\n").Get("Video", "Width"),
          "malformed configuration resets every value");
    Check(!coastline::GameConfig::Parse("[Video\nWidth=1920\n").Get("Video", "Width"),
          "a truncated section header resets configuration");
    Check(!coastline::GameConfig::Parse("[Video]\nWidth=1920\nX=\xc0\x80\n").Get("Video", "Width"),
          "invalid UTF-8 resets configuration");
    Check(coastline::GameConfig::Parse("[Paths]\nSource=é 日本 😀\n").Get("Paths", "Source") ==
              "é 日本 😀",
          "valid UTF-8 paths remain unchanged");
    config.Set("Video", "driver", "SDL");
    Check(config.Get("Video", "Driver") == "SDL", "Set replaces the key of any case");

    auto scales = coastline::GameConfig::Parse("[Video]\nRenderScale=1.5\nFieldOfView=-0.25\n"
                                               "Gain=2x\nEmpty=\n");
    Check(scales.GetFloat("Video", "RenderScale", 0.0f) == 1.5f, "a decimal is read");
    Check(scales.GetFloat("Video", "FieldOfView", 0.0f) == -0.25f, "a negative decimal is read");
    Check(scales.GetFloat("Video", "Gain", 1.0f) == 1.0f, "trailing text gives the fallback");
    Check(scales.GetFloat("Video", "Empty", 3.0f) == 3.0f, "an empty value gives the fallback");

    Check(coastline::ParseAspectRatio("auto") == 0.0f, "auto follows the window");
    Check(coastline::ParseAspectRatio(" AUTO ") == 0.0f, "auto is read without case or spaces");
    Check(coastline::ParseAspectRatio("4:3") == 4.0f / 3.0f,
          "a ratio is the width over the height");
    Check(coastline::ParseAspectRatio("16 : 9") == 16.0f / 9.0f, "a ratio may have spaces");
    Check(coastline::ParseAspectRatio("1.6") == 1.6f, "a number is the ratio itself");
    Check(!coastline::ParseAspectRatio("4:0"), "a height of 0 is no ratio");
    Check(!coastline::ParseAspectRatio("-1"), "a ratio not above 0 is refused");
    Check(!coastline::ParseAspectRatio("wide"), "text is refused");
    Check(!coastline::ParseAspectRatio("inf"), "an infinite ratio is refused");

    for (const auto &[aspect, expected] :
         std::array{std::pair{4.0f / 3.0f, std::array{320, 240}},
                    std::pair{16.0f / 9.0f, std::array{427, 240}},
                    std::pair{21.0f / 9.0f, std::array{560, 240}},
                    std::pair{32.0f / 9.0f, std::array{854, 240}},
                    std::pair{9.0f / 16.0f, std::array{320, 569}}}) {
        Check(coastline::ExpandInternalResolution(320, 240, aspect) == expected,
              "aspect ratios expand the base resolution without cropping");
    }
    Check(coastline::ExpandInternalResolution(640, 360, 4.0f / 3.0f) == std::array{640, 480},
          "a taller aspect preserves the selected width");
    Check(coastline::ExpandInternalResolution(1920, 1080, 16.0f / 9.0f) == std::array{1920, 1080},
          "float rounding does not add a pixel to an exact resolution");
    Check(coastline::ExpandInternalResolution(1, 1, 4.0f / 3.0f) == std::array{320, 240},
          "internal resolution stays at least 320x240");
    Check(coastline::ExpandInternalResolution(320, 240, 1e30f)[0] == 16384,
          "extreme aspect ratios respect the texture size limit");

    auto choices = coastline::GameConfig::Parse("[Player2]\nCar=12\nHandling=-1\nTires=x\n"
                                                "Suspension=3\nTransmission=0\n");
    Check(choices.GetIndex("Player2", "Car", 13, 0) == 12, "the last index is read");
    Check(choices.GetIndex("Player2", "Car", 12, 4) == 4, "an index past the choices falls back");
    Check(choices.GetIndex("Player2", "Handling", 3, 1) == 1, "a negative index falls back");
    Check(choices.GetIndex("Player2", "Tires", 5, 2) == 2, "a malformed index falls back");
    Check(choices.GetIndex("Player2", "Transmission", 2, 1) == 0, "index 0 is read");
    Check(choices.GetIndex("Player1", "Car", 13, 7) == 7, "an absent index falls back");
    choices.Set("Player1", "Car", "9");
    choices.Set("Player1", "CustomDecals", "1");
    auto file = std::filesystem::temp_directory_path() / "coastline-sdl-test-choices.ini";
    Check(choices.Save(file), "the choices are written");
    auto reread = coastline::GameConfig::Load(file);
    std::filesystem::remove(file);
    Check(reread.GetIndex("Player1", "Car", 13, 0) == 9 &&
              reread.GetBoolean("Player1", "CustomDecals", false) &&
              reread.GetIndex("Player2", "Suspension", 4, 0) == 3,
          "the choices are read back from the file");

    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
