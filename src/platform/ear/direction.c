#include "direction.h"

#include <math.h>
#include <string.h>

// @ghidraAddress EAR: 0x1003b4cc
static uint16_t g_wDirectionalDistance;
// @ghidraAddress EAR: 0x1003b4d0
static uint16_t g_wDirectionalDistanceBand;
// @ghidraAddress EAR: 0x1003b3b0
static int32_t g_bDirectionalBandFound;

// @ghidraAddress EAR: 0x1002e6e8
static const double g_earDirectionalLifetimeScale = 10000;
// @ghidraAddress EAR: 0x1002e6f0
static const double g_earDirectionalUpperIndexBias = 4;
// @ghidraAddress EAR: 0x1002e6f8
static const double g_earDirectionalLowerIndexBias = 3;
// @ghidraAddress EAR: 0x1002e700
static const double g_earDirectionFullCircleRadians = 6.282;
// @ghidraAddress EAR: 0x1002e708
static const double g_earDirectionEighthCircleRadians = 0.78525;

static inline int32_t TruncateDirection(long double value) {
    // Invalid x87 integer conversions produce zero in the low 32 bits.
    if (!(value >= -0x1p63L && value < 0x1p63L)) {
        return 0;
    }
    uint32_t bits = (uint32_t)(int64_t)value;
    int32_t result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

uint16_t __stdcall CalculateDirectionalPresetIndex(uint32_t distance,
                                                   uint16_t directionCount,
                                                   const EarEventParams *params) {
    if (!(params->flags & EAR_EVENT_DIRECTIONAL)) {
        return params->direction;
    }
    switch (directionCount) {
    case 1:
        return 0;
    case 16:
        // A matrix of 16 directions is searched over 128 directions.
        return CalculateDirectionSector(0, 0x7f, params->positionX, params->positionZ);
    case 32:
    case 64:
        g_wDirectionalDistance = (uint16_t)TruncateDirection((long double)distance /
                                                             (long double)params->maximumDistance *
                                                             g_earDirectionalLifetimeScale);
        g_wDirectionalDistanceBand = 0;
        g_bDirectionalBandFound = 0;
        do {
            uint16_t band = params->distanceBands[g_wDirectionalDistanceBand];
            g_wDirectionalDistanceBand++;
            // The band past the last is read when the distance lies beyond every band.
            if (g_wDirectionalDistance >= band &&
                g_wDirectionalDistance < params->distanceBands[g_wDirectionalDistanceBand]) {
                g_bDirectionalBandFound = 1;
            }
        } while (g_wDirectionalDistanceBand < EAR_EVENT_DISTANCE_BAND_COUNT &&
                 !g_bDirectionalBandFound);
        if (g_wDirectionalDistanceBand > 1) {
            return CalculateDirectionSector(
                (uint16_t)TruncateDirection(pow(2.0, (double)g_wDirectionalDistanceBand) -
                                            g_earDirectionalLowerIndexBias),
                (uint16_t)TruncateDirection(pow(2.0, (double)(g_wDirectionalDistanceBand + 1)) -
                                            g_earDirectionalUpperIndexBias),
                params->positionX,
                params->positionZ);
        }
        return 0;
    default:
        return 0;
    }
}

uint16_t __stdcall CalculateDirectionSector(uint16_t first, uint16_t last, int32_t x, int32_t z) {
    uint16_t count = (uint16_t)(last - first + 1);
    uint16_t quadrant;
    double sectorAngle;
    double angle;
    int32_t sector;
    if (z > 0) {
        quadrant = (uint16_t)((x > 0) + 2);
    } else {
        quadrant = x >= 0 ? 4 : 1;
    }
    if (count == 4) {
        return quadrant;
    }
    sectorAngle = g_earDirectionFullCircleRadians / (double)count;
    if (quadrant == 2 || quadrant == 4) {
        angle = atan2(fabs((double)x), fabs((double)z));
    } else {
        angle = atan2(fabs((double)z), fabs((double)x));
    }
    sector = -TruncateDirection(floor((angle - g_earDirectionEighthCircleRadians) / sectorAngle));
    return (uint16_t)(((quadrant - 1) * (count / 4) + sector + count / 8 - 1) % count + first);
}
