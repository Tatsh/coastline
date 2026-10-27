#include "platform/win/boot/app.h"

#include "displaylistptr.h"
#include "platform/win/displaylist.h"
#include "platform/win/render.h"
#include "platform/win/settings.h"
#include "platform/win/snapshot.h"
#include "racerslot.h"
#include "text.h"
#include "vehiclecount.h"

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
    for (;;) {
        if (GetAsyncKeyState(VK_ESCAPE) != 0) {
            ShutdownApplication(1);
        }
        Sleep(1);
    }
}
