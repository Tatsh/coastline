#ifndef COASTLINE_WIN32_EAR_MUSICDEVICE_H
#define COASTLINE_WIN32_EAR_MUSICDEVICE_H

#include <stdint.h>

#include <windows.h>
// Multimedia declarations require the core Windows SDK types first.
#include <mmsystem.h>

#include "ear/musicdevice.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Absolute CD track positions in the MCI millisecond time format. */
typedef struct EarCdTrackPosition {
    DWORD start; /*!< The start position in milliseconds from the start of the disc. */
    DWORD end;   /*!< The start plus duration minus one, in milliseconds. */
} EarCdTrackPosition;

/**
 * CD mixer initialisation marker, also set when mixer access is disabled.
 * An independently closed mixer does not clear the native marker.
 * @ghidraAddress EAR: 0x1003b7c4
 */
extern int32_t g_bCdAudioMixerInitialized;
/**
 * MIDI mixer initialisation marker, also set when mixer access is disabled.
 * An independently closed mixer does not clear the native marker.
 * @ghidraAddress EAR: 0x1003b7c8
 */
extern int32_t g_bMidiMixerInitialized;
/**
 * Nonzero after a playback registers use of the shared MIDI sequencer.
 * @ghidraAddress EAR: 0x1003b7cc
 */
extern int32_t g_bMidiSequencerActive;

/**
 * Nonzero when cached MIDI volume-control details and restoration values are available.
 * @ghidraAddress EAR: 0x1003b7d4
 */
extern int32_t g_bMidiVolumeControlAvailable;
/**
 * Nonzero when cached MIDI mute-control details and a restoration value are available.
 * @ghidraAddress EAR: 0x1003b7d8
 */
extern int32_t g_bMidiMuteControlAvailable;
/**
 * Nonzero when cached CD volume-control details and restoration values are available.
 * @ghidraAddress EAR: 0x1003b7dc
 */
extern int32_t g_bCdVolumeControlAvailable;
/**
 * Nonzero when cached CD mute-control details and a restoration value are available.
 * @ghidraAddress EAR: 0x1003b7e0
 */
extern int32_t g_bCdMuteControlAvailable;

/**
 * MCI CD device identifier, or zero when cleared after closing the device.
 * @ghidraAddress EAR: 0x1003b6f0
 */
extern uint16_t g_wCdAudioDeviceId;
/**
 * Shared MCI MIDI sequencer identifier, cleared by CloseMidiMusicDevice().
 * @ghidraAddress EAR: 0x1003b6fc
 */
extern MCIDEVICEID g_dwMidiSequencerDeviceId;

/**
 * Total CD track count, including data tracks, returned by MCI.
 * @ghidraAddress EAR: 0x1003b790
 */
extern DWORD g_dwCdTrackCount;
/**
 * Allocated track positions indexed from 1 through g_dwCdTrackCount; index zero is unused.
 * LoadCdTrackPositions() replaces the allocation and CloseCdAudioDevice() releases it.
 * Borrowed references into the table remain valid only until replacement or release.
 *
 * @ghidraAddress EAR: 0x1003b794
 */
extern EarCdTrackPosition *g_pCdTrackPositions;

/**
 * CD mixer handle, possibly shared with MIDI; closing assigns the pointer-sized -1 sentinel.
 * @ghidraAddress EAR: 0x1003b4c4
 */
extern HMIXER g_hCdAudioMixer;
/**
 * MIDI mixer handle, possibly shared with CD; closing assigns the pointer-sized -1 sentinel.
 * @ghidraAddress EAR: 0x1003b63c
 */
extern HMIXER g_hMidiMixer;

/**
 * Cached Windows CD mute-control descriptor, valid after successful control discovery.
 * @ghidraAddress EAR: 0x1003b3c8
 */
extern MIXERCONTROLA g_CdMuteMixerControl;
/**
 * Cached Windows CD volume-control descriptor, including device-specific bounds.
 * @ghidraAddress EAR: 0x1003b4e0
 */
