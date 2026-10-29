#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

#include <io.h>

#include "ear/error.h"
#include "ear/mfc.h"
#include "ear/registry.h"
#include "musicdevice.h"
#include "playbackimplementations.h"
#include "window.h"

namespace {
// @ghidraAddress EAR: 0x10037158
const char g_earMidiTemporaryFileExtension[] = ".tmp";
// @ghidraAddress EAR: 0x10037160
char g_szMidiPlaybackDeviceType[] = "sequencer";
// @ghidraAddress EAR: 0x1002e9b8
const double g_earCdPlaybackTimeUnitScale = 10000.0;

/** The size of the path of a temporary copy of a MIDI file. */
constexpr std::size_t MIDI_TEMPORARY_PATH_SIZE = 0x104;

/** The notification of a playback that started, and of one that ended. */
constexpr LPARAM PLAYBACK_STARTED = 1;
constexpr LPARAM PLAYBACK_ENDED = 2;

static inline std::uint32_t TruncateToInteger(long double value) {
    // Invalid x87 integer conversions produce zero in the low 32 bits.
    return value >= -0x1p63L && value < 0x1p63L ?
               static_cast<std::uint32_t>(static_cast<std::int64_t>(value)) :
               0;
}
} // namespace

void MarkMidiSequencerActive(void) {
    g_bMidiSequencerActive = 1;
}

MidiPlayback::MidiPlayback() {
    playParameters.dwCallback = 0;
    playParameters.dwFrom = 0;
    playParameters.dwTo = 0;
    lengthMilliseconds = 0;
    path = nullptr;
    ownsTemporaryFile = 0;
}

void MidiPlayback::ReleaseEarPlaybackResources() {
    CloseRegisteredEarPlayback();
    if (ownsTemporaryFile && path) {
        _unlink(path);
        char *temporaryPath = path;
        ::operator delete(temporaryPath);
        path = nullptr;
    }
}

void MidiPlayback::CloseRegisteredEarPlayback() {
    if (!playbackHandle) {
        return;
    }
    SetEarPlaybackDirectionalGain(0, 0.0, 0);
    MarkMidiSequencerActive();
    if (notificationFlags & 2) {
        PostEarWindowMessage(notificationEvent, PLAYBACK_ENDED);
    }
    g_PlaybackRegistry.RemovePlaybackRegistryKey(playbackHandle);
    playbackHandle = 0;
}

int MidiPlayback::IsBufferedEarPlaybackComplete() {
    // @ghidraAddress EAR: 0x1003b818
    static int queryInProgress;
    if (completionRequested) {
        return 1;
    }
    if (!active) {
        return 0;
    }
    if (queryInProgress) {
        return 0;
    }
    queryInProgress = 1;
    // @ghidraAddress EAR: 0x1003b820
    static MCI_STATUS_PARMS modeStatus = {
        reinterpret_cast<DWORD_PTR>(g_hEarNotifyWindow), 0, MCI_STATUS_MODE, 1};
    mciSendCommandA(g_dwMidiSequencerDeviceId,
                    MCI_STATUS,
                    MCI_STATUS_ITEM,
                    reinterpret_cast<DWORD_PTR>(&modeStatus));
    completionRequested = modeStatus.dwReturn == MCI_MODE_STOP;
    if (completionRequested && looping) {
        completionRequested = 0;
        mciSendCommandA(g_dwMidiSequencerDeviceId, MCI_SEEK, MCI_SEEK_TO_START, 0);
        startPositionSpecified = 0;
        active = 0;
        PostEarWindowMessage(notificationEvent, PLAYBACK_ENDED);
    }
    queryInProgress = 0;
    return completionRequested;
}

void MidiPlayback::SetEarPlaybackMute(int isMuted) {
    // @ghidraAddress EAR: 0x1003b804
    static LONG muteValue;
    // @ghidraAddress EAR: 0x10037128
    static MIXERCONTROLDETAILS muteDetails = {sizeof(MIXERCONTROLDETAILS),
                                              g_MidiMuteMixerControl.dwControlID,
                                              1,
                                              {0},
                                              sizeof(muteValue),
                                              &muteValue};
    muted = isMuted;
    muteValue = muted;
    mixerSetControlDetails(
        reinterpret_cast<HMIXEROBJ>(g_hMidiMixer), &muteDetails, MIXER_SETCONTROLDETAILSF_VALUE);
}

int MidiPlayback::PauseEarPlayback() {
    status = 1;
    if (active && mciSendCommandA(g_dwMidiSequencerDeviceId, MCI_PAUSE, 0, 0)) {
        status = 0;
    }
    return status;
}

