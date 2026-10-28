#ifndef COASTLINE_EAR_MUSICDEVICE_H
#define COASTLINE_EAR_MUSICDEVICE_H

#include <stdint.h>

#include <windows.h>
// Multimedia declarations require the core Windows SDK types first.
#include <mmsystem.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The first and the last position of a track of the CD, in milliseconds. */
typedef struct EarCdTrackPosition {
    DWORD start; /*!< The position of the first sample. */
    DWORD end;   /*!< The position of the last sample. */
} EarCdTrackPosition;

/** @ghidraAddress EAR: 0x1003b7bc */
extern int32_t g_bCdAudioInitialized;
/** @ghidraAddress EAR: 0x1003b7c0 */
extern int32_t g_bMidiSequencerInitialized;
/** @ghidraAddress EAR: 0x1003b7c4 */
extern int32_t g_bCdAudioMixerInitialized;
/** @ghidraAddress EAR: 0x1003b7c8 */
extern int32_t g_bMidiMixerInitialized;
/** @ghidraAddress EAR: 0x1003b7cc */
extern int32_t g_bMidiSequencerActive;

/** @ghidraAddress EAR: 0x1003b7d4 */
extern int32_t g_bMidiVolumeControlAvailable;
/** @ghidraAddress EAR: 0x1003b7d8 */
extern int32_t g_bMidiMuteControlAvailable;
/** @ghidraAddress EAR: 0x1003b7dc */
extern int32_t g_bCdVolumeControlAvailable;
/** @ghidraAddress EAR: 0x1003b7e0 */
extern int32_t g_bCdMuteControlAvailable;

/** @ghidraAddress EAR: 0x1003b6f0 */
extern uint16_t g_wCdAudioDeviceId;
/** @ghidraAddress EAR: 0x1003b6f4 */
extern uint16_t g_wFirstCdAudioTrack;
/** @ghidraAddress EAR: 0x1003b6f8 */
extern uint16_t g_wLastCdAudioTrack;
/** @ghidraAddress EAR: 0x1003b6fc */
extern MCIDEVICEID g_dwMidiSequencerDeviceId;

/** @ghidraAddress EAR: 0x1003b790 */
extern DWORD g_dwCdTrackCount;
/**
 * The positions of the tracks, indexed by the number of the track from 1.
 *
 * @ghidraAddress EAR: 0x1003b794
 */
extern EarCdTrackPosition *g_pCdTrackPositions;

/** @ghidraAddress EAR: 0x1003b4c4 */
extern HMIXER g_hCdAudioMixer;
/** @ghidraAddress EAR: 0x1003b63c */
extern HMIXER g_hMidiMixer;

/** @ghidraAddress EAR: 0x1003b3c8 */
extern MIXERCONTROLA g_CdMuteMixerControl;
/** @ghidraAddress EAR: 0x1003b4e0 */
extern MIXERCONTROLA g_CdVolumeMixerControl;
/** @ghidraAddress EAR: 0x1003b580 */
extern MIXERCONTROLA g_MidiVolumeMixerControl;
/** @ghidraAddress EAR: 0x1003b648 */
extern MIXERCONTROLA g_MidiMuteMixerControl;

/** @ghidraAddress EAR: 0x1003b4c8 */
extern LONG g_bSavedMidiMute;
/** @ghidraAddress EAR: 0x1003b614 */
extern DWORD g_dwSavedCdVolume;
/** @ghidraAddress EAR: 0x1003b640 */
extern LONG g_bSavedCdMute;
/** @ghidraAddress EAR: 0x1003b6dc */
extern DWORD g_dwSavedMidiVolume;
/** @ghidraAddress EAR: 0x1003b6e8 */
extern DWORD g_dwCdVolumeMaximum;
/** @ghidraAddress EAR: 0x1003b6ec */
extern DWORD g_dwMidiVolumeMaximum;

/** @ghidraAddress EAR: 0x100360d8 */
extern UINT g_uConfiguredCdMixerId;
/** @ghidraAddress EAR: 0x100360dc */
extern UINT g_uConfiguredMidiMixerId;
/** @ghidraAddress EAR: 0x100360e0 */
extern DWORD g_dwConfiguredCdMixerLineId;
/** @ghidraAddress EAR: 0x100360e4 */
extern DWORD g_dwConfiguredMidiMixerLineId;

/**
 * Open the mixers and then the CD audio device or the MIDI sequencer.
 *
 * @param cdAudio Whether to open the CD audio device rather than the MIDI sequencer.
 * @ghidraAddress EAR: 0x10003cdb
 */
void __stdcall OpenEarMusicDevice(int cdAudio);

/**
 * Close the CD audio device and release the positions of its tracks.
 *
 * @ghidraAddress EAR: 0x10003fe4
 */
void CloseCdAudioDevice(void);

/**
 * Stop and close the MIDI sequencer.
 *
 * @ghidraAddress EAR: 0x1000406e
 */
