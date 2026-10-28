#ifndef COASTLINE_SDL_BITMAP_H
#define COASTLINE_SDL_BITMAP_H

#include "sdl/win32types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read a bitmap file of the game as LoadImageA() reads one into a DIB section and GetObjectA()
 * describes it: the rows as the file stores them, from the bottom, each a multiple of four bytes.
 *
 * @param path The path of the PC release.
 * @param bitmap Receives the description, whose bmBits is allocated with malloc().
 * @return Whether the file is a bitmap the game reads.
 */
BOOL LoadGameBitmap(const char *path, BITMAP *bitmap);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SDL_BITMAP_H */
