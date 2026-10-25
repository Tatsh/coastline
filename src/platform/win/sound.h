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
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10002260
 */
bool InitializeMusicPlayback(void);

/**
 * The event the music library signals at the end of a track.
 *
 * @return The event.
 * @ghidraAddress BRD3D: 0x10002480
 */
WPARAM GetEarMusicEventHandle(void);

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
 * Give a volume of the music to the CD audio in music mode 1 and to the EAR library otherwise.
 *
 * @param bVolume The volume.
 * @return The result of HandleMusicPauseResume() or SetScaledAudioVolume().
 * @ghidraAddress BRD3D: 0x100029f0
 * @ghidraAddress BRGlide: 0x10002d30
 */
bool DispatchMusicVolumeChange(unsigned char bVolume);

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
 * Choose a track of music at random from track 3 to g_nMusicTrackMax, other than the track that
 * plays when there are more than six tracks.
 *
 * @return The track.
 * @ghidraAddress BRD3D: 0x100028d0
 * @ghidraAddress BRGlide: 0x10002c00
 */
int RandomizeMusicTrack(void);

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
 * Play the track of music before the current one, staying at the first.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10002930
 * @ghidraAddress BRGlide: 0x10002c70
 */
bool SelectPreviousMusicTrack(void);

/**
 * Play the track of music after the current one, staying at the last.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x10002970
 * @ghidraAddress BRGlide: 0x10002cb0
 */
bool SelectNextMusicTrack(void);

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
 * Play the tracks of the CD audio from the current track on.
 *
 * @return Whether the tracks play, and true when no music plays.
 * @ghidraAddress BRD3D: 0x10002510
 * @ghidraAddress BRGlide: 0x10002830
 */
bool PlayCdAudioFromCurrentTrack(void);

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

/** A sound buffer of a sample, in a list. The layout is not recovered past the members below. */
struct SoundBufferNode {
    void *pSampleData;           /*!< The data of the sample, in locked global memory. */
    DWORD dwSampleSize;          /*!< The size of the sample in bytes. */
    WAVEFORMATEX *pWaveFormat;   /*!< The format of the sample, in locked global memory. */
    DWORD dwFrequency;           /*!< The frequency of the buffer, in samples a second. */
    int nPan;                    /*!< The pan, from 0 to 800 with the centre at 400. */
    int nVolume;                 /*!< The volume before the volume of the sound effects. */
    int bLooping;                /*!< Whether the buffer plays in a loop. */
    int bPlaying;                /*!< Whether the buffer plays. */
    char abUnknown20[4];         /* +0x20 */
    int bHardware;               /*!< Whether the buffer is in the memory of the sound card. */
    int bStickyFocus;            /*!< Whether the buffer plays while the window is not active. */
    char abUnknown2c[0x70];      /* +0x2c */
    IDirectSoundBuffer *pBuffer; /*!< The buffer of DirectSound. */
    int nNameOffset;             /*!< The offset in szPath of the name of the file, or 0. */
    char szPath[MAX_PATH];       /*!< The path of the file of the sample. */
    SoundBufferNode *pNext;      /*!< The next node of the list. */
};

/**
 * The head of the list of the sound buffers. Only its first member and its link are read here.
 *
 * @ghidraAddress BRD3D: 0x11828f48
 * @ghidraAddress BRGlide: 0x1184c2a8
 */
extern SoundBufferNode g_SoundBufferListHead;

/**
 * Create the buffer of DirectSound of a sample, static with control of the frequency, the pan,
 * the volume, and the position, and copy the sample into it. The volume is set full and the pan
 * centred, and bHardware records where the buffer is. A step that fails releases the buffer.
 *
 * @param pNode The node of the sample.
 * @return DS_OK, or the error of the step that failed.
 * @ghidraAddress BRD3D: 0x100722d0
 * @ghidraAddress BRGlide: 0x1006b240
 */
HRESULT CreateDirectSoundBufferFromSample(SoundBufferNode *pNode);

/**
 * Release the buffer of DirectSound of a node.
 *
 * @param pNode The node.
 * @return Always DS_OK.
 * @ghidraAddress BRD3D: 0x10072520
 * @ghidraAddress BRGlide: 0x1006b490
 */
HRESULT ReleaseSoundBuffer(SoundBufferNode *pNode);

