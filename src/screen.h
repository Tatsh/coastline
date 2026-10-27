#ifndef COASTLINE_SCREEN_H
#define COASTLINE_SCREEN_H

/**
 * @file
 * The size of the frame, and the routines of the screen the main loop calls.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A routine the main loop calls once per frame. */
typedef void (*ScreenCallback)(void);

/**
 * The shift from the size of the frame to the size of the frame buffer, 1 in high resolution.
 *
 * @ghidraAddress USA: 0x8028a850
 * @ghidraAddress EUR: 0x8028ca50
 * @ghidraAddress JPN: 0x8028c520
 * @ghidraAddress BRD3D: 0x106c65e4
 */
extern s32 g_dwScreenScaleShift;

/**
 * The size of the frame in pixels.
 *
 * @ghidraAddress USA: 0x8028aab0
 * @ghidraAddress EUR: 0x8028ccb8
 * @ghidraAddress JPN: 0x8028c788
 * @ghidraAddress BRD3D: 0x106c0684
 * @ghidraAddress BRGlide: 0x106e7714
 */
extern s32 g_dwScreenWidth;

/**
 * @ghidraAddress USA: 0x8028aab4
 * @ghidraAddress EUR: 0x8028ccbc
 * @ghidraAddress JPN: 0x8028c78c
 * @ghidraAddress BRD3D: 0x106c299c
 * @ghidraAddress BRGlide: 0x106e9a2c
 */
extern s32 g_dwScreenHeight;

/**
 * The routines of the screen: the current one, the next, and a second.
 *
 * @ghidraAddress USA: 0x8031b318
 * @ghidraAddress EUR: 0x8031d818
 * @ghidraAddress JPN: 0x8031c448
 * @ghidraAddress BRD3D: 0x106c0964
 */
extern ScreenCallback g_pfnCurrentScreen;

/**
 * @ghidraAddress USA: 0x8031b31c
 * @ghidraAddress EUR: 0x8031d81c
 * @ghidraAddress JPN: 0x8031c44c
 * @ghidraAddress BRD3D: 0x106c198c
 */
extern ScreenCallback g_pfnNextScreen;

/**
 * @ghidraAddress USA: 0x8031b320
 * @ghidraAddress EUR: 0x8031d820
 * @ghidraAddress JPN: 0x8031c450
 * @ghidraAddress BRD3D: 0x106c1608
 */
extern ScreenCallback g_pfnSecondaryScreen;

/**
 * Set the routine the main loop calls each frame.
 *
 * @param callback The routine to call each frame.
 * @ghidraAddress USA: 0x8021c6e4
 * @ghidraAddress EUR: 0x8021d8d8
 * @ghidraAddress JPN: 0x8021d1a8
 * @ghidraAddress BRD3D: 0x10034c66
 */
void SetCurrentScreenCallback(ScreenCallback callback);

/**
 * Set a routine FlushDisplayList() calls once, when the RDP has finished the frame before, so that
 * it can change data that frame drew from.
 *
 * @param callback The routine to call once.
 * @ghidraAddress USA: 0x8021c6b8
 * @ghidraAddress EUR: 0x8021d8ac
 * @ghidraAddress JPN: 0x8021d17c
 * @ghidraAddress BRD3D: 0x10034c37
 */
void SetNextScreenCallback(ScreenCallback callback);

/**
 * Return whether a routine is the current screen.
 *
 * @param callback The routine to compare with the current screen.
 * @return 1 if the routine is the current screen, 0 otherwise.
 * @ghidraAddress USA: 0x8021c6d0
 * @ghidraAddress EUR: 0x8021d8c4
 * @ghidraAddress JPN: 0x8021d194
 * @ghidraAddress BRD3D: 0x10034c51
 */
s32 IsCurrentScreen(ScreenCallback callback);

/**
 * Set the second routine of the screen.
 *
 * @param callback The routine.
 * @ghidraAddress USA: 0x8021c6c4
 * @ghidraAddress EUR: 0x8021d8b8
 * @ghidraAddress JPN: 0x8021d188
 * @ghidraAddress BRD3D: 0x10034c44
 */
void SetSecondaryScreenCallback(ScreenCallback callback);

/**
 * Call the routine of the current screen.
 *
 * @ghidraAddress USA: 0x8021c6f0
 * @ghidraAddress EUR: 0x8021d8e4
 * @ghidraAddress JPN: 0x8021d1b4
 * @ghidraAddress BRD3D: 0x10034c73
 * @ghidraAddress BRGlide: 0x1002e324
 */
void CallCurrentGameMode(void);

/**
 * Call the current screen routine forever on N64 and return immediately on Windows.
 *
 * @ghidraAddress USA: 0x8021c718
 * @ghidraAddress EUR: 0x8021d90c
 * @ghidraAddress JPN: 0x8021d1dc
 * @ghidraAddress BRD3D: 0x10034c7e
 */
void GameModeMainLoop(void);

/**
 * Empty update callback.
 *
 * @ghidraAddress USA: 0x8021c740
 * @ghidraAddress EUR: 0x8021d940
 * @ghidraAddress JPN: 0x8021d210
 * @ghidraAddress BRD3D: 0x10034c83
 */
void StubUpdateCallback(void);

/**
 * Empty render callback.
 *
 * @ghidraAddress USA: 0x8021c6b0
 * @ghidraAddress EUR: 0x8021d8a4
 * @ghidraAddress JPN: 0x8021d174
 * @ghidraAddress BRD3D: 0x10034c32
 */
void StubRenderCallback(void);

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * Do nothing.
 *
 * @ghidraAddress BRD3D: 0x10008b80
 * @ghidraAddress BRGlide: 0x10008d60
 */
void NullStub(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SCREEN_H */
