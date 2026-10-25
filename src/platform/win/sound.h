#pragma once

/**
 * @file
 * The sound and music of the PC port.
 */

#include <windows.h>
// The headers below need the types of windows.h.
#include <dsound.h>
#include <mmsystem.h>

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
 * @ghidraAddress BRGlide: 0x1021c770
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
 * DirectSound.
 *
 * @ghidraAddress BRD3D: 0x118290f8
 * @ghidraAddress BRGlide: 0x1184c458
 */
extern IDirectSound *g_pDirectSound;

/**
 * The users of DirectSound. The last ShutdownDirectSound() releases it.
 *
 * @ghidraAddress BRD3D: 0x118290fc
 * @ghidraAddress BRGlide: 0x1184c45c
 */
extern int g_dwDirectSoundRefCount;

/**
 * The primary buffer of DirectSound.
 *
 * @ghidraAddress BRD3D: 0x11828fe4
 * @ghidraAddress BRGlide: 0x1184c344
 */
extern IDirectSoundBuffer *g_pDirectSoundPrimaryBuffer;

/**
 * The wave format of the primary buffer, in locked global memory.
 *
 * @ghidraAddress BRD3D: 0x11828f50
 * @ghidraAddress BRGlide: 0x1184c2b0
 */
extern WAVEFORMATEX *g_pDirectSoundWaveFormat;

/**
 * The list of the sound buffers, in locked global memory. The layout is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x11828f48
 * @ghidraAddress BRGlide: 0x1184c2a8
 */
extern void *g_pSoundBufferList;

/**
 * Stop the sounds and reset the state of the audio.
 *
 * @ghidraAddress BRD3D: 0x100734f0
 * @ghidraAddress BRGlide: 0x1006c460
 */
void ResetAudioSubsystem(void);

/**
 * Drop a user of DirectSound. The last user resets the audio, stops and releases the primary
 * buffer and DirectSound, and frees the wave format and the list of sound buffers.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10073730
 * @ghidraAddress BRGlide: 0x1006c6a0
 */
bool ShutdownDirectSound(void);

/**
 * The channel of the EAR library the music plays on, 100.
 *
 * @ghidraAddress BRD3D: 0x100940a8
 * @ghidraAddress BRGlide: 0x1007b078
 */
extern int g_dwEarMusicChannel;

/**
 * The users of the device of the music.
 *
 * @ghidraAddress BRD3D: 0x10220cd0
 * @ghidraAddress BRGlide: 0x1021c800
 */
extern int g_nMusicDeviceRefCount;

/**
 * Whether the EAR library plays music.
 *
 * @ghidraAddress BRD3D: 0x10220c3c
 * @ghidraAddress BRGlide: 0x1021c76c
 */
extern bool g_bEarMusicActive;

/**
 * Drop a user of the device of the music. In music mode 1 the CD audio stops, and the last user
 * closes its device. In mode 2 the music channel of the EAR library is cleared, and the last user
 * shuts the library down.
 *
 * @return Whether every command of the device succeeded, and true in music mode 0.
 * @ghidraAddress BRD3D: 0x10002cf0
 * @ghidraAddress BRGlide: 0x10003030
 */
bool ReleaseMusicDeviceReference(void);

/**
 * Drop a user of the EAR library. The last user shuts the library down.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x1002aea0
 * @ghidraAddress BRGlide: 0x10017f10
 */
bool ShutdownEarSound(void);
