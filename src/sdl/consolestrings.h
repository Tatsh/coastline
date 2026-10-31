#pragma once

/**
 * @file
 * The text of the menus of the Nintendo 64 releases, by key, with the addresses of each release
 * the import reads it from.
 */

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace coastline {

/**
 * A message of the menus of the Nintendo 64 releases. A release draws a message as one or more
 * strings of its main segment, one line each, and the import joins the lines with a newline.
 */
struct ConsoleString {
    std::string_view key;           /*!< The key of the message in the files of strings. */
    std::vector<std::uint32_t> usa; /*!< The address of each line in the USA release. */
    std::vector<std::uint32_t> eur; /*!< The address of each line in the European release. */
    std::vector<std::uint32_t> jpn; /*!< The address of each line in the Japanese release. */
};

/**
 * The messages the Japanese release draws in Japanese, and the messages the other releases draw in
 * their place, paired across the releases by the code that draws them. A message without lines in
 * a release is absent from that release.
 */
extern const std::array<ConsoleString, 92> CONSOLE_STRINGS;

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

} // namespace coastline
