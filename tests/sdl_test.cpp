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
    Check(coastline::RecordImport(defaults, ImportSource::Disc), "the record is updated");
    Check(coastline::StaleImports(defaults).empty(), "every source is current");
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
    Check(coastline::ReleaseDirectory(assets, "en-gb") == assets / "n64" / "usa",
          "en-gb without the European release draws the USA release");
    WriteText(assets / "n64" / "eur" / "segment.bin", "segment");
    Check(coastline::ReleaseDirectory(assets, "en-gb") == assets / "n64" / "eur",
          "en-gb draws the European release");
    std::filesystem::remove_all(assets);
}

void CheckConsoleStrings() {
    std::set<std::string_view> keys;
    for (const auto &message : coastline::CONSOLE_STRINGS) {
        Check(keys.insert(message.key).second, "each key of the console strings is unique");
        Check(message.key.starts_with("n64."), "each key of the console strings starts with n64.");
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
}

void CheckPerformanceParsing() {
    Check(coastline::StatmResidentPages("53287 12345 4321 1 0 9876 0\n") == 12345u,
          "the second number of statm is the resident pages");
    Check(coastline::StatmResidentPages("  7 8") == 8u, "statm may open with spaces");
    Check(!coastline::StatmResidentPages("53287"), "statm without a second number gives nothing");
    Check(!coastline::StatmResidentPages(""), "an empty statm gives nothing");
    constexpr std::string_view ROLLUP = "55a1c000-7ffd0000 ---p 00000000 00:00 0 [rollup]\n"
                                        "Rss:             1406744 kB\n"
                                        "Pss:              912345 kB\n"
                                        "Pss_Dirty:        800000 kB\n";
    Check(coastline::SmapsRollupPssBytes(ROLLUP) == 912345ull * 1024,
          "the Pss line of smaps_rollup is the proportional set size in bytes");
    Check(coastline::SmapsRollupPssBytes("Pss: 4 kB\n") == 4096u, "Pss may open the text");
    Check(!coastline::SmapsRollupPssBytes("Rss: 4 kB\nPss_Dirty: 2 kB\n"), "Pss_Dirty is not Pss");
    Check(!coastline::SmapsRollupPssBytes(""), "an empty rollup gives nothing");
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

} // namespace

int main() {
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
