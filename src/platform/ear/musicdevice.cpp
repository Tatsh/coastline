#include "musicdevice.h"

#include <new>

#include "runtime.h"
#include "shutdown.h"
#include "window.h"

int32_t g_bCdAudioInitialized;
int32_t g_bMidiSequencerInitialized;
int32_t g_bCdAudioMixerInitialized;
int32_t g_bMidiMixerInitialized;
int32_t g_bMidiSequencerActive;
int32_t g_bMidiVolumeControlAvailable;
int32_t g_bMidiMuteControlAvailable;
int32_t g_bCdVolumeControlAvailable;
int32_t g_bCdMuteControlAvailable;
uint16_t g_wCdAudioDeviceId;
uint16_t g_wFirstCdAudioTrack;
uint16_t g_wLastCdAudioTrack;
MCIDEVICEID g_dwMidiSequencerDeviceId;
DWORD g_dwCdTrackCount;
EarCdTrackPosition *g_pCdTrackPositions;
HMIXER g_hCdAudioMixer;
HMIXER g_hMidiMixer;
MIXERCONTROLA g_CdMuteMixerControl;
MIXERCONTROLA g_CdVolumeMixerControl;
MIXERCONTROLA g_MidiVolumeMixerControl;
MIXERCONTROLA g_MidiMuteMixerControl;
LONG g_bSavedMidiMute;
DWORD g_dwSavedCdVolume;
LONG g_bSavedCdMute;
DWORD g_dwSavedMidiVolume;
DWORD g_dwCdVolumeMaximum;
DWORD g_dwMidiVolumeMaximum;
UINT g_uConfiguredCdMixerId = static_cast<UINT>(-1);
UINT g_uConfiguredMidiMixerId = static_cast<UINT>(-1);
DWORD g_dwConfiguredCdMixerLineId = static_cast<DWORD>(-1);
DWORD g_dwConfiguredMidiMixerLineId = static_cast<DWORD>(-1);

namespace {
// @ghidraAddress EAR: 0x10036828
char g_szCdAudioDeviceType[] = "cdaudio";
// @ghidraAddress EAR: 0x10036830
char g_szMidiSequencerDeviceType[] = "sequencer";

/** A handle no mixer has, which closed mixers take. */
HMIXER const NO_MIXER = reinterpret_cast<HMIXER>(static_cast<UINT_PTR>(-1));

/** The number of milliseconds a closed door may take to close. */
constexpr DWORD CD_DOOR_CLOSE_MILLISECONDS = 1000;

void SetMixerControlValue(HMIXER mixer, MIXERCONTROLDETAILS *details, DWORD control, DWORD *value) {
    details->dwControlID = control;
    details->cbDetails = sizeof(*value);
    details->paDetails = value;
    mixerSetControlDetails(
        reinterpret_cast<HMIXEROBJ>(mixer), details, MIXER_SETCONTROLDETAILSF_VALUE);
}
} // namespace

void __stdcall OpenEarMusicDevice(int cdAudio) {
    if (!g_bCdAudioMixerInitialized || !g_bMidiMixerInitialized) {
        InitializeMusicMixers();
    }
    if (cdAudio) {
        g_bCdAudioInitialized = OpenCdAudioDevice();
    } else {
        g_bMidiSequencerInitialized = OpenMidiSequencerDevice();
    }
}

void CloseCdAudioDevice(void) {
    if (g_wCdAudioDeviceId) {
        mciSendCommandA(g_wCdAudioDeviceId, MCI_STOP, MCI_WAIT, 0);
        // The device is closed with a notification, though no window is given for it.
        mciSendCommandA(g_wCdAudioDeviceId, MCI_CLOSE, MCI_NOTIFY, 0);
        g_wCdAudioDeviceId = 0;
    }
    if (g_pCdTrackPositions) {
        EarCdTrackPosition *positions = g_pCdTrackPositions;
        ::operator delete(positions);
        g_pCdTrackPositions = nullptr;
        g_dwCdTrackCount = 0;
    }
    g_bCdAudioInitialized = 0;
}

