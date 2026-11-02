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
    GameConfig units;
    assert(coastline::SpeedInMph(units, "", ""));
    assert(coastline::SpeedInMph(units, "", "GB"));
    assert(coastline::SpeedInMph(units, "", "US"));
    for (const auto country : {"JP", "DE", "FR", "CA", "AU"}) {
        assert(!coastline::SpeedInMph(units, "", country));
    }
    assert(!coastline::SpeedInMph(units, "jpn", "US"));
    assert(!coastline::SpeedInMph(units, "eur", "GB"));
    assert(coastline::SpeedInMph(units, "usa", "JP"));
    assert(!units.Get("Game", "Units"));
    units.Set("Game", "Units", "mph");
    assert(coastline::SpeedInMph(units, "jpn", "JP"));
    units.Set("Game", "Units", "kph");
    assert(!coastline::SpeedInMph(units, "usa", "GB"));
    units.Set("Game", "Units", "auto");
    assert(!coastline::SpeedInMph(units, "", "JP"));
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
    const auto upscale = Find(settings, "Upscaling:");
    const auto shader = Find(settings, "Screen shader:");
    const auto aspect = Find(settings, "Aspect ratio:");
    assert(settings.Label(aspect) == "Aspect ratio: Fill screen");
    assert(!settings.RequiresRestart(aspect));
    for (const auto *ratio : {"4:3", "16:9", "21:9", "32:9", "auto"}) {
        assert(settings.Step(aspect, 1));
        assert(config.Get("Video", "AspectRatio") == ratio);
    }
    assert(!settings.RequiresRestart(upscale));
    assert(!settings.RequiresRestart(shader));
    assert(settings.Step(upscale, -1));
    assert(config.Get("Video", "ScaleFilter") == "edge-adaptive");
    assert(settings.Step(shader, 1));
    assert(config.Get("Video", "ScreenShader") == "consumer-crt");
    assert(settings.Step(shader, 1));
    assert(settings.Label(shader) == "Screen shader: PVM");
    assert(settings.Step(shader, 1));
    assert(settings.Label(shader) == "Screen shader: Aperture grille");
    const auto sounds = Find(settings, "Sound effects:");
    assert(settings.Label(sounds) == "Sound effects: PC");
    assert(settings.Step(sounds, 1));
    assert(config.Get("Game", "SoundEffects") == "N64");
    const auto effects = Find(settings, "Visual effects:");
    assert(settings.Label(effects) == "Visual effects: PC");
    assert(!settings.RequiresRestart(effects));
    assert(settings.Step(effects, 1));
    assert(config.Get("Game", "Effects") == "N64");
    const auto reflections = Find(settings, "Window reflections:");
    assert(settings.Label(reflections) == "Window reflections: Auto");
    assert(!settings.RequiresRestart(reflections));
    assert(settings.AdjustAndSave(reflections, 1, directory / "BossRally.ini") ==
           GameSettings::Adjustment::Saved);
    assert(GameConfig::Load(directory / "BossRally.ini").Get("Game", "WindowReflections") == "PC");
    assert(settings.Step(reflections, 1));
    assert(settings.Label(reflections) == "Window reflections: N64");
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

    auto presetConfig = GameConfig::Parse("[Game]\nLanguage=ja-jp\nRegion=jpn\nMusic=CD\n"
                                          "TitleScreen=japanese\nAutoSave=1\nSoundEffects=PC\n"
                                          "[Video]\nAspectRatio=21:9\n");
    GameSettings presets(presetConfig, roots);
    const auto preset = Find(presets, "Preset:");
    assert(presets.Label(preset) == "Preset: Custom");
    assert(presets.AdjustAndSave(preset, 1, path) == GameSettings::Adjustment::Saved);
    assert(presetConfig.Get("Video", "GraphicsPreset") == "n64");
    assert(presetConfig.Get("Game", "AssetPack") == "default");
    assert(presetConfig.Get("Game", "Effects") == "N64");
    assert(presetConfig.Get("Game", "WindowReflections") == "N64");
    assert(presetConfig.Get("Game", "SoundEffects") == "N64");
    assert(presetConfig.Get("Video", "TextureFilter") == "n64");
    assert(presetConfig.Get("Video", "ScreenShader") == "consumer-crt");
    assert(presetConfig.GetInteger("Video", "InternalWidth", 0) == 320);
    assert(presetConfig.GetInteger("Video", "InternalHeight", 0) == 240);
    assert(presetConfig.GetInteger("Video", "Antialias", 0) == 1);
    assert(presetConfig.Get("Video", "AlphaToCoverage") == "0");
    assert(presetConfig.Get("Video", "AnisotropicFiltering") == "0");
    assert(presetConfig.Get("Video", "AntialiasPost") == "none");
    assert(presetConfig.Get("Video", "ScaleFilter") == "nearest");
    assert(presetConfig.Get("Video", "FieldOfView") == "1");
    assert(presetConfig.Get("Video", "DrawDistance") == "1");
    assert(presetConfig.Get("Game", "TitleScreen") == "japanese");
    assert(presetConfig.Get("Game", "AutoSave") == "1");
    assert(presetConfig.GetInteger("Video", "FrameRateLimit", 0) == 30);
    assert(presetConfig.Get("Video", "AspectRatio") == "4:3");
    assert(presetConfig.Get("Game", "Region") == "jpn");
    assert(presetConfig.Get("Game", "Language") == "ja-jp");
    assert(presetConfig.Get("Game", "Music") == "XM");
    assert(presetConfig.Get("Game", "SoundEffects") == "N64");
    auto persistedPreset = GameConfig::Load(path);
    assert(persistedPreset.Get("Video", "AspectRatio") == "4:3");
    GameSettings restoredPreset(persistedPreset, roots);
    assert(restoredPreset.Label(preset) == "Preset: N64");
    assert(presets.RequiresRestart(preset));
    presets.FinishPresetSelection();
    presetConfig.Set("Video", "AspectRatio", "21:9");
    presetConfig.Set("Game", "Music", "XM");
    assert(presets.Step(preset, 1));
    assert(presetConfig.Get("Video", "GraphicsPreset") == "br");
    assert(presetConfig.Get("Game", "AssetPack") == "boss");
    assert(presetConfig.Get("Video", "ScreenShader") == "pc-crt");
    assert(presetConfig.GetInteger("Video", "InternalWidth", 0) == 640);
    assert(presetConfig.GetInteger("Video", "InternalHeight", 0) == 480);
    assert(presetConfig.GetInteger("Video", "FrameRateLimit", 0) == 60);
    assert(presetConfig.Get("Video", "AspectRatio") == "4:3");
    assert(presetConfig.Get("Game", "Music") == "CD");
    assert(presetConfig.Get("Game", "SoundEffects") == "PC");
    assert(presetConfig.Get("Game", "Effects") == "PC");
    assert(presetConfig.Get("Game", "WindowReflections") == "PC");
    assert(presetConfig.Get("Game", "Region") == "jpn");
    assert(presetConfig.Get("Game", "TitleScreen") == "japanese");
    assert(presetConfig.Get("Game", "AutoSave") == "1");
    assert(presetConfig.Get("Video", "Antialias") == "1");
    assert(presetConfig.Get("Video", "AlphaToCoverage") == "0");
    assert(presetConfig.Get("Video", "AnisotropicFiltering") == "0");
    assert(presetConfig.Get("Video", "TextureFilter") == "modern");
    assert(presetConfig.Get("Video", "FieldOfView") == "1");
    assert(presetConfig.Get("Video", "DrawDistance") == "1");
    assert(presetConfig.Get("Video", "AntialiasPost") == "none");
    assert(presetConfig.Get("Video", "ScaleFilter") == "nearest");
    assert(presets.AdjustAndSave(Find(presets, "Music:"), -1, path) ==
           GameSettings::Adjustment::Saved);
    assert(GameConfig::Load(path).Get("Game", "Music") == "XM");
    assert(presets.AdjustAndSave(preset, -1, path) == GameSettings::Adjustment::Saved);
    assert(presets.AdjustAndSave(preset, 1, path) == GameSettings::Adjustment::Saved);
    assert(GameConfig::Load(path).Get("Game", "Music") == "CD");
    presets.FinishPresetSelection();
    presetConfig.Set("Video", "AspectRatio", "32:9");
    const std::array preservedKeys{
        "TitleScreen", "AutoSave", "SoundEffects", "Music", "Region", "AssetPack"};
    presetConfig.Set("Game", "TitleScreen", "japanese");
    presetConfig.Set("Game", "AutoSave", "1");
    const auto beforeUltra = presetConfig;
    assert(presets.Step(preset, 1));
    for (const auto key : preservedKeys) {
        assert(presetConfig.Get("Game", key) == beforeUltra.Get("Game", key));
    }
    assert(presetConfig.Get("Video", "AspectRatio") == "auto");
    assert(presetConfig.GetInteger("Video", "DrawDistance", 0) == 2);
    assert(presetConfig.GetInteger("Video", "Antialias", 0) == 8);
    assert(presetConfig.Get("Video", "ScaleFilter") == "nearest");
    assert(presetConfig.Get("Video", "ScreenShader") == "none");
    assert(presetConfig.Get("Video", "AntialiasPost") == "none");
    assert(presetConfig.Get("Video", "InternalWidth") == "0");
    assert(presetConfig.Get("Video", "InternalHeight") == "0");
    assert(presetConfig.Get("Video", "FieldOfView") == "1");
    assert(presetConfig.Get("Video", "TextureFilter") == "modern");
    assert(presetConfig.Get("Video", "AnisotropicFiltering") == "16");
    assert(presetConfig.Get("Game", "Effects") == "PC");
    assert(presetConfig.Get("Video", "GraphicsPreset") == "ultra");
    assert(presetConfig.Get("Video", "WideMenus") == "auto");
    presets.FinishPresetSelection();
    presetConfig.Set("Video", "AspectRatio", "32:9");
    presetConfig.Set("Video", "GpuPreference", "high-performance");
    assert(presets.Step(preset, 1));
    assert(presetConfig.Get("Video", "AspectRatio") == "32:9");
    assert(presetConfig.GetInteger("Video", "DrawDistance", 0) == 1);
    assert(presetConfig.GetInteger("Video", "Antialias", 0) == 1);
    assert(presetConfig.Get("Video", "GraphicsPreset") == "worst");
    assert(presetConfig.Get("Video", "WideMenus") == "0");
    assert(presetConfig.Get("Video", "GpuPreference") == "high-performance");
    assert(presetConfig.Get("Video", "ScreenShader") == "none");
    assert(presetConfig.Get("Video", "ScaleFilter") == "nearest");
    assert(presetConfig.Get("Video", "TextureFilter") == "nearest");
    assert(presets.Step(preset, 1));
    assert(presetConfig.Get("Video", "GraphicsPreset") == "low-power");
    assert(presetConfig.Get("Video", "GpuPreference") == "low-power");
    assert(presetConfig.Get("Video", "AspectRatio") == "32:9");
    assert(presetConfig.GetInteger("Video", "InternalWidth", 0) == 640);
    assert(presetConfig.GetInteger("Video", "InternalHeight", 0) == 480);
    assert(presetConfig.GetInteger("Video", "Antialias", 0) == 2);
    assert(presetConfig.Get("Video", "TextureFilter") == "modern");
    assert(presetConfig.Get("Video", "ScreenShader") == "none");
    assert(presets.RequiresRestart(preset));
    assert(presets.RequiresRestart(Find(presets, "GPU preference:")));
    assert(presets.Step(Find(presets, "Screen shader:"), 1));
    assert(presets.Label(preset) == "Preset: Custom");
    assert(presets.AdjustAndSave(preset, 1, directory) == GameSettings::Adjustment::SaveFailed);
    assert(presets.Label(preset) == "Preset: Custom");
    assert(presetConfig.Get("Game", "AssetPack") == "boss");
    assert(!coastline::ApplyGraphicsPreset(presetConfig, "unknown"));

    auto browsingConfig = GameConfig::Parse("[Game]\nMusic=XM\nSoundEffects=N64\n"
                                            "AssetPack=custom\n[Video]\nGraphicsPreset=custom\n");
    GameSettings browsing(browsingConfig, roots);
    const auto browsingPreset = Find(browsing, "Preset:");
    for (int selection = 0; selection < 3; ++selection) {
        assert(browsing.AdjustAndSave(browsingPreset, 1, path) == GameSettings::Adjustment::Saved);
    }
    assert(browsingConfig.Get("Video", "GraphicsPreset") == "ultra");
    auto savedUltra = GameConfig::Load(path);
    assert(savedUltra.Get("Game", "Music") == "XM");
    assert(savedUltra.Get("Game", "SoundEffects") == "N64");
    assert(savedUltra.Get("Game", "AssetPack") == "custom");
    browsing.FinishPresetSelection();
    assert(browsing.AdjustAndSave(browsingPreset, -1, path) == GameSettings::Adjustment::Saved);
    assert(browsingConfig.Get("Game", "Music") == "CD");
    browsing.FinishPresetSelection();
    assert(browsing.AdjustAndSave(browsingPreset, 1, path) == GameSettings::Adjustment::Saved);
    assert(GameConfig::Load(path).Get("Game", "Music") == "CD");

    auto legacyConfig = GameConfig::Parse("[Video]\nGraphicsPreset=best\nAntialias=4\n");
    GameSettings legacy(legacyConfig, roots);
    assert(legacy.Label(Find(legacy, "Preset:")) == "Preset: Ultra");
    assert(legacyConfig.Get("Video", "GraphicsPreset") == "ultra");
    assert(legacyConfig.GetInteger("Video", "Antialias", 0) == 4);

    GameConfig saved;
    saved.Set("Video", "AspectRatio", "auto");
    GameSettings locked(saved, roots);
    const auto lockedAspect = Find(locked, "Aspect ratio:");
    const auto lockedPreset = Find(locked, "Preset:");
    const auto lockedAntialias = Find(locked, "Anti-aliasing:");
    GameConfig overrides;
    overrides.Set("Video", "AspectRatio", "4:3");
    locked.SetOverrides(overrides);
    assert(!locked.IsEnabled(lockedAspect));
    assert(locked.IsEnabled(lockedPreset));
    assert(locked.IsEnabled(lockedAntialias));
    assert(locked.Label(lockedAspect) == "Aspect ratio: 4:3");
    assert(!locked.Step(lockedAspect, 1));
    assert(locked.AdjustAndSave(lockedAspect, 1, directory) == GameSettings::Adjustment::Unchanged);
    assert(saved.Get("Video", "AspectRatio") == "auto");
    assert(coastline::ApplyGraphicsPreset(overrides, "n64"));
    overrides.Set("Video", "AspectRatio", "21:9");
    locked.SetOverrides(overrides);
    assert(!locked.IsEnabled(lockedPreset));
    assert(!locked.IsEnabled(lockedAntialias));
    assert(!locked.IsEnabled(Find(locked, "Internal resolution:")));
    assert(!locked.IsEnabled(Find(locked, "Screen shader:")));
    assert(!locked.IsEnabled(Find(locked, "Visual effects:")));
    assert(!locked.IsEnabled(Find(locked, "Music:")));
    assert(locked.IsEnabled(Find(locked, "GPU preference:")));
    assert(locked.Label(lockedPreset) == "Preset: N64");
    assert(locked.Label(lockedAspect) == "Aspect ratio: 21:9");
    assert(!locked.Step(lockedPreset, 1));
    assert(!saved.Get("Video", "GraphicsPreset"));
    assert(coastline::ApplyGraphicsPreset(overrides, "br"));
    locked.SetOverrides(overrides);
    const auto lockedMusic = Find(locked, "Music:");
    assert(!locked.IsEnabled(lockedMusic));
    assert(locked.Label(lockedMusic) == "Music: PC");
    assert(!locked.Step(lockedMusic, 1));
    assert(!saved.Get("Game", "Music"));
    locked.SetOverrides(GameConfig{});
    assert(locked.IsEnabled(lockedMusic));
    assert(locked.IsEnabled(lockedPreset) && locked.IsEnabled(lockedAspect));
    assert(locked.Label(lockedAspect) == "Aspect ratio: Fill screen");
    assert(!locked.IsEnabled(locked.Count()));
    std::filesystem::remove_all(directory);
}
