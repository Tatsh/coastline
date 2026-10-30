#include "racedemo.h"

#include "common/fade.h"
#include "common/game.h"
#include "common/ghost.h"
#include "common/podarchive.h"
#include "common/race.h"
#include "common/rallyconfig.h"
#include "common/screen.h"
#include "common/season.h"
#include "common/trackselect.h"

namespace coastline {

namespace {

// The time the race fades out in when a button leaves the demo, FADE_SECONDS of the race.
constexpr float DEMO_FADE_SECONDS = 0.2f;

constexpr s32 RACE_SCREEN_WIDTH = 640;
constexpr s32 RACE_SCREEN_HEIGHT = 480;

} // namespace

void OpenRaceData() {
    // The race lays out its fades and its text on the 640 by 480 screen of the default display
    // mode of the PC release, and the SDL game scales that screen to its frame.
    g_dwScreenWidth = RACE_SCREEN_WIDTH;
    g_dwScreenHeight = RACE_SCREEN_HEIGHT;
    g_GlobalRallyConfig.InitRallyConfigDefaults();
    g_GlobalRallyConfig.LoadDeviceDefaultBindings(0);
    g_GlobalRallyConfig.SelectActiveInputBindings();
    g_BossRallyPodArchive.SetFilePath("BossRally.pod");
    g_BossRallyPodArchive.OpenPodFile();
}

void StartAttractDemo() {
    g_dwGameMode = GAME_MODE_DEMO;
    g_dwRaceSessionType = RACE_SESSION_ATTRACT;
    g_dwRaceInitialized = 0;
    InitGameAndShowMenus();
}

bool StepAttractDemo(bool leave) {
    // The race leaves for the front end through ExitRaceSessionToMenu(), which ends the session.
    if (IsCurrentScreen(ExitRaceSessionToMenu)) {
        CallCurrentGameMode();
        return false;
    }
    // The buttons of the controllers of the SDL game leave the demo as ProcessRaceButtons() leaves
    // it for the pause binding.
    if (leave && !IsFadingOut()) {
        g_bDemoInterrupted = TRUE;
        g_dwRaceExitState = TRUE;
        g_bReplayAfterRace = FALSE;
        FadeSfxTo(0.0f, DEMO_FADE_SECONDS);
        SetFadeTarget(0.0f, DEMO_FADE_SECONDS);
    }
    CallCurrentGameMode();
    // The PC release advances the fades with each frame it draws, in RenderRacingScreen(), and the
    // SDL game advances them with each frame of the race. The race ends once its fade out is done.
    UpdateFadeTransitions();
    return true;
}

bool AttractDemoInterrupted() {
    return g_bDemoInterrupted != 0;
}

int AttractDemoTrack() {
    return static_cast<int>(g_dwSelectedTrackId);
}

int AttractDemoCar() {
    return static_cast<int>(g_GhostRecordingInfo.car);
}

} // namespace coastline
