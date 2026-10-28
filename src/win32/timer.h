#ifndef COASTLINE_PLATFORM_WIN_TIMER_H
#define COASTLINE_PLATFORM_WIN_TIMER_H

/**
 * @file
 * The clock of the PC port.
 */

#include <windows.h>
#ifndef __cplusplus
#include <stdbool.h>
#endif

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

/**
 * The time since the first call, from the performance counter in milliseconds rounded to the
 * nearest, or the time of the system when there is no counter. The first call divides by the
 * frequency of the counter without checking it, as the original does.
 *
 * @return The time in milliseconds.
 * @ghidraAddress BRD3D: 0x10075020
 * @ghidraAddress BRGlide: 0x1006e280
 */
DWORD GetHighResolutionTime(void);

#ifdef __cplusplus
}
#endif

#endif // COASTLINE_PLATFORM_WIN_TIMER_H
