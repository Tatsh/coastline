#include "racephases.h"

#include <PR/os.h>

#include "fade.h"
#include "frametime.h"
#include "game.h"
#include "instantreplay.h"
#include "laptime.h"
#include "racerslotaccess.h"
#include "recordingformat.h"
#include "soundeffect.h"
#include "trackmenuitem.h"
#include "trackselect.h"
#include "vehicleaccess.h"
#include "vehiclecount.h"
#include "weather.h"
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include <string.h>

#include "platform/win/boot/strings.h"
#include "platform/win/directplay.h"
#include "platform/win/ghost.h"
#include "platform/win/input.h"
#include "platform/win/settings.h"
#else
#include <PR/os_libc.h>

#include "replay.h"
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
namespace {
    /**
     * The step of the countdown of the start lights.
     *
     * @ghidraAddress USA: 0x802f7ef4
     * @ghidraAddress EUR: 0x802f9be8
     * @ghidraAddress JPN: 0x802f8628
     * @ghidraAddress BRD3D: 0x106909cc
     */
    s32 g_dwStartLightsStep;
} // namespace
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** @ghidraAddress BRD3D: 0x106909e0 */
s32 gInstantReplayActive;
/** @ghidraAddress BRD3D: 0x10680938 */
s32 g_bDemoInterrupted;
/** @ghidraAddress BRD3D: 0x106909e4 */
s32 g_bReplayAfterRace;
/** @ghidraAddress BRD3D: 0x106909d0 */
s32 g_bStartLightsVisible;
/** @ghidraAddress BRD3D: 0x106805c0 */
s32 g_dwRaceCaptionIndex;
/** @ghidraAddress BRD3D: 0x106909f0 */
s32 g_dwRaceExitState;
/** @ghidraAddress BRD3D: 0x118ab470 */
s32 g_dwRaceFinishOrderCounter;
/** @ghidraAddress BRD3D: 0x106909ec */
s32 g_dwRaceInitialized;
/** @ghidraAddress BRD3D: 0x10680750 */
s32 g_dwRacePhase;
/** @ghidraAddress BRD3D: 0x106805a8 */
s32 g_dwRacePhaseIndex;
/** @ghidraAddress BRD3D: 0x10680610 */
s32 g_dwSaveAvailable;
/** @ghidraAddress BRD3D: 0x106909b8 */
s32 g_dwSavedResultsValid;
/** @ghidraAddress BRD3D: 0x106806dc */
f32 g_fRaceCaptionTimer;
/** @ghidraAddress BRD3D: 0x106806d8 */
f32 g_fRacePhaseTimer;
/** @ghidraAddress BRD3D: 0x106805bc */
f32 g_fStartLightsDrop;
/** @ghidraAddress BRD3D: 0x11782bc0 */
s16 g_wRaceScreenLoopExited;
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define RACE_GHOST_TRACK ((u32)(s32)g_GhostRecordingInfo.track)
#else
#define RACE_GHOST_TRACK g_abGhostRecording[0]
#endif
#define FADE_SECONDS 0.2f
#if TGR_PLATFORM == TGR_PLATFORM_WIN || TGR_REGION == TGR_REGION_US
#define ATTRACT_RACE_SECONDS 39.6f
#define ENDING_RACE_SECONDS 140.0f
#else
#define CREDITS_RACE_SECONDS 42.0f
#define ENDING_RACE_SECONDS 170.0f
#endif
#define AFTER_SEASON_RACE_SECONDS 16.0f

/* Go on to the next phase of the race. */
void AdvanceRacePhase(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (!g_bNetRaceStartReached) {
        return;
    }
#endif
    g_dwRacePhaseIndex++;
    g_dwRacePhase = g_aRacePhaseTable[g_dwRacePhaseIndex].phase;
    g_fRacePhaseTimer = g_aRacePhaseTable[g_dwRacePhaseIndex].seconds;
}

/* Hold the vehicles and run the start lights before the race starts. */
static void HoldBeforeStart(void) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    const TrackMenuItem *track;
#else
    TrackMenuItem *track;
