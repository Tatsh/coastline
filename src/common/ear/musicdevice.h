#ifndef COASTLINE_EAR_MUSICDEVICE_H
#define COASTLINE_EAR_MUSICDEVICE_H

#include <stdint.h>

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Result of the most recent CD open through OpenEarMusicDevice(), cleared by CloseCdAudioDevice().
 * @ghidraAddress EAR: 0x1003b7bc
 */
/** @cond */
extern int32_t g_bCdAudioInitialized;

/**
 * Nonzero after successfully opening the MIDI sequencer through OpenEarMusicDevice().
 * CloseActiveMidiSequencer() does not clear the marker; CloseMidiMusicDevice() does.
 * @ghidraAddress EAR: 0x1003b7c0
 */
extern int32_t g_bMidiSequencerInitialized;

/**
 * One-based first audio-track number found during discovery, initially zero.
 * Native rediscovery checks only track 1 when an existing value is nonzero, preserving the value
 * unless track 1 is audio.
 * @ghidraAddress EAR: 0x1003b6f4
 */
extern uint16_t g_wFirstCdAudioTrack;
/**
 * Last permitted audio-track number, set to total track count when an audio track is found.
 * @ghidraAddress EAR: 0x1003b6f8
 */
extern uint16_t g_wLastCdAudioTrack;

/**
 * Configured Windows CD mixer device identifier, initially unsigned -1.
 * @ghidraAddress EAR: 0x100360d8
 */
extern UINT g_uConfiguredCdMixerId;
/**
 * Configured Windows MIDI mixer device identifier, initially unsigned -1.
 * @ghidraAddress EAR: 0x100360dc
 */
extern UINT g_uConfiguredMidiMixerId;
/**
 * Configured CD mixer line identifier, initially unsigned -1.
 * @ghidraAddress EAR: 0x100360e0
 */
extern DWORD g_dwConfiguredCdMixerLineId;
/**
 * Configured MIDI mixer line identifier, initially unsigned -1.
 * @ghidraAddress EAR: 0x100360e4
 */
extern DWORD g_dwConfiguredMidiMixerLineId;
/** @endcond */

/**
 * Open the mixers and then the CD audio device or the MIDI sequencer.
 *
 * @param cdAudio Whether to open the CD audio device rather than the MIDI sequencer.
 * @ghidraAddress EAR: 0x10003cdb
 */
void __stdcall OpenEarMusicDevice(int cdAudio);

/**
 * Close the CD audio device and release the positions of its tracks.
 * @ghidraAddress EAR: 0x10003fe4
 */
void CloseCdAudioDevice(void);

/**
 * Close the MIDI sequencer when it is open.
 * @ghidraAddress EAR: 0x100040bb
 */
void CloseMidiMusicDevice(void);

/**
 * @return The number of tracks of the CD.
 * @ghidraAddress EAR: 0x10004832
 */
DWORD GetCdTrackCount(void);
/**
 * Read the positions of the tracks of the CD and find its first audio track.
 *
 * @return Whether an audio track is found or a previous nonzero first-track value remains.
 * Zero also indicates a missing device or failure to close an open door.
 * @ghidraAddress EAR: 0x10004b08
 */
int LoadCdTrackPositions(void);
/**
 * @return Whether the CD audio device has a CD.
 * @ghidraAddress EAR: 0x100063c3
 */
int IsCdAudioMediaPresent(void);
/**
 * Open the door of the CD audio device.
 *
 * @return Whether the command succeeded.
 * @ghidraAddress EAR: 0x10006443
 */
int OpenCdAudioDoor(void);
/**
 * Close the door of the CD audio device and wait for it.
 *
 * @return Whether the command succeeded.
 * @ghidraAddress EAR: 0x10006480
 */
int CloseCdAudioDoorAndWait(void);

#ifdef __cplusplus
}
#endif

#endif
