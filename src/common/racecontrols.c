#include "common/racecontrols.h"

#include "common/fade.h"
#include "common/game.h"
#include "common/input.h"
#include "common/instantreplay.h"
#include "common/pause.h"
#include "common/race.h"
#include "common/racerender.h"
#include "common/vehicleaccess.h"
#include "common/vehiclecount.h"
#include "common/volumecontrols.h"

#if TGR_BOSS_RALLY
#include "common/inputpoll.h"
#include "common/session.h"
#include "common/settings.h"
#include "common/snapshot.h"
#include "common/soundsystem.h"
#include "win32/directplay.h"
#endif

#if TGR_BOSS_RALLY
/** @ghidraAddress BRD3D: 0x10680734 */
s32 g_dwPauseMenuSelection;
/** @ghidraAddress BRD3D: 0x106909b4 */
u32 g_dwRacePauseState;
#endif /* TGR_BOSS_RALLY */

#define FADE_SECONDS 0.2f
#define CONFIRM_BUTTONS (INPUT_A | INPUT_START | INPUT_Z)
#if TGR_BOSS_RALLY
#define RACE_TRANSFER_INPUT(state, mask) ((state)->TransferFlags(mask))

/**
 * A retained clear-frame request written after unpausing, without any native readers.
 *
 * @ghidraAddress BRD3D: 0x106909bc
 * @ghidraAddress BRGlide: 0x105ccb64
 */
static volatile s32 g_dwUnpauseClearFrames;

static s32 ResumeRace(s32 paused) {
    if (g_dwNetworkPlayMode && g_bNetRaceStartReached) {
        RequestNetworkUnpause();
    } else {
        FadeSfxTo(1.0f, FADE_SECONDS);
        FadeMusicTo(1.0f, FADE_SECONDS);
        g_dwUnpauseClearFrames = 2;
        paused = FALSE;
    }
    return paused;
}

static s32 ApplyUnpauseAcknowledgement(s32 paused) {
    if (g_dwNetworkDataReceived) {
        g_dwNetworkDataReceived = FALSE;
        FadeSfxTo(1.0f, FADE_SECONDS);
        FadeMusicTo(1.0f, FADE_SECONDS);
        g_dwUnpauseClearFrames = 2;
        paused = FALSE;
    }
    return paused;
}
#else
#define RACE_TRANSFER_INPUT(state, mask) TransferFlags(state, mask)
#endif /* TGR_BOSS_RALLY */

/* Take the input of the pause menu, and tell whether the race stays paused. */
s32 ProcessPauseMenu(s32 paused) {
    s32 i;

#if TGR_BOSS_RALLY
    if (g_dwRacePauseState == 2) {
        if (g_dwInputButtonMask & 0x8000) {
            if (g_dwPauseMenuSelection == 0) {
                SetFadeTarget(0.0f, FADE_SECONDS);
                FadeSfxTo(0.0f, FADE_SECONDS);
                FadeMusicTo(0.0f, FADE_SECONDS);
                g_bReplayAfterRace = FALSE;
                g_dwRaceExitState = 2;
            } else if (g_dwPauseMenuSelection == 1) {
                paused = ResumeRace(paused);
            }
        }
        if (g_dwInputButtonMask & 0x1000) {
            g_dwPauseMenuSelection = (s32)(1u - (u32)g_dwPauseMenuSelection);
        }
        if (g_dwInputButtonMask & 0x2000) {
            g_dwPauseMenuSelection = (s32)(1u - (u32)g_dwPauseMenuSelection);
        }
        paused = ApplyUnpauseAcknowledgement(paused);
        goto networkTick;
    }
#endif /* TGR_BOSS_RALLY */
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
#if !TGR_BOSS_RALLY
        UpdateStickRepeat(VEHICLE_GET(&g_aVehicles[i], Input));
        if (VEHICLE_GET(&g_aVehicles[i], Input)->buttons & INPUT_B) {
            RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[i], Input), INPUT_B);
            FadeSfxTo(1.0f, FADE_SECONDS);
            FadeMusicTo(1.0f, FADE_SECONDS);
            g_dwScreenClearFrames = 2;
            paused = FALSE;
        }
