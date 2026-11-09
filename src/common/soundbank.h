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
