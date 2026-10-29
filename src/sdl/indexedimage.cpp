#include "indexedimage.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <vector>

#include <SDL3/SDL.h>

namespace coastline {

namespace {

struct SurfaceDeleter {
    void operator()(SDL_Surface *surface) const {
        SDL_DestroySurface(surface);
    }
};
using Surface = std::unique_ptr<SDL_Surface, SurfaceDeleter>;

// A channel of five bits widened to eight, whose top five bits are the channel.
std::uint8_t Widen5(unsigned value) {
    return static_cast<std::uint8_t>(value << 3 | value >> 2);
}

} // namespace

bool LoadIndexed4Png(const std::filesystem::path &path,
                     std::span<std::uint8_t> texels,
                     std::span<std::uint8_t, 32> palette) {
    Surface surface(SDL_LoadPNG(path.string().c_str()));
    if (!surface) {
        return false;
    }
    SDL_Palette *colors = SDL_GetSurfacePalette(surface.get());
    int bits = SDL_BITSPERPIXEL(surface->format);
    // SDL pads the palette it reads to 256 colours, and a texel must use one of the first 16.
    if (colors == nullptr || (bits != 4 && bits != 8) ||
        static_cast<std::size_t>(surface->w) * static_cast<std::size_t>(surface->h) !=
            texels.size() * 2) {
        return false;
    }
    std::ranges::fill(palette, 0);
    for (int i = 0; i < std::min(colors->ncolors, 16); ++i) {
        const SDL_Color &c = colors->colors[i];
        auto half = static_cast<std::uint16_t>((c.r >> 3) << 11 | (c.g >> 3) << 6 |
                                               (c.b >> 3) << 1 | (c.a >= 0x80 ? 1 : 0));
        palette[static_cast<std::size_t>(i) * 2] = static_cast<std::uint8_t>(half >> 8);
        palette[static_cast<std::size_t>(i) * 2 + 1] = static_cast<std::uint8_t>(half);
    }
    std::ranges::fill(texels, 0);
    for (int y = 0; y < surface->h; ++y) {
        const auto *row = static_cast<const std::uint8_t *>(surface->pixels) +
                          static_cast<std::size_t>(y) * static_cast<std::size_t>(surface->pitch);
        for (int x = 0; x < surface->w; ++x) {
            unsigned index = bits == 8 ? row[x] : (row[x / 2] >> (x % 2 == 0 ? 4 : 0)) & 0xF;
            if (index >= 16) {
                return false;
            }
            std::size_t texel = static_cast<std::size_t>(y) * static_cast<std::size_t>(surface->w) +
                                static_cast<std::size_t>(x);
            texels[texel / 2] |=
                static_cast<std::uint8_t>((index & 0xF) << (texel % 2 == 0 ? 4 : 0));
        }
    }
    return true;
}

bool SaveIndexed4Png(const std::filesystem::path &path,
                     std::span<const std::uint8_t> texels,
                     std::span<const std::uint8_t, 32> palette,
                     int width,
                     int height) {
    if (width <= 0 || height <= 0 || width % 2 != 0 ||
        texels.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height) / 2) {
        return false;
    }
    // SDL writes the palette of an 8-bit surface, and not of a 4-bit one.
    Surface surface(SDL_CreateSurface(width, height, SDL_PIXELFORMAT_INDEX8));
    if (!surface) {
        return false;
    }
    for (int y = 0; y < height; ++y) {
        auto *row = static_cast<std::uint8_t *>(surface->pixels) +
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(surface->pitch);
        for (int x = 0; x < width; ++x) {
            std::size_t texel = static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                static_cast<std::size_t>(x);
            row[x] = static_cast<std::uint8_t>(texels[texel / 2] >> (texel % 2 == 0 ? 4 : 0) & 0xF);
        }
    }
    std::array<SDL_Color, 16> colors{};
    for (std::size_t i = 0; i < colors.size(); ++i) {
        unsigned half = static_cast<unsigned>(palette[i * 2]) << 8 | palette[i * 2 + 1];
        colors[i] = {Widen5(half >> 11 & 0x1F),
                     Widen5(half >> 6 & 0x1F),
                     Widen5(half >> 1 & 0x1F),
                     static_cast<std::uint8_t>(half & 1 ? 0xFF : 0)};
    }
    std::unique_ptr<SDL_Palette, decltype(&SDL_DestroyPalette)> surfacePalette(
        SDL_CreatePalette(static_cast<int>(colors.size())), SDL_DestroyPalette);
    if (!surfacePalette ||
        !SDL_SetPaletteColors(
            surfacePalette.get(), colors.data(), 0, static_cast<int>(colors.size())) ||
        !SDL_SetSurfacePalette(surface.get(), surfacePalette.get()) ||
        !SDL_SavePNG(surface.get(), path.string().c_str())) {
        return false;
    }
    // The image is kept only when it reads back as the same texels and palette.
    std::vector<std::uint8_t> readTexels(texels.size());
    std::array<std::uint8_t, 32> readPalette{};
    return LoadIndexed4Png(path, readTexels, readPalette) &&
           std::ranges::equal(readTexels, texels) && std::ranges::equal(readPalette, palette);
}

} // namespace coastline
