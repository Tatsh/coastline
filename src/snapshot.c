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

/* The room for the message of an overflow. */
#define OVERFLOW_MESSAGE_SIZE 0x50

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
/* The marker of a keyframe in a recording of the input. */
#define KEYFRAME_MARKER 0xFF

/* The snapshots copy the variables themselves, at an offset of 0. */
#define SERIALIZE(fields, buffer, capacity) SerializeSaveData(fields, buffer, capacity, 0)
#define DESERIALIZE(fields, buffer) DeserializeSaveData(fields, buffer, 0)

s32 SerializeSaveData(SaveField *fields, u8 *buffer, s32 capacity, s32 offset) {
    s32 i;
    u8 *start = buffer;
    s32 used;
    char message[OVERFLOW_MESSAGE_SIZE];

    for (i = 0; fields[i].data != NULL; i++) {
        memcpy(buffer, (u8 *)fields[i].data + offset, fields[i].size);
        buffer += fields[i].size;
    }
    used = buffer - start;
    if (capacity < used) {
        sprintf(message, "VAR SAVE OVERFLOW (%d avail, %d used)", capacity, used);
        SetDebugMessage(message);
    }
    return used;
}

s32 DeserializeSaveData(SaveField *fields, u8 *buffer, s32 offset) {
    s32 i;
    u8 *start = buffer;

    for (i = 0; fields[i].data != NULL; i++) {
        memcpy((u8 *)fields[i].data + offset, buffer, fields[i].size);
        buffer += fields[i].size;
    }
    return buffer - start;
}
#else
#define SERIALIZE(fields, buffer, capacity) SerializeSaveData(fields, buffer, capacity)
#define DESERIALIZE(fields, buffer) DeserializeSaveData(fields, buffer)

void SerializeSaveData(SaveField *fields, u8 *buffer, s32 capacity) {
    s32 i;
    u8 *start = buffer;
    char message[OVERFLOW_MESSAGE_SIZE];

    for (i = 0; fields[i].data != NULL; i++) {
        memcpy(buffer, fields[i].data, fields[i].size);
        buffer += fields[i].size;
    }
    if (capacity < buffer - start) {
        sprintf(message, "VAR SAVE OVERFLOW (%d avail, %d used)", capacity, buffer - start);
        SetDebugMessage(message);
    }
}

void DeserializeSaveData(SaveField *fields, u8 *buffer) {
    s32 i;

    for (i = 0; fields[i].data != NULL; i++) {
        memcpy(fields[i].data, buffer, fields[i].size);
        buffer += fields[i].size;
    }
}
#endif

void SaveRaceSnapshot(LastLapBuffer *buffer) {
    /* The capacity given is that of the whole buffer, not of the room after the recordings. */
    SERIALIZE(g_aRaceSnapshotFields, buffer->snapshot, sizeof(LastLapBuffer));
}

void RestoreRaceSnapshot(LastLapBuffer *buffer) {
    DESERIALIZE(g_aRaceSnapshotFields, buffer->snapshot);
}

void SaveClockForPause(void) {
    SERIALIZE(g_aPausedClockFields, g_abPausedClockState, PAUSED_CLOCK_STATE_SIZE);
}

void RestoreClockAfterPause(void) {
    DESERIALIZE(g_aPausedClockFields, g_abPausedClockState);
}

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
void RestoreReplayKeyframeState(void) {
    PlayerInputState *input;
    s32 size;

    g_dwReplayKeyframeFrameCount++;
    if (g_dwGameMode != GAME_MODE_DEMO || g_dwRaceSessionType != 0) {
        return;
    }
    input = g_aVehicles[0].input;
    if (input->playbackPosition < input->playbackLimit &&
        input->playbackBuffer[input->playbackPosition] == KEYFRAME_MARKER &&
        input->playbackBuffer[input->playbackPosition + 1] == KEYFRAME_MARKER) {
        input->playbackPosition += 2;
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
