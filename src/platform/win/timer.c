#include "platform/win/timer.h"

#include <stdbool.h>

#include <windows.h>
// The headers below need the types of windows.h.
#include <mmsystem.h>

bool g_bHighResTimerFirstCall = true;
LARGE_INTEGER g_liPerfCounterFrequency;
bool g_bHasPerfCounter;
DWORD g_dwPerfCounterBaseMs;

// A count of the performance counter in milliseconds, rounded to the nearest.
static inline DWORD CounterToMilliseconds(LARGE_INTEGER count) {
    return (DWORD)((count.QuadPart * 1000 + 500) / g_liPerfCounterFrequency.QuadPart);
}

DWORD GetHighResolutionTime(void) {
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
