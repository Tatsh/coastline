#include "debugtext.h"
#include "track.h"

void StepDebugHiddenInstance(void) {
    s32 i;
    s32 step = g_nDebugInstanceCycleStep;
    s32 instanceCount;
    s32 visibleCount;

    if (step == 0) {
        return;
    }
    instanceCount = g_LoadedTrack.instanceCount;
    visibleCount = g_nOpaqueVisibleInstanceCount;
    for (;;) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        g_nDebugHiddenInstanceIndex = (s32)((u32)g_nDebugHiddenInstanceIndex + (u32)step);
#else
        g_nDebugHiddenInstanceIndex += step;
#endif
        if (g_nDebugHiddenInstanceIndex >= instanceCount) {
            g_nDebugHiddenInstanceIndex = 0;
        }
        if (g_nDebugHiddenInstanceIndex < 0) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            g_nDebugHiddenInstanceIndex = (s32)((u32)instanceCount - 1);
#else
            g_nDebugHiddenInstanceIndex = instanceCount - 1;
#endif
        }
        if (g_nDebugHiddenInstanceIndex == 0) {
            break;
        }
        for (i = 0; i < visibleCount; i++) {
            if (g_awVisibleInstanceList[i] == g_nDebugHiddenInstanceIndex) {
                goto found;
            }
        }
    }
found:
    g_nDebugInstanceCycleStep = 0;
}
