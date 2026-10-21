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
#define AUDIO_FREQUENCY 21998

/* The size of the table of steps. */
#define MUSIC_OCTAVES 10 /*!< The octaves of the table of steps. */
#define MUSIC_NOTES 12   /*!< The notes of each octave of the table of steps. */

/** The header of an XM sample as the player keeps it, followed by its decoded data. */
typedef struct {
    u32 length;      /*!< The samples of the data, one byte each, in the order of the processor. */
    u32 loopStart;   /*!< The start of the loop, in the little-endian order of the module. */
    u32 loopLength;  /*!< The samples of the loop, in the order of the processor. */
    u8 volume;       /*!< The volume of the sample, from 0 to 64. */
    s8 finetune;     /*!< The finetune of the sample; the player does not read it. */
    u8 type;         /*!< 0 for no loop and 1 for a forward loop. */
    u8 panning;      /*!< The panning of the sample; the player does not read it. */
    s8 relativeNote; /*!< The notes to add to a note of the pattern. */
    u8 reserved;     /*!< The reserved byte of the header of the module. */
    char name[22];   /*!< The name of the sample in the module. */
} XmSample;

/** The state of a channel of the music. */
typedef struct {
    u64 portamentoTarget; /*!< The mixer step a tone portamento slides to, or 0. */
    s16 portamentoSpeed;  /*!< The change of the period a tick, or 0. */
    s16 volumeSlide;      /*!< The change of the volume a tick, or 0. */
    u8 note;              /*!< The note, from 0. */
    u8 instrument;        /*!< The instrument playing, from 1, or 0 for none. */
    u8 volume;            /*!< The volume, from 0 to 64. */
    u8 effect;            /*!< The effect command of the last row. */
    u8 effectParameter;   /*!< The parameter of the effect command of the last row. */
    u8 rowInstrument;     /*!< The instrument of the last row. */
    u8 arpeggio[4]; /*!< The three notes of an arpeggio, then the index of the note playing. */
    u8 arpeggioOn;  /*!< Whether an arpeggio plays. */
    u8 pad17;       /*!< Padding to the 8-byte alignment of the structure. */
} MusicChannel;

/** The position of the music player in its module. */
typedef struct {
    u16 speed;    /*!< The ticks of a row. */
    u16 tick;     /*!< The ticks remaining of the row. */
    u16 rowsLeft; /*!< The rows remaining of the pattern. */
    u16 pad6;     /*!< Padding to the alignment of the next member. */
    u8 *orders;   /*!< The order table of the patterns, copied from the module. */
    u8 *row;      /*!< The packed data of the next row. */
    u8 order; /*!< The index in the order table of the pattern playing, 0xFF before the first. */
    u8 songLength;         /*!< The entries of the order table that play. */
    u8 restartOrder;       /*!< The index in the order table the song restarts from at its end. */
    u8 pad13[0x18 - 0x13]; /*!< Padding to the size of the structure in the binary. */
} MusicPlayer;

/** A sample a sound effect channel plays, and the loop at its end. */
typedef struct {
    s8 *start;      /*!< The signed 8-bit samples. */
    u32 length;     /*!< The samples before the end, one byte each. */
    u32 loopLength; /*!< The bytes the channel goes back at the end, or 0 to stop. */
} SoundEffectSample;

/**
 * The menu music, an XM module the game decompresses below the program, at a fixed address.
 *
 * @ghidraAddress USA: 0x80025c00
 * @ghidraAddress JPN: 0x80025c00
 * @ghidraAddress EUR: 0x80025c00
 */
#define g_abMenuMusicModule ((u8 *)0x80025C00)

extern u8 _menuMusicSegmentRomStart[];
extern u8 _scrollSoundSegmentRomStart[];
extern u8 _selectSoundSegmentRomStart[];

/**
 * The work memory of the music player, at the start of the BSS.
 *
 * @ghidraAddress USA: 0x802ac400
 * @ghidraAddress JPN: 0x802acb30
 * @ghidraAddress EUR: 0x802ae0f0
 */
extern u8 g_abMusicModuleWorkBuffer[];

/**
 * The number of sound channels in use.
 *
 * @ghidraAddress USA: 0x802a49c0
 * @ghidraAddress JPN: 0x802a6c20
 * @ghidraAddress EUR: 0x802a7180
 */
extern s32 g_nAudioChannelCount;

