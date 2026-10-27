#ifndef COASTLINE_SOUND_MIX_H
#define COASTLINE_SOUND_MIX_H

#include "types.h"

#define SAMPLE_RATE 11000.0f
#define STEP_ONE 4294967296.0

#if TGR_PLATFORM == TGR_PLATFORM_WIN
typedef long double SoundReal;
#else
typedef f32 SoundReal;
#endif

/* The step of a channel that plays a frequency. */
static inline u64 FrequencyStep(SoundReal frequency) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double scaled = frequency * (1.0f / SAMPLE_RATE) * STEP_ONE;
    return scaled >= -0x1p63L && scaled < 0x1p63L ? (u64)(s64)scaled : ((u64)1 << 63);
#else
    return (u64)((double)(frequency * (1.0f / SAMPLE_RATE)) * STEP_ONE);
#endif
}

/* The upper half of the volume of a channel, before the lower is added. */
static inline f32 UpperVolume(s32 volume, f32 upper) {
    return (f32)((s32)((f32)volume * upper) << 16);
}

/* Pack a volume panned between the two halves of the volume of a channel. */
static inline u32 PackVolume(s32 volume, f32 upper, f32 lower) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    long double upperProduct = (long double)volume * upper;
    long double lowerProduct = (long double)volume * lower;
    u32 upperWord = upperProduct >= -0x1p63L && upperProduct < 0x1p63L ? (u32)(s64)upperProduct : 0;
    u32 lowerWord = lowerProduct >= -0x1p63L && lowerProduct < 0x1p63L ? (u32)(s64)lowerProduct : 0;
    return (upperWord << 16) + lowerWord;
#else
    return (u32)(UpperVolume(volume, upper) + lower * (f32)volume);
#endif
}

#endif