#endif
    Vehicle *vehicle;
    s16 weather;
    s32 i;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
    if (g_bNetRaceStartReached) {
        const s32 localPlayers = (s32)g_dwNumLocalPlayers;
        for (i = 0; i < localPlayers; i++) {
            VEHICLE_SET(&g_aVehicles[i], SubMessage, NULL);
            VEHICLE_SET(&g_aVehicles[i], SubMessageTime, 0.01f);
        }
    } else {
        const unsigned messageId = g_dwNetworkPlayMode && g_bNetReadyToStartSent ? 0xed : 0xee;
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            VEHICLE_SET(&g_aVehicles[i], SubMessage, GetStringResource(messageId));
            VEHICLE_SET(&g_aVehicles[i], SubMessageTime, 0.01f);
        }
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    g_bStartLightsVisible = TRUE;
    for (i = 0; i < g_dwNumVehicles; i++) {
        RACER_SLOT_SET(
            &g_aRacerSlots[i], Status, RACER_SLOT_GET(&g_aRacerSlots[i], Status) | RACER_HELD);
        vehicle = RACER_SLOT_GET(&g_aRacerSlots[i], Vehicle);
        if (vehicle == NULL || VEHICLE_GET(vehicle, Index) >= (s32)g_dwNumLocalPlayers) {
            continue;
        }
        if (g_dwGameMode == GAME_MODE_ARCADE
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            || g_dwGameMode == GAME_MODE_NETWORK
#endif
        ) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            weather = (s16)((u32)g_dwCurrentWeather - 1);
#else
            weather = g_dwCurrentWeather - 1;
#endif
            if (weather >= ARCADE_WEATHERS || weather < 0) {
                weather = 0;
            }
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            track = g_apTrackMenuItems[g_dwSelectedTrackId];
#else
            /* The descriptor of a track starts with its menu item. */
            track = (TrackMenuItem *)g_apTrackMenuItems[g_dwSelectedTrackId];
#endif
            VEHICLE_SET(vehicle,
                        TimeLeft,
                        track->arcadeTimes[VEHICLE_GET(vehicle, TableIndex)][weather].startTime);
        }
        VEHICLE_SET(vehicle, MessageTime, 1.0f);
        /* The countdown runs once for each local player, so two players step it twice a frame. */
        switch (g_dwRacePhase) {
        case RACE_PHASE_HOLD:
            g_dwStartLightsAnimation = -1;
            g_dwStartLightsStep = 0;
            break;
        case RACE_PHASE_WAIT:
            /* The original tests for the phase and does nothing in it. */
            break;
        case RACE_PHASE_COUNTDOWN:
            g_dwStartLightsAnimation = 1;
            if (g_fRacePhaseTimer < g_afStartLightThresholds[g_dwStartLightsStep]) {
                g_dwStartLightsStep++;
                if (g_dwStartLightsStep == 4) {
                    PlayGoSound();
                } else {
                    PlayBeepSound();
                }
            }
            break;
        }
    }
    g_fStartLightsDrop = 0.0f;
}

/* Keep a time attack's recording of the first player when it beats the ghost, or when there is
 * none worth keeping. */