#endif /* !TGR_BOSS_RALLY */
        if (
#if TGR_BOSS_RALLY
            (g_dwInputButtonMask & 0x8000) ||
            (g_dwPauseMenuSelection == PAUSE_CONTINUE && (g_dwInputButtonMask & 4) &&
             !(g_dwInputButtonMask & 0x3000) && !(g_dwInputButtonMask & 3))
#else
            VEHICLE_GET(&g_aVehicles[i], Input)->buttons & CONFIRM_BUTTONS
#endif
        ) {
            /* A that continues the race is left for the race to take. */
            if (g_dwPauseMenuSelection != PAUSE_CONTINUE ||
                !(VEHICLE_GET(&g_aVehicles[i], Input)->buttons & INPUT_A)) {
                RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[i], Input), CONFIRM_BUTTONS);
            }
            switch (g_dwPauseMenuSelection) {
            case PAUSE_CONTINUE:
#if TGR_BOSS_RALLY
                paused = ResumeRace(paused);
#else
                FadeSfxTo(1.0f, FADE_SECONDS);
                FadeMusicTo(1.0f, FADE_SECONDS);
                g_dwScreenClearFrames = 2;
                paused = FALSE;
#endif
                break;
            case PAUSE_RESTART:
                SetFadeTarget(0.0f, FADE_SECONDS);
                g_dwRaceExitState = FALSE;
                g_bReplayAfterRace = FALSE;
                g_dwRaceInitialized = FALSE;
#if TGR_BOSS_RALLY
                FreeAllRacerColorPanels();
                g_bNetRaceStartReached = FALSE;
                ResetAudioSubsystem();
                InvalidateFrameSnapshotSlots();
#endif
                break;
            case PAUSE_EXIT:
                SetFadeTarget(0.0f, FADE_SECONDS);
                FadeSfxTo(0.0f, FADE_SECONDS);
                FadeMusicTo(0.0f, FADE_SECONDS);
                g_bReplayAfterRace = FALSE;
                g_dwRaceExitState = TRUE;
#if TGR_BOSS_RALLY
                if (g_bBgmVolume) {
                    UnpauseMusicPlayback();
                    DispatchMusicVolumeChange(g_bBgmVolume);
                }
                if (g_dwGameMode == GAME_MODE_NETWORK) {
                    SendNetworkMessage6ToSession();
                    DisableNetworkReceive();
                }
#endif /* TGR_BOSS_RALLY */
                break;
#if TGR_BOSS_RALLY
            case PAUSE_CONFIRM_EXIT:
                paused = 2;
                g_dwRacePauseState = 2;
                g_dwPauseMenuSelection = 1;
                break;
#endif
            }
        }
#if TGR_BOSS_RALLY
        paused = ApplyUnpauseAcknowledgement(paused);
        if (g_dwInputButtonMask & 0x1000) {
            g_dwPauseMenuSelection =
                (s32)((u32)g_dwPauseMenuSelection + PAUSE_ITEMS - 1) % PAUSE_ITEMS;
            if (g_dwNetworkPlayMode && g_dwPauseMenuSelection == PAUSE_RESTART) {
                g_dwPauseMenuSelection =
                    (s32)((u32)g_dwPauseMenuSelection + PAUSE_ITEMS - 1) % PAUSE_ITEMS;
            }
        }
        if (g_dwInputButtonMask & 0x2000) {
            g_dwPauseMenuSelection = (s32)((u32)g_dwPauseMenuSelection + 1) % PAUSE_ITEMS;
            if (g_dwNetworkPlayMode && g_dwPauseMenuSelection == PAUSE_RESTART) {
                g_dwPauseMenuSelection = (s32)((u32)g_dwPauseMenuSelection + 1) % PAUSE_ITEMS;
            }
        }
        if (g_dwInputButtonMask & 1) {
            if (g_dwPauseMenuSelection == PAUSE_MUSIC) {
                DecreaseBgmVolume();
            } else if (g_dwPauseMenuSelection == PAUSE_SOUND) {
                DecreaseSfxVolume();
            }
        }
        if (g_dwInputButtonMask & 2) {
            if (g_dwPauseMenuSelection == PAUSE_MUSIC) {
                IncreaseBgmVolume();
            } else if (g_dwPauseMenuSelection == PAUSE_SOUND) {
                IncreaseSfxVolume();
            }
        }
