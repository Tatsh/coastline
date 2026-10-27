#ifndef COASTLINE_EAR_TIMER_H
#define COASTLINE_EAR_TIMER_H

#include <windows.h>

/** @ghidraAddress EAR: 0x1000f870 */
void __stdcall EarShutDownTimerExportWrapper();

/** @ghidraAddress EAR: 0x1000f87a */
extern "C" void __stdcall EAR_DLL_ShutDownTimer();

/** @ghidraAddress EAR: 0x1000fa7a */
extern "C" int __stdcall EAR_DLL_StartTimer();

/** @ghidraAddress EAR: 0x1003b4a0 */
extern UINT g_uPlaybackTimerId;

/** @ghidraAddress EAR: 0x1003b3ac */
extern WORD g_wTimerResolutionMs;

/** @ghidraAddress EAR: 0x1003b578 */
extern DWORD g_dwTimerMinimumPeriodMs;

/** @ghidraAddress EAR: 0x1003b57c */
extern DWORD g_dwTimerMaximumPeriodMs;

/** @ghidraAddress EAR: 0x1003b45c */
extern DWORD g_dwPreviousPlaybackUpdateTimeMs;

/** @ghidraAddress EAR: 0x1003b4d4 */
extern DWORD g_dwPlaybackUpdateTimeMs;

#endif
