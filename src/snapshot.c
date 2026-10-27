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

void SaveLastLapSnapshot(Vehicle *vehicle) {
    s32 i;

    if (gInstantReplayActive || g_dwGameMode == GAME_MODE_TIME_ATTACK ||
        g_dwGameMode == GAME_MODE_DEMO || vehicle->index >= (s32)g_dwNumLocalPlayers ||
        (vehicle->lap + 1 != g_dwTotalLaps && g_dwTotalLaps >= 2) ||
        vehicle->input->recordBuffers[vehicle->index] != NULL) {
        return;
    }
    osSyncPrintf("SAVING LAST LAP INFO\n");
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        g_aVehicles[i].input->recordPositions[vehicle->index] = 0;
        g_aVehicles[i].input->recordLimits[vehicle->index] = LAST_LAP_RECORDING_SIZE;
        g_aVehicles[i].input->recordBuffers[vehicle->index] =
            g_aLastLapBuffers[LAST_LAP_FIRST_SLOT - vehicle->index].recordings[0] +
            i * LAST_LAP_RECORDING_SIZE;
    }
    SaveRaceSnapshot(&g_aLastLapBuffers[LAST_LAP_FIRST_SLOT - vehicle->index]);
}
