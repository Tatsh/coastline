#ifndef COASTLINE_EAR_TIMER_H
#define COASTLINE_EAR_TIMER_H

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>
// Multimedia declarations require the core Windows SDK types first.
#include <mmsystem.h>
#else
#include <SDL3/SDL_timer.h>

#include "common/types.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward timer cancellation to EAR_DLL_ShutDownTimer().
 * @ghidraAddress EAR: 0x1000f870
 */
void __stdcall EarShutDownTimerExportWrapper(void);

/**
 * Cancel the periodic playback timer if present and clear its identifier on success.
 * Clear the last EAR error first; cancellation failure records error 24 and preserves
 * the identifier.
 * @ghidraAddress EAR: 0x1000f87a
 */
void __stdcall EAR_DLL_ShutDownTimer(void);

/**
 * Attempt to replace an existing timer and schedule playback updates every 50 milliseconds.
 * The library must be ready. Replacement proceeds even when cancelling an existing timer fails.
 * @return TRUE when a timer is created; FALSE when EAR is unavailable or creation fails.
 * Creation failure records EAR error 24.
 * @ghidraAddress EAR: 0x1000fa7a
 */
int __stdcall EAR_DLL_StartTimer(void);

/**
 * Forward periodic timer creation to EAR_DLL_StartTimer().
 * @return Whether the playback timer was created.
 * @ghidraAddress EAR: 0x1000fa70
 */
int __stdcall EarStartTimerExportWrapper(void);

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * Windows multimedia playback timer identifier, or zero when no timer is registered.
 * @ghidraAddress EAR: 0x1003b4a0
 */
/** @cond */
extern UINT g_uPlaybackTimerId;

/**
 * Requested Windows multimedia timer resolution in milliseconds, bounded by timer capabilities.
 * @ghidraAddress EAR: 0x1003b3ac
 */
extern WORD g_wTimerResolutionMs;

/**
 * Cached Windows multimedia timer capability bounds used during timer creation.
 * @ghidraAddress EAR: 0x1003b578
 */
extern TIMECAPS g_earTimerCapabilities;
/** @endcond */
#else
/**
 * SDL playback timer identifier, or zero when no timer is registered.
 * @ghidraAddress EAR: 0x1003b4a0
 */
/** @cond */
extern SDL_TimerID g_uPlaybackTimerId;
/** @endcond */

/**
 * The clock of EAR, in milliseconds, as timeGetTime() gives it in the original. A mixer that
 * renders on request counts the time of the frames it rendered.
 *
 * @return The current EAR clock in milliseconds, wrapping at 32 bits.
 */
DWORD GetEarTimeMilliseconds(void);
#endif

/**
 * Previous playback-update clock reading in milliseconds for unsigned elapsed-time calculation.
 * @ghidraAddress EAR: 0x1003b45c
 */
/** @cond */
extern DWORD g_dwPreviousPlaybackUpdateTimeMs;

/**
 * Current absolute playback-update clock reading in milliseconds, wrapping at 32 bits.
 * Timer creation resets the reading to zero until the first update.
 * @ghidraAddress EAR: 0x1003b4d4
 */
extern DWORD g_dwPlaybackUpdateTimeMs;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_TIMER_H */
