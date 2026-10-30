#ifndef COASTLINE_PLATFORM_WIN_INPUTPOLL_H
#define COASTLINE_PLATFORM_WIN_INPUTPOLL_H

/**
 * Most recent race or replay action mask.
 * @ghidraAddress BRD3D: 0x118abad0
 * @ghidraAddress BRGlide: 0x118eebe8
 */
extern unsigned g_dwInputButtonMask;

/**
 * Whether input polling sent network readiness.
 * @ghidraAddress BRD3D: 0x118abdc0
 * @ghidraAddress BRGlide: 0x118eeed8
 */
extern int g_bNetReadyToStartSent;

/**
 * Whether local input requested a pause action.
 * @ghidraAddress BRD3D: 0x1022af20
 * @ghidraAddress BRGlide: 0x10226a50
 */
extern int g_bNetGamePaused;

#ifdef __cplusplus
/**
 * Start or stop the spring and ramp its coefficient towards the scaled force bounds.
 *
 * @param bReduceForce Whether to reduce the coefficient towards the lower bound.
 * @param bEnableForce Whether to play the spring.
 * @param nForce Upper bound input, offset by eight before scaling.
 * @ghidraAddress BRD3D: 0x10078f20
 * @ghidraAddress BRGlide: 0x10072210
 */
void UpdateForceFeedbackForce(int bReduceForce, int bEnableForce, int nForce);

/**
 * Set the first direction of the effects of force feedback, when a joystick with force feedback is
 * in use and no instant replay plays.
 *
 * @param nDirection The direction.
 * @ghidraAddress BRD3D: 0x10078e10
 * @ghidraAddress BRGlide: 0x10072100
 */
void SetForceFeedbackDirection(int nDirection);
#endif /* __cplusplus */

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
 * @ghidraAddress BRGlide: 0x100706d0
 */
unsigned ReadInputDevicesButtonMask(int *pnSteering, int *pnAcceleration);

/**
 * Whether the respawn binding was pressed.
 * @ghidraAddress BRD3D: 0x118abdcc
 * @ghidraAddress BRGlide: 0x118eeee4
 */
extern int g_bRespawnRequested;

/**
 * Clear both states of the mouse.
 *
 * @ghidraAddress BRD3D: 0x100770c0
 * @ghidraAddress BRGlide: 0x100703a0
 */
void ResetMouseState(void);

/**
 * Add a user of the game controller of dwInputDeviceType. The first user creates the device,
 * exclusively with force feedback when it is on and available and shared otherwise, and sets the
 * range of X and Y to -128 to 128 with no dead zone. A property that is not set releases the
 * device.
 *
 * @return dwInputDeviceType of g_GlobalRallyConfig, or 0 without a game controller.
 * @ghidraAddress BRD3D: 0x100791d0
 * @ghidraAddress BRGlide: 0x100724c0
 */
int InitializeJoystickDevice(void);

/**
 * Drop a user of the game controller device. The last user releases the effects of force feedback
 * and the device. A count that falls below 0 is set back to 0.
 *
 * @ghidraAddress BRD3D: 0x10079550
 * @ghidraAddress BRGlide: 0x10072840
 */
void ReleaseJoystickDevice(void);

/**
 * Set the effects of force feedback to last 250 milliseconds, under the conditions of
 * SetForceFeedbackDirection().
 *
 * @ghidraAddress BRD3D: 0x10078e50
 * @ghidraAddress BRGlide: 0x10072140
 */
void SetForceFeedbackLongDuration(void);

/**
 * Set the effects of force feedback to last 125 milliseconds, under the conditions of
 * SetForceFeedbackDirection().
 *
 * @ghidraAddress BRD3D: 0x10078e90
 * @ghidraAddress BRGlide: 0x10072180
 */
void SetForceFeedbackShortDuration(void);

/**
 * Start the jolt of force feedback with the duration of g_dwForceFeedbackDuration and its
 * direction, under the conditions of SetForceFeedbackDirection().
 *
 * @ghidraAddress BRD3D: 0x10078ed0
 * @ghidraAddress BRGlide: 0x100721c0
 */
void StartForceFeedbackJolt(void);

#if TGR_PLATFORM == TGR_PLATFORM_SDL
/**
 * Read the second controller of the SDL game as the console reads a pad, for the second player of
 * a race.
 *
 * @param buttons Receives the buttons held, in the bits of libultra.
 * @param stickX Receives the stick, right positive.
 * @param stickY Receives the stick, up positive.
 * @return Whether a device is bound to the second controller.
 */
int ReadSecondControllerPad(unsigned short *buttons, signed char *stickX, signed char *stickY);
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLATFORM_WIN_INPUTPOLL_H */
