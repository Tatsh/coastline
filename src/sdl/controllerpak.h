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

/** The note of the settings of the options menu, stored with the settings of the game. */
inline constexpr std::string_view PAK_SETTINGS_NOTE = "TOP GEAR OPTIONS";

/** The notes of a Controller Pak, stored in a directory, with the settings note in another. */
class ControllerPak {
public:
    /**
     * @param directory The directory of the notes, created at the first write.
     * @param settingsDirectory The directory of PAK_SETTINGS_NOTE, or empty for the directory of
     * the other notes.
     */
    explicit ControllerPak(std::filesystem::path directory,
                           std::filesystem::path settingsDirectory = {});

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
    // The directory a note of the name is stored in.
    const std::filesystem::path &DirectoryOf(const std::string &name) const;

    std::filesystem::path m_directory;
    std::filesystem::path m_settingsDirectory;
};

/** The bytes of the image of a Controller Pak, as emulators and the tools of a pak store one. */
inline constexpr std::size_t PAK_IMAGE_BYTES = 0x8000;

/** The release of the cartridge whose game code and company code the notes of an image take. */
enum class PakRelease {
    Usa, /*!< NGRE and 5D. */
    Eur, /*!< NTRP and 7F. */
    Jpn, /*!< NTRJ and 28. */
};

/**
 * Write the notes of a pak as the image of a formatted Controller Pak, with the game code and the
 * company code of a release on each note. An image of four paks of 0x20000 bytes, as mupen64plus
 * writes one, takes the notes in its first pak, and its other paks are retained. Another file is
 * replaced by an image of one pak.
 *
 * @param pak The pak of the notes.
 * @param file The image.
 * @param release The release whose cartridge reads the notes.
 * @return Nothing once the image is written, or the reason it was not.
 */
std::optional<std::string> ExportPakImage(const ControllerPak &pak,
                                          const std::filesystem::path &file,
                                          PakRelease release = PakRelease::Usa);

/**
 * Copy the notes of any release of the game from the image of a Controller Pak, or from the first
 * pak of an image of four, to a pak. A note the pak has already is replaced and the replacement is
 * logged. The notes of other games are skipped.
 *
 * @param pak The pak the notes are written to.
 * @param file The image.
 * @return Nothing once the notes are written, or the reason they were not.
 */
std::optional<std::string> ImportPakImage(const ControllerPak &pak,
                                          const std::filesystem::path &file);

} // namespace coastline
