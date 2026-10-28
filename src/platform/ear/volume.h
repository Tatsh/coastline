#ifndef COASTLINE_EAR_VOLUME_H
#define COASTLINE_EAR_VOLUME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x10012420 */
int32_t __stdcall ConvertPercentGainToDirectSoundVolume(double gain);

/**
 * Convert a percentage gain to the DirectSound pan of the left side.
 *
 * @param gain The gain, as a percentage.
 * @return The volume of the gain, in hundredths of a decibel.
 * @ghidraAddress EAR: 0x10012400
 */
int32_t __stdcall ConvertPercentGainToLeftPan(double gain);

/**
 * Convert a percentage gain to the DirectSound pan of the right side.
 *
 * @param gain The gain, as a percentage.
 * @return The negated volume of the gain, in hundredths of a decibel.
 * @ghidraAddress EAR: 0x10012480
 */
int32_t __stdcall ConvertPercentGainToRightPan(double gain);

/** @ghidraAddress EAR: 0x10036148 */
extern double g_dMasterAttenuationLevel;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_VOLUME_H */
