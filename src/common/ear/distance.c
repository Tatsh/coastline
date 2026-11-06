#include "distance.h"

#include <math.h>
#include <string.h>

#include "error.h"

double g_adDistanceUnitToCentimeters[4] = {1, 1, 1, 1};

// @ghidraAddress EAR: 0x1002ec50
static const double g_earUserDistanceUnitScale = 100;

/** The greatest gain of an event. */
#define EAR_MAXIMUM_GAIN 10000

static inline int32_t TruncateDistance(long double value) {
    // Invalid x87 integer conversions produce zero in the low 32 bits.
    if (!(value >= -0x1p63L && value < 0x1p63L)) {
        return 0;
    }
    uint32_t bits = (uint32_t)(int64_t)value;
    int32_t result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

int32_t __stdcall CalculateRoundedPlanarDistance(const EarEventParams *params) {
    return TruncateDistance(ceil(hypot((double)params->positionX, (double)params->positionZ)));
}

static inline uint16_t CalculateDistanceFallOff(uint32_t distance, const EarEventParams *params) {
    long double travelled = (long double)distance - (long double)params->minimumDistance;
    long double rate = (long double)(int32_t)(params->maximumGain - params->minimumGain) /
                       (long double)(params->maximumDistance - params->minimumDistance);
    return (uint16_t)TruncateDistance(travelled * rate);
}

uint16_t __stdcall CalculateDistanceAttenuation(uint32_t distance, EarEventParams *params) {
    int32_t gain;
    params->maximumGain =
        params->maximumGain < EAR_MAXIMUM_GAIN ? params->maximumGain : EAR_MAXIMUM_GAIN;
    if (!(params->flags & EAR_EVENT_DISTANCE_ATTENUATION)) {
        return params->maximumGain;
    }
    if (distance <= params->minimumDistance) {
        return params->maximumGain;
    }
    if (distance >= params->maximumDistance) {
        return params->minimumGain;
    }
    // The fall-off is computed twice, once for the test and once for the gain.
    if (params->maximumGain - CalculateDistanceFallOff(distance, params) >= EAR_MAXIMUM_GAIN) {
        gain = EAR_MAXIMUM_GAIN;
    } else {
        gain = params->maximumGain - CalculateDistanceFallOff(distance, params);
    }
    return (uint16_t)gain;
}

void __stdcall ConvertPlayDistancesToCentimeters(EarEventParams *params) {
    params->positionX = TruncateDistance((long double)params->positionX *
                                         g_adDistanceUnitToCentimeters[params->distanceUnit]);
    params->positionY = TruncateDistance((long double)params->positionY *
                                         g_adDistanceUnitToCentimeters[params->distanceUnit]);
    params->positionZ = TruncateDistance((long double)params->positionZ *
                                         g_adDistanceUnitToCentimeters[params->distanceUnit]);
    params->minimumDistance = (uint32_t)TruncateDistance(
        (long double)params->minimumDistance * g_adDistanceUnitToCentimeters[params->distanceUnit]);
    params->maximumDistance = (uint32_t)TruncateDistance(
        (long double)params->maximumDistance * g_adDistanceUnitToCentimeters[params->distanceUnit]);
}

void __stdcall EarSetUserDistanceUnitExportWrapper(uint32_t unitIndex, uint32_t value) {
    EAR_DLL_SetUserDistanceUnit(unitIndex, value);
}

void __stdcall EAR_DLL_SetUserDistanceUnit(uint32_t unitIndex, uint32_t value) {
    SetEarLastError(0);
    if (unitIndex <= 3) {
        g_adDistanceUnitToCentimeters[unitIndex] =
            (double)((long double)value / g_earUserDistanceUnitScale);
    } else {
        SetEarLastError(0x24);
    }
}
