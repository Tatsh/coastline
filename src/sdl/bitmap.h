#ifndef COASTLINE_SDL_BITMAP_H
#define COASTLINE_SDL_BITMAP_H

#include "sdl/win32types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read the PNG image the import converted from a bitmap file of the game, as LoadImageA() reads
 * the bitmap into a DIB section and GetObjectA() describes it: 24-bit BGR rows from the bottom,
 * each a multiple of four bytes.
 *
 * @param path The path of the bitmap in the PC release, whose extension is replaced with .png.
 * @param bitmap Receives the description, whose bmBits is allocated with malloc().
 * @return Whether the image exists and can be read.
 */
BOOL LoadGameBitmap(const char *path, BITMAP *bitmap);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SDL_BITMAP_H */
