#include "bitmap.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

#include "gamefiles.h"

namespace {

std::uint32_t Little32(const std::vector<unsigned char> &bytes, std::size_t offset) {
    return bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

} // namespace

extern "C" BOOL LoadGameBitmap(const char *path, BITMAP *bitmap) {
    auto found = coastline::FindGameFile(path);
    if (!found) {
        return FALSE;
    }
    std::ifstream file(*found, std::ios::binary);
    std::vector<unsigned char> bytes(std::istreambuf_iterator<char>(file), {});
    // BITMAPFILEHEADER, then a BITMAPINFOHEADER of at least 40 bytes.
    if (bytes.size() < 54 || bytes[0] != 'B' || bytes[1] != 'M') {
        return FALSE;
    }
    std::uint32_t pixelOffset = Little32(bytes, 10);
    auto width = static_cast<std::int32_t>(Little32(bytes, 18));
    auto height = static_cast<std::int32_t>(Little32(bytes, 22));
    std::uint16_t bits = static_cast<std::uint16_t>(bytes[28] | (bytes[29] << 8));
    std::uint32_t compression = Little32(bytes, 30);
    if (compression != 0 || width <= 0 || height == 0) {
        return FALSE;
    }
    // A row of a DIB is a multiple of four bytes, and LoadImageA() keeps a bottom-up bitmap so.
    std::int32_t rows = height < 0 ? -height : height;
    std::uint32_t stride = ((static_cast<std::uint32_t>(width) * bits + 31) / 32) * 4;
    std::size_t size = static_cast<std::size_t>(stride) * static_cast<std::size_t>(rows);
    if (pixelOffset + size > bytes.size()) {
        return FALSE;
    }
    void *pixels = std::malloc(size);
    if (pixels == nullptr) {
        return FALSE;
    }
    std::memcpy(pixels, bytes.data() + pixelOffset, size);
    bitmap->bmType = 0;
    bitmap->bmWidth = width;
    bitmap->bmHeight = rows;
    bitmap->bmWidthBytes = static_cast<LONG>(stride);
    bitmap->bmPlanes = 1;
    bitmap->bmBitsPixel = bits;
    bitmap->bmBits = pixels;
    return TRUE;
}
