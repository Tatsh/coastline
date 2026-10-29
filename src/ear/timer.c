#include "ear/timer.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <mmsystem.h>
#endif

#include "ear/error.h"
#include "ear/playback.h"
#include "ear/runtime.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
UINT g_uPlaybackTimerId;
WORD g_wTimerResolutionMs;
TIMECAPS g_earTimerCapabilities;
#else
SDL_TimerID g_uPlaybackTimerId;
#endif
DWORD g_dwPreviousPlaybackUpdateTimeMs;
DWORD g_dwPlaybackUpdateTimeMs;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
// @ghidraAddress EAR: 0x100046e2
static void CALLBACK
EarTimerCallback(UINT timerId, UINT message, DWORD_PTR user, DWORD_PTR first, DWORD_PTR second) {
    UpdateEarPlaybackEngine();
}
#else
// @ghidraAddress EAR: 0x100046e2
static Uint32 SDLCALL EarTimerCallback(void *user, SDL_TimerID timerId, Uint32 interval) {
    UpdateEarPlaybackEngine();
    return interval;
}
#endif

void __stdcall EarShutDownTimerExportWrapper(void) {
    EAR_DLL_ShutDownTimer();
}

void __stdcall EAR_DLL_ShutDownTimer(void) {
    SetEarLastError(0);
    if (g_uPlaybackTimerId == 0) {
        return;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (timeKillEvent(g_uPlaybackTimerId) == MMSYSERR_INVALPARAM) {
        SetEarLastError(24);
        return;
    }
    timeEndPeriod(g_wTimerResolutionMs);
#else
    if (!SDL_RemoveTimer(g_uPlaybackTimerId)) {
        SetEarLastError(24);
        return;
    }
#endif
    g_uPlaybackTimerId = 0;
}

int __stdcall EarStartTimerExportWrapper(void) {
    return EAR_DLL_StartTimer();
}

int __stdcall EAR_DLL_StartTimer(void) {
    SetEarLastError(0);
    if (!IsEarReady()) {
        return FALSE;
    }
    if (g_uPlaybackTimerId != 0) {
        // Startup proceeds even when cancellation fails.
        EarShutDownTimerExportWrapper();
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    DWORD period = g_earTimerCapabilities.wPeriodMin > 5 ? g_earTimerCapabilities.wPeriodMin : 5;
    if (period >= g_earTimerCapabilities.wPeriodMax) {
        period = g_earTimerCapabilities.wPeriodMax;
    }
    g_wTimerResolutionMs = (WORD)period;
    timeBeginPeriod(g_wTimerResolutionMs);
    g_uPlaybackTimerId = timeSetEvent(50, g_wTimerResolutionMs, EarTimerCallback, 0, TIME_PERIODIC);
#else
    g_uPlaybackTimerId = SDL_AddTimer(50, EarTimerCallback, NULL);
#endif
    if (g_uPlaybackTimerId == 0) {
        SetEarLastError(24);
        return FALSE;
    }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    g_dwPreviousPlaybackUpdateTimeMs = timeGetTime();
#else
    g_dwPreviousPlaybackUpdateTimeMs = GetEarTimeMilliseconds();
#endif
    g_dwPlaybackUpdateTimeMs = 0;
    return TRUE;
}
