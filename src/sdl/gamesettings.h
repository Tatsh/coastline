#pragma once

#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "config.h"

namespace coastline {

/** Apply a graphics preset without changing audio, language, region, or aspect ratio. */
bool ApplyGraphicsPreset(GameConfig &config, std::string_view preset);

/** Persistent game choices for the options menu. */
class GameSettings {
public:
    enum class Adjustment { Unchanged, Saved, SaveFailed };

    GameSettings(GameConfig &config,
                 std::span<const std::filesystem::path> dataRoots = {},
                 std::span<const std::array<int, 2>> displayResolutions = {});

    std::size_t Count() const {
        return m_options.size();
    }
    std::string Label(std::size_t index) const;
    bool RequiresRestart(std::size_t index) const;
    /** Cycle a setting in either direction. Invalid indices and zero steps are ignored. */
    bool Step(std::size_t index, int direction);
    /** Save an adjustment immediately; restore in-memory configuration if writing fails. */
    Adjustment AdjustAndSave(std::size_t index, int direction, const std::filesystem::path &path);

private:
    struct Choice {
        std::string value;
        std::string label;
        int height = 0;
    };
    struct Option {
        std::string label;
        std::string section;
        std::string key;
        std::vector<Choice> choices;
        std::size_t fallback = 0;
        bool numeric = false;
        bool boolean = false;
        bool restart = true;
    };
    std::size_t Selected(const Option &option) const;
    GameConfig &m_config;
    std::vector<Option> m_options;
    bool m_presetRestart = true;
};

} // namespace coastline
