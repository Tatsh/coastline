#include "configuration.h"

#include <cstdlib>
#include <cstring>
#include <new>

#include "error.h"
#include "mfc.h"
#include "runtime.h"
#include "version.h"
#include "window.h"

/** The interface of the version that runs without the IAS engine, in its high word. */
#define EAR_NO_IAS_INTERFACE 0x1096

int32_t g_bGlobalConfigurationFound;
uint16_t g_wConfiguredProductTarget;
uint16_t g_wConfiguredInterfaceTarget;
uint16_t g_wConfiguredVersion;
int32_t g_bConfiguredGlobalNoIas;
int32_t g_bTitleConfigurationFound;
int32_t g_bConfiguredTitleNoIas;
uint16_t g_earAudioComponentValidationFlags;
int32_t g_earAudioComponentsValidated;

int __stdcall ValidateEarTitleConfiguration(uint32_t required) {
    if (g_bConfigurationValidationDisabled) {
        return 1;
    }
    if (!g_bGlobalConfigurationFound) {
        if (required) {
            ShowEarMessage("Your computer is not set up for Interactive Around-Sound\xae;\n"
                           "some audio components may not perform correctly.\n\n"
                           "to set up the audio engine, run the\n"
                           "IAS configuration program");
            return 0;
        }
    } else if (g_bConfiguredGlobalNoIas &&
               (uint16_t)(EarGetVersionExportWrapper() >> 16) != EAR_NO_IAS_INTERFACE) {
        SetEarLastError(0x20);
        return 0;
    }
    if (!g_bTitleConfigurationFound) {
        if (required) {
            ShowEarMessage(
                "This program has not yet been configured for Interactive Around-Sound\xae;\n"
                "if you experience performance degradation, consult the documentation\n"
                "on configuring the IAS audio engine\n");
            return 0;
        }
    } else if (g_bConfiguredTitleNoIas &&
               (uint16_t)(EarGetVersionExportWrapper() >> 16) != EAR_NO_IAS_INTERFACE) {
        SetEarLastError(0x20);
        return 0;
    }
    return 1;
}

int ValidateEarAudioComponents(void) {
    g_earAudioComponentValidationFlags = 0;
    g_earAudioComponentsValidated = 1;
    uint32_t version = EarGetVersionExportWrapper();
    // The flags were cleared above, so the original never shows these four messages.
    if (g_earAudioComponentValidationFlags & 1) {
        ShowEarMessage("Your sound card drivers need updating;\n"
                       "please contact the manufacturer");
    }
    if (g_earAudioComponentValidationFlags & 8) {
        ShowEarMessage("You are using an older version of DirectSound than expected;\n"
                       "please visit www.microsoft.com and upgrade DirectX");
    }
    if (g_earAudioComponentValidationFlags & 2) {
        ShowEarMessage("The EAR IAS audio engine file <earpds.dll> needs updating;\n"
                       "please visit www.earsound.com to get the new version");
    }
    if (g_earAudioComponentValidationFlags & 4) {
        ShowEarMessage("The EAR IAS audio engine file <earias.dll> needs updating;\n"
                       "please visit www.earsound.com to get the new version for your sound card");
    }
    if (g_wConfiguredInterfaceTarget != (uint16_t)(version >> 16) &&
        g_wConfiguredProductTarget != (uint16_t)(version >> 16)) {
        ShowEarMessage("The program has loaded a different IAS audio component then you selected "
                       "during setup;\n"
                       "you need to re-run the IAS configuration program");
        g_earAudioComponentValidationFlags |= 1;
    }
    if (g_wConfiguredVersion != (uint16_t)version) {
        ShowEarMessage("The program has loaded a different version of the IAS audio component "
                       "then previously detected;\n"
                       "please verify you have only one version of earias.dll and earpds.dll on "
                       "your computer");
        g_earAudioComponentValidationFlags |= 1;
    }
    return g_earAudioComponentValidationFlags == 0;
}

/** The two-digit groups of a code. */
#define EAR_CODE_GROUP_COUNT 4

/**
 * Read a key of the code, a character of the error messages for each group of the code, at the
 * offset the groups add up to.
 */
static void ReadEarCodeKey(const uint16_t *groups, char *key) {
    uint16_t message = 0;
    uint16_t offset = 0;
    for (int group = 0; group < EAR_CODE_GROUP_COUNT; ++group) {
        int found = 0;
        offset += groups[group];
        do {
            if (strlen(g_earErrorMessages[message]) < offset) {
                offset -= (uint16_t)strlen(g_earErrorMessages[message++]);
            } else {
                found = 1;
            }
        } while (!found);
        key[group] = g_earErrorMessages[message][offset - 1];
    }
    key[EAR_CODE_GROUP_COUNT] = '\0';
    _strlwr(key);
}

uint32_t __stdcall ValidateEarNumericCode(uint32_t code) {
    char digits[34];
    uint16_t groups[EAR_CODE_GROUP_COUNT];
    char expected[EAR_CODE_GROUP_COUNT + 1];
    char received[EAR_CODE_GROUP_COUNT + 1];
    _itoa((int)code, digits, 10);
    // The original does not check the allocation, and leaks it when a group is too small.
    char *group = static_cast<char *>(MfcAllocateWithNewHandler(3));
    for (uint16_t index = 0; index < EAR_CODE_GROUP_COUNT; ++index) {
        strncpy(group, &digits[index * 2], 2);
        group[2] = '\0';
        groups[index] = static_cast<uint16_t>(std::strtol(group, nullptr, 10));
        if (groups[index] <= 1) {
            return 0;
        }
    }
    ReadEarCodeKey(groups, expected);
    // The original sets three values it never reads before reading the key again.
    [[maybe_unused]] uint16_t unusedA = 1;
    [[maybe_unused]] uint16_t unusedB = unusedA;
    [[maybe_unused]] uint16_t unusedC = unusedB;
    ReadEarCodeKey(groups, received);
    ::operator delete(group);
    // Both keys come from the same groups, so any code whose groups exceed 1 passes.
    g_bEarCodeValidated = strncmp(expected, received, EAR_CODE_GROUP_COUNT) == 0;
    return g_bEarCodeValidated;
}

int __stdcall EAR_DLL_AAA_Validate(uint32_t code) {
    SetEarLastError(0);
    if (!ValidateEarNumericCode(code)) {
        SetEarLastError(1);
        return 0;
    }
    LoadEarConfiguration(code);
    return 1;
}
