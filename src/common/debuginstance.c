#include "common/debugtext.h"
#include "common/track.h"

#if TGR_BOSS_RALLY
/**
 * The step StepDebugHiddenInstance() takes through the placed models, or 0 to stay.
 *
 * @ghidraAddress USA: 0x8028dde0
 * @ghidraAddress Asia: 0x8028D930
 * @ghidraAddress EUR: 0x80290580
 * @ghidraAddress JPN: 0x80290020
 * @ghidraAddress BRD3D: 0x10277b20
 * @ghidraAddress BRGlide: 0x10273308
 */
static s32 g_nDebugInstanceCycleStep;
#endif /* TGR_BOSS_RALLY */

void StepDebugHiddenInstance(void) {
    s32 i;

    if (g_nDebugInstanceCycleStep != 0) {
        for (;;) {
#if TGR_BOSS_RALLY
            g_nDebugHiddenInstanceIndex =
                (s32)((u32)g_nDebugHiddenInstanceIndex + (u32)g_nDebugInstanceCycleStep);
#else
            g_nDebugHiddenInstanceIndex += g_nDebugInstanceCycleStep;
#endif
            if (g_nDebugHiddenInstanceIndex >= g_LoadedTrack.instanceCount) {
                g_nDebugHiddenInstanceIndex = 0;
            }
            if (g_nDebugHiddenInstanceIndex < 0) {
#if TGR_BOSS_RALLY
                g_nDebugHiddenInstanceIndex = (s32)((u32)g_LoadedTrack.instanceCount - 1);
#else
                g_nDebugHiddenInstanceIndex = g_LoadedTrack.instanceCount - 1;
#endif
            }
            if (g_nDebugHiddenInstanceIndex == 0) {
                break;
            }
            for (i = 0; i < g_nOpaqueVisibleInstanceCount; i++) {
                if (g_nDebugHiddenInstanceIndex == g_awVisibleInstanceList[i]) {
                    goto found;
                }
            }
        }
    found:
        g_nDebugInstanceCycleStep = 0;
    }
}
