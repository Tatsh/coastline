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
 */
extern ScreenCallback g_pfnCurrentScreen;

/**
 * @ghidraAddress USA: 0x8031b31c
 * @ghidraAddress EUR: 0x8031d81c
 * @ghidraAddress JPN: 0x8031c44c
 */
extern ScreenCallback g_pfnNextScreen;

/**
 * @ghidraAddress USA: 0x8031b320
 * @ghidraAddress EUR: 0x8031d820
 * @ghidraAddress JPN: 0x8031c450
 */
extern ScreenCallback g_pfnSecondaryScreen;

/**
 * Set the routine the main loop calls each frame.
 *
 * @param callback The routine to call each frame.
 * @ghidraAddress USA: 0x8021c6e4
 * @ghidraAddress EUR: 0x8021d8d8
 * @ghidraAddress JPN: 0x8021d1a8
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
 */
s32 IsCurrentScreen(ScreenCallback callback);

/**
 * Set the second routine of the screen.
 *
 * @param callback The routine.
 * @ghidraAddress USA: 0x8021c6c4
 * @ghidraAddress EUR: 0x8021d8b8
 * @ghidraAddress JPN: 0x8021d188
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

#ifdef __cplusplus
}
#endif

#endif
