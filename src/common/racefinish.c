#include "common/racefinish.h"

#include <PR/os.h>

#include "common/game.h"
#include "common/instantreplay.h"
#include "common/race.h"
#include "common/render.h"
#include "common/replay.h"
#include "common/system.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"

#if TGR_PC
#include <stdint.h>

#include "win32/boot/app.h"
#include "win32/boot/rallyconfig.h"
#include "win32/gameinit.h"
#include "win32/season.h"
#include "win32/snapshot.h"
#include "win32/sound.h"
#else
#include "carselect.h"
#include "common/audio.h"
#include "common/audiomix.h"
#include "common/splash.h"
#include "mainmenu.h"
#include "options.h"
#include "racemenu.h"
#endif /* TGR_PC */

#if TGR_PC
namespace {
    /**
     * The player whose recording the instant replay plays, and the length of that recording.
     *
     * @ghidraAddress USA: 0x80315e3c
     * @ghidraAddress EUR: 0x80317b2c
     * @ghidraAddress JPN: 0x8031656c
     * @ghidraAddress BRD3D: 0x106909c8
     */
    s32 g_dwBestReplayLength;
} // namespace
#endif /* TGR_PC */

#if TGR_PC
/** @ghidraAddress BRD3D: 0x10680668 */
s32 g_dwBestReplayPlayer;
#endif /* TGR_PC */

#if TGR_PC
/**
 * An address used as the stopped-channel sample token; its contents are never read.
 * @ghidraAddress BRD3D: 0x118ac70c
 * @ghidraAddress BRGlide: 0x118eef2c
 */
static u8 g_bNoSoundSample;
/**
 * A retained season-menu request without native readers.
 * @ghidraAddress BRD3D: 0x11782ddc
 * @ghidraAddress BRGlide: 0x117a613c
 */
static volatile u8 g_bReturnToSeasonMenu;
#else
#define RACE_SOUND_CHANNELS 6
#endif /* TGR_PC */

#if TGR_PC
/* @ghidraAddress BRD3D: 0x10680670 */
static u8 *volatile g_apReplayGhostPtrs[INPUT_RECORDERS];
/* @ghidraAddress BRD3D: 0x106806e8 */
static volatile s32 g_adwReplayGhostSizes[INPUT_RECORDERS];
#endif

/* Choose the instant replay of the best recording of the players. */
static void ChooseInstantReplay(void) {
    s32 i, j;

    g_dwBestReplayLength = VEHICLE_GET(&g_aVehicles[0], Input)->recordPositions[0];
    g_dwBestReplayPlayer = 0;
    for (i = 1; i < (s32)g_dwNumLocalPlayers; i++) {
        if (VEHICLE_GET(&g_aVehicles[i], Input)->recordPositions[i] < g_dwBestReplayLength) {
            g_dwBestReplayLength = VEHICLE_GET(&g_aVehicles[i], Input)->recordPositions[i];
            g_dwBestReplayPlayer = i;
        }
    }
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        g_apReplayGhostPtrs[i] =
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[g_dwBestReplayPlayer];
        g_adwReplayGhostSizes[i] =
            VEHICLE_GET(&g_aVehicles[i], Input)->recordPositions[g_dwBestReplayPlayer];
    }
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        for (j = 0; j < (s32)g_dwNumLocalPlayers; j++) {
            /* The address of the positions is printed rather than one of them. */
            osSyncPrintf("veh[%d]->recIndex[%d] = %d\n",
                         i,
                         j,
                         VEHICLE_GET(&g_aVehicles[i], Input)->recordPositions);
            VEHICLE_GET(&g_aVehicles[i], Input)->recordBuffers[j] = NULL;
        }
    }
    osSyncPrintf("bestNumber = %d, bestIndex = %d\n", g_dwBestReplayPlayer, g_dwBestReplayLength);
}