/**
 * The volume of the music the game sets.
 *
 * @ghidraAddress USA: 0x802a49c4
 * @ghidraAddress JPN: 0x802a6c24
 * @ghidraAddress EUR: 0x802a7184
 */
extern u8 g_bBgmVolume;

/**
 * The volume of the music the player chose.
 *
 * @ghidraAddress USA: 0x802a49c8
 * @ghidraAddress JPN: 0x802a6c28
 * @ghidraAddress EUR: 0x802a7188
 */
extern u8 g_bMusicVolume;

/**
 * The volume of the sound effects the game sets.
 *
 * @ghidraAddress USA: 0x802a49cc
 * @ghidraAddress JPN: 0x802a6c2c
 * @ghidraAddress EUR: 0x802a718c
 */
extern u8 g_bSfxMasterVolume;

/**
 * The volume of the sound effects the player chose.
 *
 * @ghidraAddress USA: 0x802a49d0
 * @ghidraAddress JPN: 0x802a6c30
 * @ghidraAddress EUR: 0x802a7190
 */
extern u8 g_bSfxVolume;

/**
 * The frequencies of the twelve notes from middle C.
 *
 * @ghidraAddress USA: 0x802a49d4
 * @ghidraAddress JPN: 0x802a6c34
 * @ghidraAddress EUR: 0x802a7194
 */
extern f32 g_afNoteFrequencies[MUSIC_NOTES];

/**
 * Cleared when a pattern starts.
 *
 * @ghidraAddress USA: 0x802a4a04
 * @ghidraAddress JPN: 0x802a6c64
 * @ghidraAddress EUR: 0x802a71c4
 */
extern s32 g_dwMusicPatternRow;

/**
 * A silent sample that a channel at rest points at.
 *
 * @ghidraAddress USA: 0x802a4a08
 * @ghidraAddress JPN: 0x802a6c68
 * @ghidraAddress EUR: 0x802a71c8
 */
extern s8 g_SilenceSampleData[4];

/**
 * Toggled each message of the timer. The music ticks when it is set.
 *
 * @ghidraAddress USA: 0x802a4a0c
 * @ghidraAddress JPN: 0x802a6c6c
 * @ghidraAddress EUR: 0x802a71cc
 */
extern s32 g_dwAudioTickToggle;

/**
 * The place in the ring buffer the audio interface had reached.
 *
 * @ghidraAddress USA: 0x802a4a10
 * @ghidraAddress JPN: 0x802a6c70
 * @ghidraAddress EUR: 0x802a71d0
 */
extern s32 g_dwAiDmaPrevLength;

/**
 * Where in the ring buffer the sound effects mix next.
 *
 * @ghidraAddress USA: 0x802a4a14
 * @ghidraAddress JPN: 0x802a6c74
 * @ghidraAddress EUR: 0x802a71d4
 */
extern u32 g_dwSoundEffectMixOffset;

/**
 * The queue of the messages of the audio timer.
 *
 * @ghidraAddress USA: 0x803746f0
 * @ghidraAddress JPN: 0x80375860
 * @ghidraAddress EUR: 0x80376bf0
 */
extern OSMesgQueue g_AudioMsgQueue;

/**
 * @ghidraAddress USA: 0x80374708
 * @ghidraAddress JPN: 0x80375878
 * @ghidraAddress EUR: 0x80376c08
 */
extern OSMesg g_aAudioMsgBuf[2];

/**
 * The timer that wakes the mixing thread.
 *
 * @ghidraAddress USA: 0x80374710
 * @ghidraAddress JPN: 0x80375880
 * @ghidraAddress EUR: 0x80376c10
 */
extern OSTimer g_AudioTimer;

/**
 * The stereo ring buffer the audio interface plays.
 *
 * @ghidraAddress USA: 0x803747d0
 * @ghidraAddress JPN: 0x80375940
 * @ghidraAddress EUR: 0x80376cd0
 */
extern s16 g_AudioDmaBuffer[AUDIO_RING_BYTES / sizeof(s16)];

/**
 * The sample of each instrument, or NULL.
 *
 * @ghidraAddress USA: 0x803787d0
 * @ghidraAddress JPN: 0x80379940
 * @ghidraAddress EUR: 0x8037acd0
 */
extern XmSample *g_apInstrumentData[];

/**
 * Each pattern with its header.
 *
 * @ghidraAddress USA: 0x803789d0
 * @ghidraAddress JPN: 0x80379b40
 * @ghidraAddress EUR: 0x8037aed0
 */
