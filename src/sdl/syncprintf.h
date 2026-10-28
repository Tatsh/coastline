#ifndef COASTLINE_SDL_SYNCPRINTF_H
#define COASTLINE_SDL_SYNCPRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write the debug output of the shared code to the SDL log at the verbose priority, as libultra
 * writes it to the debugger.
 *
 * @param format The printf format.
 */
void osSyncPrintf(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SDL_SYNCPRINTF_H */