/**
 * Stop the buffer of DirectSound of a node that plays.
 *
 * @param pNode The node.
 * @return The result of the stop, or DS_OK for a buffer that does not play.
 * @ghidraAddress BRD3D: 0x10072550
 * @ghidraAddress BRGlide: 0x1006b4c0
 */
HRESULT StopSoundBuffer(SoundBufferNode *pNode);

/** The number of entries of g_apSoundChannelBuffer. */
#define SOUND_CHANNEL_COUNT 15

/**
 * The sound buffers of the channels of the sound effects.
 *
 * @ghidraAddress BRD3D: 0x11828f08
 * @ghidraAddress BRGlide: 0x1184c268
 */
extern SoundBufferNode *g_apSoundChannelBuffer[SOUND_CHANNEL_COUNT];

/**
 * Report whether the buffer of a channel plays, when sound is on.
 *
 * @param nChannel The channel.
 * @return Whether it plays, false without a buffer, and true without sound.
 * @ghidraAddress BRD3D: 0x10072fe0
 * @ghidraAddress BRGlide: 0x1006bf50
 */
bool IsSoundChannelPlaying(int nChannel);

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
 * Set whether the buffer of a channel plays in a loop, when sound is on.
 *
 * @param nChannel The channel.
 * @param bLooping Whether it loops.
 * @return Whether it was set, false without a buffer, and true without sound.
 * @ghidraAddress BRD3D: 0x10072640
 * @ghidraAddress BRGlide: 0x1006b5b0
 */
bool SetAudioChannelLooping(int nChannel, int bLooping);

/**
 * A row of the banks of sounds: which channels of a sample of a set are loaded. The layout past
 * the channels is not recovered yet.
 */
struct SoundBankRow {
    int anChannels[SOUND_CHANNEL_COUNT]; /*!< Nonzero for each channel of the sample loaded. */
    int anUnknown3c[3];                  /* +0x3c */
};

/**
 * The banks of sounds, a row for each sample of a set. g_SoundBankIndexTableB and
 * g_SoundBankIndexTableC follow as rows 24 and 25. The number of rows is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x100b6540
 * @ghidraAddress BRGlide: 0x100b5d48
 */
extern SoundBankRow g_SoundBankIndexTableA[];

/**
 * The row of the banks of sounds after the 24 rows of g_SoundBankIndexTableA: for each channel of
 * the sound effects of a race, the index in g_apszSfxNamePrefixes of the file of its variant.
 *
 * @ghidraAddress BRD3D: 0x100b6c00
 * @ghidraAddress BRGlide: 0x100b6408
 */
extern SoundBankRow g_SoundBankIndexTableB;

/**
 * The row of the banks of sounds after g_SoundBankIndexTableB: for each channel of the sound
 * effects of a race, the index in g_apszSfxNamePrefixes of the file of its looping sound.
 *
 * @ghidraAddress BRD3D: 0x100b6c48
 * @ghidraAddress BRGlide: 0x100b6450
 */
extern SoundBankRow g_SoundBankIndexTableC;

/**
 * Record a sound for the even channel of a voice in the first row of the banks and in the rows of
 * the variants and the looping sounds, as one more than its index.
 *
 * @param nVoice The voice, half the channel.
 * @param nIndex The index of the sound.
 * @ghidraAddress BRD3D: 0x10073080
 * @ghidraAddress BRGlide: 0x1006bff0
 */
void SetSoundBankEntryIndex(int nVoice, int nIndex);

/**
 * Clear the first entries of the three tables of the banks of sounds.
 *
 * @ghidraAddress BRD3D: 0x10073060
 * @ghidraAddress BRGlide: 0x1006bfd0
 */
void ResetSoundBankTables(void);

/** A sound sample loaded into DirectSound. */
struct SoundSample {
    SoundBufferNode *apChannelBuffers[SOUND_CHANNEL_COUNT]; /*!< A buffer for each channel. */
    char abUnknown3c[4];                                    /* +0x3c */
    double dFrequencyScale; /*!< The scale of a frequency of a channel that plays the sample. */
};

/**
 * The sound samples loaded. The number of entries is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x100b5df0
 * @ghidraAddress BRGlide: 0x100b55f8
 */
extern SoundSample g_aSoundChannelBuffers[];

/**
 * The number of entries of g_aSoundChannelBuffers that are loaded.
 *
 * @ghidraAddress BRD3D: 0x11828f00
 * @ghidraAddress BRGlide: 0x1184c260
 */
