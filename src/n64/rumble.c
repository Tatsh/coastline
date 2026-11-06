#include "common/rumble.h"

#include "common/game.h"
#include "common/race.h"
#include "common/system.h"
#include "common/vehicle.h"

/* The motors are probed again every 64 calls. */
#define RUMBLE_PROBE_INTERVAL 64

/* The European and Japanese releases also stop the motors while a reset is pending. */
#if TGR_REGION == TGR_REGION_US || TGR_REGION == TGR_REGION_ASIA
#define RESET_PENDING() 0
#else
#define RESET_PENDING() g_dwPreNmiResetPending
#endif

void UpdateRumblePaks(s32 unused) {
    int i;
    int j;
    int k;

    /* Stop every motor outside a running race, and probe for each Pak again. */
    if (!IsCurrentScreen(ProcessRacingScreen) || g_dwGameMode == GAME_MODE_AFTER_SEASON ||
        g_wRaceScreenLoopExited || RESET_PENDING() || g_dwRacePauseState) {
        for (i = 0; i != RUMBLE_CHANNELS; i++) {
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

    /* Pulse each motor that is rumbling, unless its vehicle plays back a recording. */
#if TGR_REGION == TGR_REGION_US || TGR_REGION == TGR_REGION_ASIA
    if (g_wSerialInterfaceIdle != 0 && g_wRaceScreenLoopExited == 0 && g_dwRacePauseState == 0) {
#else
    if (g_wSerialInterfaceIdle != 0 && g_wRaceScreenLoopExited == 0 && RESET_PENDING() == 0 &&
        g_dwRacePauseState == 0) {
#endif
        for (j = 0; j != RUMBLE_CHANNELS; j++) {
            if (g_abRumblePakPresent[j] && g_awRumbleActive[j] &&
                g_aVehicles[j].input->playbackBuffer == NULL) {
                if (g_awRumbleCounter[j] == 0) {
                    if (g_awRumbleMotorOn[j]) {
                        g_awRumbleCounter[j] = g_awRumbleOffFrames[j];
                        osMotorStop(&g_aContPfs[j]);
                    } else {
                        g_awRumbleCounter[j] = g_awRumbleOnFrames[j];
                        osMotorStart(&g_aContPfs[j]);
                    }
                    g_awRumbleMotorOn[j] ^= 1;
                } else {
                    g_awRumbleCounter[j]--;
                }
                if (g_awRumbleFramesLeft[j] == 0) {
                    osMotorStop(&g_aContPfs[j]);
                    g_awRumbleMotorOn[j] = 0;
                    g_awRumbleCounter[j] = 0;
                    g_awRumbleActive[j] = 0;
                } else {
                    g_awRumbleFramesLeft[j]--;
                }
            }
        }
    }

    /* Probe for Rumble Paks inserted since. */
    /* The counter follows the reset flag by one word; this preserves IDO's separate addresses. */
    k = g_RumbleProbeState.probeTimer + 1;
    g_dwRumbleProbeTimer = k = k & (RUMBLE_PROBE_INTERVAL - 1);
    if (k == 0 && g_wSerialInterfaceIdle) {
        for (i = 0; i != RUMBLE_CHANNELS; i++) {
            g_abRumblePakPresent[i] = FALSE;
            if (osMotorInit(&g_SiMesgQueue, &g_aContPfs[i], i) == 0) {
                g_abRumblePakPresent[i] = TRUE;
            }
        }
    }
}

void PrepareRumblePaks(void) {
}
