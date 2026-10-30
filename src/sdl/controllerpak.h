#pragma once

/**
 * @file
 * The Controller Pak of the first controller of the console as a directory of the game, a file for
 * each note, named after the note.
 */

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace coastline {

/** The bytes of a page of a Controller Pak, the unit a note takes its room in. */
inline constexpr std::size_t PAK_PAGE_BYTES = 0x100;

/** The pages of a Controller Pak free for notes. */
inline constexpr std::size_t PAK_PAGES = 123;

/** The notes a Controller Pak has room for. */
inline constexpr std::size_t PAK_NOTES = 16;

/** A note of a Controller Pak. */
struct PakNote {
    std::string name;  /*!< The name of the note, such as TOP GEAR DECALS. */
    std::size_t bytes; /*!< The size of the note in bytes. */
};

/** The notes of a Controller Pak, stored in a directory. */
class ControllerPak {
public:
    /**
     * @param directory The directory of the notes, created at the first write.
     */
    explicit ControllerPak(std::filesystem::path directory);

    /**
     * @param name The name of the note.
     * @return The bytes of the note, or nothing when the pak does not have it.
     */
    std::optional<std::vector<std::uint8_t>> ReadNote(const std::string &name) const;

    /**
     * Write a note whole, creating it when the pak does not have it.
     *
     * @param name The name of the note.
     * @param bytes The bytes of the note.
     * @return Whether the note was written.
     */
    bool WriteNote(const std::string &name, std::span<const std::uint8_t> bytes) const;

    /**
     * @param name The name of the note.
     * @return Whether the note was deleted.
     */
    bool DeleteNote(const std::string &name) const;

    /** @return The notes of the pak, in the order of their names. */
    std::vector<PakNote> Notes() const;

    /** @return The bytes of the pages no note takes, as osPfsFreeBlocks() counts them. */
    std::size_t FreeBytes() const;

private:
    std::filesystem::path m_directory;
};

} // namespace coastline
