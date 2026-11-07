#include "common/volumecontrols-internal.h"

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