void StopAndCloseMidiSequencer(void) {
    mciSendCommandA(g_dwMidiSequencerDeviceId, MCI_STOP, MCI_WAIT, 0);
    mciSendCommandA(g_dwMidiSequencerDeviceId, MCI_CLOSE, MCI_WAIT, 0);
}

void CloseActiveMidiSequencer(void) {
    if (g_bMidiSequencerActive) {
        StopAndCloseMidiSequencer();
        g_bMidiSequencerActive = 0;
    }
}

void CloseMidiMusicDevice(void) {
    if (g_dwMidiSequencerDeviceId) {
        StopAndCloseMidiSequencer();
        g_dwMidiSequencerDeviceId = 0;
    }
    g_bMidiSequencerInitialized = 0;
}

void RestoreMusicMixerControls(void) {
    MIXERCONTROLDETAILS details;
    MIXERCONTROLDETAILS midiDetails;
    DWORD volume;
    DWORD mute;
    if (g_bMixerAccessDisabled) {
        return;
    }
    if (g_hCdAudioMixer != NO_MIXER) {
        details.cbStruct = sizeof(details);
        details.cChannels = 1;
        details.cMultipleItems = 0;
        if (g_bCdVolumeControlAvailable) {
            volume = g_dwSavedCdVolume;
            SetMixerControlValue(
                g_hCdAudioMixer, &details, g_CdVolumeMixerControl.dwControlID, &volume);
        }
        if (g_bCdMuteControlAvailable) {
            mute = static_cast<DWORD>(g_bSavedCdMute);
            SetMixerControlValue(
                g_hCdAudioMixer, &details, g_CdMuteMixerControl.dwControlID, &mute);
        }
        CloseCdAudioMixer();
    }
    if (g_hMidiMixer != NO_MIXER) {
        midiDetails.cbStruct = sizeof(midiDetails);
        midiDetails.cChannels = 1;
        midiDetails.cMultipleItems = 0;
        if (g_bMidiVolumeControlAvailable) {
            volume = g_dwSavedMidiVolume;
            SetMixerControlValue(
                g_hMidiMixer, &midiDetails, g_MidiVolumeMixerControl.dwControlID, &volume);
        }
        if (g_bMidiMuteControlAvailable) {
            mute = static_cast<DWORD>(g_bSavedMidiMute);
            SetMixerControlValue(
                g_hMidiMixer, &midiDetails, g_MidiMuteMixerControl.dwControlID, &mute);
        }
        CloseMidiMixer();
    }
}

uint16_t __stdcall FindCdTrackAtPosition(DWORD position) {
    for (DWORD track = 1; track <= g_dwCdTrackCount; track++) {
        // The last position of a track is not counted as in it.
        if (position >= g_pCdTrackPositions[track].start &&
            position < g_pCdTrackPositions[track].end) {
            return static_cast<uint16_t>(track);
        }
    }
    return 0;
}

DWORD __stdcall GetCdTrackStartPosition(uint16_t track) {
    if (track > g_wLastCdAudioTrack) {
        return 0;
    }
    return g_pCdTrackPositions[track].start;
}

DWORD __stdcall GetCdTrackEndPosition(uint16_t track) {
    if (track > g_wLastCdAudioTrack) {
        return 0;
    }
    return g_pCdTrackPositions[track].end;
}

DWORD GetCdTrackCount(void) {
    return g_dwCdTrackCount;
}

