#pragma once

/**
 * @file
 * The music, the volumes, and the channels of the sound effects of the Boss Rally release, which
 * the race controls.
 */

#include <stdbool.h>
#include <stdint.h>

#include "common/soundsample.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The number of levels of the volumes of the configuration. */
#define VOLUME_LEVEL_COUNT 10

/**
 * The volume of the music of each level of the configuration, from 0 to 255.
 *
 * @ghidraAddress BRD3D: 0x100adf68
 * @ghidraAddress BRGlide: 0x100ad770
 */
extern const int g_adwBgmVolumeLevels[VOLUME_LEVEL_COUNT];

/**
 * The volume of the sound effects of each level of the configuration, from 0 to 255.
 *
 * @ghidraAddress BRD3D: 0x100adf90
 * @ghidraAddress BRGlide: 0x100ad798
 */
extern const int g_adwSfxVolumeLevels[VOLUME_LEVEL_COUNT];

/**
 * The volume of the music, from 0 to 255.
 *
 * @ghidraAddress BRD3D: 0x100bbad8
 * @ghidraAddress BRGlide: 0x100bb2e0
 */
extern unsigned char g_bBgmVolume;

/**
 * The volume of the sound effects, from 0 to 255.
 *
 * @ghidraAddress BRD3D: 0x100bbae0
 * @ghidraAddress BRGlide: 0x100bb2e8
 */
extern unsigned char g_bSfxVolume;

/** The settings of the buffer of a channel, as requested or as last applied. */
struct SoundChannelState {
    int nSample;             /*!< The sample of the channel. */
    char abUnknown04[4];     /* +0x04 */
    long long llFrequency;   /*!< The frequency, before the scale of the sample. */
    uint32_t dwReserved;     /*!< Cleared with the other settings and never read. */
    uint32_t dwStereoLevels; /*!< The levels of the two sides, as SetSoundBufferStereoLevels(). */
};

/**
 * The settings requested for the buffer of each channel.
 *
 * @ghidraAddress BRD3D: 0x118ac720
 * @ghidraAddress BRGlide: 0x118eef40
 */
extern struct SoundChannelState g_aSoundChannelRequested[SOUND_CHANNEL_COUNT];

/**
 * Whether the channels that play sample 0 move to sample 25, rather than back.
 *
 * @ghidraAddress BRD3D: 0x118290f4
 * @ghidraAddress BRGlide: 0x1184c454
 */
extern int g_bAltLoopingSoundSet;

/**
 * Play a track of music, through the CD audio in music mode 1 and the EAR library otherwise.
 *
 * @param nModeIndex The track.
 * @return Always true.
 * @ghidraAddress BRD3D: 0x100027c0
 * @ghidraAddress BRGlide: 0x10002af0
 */
bool SetCurrentMusicTrack(int nModeIndex);

/**
 * Give a volume of the music to the CD audio in music mode 1 and to the EAR library otherwise.
 *
 * @param bVolume The volume.
 * @return The result of HandleMusicPauseResume() or SetScaledAudioVolume().
 * @ghidraAddress BRD3D: 0x100029f0
 * @ghidraAddress BRGlide: 0x10002d30
 */
bool DispatchMusicVolumeChange(unsigned char bVolume);

/**
 * Choose a track of music at random from track 3 to g_nMusicTrackMax, other than the track that
 * plays when there are more than six tracks.
 *
 * @return The track.
 * @ghidraAddress BRD3D: 0x100028d0
 * @ghidraAddress BRGlide: 0x10002c00
 */
int RandomizeMusicTrack(void);

/**
 * Pause the music while the game is paused.
 *
 * @return Whether the music paused, and true when no music plays.
 * @ghidraAddress BRD3D: 0x10002b70
 * @ghidraAddress BRGlide: 0x10002eb0
 */
bool PauseMusicPlayback(void);

/**
 * Resume the music when the game is no longer paused.
 *
 * @return Whether the music resumed, and true when no music plays.
 * @ghidraAddress BRD3D: 0x10002bd0
 * @ghidraAddress BRGlide: 0x10002f10
 */
bool UnpauseMusicPlayback(void);

/**
 * Silence every channel of sound that has a buffer, and centre its pan, when DirectSound plays
 * sound effects.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10072e00
 * @ghidraAddress BRGlide: 0x1006bd70
 */
bool MuteAllActiveSoundChannels(void);

/**
 * Stop the buffer of a channel, when sound is on.
 *
 * @param nChannel The channel.
 * @return Whether the buffer stopped, and true without a buffer or without sound.
 * @ghidraAddress BRD3D: 0x10072580
 * @ghidraAddress BRGlide: 0x1006b4f0
 */
bool StopAudioChannel(int nChannel);

/**
 * Move each channel that plays the sample of the other set to the sample of the current set and
 * play it in a loop again, then record the sample requested for each channel and apply the
 * frequency and the levels requested for it. A channel whose requested frequency is not 0 and that
 * no longer plays has the frequency reset to 0.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10072e60
 * @ghidraAddress BRGlide: 0x1006bdd0
 */
bool ResumeLoopingSoundChannels(void);

/**
 * Stop and free the list of sound buffers and clear the buffers of the channels of every loaded
 * sample and of the channels that play, when DirectSound plays sound effects.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x100734f0
 * @ghidraAddress BRGlide: 0x1006c460
 */
bool ResetAudioSubsystem(void);

#ifdef __cplusplus
}
#endif
