#include "race.h"

#include "game.h"
#include "render.h"
#include "screen.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define CREDIT_ARITHMETIC u32
#define CREDITS_SCREEN_CALLBACK RunRaceGameScreen
#else
#define CREDIT_ARITHMETIC s32
#define CREDITS_SCREEN_CALLBACK ProcessRacingScreen
#endif

void StartEndingCredits(void) {
    g_dwGameMode = GAME_MODE_DEMO;
    g_dwRaceSessionType = RACE_SESSION_SCROLLING_CREDITS;
    SetCurrentScreenCallback(CREDITS_SCREEN_CALLBACK);
}

void CalculateScrollPositions(void) {
    CreditLine *line;
    s32 i;
    s32 y, above, size;

#if TGR_PLATFORM == TGR_PLATFORM_WIN
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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        g_aCreditScrollLines[i].y = (s32)((u32)g_aCreditScrollLines[i].y - 1);
#else
        g_aCreditScrollLines[i].y--;
#endif
    }
}
