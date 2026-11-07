#ifndef COASTLINE_VOLUME_CONTROLS_INTERNAL_H
#define COASTLINE_VOLUME_CONTROLS_INTERNAL_H

#include "common/volumecontrols.h"

#if TGR_BOSS_RALLY
#include "common/rallyconfig.h"
#include "common/soundsystem.h"
#else
#include "common/audio.h"
#include "common/optionsettings.h"
#endif

#if TGR_BOSS_RALLY
#define BGM_LEVEL g_GlobalRallyConfig.dwMusicVolume
#define SFX_LEVEL g_GlobalRallyConfig.dwSfxVolume
#define EFFECT_VOLUME g_bSfxVolume
#define VOLUME_COUNT VOLUME_LEVEL_COUNT
#else
#define BGM_LEVEL g_OptionSettings.musicVolume
#define SFX_LEVEL g_OptionSettings.sfxVolume
#define EFFECT_VOLUME g_bSfxMasterVolume
#define VOLUME_COUNT VOLUME_LEVELS
#endif /* TGR_BOSS_RALLY */

#endif
