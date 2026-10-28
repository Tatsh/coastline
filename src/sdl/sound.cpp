// The routines of src/win32/sound.cpp the rest of the game calls, for a game without sound. Each
// reports the success of the original, and the game takes the paths of a working device.
// ponytail: silent until src/win32/sound.cpp compiles over SDL audio.

#include "win32/sound.h"

#include "soundchannel.h"

const int g_adwBgmVolumeLevels[VOLUME_LEVEL_COUNT] = {
    0x00, 0x1c, 0x38, 0x55, 0x71, 0x8d, 0xaa, 0xc6, 0xe2, 0xff};
const int g_adwSfxVolumeLevels[VOLUME_LEVEL_COUNT] = {
    0x00, 0x55, 0x78, 0x93, 0xaa, 0xbe, 0xd0, 0xe1, 0xf0, 0xff};
SoundChannelState g_aSoundChannelRequested[SOUND_CHANNEL_COUNT];
int g_bAltLoopingSoundSet;

void ApplyAudioVolumeSettings(void) {
}

bool DispatchMusicVolumeChange(unsigned char bVolume) {
    return true;
}

bool InitializeDirectSound(void) {
    return true;
}

bool InitSoundChannel(s16 channel, s32 sample, u32 length, u32 loopLength) {
    return true;
}

bool LoadSfxWaveVariants(int nEffect) {
    return true;
}

bool LoadSoundSet(int nSet) {
    return true;
}

bool MuteAllActiveSoundChannels(void) {
    return true;
}

bool PauseMusicPlayback(void) {
    return true;
}

bool PlayLoopedSoundVoiceWithStereo(int nSample, int nVoice, DWORD dwLevels) {
    return true;
}

bool PlaySoundChannelOneOnce(int nSample, DWORD dwLevels) {
    return true;
}

int RandomizeMusicTrack(void) {
    return 0;
}

bool ResetAudioSubsystem(void) {
    return true;
}

void ResetSoundBankTables(void) {
}

bool ResumeLoopingSoundChannels(void) {
    return true;
}

bool RouteMusicInitByFlag(HWND hWnd) {
    return true;
}

bool SelectNextMusicTrack(void) {
    return true;
}

bool SelectPreviousMusicTrack(void) {
    return true;
}

bool SetCurrentMusicTrack(int nModeIndex) {
    return true;
}

void SetSoundBankEntryIndex(int nVoice, int nIndex) {
}

bool SetSoundVoiceFrequencyFloat(int nSample, int nVoice, float fFrequency) {
    return true;
}

bool SetSoundVoiceStereo(int nSample, int nVoice, DWORD dwLevels) {
    return true;
}

bool StartSoundChannelMuted(s16 channel, s32 sample, u32 length, u32 loopLength) {
    return true;
}

bool StopAudioChannel(int nChannel) {
    return true;
}

bool StopSoundVoice(int nSample, int nVoice) {
    return true;
}

bool UnpauseMusicPlayback(void) {
    return true;
}