extern u8 *g_apPatternData[];

/**
 * @ghidraAddress USA: 0x80378dd0
 * @ghidraAddress JPN: 0x80379f40
 * @ghidraAddress EUR: 0x8037b2d0
 */
extern MusicChannel g_MusicChannelStates[MIXER_CHANNELS];

/**
 * The samples of the sound effect channels.
 *
 * @ghidraAddress USA: 0x80378f50
 * @ghidraAddress JPN: 0x8037a0c0
 * @ghidraAddress EUR: 0x8037b450
 */
extern SoundEffectSample g_aSoundEffectSamples[SOUND_EFFECT_CHANNELS];

/**
 * Whether the music plays.
 *
 * @ghidraAddress USA: 0x80378f98
 * @ghidraAddress JPN: 0x8037a108
 * @ghidraAddress EUR: 0x8037b498
 */
extern s32 g_bMusicPlaybackEnabled;

/**
 * @ghidraAddress USA: 0x80378fa0
 * @ghidraAddress JPN: 0x8037a110
 * @ghidraAddress EUR: 0x8037b4a0
 */
extern MusicPlayer g_MusicPlayer;

/**
 * @ghidraAddress USA: 0x80378fb8
 * @ghidraAddress JPN: 0x8037a128
 * @ghidraAddress EUR: 0x8037b4b8
 */
extern OSThread g_AudioThread;

/** The stack of the mixing thread, in doublewords. */
#define AUDIO_THREAD_STACK_WORDS 0x80

/** @ghidraAddress USA: 0x80379168 */
extern u64 g_aAudioThreadStack[AUDIO_THREAD_STACK_WORDS];

/**
 * The 32.32 step of each note of each octave.
 *
 * @ghidraAddress USA: 0x80379568
 * @ghidraAddress JPN: 0x8037a6d8
 * @ghidraAddress EUR: 0x8037ba68
 */
extern u64 g_aFreqScaleTable[MUSIC_OCTAVES][MUSIC_NOTES];

/**
 * The bytes the mixers mix this time.
 *
 * @ghidraAddress USA: 0x80379930
 * @ghidraAddress JPN: 0x8037aaa0
 * @ghidraAddress EUR: 0x8037be30
 */
extern s32 g_dwAudioMixBytes;

/**
 * @ghidraAddress USA: 0x80379934
 * @ghidraAddress JPN: 0x8037aaa4
 * @ghidraAddress EUR: 0x8037be34
 */
extern s32 g_dwAiDmaCurrentLength;

/**
 * The channel the extra music mixers reached.
 *
 * @ghidraAddress USA: 0x80379938
 * @ghidraAddress JPN: 0x8037aaa8
 * @ghidraAddress EUR: 0x8037be38
 */
extern s32 g_dwAudioChannelLoopIdx;

/**
 * Copy the order table, the patterns and the samples of an XM module into the work memory, decoding
 * the samples and following each with the start of its loop or with silence.
 *
 * @param module The XM module.
 * @param work The memory the player reads the module from.
 * @ghidraAddress USA: 0x80256720
 * @ghidraAddress JPN: 0x80257680
 * @ghidraAddress EUR: 0x80257cf0
 */
void LoadMusicModule(u8 *module, u8 *work);

/**
 * Fill the steps of every note of every octave.
 *
 * @ghidraAddress USA: 0x80256c2c
 * @ghidraAddress JPN: 0x80257b8c
 * @ghidraAddress EUR: 0x802581fc
 */
void BuildFrequencyScaleTable(void);

/**
 * Loop or stop the music channels that passed the end of their samples.
 *
 * @ghidraAddress USA: 0x80256d3c
 * @ghidraAddress JPN: 0x80257c9c
 * @ghidraAddress EUR: 0x8025830c
 */
void CheckMusicLoopPoints(void);

/**
 * Read a row of a pattern for every channel and start its notes and effects.
 *
 * @param row The packed data of the row.
 * @return The data of the next row.
 * @ghidraAddress USA: 0x80256dec
 * @ghidraAddress JPN: 0x80257d4c
 * @ghidraAddress EUR: 0x802583bc
 */
u8 *ParseMusicPatternRow(u8 *row);

/**
 * Start the music from before its first order.
 *
 * @ghidraAddress USA: 0x802571ac
 * @ghidraAddress JPN: 0x8025810c
 * @ghidraAddress EUR: 0x8025877c
 */
