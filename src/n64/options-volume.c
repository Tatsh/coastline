#include "common/audio.h"
#include "common/optionsettings.h"
#include "options.h"

void ApplyVolumeSettings(void) {
    g_bBgmVolume = g_adwBgmVolumeLevels[g_OptionSettings.musicVolume];
    g_bSfxMasterVolume = g_adwSfxVolumeLevels[g_OptionSettings.sfxVolume];
}
