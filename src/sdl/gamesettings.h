#pragma once

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
    /** End preset browsing; the next preset adjustment captures a new baseline. */
    void FinishPresetSelection();
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
    /** A stored value and its label in an options menu. */
    struct Choice {
        std::string value; /*!< The value written to the INI file. */
        std::string label; /*!< The label displayed in the menu. */
        int height = 0;    /*!< The menu row height override. */
    };
    /** An option backed by a configuration key and a list of choices. */
    struct Option {
        std::string label;           /*!< The option label. */
        std::string section;         /*!< The INI section. */
        std::string key;             /*!< The INI key. */
        std::vector<Choice> choices; /*!< The available values. */
        std::size_t fallback = 0;    /*!< The choice used for invalid values. */
        bool numeric = false;        /*!< Whether the value is an integer. */
        bool boolean = false;        /*!< Whether the value is a boolean. */
        bool restart = true;         /*!< Whether applying the option restarts the session. */
    };
    std::size_t Selected(const Option &option) const;
    const GameConfig &ConfigFor(const Option &option) const;
    GameConfig &m_config;
    GameConfig m_overrides;
    std::optional<GameConfig> m_presetBaseline;
    std::vector<Option> m_options;
    bool m_presetRestart = true;
};

} // namespace coastline