extern int g_dwLoadedSoundSampleCount;

/** The sets of sounds of LoadSoundSet(). */
enum {
    SOUND_SET_FRONT_END, /*!< The sounds of the front end. */
    SOUND_SET_RACE,      /*!< The sounds of a race. */
};

/** The number of samples of the set of the front end. */
#define SOUND_SET_FRONT_END_COUNT 9
/** The number of samples of the set of a race. */
#define SOUND_SET_RACE_COUNT 25

/**
 * The files of the samples of the set of a race, under g_szSfxDirPath. The first is null.
 *
 * @ghidraAddress BRD3D: 0x100b8938
 * @ghidraAddress BRGlide: 0x100b8140
 */
extern const char *const g_SoundSetTableA[SOUND_SET_RACE_COUNT];

/**
 * The files of the samples of the set of the front end, under g_szSfxDirPath. The first is null.
 *
 * @ghidraAddress BRD3D: 0x100b89a0
 * @ghidraAddress BRGlide: 0x100b81a8
 */
extern const char *const g_SoundSetTableB[SOUND_SET_FRONT_END_COUNT];

/**
 * The variants of the waves of the sound effects of a race, one for each channel.
 *
 * @ghidraAddress BRD3D: 0x100b64b0
 * @ghidraAddress BRGlide: 0x100b5cb8
 */
extern SoundBufferNode *g_SfxWaveVariantTable[SOUND_CHANNEL_COUNT];

/**
 * The looping sounds of a race, one for each channel.
 *
 * @ghidraAddress BRD3D: 0x100b64f8
 * @ghidraAddress BRGlide: 0x100b5d00
 */
extern SoundBufferNode *g_LoopingSoundTable[SOUND_CHANNEL_COUNT];

/**
 * The prefixes of the names of the files of the sound effects of a race, under g_szSfxDirPath. The
 * first is null, and the number of entries is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x100b84f4
 * @ghidraAddress BRGlide: 0x100b7cfc
 */
extern const char *const g_apszSfxNamePrefixes[];

/**
 * Add a node at the end of a list of sound buffers, not playing.
 *
 * @param pHead The head of the list.
 * @param pNode The node.
 * @return Always 0.
 * @ghidraAddress BRD3D: 0x10072450
 * @ghidraAddress BRGlide: 0x1006b3c0
 */
int AppendSoundBufferToList(SoundBufferNode *pHead, SoundBufferNode *pNode);

/**
 * Set the pan of the buffer of a node from nPan.
 *
 * @param pNode The node.
 * @return The result of SetPan().
 * @ghidraAddress BRD3D: 0x10072490
 * @ghidraAddress BRGlide: 0x1006b400
 */
HRESULT ApplySoundBufferPan(SoundBufferNode *pNode);

/**
 * Set the frequency of the buffer of a node from dwFrequency.
 *
 * @param pNode The node.
 * @return The result of SetFrequency().
 * @ghidraAddress BRD3D: 0x100724b0
 * @ghidraAddress BRGlide: 0x1006b420
 */
HRESULT ApplySoundBufferFrequency(SoundBufferNode *pNode);

/**
 * Set the volume of the buffer of a node from nVolume scaled by g_bSfxVolume, or silence it when
 * the sound effects are off.
 *
 * @param pNode The node.
 * @return The result of SetVolume().
 * @ghidraAddress BRD3D: 0x100724d0
 * @ghidraAddress BRGlide: 0x1006b440
 */
HRESULT ApplySoundBufferVolume(SoundBufferNode *pNode);

/**
 * Play the buffer of a node from its start: a buffer that plays goes back to its start, and one
 * that does not starts, in a loop when bLooping is set, and is marked as playing.
 *
 * @param pNode The node.
 * @return The result of SetCurrentPosition() or Play().
 * @ghidraAddress BRD3D: 0x10072a00
 * @ghidraAddress BRGlide: 0x1006b970
 */
HRESULT RestartOrPlaySoundBuffer(SoundBufferNode *pNode);

/**
 * Set whether the buffer of a node loops and play it from its start.
 *
 * @param pNode The node.
 * @param bLooping Whether the buffer plays in a loop.
 * @return The result of RestartOrPlaySoundBuffer().
 * @ghidraAddress BRD3D: 0x100729e0
 * @ghidraAddress BRGlide: 0x1006b950
 */
