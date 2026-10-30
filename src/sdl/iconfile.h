#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace coastline {

/** One image of an icon file. */
struct IconEntry {
    std::uint32_t bitCount;              /*!< The bits per pixel of the image. */
    std::span<const std::uint8_t> image; /*!< A device-independent bitmap or a PNG image. */
};

/** An image of an icon as 8-bit RGBA texels. */
struct IconPixels {
    int width = 0;                  /*!< The width in pixels. */
    int height = 0;                 /*!< The height in pixels. */
    std::vector<std::uint8_t> rgba; /*!< RGBA bytes, row by row from the top. */
};

/** The size of an image of the icon of the game, from the name of its PNG file. */
struct IconSize {
    int width = 0;    /*!< The width in pixels. */
    int height = 0;   /*!< The height in pixels. */
    int bitCount = 0; /*!< The bits per pixel of the image in the icon file. */

    bool operator==(const IconSize &) const = default;
};

/**
 * Read the size of an image of the icon from the name the import gives its PNG file.
 *
 * @param name The file name, such as 32x32x4.png.
 * @return The size, or nothing for any other name.
 */
std::optional<IconSize> ParseIconName(std::string_view name);

/**
 * Give the name of the PNG file of an image of the icon.
 *
 * @param size The size of the image.
 * @return The name, such as 32x32x4.png.
 */
std::string IconName(const IconSize &size);

/**
 * List the images of an icon file. An entry that records 0 bits per pixel takes the bits per pixel
 * of its bitmap, or 32 for a PNG image.
 *
 * @param icon The bytes of the icon file, which must outlive the entries.
 * @return The images in the order of the directory, or none when the file is not an icon file or
 * an image lies outside it.
 */
std::vector<IconEntry> ListIconImages(std::span<const std::uint8_t> icon);

/**
 * Decode an icon image that is a device-independent bitmap: the colours, then the AND mask that
 * makes a pixel transparent. A 32-bit image with any alpha takes its alpha in place of the mask.
 *
 * @param image The bitmap, from its BITMAPINFOHEADER, with the height of the colours and the mask
 * together.
 * @return The pixels, or nothing when the bitmap is compressed, is not 1, 4, 8, 24, or 32 bits a
 * pixel, or is cut short.
 */
std::optional<IconPixels> DecodeIconBitmap(std::span<const std::uint8_t> image);

} // namespace coastline
