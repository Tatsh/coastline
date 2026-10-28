// The selections of the menus that the game reads. The PC release defines them with its menus in
// src/win32/frontend.cpp, which the SDL game does not have, and the initial values are those.

#include "win32/frontendstate.h"

#include "game.h"
#include "sdl/scene.h"
#include "sdl/win32types.h"

char g_szSeasonSaveFileName[MAX_PATH];
char g_szTimeAttackSaveFileName[MAX_PATH];
int g_dwFrontEndAtRootMenu;
int g_dwSelectedTransmission;
int g_dwTireMenuIndex = 1;
int g_dwSuspensionMenuIndex = 1;
int g_dwCarMenuIndex = 1;
int g_dwTotalLapsSetting = 3;
int g_dwAccumUnlockedCarMask;
int g_dwAccumUnlockedTrackMask;
int g_dwHandlingTypeMenuIndex = 4;

// The race of the benchmark of src/win32/boot/gamestate.cpp: an arcade race of track 2 with car 5
// on tyres 2, the first handling, suspension, and transmission, and clear weather.
SceneSelection g_SdlScene = {GAME_MODE_ARCADE, 2, 5, 0, 2, 1, 1, 1};
