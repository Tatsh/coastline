#include "game.h"
#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/input.h"
#include "screen.h"
#if TGR_RENDERER == TGR_RENDERER_D3D
#include "platform/win/d3d/renderer.h"
#endif

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

int DispatchGameStateFrame(void) {
    return g_apfnGameStateHandlers[g_dwGameState]();
}
