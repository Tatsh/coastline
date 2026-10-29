#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace coastline {

/** A Nintendo 64 release the game reads models, textures, and music from. */
enum class RomRelease {
    Usa, /*!< North America, NGRE. */
    Jpn, /*!< Japan, NTRJ. */
    Eur, /*!< Europe, NTRP. */
};

/** Reports the progress of an import: a description and a fraction from 0 to 1. */
using ImportProgress = std::function<void(const std::string &step, double fraction)>;

/**
 * Convert a cartridge image in any of the three byte orders (z64, v64, n64) to big-endian.
 *
 * @param image The image, converted in place.
 * @return Whether the first word identified a byte order.
 */
bool NormalizeRomByteOrder(std::span<std::uint8_t> image);

/**
 * Identify a big-endian cartridge image by its SHA-1.
 *
 * @param image The big-endian image.
 * @return The release, or a message when the image is not an unmodified release.
 */
std::expected<RomRelease, std::string> IdentifyRom(std::span<const std::uint8_t> image);

/** A music module of the USA cartridge and its name in a music pack. */
struct UsaMusicModule {
    std::uint32_t offset; /*!< Where the module starts on the cartridge. */
    const char *name;     /*!< The name of its file in a pack, without the extension .xm. */
};

/**
 * The music modules of the USA cartridge: the menus, then the modules of the tracks. The table of
 * the tracks also lists 0x19BC10 for track 10, a sound sample rather than a module.
 */
inline constexpr UsaMusicModule USA_MUSIC_MODULES[] = {{0x0EBC00, "menu"},
                                                       {0x113660, "race1"},
                                                       {0x12EAB0, "race2"},
                                                       {0x149C80, "race3"},
                                                       {0x164B60, "race4"},
                                                       {0x17FD10, "race5"}};

/**
 * Check a cartridge image and extract its main segment and its assets, inflated, to
 * n64/usa, n64/jpn, or n64/eur under @p dataDirectory. The USA cartridge also gives the pack its
 * XM modules in music.
 *
 * @param image The cartridge image.
 * @param dataDirectory The directory of the pack to write, assets/defaults of the data directory.
 * @return The release, or a message.
 */
std::expected<RomRelease, std::string> ImportRom(const std::filesystem::path &image,
                                                 const std::filesystem::path &dataDirectory);

/** One track of a CUE sheet, with its extent in sectors of 2,352 bytes in the image file. */
struct CueTrack {
    int number = 0;                /*!< The track number. */
    bool audio = false;            /*!< Whether the track is CD audio. */
    std::uint32_t sectorSize = 0;  /*!< The bytes per sector in the image file. */
    std::filesystem::path file;    /*!< The image file of the track. */
    std::uint64_t firstSector = 0; /*!< The sector of INDEX 01. */
    std::uint64_t sectorCount = 0; /*!< The sectors up to the next track or the end of the file. */
};

/**
 * Parse a CUE sheet. Track extents end at the INDEX 00 or INDEX 01 of the next track in the same
 * file, or at the end of the file.
 *
 * @param cue The CUE sheet.
 * @return The tracks, or a message.
 */
std::expected<std::vector<CueTrack>, std::string> ParseCueSheet(const std::filesystem::path &cue);

/**
 * Copy the files of the Boss Rally disc the game opens to @p dataDirectory, and its CD audio tracks
 * as music/trackNN.flac. An ISO image has no audio tracks, and only the files are copied.
 *
 * @param disc A CUE sheet or an ISO image.
 * @param dataDirectory The directory of the pack to write, assets/defaults of the data directory.
 * @param progress Called as each file and track is written.
 * @return The number of files written, or a message.
 */
std::expected<std::size_t, std::string> ImportDisc(const std::filesystem::path &disc,
                                                   const std::filesystem::path &dataDirectory,
                                                   const ImportProgress &progress);

} // namespace coastline
