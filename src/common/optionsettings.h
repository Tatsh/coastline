#ifndef COASTLINE_OPTIONSETTINGS_H
#define COASTLINE_OPTIONSETTINGS_H

/**
 * @file
 * The settings of the options menu of the Nintendo 64 releases: the volumes of the music and of the
 * sound effects, and the units of speed.
 */

#include <PR/ultratypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The levels of each volume, from 0 for silence. */
#define VOLUME_LEVELS 11

/** The settings of the options menu, which the configuration on a Controller Pak keeps. */
typedef struct {
    s32 musicVolume; /*!< The level of the music, from 0 to #VOLUME_LEVELS - 1. */
    s32 sfxVolume;   /*!< The level of the sound effects, from 0 to #VOLUME_LEVELS - 1. */
    s32 speedInMph;  /*!< Whether speeds show in miles an hour rather than kilometres. */
} OptionSettings;

/**
 * @ghidraAddress USA: 0x802723d0
 * @ghidraAddress EUR: 0x802743b0
 * @ghidraAddress JPN: 0x80273e60
 */
extern OptionSettings g_OptionSettings;

#if !TGR_BOSS_RALLY
/**
 * The volume of the music and of the effects at each level.
 *
 * @ghidraAddress USA: 0x80272488
 * @ghidraAddress EUR: 0x80274438
 * @ghidraAddress JPN: 0x80273ee8
 */
extern s32 g_adwBgmVolumeLevels[VOLUME_LEVELS];

/**
 * @ghidraAddress USA: 0x802724b4
 * @ghidraAddress EUR: 0x80274464
 * @ghidraAddress JPN: 0x80273f14
 */
extern s32 g_adwSfxVolumeLevels[VOLUME_LEVELS];
#endif /* !TGR_BOSS_RALLY */

/**
 * Apply the music and effect volumes of the options.
 *
 * @ghidraAddress USA: 0x802118b4
 * @ghidraAddress EUR: 0x802117a4
 * @ghidraAddress JPN: 0x80211694
 */
void ApplyVolumeSettings(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_OPTIONSETTINGS_H */
