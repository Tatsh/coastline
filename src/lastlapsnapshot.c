#include <PR/os.h>

#include "game.h"
#include "instantreplay.h"
#include "laps.h"
#include "replay.h"
#include "vehicleaccess.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <string.h>

#include "win32/ghost.h"
#define LAST_LAP_SELF(field) (field)
#define LAST_LAP_NOT_STARTING() (lap != (s32)((u32)g_dwTotalLaps - 1) && g_dwTotalLaps >= 2)
#define LAST_LAP_RECORDING_ACTIVE() IsGhostRecordingActive()

/* Recording writes can change fields reloaded by the next store. */
static inline void StoreLastLapInteger(s32 *destination, s32 value) {
    memcpy(destination, &value, sizeof(value));
}

static inline void StoreLastLapBuffer(u8 **destination, u8 *value) {
    memcpy(destination, &value, sizeof(value));
}

#define LAST_LAP_STORE_INTEGER(field, value) StoreLastLapInteger(&(field), value)
#define LAST_LAP_STORE_BUFFER(field, value) StoreLastLapBuffer(&(field), value)
#else
#define LAST_LAP_SELF(field) (vehicle->field)
#define LAST_LAP_NOT_STARTING() (vehicle->lap + 1 != g_dwTotalLaps && g_dwTotalLaps >= 2)
#define LAST_LAP_RECORDING_ACTIVE() (vehicle->input->recordBuffers[vehicle->index] != NULL)
#define LAST_LAP_STORE_INTEGER(field, value) ((field) = (value))
#define LAST_LAP_STORE_BUFFER(field, value) ((field) = (value))
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
void Vehicle::SaveLastLapSnapshot(void) {
#else
void SaveLastLapSnapshot(Vehicle *vehicle) {
#endif
    s32 i;

    if (gInstantReplayActive || g_dwGameMode == GAME_MODE_TIME_ATTACK ||
        g_dwGameMode == GAME_MODE_DEMO || LAST_LAP_SELF(index) >= (s32)g_dwNumLocalPlayers ||
        LAST_LAP_NOT_STARTING() || LAST_LAP_RECORDING_ACTIVE()) {
        return;
    }
    osSyncPrintf("SAVING LAST LAP INFO\n");
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    StartGhostRecording();
#endif
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        LAST_LAP_STORE_INTEGER(
            VEHICLE_GET(&g_aVehicles[i], Input)->recordPositions[LAST_LAP_SELF(index)], 0);
        LAST_LAP_STORE_INTEGER(
            VEHICLE_GET(&g_aVehicles[i], Input)->recordLimits[LAST_LAP_SELF(index)],
            LAST_LAP_RECORDING_SIZE);
        LAST_LAP_STORE_BUFFER(
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[LAST_LAP_SELF(index)],
            g_aLastLapBuffers[LAST_LAP_FIRST_SLOT - LAST_LAP_SELF(index)].recordings[0] +
                i * LAST_LAP_RECORDING_SIZE);
    }
    SaveRaceSnapshot(&g_aLastLapBuffers[LAST_LAP_FIRST_SLOT - LAST_LAP_SELF(index)]);
}
