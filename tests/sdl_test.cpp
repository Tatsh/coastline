// Host check of the XDG directories, the BossRally.ini reader, the icon file reader, and the volume
// levels of the SDL game.

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>

#include "config.h"
#include "iconfile.h"
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

} // namespace

int main() {
    CheckIconFile();
    CheckVolumeLevels();

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
