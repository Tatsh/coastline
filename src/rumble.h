#ifndef COASTLINE_RUMBLE_H
#define COASTLINE_RUMBLE_H

/**
 * @file
 * The Rumble Paks of the first two controllers.
 */

#include <PR/os.h>
#include <PR/os_cont.h>
#include <PR/ultratypes.h>

/** The controllers whose Rumble Paks the game drives. */
#define RUMBLE_CHANNELS 2

/**
 * The Pak handle of each controller.
 *
 * @ghidraAddress USA: 0x8031a3f8
 * @ghidraAddress JPN: 0x8031b528
 * @ghidraAddress EUR: 0x8031c8f8
 */
extern OSPfs g_aContPfs[MAXCONTROLLERS];

/**
 * The queue of SI events.
 *
 * @ghidraAddress USA: 0x80272d48
 * @ghidraAddress JPN: 0x80274a18
 * @ghidraAddress EUR: 0x80274f48
 */
extern OSMesgQueue g_SiMesgQueue;

/**
 * Whether each controller holds a Rumble Pak. The start of the system checks all four; the rumble
 * reads the first RUMBLE_CHANNELS.
 *
 * @ghidraAddress USA: 0x8031b1e8
 * @ghidraAddress JPN: 0x8031c318
 * @ghidraAddress EUR: 0x8031d6e8
 */
extern u8 g_abRumblePakPresent[MAXCONTROLLERS];

/**
 * Whether the serial interface is free of Controller Pak transfers, so the motors may run.
 *
 * @ghidraAddress USA: 0x802a4be8
 * @ghidraAddress JPN: 0x802a6e10
 * @ghidraAddress EUR: 0x802a7370
 */
extern s16 g_wSerialInterfaceIdle;

/**
 * Whether each motor is pulsing.
 *
 * @ghidraAddress USA: 0x802a4bec
 * @ghidraAddress JPN: 0x802a6e14
 * @ghidraAddress EUR: 0x802a7374
 */
extern s16 g_awRumbleActive[RUMBLE_CHANNELS];

/** Whether each motor is in the on half of its pulse. @ghidraAddress USA: 0x802a4bf0 */
extern s16 g_awRumbleMotorOn[RUMBLE_CHANNELS];

/** The frames each pulse stays on. @ghidraAddress USA: 0x802a4bf4 */
extern s16 g_awRumbleOnFrames[RUMBLE_CHANNELS];

/** The frames each pulse stays off. @ghidraAddress USA: 0x802a4bf8 */
extern s16 g_awRumbleOffFrames[RUMBLE_CHANNELS];

/**
 * The frames left in the current half of each pulse.
 *
 * @ghidraAddress USA: 0x802a4bfc
 * @ghidraAddress JPN: 0x802a6e24
 * @ghidraAddress EUR: 0x802a7384
 */
extern s16 g_awRumbleCounter[RUMBLE_CHANNELS];

/** The frames left before each rumble ends. @ghidraAddress USA: 0x802a4c00 */
extern s16 g_awRumbleFramesLeft[RUMBLE_CHANNELS];

/**
 * The frames until the next probe for Rumble Paks, modulo 64.
 *
 * @ghidraAddress USA: 0x802a4c08
 * @ghidraAddress JPN: 0x802a6e30
 * @ghidraAddress EUR: 0x802a7390
 */
extern u32 g_dwRumbleProbeTimer;

/**
 * Pulse the motors of the Rumble Paks during a race, stop them outside one, and probe for Paks
 * every 64 calls. The audio thread calls it once per buffer.
 *
 * @param unused The routine never reads it. Its caller passes 0.
 * @ghidraAddress USA: 0x80260490
 * @ghidraAddress JPN: 0x80261420
 * @ghidraAddress EUR: 0x80261970
 * @stub{EUR JPN}
 */
void UpdateRumblePaks(s32 unused);

/**
 * Called before the Rumble Paks are first probed. It does nothing in the release.
 *
 * @ghidraAddress USA: 0x802607ac
 * @ghidraAddress JPN: 0x8026175c
 * @ghidraAddress EUR: 0x80261cac
 */
void PrepareRumblePaks(void);

#endif
