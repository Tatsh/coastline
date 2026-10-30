#include "racedemo.h"

#include "common/game.h"
#include "common/podarchive.h"
#include "common/race.h"
#include "common/screen.h"
#include "common/season.h"

namespace coastline {

void OpenRaceData() {
    g_BossRallyPodArchive.SetFilePath("BossRally.pod");
    g_BossRallyPodArchive.OpenPodFile();
}

void StartAttractDemo() {
    g_dwGameMode = GAME_MODE_DEMO;
    g_dwRaceSessionType = RACE_SESSION_ATTRACT;
    g_dwRaceInitialized = 0;
    InitGameAndShowMenus();
}

bool StepAttractDemo() {
    // The race leaves for the front end through ExitRaceSessionToMenu(), which ends the session.
    if (IsCurrentScreen(ExitRaceSessionToMenu)) {
        CallCurrentGameMode();
        return false;
    }
    CallCurrentGameMode();
    return true;
}

} // namespace coastline
