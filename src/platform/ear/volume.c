#include "volume.h"

#include <string.h>

double g_dMasterAttenuationLevel = 1;

// @ghidraAddress EAR: 0x1002ee10
static const double g_dGainToVolumeQuarticCoefficient = 0.000002131;
// @ghidraAddress EAR: 0x1002ee18
static const double g_dGainToVolumeCubicCoefficient = -0.00058625;
// @ghidraAddress EAR: 0x1002ee20
static const double g_dGainToVolumeQuadraticCoefficient = 0.058645625;
// @ghidraAddress EAR: 0x1002ee28
static const double g_dGainToVolumeLinearCoefficient = -2.73303125;
// @ghidraAddress EAR: 0x1002ee30
static const double g_dGainToVolumeConstant = 60;
// @ghidraAddress EAR: 0x1002ee38
static const double g_dGainToVolumeScale = -100;

static inline int32_t TruncateVolume(long double value) {
    // Invalid x87 integer conversions produce zero in the low 32 bits.
    if (!(value >= -0x1p63L && value < 0x1p63L)) {
        return 0;
    }
    uint32_t bits = (uint32_t)(int64_t)value;
    int32_t result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

int32_t __stdcall ConvertPercentGainToDirectSoundVolume(double gain) {
    long double quartic =
        (long double)g_dGainToVolumeQuarticCoefficient * gain * gain * gain * gain;
    long double cubic = (long double)g_dGainToVolumeCubicCoefficient * gain * gain * gain;
    long double sum = quartic + cubic;
    sum += (long double)g_dGainToVolumeQuadraticCoefficient * gain * gain;
    sum += (long double)g_dGainToVolumeLinearCoefficient * gain;
    sum += g_dGainToVolumeConstant;
    return TruncateVolume(sum * g_dGainToVolumeScale);
}
