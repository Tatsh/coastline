#include <PR/os.h>
#include <PR/os_libc.h>

#include "camera.h"
#include "driving.h"
#include "game.h"
#include "input.h"
#include "race.h"
#include "replay.h"
#include "system.h"
#include "timing.h"

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
#define KEYFRAME_MARKER 0xFF
#define DESERIALIZE(fields, buffer) DeserializeSaveData(fields, buffer, 0)
#endif

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
void RestoreReplayKeyframeState(void) {
    /* The original reserves stack for four bytes of locals it does not use. */
    char unused[4];
    s32 size;

    (void)unused;
    g_dwReplayKeyframeFrameCount++;
    if (g_dwGameMode != GAME_MODE_DEMO || g_dwRaceSessionType != 0) {
        return;
    }
    if (g_aVehicles[0].input->playbackPosition < g_aVehicles[0].input->playbackLimit &&
        g_aVehicles[0].input->playbackBuffer[g_aVehicles[0].input->playbackPosition] ==
            KEYFRAME_MARKER &&
        g_aVehicles[0].input->playbackBuffer[g_aVehicles[0].input->playbackPosition + 1] ==
            KEYFRAME_MARKER) {
        g_aVehicles[0].input->playbackPosition += 2;
        size = DESERIALIZE(g_aReplayKeyframeDescTable1,
                           g_aVehicles[0].input->playbackBuffer +
                               g_aVehicles[0].input->playbackPosition);
        size += DESERIALIZE(g_aReplayKeyframeDescTable2,
                            g_aVehicles[0].input->playbackBuffer +
                                g_aVehicles[0].input->playbackPosition + size);
        size += DESERIALIZE(g_aReplayKeyframeDescTable3,
                            g_aVehicles[0].input->playbackBuffer +
                                g_aVehicles[0].input->playbackPosition + size);
        /* The keyframe is padded to an even length. */
        g_aVehicles[0].input->playbackPosition += (size + 1) & ~1;
        g_aVehicles[0].cameraChanged = 1;
        ChooseReplayCamera(&g_aVehicles[0]);
        UpdateCameraViewBasisAndFov(&g_aVehicles[0],
                                    &g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE]);
    }
}
#endif
