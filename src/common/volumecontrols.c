#include "common/volumecontrols.h"

#if TGR_BOSS_RALLY
#include "common/rallyconfig.h"
#include "win32/sound.h"
#else
#include "common/audio.h"
#include "options.h"
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

void IncreaseBgmVolume(void) {
    if (BGM_LEVEL < VOLUME_COUNT - 1) {
        BGM_LEVEL++;
    }
    g_bBgmVolume = g_adwBgmVolumeLevels[BGM_LEVEL];
}

void DecreaseBgmVolume(void) {
    if (BGM_LEVEL > 0) {
        BGM_LEVEL--;
    }
    g_bBgmVolume = g_adwBgmVolumeLevels[BGM_LEVEL];
}

void IncreaseSfxVolume(void) {
    if (SFX_LEVEL < VOLUME_COUNT - 1) {
        SFX_LEVEL++;
    }
    EFFECT_VOLUME = g_adwSfxVolumeLevels[SFX_LEVEL];
}

void DecreaseSfxVolume(void) {
    if (SFX_LEVEL > 0) {
        SFX_LEVEL--;
    }
    EFFECT_VOLUME = g_adwSfxVolumeLevels[SFX_LEVEL];
}
