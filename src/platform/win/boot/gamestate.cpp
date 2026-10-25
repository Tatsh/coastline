#include "game.h"
#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/frontend.h"
#include "platform/win/input.h"
#include "platform/win/settings.h"
#include "platform/win/sound.h"
#include "platform/win/timer.h"
#include "screen.h"
#include "trackselect.h"
#if TGR_RENDERER == TGR_RENDERER_D3D
#include "platform/win/d3d/renderer.h"
#endif

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

// The first set of sounds.
#define FIRST_SOUND_SET 0
// The mode of ResetRenderSubsystems() of each renderer.
#if TGR_RENDERER == TGR_RENDERER_D3D
#define RENDER_BACKEND_MODE 1
#else
#define RENDER_BACKEND_MODE 3
#endif

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
    LoadSoundSet(FIRST_SOUND_SET);
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
        LoadSoundSet(FIRST_SOUND_SET);
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
    if (!pScreen->bBuilt) {
#if TGR_RENDERER == TGR_RENDERER_D3D
        DisplayMode *pMode = g_pMainRenderWindow->pMode;

        if (pMode != nullptr) {
            pMode->GetSurfaceDescDimensions(&g_dwFrontEndSurfaceWidth,
                                            &g_dwFrontEndSurfaceHeight,
                                            &g_dwFrontEndSurfaceBitDepth,
                                            &g_dwFrontEndSurfaceRefreshRate);
        }
#endif
        pScreen->ActivateMenuScreen();
    } else {
        pScreen->RunFrame();
    }
    if (g_pActiveMenuScreen != nullptr && g_dwNullStubEnable) {
        if (!g_dwFrontEndAtRootMenu) {
            g_dwLastInputActivityTime = GetHighResolutionTime();
        } else if (g_dwLastInputActivityTime + ATTRACT_IDLE_MS < GetHighResolutionTime()) {
            g_pActiveMenuScreen->bRunning = false;
            g_pActiveMenuScreen->ShutdownFrontEnd(false);
            g_dwGameMode = GAME_MODE_DEMO;
            g_dwRaceSessionType = RACE_SESSION_ATTRACT;
        }
    }
    return true;
}

int DispatchGameStateFrame(void) {
    return g_apfnGameStateHandlers[g_dwGameState]();
}
