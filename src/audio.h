#ifndef COASTLINE_AUDIO_H
#define COASTLINE_AUDIO_H

/**
 * @file
 * The music player of FastTracker 2 modules, the sound channels, and the mixing thread.
 */

#include <PR/ultratypes.h>

/**
 * The menu music, an XM module the game decompresses below the program. The link script places
 * it.
 *
 * @ghidraAddress USA: 0x80025c00
 */
extern u8 g_abMenuMusicModule[];

/** The work memory of the music player, at the start of the BSS. @ghidraAddress USA: 0x802ac400 */
extern u8 g_abMusicModuleWorkBuffer[];

/**
 * Start the audio interface, the music player and the mixing thread.
 *
 * @ghidraAddress USA: 0x802575c4
 */
void InitAudioSystem(void *module, void *work);

#endif
