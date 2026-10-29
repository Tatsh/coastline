#include "common/game.h"
#include "common/screen.h"
#include "common/trackselect.h"
#include "common/vehicle.h"
#include "win32/app.h"
#include "win32/rallyconfig.h"
#if TGR_PLATFORM != TGR_PLATFORM_SDL
#include "win32/frontend.h"
#else
#include "win32/frontendstate.h"
#endif
#include "common/settings.h"
#include "win32/input.h"
#include "win32/render.h"
#include "win32/sound.h"
#include "win32/timer.h"
#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/renderhost.h"
#include "sdl/scene.h"
#elif TGR_BRD3D
#include "win32/d3d/renderer.h"
#endif

namespace {
/**
 * The bits of a pixel of the surface of the front end.
 *
 * @ghidraAddress BRD3D: 0x10690a00
 */
[[maybe_unused, gnu::used]] int g_dwFrontEndSurfaceBitDepth;
/**
 * The height of the surface of the front end. The Glide library sets 480 as the game is set up.
 *
 * @ghidraAddress BRD3D: 0x10690a08
 * @ghidraAddress BRGlide: 0x105ccbb0
 */
[[maybe_unused, gnu::used]] int g_dwFrontEndSurfaceHeight;
/**
 * The refresh rate of the surface of the front end, 0 for the default.
 *
 * @ghidraAddress BRD3D: 0x10690a04
 */
[[maybe_unused, gnu::used]] int g_dwFrontEndSurfaceRefreshRate;
/**
 * The width of the surface of the front end. The Glide library sets 640 as the game is set up.
 *
 * @ghidraAddress BRD3D: 0x10690a0c
 * @ghidraAddress BRGlide: 0x105ccbb4
 */
[[maybe_unused, gnu::used]] int g_dwFrontEndSurfaceWidth;
} // namespace

int g_dwGameState;
int g_dwGameModeFrameCounter;
int g_dwGameModeContinue = true;
int (*g_apfnGameStateHandlers[])(void) = {
    GameState_InitResources,
    GameState_EnterGameMode,
    GameState_RunGameMode,
    GameState_ShowLoadingScreenAndInitGame,
    GameState_RunFrontEnd,
};

// Set the display mode of the configuration with the renderer of the library.
static inline void ResetConfiguredRenderSubsystems(void) {
    ResetRenderSubsystems(RENDER_BACKEND_MODE,
                          g_GlobalRallyConfig.dwDisplayModeWidth,
                          g_GlobalRallyConfig.dwDisplayModeHeight,
                          g_GlobalRallyConfig.dwDisplayModeBitDepth,
                          g_GlobalRallyConfig.dwDisplayModeExtra);
}

int GameState_InitResources(void) {
    OpenBossRallyPodAndInit();
    LoadSoundSet(SOUND_SET_FRONT_END);
    InitJoystickBindingNames();
    g_dwGameState = GAME_STATE_RUN_FRONT_END;
    return true;
}

int GameState_EnterGameMode(void) {
    g_dwGameState = GAME_STATE_RUN_GAME_MODE;
    return true;
}

int GameState_RunGameMode(void) {
    g_dwGameModeFrameCounter++;
    CallCurrentGameMode();
    return g_dwGameModeContinue;
}

int GameState_ShowLoadingScreenAndInitGame(void) {
    ResetConfiguredRenderSubsystems();
#if TGR_BRD3D
    g_pRenderHost->RenderLoadingScreen(g_pMainRenderWindow, "loading.img", 0);
    ResetConfiguredRenderSubsystems();
#else
    ShowGlideSplashImage("loading.img", 0);
#endif
    InitGameAndShowMenus();
    g_dwGameState = GAME_STATE_ENTER_GAME_MODE;
    return true;
}

// The track, the car, and the tyres of the benchmark.
#define BENCHMARK_TRACK 2
#define BENCHMARK_CAR 5
#define BENCHMARK_TIRES 2
// The milliseconds without input at the root screen before the attract race starts.
#define ATTRACT_IDLE_MS 90000
#if TGR_RENDERER == TGR_RENDERER_GLIDE
// The size of the screen of the menus.
#define MENU_SCREEN_WIDTH 640
#define MENU_SCREEN_HEIGHT 480
#endif

