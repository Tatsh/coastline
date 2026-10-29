#include "common/raceinit.h"

#include <PR/os.h>

#include "common/camera.h"
#include "common/game.h"
#include "common/instantreplay.h"
#include "common/race.h"
#include "common/replay.h"
#include "common/soundeffect.h"
#include "common/trackselect.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#include "common/weather.h"

#if TGR_BOSS_RALLY
#include <string.h>

#include "win32/directplay.h"
#include "win32/ghost.h"
#include "win32/rallyconfig.h"
#include "win32/settings.h"
#include "win32/sound.h"

#else
#include <PR/os_libc.h>

#include "common/assetdma.h"
#include "common/audiomix.h"
#endif /* TGR_BOSS_RALLY */

#if TGR_BOSS_RALLY
/**
 * The next of the two attract recordings to load.
 *
 * @ghidraAddress BRD3D: 0x106909f4
 * @ghidraAddress BRGlide: 0x105ccb9c
 */
static s32 g_dwAttractDemoIndex;
#endif

#define CHAMPIONSHIP_VEHICLES 20
#define CHAMPIONSHIP_ACTIVE_VEHICLES 3
#define RACE_SOUND_CHANNELS 6

#define ROM_CREDITS_RECORDING ((u32)_creditsRecordingSegmentRomStart)
#define ROM_ENDING_RECORDING ((u32)_endingRecordingSegmentRomStart)

