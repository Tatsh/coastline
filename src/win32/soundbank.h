#pragma once

#include <stdint.h>

#include "common/soundsample.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t anChannels[SOUND_CHANNEL_COUNT];
    int32_t anUnknown3c[3]; /* +0x3c */
} SoundBankRow;

/**
 * @ghidraAddress BRD3D: 0x100b6540
 * @ghidraAddress BRGlide: 0x100b5d48
 */
extern SoundBankRow g_SoundBankIndexTableA[SOUND_SAMPLE_COUNT];

/**
 * @ghidraAddress BRD3D: 0x100b6c00
 * @ghidraAddress BRGlide: 0x100b6408
 */
#define g_SoundBankIndexTableB (g_SoundBankIndexTableA[24])

/**
 * @ghidraAddress BRD3D: 0x100b6c48
 * @ghidraAddress BRGlide: 0x100b6450
 */
#define g_SoundBankIndexTableC (g_SoundBankIndexTableA[25])

#ifdef __cplusplus
}
#endif