#else
        if (VEHICLE_GET(&g_aVehicles[i], Input)->buttons & INPUT_UP) {
            RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[i], Input), INPUT_RIGHT | INPUT_LEFT);
            VEHICLE_GET(&g_aVehicles[i], Input)->buttons &= ~(INPUT_RIGHT | INPUT_LEFT);
            RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[i], Input), INPUT_UP);
            g_dwPauseMenuSelection = (g_dwPauseMenuSelection + PAUSE_ITEMS - 1) % PAUSE_ITEMS;
        }
        if (VEHICLE_GET(&g_aVehicles[i], Input)->buttons & INPUT_DOWN) {
            RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[i], Input), INPUT_RIGHT | INPUT_LEFT);
            VEHICLE_GET(&g_aVehicles[i], Input)->buttons &= ~(INPUT_RIGHT | INPUT_LEFT);
            RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[i], Input), INPUT_DOWN);
            g_dwPauseMenuSelection = (g_dwPauseMenuSelection + 1) % PAUSE_ITEMS;
        }
        if (VEHICLE_GET(&g_aVehicles[i], Input)->buttons & INPUT_LEFT) {
            if (g_dwPauseMenuSelection != PAUSE_CONTINUE) {
                RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[i], Input), INPUT_LEFT);
            }
            if (g_dwPauseMenuSelection == PAUSE_MUSIC) {
                DecreaseBgmVolume();
            } else if (g_dwPauseMenuSelection == PAUSE_SOUND) {
                DecreaseSfxVolume();
            }
        }
        if (VEHICLE_GET(&g_aVehicles[i], Input)->buttons & INPUT_RIGHT) {
            if (g_dwPauseMenuSelection != PAUSE_CONTINUE) {
                RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[i], Input), INPUT_RIGHT);
            }
            if (g_dwPauseMenuSelection == PAUSE_MUSIC) {
                IncreaseBgmVolume();
            } else if (g_dwPauseMenuSelection == PAUSE_SOUND) {
                IncreaseSfxVolume();
            }
        }
#endif /* TGR_BOSS_RALLY */
    }
#if TGR_BOSS_RALLY
networkTick:
    if (g_dwNetworkPlayMode && g_bNetRaceStartReached) {
        TickNetworkUnpauseRequest();
    }
#else
    DrawPauseMenu();
#endif
    return paused;
}

/* Take the buttons of the players while the race runs, and tell whether it is paused. */
s32 ProcessRaceButtons(s32 paused) {
    s32 i;

    if (g_dwGameMode == GAME_MODE_DEMO || g_dwGameMode == GAME_MODE_AFTER_SEASON) {
        if (IsFadingOut()) {
            return paused;
        }
        /* Either controller ends a race the computer drives. */
        for (i = 0; i < 2; i++) {
            if (
#if TGR_BOSS_RALLY
                g_dwInputButtonMask & 0x4000
#else
                g_aPlayerInputState[i].buttons & CONFIRM_BUTTONS
#endif
            ) {
                RACE_TRANSFER_INPUT(&g_aPlayerInputState[i], CONFIRM_BUTTONS);
                g_bDemoInterrupted = TRUE;
                g_dwRaceExitState = TRUE;
                g_bReplayAfterRace = FALSE;
                FadeSfxTo(0.0f, FADE_SECONDS);
                SetFadeTarget(0.0f, FADE_SECONDS);
            }
        }
        return paused;
    }
#if TGR_BOSS_RALLY
    if ((s32)g_dwNumLocalPlayers > 0 && g_bNetGamePaused) {
        g_bNetGamePaused = FALSE;
        if (gInstantReplayActive) {
            SetFadeTarget(0.0f, FADE_SECONDS);
            g_bReplayAfterRace = FALSE;
            g_dwRaceExitState = TRUE;
        } else {
            paused = TRUE;
            g_dwPauseMenuSelection = PAUSE_CONTINUE;
        }
        FadeSfxTo(0.0f, FADE_SECONDS);
        FadeMusicTo(0.0f, FADE_SECONDS);
        RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[0], Input), INPUT_START);
        RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[1], Input), INPUT_START);
    }
#else
    for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
        if (VEHICLE_GET(&g_aVehicles[i], Input)->buttons & INPUT_START) {
            if (gInstantReplayActive) {
                SetFadeTarget(0.0f, FADE_SECONDS);
                g_bReplayAfterRace = FALSE;
                g_dwRaceExitState = TRUE;
            } else {
                paused = TRUE;
                g_dwPauseMenuSelection = PAUSE_CONTINUE;
            }
            FadeSfxTo(0.0f, FADE_SECONDS);
            FadeMusicTo(0.0f, FADE_SECONDS);
            RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[0], Input), INPUT_START);
            RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[1], Input), INPUT_START);
            return paused;
        }
        if (VEHICLE_GET(&g_aVehicles[i], Input)->buttons & INPUT_A) {
            /* A restarts the instant replay. */
            if (gInstantReplayActive) {
                SetFadeTarget(0.0f, FADE_SECONDS);
                g_bReplayAfterRace = TRUE;
                g_dwRaceExitState = TRUE;
            }
            RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[0], Input), INPUT_A);
            RACE_TRANSFER_INPUT(VEHICLE_GET(&g_aVehicles[1], Input), INPUT_A);
            return paused;
        }
    }
#endif /* TGR_BOSS_RALLY */
    return paused;
}
