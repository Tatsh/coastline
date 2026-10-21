#ifndef COASTLINE_AUDIO_H
#define COASTLINE_AUDIO_H

/**
 * @file
 * The music player of FastTracker 2 modules, the sound channels, and the mixing thread.
 *
 * The player plays the first sample of each instrument of an XM module, 8-bit and without
 * ping-pong loops, on the channels of the music mixer. A tick comes every other message of a timer
 * of 10 milliseconds, and the mixing thread mixes what the audio interface has played since the
 * last message.
 */

#include <PR/os.h>
#include <PR/ultratypes.h>

#include "audiomix.h"
#include "render.h"
#include "vehicle.h"

/** The sample rate the audio interface is asked for. */
#define AUDIO_FREQUENCY 22000

/** The octaves and notes of the table of steps. */
#define MUSIC_OCTAVES 10
#define MUSIC_NOTES 12

/** The header of an XM sample as the player keeps it, followed by its decoded data. */
typedef struct {
    u32 length;
    /** Left in the little-endian order of the module. */
    u32 loopStart;
    u32 loopLength;
    u8 volume;
    s8 finetune;
    /** 0 for no loop and 1 for a forward loop. */
    u8 type;
    u8 panning;
    /** The notes to add to a note of the pattern. */
    s8 relativeNote;
    u8 reserved;
    char name[22];
} XmSample;

/** The state of a channel of the music. */
typedef struct {
    /** The step a tone portamento slides to, or 0. */
    u64 portamentoTarget;
    /** The change of the period a tick, or 0. */
    s16 portamentoSpeed;
    /** The change of the volume a tick, or 0. */
    s16 volumeSlide;
    /** The note, from 0. */
    u8 note;
    /** The instrument playing, from 1, or 0 for none. */
    u8 instrument;
    /** The volume, from 0 to 64. */
    u8 volume;
    u8 effect;
    u8 effectParameter;
    /** The instrument of the last row. */
    u8 rowInstrument;
    /** The three notes of an arpeggio, then the step through them, which the arpeggio also reads.
     */
    u8 arpeggio[4];
    /** Whether an arpeggio plays. */
    u8 arpeggioOn;
    u8 pad17;
} MusicChannel;

/** The position of the music player in its module. */
typedef struct {
    /** The ticks of a row. */
    u16 speed;
    /** The ticks left of the row. */
    u16 tick;
    /** The rows left of the pattern. */
    u16 rowsLeft;
    u16 pad6;
    /** The order of the patterns. */
    u8 *orders;
    /** The packed data of the next row. */
    u8 *row;
    u8 order;
    u8 songLength;
    u8 restartOrder;
    u8 pad13[0x18 - 0x13];
} MusicPlayer;

/** A sample a sound effect channel plays, and the loop at its end. */
typedef struct {
    s8 *start;
    u32 length;
    /** The bytes the channel goes back at the end, or 0 to stop. */
    u32 loopLength;
} SoundEffectSample;

/**
 * The menu music, an XM module the game decompresses below the program. The link script places
 * it.
 *
 * @ghidraAddress USA: 0x80025c00
 */
extern u8 g_abMenuMusicModule[];

/** The work memory of the music player, at the start of the BSS. @ghidraAddress USA: 0x802ac400 */
extern u8 g_abMusicModuleWorkBuffer[];

/** The number of sound channels in use. @ghidraAddress USA: 0x802a49c0 */
extern s32 g_nAudioChannelCount;

/** The volume of the music the game sets. @ghidraAddress USA: 0x802a49c4 */
extern u8 g_bBgmVolume;

/** The volume of the music the player chose. @ghidraAddress USA: 0x802a49c8 */
extern u8 g_bMusicVolume;

/** The volume of the sound effects the game sets. @ghidraAddress USA: 0x802a49cc */
extern u8 g_bSfxMasterVolume;

/** The volume of the sound effects the player chose. @ghidraAddress USA: 0x802a49d0 */
extern u8 g_bSfxVolume;

/** The frequencies of the twelve notes from middle C. @ghidraAddress USA: 0x802a49d4 */
extern f32 g_afNoteFrequencies[MUSIC_NOTES];

/** Cleared when a pattern starts. @ghidraAddress USA: 0x802a4a04 */
extern s32 g_dwMusicPatternRow;

/** A silent sample that a channel at rest points at. @ghidraAddress USA: 0x802a4a08 */
extern s8 g_SilenceSampleData[4];

/**
 * Toggled each message of the timer. The music ticks when it is set.
 *
 * @ghidraAddress USA: 0x802a4a0c
 */
