#include "gamesettings.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <tuple>
#include <utility>

#include "language.h"

namespace coastline {

GameSettings::GameSettings(GameConfig &config,
                           std::span<const std::filesystem::path> dataRoots,
                           std::span<const std::array<int, 2>> displayResolutions)
    : m_config(config) {
    std::set<std::string> packs{"default"};
    const auto availableRegions = AvailableCartridgeRegions(dataRoots);
    std::set<std::string> regions(availableRegions.begin(), availableRegions.end());
    for (const auto &root : dataRoots) {
        std::error_code error;
        for (std::filesystem::directory_iterator entry(root / "assets", error), end;
             !error && entry != end;
             entry.increment(error)) {
            if (!entry->is_directory(error)) {
                continue;
            }
            const auto filename = entry->path().filename().u8string();
            const std::string id(filename.begin(), filename.end());
            if (id != "usa" && id != "eur" && id != "jpn") {
                packs.insert(id);
            }
        }
    }
    packs.insert(config.Get("Game", "AssetPack").value_or("default"));
    const auto currentRegion = config.Get("Game", "Region").value_or("usa");
    if (currentRegion == "usa" || currentRegion == "eur" || currentRegion == "jpn") {
        regions.insert(currentRegion);
    }
    if (regions.empty()) {
        regions.insert("usa");
    }
    Option pack{"Asset pack", "Game", "AssetPack", {{"default", "PC + branded images"}}};
    for (const auto &id : packs) {
        if (id != "default") {
            pack.choices.push_back({id, id == "boss" ? "Pure PC" : id});
        }
    }
    m_options.push_back(std::move(pack));
    Option region{"Models and textures", "Game", "Region", {}};
    for (const auto &id : regions) {
        if (id == currentRegion) {
            region.fallback = region.choices.size();
        }
        region.choices.push_back({id, id == "jpn" ? "JPN" : id == "eur" ? "EUR" : "USA"});
    }
    m_options.push_back(std::move(region));
    m_options.push_back({"Music", "Game", "Music", {{"XM", "N64"}, {"CD", "PC (CD)"}}});
    m_options.push_back({"Sound effects", "Game", "SoundEffects", {{"PC", "PC"}, {"N64", "N64"}}});
    m_options.push_back({"Visual effects", "Game", "Effects", {{"PC", "PC"}, {"N64", "N64"}}});
    m_options.push_back({"Anti-aliasing",
                         "Video",
                         "Antialias",
                         {{"1", "Off"}, {"2", "2x"}, {"4", "4x"}, {"8", "8x"}},
                         0,
                         true});
    m_options.push_back({"Anisotropic filtering",
                         "Video",
                         "AnisotropicFiltering",
                         {{"0", "Off"}, {"2", "2x"}, {"4", "4x"}, {"8", "8x"}, {"16", "16x"}},
                         0,
                         true});
    m_options.push_back({"Texture filter",
                         "Video",
                         "TextureFilter",
                         {{"modern", "Modern"}, {"n64", "N64"}, {"nearest", "Nearest"}}});
    m_options.push_back(
        {"Field of view",
         "Video",
         "FieldOfView",
         {{"0.75", "75%"}, {"1", "100%"}, {"1.25", "125%"}, {"1.5", "150%"}, {"2", "200%"}},
         1,
         true});
    Option resolution{"Internal resolution", "Video", "InternalWidth", {{"0", "Window size"}}};
    constexpr std::array standardResolutions{
        std::array{320, 240},   std::array{400, 300},   std::array{512, 384},
        std::array{640, 360},   std::array{640, 480},   std::array{720, 480},
        std::array{720, 576},   std::array{800, 600},   std::array{960, 540},
        std::array{1024, 576},  std::array{1024, 768},  std::array{1152, 864},
        std::array{1280, 720},  std::array{1280, 768},  std::array{1280, 800},
        std::array{1280, 960},  std::array{1280, 1024}, std::array{1366, 768},
        std::array{1440, 900},  std::array{1600, 900},  std::array{1600, 1200},
        std::array{1680, 1050}, std::array{1920, 1080}, std::array{1920, 1200},
        std::array{2048, 1536}, std::array{2560, 1080}, std::array{2560, 1440},
        std::array{2560, 1600}, std::array{3440, 1440}, std::array{3840, 1600},
        std::array{3840, 2160}, std::array{4096, 2160}, std::array{5120, 1440},
        std::array{5120, 2880}, std::array{6016, 3384}, std::array{7680, 2160},
        std::array{7680, 4320}, std::array{8192, 4320}, std::array{15360, 8640}};
    std::set<std::array<int, 2>> sizes{{320, 240}};
    for (const auto &size : standardResolutions) {
        if (std::ranges::any_of(displayResolutions, [&](const auto &mode) {
                return size[0] <= mode[0] && size[1] <= mode[1];
            })) {
            sizes.insert(size);
        }
    }
    for (const auto &size : displayResolutions) {
        if (size[0] >= 320 && size[1] >= 240 && size[0] <= 16384 && size[1] <= 16384) {
            sizes.insert(size);
        }
    }
    std::vector<std::array<int, 2>> sorted(sizes.begin(), sizes.end());
    std::ranges::sort(sorted, [](const auto &a, const auto &b) {
        return std::tuple{static_cast<long long>(a[0]) * a[1], a[0], a[1]} <
               std::tuple{static_cast<long long>(b[0]) * b[1], b[0], b[1]};
    });
    for (const auto &size : sorted) {
        const auto width = std::to_string(size[0]);
        resolution.choices.push_back({width, width + "x" + std::to_string(size[1]), size[1]});
    }
    m_options.push_back(std::move(resolution));
    m_options.push_back(
        {"Post anti-aliasing", "Video", "AntialiasPost", {{"none", "Off"}, {"fxaa", "FXAA"}}});
    m_options.push_back({.label = "Auto-save race progress",
                         .section = "Game",
                         .key = "AutoSave",
                         .choices = {{"0", "Off"}, {"1", "On"}},
                         .fallback = 1,
                         .boolean = true,
                         .restart = false});
    m_options.push_back({"Draw distance",
                         "Video",
                         "DrawDistance",
                         {{"1", "1x"}, {"1.5", "1.5x"}, {"2", "2x"}, {"3", "3x"}, {"4", "4x"}},
                         0,
                         true});
    Option title{"Title screen", "Game", "TitleScreen", {}};
    const auto storedTitle = config.Get("Game", "TitleScreen");
    if (regions.contains("usa") || regions.contains("eur")) {
        title.choices.push_back({"international", "USA/Europe"});
    } else if (storedTitle == "international") {
        title.choices.push_back({"international", "USA/Europe missing"});
    }
    if (regions.contains("jpn")) {
        if (currentRegion == "jpn") {
            title.fallback = title.choices.size();
        }
        title.choices.push_back({"japanese", "Japan"});
    } else if (storedTitle == "japanese") {
        title.choices.push_back({"japanese", "Japan missing"});
    }
    m_options.push_back(std::move(title));
    m_options.push_back(
        {"Upscaling",
         "Video",
         "ScaleFilter",
         {{"linear", "Bilinear"}, {"nearest", "Off"}, {"edge-adaptive", "Edge adaptive"}}});
    m_options.push_back({"Screen shader",
                         "Video",
                         "ScreenShader",
                         {{"none", "Off"},
                          {"consumer-crt", "Consumer CRT"},
                          {"pvm", "PVM"},
                          {"aperture-grille", "Aperture grille"}}});
    m_options.push_back({"Aspect ratio",
                         "Video",
                         "AspectRatio",
                         {{"auto", "Fill screen"},
                          {"4:3", "4:3"},
                          {"16:9", "16:9"},
                          {"21:9", "21:9"},
                          {"32:9", "32:9"}}});
    for (auto &option : m_options) {
        if (option.key == "Music" || option.key == "Effects" || option.key == "FieldOfView" ||
            option.key == "DrawDistance" || option.key == "InternalWidth" ||
            option.key == "AntialiasPost" || option.key == "ScaleFilter" ||
            option.key == "ScreenShader" || option.key == "AspectRatio") {
            option.restart = false;
        }
    }
}

std::size_t GameSettings::Selected(const Option &option) const {
    if (option.key == "InternalWidth") {
        const auto width = m_config.GetInteger("Video", "InternalWidth", 0);
        const auto height = m_config.GetInteger("Video", "InternalHeight", 0);
        for (std::size_t index = 0; index < option.choices.size(); ++index) {
            const auto &choice = option.choices[index];
            if (width == std::stoi(choice.value) && height == choice.height) {
                return index;
            }
        }
        return option.fallback;
    }
    const auto stored = m_config.Get(option.section, option.key);
    if (!stored) {
        return option.fallback;
    }
    if (option.boolean) {
        const bool value = m_config.GetBoolean(
            option.section, option.key, option.choices[option.fallback].value == "1");
        for (std::size_t index = 0; index < option.choices.size(); ++index) {
            if ((option.choices[index].value == "1") == value) {
                return index;
            }
        }
        return option.fallback;
    }
    const float number =
        m_config.GetFloat(option.section, option.key, std::numeric_limits<float>::quiet_NaN());
    for (std::size_t index = 0; index < option.choices.size(); ++index) {
        const auto &choice = option.choices[index];
        if (option.numeric ? std::abs(number - std::stof(choice.value)) < 0.00001f :
                             *stored == choice.value) {
            return index;
        }
    }
    return option.fallback;
}

std::string GameSettings::Label(std::size_t index) const {
    if (index >= m_options.size()) {
        return {};
    }
    const auto &option = m_options[index];
    const auto &choice = option.choices[Selected(option)];
    if (option.key == "InternalWidth" && Selected(option) == option.fallback) {
        const auto width = m_config.GetInteger("Video", "InternalWidth", 0);
        const auto height = m_config.GetInteger("Video", "InternalHeight", 0);
        if (width > 0 && height > 0) {
            return option.label + ": " + std::to_string(width) + "x" + std::to_string(height);
        }
    }
    if (option.numeric) {
        const float number =
            m_config.GetFloat(option.section, option.key, std::numeric_limits<float>::quiet_NaN());
        if (std::isfinite(number) && std::abs(number - std::stof(choice.value)) >= 0.00001f) {
            return option.label + ": Custom (" + *m_config.Get(option.section, option.key) + ")";
        }
    }
    return option.label + ": " + choice.label;
}

bool GameSettings::RequiresRestart(std::size_t index) const {
    return index < m_options.size() && m_options[index].restart;
}

bool GameSettings::Step(std::size_t index, int direction) {
    if (index >= m_options.size() || direction == 0) {
        return false;
    }
    const auto &option = m_options[index];
    if (option.choices.size() < 2) {
        return false;
    }
    auto selected = Selected(option);
    selected = (selected + (direction > 0 ? 1 : option.choices.size() - 1)) % option.choices.size();
    m_config.Set(option.section, option.key, option.choices[selected].value);
    if (option.key == "InternalWidth") {
        m_config.Set("Video", "InternalHeight", std::to_string(option.choices[selected].height));
        m_config.Set("Video", "RenderScale", "1");
    }
    return true;
}

GameSettings::Adjustment
GameSettings::AdjustAndSave(std::size_t index, int direction, const std::filesystem::path &path) {
    GameConfig original = m_config;
    if (!Step(index, direction)) {
        return Adjustment::Unchanged;
    }
    if (m_config.Save(path)) {
        return Adjustment::Saved;
    }
    m_config = std::move(original);
    return Adjustment::SaveFailed;
}

} // namespace coastline