static void KeepTimeAttackRecording(s32 player) {
#if TGR_PLATFORM != TGR_PLATFORM_WIN
    s32 j;

    osSyncPrintf("# Format SB BitSize 24 Rows %d Columns 1\n",
                 VEHICLE_GET(&g_aVehicles[player], Input)->recordPositions[player] >> 1);
    for (j = 0; j < VEHICLE_GET(&g_aVehicles[player], Input)->recordPositions[player]; j += 2) {
        osSyncPrintf("%.02x %.02x\n",
                     VEHICLE_GET(&g_aVehicles[player], Input)->recordBuffers[player][j],
                     VEHICLE_GET(&g_aVehicles[player], Input)->recordBuffers[player][j + 1]);
    }
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */
    if (player == 0) {
        if (
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            VEHICLE_GET(&g_aVehicles[player], RacePosition) == 0 ||
            RACE_GHOST_TRACK != g_dwSelectedTrackId ||
#else
            VEHICLE_GET(&g_aVehicles[player], Input)->recordPositions[player] <
                (s32)g_dwGhostRecordingSize ||
            RACE_GHOST_TRACK != g_dwSelectedTrackId ||
#endif
            (s32)g_dwGhostRecordingSize <= RECORDING_HEADER_SIZE) {
            osSyncPrintf("%d: %d<%d=%d || %d!=%d=%d || %d<=%d=%d\n",
                         player,
                         VEHICLE_GET(&g_aVehicles[player], Input)->recordPositions[player],
                         g_dwGhostRecordingSize,
                         VEHICLE_GET(&g_aVehicles[player], Input)->recordPositions[player] <
                             (s32)g_dwGhostRecordingSize,
                         RACE_GHOST_TRACK,
                         g_dwSelectedTrackId,
                         RACE_GHOST_TRACK != g_dwSelectedTrackId,
                         g_dwGhostRecordingSize,
                         RECORDING_HEADER_SIZE,
                         (s32)g_dwGhostRecordingSize <= RECORDING_HEADER_SIZE);
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            g_dwGhostRecordingSize = sizeof(g_GhostRecordingInfo);
            memcpy(&g_GhostRecordingInfo,
                   VEHICLE_GET(&g_aVehicles[player], Input)->recordBuffers[player],
                   RECORDING_HEADER_SIZE);
            g_GhostRecordingInfo.reserved[0] = 0;
            g_GhostRecordingInfo.reserved[1] = 0;
#else
            g_dwGhostRecordingSize =
                VEHICLE_GET(&g_aVehicles[player], Input)->recordPositions[player];
            memcpy(g_abGhostRecording,
                   VEHICLE_GET(&g_aVehicles[player], Input)->recordBuffers[player],
                   g_dwGhostRecordingSize);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            VEHICLE_SET(&g_aVehicles[player], Message, GetStringResource(0xef));
#else
            VEHICLE_SET(&g_aVehicles[player], Message, "NEW RECORD!");
#endif
            VEHICLE_SET(&g_aVehicles[player], MessageTime, 1.0f);
            FormatLapTime(VEHICLE_GET(&g_aVehicles[player], MessageText),
                          VEHICLE_GET(&g_aVehicles[player], TotalTime));
            VEHICLE_SET(
                &g_aVehicles[player], SubMessage, VEHICLE_GET(&g_aVehicles[player], MessageText));
            VEHICLE_SET(&g_aVehicles[player], SubMessageTime, 1.0f);
            g_dwSaveAvailable = TRUE;
        }
    }
    VEHICLE_GET(&g_aVehicles[player], Input)->recordBuffers[player] = NULL;
}

/* Run the race, and tell whether to go on to the next phase. */
static s32 RunRacingPhase(void) {
    s32 allFinished, i, j;

    allFinished = TRUE;
#if TGR_PLATFORM == TGR_PLATFORM_WIN || TGR_REGION == TGR_REGION_US
    if ((g_dwGameMode == GAME_MODE_DEMO &&
         ((g_dwRaceSessionType == RACE_SESSION_SCROLLING_CREDITS &&
           ENDING_RACE_SECONDS < VEHICLE_GET(&g_aVehicles[0], TotalTime)) ||
          (g_dwRaceSessionType != RACE_SESSION_SCROLLING_CREDITS &&
           ATTRACT_RACE_SECONDS < VEHICLE_GET(&g_aVehicles[0], TotalTime)))) ||
#else
    /* The demonstration race ends with its recording. */
    if ((g_dwGameMode == GAME_MODE_DEMO &&
         ((g_dwRaceSessionType == RACE_SESSION_SCROLLING_CREDITS &&
           ENDING_RACE_SECONDS < VEHICLE_GET(&g_aVehicles[0], TotalTime)) ||
          (g_dwRaceSessionType == RACE_SESSION_CREDIT_CAPTIONS &&
           CREDITS_RACE_SECONDS < VEHICLE_GET(&g_aVehicles[0], TotalTime)) ||
          (g_dwRaceSessionType == RACE_SESSION_ATTRACT &&
           VEHICLE_GET(&g_aVehicles[0], Input)->playbackPosition ==
               VEHICLE_GET(&g_aVehicles[0], Input)->playbackLimit))) ||
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN || TGR_REGION == TGR_REGION_US */
        (g_dwGameMode == GAME_MODE_AFTER_SEASON &&
         AFTER_SEASON_RACE_SECONDS < VEHICLE_GET(&g_aVehicles[0], TotalTime))) {
        if (!IsFadingOut()) {
            SetFadeTarget(0.0f, FADE_SECONDS);
            FadeSfxTo(0.0f, FADE_SECONDS);
            g_dwRaceExitState = TRUE;
            gInstantReplayActive = FALSE;
            g_bReplayAfterRace = FALSE;
            allFinished = FALSE;
        }
    }
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        if (!(RACER_SLOT_GET(&g_aRacerSlots[i], Status) & RACER_FINISHED)) {
            allFinished = FALSE;
            continue;
        }
        if (g_dwGameMode == GAME_MODE_TIME_ATTACK &&
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[i] != NULL) {
            KeepTimeAttackRecording(i);
        } else if (g_dwGameMode == GAME_MODE_ARCADE
#if TGR_PLATFORM == TGR_PLATFORM_WIN
                   || g_dwGameMode == GAME_MODE_NETWORK
#endif
        ) {
            /* A finished player records no further. */
            for (j = 0; j < (s32)g_dwNumLocalPlayers; j++) {
                VEHICLE_GET(&g_aVehicles[i], Input)->recordLimits[j] =
                    VEHICLE_GET(&g_aVehicles[i], Input)->recordPositions[j];
            }
        }
    }
    if (allFinished) {
        return TRUE;
    }
#if TGR_PLATFORM != TGR_PLATFORM_WIN
    if (g_dwGameMode == GAME_MODE_ARCADE && g_dwNumLocalPlayers == 1 &&
        (f64)VEHICLE_GET(&g_aVehicles[0], TimeLeft) == 0.0) {
        g_bReplayAfterRace = FALSE;
        g_aVehicles[1].message = g_aVehicles[0].message = "%ryTIME UP!";
        g_aVehicles[1].messageTime = 2.0f;
        g_aVehicles[0].messageTime = g_aVehicles[1].messageTime;
        return TRUE;
    }
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */
    return FALSE;
}