int MidiPlayback::StartEarPlayback() {
    MCI_SEEK_PARMS seek;
    char error[16];
    MCIERROR result;
    if (startPositionSpecified) {
        // Only the position of the seek is set.
        seek.dwTo = TruncateToInteger(static_cast<long double>(lengthMilliseconds) * startPosition);
        mciSendCommandA(
            g_dwMidiSequencerDeviceId, MCI_SEEK, MCI_TO, reinterpret_cast<DWORD_PTR>(&seek));
    }
    if (notificationFlags & 1) {
        PostEarWindowMessage(notificationEvent, PLAYBACK_STARTED);
    }
    result = mciSendCommandA(
        g_dwMidiSequencerDeviceId, MCI_PLAY, 0, reinterpret_cast<DWORD_PTR>(&playParameters));
    if (!result) {
        status = 0;
        active = 1;
        SetEarPlaybackDirectionalGain(0, pendingGain, 0);
        return 1;
    }
    mciSendCommandA(g_dwMidiSequencerDeviceId, MCI_CLOSE, 0, 0);
    // The text of the error is formatted and not used.
    _itoa(static_cast<int>(result), error, 10);
    return 0;
}

int MidiPlayback::InitializeEarPlayback(const void *source, int isStreaming, int isLooping) {
    MCI_OPEN_PARMSA open;
    MCI_STATUS_PARMS length;
    // A handle no registered playback has.
    playbackHandle = 0xfffffffe;
    looping = isLooping;
    streaming = 0;
    suppressionEnabled = 0;
    suppressionMarker = 0;
    directionIndex = 4;
    matrixId = 1;
    frequencyScale = 1.0;
    baseFrequencyScale = 1.0;
    if ((flags & 0x6000) == 0x2000) {
        // A file in memory is written to a temporary file for the sequencer to open.
        path = static_cast<char *>(::operator new(MIDI_TEMPORARY_PATH_SIZE));
        GetTempPathA(MIDI_TEMPORARY_PATH_SIZE, path);
        _ultoa(timeGetTime(), path + std::strlen(path), 8);
        std::strcat(path, g_earMidiTemporaryFileExtension);
        CFile file;
        file.Open(path, CFile::modeCreate | CFile::typeBinary | CFile::modeWrite);
        // The size of the file in memory is given as the reserved event control.
        file.Write(source, reservedEventControl);
        file.Close();
        ownsTemporaryFile = 1;
    } else {
        path = static_cast<char *>(const_cast<void *>(source));
    }
    active = 1;
    SetEarPlaybackDirectionalGain(0, 0.0, 0);
    active = 0;
    mciSendCommandA(g_dwMidiSequencerDeviceId, MCI_STOP, MCI_WAIT, 0);
    mciSendCommandA(g_dwMidiSequencerDeviceId, MCI_CLOSE, MCI_WAIT, 0);
    // Only the type and the file of the device are given.
    open.lpstrDeviceType = g_szMidiPlaybackDeviceType;
    open.lpstrElementName = path;
    if (mciSendCommandA(
            0, MCI_OPEN, MCI_OPEN_TYPE | MCI_OPEN_ELEMENT, reinterpret_cast<DWORD_PTR>(&open))) {
        return 0;
    }
    g_dwMidiSequencerDeviceId = open.wDeviceID;
    mciSendCommandA(g_dwMidiSequencerDeviceId, MCI_SEEK, MCI_SEEK_TO_END, 0);
    length.dwItem = MCI_STATUS_LENGTH;
    mciSendCommandA(g_dwMidiSequencerDeviceId,
                    MCI_STATUS,
                    MCI_STATUS_ITEM,
                    reinterpret_cast<DWORD_PTR>(&length));
    lengthMilliseconds = static_cast<DWORD>(length.dwReturn);
    mciSendCommandA(g_dwMidiSequencerDeviceId, MCI_SEEK, MCI_SEEK_TO_START, 0);
    return 1;
}

void MidiPlayback::SetEarPlaybackDirectionalGain(std::uint16_t direction,
                                                 double gain,
                                                 std::uint16_t matrix) {
    if (!active) {
        pendingGain = gain;
        return;
    }
    // @ghidraAddress EAR: 0x1003b80c
    static DWORD volumeValue;
    // @ghidraAddress EAR: 0x10037140
    static MIXERCONTROLDETAILS volumeDetails = {sizeof(MIXERCONTROLDETAILS),
                                                g_MidiVolumeMixerControl.dwControlID,
                                                1,
                                                {0},
                                                sizeof(volumeValue),
                                                &volumeValue};
    volumeValue = TruncateToInteger(static_cast<long double>(g_dwMidiVolumeMaximum) * gain);
    mixerSetControlDetails(
        reinterpret_cast<HMIXEROBJ>(g_hMidiMixer), &volumeDetails, MIXER_SETCONTROLDETAILSF_VALUE);
}

