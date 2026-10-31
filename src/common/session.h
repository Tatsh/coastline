#pragma once

/**
 * @file
 * The state of the session of the Boss Rally release the race shares with the front end.
 */

#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "common/frontendstate.h"
#endif

/**
 * Path of BossRally.cfg, built from g_szInstallDirectory.
 *
 * @ghidraAddress BRD3D: 0x10b4fbe8
 * @ghidraAddress BRGlide: 0x10b72f48
 */
#if TGR_PLATFORM != TGR_PLATFORM_SDL
extern char g_szConfigFilePath[];
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Whether the front end is up. MenuScreen::ShutdownFrontEnd() clears it, notices of network play
 * are posted only while it is set, and NullStub() is called after the textures are loaded again.
 *
 * @ghidraAddress BRD3D: 0x100ac300
 * @ghidraAddress BRGlide: 0x100abaa0
 */
#if TGR_PLATFORM != TGR_PLATFORM_SDL
extern int g_dwNullStubEnable;
#endif

/**
 * Release every subsystem of the game and end the process.
 *
 * @param nExitCode The exit code of the process.
 * @ghidraAddress BRD3D: 0x10038f30
 * @ghidraAddress BRGlide: 0x100325b0
 */
[[noreturn]] void ShutdownApplication(int nExitCode);

/**
 * Free the colour buffers of each racer.
 *
 * @ghidraAddress BRD3D: 0x1002c4a0
 * @ghidraAddress BRGlide: 0x10019a10
 */
void FreeAllRacerColorPanels(void);

#ifdef __cplusplus
}
#endif
