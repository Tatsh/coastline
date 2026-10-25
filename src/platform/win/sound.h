#pragma once

/**
 * @file
 * The sound and music of the PC port.
 */

#include <windows.h>

/**
 * The message the music library sends the main window, in music mode 2.
 *
 * @ghidraAddress BRD3D: 0x10575478
 */
extern UINT g_uEarSoundWindowMessage;

/**
 * The MCI device of the CD audio, in music mode 1.
 *
 * @ghidraAddress BRD3D: 0x10220c40
 */
extern MCIDEVICEID g_dwMciCdAudioDeviceId;

/**
 * Start the sound of the game for a window, when the game plays music in mode 2.
 *
 * @param hWnd The window.
 * @ghidraAddress BRD3D: 0x1002adc0
 */
void InitializeEarSound(HWND hWnd);

/**
 * Start the music library and its channel.
 *
 * @ghidraAddress BRD3D: 0x10002260
 */
void InitializeMusicPlayback(void);

/**
 * The event the music library signals at the end of a track.
 *
 * @return The event.
 * @ghidraAddress BRD3D: 0x10002480
 */
WPARAM GetEarMusicEventHandle(void);

/**
 * Play a track of music.
 *
 * @param nModeIndex The track.
 * @ghidraAddress BRD3D: 0x100027c0
 */
void SetCurrentMusicTrack(int nModeIndex);

/**
 * Play the next track of music, from the first again after the last.
 *
 * @ghidraAddress BRD3D: 0x100029b0
 */
void AdvanceMusicTrackWrapping(void);

/**
 * Stop the track of music that plays.
 *
 * @ghidraAddress BRD3D: 0x10002c30
 */
void StopMusicTrack(void);

/**
 * Close the channel of the music library.
 *
 * @ghidraAddress BRD3D: 0x10002440
 */
void ShutDownEarMusicChannel(void);

/**
 * Play the track of the CD audio that is current, again.
 *
 * @ghidraAddress BRD3D: 0x10002510
 */
void PlayCdAudioFromCurrentTrack(void);