void ResetMusicChannels(void);

/**
 * Run a tick of the music: read a row when one is due, and run the arpeggios, portamentos and
 * volume slides.
 *
 * @ghidraAddress USA: 0x8025721c
 * @ghidraAddress JPN: 0x8025817c
 * @ghidraAddress EUR: 0x802587ec
 */
void UpdateMusicPlayback(void);

/**
 * Start the audio interface, the music player and the mixing thread.
 *
 * @param module The XM module to play.
 * @param work The memory the player reads the module from.
 * @ghidraAddress USA: 0x802575c4
 * @ghidraAddress JPN: 0x80258524
 * @ghidraAddress EUR: 0x80258b94
 */
void InitAudioSystem(void *module, void *work);

/**
 * Stop the music and clear the state of every channel.
 *
 * @ghidraAddress USA: 0x802578f4
 * @ghidraAddress JPN: 0x80258854
 * @ghidraAddress EUR: 0x80258ec4
 */
void ResetMusicPlaybackState(void);

/**
 * Start playing an XM module.
 *
 * @param module The XM module to play.
 * @param work The memory the player reads the module from.
 * @ghidraAddress USA: 0x80257964
 * @ghidraAddress JPN: 0x802588c4
 * @ghidraAddress EUR: 0x80258f34
 */
void InitMusicPlayback(void *module, void *work);

/**
 * The first sound effect channel that is silent.
 *
 * @return The channel, or -1 if all play.
 * @ghidraAddress USA: 0x802579f4
 * @ghidraAddress JPN: 0x80258954
 * @ghidraAddress EUR: 0x80258fc4
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
 * @ghidraAddress JPN: 0x80258a64
 * @ghidraAddress EUR: 0x802590d4
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
 * @ghidraAddress JPN: 0x80258ad8
 * @ghidraAddress EUR: 0x80259148
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
 * @ghidraAddress JPN: 0x80258ba4
 * @ghidraAddress EUR: 0x80259214
 */
void CheckSoundEffectLoopPoints(void);

/**
 * The mixing thread: on each message of the timer, scale the volumes, mix what the audio interface
 * has played, tick the music, and feed the Rumble Paks.
 *
 * @param arg The argument of the thread, which is not read.
 * @ghidraAddress USA: 0x80257d3c
 * @ghidraAddress JPN: 0x80258c9c
 * @ghidraAddress EUR: 0x8025930c
 */
void AudioMixingThreadMain(void *arg);

/** The sound effects of a race. */
#define SOUND_EFFECTS 16

/**
 * The sound effects: the engine, which the airplane shares, the hits, the skids and the rolling on
 * each surface of the track, the countdown and the waterfall. The skids run from 4 to 7 and the
 * rolling from 8 to 12; surfaces 0 and 3 share theirs.
 */
enum {
    SOUND_EFFECT_NONE,                       /*!< No effect plays. */
    SOUND_EFFECT_ENGINE = SOUND_EFFECT_NONE, /*!< The engine, also used for the airplane. */
    SOUND_EFFECT_VEHICLE_IMPACT,             /*!< A hit of another vehicle. */
    SOUND_EFFECT_IMPACT,                     /*!< A hit of the track. */
    SOUND_EFFECT_LANDING,                    /*!< A wheel landing on the ground. */
    SOUND_EFFECT_SKID_SURFACE_2,             /*!< A skid on surface 2. */
    SOUND_EFFECT_SKID_SURFACE_1,             /*!< A skid on surface 1. */
    SOUND_EFFECT_6,              /*!< A skid sound the reconstructed code does not choose. */
    SOUND_EFFECT_SKID_SURFACE_0, /*!< A skid on surfaces 0 and 3, and on every surface in snow. */
    SOUND_EFFECT_ROLL_SURFACE_1, /*!< The rolling on surface 1. */
    SOUND_EFFECT_ROLL_SURFACE_2, /*!< The rolling on surface 2. */
    SOUND_EFFECT_ROLL_SURFACE_0, /*!< The rolling on surfaces 0 and 3, and on the slow surface
                                      in snow. */
    SOUND_EFFECT_ROLL_SNOW,      /*!< The rolling on surfaces 0 to 3 in snow. */
    SOUND_EFFECT_SLOW_SURFACE,   /*!< The skid and the rolling on the slow surface. */
    SOUND_EFFECT_BEEP,           /*!< A beep of the countdown. */
    SOUND_EFFECT_GO,             /*!< The tone of the countdown that starts the race. */
    SOUND_EFFECT_WATERFALL,      /*!< The waterfall of the jungle track. */
};

