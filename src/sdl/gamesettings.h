#pragma once

#include <array>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "config.h"

namespace coastline {

/**
 * Apply a graphics preset without changing language or region.
 * N64 selects console music and BR selects PC CD music; other presets preserve the music source.
 * N64 and BR select N64 and PC sound effects, respectively.
 * N64 and BR select 4:3; Ultra fills the screen. Other presets preserve the aspect ratio.
 *
 * @param config The configuration to update in memory.
 * @param preset custom, n64, br, ultra, worst, or low-power.
 * @return Whether the identifier selects a recognised preset.
 */
bool ApplyGraphicsPreset(GameConfig &config, std::string_view preset);

/** Persistent game choices for the options menu. */
class GameSettings {
public:
    /** The outcome of an immediate setting adjustment and persistence attempt. */
    enum class Adjustment {
        Unchanged, /*!< The request did not change the configuration. */
        Saved,     /*!< The changed configuration was written successfully. */
        SaveFailed /*!< Writing failed and the previous in-memory configuration was restored. */
    };

    /**
     * Build available options from the configuration, installed packs, and display modes.
     *
     * @param config The mutable configuration. Its lifetime must exceed the settings model's.
     * @param dataRoots The directories searched for selectable asset packs.
     * @param displayResolutions Available output widths and heights, in pixels.
     */
    GameSettings(GameConfig &config,
                 std::span<const std::filesystem::path> dataRoots = {},
                 std::span<const std::array<int, 2>> displayResolutions = {});

    /** @return The number of options available in this settings model. */
    std::size_t Count() const {
        return m_options.size();
    }
    /**
     * Replace temporary values supplied by command-line arguments without saving them.
     *
     * @param overrides Settings that take precedence over saved values and disable matching
     * options.
     */
    void SetOverrides(GameConfig overrides);
    /**
     * @param index The zero-based option index.
     * @return Whether the option is valid and is not controlled by a command-line override.
     */
    bool IsEnabled(std::size_t index) const;
    /**
     * @param index The zero-based option index.
     * @return The option label and its selected value, or empty text for an invalid index.
     */
    std::string Label(std::size_t index) const;
    /**
     * @param index The zero-based option index.
     * @return Whether the option requires rebuilding the game session, or false for an invalid
     * index. For graphics presets, the result reflects the most recent preset adjustment.
     */
    bool RequiresRestart(std::size_t index) const;
    /**
     * Cycle a setting in either direction. Invalid or locked options, zero steps, and options
     * with fewer than two choices are ignored.
     *
     * @param index The zero-based option index.
     * @param direction A negative value for the previous choice, positive for the next.
     * @return Whether the configuration changed.
     */
    bool Step(std::size_t index, int direction);
    /**
     * Save an adjustment immediately; restore in-memory configuration if writing fails.
     *
     * @param index The zero-based option index.
     * @param direction A negative value for the previous choice, positive for the next.
     * @param path The INI file to write.
     * @return Unchanged for an ignored request, Saved on success, or SaveFailed after rollback.
     */
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
    const GameConfig &ConfigFor(const Option &option) const;
    GameConfig &m_config;
    GameConfig m_overrides;
    std::vector<Option> m_options;
    bool m_presetRestart = true;
};

} // namespace coastline
