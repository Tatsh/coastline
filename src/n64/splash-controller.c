#include <PR/os.h>

#include "common/fade.h"
#include "common/game.h"
#include "common/heap.h"
#include "common/input.h"
#include "common/render.h"
#include "common/splash.h"
#include "common/system.h"
#include "common/text.h"
#include "common/timing.h"
#include "mainmenu.h"
#include "pakmenu.h"
#include "sprite.h"

#define FADE_TIME 0.2f
#define CONTROLLER_CHECK_FRAMES 910

void ProcessControllerCheck(void) {
#if TGR_REGION != TGR_REGION_US && TGR_REGION != TGR_REGION_ASIA
    /* The value of the flag comes from a variable, loaded in a register of its own. */
    u32 blanked = TRUE;

#endif
    if (!g_bControllerCheckStarted) {
        g_bControllerCheckStarted = 1;
#if TGR_REGION == TGR_REGION_US || TGR_REGION == TGR_REGION_ASIA
        ResetHeap();
        /* Show two black frames before the pictures load. */
        osViBlack(1);
        StartHighResFrame();
        FlushDisplayList();
        osViBlack(1);
        StartHighResFrame();
        FlushDisplayList();
#else
        /* Show three black frames, the heap reset after the first, before the pictures load. */
        g_dwKeepScreenBlanked = blanked;
        BlankDisplay();
        StartHighResFrame();
        FlushDisplayList();
        ResetHeap();
        BlankDisplay();
        StartHighResFrame();
        FlushDisplayList();
        BlankDisplay();
        StartHighResFrame();
        FlushDisplayList();
#endif /* TGR_REGION == TGR_REGION_US */
        LoadTextureFromRom(&g_texBackground);
        LoadTextureFromRom(&g_texLogo);
        LoadTextureFromRom(&g_texButtonA);
        SetFadeTarget(1.0f, FADE_TIME);
        g_wScreenFrameCount = 0;
#if TGR_REGION != TGR_REGION_US && TGR_REGION != TGR_REGION_ASIA
        g_dwKeepScreenBlanked = FALSE;
#endif
    }

    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
    SetupProjectionMatrix((f32)g_dwScreenWidth, (f32)g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    ClearDepthBuffer();
    ClearScreen(0, 0, 0);
    DrawSpriteTinted(&g_texBackground, 192, 0, 64);
    DrawTexturedSprite(&g_texLogo, g_texLogo.originX, g_texLogo.originY + 8);

    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(30);
    DrawText("%ryWARNING", 160, 41);
#if TGR_REGION == TGR_REGION_US || TGR_REGION == TGR_REGION_ASIA
    SetTextSize(18);
    switch (g_bControllerWarningType) {
    case 0:
        DrawText("%ywCONTROLLER NOT FOUND", 160, 124);
        break;
    case 1:
        DrawText("%ywCONTROLLER PAK NOT FOUND", 160, 105);
        DrawText("%ywGAME DATA CANNOT BE SAVED", 160, 125);
        DrawText("%ywIF YOU CHOOSE TO PROCEED", 160, 145);
        break;
    case 2:
        DrawText("%ywCONTROLLER PAK CONTAINS GAME", 160, 85);
        DrawText("%ywDATA SAVED FROM ANOTHER GAME.", 160, 105);
        DrawText("%ywTHERE MAY NOT BE ENOUGH PAGES", 160, 125);
        DrawText("%ywAND/OR FILES FREE WHEN YOU NEED", 160, 145);
        DrawText("%ywTO SAVE GAME DATA FROM THIS GAME.", 160, 165);
        break;
    }
    DrawTexturedSprite(&g_texButtonA, 254, 432);
    SetTextAlignLeft();
    SetTextSize(11);
    DrawText("%wwPROCEED", 142, g_dwScreenHeight * 19 / 20 - 3);
#elif TGR_REGION == TGR_REGION_EU
    SetTextSize(18);
    switch (g_bControllerWarningType) {
    case 0:
        DrawText("%ywCONTROLLER NOT FOUND", 160, 124);
        break;
    case 1:
        DrawText("%ywCONTROLLER PAK NOT FOUND", 160, 95);
        DrawText("%ywGHOST CAR AND PAINT SHOP DATA", 160, 115);
        DrawText("%ywCANNOT BE SAVED", 160, 135);
        DrawText("%ywIF YOU CHOOSE TO PROCEED", 160, 155);
        break;
    case 2:
        DrawText("%ywCONTROLLER PAK CONTAINS GAME", 160, 85);
        DrawText("%ywDATA SAVED FROM ANOTHER GAME.", 160, 105);
        DrawText("%ywTHERE MAY NOT BE ENOUGH PAGES", 160, 125);
        DrawText("%ywAND/OR FILES FREE WHEN YOU NEED", 160, 145);
        DrawText("%ywTO SAVE GAME DATA FROM THIS GAME.", 160, 165);
        break;
    }
    SetTextAlignLeft();
    SetTextSize(11);
    DrawTexturedSprite(&g_texButtonA, 254, 432);
    DrawText("%wwPROCEED", 142, g_dwScreenHeight * 19 / 20 - 3);
#else
    SetTextSize(20);
    switch (g_bControllerWarningType) {
    case 0:
        DrawText(" %yw%jコントローラがありません。", 160, 124);
        break;
    case 1:
        DrawText("%yw%j コントローラパックがないので、", 160, 95);
        DrawText("%yw%jゴーストカーデータとペイントショップ", 160, 115);
        DrawText("%yw %jのデカールデータがセーブできません。", 160, 135);
        DrawText("%yw%jゲームをはじめますか？", 160, 155);
        break;
    case 2:
        DrawText("%yw%j他のゲームデータが", 160, 85);
        DrawText(" %yw%jセーブされています。", 160, 105);
        DrawText("%yw%j空きページが足らないので", 160, 125);
        DrawText("%yw%jこのゲームデータを", 160, 145);
        DrawText(" %yw%jセーブできません。", 160, 165);
        break;
    }
    SetTextAlignLeft();
    SetTextSize(11);
    DrawTexturedSprite(&g_texButtonA, 278, 432);
    DrawText("%wwOK", 154, g_dwScreenHeight * 19 / 20 - 3);
#endif /* TGR_REGION == TGR_REGION_US */
    gDPPipeSync(gDisplayListPtr++);
    ApplyScreenFade();
    FlushDisplayList();

    if (g_aPlayerInputState[0].buttons & INPUT_A) {
        TransferFlags(&g_aPlayerInputState[0], INPUT_A);
        SetFadeTarget(0.0f, FADE_TIME);
    } else if (g_wScreenFrameCount++ == CONTROLLER_CHECK_FRAMES) {
        SetFadeTarget(0.0f, FADE_TIME);
    }

    if (IsFadedToBlack()) {
        g_bControllerCheckStarted = 0;
        ClearFramebuffersForWideMode();
        gControllerPakState = 0;
        g_wScreenFrameCount = 0;
        SetCurrentScreenCallback(ProcessMainMenu);
    }
}
