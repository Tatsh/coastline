#include "game.h"
#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/frontend.h"
#include "platform/win/input.h"
#include "platform/win/render.h"
#include "platform/win/settings.h"
#include "platform/win/sound.h"
#include "platform/win/timer.h"
#include "screen.h"
#include "trackselect.h"
#include "vehicle.h"
#if TGR_RENDERER == TGR_RENDERER_D3D
#include "platform/win/d3d/renderer.h"
#endif

namespace {
/**
 * The bits of a pixel of the surface of the front end.
 *
 * @ghidraAddress BRD3D: 0x10690a00
 */
int g_dwFrontEndSurfaceBitDepth;
/**
 * The height of the surface of the front end. The Glide library sets 480 as the game is set up.
 *
 * @ghidraAddress BRD3D: 0x10690a08
 * @ghidraAddress BRGlide: 0x105ccbb0
 */
int g_dwFrontEndSurfaceHeight;
/**
 * The refresh rate of the surface of the front end, 0 for the default.
 *
 * @ghidraAddress BRD3D: 0x10690a04
 */
int g_dwFrontEndSurfaceRefreshRate;
/**
 * The width of the surface of the front end. The Glide library sets 640 as the game is set up.
 *
 * @ghidraAddress BRD3D: 0x10690a0c
 * @ghidraAddress BRGlide: 0x105ccbb4
 */
int g_dwFrontEndSurfaceWidth;
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
#if TGR_RENDERER == TGR_RENDERER_D3D
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
#if TGR_RENDERER == TGR_RENDERER_D3D
        DisplayMode *pMode = g_pMainRenderWindow->GetMode();

        if (pMode != nullptr) {
            pMode->GetSurfaceDescDimensions(&g_dwFrontEndSurfaceWidth,
                                            &g_dwFrontEndSurfaceHeight,
                                            &g_dwFrontEndSurfaceBitDepth,
                                            &g_dwFrontEndSurfaceRefreshRate);
        }
#endif /* TGR_RENDERER == TGR_RENDERER_D3D */
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
