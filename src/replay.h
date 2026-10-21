#ifndef COASTLINE_REPLAY_H
#define COASTLINE_REPLAY_H

/**
 * @file
 * The ghost recording a race replays.
 */

#include <PR/ultratypes.h>

/**
 * The recorded input of the ghost. A Controller Pak or the cartridge fills it.
 *
 * @ghidraAddress USA: 0x80307f00
 */
extern u8 g_abGhostRecording[];

/** The bytes of g_abGhostRecording in use. @ghidraAddress USA: 0x80270784 */
extern u32 g_dwGhostRecordingSize;

#endif
