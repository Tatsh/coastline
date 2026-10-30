#include "racedemo.h"

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "common/fade.h"
#include "common/game.h"
#include "common/ghost.h"
#include "common/race.h"
#include "common/rallyconfig.h"
#include "common/screen.h"
#include "common/season.h"
#include "common/soundbuffer.h"
#include "common/strings.h"
#include "common/trackselect.h"
#include "common/vehicle.h"
#include "common/vehicleaccess.h"

namespace coastline {

namespace {

// The time the race fades out in when a button leaves the demo, FADE_SECONDS of the race.
constexpr float DEMO_FADE_SECONDS = 0.2f;

constexpr s32 RACE_SCREEN_WIDTH = 640;
constexpr s32 RACE_SCREEN_HEIGHT = 480;

// The end of the demo of a cartridge, as the steps of the race before the last of its recording
// that the screen fades to black over and the time of the fade.
struct ConsoleDemoEnd {
    const char *release;
    unsigned steps;
    float seconds;
};

// The demos of the cartridges in the order they play. The North American release ends its demo by
// its time, with the fade of the race. The European release fades the screen over the last 15
// steps of its recording, 0.6 seconds, and the Japanese release clears it to black for its last 15,
// half a second.
constexpr std::array<ConsoleDemoEnd, 3> CONSOLE_DEMOS{{
    {"usa", 6, DEMO_FADE_SECONDS},
    {"jpn", 15, 1.0f / 30.0f},
    {"eur", 18, 0.6f},
}};

// The recordings of the PC release, before the demos of the cartridges.
constexpr std::array<const char *, 2> PC_DEMOS{"RallyIntro1.dat", "RallyIntro2.dat"};

// The demo to play next, by its place in the rotation, and the one playing. A demo of a cartridge
// chosen to play next takes the place of the next one when it was imported.
std::size_t g_nextDemo = 0;
const ConsoleDemoEnd *g_chosenDemo = nullptr;
std::string g_demoPath;
const ConsoleDemoEnd *g_consoleDemo = nullptr;

} // namespace

void OpenRaceData(bool playSounds) {
    // The race lays out its fades on the 640 by 480 screen of the default display mode of the PC
    // release, and the SDL game draws them across its frame.
    g_dwScreenWidth = RACE_SCREEN_WIDTH;
    g_dwScreenHeight = RACE_SCREEN_HEIGHT;
    LoadStringTableFromDll();
    g_GlobalRallyConfig.InitRallyConfigDefaults();
    g_GlobalRallyConfig.LoadDeviceDefaultBindings(0);
    g_GlobalRallyConfig.SelectActiveInputBindings();
    // OpenBossRallyPodAndInit() opens the sound of the race when the settings enable it.
    if (playSounds && !InitializeDirectSound()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "The sound effects of the race did not start.");
    }
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
    // The demo of a cartridge ends with its recording, as RunRacingPhase() ends a demo by its time.
    if (g_consoleDemo != nullptr && g_dwRaceInitialized && !IsFadingOut() &&
        g_adwGhostPlaybackFrame[1] + static_cast<int>(g_consoleDemo->steps) + 1 >=
            static_cast<int>(g_adwGhostFrameCount[1])) {
        g_dwRaceExitState = TRUE;
        g_bReplayAfterRace = FALSE;
        FadeSfxTo(0.0f, g_consoleDemo->seconds);
        SetFadeTarget(0.0f, g_consoleDemo->seconds);
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

std::array<std::uint8_t, 3> AttractDemoColor() {
    const u8 *color = VEHICLE_GET(&g_aVehicles[0], Color);
    return {color[0], color[1], color[2]};
}

bool AttractDemoFromConsole() {
    return g_consoleDemo != nullptr;
}

bool ChooseAttractDemo(std::string_view release) {
    for (const ConsoleDemoEnd &demo : CONSOLE_DEMOS) {
        if (release == demo.release) {
            g_chosenDemo = &demo;
            return true;
        }
    }
    return false;
}

} // namespace coastline

extern "C" const char *NextAttractDemoRecording(void) {
    using coastline::CONSOLE_DEMOS;
    using coastline::PC_DEMOS;
    // The rotation omits the cartridges that were not imported.
    std::vector<std::size_t> present;
    for (std::size_t i = 0; i < PC_DEMOS.size() + CONSOLE_DEMOS.size(); ++i) {
        if (i < PC_DEMOS.size()) {
            present.push_back(i);
            continue;
        }
        std::error_code error;
        auto json = std::filesystem::path("n64") / CONSOLE_DEMOS[i - PC_DEMOS.size()].release /
                    "attract.json";
        if (std::filesystem::exists(json, error)) {
            if (coastline::g_chosenDemo == &CONSOLE_DEMOS[i - PC_DEMOS.size()]) {
                coastline::g_nextDemo = present.size();
            }
            present.push_back(i);
        }
    }
    coastline::g_chosenDemo = nullptr;
    std::size_t demo = present[coastline::g_nextDemo % present.size()];
    coastline::g_nextDemo = (coastline::g_nextDemo + 1) % present.size();
    if (demo < PC_DEMOS.size()) {
        coastline::g_consoleDemo = nullptr;
        coastline::g_demoPath = PC_DEMOS[demo];
    } else {
        coastline::g_consoleDemo = &CONSOLE_DEMOS[demo - PC_DEMOS.size()];
        coastline::g_demoPath =
            (std::filesystem::path("n64") / coastline::g_consoleDemo->release / "attract.dat")
                .generic_string();
    }
    return coastline::g_demoPath.c_str();
}