int LoadCdTrackPositions(void) {
    MCI_STATUS_PARMS status;
    uint16_t track;
    if (!g_wCdAudioDeviceId) {
        return 0;
    }
    if (!WaitForCdAudioReady()) {
        return 0;
    }
    status.dwCallback = reinterpret_cast<DWORD_PTR>(g_hEarNotifyWindow);
    status.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
    mciSendCommandA(
        g_wCdAudioDeviceId, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&status));
    g_dwCdTrackCount = static_cast<DWORD>(status.dwReturn);
    if (g_pCdTrackPositions) {
        EarCdTrackPosition *positions = g_pCdTrackPositions;
        ::operator delete(positions);
    }
    // One more entry than tracks, for the numbering of the tracks from 1.
    g_pCdTrackPositions = static_cast<EarCdTrackPosition *>(
        ::operator new(g_dwCdTrackCount * sizeof(EarCdTrackPosition) + sizeof(EarCdTrackPosition)));
    status.dwItem = MCI_STATUS_POSITION;
    for (track = 1; track <= g_dwCdTrackCount; track++) {
        status.dwTrack = track;
        mciSendCommandA(g_wCdAudioDeviceId,
                        MCI_STATUS,
                        MCI_STATUS_ITEM | MCI_TRACK,
                        reinterpret_cast<DWORD_PTR>(&status));
        g_pCdTrackPositions[track].start = static_cast<DWORD>(status.dwReturn);
    }
    status.dwItem = MCI_STATUS_LENGTH;
    for (track = 1; track <= g_dwCdTrackCount; track++) {
        status.dwTrack = track;
        mciSendCommandA(g_wCdAudioDeviceId,
                        MCI_STATUS,
                        MCI_STATUS_ITEM | MCI_TRACK,
                        reinterpret_cast<DWORD_PTR>(&status));
        g_pCdTrackPositions[track].end =
            static_cast<DWORD>(status.dwReturn) + g_pCdTrackPositions[track].start - 1;
    }
    // The first audio track found before is not forgotten, so the search then reads one track.
    track = 1;
    do {
        status.dwTrack = track;
        status.dwItem = MCI_CDA_STATUS_TYPE_TRACK;
        mciSendCommandA(g_wCdAudioDeviceId,
                        MCI_STATUS,
                        MCI_STATUS_ITEM | MCI_TRACK,
                        reinterpret_cast<DWORD_PTR>(&status));
        if (status.dwReturn == MCI_CDA_TRACK_AUDIO) {
            g_wFirstCdAudioTrack = track;
            g_wLastCdAudioTrack = static_cast<uint16_t>(g_dwCdTrackCount);
        }
        track++;
    } while (!g_wFirstCdAudioTrack && track <= g_dwCdTrackCount);
    if (!g_wFirstCdAudioTrack) {
        return 0;
    }
    return 1;
}

int __stdcall OpenCdAudioMixer(UINT mixer) {
    MMRESULT result;
    if (mixer == g_uConfiguredCdMixerId) {
        if (g_bCdAudioMixerInitialized) {
            return 1;
        }
    } else if (mixer == g_uConfiguredMidiMixerId && g_bMidiMixerInitialized) {
        g_hCdAudioMixer = g_hMidiMixer;
        g_bCdAudioMixerInitialized = 1;
        return 1;
    }
    result = mixerOpen(&g_hCdAudioMixer, mixer, 0, 0, 0);
    if (!result) {
        g_bCdAudioMixerInitialized = 1;
        return 1;
    }
    return 0;
}

void CloseCdAudioMixer(void) {
    if (g_hCdAudioMixer == NO_MIXER) {
        return;
    }
    if (g_hCdAudioMixer == g_hMidiMixer) {
        g_hCdAudioMixer = NO_MIXER;
        g_bCdAudioMixerInitialized = 0;
        return;
    }
    // A mixer of its own is closed without clearing the flag that it opened.
    mixerClose(g_hCdAudioMixer);
    g_hCdAudioMixer = NO_MIXER;
}

