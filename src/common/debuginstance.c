#include "common/debugtext.h"
#include "common/track.h"

#if TGR_PC
/**
 * The step StepDebugHiddenInstance() takes through the placed models, or 0 to stay.
 *
 * @ghidraAddress USA: 0x8028dde0
 * @ghidraAddress EUR: 0x80290580
 * @ghidraAddress JPN: 0x80290020
 * @ghidraAddress BRD3D: 0x10277b20
 * @ghidraAddress BRGlide: 0x10273308
 */
static s32 g_nDebugInstanceCycleStep;
#endif /* TGR_PC */

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
#if TGR_PC
        g_nDebugHiddenInstanceIndex = (s32)((u32)g_nDebugHiddenInstanceIndex + (u32)step);
#else
        g_nDebugHiddenInstanceIndex += step;
#endif
        if (g_nDebugHiddenInstanceIndex >= instanceCount) {
            g_nDebugHiddenInstanceIndex = 0;
        }
        if (g_nDebugHiddenInstanceIndex < 0) {
#if TGR_PC
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
