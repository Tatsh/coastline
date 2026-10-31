// Host check of the XDG directories, the layers of the asset packs, the record of the versions of
// the import, the BossRally.ini reader, the icon file reader, and the volume levels of the SDL
// game.

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
#include <string>
#include <vector>

#include "config.h"
#include "iconfile.h"
#include "importversion.h"
#include "language.h"
#include "paths.h"
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
    WriteText(defaults / "import.ini", "[Import]\nDisc=0\nUsa=1\nEur=1\n");
    Check(coastline::StaleImports(defaults) == std::vector{ImportSource::Disc},
          "an older version is old");
    Check(coastline::RecordImport(defaults, ImportSource::Disc), "the record is updated");
    Check(coastline::StaleImports(defaults).empty(), "every source is current");
    Check(coastline::GameConfig::Load(defaults / "import.ini").GetInteger("Import", "Eur", 0) == 1,
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
    Check(coastline::ReleaseDirectory(assets, "ja") == assets / "n64" / "usa",
          "ja without the Japanese release draws the USA release");
    WriteText(assets / "n64" / "jpn" / "segment.bin", "segment");
    Check(coastline::ReleaseDirectory(assets, "ja") == assets / "n64" / "jpn",
          "ja draws the Japanese release");
    Check(coastline::ReleaseDirectory(assets, "en-us") == assets / "n64" / "usa",
          "en-us draws the USA release");
    std::filesystem::remove_all(assets);
}

} // namespace

int main() {
    CheckIconFile();
    CheckVolumeLevels();
    CheckPackAssets();
    CheckImportVersions();
    CheckLanguage();

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
                                               "PlaySFX=yes\r\nbroken\r\nWidth=wide\r\n");
    Check(config.Get("video", "DRIVER") == "D3D", "names compare without case");
    Check(config.GetInteger("Video", "Width", 0) == 1920, "values are trimmed");
    Check(!config.GetBoolean("Game", "PlayMusic", true), "0 is false");
    Check(config.GetBoolean("Game", "PlaySFX", false), "yes is true");
    Check(config.GetInteger("Game", "Width", 7) == 7, "a malformed number gives the fallback");
    Check(!config.Get("Game", "broken"), "a line without = is skipped");
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
