#ifndef COASTLINE_SOUNDEFFECT_H
#define COASTLINE_SOUNDEFFECT_H

/**
 * @file
 * The sound effects of a race.
 */

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

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
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    s32 sample; /*!< The sample of the sound in DirectSound. */
#else
    s8 *sample; /*!< The samples, signed 8-bit, followed by guardLength samples of its start. */
#endif
    u32 romStart;    /*!< The cartridge address of the compressed sound. */
    u32 romEnd;      /*!< The cartridge address of the end of the compressed sound. */
    u32 length;      /*!< The samples of the decompressed sound, one byte each. */
    s32 loopLength;  /*!< The samples a loop repeats, or -1 for the whole sound. */
    s32 guardLength; /*!< The samples of the start copied after the end, or -1 for the whole
                          sound. */
} SoundEffect;

/**
 * The sound effects of a race.
 *
 * @ghidraAddress USA: 0x8028bc04
 * @ghidraAddress EUR: 0x8028e36c
 * @ghidraAddress JPN: 0x8028dd9c
 * @ghidraAddress BRD3D: 0x100b3aa8
 * @ghidraAddress BRGlide: 0x100b32b0
 */
extern SoundEffect g_aSoundEffects[SOUND_EFFECTS];

#ifdef __cplusplus
}
#endif

#endif
