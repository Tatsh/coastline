#include "common/game.h"
#include "common/input.h"
#include "common/race.h"
#include "common/screen.h"

void StartAttractDemo(void) {
    g_dwGameMode = GAME_MODE_DEMO;
    g_dwRaceSessionType = RACE_SESSION_ATTRACT;
    SetCurrentScreenCallback(ProcessRacingScreen);
}

void StartCreditsRace(void) {
    g_dwGameMode = GAME_MODE_DEMO;
    if (g_dwMultiplayerMode) {
        g_dwRaceSessionType = RACE_SESSION_SCROLLING_CREDITS;
    } else {
        g_dwRaceSessionType = RACE_SESSION_CREDIT_CAPTIONS;
    }
    SetCurrentScreenCallback(ProcessRacingScreen);
}
