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

#include "ambientsound.h"
#include "audiomix.h"
#include "render.h"
#include "soundchannel.h"
#include "soundeffect.h"
#include "soundpan.h"
#include "soundsource.h"
#include "vehicle.h"
#include "vehiclesound.h"

#ifdef __cplusplus
extern "C" {
#endif

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
 * @ghidraAddress EUR: 0x80025c00
 * @ghidraAddress JPN: 0x80025c00
 */
#define g_abMenuMusicModule ((u8 *)0x80025C00)

extern u8 _menuMusicSegmentRomStart[];
extern u8 _scrollSoundSegmentRomStart[];
extern u8 _selectSoundSegmentRomStart[];

/**
 * The work memory of the music player, at the start of the BSS.
 *
 * @ghidraAddress USA: 0x802ac400
 * @ghidraAddress EUR: 0x802ae0f0
 * @ghidraAddress JPN: 0x802acb30
 */
extern u8 g_abMusicModuleWorkBuffer[];

/**
 * The number of sound channels in use.
 *
 * @ghidraAddress USA: 0x802a49c0
 * @ghidraAddress EUR: 0x802a7180
 * @ghidraAddress JPN: 0x802a6c20
 */
extern s32 g_nAudioChannelCount;

/**
 * The volume of the music the game sets.
 *
 * @ghidraAddress USA: 0x802a49c4
 * @ghidraAddress EUR: 0x802a7184
 * @ghidraAddress JPN: 0x802a6c24
 */
extern u8 g_bBgmVolume;

/**
 * The music-volume factor, from 0 to 255.
 *
 * @ghidraAddress USA: 0x802a49c8
 * @ghidraAddress EUR: 0x802a7188
 * @ghidraAddress JPN: 0x802a6c28
 * @ghidraAddress BRD3D: 0x100bbadc
 * @ghidraAddress BRGlide: 0x100bb2e4
 */
extern u8 g_bMusicVolume;

/**
 * The volume of the sound effects the game sets.
 *
 * @ghidraAddress USA: 0x802a49cc
 * @ghidraAddress EUR: 0x802a718c
 * @ghidraAddress JPN: 0x802a6c2c
 */
extern u8 g_bSfxMasterVolume;

/**
 * The volume of the sound effects the player chose.
 *
 * @ghidraAddress USA: 0x802a49d0
 * @ghidraAddress EUR: 0x802a7190
 * @ghidraAddress JPN: 0x802a6c30
 */
extern u8 g_bSfxVolume;

/**
 * The frequencies of the twelve notes from middle C.
 *
 * @ghidraAddress USA: 0x802a49d4
 * @ghidraAddress EUR: 0x802a7194
 * @ghidraAddress JPN: 0x802a6c34
 */
extern f32 g_afNoteFrequencies[MUSIC_NOTES];

/**
 * Cleared when a pattern starts.
 *
 * @ghidraAddress USA: 0x802a4a04
 * @ghidraAddress EUR: 0x802a71c4
 * @ghidraAddress JPN: 0x802a6c64
 */
extern s32 g_dwMusicPatternRow;

/**
 * A silent sample that a channel at rest points at.
 *
 * @ghidraAddress USA: 0x802a4a08
 * @ghidraAddress EUR: 0x802a71c8
 * @ghidraAddress JPN: 0x802a6c68
 */
extern s8 g_SilenceSampleData[4];

/**
 * Toggled each message of the timer. The music ticks when it is set.
 *
 * @ghidraAddress USA: 0x802a4a0c
 * @ghidraAddress EUR: 0x802a71cc
 * @ghidraAddress JPN: 0x802a6c6c
 */
extern s32 g_dwAudioTickToggle;

/**
 * The place in the ring buffer the audio interface had reached.
 *
 * @ghidraAddress USA: 0x802a4a10
 * @ghidraAddress EUR: 0x802a71d0
 * @ghidraAddress JPN: 0x802a6c70
 */
extern s32 g_dwAiDmaPrevLength;

/**
 * Where in the ring buffer the sound effects mix next.
 *
 * @ghidraAddress USA: 0x802a4a14
 * @ghidraAddress EUR: 0x802a71d4
 * @ghidraAddress JPN: 0x802a6c74
 */
extern u32 g_dwSoundEffectMixOffset;

/**
 * The queue of the messages of the audio timer.
 *
 * @ghidraAddress USA: 0x803746f0
 * @ghidraAddress EUR: 0x80376bf0
 * @ghidraAddress JPN: 0x80375860
 */
extern OSMesgQueue g_AudioMsgQueue;

/**
 * @ghidraAddress USA: 0x80374708
 * @ghidraAddress EUR: 0x80376c08
 * @ghidraAddress JPN: 0x80375878
 */
extern OSMesg g_aAudioMsgBuf[2];

/**
 * The timer that wakes the mixing thread.
 *
 * @ghidraAddress USA: 0x80374710
 * @ghidraAddress EUR: 0x80376c10
 * @ghidraAddress JPN: 0x80375880
 */
extern OSTimer g_AudioTimer;

/**
 * The stereo ring buffer the audio interface plays.
 *
 * @ghidraAddress USA: 0x803747d0
 * @ghidraAddress EUR: 0x80376cd0
 * @ghidraAddress JPN: 0x80375940
 */
extern s16 g_AudioDmaBuffer[AUDIO_RING_BYTES / sizeof(s16)];

/**
 * The sample of each instrument, or NULL.
 *
 * @ghidraAddress USA: 0x803787d0
 * @ghidraAddress EUR: 0x8037acd0
 * @ghidraAddress JPN: 0x80379940
 */
extern XmSample *g_apInstrumentData[];

/**
 * Each pattern with its header.
 *
 * @ghidraAddress USA: 0x803789d0
 * @ghidraAddress EUR: 0x8037aed0
 * @ghidraAddress JPN: 0x80379b40
 */
extern u8 *g_apPatternData[];

/**
 * @ghidraAddress USA: 0x80378dd0
 * @ghidraAddress EUR: 0x8037b2d0
 * @ghidraAddress JPN: 0x80379f40
 */
extern MusicChannel g_MusicChannelStates[MIXER_CHANNELS];

/**
 * The samples of the sound effect channels.
 *
 * @ghidraAddress USA: 0x80378f50
 * @ghidraAddress EUR: 0x8037b450
 * @ghidraAddress JPN: 0x8037a0c0
 */
extern SoundEffectSample g_aSoundEffectSamples[SOUND_EFFECT_CHANNELS];

/**
 * Whether the music plays.
 *
 * @ghidraAddress USA: 0x80378f98
 * @ghidraAddress EUR: 0x8037b498
 * @ghidraAddress JPN: 0x8037a108
 */
extern s32 g_bMusicPlaybackEnabled;

/**
 * @ghidraAddress USA: 0x80378fa0
 * @ghidraAddress EUR: 0x8037b4a0
 * @ghidraAddress JPN: 0x8037a110
 */
extern MusicPlayer g_MusicPlayer;

/**
 * @ghidraAddress USA: 0x80378fb8
 * @ghidraAddress EUR: 0x8037b4b8
 * @ghidraAddress JPN: 0x8037a128
 */
extern OSThread g_AudioThread;

/** The stack of the mixing thread, in doublewords. */
#define AUDIO_THREAD_STACK_WORDS 0x80

/**
 * @ghidraAddress USA: 0x80379168
 * @ghidraAddress EUR: 0x8037b668
 * @ghidraAddress JPN: 0x8037a2d8
 */
extern u64 g_aAudioThreadStack[AUDIO_THREAD_STACK_WORDS];

/**
 * The 32.32 step of each note of each octave.
 *
 * @ghidraAddress USA: 0x80379568
 * @ghidraAddress EUR: 0x8037ba68
 * @ghidraAddress JPN: 0x8037a6d8
 */
extern u64 g_aFreqScaleTable[MUSIC_OCTAVES][MUSIC_NOTES];

/**
 * The bytes the mixers mix this time.
 *
 * @ghidraAddress USA: 0x80379930
 * @ghidraAddress EUR: 0x8037be30
 * @ghidraAddress JPN: 0x8037aaa0
 */
extern s32 g_dwAudioMixBytes;

/**
 * @ghidraAddress USA: 0x80379934
 * @ghidraAddress EUR: 0x8037be34
 * @ghidraAddress JPN: 0x8037aaa4
 */
extern s32 g_dwAiDmaCurrentLength;

/**
 * The channel the extra music mixers reached.
 *
 * @ghidraAddress USA: 0x80379938
 * @ghidraAddress EUR: 0x8037be38
 * @ghidraAddress JPN: 0x8037aaa8
 */
extern s32 g_dwAudioChannelLoopIdx;

/**
 * Copy the order table, the patterns and the samples of an XM module into the work memory, decoding
 * the samples and following each with the start of its loop or with silence.
 *
 * @param module The XM module.
 * @param work The memory the player reads the module from.
 * @ghidraAddress USA: 0x80256720
 * @ghidraAddress EUR: 0x80257cf0
 * @ghidraAddress JPN: 0x80257680
 */
void LoadMusicModule(u8 *module, u8 *work);

/**
 * Fill the steps of every note of every octave.
 *
 * @ghidraAddress USA: 0x80256c2c
 * @ghidraAddress EUR: 0x802581fc
 * @ghidraAddress JPN: 0x80257b8c
 */
void BuildFrequencyScaleTable(void);

/**
 * Loop or stop the music channels that passed the end of their samples.
 *
 * @ghidraAddress USA: 0x80256d3c
 * @ghidraAddress EUR: 0x8025830c
 * @ghidraAddress JPN: 0x80257c9c
 */
void CheckMusicLoopPoints(void);

/**
 * Read a row of a pattern for every channel and start its notes and effects.
 *
 * @param row The packed data of the row.
 * @return The data of the next row.
 * @ghidraAddress USA: 0x80256dec
 * @ghidraAddress EUR: 0x802583bc
 * @ghidraAddress JPN: 0x80257d4c
 */
u8 *ParseMusicPatternRow(u8 *row);

/**
 * Start the music from before its first order.
 *
 * @ghidraAddress USA: 0x802571ac
 * @ghidraAddress EUR: 0x8025877c
 * @ghidraAddress JPN: 0x8025810c
 */
void ResetMusicChannels(void);

/**
 * Run a tick of the music: read a row when one is due, and run the arpeggios, portamentos and
 * volume slides.
 *
 * @ghidraAddress USA: 0x8025721c
 * @ghidraAddress EUR: 0x802587ec
 * @ghidraAddress JPN: 0x8025817c
 */
void UpdateMusicPlayback(void);

/**
 * Start the audio interface, the music player and the mixing thread.
 *
 * @param module The XM module to play.
 * @param work The memory the player reads the module from.
 * @ghidraAddress USA: 0x802575c4
 * @ghidraAddress EUR: 0x80258b94
 * @ghidraAddress JPN: 0x80258524
 */
void InitAudioSystem(void *module, void *work);

/**
 * Stop the music and clear the state of every channel.
 *
 * @ghidraAddress USA: 0x802578f4
 * @ghidraAddress EUR: 0x80258ec4
 * @ghidraAddress JPN: 0x80258854
 */
void ResetMusicPlaybackState(void);

/**
 * Start playing an XM module.
 *
 * @param module The XM module to play.
 * @param work The memory the player reads the module from.
 * @ghidraAddress USA: 0x80257964
 * @ghidraAddress EUR: 0x80258f34
 * @ghidraAddress JPN: 0x802588c4
 */
void InitMusicPlayback(void *module, void *work);

/**
 * The first sound effect channel that is silent.
 *
 * @return The channel, or -1 if all play.
 * @ghidraAddress USA: 0x802579f4
 * @ghidraAddress EUR: 0x80258fc4
 * @ghidraAddress JPN: 0x80258954
 */
s16 FindFreeSoundChannel(void);

/**
 * Set the levels of a sound effect channel, the right in the upper half and the left in the lower,
 * each at most 32.
 *
 * @param channel The sound effect channel.
 * @param levels The right level in the upper 16 bits and the left in the lower 16 bits.
 * @ghidraAddress USA: 0x80257be4
 * @ghidraAddress EUR: 0x802591b4
 * @ghidraAddress JPN: 0x80258b44
 */
void SetSoundChannelVolume(s16 channel, u32 levels);

/**
 * Loop or stop the sound effect channels that passed the end of their samples.
 *
 * @ghidraAddress USA: 0x80257c44
 * @ghidraAddress EUR: 0x80259214
 * @ghidraAddress JPN: 0x80258ba4
 */
void CheckSoundEffectLoopPoints(void);

/**
 * The mixing thread: on each message of the timer, scale the volumes, mix what the audio interface
 * has played, tick the music, and feed the Rumble Paks.
 *
 * @param arg The argument of the thread, which is not read.
 * @ghidraAddress USA: 0x80257d3c
 * @ghidraAddress EUR: 0x8025930c
 * @ghidraAddress JPN: 0x80258c9c
 */
void AudioMixingThreadMain(void *arg);

/**
 * Do nothing with a camera.
 *
 * @param camera The camera.
 * @ghidraAddress USA: 0x8022ba98
 * @ghidraAddress EUR: 0x8022cfa8
 * @ghidraAddress JPN: 0x8022c760
 */
void StubSoundUpdate(CameraFrame *camera);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_AUDIO_H */