HRESULT PlaySoundBuffer(SoundBufferNode *pNode, int bLooping);

/**
 * Set the frequency of the buffer of a node, when sound is on.
 *
 * @param pNode The node.
 * @param dwFrequency The frequency, in samples a second.
 * @return Whether the frequency was set, and true without sound.
 * @ghidraAddress BRD3D: 0x10072700
 * @ghidraAddress BRGlide: 0x1006b670
 */
bool SetSoundBufferFrequency(SoundBufferNode *pNode, DWORD dwFrequency);

/**
 * Report whether the buffer of a node plays.
 *
 * @param pNode The node.
 * @return Whether it plays, and false when its status cannot be read.
 * @ghidraAddress BRD3D: 0x10073020
 * @ghidraAddress BRGlide: 0x1006bf90
 */
bool IsSoundBufferPlaying(SoundBufferNode *pNode);

/**
 * Set the frequency of a channel of a sample, when sound is on.
 *
 * @param nSample The sample.
 * @param nChannel The channel.
 * @param dwFrequency The frequency, in samples a second.
 * @return Whether the frequency was set, and true without sound.
 * @ghidraAddress BRD3D: 0x10072770
 * @ghidraAddress BRGlide: 0x1006b6e0
 */
bool SetSoundChannelFrequency(int nSample, int nChannel, DWORD dwFrequency);

/**
 * Set the frequency of the even channel of a voice of a sample, dropping the fraction.
 *
 * @param nSample The sample.
 * @param nVoice The voice, half the channel.
 * @param fFrequency The frequency, in samples a second.
 * @return The result of SetSoundChannelFrequency().
 * @ghidraAddress BRD3D: 0x10072750
 * @ghidraAddress BRGlide: 0x1006b6c0
 */
bool SetSoundVoiceFrequencyFloat(int nSample, int nVoice, float fFrequency);

/**
 * Stop the buffer of the first channel of a voice of a sample, when sound is on.
 *
 * @param nSample The sample.
 * @param nVoice The voice. Its first channel is twice the voice.
 * @return Whether the buffer stopped, and true without a buffer or without sound.
 * @ghidraAddress BRD3D: 0x10072ba0
 * @ghidraAddress BRGlide: 0x1006bb10
 */
bool StopSoundVoice(int nSample, int nVoice);

/**
 * Set the volume and the pan of the buffer of a node from the levels of its two sides.
 *
 * @param pNode The node.
 * @param dwLevels The level of the left side in the high word and of the right in the low word,
 * each up to 32.
 * @return Whether the levels were set.
 * @ghidraAddress BRD3D: 0x10072820
 * @ghidraAddress BRGlide: 0x1006b790
 */
bool SetSoundBufferStereoLevels(SoundBufferNode *pNode, DWORD dwLevels);

/** The settings last applied to the buffer of a channel. */
struct SoundChannelState {
    int nSample;           /*!< The sample of the channel. */
    char abUnknown04[4];   /* +0x04 */
    long long llFrequency; /*!< The frequency, before the scale of the sample. */
    char abUnknown10[4];   /* +0x10 */
    DWORD dwStereoLevels;  /*!< The levels of the two sides, as SetSoundBufferStereoLevels(). */
};

/**
 * The settings last applied to the buffer of each channel.
 *
 * @ghidraAddress BRD3D: 0x11828d20
 * @ghidraAddress BRGlide: 0x1184c080
 */
extern SoundChannelState g_aSoundChannelState[SOUND_CHANNEL_COUNT];

/**
 * Set the levels of the two sides of the buffer of a channel and record them, when sound is on.
 *
 * @param nChannel The channel.
 * @param dwLevels The levels, as SetSoundBufferStereoLevels().
 * @return Whether the levels were set, and true without sound.
 * @ghidraAddress BRD3D: 0x100727c0
 * @ghidraAddress BRGlide: 0x1006b730
 */
bool SetAudioChannelStereoLevels(int nChannel, DWORD dwLevels);

/**
 * The scale of a frequency of each channel, from the sample assigned to it.
 *
 * @ghidraAddress BRD3D: 0x11828e88
 * @ghidraAddress BRGlide: 0x1184c1e8
 */
extern double g_adChannelFrequencyScale[SOUND_CHANNEL_COUNT];