/** The first skid sound. */
#define SOUND_EFFECT_FIRST_SKID SOUND_EFFECT_SKID_SURFACE_2
/** The first rolling sound. */
#define SOUND_EFFECT_FIRST_ROLL SOUND_EFFECT_ROLL_SURFACE_1
/** The end of the rolling sounds. */
#define SOUND_EFFECT_ROLL_END (SOUND_EFFECT_SLOW_SURFACE + 1)

/** A sound effect of a race, as LoadAllSoundEffects() loads it. */
typedef struct {
    s8 *sample;     /*!< The samples, signed 8-bit, followed by guardLength samples of its start. */
    u32 romStart;   /*!< The cartridge address of the compressed sound. */
    u32 romEnd;     /*!< The cartridge address of the end of the compressed sound. */
    u32 length;     /*!< The samples of the decompressed sound, one byte each. */
    s32 loopLength; /*!< The samples a loop repeats, or -1 for the whole sound. */
    s32 guardLength; /*!< The samples of the start copied after the end, or -1 for the whole
                          sound. */
} SoundEffect;

/**
 * The sound effects of a race.
 *
 * @ghidraAddress USA: 0x8028bc04
 * @ghidraAddress JPN: 0x8028dd9c
 * @ghidraAddress EUR: 0x8028e36c
 */
extern SoundEffect g_aSoundEffects[SOUND_EFFECTS];

/**
 * The sound effect channel 3 plays once, or -1.
 *
 * @ghidraAddress USA: 0x8028b7ec
 * @ghidraAddress JPN: 0x8028d4cc
 * @ghidraAddress EUR: 0x8028da9c
 */
extern s32 g_dwChannel3PlayingSfxId;

/** The nearest sound of the track this frame and the last. */
typedef struct {
    Vec3f position;                 /*!< Where the nearest sound is. */
    Vec3f previousPosition;         /*!< Where the sound was in the last frame. */
    CameraFrame *listener;          /*!< The camera that hears the sound. */
    CameraFrame *previousListener;  /*!< The camera that heard the sound in the last frame. */
    Vec3f previousListenerPosition; /*!< Where the camera was in the last frame. */
    s32 sound;                      /*!< The sound effect, or -1. */
    s32 previousSound;              /*!< The sound effect of the last frame, or -1. */
    s32 source;                     /*!< The source of the sound, or -1. */
    s32 previousSource;             /*!< The source of the last frame, or -1. */
    f32 distance;                   /*!< How far the sound is from its camera. */
    f32 frequency; /*!< The frequency in hertz, negative for a source that does not move. */
    s32 volume;    /*!< The volume out of 256. */
    s32 playing;   /*!< Whether the sound plays. */
} SoundSourceTracking;

/**
 * The nearest sound of the track.
 *
 * @ghidraAddress USA: 0x8028b7a0
 * @ghidraAddress JPN: 0x8028d480
 * @ghidraAddress EUR: 0x8028da50
 */
extern SoundSourceTracking g_ClosestSoundSource;

/**
 * The factor the Doppler effect raises a sound by, from the motion of the source and the listener
 * along the line between them over the frame.
 *
 * @param source The source.
 * @param previousSource Where the source was.
 * @param listener The listener.
 * @param previousListener Where the listener was.
 * @return The factor.
 * @ghidraAddress USA: 0x8022b0f8
 * @ghidraAddress JPN: 0x8022bdc0
 * @ghidraAddress EUR: 0x8022c608
 */
f32 ComputeDopplerFactor(Vec3f *source,
                         Vec3f *previousSource,
                         Vec3f *listener,
                         Vec3f *previousListener);

/**
 * Pan a sound between the two halves of the volume of a channel by where it is across a camera, and
 * set its volume by its distance.
 *
 * @param source The source.
 * @param camera The camera that hears it.
 * @param upper The share of the upper half of the volume.
 * @param lower The share of the lower half.
 * @param volume The volume, 1024 over the distance, from at least 32.
 * @param narrow Whether to pan only a little.
 * @ghidraAddress USA: 0x8022b1d4
 * @ghidraAddress JPN: 0x8022be9c
 * @ghidraAddress EUR: 0x8022c6e4
 */
void ComputeStereoPan(
    Vec3f *source, CameraFrame *camera, f32 *upper, f32 *lower, s32 *volume, s32 narrow);

