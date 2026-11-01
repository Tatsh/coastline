#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include "config.h"

namespace coastline {

/** Persistent game choices for the options menu. */
class GameSettings {
public:
    enum class Adjustment { Unchanged, Saved, SaveFailed };

    GameSettings(GameConfig &config, std::span<const std::filesystem::path> dataRoots = {});

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
};

} // namespace coastline
