#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <span>
#include <string>
#include <string_view>

#include "language.h"

namespace coastline {

/**
 * Read the string resources of a Windows library.
 *
 * @param image The bytes of the library file.
 * @return The strings of the RT_STRING resources as UTF-8, by identifier. A library that is not a
 * PE image or has no string resources gives none.
 */
std::map<unsigned, std::string> ReadLibraryStrings(std::span<const std::uint8_t> image);

/**
 * Append a code point to UTF-8 text.
 *
 * @param text The UTF-8 text.
 * @param c The code point.
 */
void AppendUtf8(std::string &text, char32_t c);

/**
 * Decode the code point of UTF-8 text at a position, and step past it. A sequence cut short by the
 * end of the text decodes from the bytes it has.
 *
 * @param text The UTF-8 text.
 * @param position The byte of the code point, moved to the byte after it.
 * @return The code point.
 */
char32_t NextCodePoint(std::string_view text, std::size_t &position);

/**
 * Read the strings of a language and of en-us, each a JSON object of strings by key, from
 * strings/LANGUAGE.json and strings/en-us.json under @p assets, for the race and the menus. A key
 * the language lacks takes its en-us string.
 *
 */
class GameText {
public:
    /**
     * Load strings and register the exclusive active catalogue with SDL.
     *
     * @param assets The prepared asset directory with strings/LANGUAGE.json and strings/en-us.json.
     * @param language The requested language; missing keys use en-us strings.
     */
    GameText(const std::filesystem::path &assets, std::string language);
    /** Unregister the active catalogue and invalidate references to its strings. */
    ~GameText();
    /** Prevent copying the registered catalogue. */
    GameText(const GameText &) = delete;
    /** Prevent assignment of the registered catalogue. */
    GameText &operator=(const GameText &) = delete;

    /** @return The active text catalogue, or null outside its lifetime. */
    static GameText *Active();
    /** @return The selected language. */
    const std::string &Language() const {
        return m_language;
    }
    /** @return Whether the catalogue is empty. */
    bool Empty() const {
        return m_strings.empty();
    }
    /**
     * @param key The catalogue key to query without logging a missing translation.
     * @return The stored string, valid for the catalogue's lifetime, or null when unavailable.
     */
    const std::string *Find(std::string_view key) const;
    /**
     * @param key The catalogue key, whose storage must outlast a returned fallback view.
     * @return The stored string or @p key, logging a missing key once per catalogue.
     */
    std::string_view Resolve(std::string_view key);

private:
    std::string m_language;
    StringTable m_strings;
    std::set<std::string, std::less<>> m_logged;
};

/**
 * @param key The key of a string, such as 1 or n64.carSelect.rumblePakNote.
 * @return The active catalogue's string in UTF-8, or nothing.
 */
const std::string *FindString(std::string_view key);

/**
 * @param key The key of a string, such as n64.carSelect.rumblePakNote.
 * @return The active catalogue's string in UTF-8, the string of the selected language or else of
 * en-us, or @p key when neither file has it, logged the first time.
 */
std::string_view GameString(std::string_view key);

} // namespace coastline
