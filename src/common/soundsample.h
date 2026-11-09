#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The number of sound channels. */
#define SOUND_CHANNEL_COUNT 15
/** The number of sound sample rows, including variant rows. */
#define SOUND_SAMPLE_COUNT 26

struct SoundBufferNode;

/** A channel word before and after sound buffers load. */
typedef union {
    uint32_t initialValue;          /*!< The channel's value in the initial table. */
    struct SoundBufferNode *buffer; /*!< The loaded sound buffer. */
} SoundChannelSlot;

/** The loaded buffers and frequency multiplier of one sound sample. */
typedef struct {
    SoundChannelSlot channelBuffers[SOUND_CHANNEL_COUNT]; /*!< The sample's channel buffers. */
    double dFrequencyScale;                               /*!< The sample's frequency multiplier. */
} SoundSample;

/**
 * Initial channel words become buffer pointers when samples load.
 *
 * @ghidraAddress BRD3D: 0x100b5df0
 * @ghidraAddress BRGlide: 0x100b55f8
 */
extern SoundSample g_aSoundChannelBuffers[SOUND_SAMPLE_COUNT];

/**
 * Sound buffers loaded from h.wav variants.
 *
 * @ghidraAddress BRD3D: 0x100b64b0
 * @ghidraAddress BRGlide: 0x100b5cb8
 */
#define g_SfxWaveVariantTable (g_aSoundChannelBuffers[24].channelBuffers)

/**
 * Sound buffers loaded from r.wav variants.
 *
 * @ghidraAddress BRD3D: 0x100b64f8
 * @ghidraAddress BRGlide: 0x100b5d00
 */
#define g_LoopingSoundTable (g_aSoundChannelBuffers[25].channelBuffers)

#ifdef __cplusplus
}
#endif
