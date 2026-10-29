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

/** @ghidraAddress EAR: 0x1000f870 */
void __stdcall EarShutDownTimerExportWrapper(void);

/** @ghidraAddress EAR: 0x1000f87a */
void __stdcall EAR_DLL_ShutDownTimer(void);

/** @ghidraAddress EAR: 0x1000fa7a */
int __stdcall EAR_DLL_StartTimer(void);

/** @ghidraAddress EAR: 0x1000fa70 */
int __stdcall EarStartTimerExportWrapper(void);

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** @ghidraAddress EAR: 0x1003b4a0 */
extern UINT g_uPlaybackTimerId;

/** @ghidraAddress EAR: 0x1003b3ac */
extern WORD g_wTimerResolutionMs;

/** @ghidraAddress EAR: 0x1003b578 */
extern TIMECAPS g_earTimerCapabilities;
#else
/** @ghidraAddress EAR: 0x1003b4a0 */
extern SDL_TimerID g_uPlaybackTimerId;

/**
 * The clock of EAR, in milliseconds, as timeGetTime() gives it in the original. A mixer that
 * renders on request counts the time of the frames it rendered.
 *
 * @return The time.
 */
DWORD GetEarTimeMilliseconds(void);
#endif

/** @ghidraAddress EAR: 0x1003b45c */
extern DWORD g_dwPreviousPlaybackUpdateTimeMs;

/** @ghidraAddress EAR: 0x1003b4d4 */
extern DWORD g_dwPlaybackUpdateTimeMs;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_TIMER_H */
