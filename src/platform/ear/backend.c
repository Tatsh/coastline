#include "backend.h"

#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <winver.h>

#include "error.h"

int32_t g_earAudioComponentsValidated;
uint16_t g_earAudioComponentValidationFlags;

// @ghidraAddress EAR: 0x1003b7d0
[[maybe_unused, gnu::used]] static uint32_t g_dwEarReservedInitializationOption;

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

// @ghidraAddress EAR: 0x1003b9a0
static const char g_earEmptyVersionPrefix[] = "";
// @ghidraAddress EAR: 0x1003722c
static const char g_earVersionHexLeadingZero[] = "0";
// @ghidraAddress EAR: 0x10037250
static const char g_earVersionResourcePathSeparator[] = "\\";
// @ghidraAddress EAR: 0x10037264
static const char g_earVersionComponentSeparator[] = ".";

static inline void BuildProductVersionQuery(void *versionData, char *queryPath) {
    void *translationValue;
    UINT translationSize;
    char language[16];
    lstrcpyA(queryPath, "\\VarFileInfo\\Translation");
    if (VerQueryValueA(versionData, queryPath, &translationValue, &translationSize)) {
        DWORD translation;
        memcpy(&translation, translationValue, sizeof(translation));
        DWORD packed = (translation >> 16) | (translation << 16);
        strcpy(language, g_earEmptyVersionPrefix);
        if (packed < 0x10000000) {
            strcpy(language, g_earVersionHexLeadingZero);
        }
        char hexadecimal[9];
        _itoa((int)packed, hexadecimal, 16);
        strcat(language, hexadecimal);
    } else {
        strcpy(language, "040904B0");
    }
    lstrcpyA(queryPath, "\\StringFileInfo\\");
    lstrcatA(queryPath, language);
    lstrcatA(queryPath, g_earVersionResourcePathSeparator);
    lstrcatA(queryPath, "ProductVersion");
}

static inline void GetExpectedDirectSoundVersion(DWORD *versionMS, DWORD *versionLS) {
    *versionMS = 0;
    *versionLS = 0;
    OSVERSIONINFOA info;
    info.dwOSVersionInfoSize = sizeof(info);
    if (GetVersionExA(&info)) {
        if (info.dwPlatformId == VER_PLATFORM_WIN32_NT) {
            *versionMS = 0x00050000;
            *versionLS = 0x074f0001;
        } else if (info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS && info.dwMajorVersion == 4) {
            if (info.dwMinorVersion == 10) {
                *versionMS = 0x00040005;
                *versionLS = 0x000107ce;
            } else if (info.dwMinorVersion == 0) {
                *versionMS = 0x00040005;
                *versionLS = 0x000000a5;
            }
        }
    }
}

int __stdcall GetDirectSoundVersion(void) {
    return QueryDirectSoundProductVersion();
}

#ifdef ENABLE_PATCHES
// Read the next component of a version string, or 0 when the string has no more components.
static uint16_t ReadVersionComponent(char *text, const char *delimiters) {
    const char *token = strtok(text, delimiters);
    return token != nullptr ? (uint16_t)atoi(token) : 0;
}
#endif

int __stdcall QueryDirectSoundProductVersion(void) {
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
        GetExpectedDirectSoundVersion(&expectedMS, &expectedLS);
        char delimiters[] = {g_earVersionComponentSeparator[0], g_earVersionComponentSeparator[1]};
        char *text = productVersionValue;
#ifdef ENABLE_PATCHES
        uint16_t major = ReadVersionComponent(text, delimiters);
        uint16_t minor = ReadVersionComponent(nullptr, delimiters);
        uint16_t build = ReadVersionComponent(nullptr, delimiters);
        uint16_t revision = ReadVersionComponent(nullptr, delimiters);
#else
        uint16_t major = (uint16_t)(atoi(strtok(text, delimiters)));
        uint16_t minor = (uint16_t)(atoi(strtok(nullptr, delimiters)));
        uint16_t build = (uint16_t)(atoi(strtok(nullptr, delimiters)));
        uint16_t revision = (uint16_t)(atoi(strtok(nullptr, delimiters)));
#endif
        uint16_t expectedMajor = (uint16_t)(expectedMS >> 16);
        uint16_t expectedMinor = (uint16_t)expectedMS;
        uint16_t expectedBuild = (uint16_t)(expectedLS >> 16);
        uint16_t expectedRevision = (uint16_t)expectedLS;
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