/**
 * Play a sound effect once on channel 3.
 *
 * @param sound The sound effect.
 * @ghidraAddress USA: 0x8022b370
 * @ghidraAddress JPN: 0x8022c038
 * @ghidraAddress EUR: 0x8022c880
 */
void PlaySoundEffect(s32 sound);

/**
 * Offer a sound of the track as the nearest this frame; the nearest plays.
 *
 * @param source The source the sound is kept under.
 * @param sound The sound effect.
 * @param volume The volume, out of 256.
 * @param frequency The frequency, in hertz, negative for a source that does not move.
 * @param position The position of the source.
 * @param listener The camera that hears it.
 * @ghidraAddress USA: 0x8022b494
 * @ghidraAddress JPN: 0x8022c15c
 * @ghidraAddress EUR: 0x8022c9a4
 */
void UpdateClosestSoundSource(
    s32 source, s32 sound, s32 volume, f32 frequency, Vec3f *position, CameraFrame *listener);

/**
 * Play the beep of the countdown, and of the laps.
 *
 * @ghidraAddress USA: 0x8022b3c4
 * @ghidraAddress JPN: 0x8022c08c
 * @ghidraAddress EUR: 0x8022c8d4
 */
void PlayBeepSound(void);

/**
 * Play the sound of the start of the race.
 *
 * @ghidraAddress USA: 0x8022b3e4
 * @ghidraAddress JPN: 0x8022c0ac
 * @ghidraAddress EUR: 0x8022c8f4
 */
void PlayGoSound(void);

/**
 * Forget the nearest sound of the track found the last frame.
 *
 * @ghidraAddress USA: 0x8022b404
 * @ghidraAddress JPN: 0x8022c0cc
 * @ghidraAddress EUR: 0x8022c914
 */
void ResetSoundSourceTracking(void);

/**
 * Stop the sounds of the track and forget their sources.
 *
 * @ghidraAddress USA: 0x8022b428
 * @ghidraAddress JPN: 0x8022c0f0
 * @ghidraAddress EUR: 0x8022c938
 */
void ResetAllSoundState(void);

/**
 * Play the nearest sound of the track on its channel, panned and pitched for the camera.
 *
 * @ghidraAddress USA: 0x8022b534
 * @ghidraAddress JPN: 0x8022c1fc
 * @ghidraAddress EUR: 0x8022ca44
 */
void UpdateAmbientSoundChannel(void);

/**
 * Offer the sound of the airplane as the nearest sound of the track, on the tracks that have one.
 *
 * @param instance The placed model of the airplane.
 * @param position Its position.
 * @param camera The camera that hears it.
 * @ghidraAddress USA: 0x8022b954
 * @ghidraAddress JPN: 0x8022c61c
 * @ghidraAddress EUR: 0x8022ce64
 */
void UpdateAirplaneSound(s32 instance, Vec3f *position, CameraFrame *camera);

/**
 * Offer the sound of a waterfall as the nearest sound of the track, on the tracks that have one.
 *
 * @param instance The source the sound is kept under.
 * @param position The position of the waterfall.
 * @param camera The camera that hears it.
 * @ghidraAddress USA: 0x8022ba10
 * @ghidraAddress JPN: 0x8022c6d8
 * @ghidraAddress EUR: 0x8022cf20
 */
void UpdateWaterfallSound(s32 instance, Vec3f *position, CameraFrame *camera);

/**
 * Do nothing with a camera.
 *
 * @param camera The camera.
 * @ghidraAddress USA: 0x8022ba98
 * @ghidraAddress JPN: 0x8022c760
 * @ghidraAddress EUR: 0x8022cfa8
 */
void StubSoundUpdate(CameraFrame *camera);

/**
 * Load the sound effects of a race and start their channels muted.
 *
 * @ghidraAddress USA: 0x8022baa0
 * @ghidraAddress JPN: 0x8022c768
 * @ghidraAddress EUR: 0x8022cfb0
 */
void LoadAllSoundEffects(void);

/**
 * Play the engine, the impacts and the rumble of a vehicle for a frame.
 *
 * @param vehicle The vehicle.
 * @ghidraAddress USA: 0x8022bcb4
 * @ghidraAddress JPN: 0x8022c97c
 * @ghidraAddress EUR: 0x8022d1c4
 */
void UpdateVehicleSounds(Vehicle *vehicle);

#endif