extern s32 g_dwAudioTickToggle;

/** The place in the ring buffer the audio interface had reached. @ghidraAddress USA: 0x802a4a10 */
extern s32 g_dwAiDmaPrevLength;

/** Where in the ring buffer the sound effects mix next. @ghidraAddress USA: 0x802a4a14 */
extern u32 g_dwSoundEffectMixOffset;

/** The queue of the messages of the audio timer. @ghidraAddress USA: 0x803746f0 */
extern OSMesgQueue g_AudioMsgQueue;

/** @ghidraAddress USA: 0x80374708 */
extern OSMesg g_aAudioMsgBuf[2];

/** The timer that wakes the mixing thread. @ghidraAddress USA: 0x80374710 */
extern OSTimer g_AudioTimer;

/** The stereo ring buffer the audio interface plays. @ghidraAddress USA: 0x803747d0 */
extern s16 g_AudioDmaBuffer[AUDIO_RING_BYTES / sizeof(s16)];

/** The sample of each instrument, or NULL. @ghidraAddress USA: 0x803787d0 */
extern XmSample *g_apInstrumentData[];

/** Each pattern with its header. @ghidraAddress USA: 0x803789d0 */
extern u8 *g_apPatternData[];

/** @ghidraAddress USA: 0x80378dd0 */
extern MusicChannel g_MusicChannelStates[MIXER_CHANNELS];

/** The samples of the sound effect channels. @ghidraAddress USA: 0x80378f50 */
extern SoundEffectSample g_aSoundEffectSamples[SOUND_EFFECT_CHANNELS];

/** Whether the music plays. @ghidraAddress USA: 0x80378f98 */
extern s32 g_bMusicPlaybackEnabled;

/** @ghidraAddress USA: 0x80378fa0 */
extern MusicPlayer g_MusicPlayer;

/** @ghidraAddress USA: 0x80378fb8 */
extern OSThread g_AudioThread;

/** The stack of the mixing thread, in doublewords. */
#define AUDIO_THREAD_STACK_WORDS 0x80

/** @ghidraAddress USA: 0x80379168 */
extern u64 g_aAudioThreadStack[AUDIO_THREAD_STACK_WORDS];

/** The 32.32 step of each note of each octave. @ghidraAddress USA: 0x80379568 */
extern u64 g_aFreqScaleTable[MUSIC_OCTAVES][MUSIC_NOTES];

/** The bytes the mixers mix this time. @ghidraAddress USA: 0x80379930 */
extern s32 g_dwAudioMixBytes;

/** @ghidraAddress USA: 0x80379934 */
extern s32 g_dwAiDmaCurrentLength;

/** The channel the extra music mixers reached. @ghidraAddress USA: 0x80379938 */
extern s32 g_dwAudioChannelLoopIdx;

/**
 * Copy the order table, the patterns and the samples of an XM module into the work memory, decoding
 * the samples and following each with the start of its loop or with silence.
 *
 * @param module The XM module.
 * @param work The memory the player reads the module from.
 * @ghidraAddress USA: 0x80256720
 */
void LoadMusicModule(u8 *module, u8 *work);

/**
 * Fill the steps of every note of every octave.
 *
 * @ghidraAddress USA: 0x80256c2c
 */
void BuildFrequencyScaleTable(void);

/**
 * Loop or stop the music channels that passed the end of their samples.
 *
 * @ghidraAddress USA: 0x80256d3c
 */
void CheckMusicLoopPoints(void);

/**
 * Read a row of a pattern for every channel and start its notes and effects.
 *
 * @param row The packed data of the row.
 * @return The data of the next row.
 * @ghidraAddress USA: 0x80256dec
 */
u8 *ParseMusicPatternRow(u8 *row);

/**
 * Start the music from before its first order.
 *
 * @ghidraAddress USA: 0x802571ac
 */
void ResetMusicChannels(void);

/**
 * Run a tick of the music: read a row when one is due, and run the arpeggios, portamentos and
 * volume slides.
 *
 * @ghidraAddress USA: 0x8025721c
 */
void UpdateMusicPlayback(void);

/**
 * Start the audio interface, the music player and the mixing thread.
 *
 * @param module The XM module to play.
 * @param work The memory the player reads the module from.
 * @ghidraAddress USA: 0x802575c4
 */
void InitAudioSystem(void *module, void *work);

/**
 * Stop the music and clear the state of every channel.
 *
 * @ghidraAddress USA: 0x802578f4
 */
void ResetMusicPlaybackState(void);