/* Go on from the race once the screen faded out. */
void FinishRace(void) {
    s32 i, j;

    for (i = 0; i < MAX_VEHICLES; i++) {
        for (j = 0; j < (s32)g_dwNumLocalPlayers; j++) {
            VEHICLE_GET(&g_aVehicles[i], Input)->recordLimits[j] =
                VEHICLE_GET(&g_aVehicles[i], Input)->recordPositions[j];
        }
    }
#if TGR_PC
    g_bAltLoopingSoundSet = FALSE;
    for (i = 0; i < SOUND_CHANNEL_COUNT; i++) {
        g_aSoundChannelRequested[i].dwReserved = 0;
        g_aSoundChannelRequested[i].dwStereoLevels = 0;
        g_aSoundChannelRequested[i].llFrequency = 0;
        g_aSoundChannelRequested[i].nSample = (int)(uintptr_t)&g_bNoSoundSample;
        StopAudioChannel(i);
    }
#else
    g_dwSoundEffectEchoOffset = 0;
    for (i = 0; i < RACE_SOUND_CHANNELS; i++) {
        g_aSoundEffectChannels[i].volume = 0;
        g_aSoundEffectChannels[i].mixVolume = 0;
        g_aSoundEffectChannels[i].step = 0;
        g_aSoundEffectChannels[i].sample = g_SilenceSampleData;
    }
#endif /* TGR_PC */
    g_bRaceScreenActive = FALSE;
    if (g_dwRaceExitState) {
        if (g_bReplayAfterRace) {
            if (!gInstantReplayActive) {
                ChooseInstantReplay();
            }
            gInstantReplayActive = TRUE;
        } else {
            ClearFramebuffersHighRes();
            g_dwTrackMirrored = FALSE;
            if (g_dwGameMode == GAME_MODE_AFTER_SEASON) {
                if (VEHICLE_GET(&g_aVehicles[0], Profile)->season) {
#if !TGR_PC
                season:
#endif
                    /* After the race of a season or the credits, the championship goes on. */
                    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
                    SetTrackFromSeason();
#if TGR_PC
                    g_bReturnToSeasonMenu = TRUE;
                    SetCurrentScreenCallback(ShutdownGameSessionToSeasonMenu);
#else
                    g_bSaveLoadEntryPoint = SAVE_LOAD_FROM_SEASON;
                    SetCurrentScreenCallback(ProcessDataSaveLoadMenu);
#endif
                } else {
                    /* The last season ends with the credits. */
                    StartEndingCredits();
                }
            } else if (g_dwGameMode == GAME_MODE_DEMO &&
                       g_dwRaceSessionType == RACE_SESSION_SCROLLING_CREDITS) {
                /* The credits share the return to the championship of the race after a season. */
#if TGR_PC
                SetCurrentScreenCallback(ExitRaceSessionToMenu);
#else
                goto season;
#endif
            } else if (g_dwSavedResultsValid &&
                       (g_dwGameMode == GAME_MODE_TIME_ATTACK || g_dwGameMode == GAME_MODE_ARCADE ||
                        g_dwGameMode == GAME_MODE_CHAMPIONSHIP
#if TGR_PC
                        || g_dwGameMode == GAME_MODE_NETWORK
#endif
                        )) {
                RestoreRaceResultsFromSlots();
#if TGR_PC
                SetCurrentScreenCallback(ExitRaceAndProcessSeasonResults);
#else
                SetCurrentScreenCallback(ProcessRaceResultsScreen);
#endif
            } else if (g_dwGameMode == GAME_MODE_DEMO &&
                       g_dwRaceSessionType == RACE_SESSION_CREDIT_CAPTIONS) {
#if TGR_PC
                SetCurrentScreenCallback(ExitRaceSessionToMenu);
#else
                SetCurrentScreenCallback(ProcessOptionsMenu);
#endif
            } else if (g_dwGameMode == GAME_MODE_DEMO &&
                       g_dwRaceSessionType == RACE_SESSION_ATTRACT && !g_bDemoInterrupted) {
#if TGR_PC
                SetCurrentScreenCallback(ExitRaceSessionToMenu);
#else
                SetCurrentScreenCallback(ProcessSplashScreens);
#endif
            } else {
#if TGR_PC
                if (g_dwRaceExitState == 2) {
                    if (g_dwSavedSfxVolume != g_GlobalRallyConfig.GetSfxVolume() ||
                        g_dwSavedMusicVolume != g_GlobalRallyConfig.GetMusicVolume()) {
                        g_GlobalRallyConfig.SaveConfigToFile(g_szConfigFilePath);
                    }
                    ShutdownApplication(0);
                }
                SetCurrentScreenCallback(ExitRaceSessionToMenu);
#else
                SetCurrentScreenCallback(ProcessMainMenu);
#endif /* TGR_PC */
            }
            gInstantReplayActive = FALSE;
        }
        g_dwRaceInitialized = FALSE;
#if TGR_PC
        if (!gInstantReplayActive) {
            FreeAllRacerColorPanels();
        }
        InvalidateFrameSnapshotSlots();
#endif
    }
    g_wRaceScreenLoopExited = TRUE;
    for (i = 0; i < MAX_VEHICLES; i++) {
        VEHICLE_GET(&g_aVehicles[i], Input)->playbackBuffer = NULL;
    }
}
