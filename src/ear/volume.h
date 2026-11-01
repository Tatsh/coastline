#ifndef COASTLINE_EAR_VOLUME_H
#define COASTLINE_EAR_VOLUME_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Approximate percentage gain as DirectSound attenuation using the native quartic polynomial.
 * @param gain The percentage amplitude, normally 0..100; input and output are not clamped.
 * @return Attenuation in hundredths of a decibel, truncated to an integer's low 32 bits;
 * zero when the polynomial cannot be converted to a signed 64-bit integer.
 * @ghidraAddress EAR: 0x10012420
 */
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

/**
 * Linear master attenuation multiplier applied to PCM gain, initially one.
 * @ghidraAddress EAR: 0x10036148
 */
extern double g_dMasterAttenuationLevel;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_VOLUME_H */
