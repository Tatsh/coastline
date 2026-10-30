#pragma once

/**
 * @file
 * The music of the PC port.
 */

#include <windows.h>
// The headers below need the types of windows.h.
#include <mmsystem.h>

#include "common/soundbuffer.h"
#include "common/soundsystem.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10002260
 * @ghidraAddress BRGlide: 0x10002580
 */
bool InitializeMusicPlayback(void);

/**
 * The event the music library signals at the end of a track.
 *
 * @return The event.
 * @ghidraAddress BRD3D: 0x10002480
 * @ghidraAddress BRGlide: 0x100027a0
 */
WPARAM GetEarMusicEventHandle(void);

/**
 * Make a track of the CD audio current, and post a notice of MCI to the window of the music that
 * the play finished, for the window to play the track.
 *
 * @param nTrack The track.
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10002870
 * @ghidraAddress BRGlide: 0x10002ba0
 */
bool PostCdAudioPlayNotify(int nTrack);

/**
 * Play a track of music through the EAR library.
 *
 * @param nTrack The track.
 * @return Always true.
 * @ghidraAddress BRD3D: 0x100027f0
 * @ghidraAddress BRGlide: 0x10002b20
 */
bool PlayEarMusicTrack(int nTrack);

/**
 * The track of music that plays, from the CD audio in music mode 1 and the EAR library otherwise.
 *
 * @return The track.
 * @ghidraAddress BRD3D: 0x10002910
 * @ghidraAddress BRGlide: 0x10002c50
 */
int GetCurrentMusicTrack(void);

/**
 * The track of the CD audio that plays, as MCI reports it.
 *
 * @return The track, or 0 when no music plays.
 * @ghidraAddress BRD3D: 0x100024c0
 * @ghidraAddress BRGlide: 0x100027e0
 */
int GetCdAudioCurrentTrack(void);

/**
 * The track of music the EAR library plays.
 *
 * @return The track, or 0 when no music plays.
 * @ghidraAddress BRD3D: 0x10002490
 * @ghidraAddress BRGlide: 0x100027b0
 */
int GetEarMusicCurrentTrack(void);

/**
 * The volume of the music the CD audio last had. Only whether it is 0 matters, and the music of the
 * CD is paused at 0.
 *
 * @ghidraAddress BRD3D: 0x10220cdc
 * @ghidraAddress BRGlide: 0x1021c80c
 */
extern unsigned char g_bCdMusicVolume;

/**
 * Play the current track of the music again when the music is active.
 *
 * @return The result of SetCurrentMusicTrack(), or true when the music is not active.
 * @ghidraAddress BRD3D: 0x10002b40
 * @ghidraAddress BRGlide: 0x10002e80
 */
bool ReplayCurrentMusicTrack(void);

/**
 * Pause the music of the CD when its volume turns 0, and play its track again when the volume turns
 * from 0, while a track plays. The volume is recorded in g_bCdMusicVolume.
 *
 * @param bVolume The volume.
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10002a80
 * @ghidraAddress BRGlide: 0x10002dc0
 */
bool HandleMusicPauseResume(unsigned char bVolume);

/**
 * Set the attenuation of the channel of the music of the EAR library from a volume, scaled from 0
 * to 255 into 0 to 10000, when the music is active.
 *
 * @param bVolume The volume.
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10002a20
 * @ghidraAddress BRGlide: 0x10002d60
 */
bool SetScaledAudioVolume(unsigned char bVolume);

/**
 * Play the tracks of the CD audio from a track on.
 *
 * @param hWnd The window told when the tracks end.
 * @param nTrack The first track.
 * @return The error of MCI, or 0.
 * @ghidraAddress BRD3D: 0x10002550
 * @ghidraAddress BRGlide: 0x10002870
 */
MCIERROR MciPlayCdTracks(HWND hWnd, unsigned char nTrack);

/**
 * The track of music that plays.
 *
 * @ghidraAddress BRD3D: 0x10220cd4
 * @ghidraAddress BRGlide: 0x1021c804
 */
extern int g_nCurrentMusicTrack;

