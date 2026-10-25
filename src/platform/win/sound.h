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
 * @ghidraAddress BRGlide: 0x104b1620
 */
extern UINT g_uEarSoundWindowMessage;

/**
 * The MCI device of the CD audio, in music mode 1.
 *
 * @ghidraAddress BRD3D: 0x10220c40
 */
extern MCIDEVICEID g_dwMciCdAudioDeviceId;

/**
 * Load the EAR library and start it for a window, when the game plays music in mode 2. Only the
 * first call does the work. A failure shows a message and exits the game.
 *
 * @param hWnd The window the library posts its messages to.
 * @return Always true.
 * @ghidraAddress BRD3D: 0x1002adc0
 * @ghidraAddress BRGlide: 0x10017e30
 */
bool InitializeEarSound(HWND hWnd);

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
 * Pause the music while the game is paused.
 *
 * @ghidraAddress BRD3D: 0x10002b70
 */
void PauseMusicPlayback(void);

/**
 * Silence every channel of sound that plays.
 *
 * @ghidraAddress BRD3D: 0x10072e00
 */
void MuteAllActiveSoundChannels(void);

/**
 * Play the track of the CD audio that is current, again.
 *
 * @ghidraAddress BRD3D: 0x10002510
 */
void PlayCdAudioFromCurrentTrack(void);

/**
 * Release DirectSound and its buffers.
 *
 * @ghidraAddress BRD3D: 0x10073730
 */
void ShutdownDirectSound(void);

/**
 * Release the device of the music.
 *
 * @ghidraAddress BRD3D: 0x10002cf0
 */
void ReleaseMusicDeviceReference(void);

/**
 * Drop a user of the EAR library. The last user shuts the library down.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x1002aea0
 * @ghidraAddress BRGlide: 0x10017f10
 */
bool ShutdownEarSound(void);