int MidiPlayback::ResumeEarPlayback() {
    if (status && active &&
        !mciSendCommandA(g_dwMidiSequencerDeviceId,
                         MCI_PLAY,
                         0,
                         reinterpret_cast<DWORD_PTR>(&resumeParameters))) {
        status = 0;
    }
    return status == 0;
}

void MidiPlayback::UpdateDirectSoundPlaybackGainAndPan() {
}

void MidiPlayback::UpdateDirectSoundPlaybackFrequency() {
}

CdAudioPlayback::CdAudioPlayback() {
    wholeDisc = 0;
    durationMilliseconds = 0;
}

void CdAudioPlayback::ReleaseEarPlaybackResources() {
    CloseRegisteredEarPlayback();
}

void CdAudioPlayback::CloseRegisteredEarPlayback() {
    if (!playbackHandle) {
        return;
    }
    mciSendCommandA(g_wCdAudioDeviceId, MCI_STOP, 0, 0);
    if (notificationFlags & 2) {
        PostEarWindowMessage(notificationEvent, PLAYBACK_ENDED);
    }
    SetEarPlaybackMute(0);
    SetEarPlaybackDirectionalGain(0, 1.0, 0);
    g_PlaybackRegistry.RemovePlaybackRegistryKey(playbackHandle);
    playbackHandle = 0;
}

int CdAudioPlayback::IsBufferedEarPlaybackComplete() {
    if (completionRequested) {
        return 1;
    }
    if (!active) {
        return 0;
    }
    // @ghidraAddress EAR: 0x1003b848
    static MCI_STATUS_PARMS modeStatus = {
        reinterpret_cast<DWORD_PTR>(g_hEarNotifyWindow), 0, MCI_STATUS_MODE, 1};
    mciSendCommandA(
        g_wCdAudioDeviceId, MCI_STATUS, MCI_STATUS_ITEM, reinterpret_cast<DWORD_PTR>(&modeStatus));
    completionRequested = modeStatus.dwReturn == MCI_MODE_STOP;
    if (completionRequested && looping) {
        resumeParameters.dwFrom = playParameters.dwFrom;
        resumeParameters.dwTo = playParameters.dwTo;
        completionRequested = !StartEarPlayback();
    }
    return completionRequested;
}

void CdAudioPlayback::SetEarPlaybackMute(int isMuted) {
    // @ghidraAddress EAR: 0x1003b838
    static LONG muteValue;
    // @ghidraAddress EAR: 0x10037180
    static MIXERCONTROLDETAILS muteDetails = {sizeof(MIXERCONTROLDETAILS),
                                              g_CdMuteMixerControl.dwControlID,
                                              1,
                                              {0},
                                              sizeof(muteValue),
                                              &muteValue};
    muted = isMuted;
    muteValue = muted;
    mixerSetControlDetails(
        reinterpret_cast<HMIXEROBJ>(g_hCdAudioMixer), &muteDetails, MIXER_SETCONTROLDETAILSF_VALUE);
}

int CdAudioPlayback::PauseEarPlayback() {
    status = 1;
    if (mciSendCommandA(g_wCdAudioDeviceId, MCI_PAUSE, 0, 0)) {
        status = 0;
    } else {
        // @ghidraAddress EAR: 0x1003b858
        static MCI_STATUS_PARMS positionStatus = {
            reinterpret_cast<DWORD_PTR>(g_hEarNotifyWindow), 0, MCI_STATUS_POSITION, 1};
        mciSendCommandA(g_wCdAudioDeviceId,
                        MCI_STATUS,
                        MCI_STATUS_ITEM,
                        reinterpret_cast<DWORD_PTR>(&positionStatus));
        resumeParameters.dwFrom = static_cast<DWORD>(positionStatus.dwReturn);
    }
    return status;
}

