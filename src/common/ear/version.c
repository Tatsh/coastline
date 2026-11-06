#include "version.h"

#include <math.h>

#include "ear/backend.h"
#include "ear/configuration.h"
#include "error.h"

// @ghidraAddress EAR: 0x1002ebe0
static const double g_earVersionUnity = 1;
// @ghidraAddress EAR: 0x1002ebe8
static const double g_earVersionMinorScale = 100;
// @ghidraAddress EAR: 0x1002edc8
static const double g_earLibraryVersion = 1.54;
// @ghidraAddress EAR: 0x1002edd0
static const uint16_t g_earLibraryBuildNumber = 0x1096;

uint32_t __stdcall EarGetVersionExportWrapper(void) {
    return EAR_DLL_GetVersion();
}

uint32_t __stdcall EAR_DLL_GetVersion(void) {
    SetEarLastError(0);
    uint16_t version = (uint16_t)((int64_t)((long double)g_earLibraryVersion / g_earVersionUnity));
    version = (uint16_t)(version << 8);
    uint16_t minor = (uint16_t)((int64_t)((long double)(fmod(g_earLibraryVersion, 1.0)) *
                                          g_earVersionMinorScale));
    version |= minor;
    if (g_earAudioComponentsValidated) {
        // The backend result does not alter the reported library version.
        GetDirectSoundVersion();
    }
    return ((uint32_t)g_earLibraryBuildNumber << 16) | version;
}
