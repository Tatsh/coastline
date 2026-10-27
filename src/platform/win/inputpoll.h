#ifndef COASTLINE_PLATFORM_WIN_INPUTPOLL_H
#define COASTLINE_PLATFORM_WIN_INPUTPOLL_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Poll input devices and translate active bindings into race or replay actions.
 *
 * @param pnSteering Receives steering outside replay and early failure paths.
 * @param pnAcceleration Receives acceleration outside replay, pause, and early failure paths.
 * @return Action mask, also written to g_dwInputButtonMask.
 * @ghidraAddress BRD3D: 0x100773f0
 */
unsigned ReadInputDevicesButtonMask(int *pnSteering, int *pnAcceleration);

#ifdef __cplusplus
}
#endif

#endif
