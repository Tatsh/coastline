#ifndef COASTLINE_PLATFORM_WIN_TIMER_H
#define COASTLINE_PLATFORM_WIN_TIMER_H

/**
 * @file
 * The clock of the PC port.
 */

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/win32types.h"
#else
#include <windows.h>
#endif
#ifndef __cplusplus
#include <stdbool.h>
#endif

#include "common/frameclock.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Whether GetHighResolutionTime() has not run yet.
 *
 * @ghidraAddress BRD3D: 0x100bbad4
 * @ghidraAddress BRGlide: 0x100bb2dc
 */
extern int g_bHighResTimerFirstCall;

/**
 * The frequency of the performance counter, in counts a second.
 *
 * @ghidraAddress BRD3D: 0x118ab120
 * @ghidraAddress BRGlide: 0x118ee238
 */
extern LARGE_INTEGER g_liPerfCounterFrequency;

/**
 * Whether the system has a performance counter.
 *
 * @ghidraAddress BRD3D: 0x118ab128
 * @ghidraAddress BRGlide: 0x118ee240
 */
extern int g_bHasPerfCounter;

/**
 * The time of the performance counter at the first call of GetHighResolutionTime(), in
 * milliseconds.
 *
 * @ghidraAddress BRD3D: 0x118ab130
 * @ghidraAddress BRGlide: 0x118ee248
 */
extern DWORD g_dwPerfCounterBaseMs;

#ifdef __cplusplus
}
#endif

#endif // COASTLINE_PLATFORM_WIN_TIMER_H
