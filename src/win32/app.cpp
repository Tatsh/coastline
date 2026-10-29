#include "win32/app.h"

#include "common/displaylistptr.h"
#include "common/racerslot.h"
#include "common/settings.h"
#include "common/snapshot.h"
#include "common/text.h"
#include "common/vehiclecount.h"
#include "win32/displaylist.h"
#include "win32/render.h"

/** @ghidraAddress BRD3D: 0x106805a0 */
int g_dwActivateMinimized;
/** @ghidraAddress BRD3D: 0x1068059c */
int g_dwActivateState;
/** @ghidraAddress BRD3D: 0x118abe2c */
int g_dwAppExitCode;
/** @ghidraAddress BRD3D: 0x1027736c */
DirectDrawRenderer *g_pMainRenderWindow;

void FreeAllRacerColorPanels(void) {
    for (s32 index = 0; index < g_dwNumVehicles; index++) {
        g_aRacerSlots[index].FreeEnemyCarColorPanels();
    }
}

[[noreturn]] void ShowFatalErrorScreen(const char *pszMessage) {
    GbiCommand commands[0x1000];
    InvalidateFrameSnapshotSlots();
    gDisplayListPtr = commands;
    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(20);
    DrawText(pszMessage, g_dwConfigFramebufferWidth / 2, 220);
    gDisplayListPtr->w0 = GBI_OP_END_DISPLAY_LIST << 24;
    gDisplayListPtr->w1 = 0;
    gDisplayListPtr++;
    g_pfnRenderBackendSubmitDisplayList(commands);
    auto getKeyState = GetAsyncKeyState;
    auto sleep = Sleep;
    for (;;) {
        if (getKeyState(VK_ESCAPE) != 0) {
            ShutdownApplication(1);
        }
        sleep(1);
    }
}
