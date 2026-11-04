#ifndef COASTLINE_AUDIOMIX_H
#define COASTLINE_AUDIOMIX_H

/**
 * @file
 * The hand-written routines of audiomix.s: copies of 64-bit words and the sample mixers, and the
 * channel records they read.
 */

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The bytes of the stereo ring buffer the mixers write. */
#define AUDIO_RING_BYTES 0x4000

/**
 * A channel of a mixer. The mixer reads the sample address and its fraction as one 32.32 fixed
 * point position.
 */
typedef struct {
#if TGR_PLATFORM == TGR_PLATFORM_N64
    union {
        s8 *sample;                  /*!< The next signed 8-bit sample. */
        unsigned long sampleAddress; /*!< The whole part of the 32.32 position. */
    };
#else
    s8 *sample; /*!< The next sample, signed 8-bit, and the whole part of the position. */
#endif
    u32 fraction; /*!< The fraction of the position past sample, the low half of the 32.32 value. */
    u64 step;     /*!< The 32.32 step added to the position before each sample, or 0 when silent. */
    s32 mixVolume; /*!< The volume the mixer multiplies each sample by. */
    s32 volume;    /*!< The volume the game sets, which the mixing thread scales into mixVolume. */
} MixerChannel;

/** The channels of the music mixer. */
#define MIXER_CHANNELS 16

/** The channels of the sound effect mixer. */
#define SOUND_EFFECT_CHANNELS 6

#if TGR_PLATFORM == TGR_PLATFORM_SDL
/** Mixer channels and write position for one SDL audio context. */
typedef struct {
    u16 writePosition; /*!< The byte offset of the next write within the stereo ring buffer. */
    MixerChannel musicChannels[MIXER_CHANNELS]; /*!< The music mixer's sample channels. */
    u64 soundEffectSampleMask; /*!< The mask that clears low bits during packed effect mixing. */
    MixerChannel soundEffectChannels[SOUND_EFFECT_CHANNELS]; /*!< The sound effect channels. */
    s32 soundEffectEchoOffset; /*!< The sample offset from channel zero to its echo, or zero. */
} SdlMixerState;

/**
 * @pre An N64MixerContext is active.
 * @return The mixer storage, valid until the context is destroyed.
 */
SdlMixerState *GetSdlMixerState(void);

/** @copydoc SdlMixerState::writePosition */
#define g_wAudioBufferWritePos (GetSdlMixerState()->writePosition)
/** @copydoc SdlMixerState::musicChannels */
#define g_aMixerChannels (GetSdlMixerState()->musicChannels)
/** @copydoc SdlMixerState::soundEffectSampleMask */
#define g_qwSoundEffectSampleMask (GetSdlMixerState()->soundEffectSampleMask)
/** @copydoc SdlMixerState::soundEffectChannels */
#define g_aSoundEffectChannels (GetSdlMixerState()->soundEffectChannels)
/** @copydoc SdlMixerState::soundEffectEchoOffset */
#define g_dwSoundEffectEchoOffset (GetSdlMixerState()->soundEffectEchoOffset)
#else
/**
 * The write position in the stereo ring buffer of the music mixers.
 *
 * @ghidraAddress USA: 0x802a4790
 * @ghidraAddress EUR: 0x802a6f50
 * @ghidraAddress JPN: 0x802a69f0
 */
extern u16 g_wAudioBufferWritePos;

/**
 * The channels of the music mixers, 8 bytes after the write position.
 *
 * @ghidraAddress USA: 0x802a4798
 * @ghidraAddress EUR: 0x802a6f58
 * @ghidraAddress JPN: 0x802a69f8
 */
extern MixerChannel g_aMixerChannels[MIXER_CHANNELS];

#if TGR_PLATFORM == TGR_PLATFORM_N64
/**
 * Upper word of the mask that clears low bits during packed sound effect mixing.
 *
 * @ghidraAddress USA: 0x802a4918
 * @ghidraAddress EUR: 0x802a70d8
 * @ghidraAddress JPN: 0x802a6b78
 */
extern s32 g_nSoundEffectSampleMaskUpper;

/**
 * Lower word of the mask that clears low bits during packed sound effect mixing.
 *
 * @ghidraAddress USA: 0x802a491c
 * @ghidraAddress EUR: 0x802a70dc
 * @ghidraAddress JPN: 0x802a6b7c
 */
extern u32 g_dwSoundEffectSampleMaskLower;
#else
/** The mask that clears low bits during packed sound effect mixing. */
extern u64 g_qwSoundEffectSampleMask;
#endif

/**
 * The channels of the sound effects, 8 bytes after the mask. The volume of each holds the levels of
 * the right and the left in its upper and lower halves.
 *
 * @ghidraAddress USA: 0x802a4920
 * @ghidraAddress EUR: 0x802a70e0
 * @ghidraAddress JPN: 0x802a6b80
 */
extern MixerChannel g_aSoundEffectChannels[SOUND_EFFECT_CHANNELS];

/**
 * The distance back to the echo of sound effect channel 0, or 0 for none.
 *
 * @ghidraAddress USA: 0x802a49b0
 * @ghidraAddress EUR: 0x802a7170
 * @ghidraAddress JPN: 0x802a6c10
 */
extern s32 g_dwSoundEffectEchoOffset;
#endif

/**
 * Copy 16 bytes between 8-byte aligned addresses.
 *
 * @param dst The destination.
 * @param src The source.
 * @ghidraAddress USA: 0x80256270
 * @ghidraAddress EUR: 0x80257840
 * @ghidraAddress JPN: 0x802571d0
 */
void CopyVec4(void *dst, const void *src);

/**
 * Fill a byte count rounded up to 64 with a 32-bit value. The address must be 8-byte aligned.
 *
 * @param dst The start of the memory to fill.
 * @param size The count of bytes to fill.
 * @param value The value of each word. A value of -1 sets every bit, and a positive value leaves
 * the upper halfword of each word clear.
 * @ghidraAddress USA: 0x80256290
 * @ghidraAddress EUR: 0x80257860
 * @ghidraAddress JPN: 0x802571f0
 */
void MemFill64Aligned(void *dst, u32 size, s32 value);

/**
 * Mix six channels into the ring buffer, replacing what it held.
 *
 * @param ring The start of the ring buffer.
 * @param bytes The count of bytes to mix from the write position.
 * @ghidraAddress USA: 0x802562f0
 * @ghidraAddress EUR: 0x802578c0
 * @ghidraAddress JPN: 0x80257250
 */
void MixAudioSamples(s16 *ring, u32 bytes);

/**
 * Add a pair of channels to the ring buffer.
 *
 * @param ring The start of the ring buffer.
 * @param bytes The count of bytes to mix from the write position.
 * @param pairOffset Offset of the pair's records from those of the first extra pair.
 * @ghidraAddress USA: 0x8025649c
 * @ghidraAddress EUR: 0x80257a6c
 * @ghidraAddress JPN: 0x802573fc
 */
void MixAudioStereoSamples(s16 *ring, u32 bytes, u32 pairOffset);

/**
 * Add the sound effect channels to the ring buffer from an offset.
 *
 * @param ring The start of the ring buffer.
 * @param bytes The count of bytes to mix.
 * @param offset The offset in bytes into the ring buffer to start at.
 * @ghidraAddress USA: 0x8025658c
 * @ghidraAddress EUR: 0x80257b5c
 * @ghidraAddress JPN: 0x802574ec
 */
void MixSoundEffectChannels(s16 *ring, u32 bytes, u32 offset);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_AUDIOMIX_H */