/* Step the phases of the race. */
void StepRacePhases(void) {
    s32 i;
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double timer;
#endif

    /* The phases share the countdown of the timer and the step to the next phase. */
    if (g_dwRacePhase < RACE_PHASE_GO) {
        HoldBeforeStart();
    tick:
        if (!g_dwRacePauseState) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
            timer = (long double)g_fRacePhaseTimer - g_fFrameDeltaTime;
            g_fRacePhaseTimer = (f32)timer;
            if (!(timer >= 0.0L)) {
#else
            g_fRacePhaseTimer -= g_fFrameDeltaTime;
            if (g_fRacePhaseTimer < 0.0f) {
#endif
            advance:
                AdvanceRacePhase();
            }
        }
    } else if (g_dwRacePhase == RACE_PHASE_GO) {
        g_dwStartLightsAnimation = 1;
        g_bStartLightsVisible = TRUE;
        for (i = 0; i < g_dwNumVehicles; i++) {
            RACER_SLOT_SET(
                &g_aRacerSlots[i], Status, RACER_SLOT_GET(&g_aRacerSlots[i], Status) & ~RACER_HELD);
        }
        /* The lights rise away faster and faster through the phase. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        {
            const long double share =
                ((long double)g_aRacePhaseTable[g_dwRacePhaseIndex].seconds - g_fRacePhaseTimer) /
                g_aRacePhaseTable[g_dwRacePhaseIndex].seconds;
            g_fStartLightsDrop = (f32)((share * share) * 15.0L);
        }
#else
        g_fStartLightsDrop = (g_aRacePhaseTable[g_dwRacePhaseIndex].seconds - g_fRacePhaseTimer) /
                             g_aRacePhaseTable[g_dwRacePhaseIndex].seconds;
        g_fStartLightsDrop = g_fStartLightsDrop * (15.0f * g_fStartLightsDrop);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
        goto tick;
    } else if (g_dwRacePhase == RACE_PHASE_RACING) {
        if (RunRacingPhase()) {
            goto advance;
        }
    } else if (g_dwRacePhase == RACE_PHASE_FINISHED) {
        if ((g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_ARCADE ||
             g_dwGameMode == GAME_MODE_CHAMPIONSHIP
#if TGR_PLATFORM == TGR_PLATFORM_WIN
             || g_dwGameMode == GAME_MODE_NETWORK
#endif
             ) &&
            !gInstantReplayActive) {
            SaveRaceResultsToSlots();
        }
        goto advance;
    } else if (g_dwRacePhase == RACE_PHASE_AFTER) {
        goto tick;
    } else if (g_dwRacePhase == RACE_PHASE_LEAVE) {
        if (!g_dwRaceExitState) {
            FadeSfxTo(0.0f, FADE_SECONDS);
            if (!g_bReplayAfterRace) {
                FadeMusicTo(0.0f, FADE_SECONDS);
            }
            SetFadeTarget(0.0f, FADE_SECONDS);
            g_dwRaceExitState = TRUE;
        }
    }
}
