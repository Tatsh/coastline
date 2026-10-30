#include <stdbool.h>

#include "common/frameclock.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <windows.h>

#include "win32/timer.h"
// The headers below need the types of windows.h.
#include <mmsystem.h>

int g_bHighResTimerFirstCall = true;
LARGE_INTEGER g_liPerfCounterFrequency;
int g_bHasPerfCounter;
DWORD g_dwPerfCounterBaseMs;

// A count of the performance counter in milliseconds, rounded to the nearest.
static inline DWORD CounterToMilliseconds(LARGE_INTEGER count) {
    return (DWORD)((count.QuadPart * 1000 + 500) / g_liPerfCounterFrequency.QuadPart);
}

u32 GetHighResolutionTime(void) {
    LARGE_INTEGER count;

    if (g_bHighResTimerFirstCall) {
        g_bHasPerfCounter = QueryPerformanceFrequency(&g_liPerfCounterFrequency);
        // The result of the first read is not checked.
        QueryPerformanceCounter(&count);
        g_dwPerfCounterBaseMs = CounterToMilliseconds(count);
        g_bHighResTimerFirstCall = false;
    }
    if (g_bHasPerfCounter && QueryPerformanceCounter(&count)) {
        return CounterToMilliseconds(count) - g_dwPerfCounterBaseMs;
    }
    return timeGetTime();
}
#else
#include <SDL3/SDL.h>

static bool s_bHighResTimerFirstCall = true;
static Uint64 s_qwPerfCounterFrequency;
static unsigned s_dwPerfCounterBaseMs;

// A count of the performance counter in milliseconds, rounded to the nearest.
static inline unsigned CounterToMilliseconds(Uint64 count) {
    return (unsigned)((count * 1000 + 500) / s_qwPerfCounterFrequency);
}

u32 GetHighResolutionTime(void) {
    if (s_bHighResTimerFirstCall) {
        s_qwPerfCounterFrequency = SDL_GetPerformanceFrequency();
        s_dwPerfCounterBaseMs = CounterToMilliseconds(SDL_GetPerformanceCounter());
        s_bHighResTimerFirstCall = false;
    }
    return CounterToMilliseconds(SDL_GetPerformanceCounter()) - s_dwPerfCounterBaseMs;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
