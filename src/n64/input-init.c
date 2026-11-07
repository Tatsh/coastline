#include <PR/os.h>
#include <PR/os_libc.h>

#include "common/input.h"
#include "common/race.h"
#include "common/rumble.h"
#include "common/splash.h"
#include "common/system.h"
#include "common/vehicle.h"
#include "options.h"
#include "pakmenu.h"

#define WARN_ABOUT_CONTROLLER(warning)                                                             \
    do {                                                                                           \
        SetCurrentScreenCallback(ProcessControllerCheck);                                          \
        g_bControllerWarningType = (warning);                                                      \
        while (g_pfnCurrentScreen == ProcessControllerCheck) {                                     \
            ProcessControllerCheck();                                                              \
        }                                                                                          \
    } while (0)

/* IDO requires a local static and an explicit first warning for the original register order. */
void InitControllersAndPaks(void) {
    static u8 pakInitialized;
    u8 plugged;

    UpdateControllerState();
    g_wSerialInterfaceIdle = FALSE;
    if (g_aPlayerInputState[0].controllerMissing) {
        SetCurrentScreenCallback(ProcessControllerCheck);
        g_bControllerWarningType = CONTROLLER_WARNING_NO_CONTROLLER;
        while (g_pfnCurrentScreen == ProcessControllerCheck) {
            ProcessControllerCheck();
        }
    }

    osPfsIsPlug(&g_SiMesgQueue, &plugged);
    if (!(plugged & (1 << 0))) {
        WARN_ABOUT_CONTROLLER(CONTROLLER_WARNING_NO_PAK);
    } else {
        if (!pakInitialized) {
            pakInitialized = TRUE;
            osSyncPrintf("\nInitializing controller pak...\n");
            gControllerPakState = osPfsInitPak(&g_SiMesgQueue, &g_aPakPfs[0], 0);
        }
        if (gControllerPakState != 0) {
            if (gControllerPakState == PFS_ERR_ID_FATAL) {
                /* The result is discarded: the warning follows whether a Rumble Pak answers. */
                osMotorInit(&g_SiMesgQueue, &g_aContPfs[0], 0);
            }
            WARN_ABOUT_CONTROLLER(CONTROLLER_WARNING_NO_PAK);
        } else {
            memcpy(g_aabSavePakIds[0], g_aPakPfs[0].id, sizeof(g_aPakPfs[0].id));
            g_abSavePakIdKnown[0] = TRUE;
        }
    }

    /* The Pak of the second controller gets no warning. */
    if (plugged & (1 << 1)) {
        gControllerPakState = osPfsInitPak(&g_SiMesgQueue, &g_aPakPfs[1], 1);
        if (gControllerPakState != 0) {
            if (gControllerPakState == PFS_ERR_ID_FATAL) {
                osMotorInit(&g_SiMesgQueue, &g_aContPfs[1], 1);
            }
        } else {
            memcpy(g_aabSavePakIds[1], g_aPakPfs[1].id, sizeof(g_aPakPfs[1].id));
            g_abSavePakIdKnown[1] = TRUE;
        }
    }

    if (g_aPlayerInputState[0].buttons & INPUT_START) {
        TransferFlags(&g_aPlayerInputState[0], INPUT_START);
        SetCurrentScreenCallback(ProcessPakMenuScreen);
        while (g_pfnCurrentScreen == ProcessPakMenuScreen) {
            ProcessPakMenuScreen();
        }
    }
    if (!InitPakAndEnumerateFiles()) {
        WARN_ABOUT_CONTROLLER(CONTROLLER_WARNING_FOREIGN_PAK);
    }
    g_wSerialInterfaceIdle = TRUE;
}
