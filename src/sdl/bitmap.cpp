#include "bitmap.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

#include <SDL3/SDL.h>

#include "gamefiles.h"

namespace {

struct SurfaceDeleter {
    void operator()(SDL_Surface *surface) const {
        SDL_DestroySurface(surface);
    }
};

} // namespace

extern "C" BOOL LoadGameBitmap(const char *path, BITMAP *bitmap) {
    // The import converts each bitmap to a PNG image of the same name.
    std::string png(path);
    auto dot = png.find_last_of('.');
    png = (dot == std::string::npos ? png : png.substr(0, dot)) + ".png";
    auto found = coastline::FindGameFile(png.c_str());
    if (!found) {
        return FALSE;
    }
    std::unique_ptr<SDL_Surface, SurfaceDeleter> loaded(SDL_LoadPNG(found->string().c_str()));
    if (!loaded) {
        return FALSE;
    }
    // The bitmap of LoadImageA(): BGR rows from the bottom, each a multiple of four bytes.
    std::unique_ptr<SDL_Surface, SurfaceDeleter> surface(
        SDL_ConvertSurface(loaded.get(), SDL_PIXELFORMAT_BGR24));
    if (!surface) {
        return FALSE;
    }
    std::uint32_t stride = ((static_cast<std::uint32_t>(surface->w) * 24 + 31) / 32) * 4;
    std::size_t size = static_cast<std::size_t>(stride) * static_cast<std::size_t>(surface->h);
    auto *pixels = static_cast<unsigned char *>(std::calloc(size, 1));
    if (pixels == nullptr) {
        return FALSE;
    }
    for (int row = 0; row < surface->h; ++row) {
        std::memcpy(pixels + static_cast<std::size_t>(surface->h - 1 - row) * stride,
                    static_cast<const unsigned char *>(surface->pixels) +
                        static_cast<std::size_t>(row) * static_cast<std::size_t>(surface->pitch),
                    static_cast<std::size_t>(surface->w) * 3);
    }
    bitmap->bmType = 0;
    bitmap->bmWidth = surface->w;
    bitmap->bmHeight = surface->h;
    bitmap->bmWidthBytes = static_cast<LONG>(stride);
    bitmap->bmPlanes = 1;
    bitmap->bmBitsPixel = 24;
    bitmap->bmBits = pixels;
    return TRUE;
}
