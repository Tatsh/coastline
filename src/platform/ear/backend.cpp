#include "backend.h"

#include <cstdlib>
#include <cstring>

#include <windows.h>
#include <winver.h>

std::int32_t g_earAudioComponentsValidated;
std::uint16_t g_earAudioComponentValidationFlags;

namespace {
// @ghidraAddress EAR: 0x1003b9a0
const char g_earEmptyVersionPrefix[] = "";
// @ghidraAddress EAR: 0x1003722c
const char g_earVersionHexLeadingZero[] = "0";
// @ghidraAddress EAR: 0x10037250
const char g_earVersionResourcePathSeparator[] = "\\";
// @ghidraAddress EAR: 0x10037264
const char g_earVersionComponentSeparator[] = ".";

static inline void BuildProductVersionQuery(void *versionData, char *queryPath) {
    void *translationValue;
    UINT translationSize;
    char language[16];
    lstrcpyA(queryPath, "\\VarFileInfo\\Translation");
    if (VerQueryValueA(versionData, queryPath, &translationValue, &translationSize)) {
        DWORD translation;
        std::memcpy(&translation, translationValue, sizeof(translation));
        DWORD packed = (translation >> 16) | (translation << 16);
        std::strcpy(language, g_earEmptyVersionPrefix);
        if (packed < 0x10000000) {
            std::strcpy(language, g_earVersionHexLeadingZero);
        }
        char hexadecimal[9];
        _itoa(static_cast<int>(packed), hexadecimal, 16);
        std::strcat(language, hexadecimal);
    } else {
        std::strcpy(language, "040904B0");
    }
    lstrcpyA(queryPath, "\\StringFileInfo\\");
    lstrcatA(queryPath, language);
    lstrcatA(queryPath, g_earVersionResourcePathSeparator);
    lstrcatA(queryPath, "ProductVersion");
}

static inline void GetExpectedDirectSoundVersion(DWORD &versionMS, DWORD &versionLS) {
    versionMS = 0;
    versionLS = 0;
    OSVERSIONINFOA info;
    info.dwOSVersionInfoSize = sizeof(info);
    if (GetVersionExA(&info)) {
        if (info.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            versionMS = 0x00050000;
            versionLS = 0x074f0001;
        } else if (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && info.dwMajorVersion == 4) {
            if (info.dwMinorVersion == 10) {
                versionMS = 0x00040005;
                versionLS = 0x000107ce;
            } else if (info.dwMinorVersion == 0) {
                versionMS = 0x00040005;
                versionLS = 0x000000a5;
            }
        }
    }
}
} // namespace

int __stdcall GetDirectSoundVersion() {
    return QueryDirectSoundProductVersion();
}

int __stdcall QueryDirectSoundProductVersion() {
    char path[256];
    lstrcpyA(path, "c:\\windows\\system\\dsound.dll");
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeA(path, &handle);
    if (size == 0) {
        return FALSE;
    }
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, size);
    void *versionData = GlobalLock(memory);
    GetFileVersionInfoA(path, handle, size, versionData);
    char queryPath[256];
    BuildProductVersionQuery(versionData, queryPath);
    void *productVersionValue;
    UINT productVersionSize;
    BOOL result = VerQueryValueA(versionData, queryPath, &productVersionValue, &productVersionSize);
    if (result) {
        DWORD expectedMS;
        DWORD expectedLS;
        GetExpectedDirectSoundVersion(expectedMS, expectedLS);
        char delimiters[] = {g_earVersionComponentSeparator[0], g_earVersionComponentSeparator[1]};
        char *text = static_cast<char *>(productVersionValue);
        std::uint16_t major = static_cast<std::uint16_t>(std::atoi(std::strtok(text, delimiters)));
        std::uint16_t minor =
            static_cast<std::uint16_t>(std::atoi(std::strtok(nullptr, delimiters)));
        std::uint16_t build =
            static_cast<std::uint16_t>(std::atoi(std::strtok(nullptr, delimiters)));
        std::uint16_t revision =
            static_cast<std::uint16_t>(std::atoi(std::strtok(nullptr, delimiters)));
        std::uint16_t expectedMajor = static_cast<std::uint16_t>(expectedMS >> 16);
        std::uint16_t expectedMinor = static_cast<std::uint16_t>(expectedMS);
        std::uint16_t expectedBuild = static_cast<std::uint16_t>(expectedLS >> 16);
        std::uint16_t expectedRevision = static_cast<std::uint16_t>(expectedLS);
        if (major == expectedMajor && minor == expectedMinor && build == expectedBuild &&
            revision == expectedRevision) {
            result = TRUE;
        } else {
            // The original compares major and minor independently.
            if (major < expectedMajor || minor < expectedMinor) {
                g_earAudioComponentValidationFlags |= 8;
            } else if (major > expectedMajor || minor > expectedMinor) {
                g_earAudioComponentValidationFlags |= 2;
            }
            result = FALSE;
        }
    }
    GlobalUnlock(memory);
    GlobalFree(memory);
    return result;
}
