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

/**
 * Check a cartridge image and copy it, big-endian, to roms/usa.z64, jpn.z64, or eur.z64 under
 * @p dataDirectory.
 *
 * @param image The cartridge image.
 * @param dataDirectory The data directory of the game.
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
 * Copy the game data of the Boss Rally disc to @p dataDirectory, and its CD audio tracks to
 * music/trackNN.flac. An ISO image has no audio tracks, and only the data is copied. The
 * installers, DirectX, and the Windows programs are skipped.
 *
 * @param disc A CUE sheet or an ISO image.
 * @param dataDirectory The data directory of the game.
 * @param progress Called as each file and track is written.
 * @return The number of files written, or a message.
 */
std::expected<std::size_t, std::string> ImportDisc(const std::filesystem::path &disc,
                                                   const std::filesystem::path &dataDirectory,
                                                   const ImportProgress &progress);

} // namespace coastline
