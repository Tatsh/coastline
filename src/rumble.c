#include "rumble.h"

#include "game.h"
#include "racehud.h"
#include "system.h"
#include "vehicle.h"

/* The game mode that follows the results of a finished season. The motors stay off in it. */
#define GAME_MODE_AFTER_SEASON 5

/* The motors are probed again every 64 calls. */
#define RUMBLE_PROBE_INTERVAL 64

void UpdateRumblePaks(s32 unused) {
    int i;

    /* Stop every motor outside a running race, and probe for each Pak again. */
    if (!IsCurrentScreen(ProcessRaceHud) || g_dwGameMode == GAME_MODE_AFTER_SEASON ||
        g_wRaceScreenLoopExited || g_dwRacePauseState) {
        for (i = 0; i < RUMBLE_CHANNELS; i++) {
            if (g_wSerialInterfaceIdle) {
                g_awRumbleActive[i] = 0;
                g_abRumblePakPresent[i] = FALSE;
                if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[i], i) == 0) {
                    g_abRumblePakPresent[i] = TRUE;
                    osMotorStop(&g_aContPfs[i]);
                }
            }
        }
    }

    /* Pulse each motor that is rumbling, unless its vehicle plays back a ghost. */
    if (g_wSerialInterfaceIdle && !g_wRaceScreenLoopExited && !g_dwRacePauseState) {
        for (i = 0; i < RUMBLE_CHANNELS; i++) {
            if (!g_abRumblePakPresent[i] || !g_awRumbleActive[i]) {
                continue;
            }
            if (g_aVehicles[i].replay->ghostData != NULL) {
                continue;
            }
            if (g_awRumbleCounter[i] == 0) {
                if (g_awRumbleMotorOn[i]) {
                    g_awRumbleCounter[i] = g_awRumbleOffFrames[i];
                    osMotorStop(&g_aContPfs[i]);
                } else {
                    g_awRumbleCounter[i] = g_awRumbleOnFrames[i];
                    osMotorStart(&g_aContPfs[i]);
                }
                g_awRumbleMotorOn[i] ^= 1;
            } else {
                g_awRumbleCounter[i]--;
            }
            if (g_awRumbleFramesLeft[i] == 0) {
                osMotorStop(&g_aContPfs[i]);
                g_awRumbleMotorOn[i] = 0;
                g_awRumbleCounter[i] = 0;
                g_awRumbleActive[i] = 0;
            } else {
                g_awRumbleFramesLeft[i]--;
            }
        }
    }

    /* Probe for Rumble Paks inserted since. */
    g_dwRumbleProbeTimer = (g_dwRumbleProbeTimer + 1) % RUMBLE_PROBE_INTERVAL;
    if (g_dwRumbleProbeTimer == 0 && g_wSerialInterfaceIdle) {
        for (i = 0; i < RUMBLE_CHANNELS; i++) {
            g_abRumblePakPresent[i] = FALSE;
            if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[i], i) == 0) {
                g_abRumblePakPresent[i] = TRUE;
            }
        }
    }
}

void PrepareRumblePaks(void) {
}
