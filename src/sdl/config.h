#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace coastline {

/**
 * The settings of BossRally.ini. The PC release writes the sections Video and Game, and the SDL
 * game adds keys to the same sections. Section and key names compare without case, as
 * the Windows profile routines compare them.
 */
class GameConfig {
public:
    /**
     * Read @p path. A missing or unreadable file gives an empty configuration.
     *
     * @param path The INI file.
     * @return The settings the file has.
     */
    static GameConfig Load(const std::filesystem::path &path);

    /**
     * Parse INI section headers, Key=Value lines, and comments that start with ; or #.
     *
     * @param text The text of the file.
     * @return The settings the text has.
     */
    static GameConfig Parse(std::string_view text);

    /**
     * Write every setting to @p path.
     * Existing files are truncated; a failed write can produce a partial configuration.
     *
     * @param path The INI file.
     * @return Whether opening and writing the stream succeeds.
     */
    bool Save(const std::filesystem::path &path) const;

    /**
     * @param section The section name.
     * @param key The key name.
     * @return The value, or nothing when the key is absent.
     */
    std::optional<std::string> Get(std::string_view section, std::string_view key) const;

    /**
     * @param section The section name.
     * @param key The key name.
     * @param fallback The value for an absent or malformed key.
     * @return The value as an integer.
     */
    long GetInteger(std::string_view section, std::string_view key, long fallback) const;

    /**
     * @param section The section name.
     * @param key The key name.
     * @param count The number of choices.
     * @param fallback The value for an absent or malformed key, or a value outside the choices.
     * @return A choice index from 0 to @p count - 1, or @p fallback for invalid input.
     */
    int
    GetIndex(std::string_view section, std::string_view key, std::size_t count, int fallback) const;

    /**
     * @param section The section name.
     * @param key The key name.
     * @param fallback The value for an absent or malformed key.
     * @return The value as a number with a fraction.
     */
    float GetFloat(std::string_view section, std::string_view key, float fallback) const;

    /**
     * @param section The section name.
     * @param key The key name.
     * @param fallback The value for an absent or malformed key.
     * @return Whether the value is a non-zero integer, true, yes, or on, ignoring case.
     */
    bool GetBoolean(std::string_view section, std::string_view key, bool fallback) const;

    /**
     * Store a value, replacing any value of the same key.
     *
     * @param section The section name.
     * @param key The key name.
     * @param value The new value.
     */
    void Set(std::string_view section, std::string_view key, std::string value);

private:
    struct CaseInsensitiveLess {
        using is_transparent = void;
        bool operator()(std::string_view left, std::string_view right) const;
    };
    using Section = std::map<std::string, std::string, CaseInsensitiveLess>;
    std::map<std::string, Section, CaseInsensitiveLess> m_sections;
};

/**
 * Read a finite, positive aspect ratio or an automatic-sizing setting.
 *
 * @param text Empty text, auto (case-insensitive), a ratio such as 4:3, or a number such as 1.6.
 * @return Width divided by height, 0 for empty text or auto, or std::nullopt for invalid input.
 */
std::optional<float> ParseAspectRatio(std::string_view text);

/**
 * Expand a resolution to an aspect ratio, with minimum dimensions of 320x240.
 * Each dimension is limited to 16384 pixels, even when the requested ratio requires more.
 *
 * @param width The requested internal width, in pixels.
 * @param height The requested internal height, in pixels.
 * @param aspect The output width divided by height; invalid ratios only clamp the dimensions.
 * @return The expanded width and height, in pixels.
 */
std::array<int, 2> ExpandInternalResolution(int width, int height, float aspect);

} // namespace coastline
