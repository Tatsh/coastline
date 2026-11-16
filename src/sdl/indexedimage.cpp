#include "indexedimage.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>

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

bool ReadIndexed4Png(Surface surface,
                     std::span<std::uint8_t> texels,
                     std::span<std::uint8_t, 32> palette) {
    if (!surface) {
        return false;
    }
    SDL_Palette *colors = SDL_GetSurfacePalette(surface.get());
    int bits = SDL_BITSPERPIXEL(surface->format);
    // SDL pads the palette it reads to 256 colours, and a texel must use one of the first 16.
    auto rowBytes = (static_cast<std::size_t>(surface->w) + 1) / 2;
    if (colors == nullptr || (bits != 4 && bits != 8) ||
        rowBytes * static_cast<std::size_t>(surface->h) != texels.size()) {
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
            texels[static_cast<std::size_t>(y) * rowBytes + static_cast<std::size_t>(x) / 2] |=
                static_cast<std::uint8_t>((index & 0xF) << (x % 2 == 0 ? 4 : 0));
        }
    }
    return true;
}

// The 8-bit surface of a 4-bit image, as SDL writes the palette of an 8-bit surface and not of a
// 4-bit one. The surface is null when the texels do not fill the size.
Surface Indexed4Surface(std::span<const std::uint8_t> texels,
                        std::span<const std::uint8_t, 32> palette,
                        int width,
                        int height) {
    auto rowBytes = (static_cast<std::size_t>(width) + 1) / 2;
    if (width <= 0 || height <= 0 || texels.size() != rowBytes * static_cast<std::size_t>(height)) {
        return nullptr;
    }
    Surface surface(SDL_CreateSurface(width, height, SDL_PIXELFORMAT_INDEX8));
    if (!surface) {
        return nullptr;
    }
    for (int y = 0; y < height; ++y) {
        auto *row = static_cast<std::uint8_t *>(surface->pixels) +
                    static_cast<std::size_t>(y) * static_cast<std::size_t>(surface->pitch);
        for (int x = 0; x < width; ++x) {
            std::uint8_t pair =
                texels[static_cast<std::size_t>(y) * rowBytes + static_cast<std::size_t>(x) / 2];
            row[x] = static_cast<std::uint8_t>(pair >> (x % 2 == 0 ? 4 : 0) & 0xF);
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
        !SDL_SetSurfacePalette(surface.get(), surfacePalette.get())) {
        return nullptr;
    }
    return surface;
}

bool MatchesIndexed4(Surface read,
                     std::span<const std::uint8_t> texels,
                     std::span<const std::uint8_t, 32> palette) {
    std::vector<std::uint8_t> readTexels(texels.size());
    std::array<std::uint8_t, 32> readPalette{};
    return ReadIndexed4Png(std::move(read), readTexels, readPalette) &&
           std::ranges::equal(readTexels, texels) && std::ranges::equal(readPalette, palette);
}

// The surface is null when the texels do not fill the size.
Surface Rgba8Surface(std::span<const std::uint8_t> rgba, int width, int height) {
    auto rowBytes = static_cast<std::size_t>(width) * 4;
    if (width <= 0 || height <= 0 || rgba.size() != rowBytes * static_cast<std::size_t>(height)) {
        return nullptr;
    }
    Surface surface(SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32));
    if (!surface) {
        return nullptr;
    }
    for (int y = 0; y < height; ++y) {
        std::memcpy(static_cast<std::uint8_t *>(surface->pixels) +
                        static_cast<std::size_t>(y) * static_cast<std::size_t>(surface->pitch),
                    rgba.data() + static_cast<std::size_t>(y) * rowBytes,
                    rowBytes);
    }
    return surface;
}

