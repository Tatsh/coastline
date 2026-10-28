#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The number of sound channels. */
#define SOUND_CHANNEL_COUNT 15
#define SOUND_SAMPLE_COUNT 26

struct SoundBufferNode;

typedef union {
    uint32_t initialValue;
    struct SoundBufferNode *buffer;
} SoundChannelSlot;

typedef struct {
    SoundChannelSlot channelBuffers[SOUND_CHANNEL_COUNT];
    char abUnknown3c[4]; /* +0x3c */
    double dFrequencyScale;
} SoundSample;

/**
 * Initial channel words become buffer pointers when samples load.
 *
 * @ghidraAddress BRD3D: 0x100b5df0
 * @ghidraAddress BRGlide: 0x100b55f8
 */
extern SoundSample g_aSoundChannelBuffers[SOUND_SAMPLE_COUNT];

/**
 * @ghidraAddress BRD3D: 0x100b64b0
 * @ghidraAddress BRGlide: 0x100b5cb8
 */
#define g_SfxWaveVariantTable (g_aSoundChannelBuffers[24].channelBuffers)

/**
 * @ghidraAddress BRD3D: 0x100b64f8
 * @ghidraAddress BRGlide: 0x100b5d00
 */
#define g_LoopingSoundTable (g_aSoundChannelBuffers[25].channelBuffers)

#ifdef __cplusplus
}
#endif