extern MIXERCONTROLA g_CdVolumeMixerControl;
/**
 * Cached Windows MIDI volume-control descriptor, including device-specific bounds.
 * @ghidraAddress EAR: 0x1003b580
 */
extern MIXERCONTROLA g_MidiVolumeMixerControl;
/**
 * Cached Windows MIDI mute-control descriptor, valid after successful control discovery.
 * @ghidraAddress EAR: 0x1003b648
 */
extern MIXERCONTROLA g_MidiMuteMixerControl;

/**
 * MIDI mute value captured for restoration, with nonzero indicating mute.
 * @ghidraAddress EAR: 0x1003b4c8
 */
extern LONG g_bSavedMidiMute;
/**
 * CD volume captured for restoration in the mixer's device-specific integer range.
 * @ghidraAddress EAR: 0x1003b614
 */
extern DWORD g_dwSavedCdVolume;
/**
 * CD mute value captured for restoration, with nonzero indicating mute.
 * @ghidraAddress EAR: 0x1003b640
 */
extern LONG g_bSavedCdMute;
/**
 * MIDI volume captured for restoration in the mixer's device-specific integer range.
 * @ghidraAddress EAR: 0x1003b6dc
 */
extern DWORD g_dwSavedMidiVolume;
/**
 * Maximum CD volume reported by the cached volume-control bounds.
 * @ghidraAddress EAR: 0x1003b6e8
 */
extern DWORD g_dwCdVolumeMaximum;
/**
 * Maximum MIDI volume reported by the cached volume-control bounds.
 * @ghidraAddress EAR: 0x1003b6ec
 */
extern DWORD g_dwMidiVolumeMaximum;

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
 * Give the volume and the mute controls of the mixers back their values and close the mixers.
 *
 * @ghidraAddress EAR: 0x100040e2
 */
void RestoreMusicMixerControls(void);

/**
 * Find the track at a position using the loaded table.
 * The native comparison excludes the stored final millisecond of each track.
 *
 * @param position The position, in milliseconds.
 * @return The number of the track, or 0 for none.
 * @ghidraAddress EAR: 0x1000477c
 */
uint16_t __stdcall FindCdTrackAtPosition(DWORD position);

/**
 * @param track The one-based track number in a loaded table. Zero is not a valid index.
 * @return The absolute track start in milliseconds, or zero past the last audio track.
 * @ghidraAddress EAR: 0x100047cf
 */
DWORD __stdcall GetCdTrackStartPosition(uint16_t track);

/**
 * @param track The one-based track number in a loaded table. Zero is not a valid index.
 * @return The absolute start plus duration minus one in milliseconds, or zero past the last
 * audio track.
 * @ghidraAddress EAR: 0x10004800
 */
DWORD __stdcall GetCdTrackEndPosition(uint16_t track);

/**
 * Open the mixer of the CD audio, sharing the MIDI mixer when it is the same device.
 *
 * @param mixer The mixer device.
 * @return Whether opening succeeds or an existing initialisation marker permits reuse.
 * A stale marker does not guarantee a valid mixer handle.
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
 * @return Whether opening succeeds or an existing initialisation marker permits reuse.
 * A stale marker does not guarantee a valid mixer handle.
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
 * @return Zero when either required mixer open fails; otherwise nonzero, including when mixer
 * access is disabled. Control-discovery failures do not change the return value.
 * @ghidraAddress EAR: 0x10006262
 */
int InitializeMusicMixers(void);

/**
 * Open the CD audio device, timed in milliseconds, and read its tracks.
 *
 * @return Whether the device opens, accepts millisecond timing, reports media, and loads tracks.
 * Track loading can accept a previously discovered first audio-track number.
 * @ghidraAddress EAR: 0x10006313
 */
int OpenCdAudioDevice(void);

/**
 * @return The mode of the CD audio device, an MCI_MODE value.
 * @ghidraAddress EAR: 0x10006409
 */
DWORD GetCdAudioDeviceMode(void);

/**
 * @param track The track number to compare with the cached first and last audio-track numbers.
 * @return Whether the number falls within the inclusive cached range, without querying track type.
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

#endif /* COASTLINE_WIN32_EAR_MUSICDEVICE_H */
