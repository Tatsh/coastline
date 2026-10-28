#ifndef COASTLINE_SDL_STRINGRESOURCES_H
#define COASTLINE_SDL_STRINGRESOURCES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Read a string resource of a Windows library as LoadStringA() does: the UTF-16 string of the
 * RT_STRING resource converted to Windows-1252, truncated to the buffer, and terminated.
 *
 * @param image The bytes of the library file.
 * @param imageSize The size of the file.
 * @param id The identifier of the string.
 * @param buffer The buffer for the string.
 * @param bufferSize The size of the buffer, at least 1.
 * @return The characters copied without the terminator, or 0 when the library has no such string.
 */
int LoadLibraryString(
    const unsigned char *image, size_t imageSize, unsigned id, char *buffer, int bufferSize);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SDL_STRINGRESOURCES_H */
