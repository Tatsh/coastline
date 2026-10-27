#ifndef COASTLINE_EAR_VOLUME_H
#define COASTLINE_EAR_VOLUME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x10012420 */
int32_t __stdcall ConvertPercentGainToDirectSoundVolume(double gain);

/** @ghidraAddress EAR: 0x10036148 */
extern double g_dMasterAttenuationLevel;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_VOLUME_H */
