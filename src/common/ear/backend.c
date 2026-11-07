#include "ear/backend.h"

#include "ear/error.h"

// @ghidraAddress EAR: 0x1003b7d0
#if defined(_MSC_VER)
static uint32_t g_dwEarReservedInitializationOption;
#else
[[maybe_unused, gnu::used]] static uint32_t g_dwEarReservedInitializationOption;
#endif

int __stdcall ValidateEarStereoSolution(uint32_t enabled) {
    if (enabled) {
        SetEarLastError(0x20);
        return 0;
    }
    return 1;
}

int __stdcall SetReservedEarInitializationOption(uint32_t option) {
    g_dwEarReservedInitializationOption = option;
    return 1;
}

int RejectUnsupportedInitializationFeature4000(void) {
    return 0;
}

int RejectUnsupportedInitializationFeature2000(void) {
    return 0;
}

int __stdcall GetDirectSoundVersion(void) {
    return QueryDirectSoundProductVersion();
}