void StopAndCloseMidiSequencer(void);

/**
 * Stop and close the MIDI sequencer when a playback is using it.
 *
 * @ghidraAddress EAR: 0x1000409e
 */
void CloseActiveMidiSequencer(void);

/**
 * Close the MIDI sequencer when it is open.
 *
 * @ghidraAddress EAR: 0x100040bb
 */
void CloseMidiMusicDevice(void);

/**
 * Give the volume and the mute controls of the mixers back their values and close the mixers.
 *
 * @ghidraAddress EAR: 0x100040e2
 */
void RestoreMusicMixerControls(void);

/**
 * Find the track that plays at a position.
 *
 * @param position The position, in milliseconds.
 * @return The number of the track, or 0 for none.
 * @ghidraAddress EAR: 0x1000477c
 */
uint16_t __stdcall FindCdTrackAtPosition(DWORD position);

/**
 * @param track The number of the track.
 * @return The position of the first sample of the track, or 0 past the last audio track.
 * @ghidraAddress EAR: 0x100047cf
 */
DWORD __stdcall GetCdTrackStartPosition(uint16_t track);

/**
 * @param track The number of the track.
 * @return The position of the last sample of the track, or 0 past the last audio track.
 * @ghidraAddress EAR: 0x10004800
 */
DWORD __stdcall GetCdTrackEndPosition(uint16_t track);

/**
 * @return The number of tracks of the CD.
 * @ghidraAddress EAR: 0x10004832
 */
DWORD GetCdTrackCount(void);

/**
 * Read the positions of the tracks of the CD and find its first audio track.
 *
 * @return Whether the CD has an audio track.
 * @ghidraAddress EAR: 0x10004b08
 */
int LoadCdTrackPositions(void);

/**
 * Open the mixer of the CD audio, sharing the MIDI mixer when it is the same device.
 *
 * @param mixer The mixer device.
 * @return Whether the mixer is open.
 * @ghidraAddress EAR: 0x10005e20
 */
int __stdcall OpenCdAudioMixer(UINT mixer);

/**
 * Close the mixer of the CD audio, unless the MIDI mixer shares it.
 *
 * @ghidraAddress EAR: 0x10005ea9
 */
void CloseCdAudioMixer(void);

/**
 * Open the mixer of the MIDI music, sharing the CD audio mixer when it is the same device.
 *
 * @param mixer The mixer device.
 * @return Whether the mixer is open.
 * @ghidraAddress EAR: 0x10005ef3
 */
int __stdcall OpenMidiMixer(UINT mixer);

/**
 * Close the mixer of the MIDI music, unless the CD audio mixer shares it.
 *
 * @ghidraAddress EAR: 0x10005f7c
 */
void CloseMidiMixer(void);

/**
 * Find the volume and the mute controls of a line of the MIDI mixer and save their values.
 *
 * @param line The line.
 * @return Whether both controls were found.
 * @ghidraAddress EAR: 0x10005fc6
 */
int __stdcall CacheMidiMixerControls(DWORD line);

/**
 * Find the volume and the mute controls of a line of the CD audio mixer and save their values.
 *
 * @param line The line.
 * @return Whether both controls were found.
 * @ghidraAddress EAR: 0x10006114
 */
int __stdcall CacheCdAudioMixerControls(DWORD line);

/**
 * Open the mixers the configuration selects and save the values of their controls.
 *
 * @return Whether every mixer opened.
 * @ghidraAddress EAR: 0x10006262
 */
int InitializeMusicMixers(void);

/**
 * Open the CD audio device, timed in milliseconds, and read its tracks.
 *
 * @return Whether the device opened with a CD that has an audio track.
 * @ghidraAddress EAR: 0x10006313
 */
int OpenCdAudioDevice(void);

/**
 * @return Whether the CD audio device has a CD.
 * @ghidraAddress EAR: 0x100063c3
 */
int IsCdAudioMediaPresent(void);

/**
 * @return The mode of the CD audio device, an MCI_MODE value.
 * @ghidraAddress EAR: 0x10006409
 */
DWORD GetCdAudioDeviceMode(void);

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

/**
 * @param track The number of the track.
 * @return Whether the track is an audio track of the CD.
 * @ghidraAddress EAR: 0x100064bd
 */
int __stdcall IsCdTrackInRange(DWORD track);

/**
 * Wait for an open CD audio device to close, closing its door when it has no CD and waiting up to a
 * second at a time.
 *
 * @return Whether the device is ready.
 * @ghidraAddress EAR: 0x100064f5
 */
int WaitForCdAudioReady(void);

/**
 * Open the MIDI sequencer.
 *
 * @return Whether the sequencer opened.
 * @ghidraAddress EAR: 0x10006560
 */
int OpenMidiSequencerDevice(void);

/**
 * Record that a playback is using the MIDI sequencer.
 *
 * @ghidraAddress EAR: 0x100094f0
 */
void MarkMidiSequencerActive(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_MUSICDEVICE_H */
