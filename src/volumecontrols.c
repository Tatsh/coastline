#include "volumecontrols.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/sound.h"
#define BGM_LEVEL g_GlobalRallyConfig.dwMusicVolume
#define SFX_LEVEL g_GlobalRallyConfig.dwSfxVolume
#define EFFECT_VOLUME g_bSfxVolume
#define VOLUME_COUNT VOLUME_LEVEL_COUNT
#else
#include "audio.h"
#include "options.h"
#define BGM_LEVEL g_OptionSettings.musicVolume
#define SFX_LEVEL g_OptionSettings.sfxVolume
#define EFFECT_VOLUME g_bSfxMasterVolume
#define VOLUME_COUNT VOLUME_LEVELS
#endif

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
