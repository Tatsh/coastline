#include "common/volumecontrols-internal.h"

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
