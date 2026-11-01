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
    GameSettings settings(config, roots);
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

    const auto scale = Find(settings, "Internal resolution:");
    assert(settings.Label(scale) == "Internal resolution: 100%");
    assert(settings.Step(scale, -1));
    assert(config.GetFloat("Video", "RenderScale", 0) == 0.75f);
    assert(config.GetInteger("Video", "InternalWidth", -1) == 0);
    assert(config.GetInteger("Video", "InternalHeight", -1) == 0);
    assert(config.GetInteger("Video", "Width", 0) == 1280);

    const auto music = Find(settings, "Music:");
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
