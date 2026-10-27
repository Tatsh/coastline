#include "distance.h"

#include "error.h"

double g_adDistanceUnitToCentimeters[4] = {1, 1, 1, 1};

// @ghidraAddress EAR: 0x1002ec50
static const double g_earUserDistanceUnitScale = 100;

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
