#pragma once

/**
 * @file
 * The settings of the options menu, graphics presets, and the rules that resolve speed units
 * and the volume of sound effects.
 */

#include <array>
#include <filesystem>
#include <optional>
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
 * N64 and BR select N64 and PC window reflections, respectively.
 * N64 and BR select 4:3; Ultra fills the screen. Other presets preserve the aspect ratio.
 *
 * @param config The configuration to update in memory.
 * @param preset custom, n64, br, ultra, worst, or low-power.
 * @return Whether the identifier selects a recognised preset.
 */
bool ApplyGraphicsPreset(GameConfig &config, std::string_view preset);

/**
 * Resolve speed units without recording an automatic choice as a user preference.
 * Game.Units values mph and kph override region and locale. Otherwise, explicit eur and jpn
 * regions use KPH and usa uses MPH. Without a region, MPH jurisdictions use MPH, other
 * countries use KPH, and an absent country defaults to MPH.
 *
 * @param config Saved settings; an absent or invalid Game.Units value selects automatic units.
 * @param region The explicit CLI region, or empty to use the system locale.
 * @param country The first available ISO 3166 country code in preferred locale order, or empty.
 * @return Whether speeds use miles per hour rather than kilometres per hour.
 */
bool SpeedInMph(const GameConfig &config, std::string_view region, std::string_view country);

/**
 * Resolve SFX volume from a manual preference or the selected sample source.
 *
 * @param config Saved settings. Game.SfxVolume accepts 0 through 10; other values are automatic.
 * @param n64 Whether N64 sound effects are selected.
 * @return The manual volume, or 10 for N64 samples and 4 for PC samples.
 */
int SoundEffectsVolume(const GameConfig &config, bool n64);

/** Persistent game choices for the options menu. */
class GameSettings {
public:
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

    /**
     * Count the options of the menu.
     *
     * @return The number of options available in this settings model.
     */
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
     * Report whether the user may change an option.
     *
     * @param index The zero-based option index.
     * @return Whether the option is valid and is not controlled by a command-line override.
     */
    bool IsEnabled(std::size_t index) const;
    /**
     * Format the menu text of an option from its saved or overridden value.
     *
     * @param index The zero-based option index.
     * @return The option label and its selected value, or empty text for an invalid index.
     */
    std::string Label(std::size_t index) const;
    /**
     * Report whether applying an option restarts the game at the logos.
     *
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
    /** End preset browsing; the next preset adjustment captures a new baseline. */
    void FinishPresetSelection();

private:
    // A stored value and its label in an options menu.
    struct Choice {
        std::string value; // Written to the INI file.
        std::string label; // Displayed in the menu.
        int height = 0;    // The menu row height override.
    };
    // An option backed by a configuration key and a list of choices.
    struct Option {
        std::string label;
        std::string section; // INI section.
        std::string key;     // INI key.
        std::vector<Choice> choices;
        std::size_t fallback = 0; // The choice used for invalid values.
        bool numeric = false;     // Whether the value is an integer.
        bool boolean = false;
        bool restart = true; // Whether applying the option restarts the session.
    };
    // Return the index of the choice that matches the current value of an option, or
    // Option::fallback when the value is absent or matches none. Numeric values compare within
    // 0.00001, and the internal resolution compares both its width and its height.
    std::size_t Selected(const Option &option) const;
    // Return m_overrides when it sets the key of the option, otherwise m_config.
    const GameConfig &ConfigFor(const Option &option) const;
    GameConfig &m_config;   // The saved configuration that Step() changes.
    GameConfig m_overrides; // The command-line values that lock their options.
    // The configuration before the first preset step since FinishPresetSelection(). Each
    // preset step applies to a copy of the baseline.
    std::optional<GameConfig> m_presetBaseline;
    std::vector<Option> m_options; // In menu order.
    // Whether the last preset step changed a setting that restarts the game, or the frame
    // pacing or GPU preference.
    bool m_presetRestart = true;
};

} // namespace coastline