/**
 * Stop the buffer of a channel and give the channel the buffer of a sample for it, when sound is
 * on.
 *
 * @param nSample The sample.
 * @param nChannel The channel.
 * @return Whether the sample has a buffer for the channel, and true without sound.
 * @ghidraAddress BRD3D: 0x100725c0
 * @ghidraAddress BRGlide: 0x1006b530
 */
bool AssignAudioChannel(int nSample, int nChannel);

/**
 * Set the frequency of the buffer of a channel, scaled by its sample, and record it, when sound is
 * on.
 *
 * @param nChannel The channel.
 * @param llFrequency The frequency in units of 2 to the power of -32.
 * @return Whether the frequency was set, and true without sound.
 * @ghidraAddress BRD3D: 0x10072680
 * @ghidraAddress BRGlide: 0x1006b5f0
 */
bool SetAudioChannelFrequency(int nChannel, long long llFrequency);

/**
 * The settings requested for the buffer of each channel.
 *
 * @ghidraAddress BRD3D: 0x118ac720
 * @ghidraAddress BRGlide: 0x118eef40
 */
extern SoundChannelState g_aSoundChannelRequested[SOUND_CHANNEL_COUNT];

/**
 * Play the buffer of a sample for a channel and give it to the channel, when sound is on. The
 * frequency of the buffer, without the scale of the channel, is requested and recorded.
 *
 * @param nSample The sample.
 * @param nChannel The channel.
 * @param bLooping Whether the buffer plays in a loop.
 * @return Whether the buffer plays, false without a buffer, and true without sound.
 * @ghidraAddress BRD3D: 0x10072910
 * @ghidraAddress BRGlide: 0x1006b880
 */
bool StartSoundChannelPlayback(int nSample, int nChannel, int bLooping);

/**
 * Whether the channels that play sample 0 move to sample 25, rather than back.
 *
 * @ghidraAddress BRD3D: 0x118290f4
 * @ghidraAddress BRGlide: 0x1184c454
 */
extern int g_bAltLoopingSoundSet;

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
 * Set the levels of the two sides of a channel of a sample and play it, when sound is on.
 *
 * @param nSample The sample, an index into g_aSoundChannelBuffers.
 * @param nChannel The channel.
 * @param dwLevels The levels, as SetSoundBufferStereoLevels() takes them.
 * @param bLooping Whether the buffer plays in a loop.
 * @return Whether the levels were set and the buffer played, and true without sound.
 * @ghidraAddress BRD3D: 0x10072a90
 * @ghidraAddress BRGlide: 0x1006ba00
 */
bool SetStereoAndPlaySoundChannel(int nSample, int nChannel, DWORD dwLevels, int bLooping);

/**
 * Set the levels of the two sides of channel 1 of a sample and play it.
 *
 * @param nSample The sample.
 * @param dwLevels The levels, as SetSoundBufferStereoLevels() takes them.
 * @param bLooping Whether the buffer plays in a loop.
 * @return The result of SetStereoAndPlaySoundChannel().
 * @ghidraAddress BRD3D: 0x10072a70
 * @ghidraAddress BRGlide: 0x1006b9e0
 */
bool PlaySoundChannelOneWithStereo(int nSample, DWORD dwLevels, int bLooping);

/**
 * Set the levels of the two sides of a channel of a sample, when sound is on.
 *
 * @param nSample The sample.
 * @param nChannel The channel.
 * @param dwLevels The levels, as SetSoundBufferStereoLevels() takes them.
 * @return Whether the levels were set, and true without sound.
 * @ghidraAddress BRD3D: 0x10072b30
 * @ghidraAddress BRGlide: 0x1006baa0
 */
bool SetSoundChannelStereo(int nSample, int nChannel, DWORD dwLevels);

/**
 * Set the levels of the two sides of the even channel of a voice of a sample.
 *
 * @param nSample The sample.
 * @param nVoice The voice, half the channel.
 * @param dwLevels The levels, as SetSoundBufferStereoLevels() takes them.
 * @return The result of SetSoundChannelStereo().
 * @ghidraAddress BRD3D: 0x10072b80
 * @ghidraAddress BRGlide: 0x1006baf0
 */
bool SetSoundVoiceStereo(int nSample, int nVoice, DWORD dwLevels);