/**
 * The first track of music.
 *
 * @ghidraAddress BRD3D: 0x10220c44
 * @ghidraAddress BRGlide: 0x1021c774
 */
extern int g_nMusicTrackMin;

/**
 * The last track of music.
 *
 * @ghidraAddress BRD3D: 0x10220c38
 * @ghidraAddress BRGlide: 0x1021c768
 */
extern int g_nMusicTrackMax;

/**
 * The window told when the tracks of the CD audio end.
 *
 * @ghidraAddress BRD3D: 0x10220c4c
 * @ghidraAddress BRGlide: 0x1021c77c
 */
extern HWND g_hMusicNotifyWnd;

/**
 * Whether the CD audio was paused.
 *
 * @ghidraAddress BRD3D: 0x10220cd8
 * @ghidraAddress BRGlide: 0x1021c808
 */
extern int g_bCdAudioPlaying;

/**
 * Play the next track of music, from the first again after the last.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x100029b0
 * @ghidraAddress BRGlide: 0x10002cf0
 */
bool AdvanceMusicTrackWrapping(void);

/**
 * Stop the track of music that plays. A CD audio that does not stop has its device closed.
 *
 * @return Whether the music stopped, and true when no music plays.
 * @ghidraAddress BRD3D: 0x10002c30
 * @ghidraAddress BRGlide: 0x10002f70
 */
bool StopMusicTrack(void);

/**
 * Close the music channel of the EAR library.
 *
 * @return The result of the EAR library, or 1 when no music plays.
 * @ghidraAddress BRD3D: 0x10002440
 * @ghidraAddress BRGlide: 0x10002760
 */
int ShutDownEarMusicChannel(void);

/**
 * Pause the CD audio. A CD audio that does not pause has its device closed.
 *
 * @return Whether the CD audio paused, and true when no music plays.
 * @ghidraAddress BRD3D: 0x10002ae0
 * @ghidraAddress BRGlide: 0x10002e20
 */
bool PauseCdAudio(void);

/**
 * Play the tracks of the CD audio from the current track on.
 *
 * @return Whether the tracks play, and true when no music plays.
 * @ghidraAddress BRD3D: 0x10002510
 * @ghidraAddress BRGlide: 0x10002830
 */
bool PlayCdAudioFromCurrentTrack(void);

/**
 * Set the music up, opening CD audio through MCI when g_dwPlayMusicMode is 1 and starting the
 * software music otherwise.
 *
 * @param hWnd The main window.
 * @return The result of OpenCdaudioMci() or InitSoftwareMusicStack().
 * @ghidraAddress BRD3D: 0x100025c0
 * @ghidraAddress BRGlide: 0x100028e0
 */
bool RouteMusicInitByFlag(HWND hWnd);

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
 * Whether music is active, with the tracks of the CD read or the channel of the EAR library
 * started.
 *
 * @ghidraAddress BRD3D: 0x10220c3c
 * @ghidraAddress BRGlide: 0x1021c76c
 */
extern int g_bMusicActive;

/**
 * Whether the output of the EAR library is set up.
 *
 * @ghidraAddress BRD3D: 0x10220c48
 * @ghidraAddress BRGlide: 0x1021c778
 */
extern int g_bEarOutputInitialized;

/**
 * Open the CD audio through MCI for the first user of the device of the music, with the time in
 * tracks, and read the number of tracks. The music then plays tracks 2 to the last.
 *
 * @param hWnd The window told of the end of a track.
 * @return False when the device does not open or answer, true otherwise.
 * @ghidraAddress BRD3D: 0x10002660
 * @ghidraAddress BRGlide: 0x10002980
 */
bool OpenCdaudioMci(HWND hWnd);

/**
 * Set up the EAR library for the first user of the device of the music and start its music.
 *
 * @param hWnd The main window.
 * @return Always true.
 * @ghidraAddress BRD3D: 0x100025f0
 * @ghidraAddress BRGlide: 0x10002910
 */
bool InitSoftwareMusicStack(HWND hWnd);

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

#ifdef __cplusplus
}
#endif