int CdAudioPlayback::StartEarPlayback() {
    DWORD originalFrom;
    DWORD originalTo;
    std::uint16_t track;
    if (notificationFlags & 1) {
        PostEarWindowMessage(notificationEvent, PLAYBACK_STARTED);
    }
    originalFrom = playParameters.dwFrom;
    // The start and the end are fractions of the tracks, or the numbers of tracks of the disc.
    if (startPositionSpecified) {
        if (wholeDisc) {
            track = static_cast<std::uint16_t>(TruncateToInteger(
                static_cast<long double>(startPosition) * g_earCdPlaybackTimeUnitScale));
            if (track >= g_wFirstCdAudioTrack && track <= g_wLastCdAudioTrack) {
                playParameters.dwFrom = GetCdTrackStartPosition(track);
            }
        } else {
            playParameters.dwFrom += TruncateToInteger(
                static_cast<long double>(playParameters.dwTo - playParameters.dwFrom) *
                startPosition);
        }
    }
    originalTo = playParameters.dwTo;
    if (backendOverrideEnabled) {
        if (wholeDisc) {
            track = static_cast<std::uint16_t>(TruncateToInteger(
                static_cast<long double>(frequencyScale) * g_earCdPlaybackTimeUnitScale));
            if (frequencyScale >= startPosition && track <= g_wLastCdAudioTrack) {
                playParameters.dwTo = GetCdTrackStartPosition(track);
            }
        } else {
            playParameters.dwTo -= TruncateToInteger(
                static_cast<long double>(playParameters.dwTo - playParameters.dwFrom) *
                frequencyScale);
        }
    }
    if (mciSendCommandA(g_wCdAudioDeviceId,
                        MCI_PLAY,
                        MCI_FROM | MCI_TO,
                        reinterpret_cast<DWORD_PTR>(&playParameters))) {
        return 0;
    }
    active = 1;
    status = 0;
    resumeParameters.dwFrom = playParameters.dwFrom;
    resumeParameters.dwTo = playParameters.dwTo;
    if (startPositionSpecified && !wholeDisc) {
        playParameters.dwFrom = originalFrom;
        startPositionSpecified = 0;
    }
    if (backendOverrideEnabled && !wholeDisc) {
        playParameters.dwTo = originalTo;
        backendOverrideEnabled = 0;
    }
    return 1;
}

int CdAudioPlayback::InitializeEarPlayback(const void *source, int isStreaming, int isLooping) {
    // The source is the number of the track, or 0 for every audio track of the disc.
    DWORD sourceTrack = static_cast<DWORD>(reinterpret_cast<std::uintptr_t>(source));
    std::uint16_t firstTrack;
    std::uint16_t lastTrack;
    if (sourceTrack > g_wLastCdAudioTrack) {
        SetEarLastError(0x0d);
        return 0;
    }
    playParameters.dwCallback = reinterpret_cast<DWORD_PTR>(g_hEarNotifyWindow);
    resumeParameters.dwCallback = reinterpret_cast<DWORD_PTR>(g_hEarNotifyWindow);
    firstTrack = static_cast<std::uint16_t>(sourceTrack);
    lastTrack = static_cast<std::uint16_t>(sourceTrack);
    if (!sourceTrack) {
        firstTrack = g_wFirstCdAudioTrack;
        lastTrack = g_wLastCdAudioTrack;
        wholeDisc = 1;
    }
    playParameters.dwFrom = GetCdTrackStartPosition(firstTrack);
    playParameters.dwTo = GetCdTrackEndPosition(lastTrack);
    durationMilliseconds = playParameters.dwTo - playParameters.dwFrom;
    // A handle no registered playback has.
    playbackHandle = 0xffffffff;
    looping = isLooping;
    streaming = isStreaming;
    suppressionEnabled = 0;
    suppressionMarker = 0;
    directionIndex = 4;
    matrixId = 1;
    frequencyScale = 1.0;
    baseFrequencyScale = 1.0;
    return 1;
}

void CdAudioPlayback::SetEarPlaybackDirectionalGain(std::uint16_t direction,
                                                    double gain,
                                                    std::uint16_t matrix) {
    // @ghidraAddress EAR: 0x1003b840
    static DWORD volumeValue;
    // @ghidraAddress EAR: 0x10037198
    static MIXERCONTROLDETAILS volumeDetails = {sizeof(MIXERCONTROLDETAILS),
                                                g_CdVolumeMixerControl.dwControlID,
                                                1,
                                                {0},
                                                sizeof(volumeValue),
                                                &volumeValue};
    volumeValue = TruncateToInteger(static_cast<long double>(g_dwCdVolumeMaximum) * gain);
    mixerSetControlDetails(reinterpret_cast<HMIXEROBJ>(g_hCdAudioMixer),
                           &volumeDetails,
                           MIXER_SETCONTROLDETAILSF_VALUE);
}

int CdAudioPlayback::ResumeEarPlayback() {
    if (status && active &&
        !mciSendCommandA(g_wCdAudioDeviceId,
                         MCI_PLAY,
                         MCI_FROM | MCI_TO,
                         reinterpret_cast<DWORD_PTR>(&resumeParameters))) {
        status = 0;
    }
    return status == 0;
}

std::uint32_t CdAudioPlayback::GetEarPlaybackMediaLength() {
    return durationMilliseconds;
}

void CdAudioPlayback::UpdateDirectSoundPlaybackGainAndPan() {
}

void CdAudioPlayback::UpdateDirectSoundPlaybackFrequency() {
}
