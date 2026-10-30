#include "iconfile.h"

#include <array>
#include <charconv>
#include <cstddef>

namespace coastline {

namespace {

constexpr std::size_t ICON_HEADER = 6;
constexpr std::size_t FILE_ENTRY = 16;
constexpr std::size_t BITMAP_HEADER = 40;

std::uint32_t Little16(std::span<const std::uint8_t> bytes, std::size_t at) {
    return static_cast<std::uint32_t>(bytes[at] | (bytes[at + 1] << 8));
}

std::uint32_t Little32(std::span<const std::uint8_t> bytes, std::size_t at) {
    return Little16(bytes, at) | Little16(bytes, at + 2) << 16;
}

} // namespace

std::optional<IconSize> ParseIconName(std::string_view name) {
    if (!name.ends_with(".png")) {
        return std::nullopt;
    }
    name.remove_suffix(4);
    std::array<int, 3> values{};
    for (std::size_t i = 0; i < values.size(); ++i) {
        auto [end, error] = std::from_chars(name.data(), name.data() + name.size(), values[i]);
        if (error != std::errc() || values[i] <= 0) {
            return std::nullopt;
        }
        name.remove_prefix(static_cast<std::size_t>(end - name.data()));
        if (i + 1 < values.size()) {
            if (!name.starts_with('x')) {
                return std::nullopt;
            }
            name.remove_prefix(1);
        }
    }
    if (!name.empty()) {
        return std::nullopt;
    }
    return IconSize{values[0], values[1], values[2]};
}

std::string IconName(const IconSize &size) {
    return std::to_string(size.width) + 'x' + std::to_string(size.height) + 'x' +
           std::to_string(size.bitCount) + ".png";
}

std::vector<IconEntry> ListIconImages(std::span<const std::uint8_t> icon) {
    if (icon.size() < ICON_HEADER || Little16(icon, 0) != 0 || Little16(icon, 2) != 1) {
        return {};
    }
    std::size_t count = Little16(icon, 4);
    if (icon.size() < ICON_HEADER + count * FILE_ENTRY) {
        return {};
    }
    std::vector<IconEntry> entries;
    for (std::size_t i = 0; i < count; ++i) {
        std::size_t entry = ICON_HEADER + i * FILE_ENTRY;
        std::size_t size = Little32(icon, entry + 8);
        std::size_t offset = Little32(icon, entry + 12);
        if (offset > icon.size() || size > icon.size() - offset) {
            return {};
        }
        auto image = icon.subspan(offset, size);
        std::uint32_t bitCount = Little16(icon, entry + 6);
        if (bitCount == 0) {
            bool png = image.size() >= 4 && image[0] == 0x89 && image[1] == 'P' &&
                       image[2] == 'N' && image[3] == 'G';
            bitCount = png ? 32 : image.size() >= BITMAP_HEADER ? Little16(image, 14) : 0;
        }
        entries.push_back({bitCount, image});
    }
    return entries;
}

std::optional<IconPixels> DecodeIconBitmap(std::span<const std::uint8_t> image) {
    if (image.size() < BITMAP_HEADER) {
        return std::nullopt;
    }
    std::size_t headerSize = Little32(image, 0);
    auto width = static_cast<std::int32_t>(Little32(image, 4));
    auto doubleHeight = static_cast<std::int32_t>(Little32(image, 8));
    std::uint32_t bitCount = Little16(image, 14);
    std::uint32_t compression = Little32(image, 16);
    std::size_t colorsUsed = Little32(image, 32);
    if (headerSize < BITMAP_HEADER || width <= 0 || doubleHeight <= 0 || doubleHeight % 2 != 0 ||
        compression != 0 ||
        (bitCount != 1 && bitCount != 4 && bitCount != 8 && bitCount != 24 && bitCount != 32)) {
        return std::nullopt;
    }
    std::size_t columns = static_cast<std::size_t>(width);
    std::size_t rows = static_cast<std::size_t>(doubleHeight / 2);
    std::size_t paletteSize = 0;
    if (bitCount <= 8) {
        paletteSize = colorsUsed != 0 ? colorsUsed : std::size_t{1} << bitCount;
    }
    std::size_t colorStride = (columns * bitCount + 31) / 32 * 4;
    std::size_t maskStride = (columns + 31) / 32 * 4;
    std::size_t colors = headerSize + paletteSize * 4;
    std::size_t mask = colors + colorStride * rows;
    if (image.size() < headerSize || image.size() - headerSize < paletteSize * 4 ||
        image.size() < mask || image.size() - mask < maskStride * rows) {
        return std::nullopt;
    }
    bool hasAlpha = false;
    for (std::size_t row = 0; bitCount == 32 && row < rows; ++row) {
        for (std::size_t x = 0; x < columns; ++x) {
            hasAlpha = hasAlpha || image[colors + row * colorStride + x * 4 + 3] != 0;
        }
    }
    IconPixels pixels{width, doubleHeight / 2, std::vector<std::uint8_t>(columns * rows * 4)};
    for (std::size_t y = 0; y < rows; ++y) {
        // The rows run from the bottom.
        std::size_t row = rows - 1 - y;
        auto source = image.subspan(colors + row * colorStride, colorStride);
        for (std::size_t x = 0; x < columns; ++x) {
            std::uint8_t *out = &pixels.rgba[(y * columns + x) * 4];
            const std::uint8_t *bgr = nullptr;
            if (bitCount <= 8) {
                std::size_t bit = x * bitCount;
                std::size_t index = source[bit / 8] >> (8 - bitCount - bit % 8) &
                                    ((std::size_t{1} << bitCount) - 1);
                if (index >= paletteSize) {
                    return std::nullopt;
                }
                bgr = &image[headerSize + index * 4];
            } else {
                bgr = &source[x * (bitCount / 8)];
            }
            out[0] = bgr[2];
            out[1] = bgr[1];
            out[2] = bgr[0];
            bool masked = image[mask + row * maskStride + x / 8] >> (7 - x % 8) & 1;
            if (hasAlpha) {
                out[3] = bgr[3];
            } else {
                out[3] = masked ? 0 : 255;
            }
        }
    }
    return pixels;
}

} // namespace coastline
