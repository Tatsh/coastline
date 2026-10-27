#include "timer.h"

#include <mmsystem.h>

#include "error.h"
#include "playback.h"
#include "runtime.h"

UINT g_uPlaybackTimerId;
WORD g_wTimerResolutionMs;
DWORD g_dwTimerMinimumPeriodMs;
DWORD g_dwTimerMaximumPeriodMs;
DWORD g_dwPreviousPlaybackUpdateTimeMs;
DWORD g_dwPlaybackUpdateTimeMs;

// @ghidraAddress EAR: 0x100046e2
static void CALLBACK
EarTimerCallback(UINT timerId, UINT message, DWORD_PTR user, DWORD_PTR first, DWORD_PTR second) {
    UpdateEarPlaybackEngine();
}

void __stdcall EarShutDownTimerExportWrapper(void) {
    EAR_DLL_ShutDownTimer();
}

void __stdcall EAR_DLL_ShutDownTimer(void) {
    SetEarLastError(0);
    if (g_uPlaybackTimerId == 0) {
        return;
    }
    if (timeKillEvent(g_uPlaybackTimerId) == MMSYSERR_INVALPARAM) {
        SetEarLastError(24);
        return;
    }
    timeEndPeriod(g_wTimerResolutionMs);
    g_uPlaybackTimerId = 0;
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
    DWORD period = g_dwTimerMinimumPeriodMs > 5 ? g_dwTimerMinimumPeriodMs : 5;
    if (period >= g_dwTimerMaximumPeriodMs) {
        period = g_dwTimerMaximumPeriodMs;
    }
    g_wTimerResolutionMs = (WORD)period;
    timeBeginPeriod(g_wTimerResolutionMs);
    g_uPlaybackTimerId = timeSetEvent(50, g_wTimerResolutionMs, EarTimerCallback, 0, TIME_PERIODIC);
    if (g_uPlaybackTimerId == 0) {
        SetEarLastError(24);
        return FALSE;
    }
    g_dwPreviousPlaybackUpdateTimeMs = timeGetTime();
    g_dwPlaybackUpdateTimeMs = 0;
    return TRUE;
}
