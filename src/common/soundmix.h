#ifndef COASTLINE_SOUND_MIX_H
#define COASTLINE_SOUND_MIX_H

/**
 * @file
 * The mixing rate and the frequency-step and packed-volume conversions of the sound mixer.
 */

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SAMPLE_RATE 11000.0f  /*!< The mixing rate, in samples per second. */
#define STEP_ONE 4294967296.0 /*!< One unit in the 32.32 sample-position format. */

#if TGR_BOSS_RALLY
/** The precision used for Boss Rally sound-mixing calculations. */
typedef long double SoundReal;
#else
/** The precision used for Nintendo 64 sound-mixing calculations. */
typedef f32 SoundReal;
#endif

/**
 * Convert a frequency to the step of the sample position for each mixed sample. In Boss Rally, a
 * step outside the signed 64-bit range becomes 1 shifted left by 63.
 *
 * @param frequency The frequency, in samples per second.
 * @return The 32.32 sample-position step for @p frequency.
 */
static inline u64 FrequencyStep(SoundReal frequency) {
#if TGR_BOSS_RALLY
    long double scaled = frequency * (1.0f / SAMPLE_RATE) * STEP_ONE;
    return scaled >= -0x1p63L && scaled < 0x1p63L ? (u64)(s64)scaled : ((u64)1 << 63);
#else
    return (u64)((double)(frequency * (1.0f / SAMPLE_RATE)) * STEP_ONE);
#endif
}

/**
 * Scale a volume by the share of the upper half and shift the result to bits 16 and above.
 *
 * @param volume The volume of the channel.
 * @param upper The share of the volume in the upper half.
 * @return The upper half of a channel's packed volume.
 */
static inline f32 UpperVolume(s32 volume, f32 upper) {
    return (f32)((s32)((f32)volume * upper) << 16);
}

/**
 * Pack a volume scaled by two shares into one word, the upper share in bits 16 and above and the
 * lower share below them. In Boss Rally, a product outside the signed 64-bit range packs as 0.
 *
 * @param volume The volume of the channel.
 * @param upper The share of the volume in the upper half.
 * @param lower The share of the volume in the lower half.
 * @return The packed volume panned between its upper and lower halves.
 */
static inline u32 PackVolume(s32 volume, f32 upper, f32 lower) {
#if TGR_BOSS_RALLY
    long double upperProduct = (long double)volume * upper;
    long double lowerProduct = (long double)volume * lower;
    u32 upperWord = upperProduct >= -0x1p63L && upperProduct < 0x1p63L ? (u32)(s64)upperProduct : 0;
    u32 lowerWord = lowerProduct >= -0x1p63L && lowerProduct < 0x1p63L ? (u32)(s64)lowerProduct : 0;
    return (upperWord << 16) + lowerWord;
#else
    return (u32)(UpperVolume(volume, upper) + lower * (f32)volume);
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SOUND_MIX_H */
