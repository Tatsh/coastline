#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

namespace coastline {

/**
 * Write a 4-bit colour-indexed image of the console as an indexed PNG image.
 *
 * @param path The PNG image to write.
 * @param texels The texels, two to a byte with the first in the high four bits, row by row.
 * @param palette The 16 colours, each a big-endian RGBA5551 half.
 * @param width The width in texels, even.
 * @param height The height in texels.
 * @return Whether the image was written and reads back with the same texels and palette.
 */
bool SaveIndexed4Png(const std::filesystem::path &path,
                     std::span<const std::uint8_t> texels,
                     std::span<const std::uint8_t, 32> palette,
                     int width,
                     int height);

/**
 * Read an indexed PNG image as the 4-bit texels and the RGBA5551 palette of the console.
 *
 * @param path The PNG image.
 * @param texels Receives the texels, two to a byte with the first in the high four bits.
 * @param palette Receives the 16 colours, each a big-endian RGBA5551 half.
 * @return Whether the image is indexed with at most 16 colours and its texels fill @p texels.
 */
bool LoadIndexed4Png(const std::filesystem::path &path,
                     std::span<std::uint8_t> texels,
                     std::span<std::uint8_t, 32> palette);

/**
 * Write 8-bit intensity texels as a PNG image, and read it back to check it.
 *
 * @param path The image to write.
 * @param texels The texels, a byte each, row by row.
 * @param width The width in texels.
 * @param height The height in texels.
 * @return Whether the image was written and reads back as the same texels.
 */
bool SaveIntensity8Png(const std::filesystem::path &path,
                       std::span<const std::uint8_t> texels,
                       int width,
                       int height);

/**
 * Write RGBA 5551 texels as a PNG image of 8 bits a channel, and read it back to check it.
 *
 * @param path The image to write.
 * @param texels The texels, two big-endian bytes each, row by row.
 * @param width The width in texels.
 * @param height The height in texels.
 * @return Whether the image was written and reads back as the same texels.
 */
bool SaveRgba5551Png(const std::filesystem::path &path,
                     std::span<const std::uint8_t> texels,
                     int width,
                     int height);

} // namespace coastline
