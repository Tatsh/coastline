#include <PR/os.h>

#include "input.h"
#include "system.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "screen.h"
#else
#include "debugtext.h"
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** @ghidraAddress BRD3D: 0x106c6648 */
s32 g_bRaceScreenActive;
/** @ghidraAddress BRD3D: 0x106c56d8 */
s32 g_dwElapsedTimeMs;
/** @ghidraAddress BRD3D: 0x10690a30 */
s32 g_dwFrameDeltaMs;
/** @ghidraAddress BRD3D: 0x106c0204 */
u32 g_dwFrameTimestamp;
/** @ghidraAddress BRD3D: 0x106c64f8 */
s32 g_dwPrevElapsedTimeMs;
/** @ghidraAddress BRD3D: 0x106c2cfc */
f32 g_fFrameDeltaTime;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

/* A race advances by 1/30 of a second, or 1/25 at console PAL rates. */
#if TGR_PLATFORM == TGR_PLATFORM_N64 && TGR_REGION == TGR_REGION_EU
#define RACE_STEP_NANOSECONDS 40000000
#else
#define RACE_STEP_NANOSECONDS 33333333
#endif

u64 g_qwTimestampBase = 0;

void ResetFrameClock(void) {
    g_qwTimestampBase = 0;
    g_dwFrameTimestamp = 0;
    g_dwPrevElapsedTimeMs = g_dwElapsedTimeMs = 0;
    g_dwFrameDeltaMs = 0;
    g_fFrameDeltaTime = 0.0f;
}

void UpdateFrameTiming(void) {
    u32 now;
    u32 delta;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    volatile f32 roundedDelta;
#endif

    if (g_bRaceScreenActive) {
        g_qwTimestampBase += (u64)RACE_STEP_NANOSECONDS * osClockRate / 1000000000;
        g_dwFrameTimestamp = osGetCount();
    } else if (g_qwTimestampBase != 0) {
        now = osGetCount();
        g_qwTimestampBase += (u32)(now - g_dwFrameTimestamp);
        g_dwFrameTimestamp = now;
    } else {
        g_qwTimestampBase = g_dwFrameTimestamp = osGetCount();
    }
    g_dwPrevElapsedTimeMs = g_dwElapsedTimeMs;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    /* Windows discards the upper microsecond word before converting to milliseconds. */
    g_dwElapsedTimeMs = (u32)(g_qwTimestampBase * 1000000 / osClockRate) / 1000;
#else
    g_dwElapsedTimeMs = g_qwTimestampBase * 1000000 / osClockRate / 1000;
#endif
    UpdateControllerState();
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    NullStub();
    NullStub();
    delta = (u32)g_dwElapsedTimeMs - (u32)g_dwPrevElapsedTimeMs;
    roundedDelta = (f32)delta;
    g_fFrameDeltaTime = roundedDelta / 1000.0f;
#else
    DebugFrameStubA();
    DebugFrameStubB();
    delta = g_dwElapsedTimeMs - g_dwPrevElapsedTimeMs;
    g_fFrameDeltaTime = (f32)delta / 1000.0f;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    g_dwFrameDeltaMs = delta;
}
