#include <stdbool.h>

#include <SDL3/SDL.h>

#include "common/frameclock.h"

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
