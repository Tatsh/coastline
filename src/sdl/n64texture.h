#pragma once

/**
 * @file
 * The texels of the Nintendo 64 as 32-bit pixels, for the texture packs of the SDL game.
 */

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace coastline {

/** The formats of the texels of the console, as G_IM_FMT_* and G_IM_SIZ_* pair them. */
enum class TexelFormat : std::uint8_t {
    Intensity4,       /*!< 4-bit intensity, which is also the alpha. */
    Intensity8,       /*!< 8-bit intensity, which is also the alpha. */
    IntensityAlpha4,  /*!< 3-bit intensity and 1-bit alpha. */
    IntensityAlpha8,  /*!< 4-bit intensity and 4-bit alpha. */
    IntensityAlpha16, /*!< 8-bit intensity and 8-bit alpha. */
    Rgba16,           /*!< 5-bit red, green, and blue, and 1-bit alpha. */
    Rgba32,           /*!< 8-bit red, green, blue, and alpha. */
};

/**
 * @param format A G_IM_FMT_* value.
 * @param size A G_IM_SIZ_* value.
 * @return The format of the pair, or Rgba16 for a pair the front end does not use.
 */
TexelFormat TexelFormatOf(int format, int size);

/**
 * @param format The format.
 * @return The bits of a texel.
 */
int BitsPerTexel(TexelFormat format);

/**
 * Decode texels of the console, stored in rows from the top, into pixels in the byte order R, G, B,
 * A.
 *
 * @param texels The big-endian texels.
 * @param width The width in texels.
 * @param height The height in texels.
 * @param format The format of the texels.
 * @return The pixels, or fewer when the texels end early.
 */
std::vector<std::uint8_t>
DecodeTexels(std::span<const std::uint8_t> texels, int width, int height, TexelFormat format);

/**
 * Write pixels in the byte order R, G, B, A to a PNG file.
 *
 * @param path The file.
 * @param pixels The pixels.
 * @param width The width.
 * @param height The height.
 * @return Whether the file was written.
 */
bool WritePng(const std::filesystem::path &path,
              std::span<const std::uint8_t> pixels,
              int width,
              int height);

} // namespace coastline
