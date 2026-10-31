#pragma once

/**
 * @file
 * The keys of the text of the game: the messages of the Nintendo 64 releases with the addresses of
 * each release the import reads them from, and the key of each string of BRString.dll. A message
 * that the PC and the Nintendo 64 releases draw alike has one key in both tables.
 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace coastline {

/**
 * A message of the Nintendo 64 releases. A release draws a message as one or more strings of its
 * main segment, one line each, and the import joins the lines with a newline.
 */
struct ConsoleString {
    std::string_view key;           /*!< The key of the message in the files of strings. */
    std::vector<std::uint32_t> usa; /*!< The address of each line in the USA release. */
    std::vector<std::uint32_t> eur; /*!< The address of each line in the European release. */
    std::vector<std::uint32_t> jpn; /*!< The address of each line in the Japanese release. */
};

/**
 * The messages the Japanese release draws in Japanese and the messages the other releases draw in
 * their place, then the messages of the race that a string of BRString.dll draws in the PC release,
 * paired across the releases by the code that draws them. A message without lines in a release is
 * absent from that release.
 */
extern const std::array<ConsoleString, 147> CONSOLE_STRINGS;

/** A message of the menus in English that the game words differently from the console. */
struct SaveFolderString {
    std::string_view key;  /*!< The key of the message in the files of strings. */
    std::string_view text; /*!< The text, with a newline between lines. */
};

/**
 * The messages of the console that name the Controller Pak where the game saves to a folder, as
 * the import writes them to strings/en-us.json with the text of the USA cartridge. Every other
 * language takes them from there.
 */
extern const std::array<SaveFolderString, 10> SAVE_FOLDER_STRINGS;

/** The number of strings of BRString.dll, numbered from 1. */
inline constexpr std::size_t LIBRARY_STRING_COUNT = 303;

/**
 * The key of each string of BRString.dll, by its number less 1. The import writes the string under
 * the key, and GetStringResource() of the race reads it from there. A key of the race (race.) is
 * also in CONSOLE_STRINGS, where the PC and the Nintendo 64 releases draw the same text in the
 * same place, and every other key (pc.) is the PC release's alone.
 */
extern const std::array<std::string_view, LIBRARY_STRING_COUNT> LIBRARY_STRING_KEYS;

} // namespace coastline
