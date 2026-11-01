#include <array>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string_view>

#include "gamesettings.h"

namespace {

std::size_t Find(const coastline::GameSettings &settings, std::string_view prefix) {
    for (std::size_t index = 0; index < settings.Count(); ++index) {
        if (settings.Label(index).starts_with(prefix)) {
            return index;
        }
    }
    assert(false);
    return settings.Count();
}

void File(const std::filesystem::path &path, const char *text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    file << text;
    assert(file);
}

} // namespace

int main() {
    using coastline::GameConfig;
    using coastline::GameSettings;
    const auto directory =
        std::filesystem::temp_directory_path() /
        ("coastline-settings-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto system = directory / "system";
    const auto user = directory / "user";
    File(system / "assets/default/pack.json", "{}");
    File(system / "assets/default/n64/base/cartridge.ini", "[Cartridge]\nRegion=usa\n");
    File(system / "assets/boss/pack.json", "{}");
    File(user / "assets/jpn/pack.json", "{}");
    File(user / "assets/jpn/n64/base/cartridge.ini", "[Cartridge]\nRegion=jpn\n");
    File(user / "assets/custom/menu-items/model.gltf", "{}");
    const std::array roots{system, user};
    auto config = GameConfig::Parse("[Other]\nPreserved=value\n[Video]\nFieldOfView=1.250000\n"
                                    "Antialias=broken\nInternalWidth=800\nInternalHeight=600\n"
                                    "Width=1280\n");
    const std::array displayResolutions{std::array{1920, 1080},
                                        std::array{3840, 2160},
                                        std::array{1920, 1080},
                                        std::array{2880, 1800}};
    GameSettings settings(config, roots, displayResolutions);
    const auto pack = Find(settings, "Asset pack:");
    assert(settings.Label(pack) == "Asset pack: PC + branded images");
    assert(settings.Step(pack, 1));
    assert(config.Get("Game", "AssetPack") == "boss");
    assert(settings.Label(pack) == "Asset pack: Pure PC");
    assert(settings.Step(pack, 1));
    assert(config.Get("Game", "AssetPack") == "custom");
    assert(settings.Step(pack, 1));
    assert(config.Get("Game", "AssetPack") == "default");
    assert(settings.Step(pack, -1));
    assert(config.Get("Game", "AssetPack") == "custom");

    const auto region = Find(settings, "Models and textures:");
    assert(settings.Label(region) == "Models and textures: USA");
    assert(settings.Step(region, 1));
    assert(config.Get("Game", "Region") == "jpn");
    assert(settings.Step(region, 1));
    assert(config.Get("Game", "Region") == "usa");

    const auto fov = Find(settings, "Field of view:");
    assert(settings.Label(fov) == "Field of view: 125%");
    assert(settings.Step(fov, -1));
    assert(config.GetFloat("Video", "FieldOfView", 0) == 1.0f);
    config.Set("Video", "FieldOfView", "1.2");
    assert(settings.Label(fov) == "Field of view: Custom (1.2)");
    assert(config.Get("Video", "FieldOfView") == "1.2");
    config.Set("Video", "FieldOfView", "nan");
    assert(settings.Label(fov) == "Field of view: 100%");
    const auto antialias = Find(settings, "Anti-aliasing:");
    assert(settings.Label(antialias) == "Anti-aliasing: Off");
    assert(settings.Step(antialias, -1));
    assert(config.GetInteger("Video", "Antialias", 0) == 8);

    const auto resolution = Find(settings, "Internal resolution:");
    assert(settings.Label(resolution) == "Internal resolution: 800x600");
    assert(!settings.RequiresRestart(resolution));
    assert(settings.Step(resolution, -1));
    assert(settings.Label(resolution) == "Internal resolution: 720x576");
    config.Set("Video", "InternalWidth", "0");
    config.Set("Video", "InternalHeight", "0");
    assert(settings.Label(resolution) == "Internal resolution: Window size");
    assert(settings.Step(resolution, 1));
    assert(settings.Label(resolution) == "Internal resolution: 320x240");
    assert(settings.Step(resolution, -1));
    assert(settings.Step(resolution, -1));
    assert(settings.Label(resolution) == "Internal resolution: 3840x2160");
    bool foundDesktop = false;
    for (int step = 0;
         step < 100 && settings.Label(resolution) != "Internal resolution: Window size";
         ++step) {
        foundDesktop |= settings.Label(resolution) == "Internal resolution: 2880x1800";
        assert(settings.Step(resolution, -1));
    }
    assert(foundDesktop);
    assert(config.GetFloat("Video", "RenderScale", 0) == 1.0f);
    assert(config.GetInteger("Video", "InternalWidth", -1) == 0);
    assert(config.GetInteger("Video", "InternalHeight", -1) == 0);
    assert(config.GetInteger("Video", "Width", 0) == 1280);

    const auto music = Find(settings, "Music:");
    const auto sounds = Find(settings, "Sound effects:");
    assert(settings.Label(sounds) == "Sound effects: PC");
    assert(settings.Step(sounds, 1));
    assert(config.Get("Game", "SoundEffects") == "N64");
    const auto effects = Find(settings, "Visual effects:");
    assert(settings.Label(effects) == "Visual effects: PC");
    assert(!settings.RequiresRestart(effects));
    assert(settings.Step(effects, 1));
    assert(config.Get("Game", "Effects") == "N64");
    const auto autosave = Find(settings, "Auto-save race progress:");
    assert(settings.Label(autosave) == "Auto-save race progress: On");
    assert(!settings.RequiresRestart(autosave));
    assert(!settings.RequiresRestart(fov));
    assert(!settings.RequiresRestart(settings.Count()));
    config.Set("Game", "AutoSave", "oFF");
    assert(settings.Label(autosave) == "Auto-save race progress: Off");
    assert(settings.Step(autosave, 1));
    assert(config.GetBoolean("Game", "AutoSave", false));
    config.Set("Game", "AutoSave", "TrUe");
    assert(settings.Label(autosave) == "Auto-save race progress: On");
    assert(settings.Step(autosave, -1));
    assert(!config.GetBoolean("Game", "AutoSave", true));
    const auto distance = Find(settings, "Draw distance:");
    assert(settings.Label(distance) == "Draw distance: 1x");
    assert(!settings.RequiresRestart(distance));
    assert(settings.Step(distance, -1));
    assert(config.GetFloat("Video", "DrawDistance", 0) == 4.0f);
    assert(settings.Step(distance, 1));
    assert(config.GetFloat("Video", "DrawDistance", 0) == 1.0f);
    assert(settings.Step(distance, 1));
    assert(config.GetFloat("Video", "DrawDistance", 0) == 1.5f);
    const auto title = Find(settings, "Title screen:");
    assert(settings.Label(title) == "Title screen: USA/Europe");
    assert(settings.RequiresRestart(title));
    config.Set("Game", "Language", "ja");
    assert(settings.Step(title, -1));
    assert(config.Get("Game", "TitleScreen") == "japanese");
    assert(config.Get("Game", "Region") == "usa");
    assert(config.Get("Game", "Language") == "ja");
    assert(settings.Step(title, -1));
    assert(config.Get("Game", "TitleScreen") == "international");
    assert(settings.Step(title, 1));
    assert(config.Get("Game", "TitleScreen") == "japanese");
    assert(settings.Step(music, 1));
    assert(config.Get("Game", "Music") == "CD");
    assert(!settings.Step(music, 0));
    assert(!settings.Step(settings.Count(), 1));
    assert(settings.Label(settings.Count()).empty());
    const auto path = directory / "BossRally.ini";
    assert(config.Save(path));
    auto loaded = GameConfig::Load(path);
    GameSettings restored(loaded, roots);
    for (std::size_t index = 0; index < settings.Count(); ++index) {
        assert(restored.Label(index) == settings.Label(index));
    }
    assert(loaded.Get("Other", "Preserved") == "value");
    const auto previousMusic = settings.Label(music);
    assert(settings.AdjustAndSave(music, 1, directory) == GameSettings::Adjustment::SaveFailed);
    assert(settings.Label(music) == previousMusic);
    assert(config.Get("Other", "Preserved") == "value");
    assert(settings.AdjustAndSave(settings.Count(), 1, directory) ==
           GameSettings::Adjustment::Unchanged);
    assert(settings.AdjustAndSave(music, 1, path) == GameSettings::Adjustment::Saved);
    assert(GameConfig::Load(path).Get("Game", "Music") == "XM");

    loaded.Set("Game", "AssetPack", "my-pack");
    GameSettings custom(loaded);
    assert(custom.Label(pack) == "Asset pack: my-pack");
    assert(custom.Step(pack, -1));
    assert(loaded.Get("Game", "AssetPack") == "default");
    GameConfig empty;
    GameSettings single(empty);
    assert(!single.Step(Find(single, "Models and textures:"), 1));
    empty.Set("Game", "TitleScreen", "japanese");
    GameSettings unavailable(empty);
    const auto unavailableTitle = Find(unavailable, "Title screen:");
    assert(unavailable.Label(unavailableTitle) == "Title screen: Japan missing");
    assert(unavailable.Step(unavailableTitle, 1));
    assert(empty.Get("Game", "TitleScreen") == "international");
    auto japaneseOnly = GameConfig::Parse("[Game]\nRegion=jpn\nTitleScreen=international\n");
    GameSettings missingInternational(japaneseOnly);
    const auto missingTitle = Find(missingInternational, "Title screen:");
    assert(missingInternational.Label(missingTitle) == "Title screen: USA/Europe missing");
    assert(missingInternational.Step(missingTitle, -1));
    assert(japaneseOnly.Get("Game", "TitleScreen") == "japanese");
    japaneseOnly.Set("Game", "TitleScreen", "regional");
    assert(missingInternational.Label(missingTitle) == "Title screen: Japan");
    File(user / "assets" / std::filesystem::path(u8"日本") / "model.glb", "glTF");
    GameConfig unicodeConfig;
    GameSettings unicode(unicodeConfig, roots);
    bool foundUnicode = false;
    for (int step = 0; step < 4; ++step) {
        unicode.Step(Find(unicode, "Asset pack:"), 1);
        foundUnicode |= unicodeConfig.Get("Game", "AssetPack") == "日本";
    }
    assert(foundUnicode);
    std::filesystem::remove_all(directory);
}
