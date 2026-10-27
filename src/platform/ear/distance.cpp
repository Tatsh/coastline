#include "distance.h"

#include "error.h"

double g_adDistanceUnitToCentimeters[4] = {1, 1, 1, 1};

namespace {
// @ghidraAddress EAR: 0x1002ec50
const double g_earUserDistanceUnitScale = 100;
} // namespace

void __stdcall EarSetUserDistanceUnitExportWrapper(std::uint32_t unitIndex, std::uint32_t value) {
    EAR_DLL_SetUserDistanceUnit(unitIndex, value);
}

void __stdcall EAR_DLL_SetUserDistanceUnit(std::uint32_t unitIndex, std::uint32_t value) {
    SetEarLastError(0);
    if (unitIndex <= 3) {
        g_adDistanceUnitToCentimeters[unitIndex] =
            static_cast<double>(static_cast<long double>(value) / g_earUserDistanceUnitScale);
    } else {
        SetEarLastError(0x24);
    }
}
