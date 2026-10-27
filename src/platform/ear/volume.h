#ifndef COASTLINE_EAR_VOLUME_H
#define COASTLINE_EAR_VOLUME_H

#include <cstdint>

/** @ghidraAddress EAR: 0x10012420 */
std::int32_t __stdcall ConvertPercentGainToDirectSoundVolume(double gain);

/** @ghidraAddress EAR: 0x10036148 */
extern double g_dMasterAttenuationLevel;

#endif