bool MatchesRgba8(Surface read, std::span<const std::uint8_t> rgba, int width, int height) {
    auto rowBytes = static_cast<std::size_t>(width) * 4;
    Surface converted(read ? SDL_ConvertSurface(read.get(), SDL_PIXELFORMAT_RGBA32) : nullptr);
    if (!converted || converted->w != width || converted->h != height) {
        return false;
    }
    for (int y = 0; y < height; ++y) {
        if (std::memcmp(static_cast<const std::uint8_t *>(converted->pixels) +
                            static_cast<std::size_t>(y) *
                                static_cast<std::size_t>(converted->pitch),
                        rgba.data() + static_cast<std::size_t>(y) * rowBytes,
                        rowBytes) != 0) {
            return false;
        }
    }
    return true;
}

std::optional<std::vector<std::uint8_t>> EncodePng(SDL_Surface *surface) {
    if (surface == nullptr) {
        return std::nullopt;
    }
    SDL_IOStream *stream = SDL_IOFromDynamicMem();
    if (stream == nullptr) {
        return std::nullopt;
    }
    std::optional<std::vector<std::uint8_t>> bytes;
    if (SDL_SavePNG_IO(surface, stream, false)) {
        const auto *data = static_cast<const std::uint8_t *>(SDL_GetPointerProperty(
            SDL_GetIOProperties(stream), SDL_PROP_IOSTREAM_DYNAMIC_MEMORY_POINTER, nullptr));
        const Sint64 size = SDL_GetIOSize(stream);
        if (data != nullptr && size >= 0) {
            bytes.emplace(data, data + size);
        }
    }
    SDL_CloseIO(stream);
    return bytes;
}

Surface DecodePng(std::span<const std::uint8_t> bytes) {
    return Surface(SDL_LoadPNG_IO(SDL_IOFromConstMem(bytes.data(), bytes.size()), true));
}

} // namespace

bool LoadIndexed4Png(const std::filesystem::path &path,
                     std::span<std::uint8_t> texels,
                     std::span<std::uint8_t, 32> palette) {
    return ReadIndexed4Png(Surface(SDL_LoadPNG(path.string().c_str())), texels, palette);
}

bool LoadIndexed4Png(std::span<const std::uint8_t> bytes,
                     std::span<std::uint8_t> texels,
                     std::span<std::uint8_t, 32> palette) {
    if (bytes.empty()) {
        return false;
    }
    return ReadIndexed4Png(DecodePng(bytes), texels, palette);
}

bool SaveIndexed4Png(const std::filesystem::path &path,
                     std::span<const std::uint8_t> texels,
                     std::span<const std::uint8_t, 32> palette,
                     int width,
                     int height) {
    Surface surface = Indexed4Surface(texels, palette, width, height);
    // The image is retained only when it reads back as the same texels and palette.
    return surface && SDL_SavePNG(surface.get(), path.string().c_str()) &&
           MatchesIndexed4(Surface(SDL_LoadPNG(path.string().c_str())), texels, palette);
}

std::optional<std::vector<std::uint8_t>>
EncodeIndexed4Png(std::span<const std::uint8_t> texels,
                  std::span<const std::uint8_t, 32> palette,
                  int width,
                  int height) {
    auto bytes = EncodePng(Indexed4Surface(texels, palette, width, height).get());
    if (!bytes || !MatchesIndexed4(DecodePng(*bytes), texels, palette)) {
        return std::nullopt;
    }
    return bytes;
}

bool SaveRgba8Png(const std::filesystem::path &path,
                  std::span<const std::uint8_t> rgba,
                  int width,
                  int height) {
    Surface surface = Rgba8Surface(rgba, width, height);
    return surface && SDL_SavePNG(surface.get(), path.string().c_str()) &&
           MatchesRgba8(Surface(SDL_LoadPNG(path.string().c_str())), rgba, width, height);
}

std::optional<std::vector<std::uint8_t>>
EncodeRgba8Png(std::span<const std::uint8_t> rgba, int width, int height) {
    auto bytes = EncodePng(Rgba8Surface(rgba, width, height).get());
    if (!bytes || !MatchesRgba8(DecodePng(*bytes), rgba, width, height)) {
        return std::nullopt;
    }
    return bytes;
}

} // namespace coastline
