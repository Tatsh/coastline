#include "playerselect.h"

#include "fade.h"
#include "input.h"
#include "mainmenu.h"
#include "racemenu.h"
#include "system.h"
#include "text.h"

/* The item of the screen that the controllers can make unavailable. */
#define ITEM_TWO_PLAYERS 1

void TwoControllersPostDrawCallback(s32 item) {
    s32 width, height;

    if (!(g_apPlayerCountMenuItems[item]->flags & MENU_ITEM_UNAVAILABLE)) {
        return;
    }

    /* Draw the fade overlay on a panel below the menu and write the note on it. */
    width = g_dwScreenWidth;
    height = g_dwScreenHeight;
    SetScissorRect(width * 9 / 32, height * 27 / 64, width * 14 / 32, height * 4 / 16);
    DrawScreenFadeOverlay(0.4f);
    SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);

    SetTextStyleNormal();
    SetTextAlignCenter();
#if TGR_REGION == TGR_REGION_JP
    SetTextSize(18);
    DrawText("%ww%jコントローラを", g_dwScreenWidth / 2, g_dwScreenHeight * 10 / 16 - 30);
    DrawText("%ww%jコントローラ%e2", g_dwScreenWidth / 2, g_dwScreenHeight * 10 / 16 - 15);
    DrawText(" %ww%jにさして下さい。", g_dwScreenWidth / 2, g_dwScreenHeight * 10 / 16);
#else
    SetTextSize(15);
    DrawText("%wwYou Need To Have", g_dwScreenWidth / 2, g_dwScreenHeight * 10 / 16 - 30);
    DrawText("%wwTwo Controllers", g_dwScreenWidth / 2, g_dwScreenHeight * 10 / 16 - 15);
    DrawText("%wwPlugged in", g_dwScreenWidth / 2, g_dwScreenHeight * 10 / 16);
#endif /* TGR_REGION == TGR_REGION_JP */
}

void ProcessPlayerCountSelect(void) {
    /* The state of the screen: 0 before it starts, 1 while it runs, and -1 once it is left. */
    /**
     * @ghidraAddress USA: 0x802723c4
     * @ghidraAddress EUR: 0x802743a4
     * @ghidraAddress JPN: 0x80273e54
     */
    static s32 g_dwPlayerCountSelectState = 0;

    if (g_dwPlayerCountSelectState <= 0) {
        ResetAndInitDisplay(g_apPlayerCountMenuItems, &g_dwPlayerCountMenuItemCount);
        g_pfnScreenPostDrawCallback = TwoControllersPostDrawCallback;
        if (g_dwPlayerCountSelectState == 0) {
            g_dwPlayerCountMenuSelection = 0;
        }
        g_dwPlayerCountSelectState = 1;
    }

    /* Two players need the first two controllers. */
    if (g_aPlayerInputState[0].controllerMissing || g_aPlayerInputState[1].controllerMissing) {
        g_apPlayerCountMenuItems[ITEM_TWO_PLAYERS]->flags |= MENU_ITEM_UNAVAILABLE;
    } else {
        g_apPlayerCountMenuItems[ITEM_TWO_PLAYERS]->flags &= ~MENU_ITEM_UNAVAILABLE;
    }

    switch (ProcessScreenState("1P/2P SELECT",
                               g_dwPlayerCountMenuItemCount,
                               g_apPlayerCountMenuItems,
                               &g_dwPlayerCountMenuSelection,
                               NULL,
                               0x00,
                               0x00,
                               0x00,
                               0x3A,
                               0x86,
                               0x00)) {
    case MENU_RESULT_CHOSEN:
        g_dwNumLocalPlayers = g_dwPlayerCountMenuSelection + 1;
        SetCurrentScreenCallback(ProcessTrackMenu);
        g_dwPlayerCountSelectState = -1;
        break;
    case MENU_RESULT_CANCELLED:
        SetCurrentScreenCallback(ProcessMainMenu);
        g_dwPlayerCountSelectState = -1;
        break;
    }
}
