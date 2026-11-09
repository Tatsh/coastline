#pragma once

#include <stdint.h>

#include "common/soundsample.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A sound-bank row that maps game channels to samples. */
typedef struct {
    int32_t anChannels[SOUND_CHANNEL_COUNT]; /*!< The sample index of each sound channel. */
    double dFrequencyScale;                  /*!< The frequency multiplier for this row. */
} SoundBankRow;

/**
 * Sound-channel mappings and frequency scaling for each sample row.
 *
 * @ghidraAddress BRD3D: 0x100b6540
 * @ghidraAddress BRGlide: 0x100b5d48
 */
extern SoundBankRow g_SoundBankIndexTableA[SOUND_SAMPLE_COUNT];

/**
 * Sound-channel mappings for the h.wav sample variants.
 *
 * @ghidraAddress BRD3D: 0x100b6c00
 * @ghidraAddress BRGlide: 0x100b6408
 */
#define g_SoundBankIndexTableB (g_SoundBankIndexTableA[24])

/**
 * Sound-channel mappings for the r.wav sample variants.
 *
 * @ghidraAddress BRD3D: 0x100b6c48
 * @ghidraAddress BRGlide: 0x100b6450
 */
#define g_SoundBankIndexTableC (g_SoundBankIndexTableA[25])

#ifdef __cplusplus
}
#endif
