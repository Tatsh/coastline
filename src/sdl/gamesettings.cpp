#include "gamesettings.h"

#include <cmath>
#include <limits>
#include <set>
#include <utility>

namespace coastline {

GameSettings::GameSettings(GameConfig &config, std::span<const std::filesystem::path> dataRoots)
    : m_config(config) {
    std::set<std::string> packs{"default"};
    std::set<std::string> regions;
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
            const auto region = GameConfig::Load(entry->path() / "n64/base/cartridge.ini")
                                    .Get("Cartridge", "Region");
            if (region && (*region == "usa" || *region == "eur" || *region == "jpn")) {
                regions.insert(*region);
            }
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
    m_options.push_back({"Internal resolution",
                         "Video",
                         "RenderScale",
                         {{"0.5", "50%"},
                          {"0.67", "67%"},
                          {"0.75", "75%"},
                          {"1", "100%"},
                          {"1.5", "150%"},
                          {"2", "200%"}},
                         3,
                         true});
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
}

std::size_t GameSettings::Selected(const Option &option) const {
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
    if (option.key == "RenderScale") {
        m_config.Set("Video", "InternalWidth", "0");
        m_config.Set("Video", "InternalHeight", "0");
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