#if TGR_PLATFORM == TGR_PLATFORM_SDL
int GameState_RunFrontEnd(void) {
    // The SDL game has no menus of the PC release, and starts the race of its command line as the
    // benchmark does.
    g_dwGameMode = g_SdlScene.gameMode;
    g_dwSelectedTrackId = g_SdlScene.track;
    g_dwChosenCar = g_SdlScene.car;
    g_dwChosenWeather = g_SdlScene.weather;
    g_GlobalRallyConfig.dwInputDeviceType = 0;
    g_GlobalRallyConfig.pActiveInputBindings = g_GlobalRallyConfig.aInputBindingsKeyboard;
    g_dwHandlingType = g_SdlScene.handling;
    g_dwSuspensionType = g_SdlScene.suspension;
    g_dwTireType = g_SdlScene.tires;
    g_dwTransmissionType = g_SdlScene.transmission;
    g_dwNullStubEnable = false;
    g_dwGameState = GAME_STATE_LOAD_GAME;
    return true;
}
#else
int GameState_RunFrontEnd(void) {
    MenuScreen *pScreen = g_pActiveMenuScreen;

    if (pScreen == nullptr && !g_dwNullStubEnable) {
#if TGR_RENDERER == TGR_RENDERER_GLIDE
        g_dwFrontEndSurfaceWidth = MENU_SCREEN_WIDTH;
        g_dwFrontEndSurfaceHeight = MENU_SCREEN_HEIGHT;
        g_dwConfigFramebufferWidth = MENU_SCREEN_WIDTH;
        g_dwScreenWidth = MENU_SCREEN_WIDTH;
        g_dwConfigFramebufferHeight = MENU_SCREEN_HEIGHT;
        g_dwScreenHeight = MENU_SCREEN_HEIGHT;
#endif
        ResetAudioSubsystem();
        g_dwGameState = GAME_STATE_LOAD_GAME;
        return true;
    }
    if (pScreen == nullptr && g_dwNullStubEnable) {
        LoadSoundSet(SOUND_SET_FRONT_END);
        InitializeFrontEndResources();
        g_dwLastInputActivityTime = GetHighResolutionTime();
        pScreen = g_pActiveMenuScreen;
    }
    if (g_dwRunBenchmark) {
        g_dwGameMode = GAME_MODE_ARCADE;
        g_dwSelectedTrackId = BENCHMARK_TRACK;
        g_dwChosenCar = BENCHMARK_CAR;
        g_dwChosenWeather = 0;
        g_GlobalRallyConfig.dwInputDeviceType = 0;
        g_GlobalRallyConfig.pActiveInputBindings = g_GlobalRallyConfig.aInputBindingsKeyboard;
        g_dwHandlingType = 1;
        g_dwSuspensionType = 1;
        g_dwTireType = BENCHMARK_TIRES;
        g_dwTransmissionType = 1;
        g_dwNullStubEnable = false;
        g_dwGameState = GAME_STATE_LOAD_GAME;
        return true;
    }
    if (!pScreen->IsBuilt()) {
#if TGR_BRD3D
        DisplayMode *pMode = g_pMainRenderWindow->GetMode();

        if (pMode != nullptr) {
            pMode->GetSurfaceDescDimensions(&g_dwFrontEndSurfaceWidth,
                                            &g_dwFrontEndSurfaceHeight,
                                            &g_dwFrontEndSurfaceBitDepth,
                                            &g_dwFrontEndSurfaceRefreshRate);
        }
#endif /* TGR_BRD3D */
        pScreen->ActivateMenuScreen();
    } else {
        pScreen->RunFrame();
    }
    if (g_pActiveMenuScreen != nullptr && g_dwNullStubEnable) {
        if (!g_dwFrontEndAtRootMenu) {
            g_dwLastInputActivityTime = GetHighResolutionTime();
        } else if (g_dwLastInputActivityTime + ATTRACT_IDLE_MS < GetHighResolutionTime()) {
            g_pActiveMenuScreen->SetRunning(false);
            g_pActiveMenuScreen->ShutdownFrontEnd(false);
            g_dwGameMode = GAME_MODE_DEMO;
            g_dwRaceSessionType = RACE_SESSION_ATTRACT;
        }
    }
    return true;
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */

void SetGameStateFrontEnd(void) {
    g_dwNullStubEnable = true;
    g_dwGameState = GAME_STATE_RUN_FRONT_END;
}

int DispatchGameStateFrame(void) {
    return g_apfnGameStateHandlers[g_dwGameState]();
}

void ResetAllPlayers(void) {
    for (int player = 0; player < MAX_PLAYERS; ++player) {
        g_aVehicles[player].InitPlayerVehicleSlot();
        g_aPlayerInputState[player].InitPlayerInputState();
    }
}
