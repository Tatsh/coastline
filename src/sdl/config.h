#pragma once

#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace coastline {

/**
 * The settings of BossRally.ini. The PC release writes the sections Video and Game, and the SDL
 * game adds its own keys to the same sections. Section and key names compare without case, as
 * the Windows profile routines compare them.
 */
class GameConfig {
public:
    /**
     * Read @p path. A missing file gives an empty configuration.
     *
     * @param path The INI file.
     * @return The settings the file has.
     */
    static GameConfig Load(const std::filesystem::path &path);

    /**
     * Parse INI text: [Section] lines, Key=Value lines, and comments that start with ; or #.
     *
     * @param text The text of the file.
     * @return The settings the text has.
     */
    static GameConfig Parse(std::string_view text);

    /**
     * Write every setting to @p path.
     *
     * @param path The INI file.
     * @return Whether the file was written.
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
     * @return The value as the index of a choice, from 0 to @p count - 1.
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
     * @param fallback The value for an absent key.
     * @return Whether the value is a non-zero number, true, yes, or on.
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
 * Read an aspect ratio as a setting gives it.
 *
 * @param text auto, a ratio of two numbers such as 4:3 or 16:9, or a number such as 1.6.
 * @return The width over the height, 0 for auto, or nothing when the text is none of these or the
 * ratio is not above 0.
 */
std::optional<float> ParseAspectRatio(std::string_view text);

} // namespace coastline
