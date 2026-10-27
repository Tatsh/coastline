#include "version.h"

#include <cmath>

#include "backend.h"
#include "error.h"

namespace {
// @ghidraAddress EAR: 0x1002ebe0
const double g_earVersionUnity = 1;
// @ghidraAddress EAR: 0x1002ebe8
const double g_earVersionMinorScale = 100;
// @ghidraAddress EAR: 0x1002edc8
const double g_earLibraryVersion = 1.54;
// @ghidraAddress EAR: 0x1002edd0
const std::uint16_t g_earLibraryBuildNumber = 0x1096;
} // namespace

std::uint32_t __stdcall EarGetVersionExportWrapper() {
    return EAR_DLL_GetVersion();
}

std::uint32_t __stdcall EAR_DLL_GetVersion() {
    SetEarLastError(0);
    std::uint16_t version = static_cast<std::uint16_t>(static_cast<std::int64_t>(
        static_cast<long double>(g_earLibraryVersion) / g_earVersionUnity));
    version = static_cast<std::uint16_t>(version << 8);
    std::uint16_t minor = static_cast<std::uint16_t>(static_cast<std::int64_t>(
        static_cast<long double>(std::fmod(g_earLibraryVersion, 1.0)) * g_earVersionMinorScale));
    version |= minor;
    if (g_earAudioComponentsValidated) {
        // The backend result does not alter the reported library version.
        GetDirectSoundVersion();
    }
    return (static_cast<std::uint32_t>(g_earLibraryBuildNumber) << 16) | version;
}
