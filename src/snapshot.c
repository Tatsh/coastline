#include <PR/os.h>
#include <PR/os_libc.h>

#include "camera.h"
#include "game.h"
#include "input.h"
#include "race.h"
#include "replay.h"
#include "system.h"
#include "timing.h"

/* The room for the message of an overflow. */
#define OVERFLOW_MESSAGE_SIZE 0x50

void SerializeSaveData(SaveField *fields, u8 *buffer, s32 capacity) {
    char message[OVERFLOW_MESSAGE_SIZE];
    s32 used = 0;

    for (; fields->data != NULL; fields++) {
        memcpy(&buffer[used], fields->data, fields->size);
        used += fields->size;
    }
    if (capacity < used) {
        sprintf(message, "VAR SAVE OVERFLOW (%d avail, %d used)", capacity, used);
        SetDebugMessage(message);
    }
}

void DeserializeSaveData(SaveField *fields, u8 *buffer) {
    s32 used = 0;

    for (; fields->data != NULL; fields++) {
        memcpy(fields->data, &buffer[used], fields->size);
        used += fields->size;
    }
}

void SaveRaceSnapshot(LastLapBuffer *buffer) {
    /* The capacity given is that of the whole buffer, not of the room after the recordings. */
    SerializeSaveData(g_aRaceSnapshotFields, buffer->snapshot, sizeof(LastLapBuffer));
}

void RestoreRaceSnapshot(LastLapBuffer *buffer) {
    DeserializeSaveData(g_aRaceSnapshotFields, buffer->snapshot);
}

void SaveClockForPause(void) {
    SerializeSaveData(g_aPausedClockFields, g_abPausedClockState, PAUSED_CLOCK_STATE_SIZE);
}

void RestoreClockAfterPause(void) {
    DeserializeSaveData(g_aPausedClockFields, g_abPausedClockState);
}

void SaveLastLapSnapshot(Vehicle *vehicle) {
    LastLapBuffer *buffer;
    PlayerInputState *input;
    s32 i;

    if (gInstantReplayActive) {
        return;
    }
    if (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_DEMO) {
        return;
    }
    if (vehicle->index >= (s32)g_dwNumLocalPlayers) {
        return;
    }
    if (vehicle->lap + 1 != g_dwTotalLaps && g_dwTotalLaps >= 2) {
        return;
    }
    if (vehicle->input->recordBuffers[vehicle->index] != NULL) {
        return;
    }
    osSyncPrintf("SAVING LAST LAP INFO\n");
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        input = g_aVehicles[i].input;
        input->recordPositions[vehicle->index] = 0;
        g_aVehicles[i].input->recordLimits[vehicle->index] = LAST_LAP_RECORDING_SIZE;
        g_aVehicles[i].input->recordBuffers[vehicle->index] =
            g_aLastLapBuffers[LAST_LAP_FIRST_SLOT - vehicle->index].recordings[i];
    }
    buffer = &g_aLastLapBuffers[LAST_LAP_FIRST_SLOT - vehicle->index];
    SaveRaceSnapshot(buffer);
}