#if TGR_BOSS_RALLY
#define RECORDING_SELF this
#define RECORDING_OPTION(offset) ((s32)(s8)input->playbackBuffer[offset])
#define RECORDING_TRACK(byte) ((s32)(s8)(byte))
void Vehicle::SetupFromRecording(void) {
#else
#define RECORDING_SELF vehicle
#define RECORDING_OPTION(offset) (vehicle->input->playbackBuffer[offset])
#define RECORDING_TRACK(byte) (byte)
void SetupFromRecording(Vehicle *vehicle) {
#endif /* TGR_BOSS_RALLY */
    RECORDING_SELF->steeringMode = RECORDING_OPTION(2);
    RECORDING_SELF->chosenTransmission = RECORDING_OPTION(3);
    RECORDING_SELF->tireType = RECORDING_OPTION(4);
    RECORDING_SELF->suspensionStiffness = RECORDING_OPTION(5);
    RECORDING_SELF->input->controllerLayout = RECORDING_SELF->input->playbackBuffer[6];
}

/* Set up the vehicles, the recordings and the replays of the mode of play. */
void SetupRaceVehicles(void) {
    Vehicle *vehicle;
    PlayerInputState *ghost;
    u8 *recording;
    s32 i;

    switch (g_dwGameMode) {
    case GAME_MODE_CHAMPIONSHIP:
        g_dwNumVehicles = CHAMPIONSHIP_VEHICLES;
        g_dwNumActiveVehicles = CHAMPIONSHIP_ACTIVE_VEHICLES;
#if TGR_BOSS_RALLY
        g_aVehicles[0].SetVehicleCarId(g_dwChosenCar);
#endif
        goto replays;
    case GAME_MODE_ARCADE:
        g_dwNumVehicles = 2;
        g_dwNumActiveVehicles = 2;
#if TGR_BOSS_RALLY
        g_aVehicles[0].SetVehicleCarId(g_dwChosenCar);
        g_aVehicles[1].SetVehicleCarId(g_dwChosenCar);
        g_aVehicles[1].SetCustomCarModel(0);
#endif
    replays:
        if (gInstantReplayActive) {
            /* The replay starts from the snapshot of the last lap of the best player. */
            RestoreRaceSnapshot(&g_aLastLapBuffers[LAST_LAP_FIRST_SLOT - g_dwBestReplayPlayer]);
#if TGR_BOSS_RALLY
            for (i = 0; i < SOUND_CHANNEL_COUNT; i++) {
                g_aSoundChannelRequested[i].dwReserved = 0;
                g_aSoundChannelRequested[i].dwStereoLevels = 0;
            }
#else
            for (i = 0; i < RACE_SOUND_CHANNELS; i++) {
                g_aSoundEffectChannels[i].volume = 0;
                g_aSoundEffectChannels[i].mixVolume = 0;
            }
            for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
                VEHICLE_GET(&g_aVehicles[i], Input)->playbackBuffer = g_apReplayGhostPtrs[i];
                VEHICLE_GET(&g_aVehicles[i], Input)->playbackLimit = g_adwReplayGhostSizes[i];
                VEHICLE_GET(&g_aVehicles[i], Input)->playbackPosition = 0;
            }
#endif /* TGR_BOSS_RALLY */
        } else {
            for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
                VEHICLE_GET(&g_aVehicles[i], Input)->playbackBuffer = NULL;
                VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[0] = NULL;
                VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[1] = NULL;
            }
        }
        g_bReplayAfterRace = gInstantReplayActive == 0;
        break;

#if TGR_BOSS_RALLY
    case GAME_MODE_NETWORK:
        if (gInstantReplayActive) {
            goto replays;
        }
        g_dwNumLocalPlayers = 1;
        g_dwNumVehicles = g_dwNumActiveVehicles = g_bNetSpectator ? 0 : 1;
        g_aVehicles[0].SetVehicleCarId(g_dwChosenCar);
        if (g_dwNetworkPlayMode) {
            ResetNetworkState();
            EnableNetworkReceive();
            WaitForPlayerIdAssignment();
            g_aVehicles[0].SetGridIndex(GetLocalPlayerIndex());
            strcpy(g_aVehicles[0].GetPlayerName(), g_GlobalRallyConfig.GetPlayerName());
            CopyPlayerNameToSlotBuffer(g_aVehicles[0].GetGridIndex(),
                                       g_GlobalRallyConfig.GetPlayerName());
        }
        {
            unsigned players = GetDirectPlaySessionPlayerCount();
            while ((u32)g_dwNumActiveVehicles < players) {
                const s32 slot = PopJoinedPlayerSlot();
                if (slot >= 0) {
                    AddNetworkPlayerSlot(slot);
                }
                players = GetDirectPlaySessionPlayerCount();
            }
        }
        goto replays;
#endif /* TGR_BOSS_RALLY */
    case GAME_MODE_AFTER_SEASON:
        g_dwNumLocalPlayers = 1;
        g_dwNumVehicles = 1;
        g_dwNumActiveVehicles = 1;
        g_bReplayAfterRace = FALSE;
        gInstantReplayActive = FALSE;
#if TGR_BOSS_RALLY
        g_aVehicles[0].SetCustomDecals(0);
#else
        g_aVehicles[0].customDecals = TRUE;
#endif
        VEHICLE_SET(&g_aVehicles[0],
                    ActiveCamera,
                    &VEHICLE_GET(&g_aVehicles[0], Cameras)[VEHICLE_CAMERA_CHASE]);
        g_dwReplayCameraSwitchTimer = 0;
        break;

    case GAME_MODE_DEMO:
        g_dwCurrentWeather = 1;
        g_dwNumLocalPlayers = 1;
        g_dwNumVehicles = 1;
        g_dwNumActiveVehicles = 1;
        g_bReplayAfterRace = FALSE;
        VEHICLE_SET(&g_aVehicles[0], CustomDecals, FALSE);
#if TGR_BOSS_RALLY
        VEHICLE_GET(&g_aVehicles[0], Input)->playbackBuffer = (u8 *)&g_GhostRecordingInfo;
        switch (g_dwRaceSessionType) {
        case RACE_SESSION_SCROLLING_CREDITS:
            CalculateScrollPositions();
            ReadGhostFileData("RallyOutro.dat", false);
            goto loadedDemo;
        case RACE_SESSION_CREDIT_CAPTIONS:
            ReadGhostFileData("RallyCredits.dat", false);
            goto loadedDemo;
        case RACE_SESSION_ATTRACT:
            ReadGhostFileData(g_dwAttractDemoIndex == 0 ? "RallyIntro1.dat" : "RallyIntro2.dat",
                              false);
            g_dwAttractDemoIndex = (s32)((u32)g_dwAttractDemoIndex + 1);
            if (g_dwAttractDemoIndex > 1) {
                g_dwAttractDemoIndex = 0;
            }
        loadedDemo:
            ClearGhostPlaybackState();
            g_bDemoInterrupted = FALSE;
            break;
        }
#else
        VEHICLE_GET(&g_aVehicles[0], Input)->playbackBuffer = g_abPlaybackRecording;
        switch (g_dwRaceSessionType) {
        case RACE_SESSION_ATTRACT:
            VEHICLE_GET(&g_aVehicles[0], Input)->playbackLimit =
                DmaReadAssetSize(ROM_GHOST_RECORDING);
            DecompressFromRom(
                VEHICLE_GET(&g_aVehicles[0], Input)->playbackBuffer, ROM_GHOST_RECORDING, NULL);
            break;
        case RACE_SESSION_CREDIT_CAPTIONS:
            VEHICLE_GET(&g_aVehicles[0], Input)->playbackLimit =
                DmaReadAssetSize(ROM_CREDITS_RECORDING);
            DecompressFromRom(
                VEHICLE_GET(&g_aVehicles[0], Input)->playbackBuffer, ROM_CREDITS_RECORDING, NULL);
            break;
        case RACE_SESSION_SCROLLING_CREDITS:
            CalculateScrollPositions();
            VEHICLE_GET(&g_aVehicles[0], Input)->playbackLimit =
                DmaReadAssetSize(ROM_ENDING_RECORDING);
            DecompressFromRom(
                VEHICLE_GET(&g_aVehicles[0], Input)->playbackBuffer, ROM_ENDING_RECORDING, NULL);
            break;
        }
        VEHICLE_GET(&g_aVehicles[0], Input)->playbackPosition = RECORDING_HEADER_SIZE;
#endif /* TGR_BOSS_RALLY */
        VEHICLE_GET(&g_aVehicles[0], Input)->recordBuffers[0] = NULL;
        VEHICLE_GET(&g_aVehicles[0], Input)->recordBuffers[1] = NULL;
        vehicle = &g_aVehicles[0];
        VEHICLE_GET(vehicle, Color)[0] = 0xff;
        VEHICLE_GET(vehicle, Color)[1] = 0xff;
        VEHICLE_GET(vehicle, Color)[2] = 0xff;
#if TGR_BOSS_RALLY
        g_dwSelectedTrackId = (s8)VEHICLE_GET(&g_aVehicles[0], Input)->playbackBuffer[0];
        {
            const s32 car = (s8)VEHICLE_GET(&g_aVehicles[0], Input)->playbackBuffer[1];
            g_aVehicles[0].SetSelectedCar(car);
            g_aVehicles[0].SetVehicleCarId(car);
        }
        g_aVehicles[0].SetupFromRecording();
        g_dwChosenWeather = (s8)VEHICLE_GET(&g_aVehicles[0], Input)->playbackBuffer[7];
        g_dwCurrentWeather = g_dwChosenWeather;
#else
        g_dwSelectedTrackId = VEHICLE_GET(&g_aVehicles[0], Input)->playbackBuffer[0];
        g_aVehicles[0].carModel = g_aVehicles[0].selectedCar =
            VEHICLE_GET(&g_aVehicles[0], Input)->playbackBuffer[1];
        SetupFromRecording(&g_aVehicles[0]);
#endif /* TGR_BOSS_RALLY */
        break;

    case GAME_MODE_TIME_ATTACK:
#if TGR_BOSS_RALLY
        g_aVehicles[0].SetVehicleCarId(g_dwChosenCar);
#else
        /* The strip mine and its mirror are raced in the fog. */
        if (g_dwSelectedTrackId == TRACK_STRIP_MINE ||
            g_dwSelectedTrackId == TRACK_MIRROR_STRIP_MINE) {
            g_dwCurrentWeather = 1;
        } else {
            g_dwCurrentWeather = 0;
        }
#endif /* TGR_BOSS_RALLY */
        g_dwNumVehicles = g_dwNumActiveVehicles = g_dwNumLocalPlayers + 1;
        ghost = VEHICLE_GET(&g_aVehicles[g_dwNumLocalPlayers], Input);
        g_bReplayAfterRace = FALSE;
#if TGR_BOSS_RALLY
        g_dwSaveAvailable = TRUE;
#else
        g_dwSaveAvailable = FALSE;
#endif
        ghost->playbackBuffer = g_abPlaybackRecording;
        osSyncPrintf("savedRecordingLength = %d\n", g_dwGhostRecordingSize);
        memcpy(VEHICLE_GET(&g_aVehicles[g_dwNumLocalPlayers], Input)->playbackBuffer,
#if TGR_BOSS_RALLY
               &g_GhostRecordingInfo,
#else
               g_abGhostRecording,
#endif
               g_dwGhostRecordingSize);
        recording = VEHICLE_GET(&g_aVehicles[g_dwNumLocalPlayers], Input)->playbackBuffer;
#if TGR_BOSS_RALLY
        {
            const s32 car = (s8)recording[1];
            g_aVehicles[g_dwNumLocalPlayers].SetSelectedCar(car);
            g_aVehicles[g_dwNumLocalPlayers].SetVehicleCarId(car);
        }
        ClearGhostPlaybackState();
        if ((s32)(s8)VEHICLE_GET(&g_aVehicles[g_dwNumLocalPlayers], Input)->playbackBuffer[0] !=
                (s32)g_dwSelectedTrackId ||
            (s32)(s8)VEHICLE_GET(&g_aVehicles[g_dwNumLocalPlayers], Input)->playbackBuffer[7] !=
                g_dwChosenWeather) {
#else
        g_aVehicles[g_dwNumLocalPlayers].carModel = g_aVehicles[g_dwNumLocalPlayers].selectedCar =
            recording[1];
        if (VEHICLE_GET(&g_aVehicles[g_dwNumLocalPlayers], Input)->playbackBuffer[0] !=
            g_dwSelectedTrackId) {
#endif /* TGR_BOSS_RALLY */
            osSyncPrintf(
                "WRONG TRACK (%d!=%d)\n",
                RECORDING_TRACK(
                    VEHICLE_GET(&g_aVehicles[g_dwNumLocalPlayers], Input)->playbackBuffer[0]),
                g_dwSelectedTrackId);
            VEHICLE_GET(&g_aVehicles[g_dwNumLocalPlayers], Input)->playbackBuffer = NULL;
#if TGR_BOSS_RALLY
            g_dwNumActiveVehicles = 1;
            g_dwNumVehicles = 1;
#endif
        } else {
            osSyncPrintf(
                "TRACK=%d\n",
                RECORDING_TRACK(
                    VEHICLE_GET(&g_aVehicles[g_dwNumLocalPlayers], Input)->playbackBuffer[0]));
            VEHICLE_GET(&g_aVehicles[g_dwNumLocalPlayers], Input)->playbackPosition =
                RECORDING_HEADER_SIZE;
            osSyncPrintf("PLAYLIMIT=%d\n", g_dwGhostRecordingSize);
            VEHICLE_GET(&g_aVehicles[g_dwNumLocalPlayers], Input)->playbackLimit =
                g_dwGhostRecordingSize;
        }
        /* Each player records into a buffer of their own, under the same index. */
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[i] = g_aabInputRecordings[i];
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[i][0] = g_dwSelectedTrackId;
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[i][1] =
                VEHICLE_GET(&g_aVehicles[i], CarModel);
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[i][2] =
                VEHICLE_GET(&g_aVehicles[i], Profile)->handling;
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[i][3] =
                VEHICLE_GET(&g_aVehicles[i], Profile)->transmission;
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[i][4] =
                VEHICLE_GET(&g_aVehicles[i], Profile)->tires;
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[i][5] =
                VEHICLE_GET(&g_aVehicles[i], Profile)->suspension;
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[i][6] =
                VEHICLE_GET(&g_aVehicles[i], Profile)->controllerLayout;
#if TGR_BOSS_RALLY
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[i][7] = g_dwChosenWeather;
#endif
            VEHICLE_GET(&g_aVehicles[i], Input)->recordPositions[i] = RECORDING_HEADER_SIZE;
            VEHICLE_GET(&g_aVehicles[i], Input)->recordLimits[i] = RECORDING_CAPACITY;
        }
        break;

    default:
#if TGR_BOSS_RALLY
        g_aVehicles[0].SetVehicleCarId(g_dwChosenCar);
#endif
        g_dwNumVehicles = g_dwNumLocalPlayers;
        g_dwNumActiveVehicles = g_dwNumLocalPlayers;
        g_bReplayAfterRace = FALSE;
        break;
    }
}