int __stdcall OpenMidiMixer(UINT mixer) {
    MMRESULT result;
    if (mixer == g_uConfiguredMidiMixerId) {
        if (g_bMidiMixerInitialized) {
            return 1;
        }
    } else if (mixer == g_uConfiguredCdMixerId && g_bCdAudioMixerInitialized) {
        g_hMidiMixer = g_hCdAudioMixer;
        g_bMidiMixerInitialized = 1;
        return 1;
    }
    result = mixerOpen(&g_hMidiMixer, mixer, 0, 0, 0);
    if (!result) {
        g_bMidiMixerInitialized = 1;
        return 1;
    }
    return 0;
}

void CloseMidiMixer(void) {
    if (g_hMidiMixer == NO_MIXER) {
        return;
    }
    if (g_hMidiMixer == g_hCdAudioMixer) {
        g_hMidiMixer = NO_MIXER;
        g_bMidiMixerInitialized = 0;
        return;
    }
    // A mixer of its own is closed without clearing the flag that it opened.
    mixerClose(g_hMidiMixer);
    g_hMidiMixer = NO_MIXER;
}

int __stdcall CacheMidiMixerControls(DWORD line) {
    MIXERLINECONTROLSA controls;
    MIXERCONTROLDETAILS volumeDetails;
    MIXERCONTROLDETAILS muteDetails;
    DWORD volume;
    LONG mute;
    MMRESULT result;
    g_bMidiVolumeControlAvailable = 0;
    g_bMidiMuteControlAvailable = 0;
    controls.cbStruct = sizeof(controls);
    controls.dwLineID = line;
    controls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    controls.cControls = 1;
    controls.cbmxctrl = sizeof(MIXERCONTROLA);
    controls.pamxctrl = &g_MidiVolumeMixerControl;
    result = mixerGetLineControlsA(
        reinterpret_cast<HMIXEROBJ>(g_hMidiMixer), &controls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (result) {
        return 0;
    }
    controls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
    controls.pamxctrl = &g_MidiMuteMixerControl;
    result = mixerGetLineControlsA(
        reinterpret_cast<HMIXEROBJ>(g_hMidiMixer), &controls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (result) {
        return 0;
    }
    volumeDetails.cbStruct = sizeof(volumeDetails);
    volumeDetails.dwControlID = g_MidiVolumeMixerControl.dwControlID;
    volumeDetails.cChannels = 1;
    volumeDetails.cMultipleItems = 0;
    volumeDetails.cbDetails = sizeof(volume);
    volumeDetails.paDetails = &volume;
    mixerGetControlDetailsA(
        reinterpret_cast<HMIXEROBJ>(g_hMidiMixer), &volumeDetails, MIXER_GETCONTROLDETAILSF_VALUE);
    g_dwSavedMidiVolume = volume;
    g_dwMidiVolumeMaximum = g_MidiVolumeMixerControl.Bounds.dwMaximum;
    g_bMidiVolumeControlAvailable = 1;
    muteDetails.cbStruct = sizeof(muteDetails);
    muteDetails.dwControlID = g_MidiMuteMixerControl.dwControlID;
    muteDetails.cChannels = 1;
    muteDetails.cMultipleItems = 0;
    muteDetails.cbDetails = sizeof(mute);
    muteDetails.paDetails = &mute;
    mixerGetControlDetailsA(
        reinterpret_cast<HMIXEROBJ>(g_hMidiMixer), &muteDetails, MIXER_GETCONTROLDETAILSF_VALUE);
    g_bSavedMidiMute = mute;
    g_bMidiMuteControlAvailable = 1;
    return 1;
}

int __stdcall CacheCdAudioMixerControls(DWORD line) {
    MIXERLINECONTROLSA controls;
    MIXERCONTROLDETAILS volumeDetails;
    MIXERCONTROLDETAILS muteDetails;
    DWORD volume;
    LONG mute;
    MMRESULT result;
    g_bCdVolumeControlAvailable = 0;
    g_bCdMuteControlAvailable = 0;
    controls.cbStruct = sizeof(controls);
    controls.dwLineID = line;
    controls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
    controls.cControls = 1;
    controls.cbmxctrl = sizeof(MIXERCONTROLA);
    controls.pamxctrl = &g_CdVolumeMixerControl;
    result = mixerGetLineControlsA(
        reinterpret_cast<HMIXEROBJ>(g_hCdAudioMixer), &controls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (result) {
        return 0;
    }
    controls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
    controls.pamxctrl = &g_CdMuteMixerControl;
    result = mixerGetLineControlsA(
        reinterpret_cast<HMIXEROBJ>(g_hCdAudioMixer), &controls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
    if (result) {
        return 0;
    }
    volumeDetails.cbStruct = sizeof(volumeDetails);
    volumeDetails.dwControlID = g_CdVolumeMixerControl.dwControlID;
    volumeDetails.cChannels = 1;
    volumeDetails.cMultipleItems = 0;
    volumeDetails.cbDetails = sizeof(volume);
    volumeDetails.paDetails = &volume;
    mixerGetControlDetailsA(reinterpret_cast<HMIXEROBJ>(g_hCdAudioMixer),
                            &volumeDetails,
                            MIXER_GETCONTROLDETAILSF_VALUE);
    g_dwSavedCdVolume = volume;
    g_bCdVolumeControlAvailable = 1;
    g_dwCdVolumeMaximum = g_CdVolumeMixerControl.Bounds.dwMaximum;
    muteDetails.cbStruct = sizeof(muteDetails);
    muteDetails.dwControlID = g_CdMuteMixerControl.dwControlID;
    muteDetails.cChannels = 1;
    muteDetails.cMultipleItems = 0;
    muteDetails.cbDetails = sizeof(mute);
    muteDetails.paDetails = &mute;
    mixerGetControlDetailsA(
        reinterpret_cast<HMIXEROBJ>(g_hCdAudioMixer), &muteDetails, MIXER_GETCONTROLDETAILSF_VALUE);
    g_bSavedCdMute = mute;
    g_bCdMuteControlAvailable = 1;
    return 1;
}

int InitializeMusicMixers(void) {
    int opened;
    if (g_bMixerAccessDisabled) {
        g_bCdAudioMixerInitialized = 1;
        g_bMidiMixerInitialized = 1;
        return 1;
    }
    opened = 1;
    if (!g_bCdAudioMixerInitialized) {
        g_hCdAudioMixer = NO_MIXER;
        if (OpenCdAudioMixer(g_uConfiguredCdMixerId)) {
            g_bCdAudioMixerInitialized = CacheCdAudioMixerControls(g_dwConfiguredCdMixerLineId);
        } else {
            opened = 0;
        }
    }
    if (!g_bMidiMixerInitialized) {
        g_hMidiMixer = NO_MIXER;
        if (OpenMidiMixer(g_uConfiguredMidiMixerId)) {
            g_bMidiMixerInitialized = CacheMidiMixerControls(g_dwConfiguredMidiMixerLineId);
        } else {
            opened = 0;
        }
    }
    return opened;
}

int OpenCdAudioDevice(void) {
    // Only the type of the device is given; the rest of the parameters are not set.
    MCI_OPEN_PARMSA open;
    MCI_SET_PARMS set;
    if (!g_bCdAudioMixerInitialized) {
        return 0;
    }
    open.lpstrDeviceType = g_szCdAudioDeviceType;
    if (mciSendCommandA(0, MCI_OPEN, MCI_OPEN_TYPE, reinterpret_cast<DWORD_PTR>(&open))) {
        return 0;
    }
    g_wCdAudioDeviceId = static_cast<uint16_t>(open.wDeviceID);
    set.dwCallback = reinterpret_cast<DWORD_PTR>(g_hEarNotifyWindow);
    set.dwTimeFormat = MCI_FORMAT_MILLISECONDS;
    set.dwAudio = 0;
    if (mciSendCommandA(
            g_wCdAudioDeviceId, MCI_SET, MCI_SET_TIME_FORMAT, reinterpret_cast<DWORD_PTR>(&set))) {
        return 0;
    }
    if (!IsCdAudioMediaPresent()) {
        CloseCdAudioDevice();
        return 0;
    }
    if (!LoadCdTrackPositions()) {
        CloseCdAudioDevice();
        return 0;
    }
    return 1;
}

int IsCdAudioMediaPresent(void) {
    // The result of the command is not checked, so a failure reads an unset value.
    MCI_STATUS_PARMS status;
    status.dwCallback = reinterpret_cast<DWORD_PTR>(g_hEarNotifyWindow);
    status.dwItem = MCI_STATUS_MEDIA_PRESENT;
    mciSendCommandA(
        g_wCdAudioDeviceId, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&status));
    if (!status.dwReturn) {
        return 0;
    }
    return 1;
}

DWORD GetCdAudioDeviceMode(void) {
    MCI_STATUS_PARMS status;
    status.dwCallback = reinterpret_cast<DWORD_PTR>(g_hEarNotifyWindow);
    status.dwItem = MCI_STATUS_MODE;
    mciSendCommandA(
        g_wCdAudioDeviceId, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&status));
    return static_cast<DWORD>(status.dwReturn);
}

int OpenCdAudioDoor(void) {
    MCI_SET_PARMS set;
    set.dwCallback = reinterpret_cast<DWORD_PTR>(g_hEarNotifyWindow);
    if (mciSendCommandA(
            g_wCdAudioDeviceId, MCI_SET, MCI_SET_DOOR_OPEN, reinterpret_cast<DWORD_PTR>(&set))) {
        return 0;
    }
    return 1;
}

int CloseCdAudioDoorAndWait(void) {
    MCI_SET_PARMS set;
    set.dwCallback = reinterpret_cast<DWORD_PTR>(g_hEarNotifyWindow);
    if (mciSendCommandA(g_wCdAudioDeviceId,
                        MCI_SET,
                        MCI_SET_DOOR_CLOSED | MCI_WAIT,
                        reinterpret_cast<DWORD_PTR>(&set))) {
        return 0;
    }
    return 1;
}

int __stdcall IsCdTrackInRange(DWORD track) {
    int inRange;
    if (track >= g_wFirstCdAudioTrack && track <= g_wLastCdAudioTrack) {
        inRange = 1;
    } else {
        inRange = 0;
    }
    return inRange;
}

int WaitForCdAudioReady(void) {
    DWORD start;
    if (GetCdAudioDeviceMode() != MCI_MODE_OPEN) {
        return 1;
    }
    if (!IsCdAudioMediaPresent() && !CloseCdAudioDoorAndWait()) {
        return 0;
    }
    start = timeGetTime();
    while (GetCdAudioDeviceMode() == MCI_MODE_OPEN &&
           timeGetTime() - start < CD_DOOR_CLOSE_MILLISECONDS) {
    }
    if (GetCdAudioDeviceMode() == MCI_MODE_OPEN) {
        return 0;
    }
    return WaitForCdAudioReady();
}

int OpenMidiSequencerDevice(void) {
    MCI_OPEN_PARMSA open;
    if (!g_bMidiMixerInitialized) {
        return 0;
    }
    open.lpstrDeviceType = g_szMidiSequencerDeviceType;
    if (mciSendCommandA(0, MCI_OPEN, MCI_OPEN_TYPE, reinterpret_cast<DWORD_PTR>(&open))) {
        return 0;
    }
    g_dwMidiSequencerDeviceId = open.wDeviceID;
    return 1;
}

void ShutdownMusicDevicesAndRestoreMixer(void) {
    CloseCdAudioDevice();
    CloseMidiMusicDevice();
    RestoreMusicMixerControls();
}
