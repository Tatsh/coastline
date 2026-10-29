#include "common/game.h"
#include "common/race.h"
#include "common/render.h"
#include "common/screen.h"
#include "common/text.h"

#if TGR_PC
#define CREDIT_ARITHMETIC u32
#define CREDITS_SCREEN_CALLBACK RunRaceGameScreen
#define CREDIT_SCREEN_CENTER (g_dwScreenWidth / 2)
#define CREDIT_SCREEN_BOTTOM ((s32)((u32)g_dwScreenHeight + 40))
#else
#define CREDIT_ARITHMETIC s32
#define CREDITS_SCREEN_CALLBACK ProcessRacingScreen
#define CREDIT_SCREEN_CENTER 160
#define CREDIT_SCREEN_BOTTOM 280
#endif /* TGR_PC */

void StartEndingCredits(void) {
    g_dwGameMode = GAME_MODE_DEMO;
    g_dwRaceSessionType = RACE_SESSION_SCROLLING_CREDITS;
    SetCurrentScreenCallback(CREDITS_SCREEN_CALLBACK);
}

void CalculateScrollPositions(void) {
    CreditLine *line;
    s32 i;
    s32 y, above, size;

#if TGR_PC
    y = g_dwScreenHeight;
#else
    y = 240;
#endif
    for (i = 0; g_aCreditScrollLines[i].text != NULL; i++) {
        line = &g_aCreditScrollLines[i];
        size = line->size;
        /* Windows arithmetic wraps before the signed division. */
        above = (s32)((CREDIT_ARITHMETIC)size * 3) / 4;
        y = (s32)((CREDIT_ARITHMETIC)y + above);
        line->y = y;
        y = (s32)((CREDIT_ARITHMETIC)y + size - above + line->gap);
    }
}

void ScrollTextUp(void) {
    s32 i;

    for (i = 0; g_aCreditScrollLines[i].text != NULL; i++) {
#if TGR_PC
        g_aCreditScrollLines[i].y = (s32)((u32)g_aCreditScrollLines[i].y - 1);
#else
        g_aCreditScrollLines[i].y--;
#endif
    }
}

#if TGR_PC
void DrawScrollingText(const ViewportRect *viewport) {
#else
void DrawScrollingText(void) {
    const ViewportRect *viewport = &g_aViewportRects[0];
#endif
    s32 i;

    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    SetTextStyleNormal();
    SetTextAlignCenter();
    SetScissorRect(viewport->x, viewport->y, viewport->width, viewport->height);
    for (i = 0; g_aCreditScrollLines[i].text != NULL; i++) {
        if (g_aCreditScrollLines[i].y >= -0x4F &&
            g_aCreditScrollLines[i].y < CREDIT_SCREEN_BOTTOM) {
            SetTextSize(g_aCreditScrollLines[i].size);
            DrawText(g_aCreditScrollLines[i].text, CREDIT_SCREEN_CENTER, g_aCreditScrollLines[i].y);
        }
    }
}
