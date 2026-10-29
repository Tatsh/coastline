#include "n64texture.h"

#include <SDL3/SDL.h>

namespace coastline {

namespace {

// The RDP formats and sizes of the console.
constexpr int FORMAT_RGBA = 0;
constexpr int FORMAT_IA = 3;
constexpr int FORMAT_I = 4;
constexpr int SIZE_4 = 0;
constexpr int SIZE_8 = 1;
constexpr int SIZE_16 = 2;
constexpr int SIZE_32 = 3;

std::uint8_t Expand(unsigned value, unsigned bits) {
    return static_cast<std::uint8_t>(value * 255u / ((1u << bits) - 1u));
}

} // namespace

TexelFormat TexelFormatOf(int format, int size) {
    if (format == FORMAT_I) {
        return size == SIZE_4 ? TexelFormat::Intensity4 : TexelFormat::Intensity8;
    }
    if (format == FORMAT_IA) {
        switch (size) {
        case SIZE_4:
            return TexelFormat::IntensityAlpha4;
        case SIZE_8:
            return TexelFormat::IntensityAlpha8;
        default:
            return TexelFormat::IntensityAlpha16;
        }
    }
    return format == FORMAT_RGBA && size == SIZE_32 ? TexelFormat::Rgba32 : TexelFormat::Rgba16;
}

int BitsPerTexel(TexelFormat format) {
    switch (format) {
    case TexelFormat::Intensity4:
    case TexelFormat::IntensityAlpha4:
        return 4;
    case TexelFormat::Intensity8:
    case TexelFormat::IntensityAlpha8:
        return 8;
    case TexelFormat::Rgba32:
        return 32;
    default:
        return 16;
    }
}

std::vector<std::uint8_t>
DecodeTexels(std::span<const std::uint8_t> texels, int width, int height, TexelFormat format) {
    int bits = BitsPerTexel(format);
    std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    count = std::min(count, texels.size() * 8 / static_cast<std::size_t>(bits));
    std::vector<std::uint8_t> pixels(count * 4);
    for (std::size_t i = 0; i < count; ++i) {
        std::size_t bit = i * static_cast<std::size_t>(bits);
        const std::uint8_t *at = &texels[bit / 8];
        unsigned nibble = (i & 1) ? at[0] & 0xfu : at[0] >> 4;
        std::uint8_t r = 0;
        std::uint8_t g = 0;
        std::uint8_t b = 0;
        std::uint8_t a = 0;
        switch (format) {
        case TexelFormat::Intensity4:
            r = g = b = a = Expand(nibble, 4);
            break;
        case TexelFormat::Intensity8:
            r = g = b = a = at[0];
            break;
        case TexelFormat::IntensityAlpha4:
            r = g = b = Expand(nibble >> 1, 3);
            a = (nibble & 1) ? 0xff : 0;
            break;
        case TexelFormat::IntensityAlpha8:
            r = g = b = Expand(at[0] >> 4, 4);
            a = Expand(at[0] & 0xfu, 4);
            break;
        case TexelFormat::IntensityAlpha16:
            r = g = b = at[0];
            a = at[1];
            break;
        case TexelFormat::Rgba16: {
            unsigned value = static_cast<unsigned>(at[0]) << 8 | at[1];
            r = Expand(value >> 11 & 0x1fu, 5);
            g = Expand(value >> 6 & 0x1fu, 5);
            b = Expand(value >> 1 & 0x1fu, 5);
            a = (value & 1) ? 0xff : 0;
            break;
        }
        case TexelFormat::Rgba32:
            r = at[0];
            g = at[1];
            b = at[2];
            a = at[3];
            break;
        }
        std::uint8_t *out = &pixels[i * 4];
        out[0] = r;
        out[1] = g;
        out[2] = b;
        out[3] = a;
    }
    return pixels;
}

bool WritePng(const std::filesystem::path &path,
              std::span<const std::uint8_t> pixels,
              int width,
              int height) {
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    SDL_Surface *surface = SDL_CreateSurfaceFrom(width,
                                                 height,
                                                 SDL_PIXELFORMAT_RGBA32,
                                                 const_cast<std::uint8_t *>(pixels.data()),
                                                 width * 4);
    bool written = surface != nullptr && SDL_SavePNG(surface, path.string().c_str());
    SDL_DestroySurface(surface);
    return written;
}

} // namespace coastline