/**
 * Set the levels of the two sides of channel 1 of a sample and play it once.
 *
 * @param nSample The sample.
 * @param dwLevels The levels, as SetSoundBufferStereoLevels() takes them.
 * @return The result of PlaySoundChannelOneWithStereo().
 * @ghidraAddress BRD3D: 0x10072af0
 * @ghidraAddress BRGlide: 0x1006ba60
 */
bool PlaySoundChannelOneOnce(int nSample, DWORD dwLevels);

/**
 * Set the levels of the two sides of the even channel of a voice of a sample and play it in a
 * loop.
 *
 * @param nSample The sample.
 * @param nVoice The voice, half the channel.
 * @param dwLevels The levels, as SetSoundBufferStereoLevels() takes them.
 * @return The result of SetStereoAndPlaySoundChannel().
 * @ghidraAddress BRD3D: 0x10072b10
 * @ghidraAddress BRGlide: 0x1006ba80
 */
bool PlayLoopedSoundVoiceWithStereo(int nSample, int nVoice, DWORD dwLevels);

/**
 * Load a wave file into a node of a buffer of DirectSound at the end of g_SoundBufferListHead, at
 * its own frequency, the centre, and the volume of the sound effects. A step that fails frees what
 * it created.
 *
 * @param pszPath The path of the file.
 * @return The node, or null.
 * @ghidraAddress BRD3D: 0x10072ca0
 * @ghidraAddress BRGlide: 0x1006bc10
 */
SoundBufferNode *LoadAudioFile(const char *pszPath);

/**
 * Load the three files of a sound effect of a race, named by the prefixes the banks of sounds
 * select for its channel: the wave (`.wav`), the variant (`h.wav`), and the looping sound
 * (`r.wav`). Without sound, or when the first bank does not enable the channel, the three are
 * cleared.
 *
 * @param nEffect The effect, half the channel.
 * @return Whether the files loaded, and false at once when the looping sound does not.
 * @ghidraAddress BRD3D: 0x100730a0
 * @ghidraAddress BRGlide: 0x1006c010
 */
bool LoadSfxWaveVariants(int nEffect);

/**
 * Load the samples of a set of sounds into the channels the banks of sounds enable, from the
 * second sample to the one before the last. The set of a race first loads the variants of its sound
 * effects. Nothing loads without sound.
 *
 * @param nSet The set, by the SOUND_SET_ values.
 * @return Whether every sample loaded.
 * @ghidraAddress BRD3D: 0x10073320
 * @ghidraAddress BRGlide: 0x1006c290
 */
bool LoadSoundSet(int nSet);

/**
 * Stop every sound buffer of a list.
 *
 * @param pHead The head of the list.
 * @return Always DS_OK.
 * @ghidraAddress BRD3D: 0x10072bf0
 * @ghidraAddress BRGlide: 0x1006bb60
 */
HRESULT StopAllSoundBuffersInList(SoundBufferNode *pHead);

/**
 * Release and free every node of a list of sound buffers, with the sample and the format of
 * each, and empty the list. The head is not freed.
 *
 * @param pHead The head of the list.
 * @return Always DS_OK.
 * @ghidraAddress BRD3D: 0x10072c20
 * @ghidraAddress BRGlide: 0x1006bb90
 */
HRESULT FreeSoundBufferList(SoundBufferNode *pHead);

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
 * Set the volumes of the music and the sound effects from the levels of g_GlobalRallyConfig.
 *
 * @ghidraAddress BRD3D: 0x10060d90
 * @ghidraAddress BRGlide: 0x10059e00
 */
void ApplyAudioVolumeSettings(void);

/**
 * Add a user of DirectSound when sound effects play. The first user clears the channels and the
 * banks, allocates the wave format of 22050 Hz, 16 bits, and two channels, and creates
 * DirectSound with priority and its primary buffer, playing in a loop. A step that fails releases
 * what it created.
 *
 * @return Whether the last step succeeded, and true without sound effects or for a later user.
 * @ghidraAddress BRD3D: 0x10073560
 * @ghidraAddress BRGlide: 0x1006c4d0
 */
bool InitializeDirectSound(void);

/**
 * Stop and free the list of sound buffers and clear the buffers of the channels of every loaded
 * sample and of the channels that play, when DirectSound plays sound effects.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x100734f0
 * @ghidraAddress BRGlide: 0x1006c460
 */
bool ResetAudioSubsystem(void);

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