/**
 * Start playing an XM module.
 *
 * @param module The XM module to play.
 * @param work The memory the player reads the module from.
 * @ghidraAddress USA: 0x80257964
 */
void InitMusicPlayback(void *module, void *work);

/**
 * The first sound effect channel that is silent.
 *
 * @return The channel, or -1 if all play.
 * @ghidraAddress USA: 0x802579f4
 */
s16 FindFreeSoundChannel(void);

/**
 * Play a sample on a sound effect channel at its own rate and the middle levels.
 *
 * @param channel The sound effect channel.
 * @param sample The first byte of the sample.
 * @param length The length of the sample, in bytes.
 * @param loopLength The length of the loop at the end of the sample in bytes, or 0 to play once.
 * @ghidraAddress USA: 0x80257b04
 */
void InitSoundChannel(s16 channel, s8 *sample, u32 length, u32 loopLength);

/**
 * Play a sample on a sound effect channel at its own rate and no level, for SetSoundChannelVolume()
 * to raise.
 *
 * @param channel The sound effect channel.
 * @param sample The first byte of the sample.
 * @param length The length of the sample, in bytes.
 * @param loopLength The length of the loop at the end of the sample in bytes, or 0 to play once.
 * @ghidraAddress USA: 0x80257b78
 */
void StartSoundChannelMuted(s16 channel, s8 *sample, u32 length, u32 loopLength);

/**
 * Set the levels of a sound effect channel, the right in the upper half and the left in the lower,
 * each at most 32.
 *
 * @param channel The sound effect channel.
 * @param levels The right level in the upper 16 bits and the left in the lower 16 bits.
 * @ghidraAddress USA: 0x80257be4
 */
void SetSoundChannelVolume(s16 channel, u32 levels);

/**
 * Loop or stop the sound effect channels that passed the end of their samples.
 *
 * @ghidraAddress USA: 0x80257c44
 */
void CheckSoundEffectLoopPoints(void);

/**
 * The mixing thread: on each message of the timer, scale the volumes, mix what the audio interface
 * has played, tick the music, and feed the Rumble Paks.
 *
 * @param arg The argument of the thread, which is not read.
 * @ghidraAddress USA: 0x80257d3c
 */
void AudioMixingThreadMain(void *arg);

/**
 * The end of a routine with no start, which restores the registers of the mixing thread. Nothing
 * calls it.
 *
 * @ghidraAddress USA: 0x80258040
 */
void UnusedAudioMixingStub(void);

/**
 * Play the beep of the countdown, and of the laps.
 *
 * @ghidraAddress USA: 0x8022b3c4
 */
void PlayBeepSound(void);

/**
 * Play the sound of the start of the race.
 *
 * @ghidraAddress USA: 0x8022b3e4
 */
void PlayGoSound(void);

/**
 * Forget the nearest sound of the track found the last frame.
 *
 * @ghidraAddress USA: 0x8022b404
 */
void ResetSoundSourceTracking(void);

/**
 * Stop the sounds of the track and forget their sources.
 *
 * @ghidraAddress USA: 0x8022b428
 */
void ResetAllSoundState(void);

/**
 * Play the nearest sound of the track on its channel, panned and pitched for the camera.
 *
 * @ghidraAddress USA: 0x8022b534
 */
void UpdateAmbientSoundChannel(void);

/**
 * Offer the sound of the airplane as the nearest sound of the track, on the tracks that have one.
 *
 * @param instance The placed model of the airplane.
 * @param position Its position.
 * @param camera The camera that hears it.
 * @ghidraAddress USA: 0x8022b954
 */
void UpdateAirplaneSound(s32 instance, Vec3f *position, CameraFrame *camera);

/**
 * Offer the sound of a waterfall as the nearest sound of the track, on the tracks that have one.
 *
 * @param instance The source the sound is kept under.
 * @param position The position of the waterfall.
 * @param camera The camera that hears it.
 * @ghidraAddress USA: 0x8022ba10
 */
void UpdateWaterfallSound(s32 instance, Vec3f *position, CameraFrame *camera);

/**
 * Do nothing with a camera.
 *
 * @param camera The camera.
 * @ghidraAddress USA: 0x8022ba98
 */
void StubSoundUpdate(CameraFrame *camera);

/**
 * Load the sound effects of a race and start their channels muted.
 *
 * @ghidraAddress USA: 0x8022baa0
 */
void LoadAllSoundEffects(void);

/**
 * Play the engine, the impacts and the rumble of a vehicle for a frame.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8022bcb4
 */
void UpdateVehicleSounds(Vehicle *vehicle);

#endif
