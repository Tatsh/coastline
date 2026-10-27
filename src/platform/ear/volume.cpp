#include "volume.h"

#include <bit>

double g_dMasterAttenuationLevel = 1;

namespace {
// @ghidraAddress EAR: 0x1002ee10
const double g_dGainToVolumeQuarticCoefficient = 0.000002131;
// @ghidraAddress EAR: 0x1002ee18
const double g_dGainToVolumeCubicCoefficient = -0.00058625;
// @ghidraAddress EAR: 0x1002ee20
const double g_dGainToVolumeQuadraticCoefficient = 0.058645625;
// @ghidraAddress EAR: 0x1002ee28
const double g_dGainToVolumeLinearCoefficient = -2.73303125;
// @ghidraAddress EAR: 0x1002ee30
const double g_dGainToVolumeConstant = 60;
// @ghidraAddress EAR: 0x1002ee38
const double g_dGainToVolumeScale = -100;

static inline std::int32_t TruncateVolume(long double value) {
    // Invalid x87 integer conversions produce zero in the low 32 bits.
    if (!(value >= -0x1p63L && value < 0x1p63L)) {
        return 0;
    }
    return std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
} // namespace

std::int32_t __stdcall ConvertPercentGainToDirectSoundVolume(double gain) {
    long double quartic =
        static_cast<long double>(g_dGainToVolumeQuarticCoefficient) * gain * gain * gain * gain;
    long double cubic =
        static_cast<long double>(g_dGainToVolumeCubicCoefficient) * gain * gain * gain;
    long double sum = quartic + cubic;
    sum += static_cast<long double>(g_dGainToVolumeQuadraticCoefficient) * gain * gain;
    sum += static_cast<long double>(g_dGainToVolumeLinearCoefficient) * gain;
    sum += g_dGainToVolumeConstant;
    return TruncateVolume(sum * g_dGainToVolumeScale);
}
