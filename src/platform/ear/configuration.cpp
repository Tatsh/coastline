#include "configuration.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include <windows.h>

#include "error.h"
#include "mfc.h"
#include "musicdevice.h"
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
int32_t g_bConfiguredDvdDrive;
int32_t g_bConfiguredCdChanger;
int32_t g_bConfiguredMultiDisc;
int32_t g_bTitleConfigurationFound;
char g_szConfiguredTitle[80];
int32_t g_bConfiguredTitleNoIas;
int32_t g_bConfiguredTitleIsDvd;
uint32_t g_dwConfiguredLaunchCount;
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

/** The code that reads only the global section. */
#define EAR_GLOBAL_ONLY_CODE 0x98bdf5
/** The characters each line of the configuration is read with, its line break included. */
#define EAR_CONFIGURATION_LINE 0x4f
/** The bytes of the line of the launch count, its line break written as two. */
#define EAR_LAUNCH_COUNT_LINE_BYTES 0xd
/** The count after which the launch count starts again. */
#define EAR_LAUNCH_COUNT_LIMIT 9999

// @ghidraAddress EAR: 0x10036170
static const char g_szEarConfigFileSuffix[] = "\\config.ear";
// @ghidraAddress EAR: 0x10036334
static const char g_abTitleKeyAlphabet0[] = "XVKFYRSZLT";
// @ghidraAddress EAR: 0x10036340
static const char g_abTitleKeyAlphabet1[] = "QNJBDPKWZH";

/** Whether a line starts with a key, compared over the length of a second spelling of it. */
static int IsEarConfigurationKey(const char *line, const char *measured, const char *key) {
    return std::strncmp(line, key, std::strlen(measured)) == 0;
}

/** Whether the flag after a key in a line is set, which is the character 1. */
static int32_t ReadEarConfigurationFlag(const char *line, const char *key) {
    return line[std::strlen(key)] == '1';
}

/** The number after a key in a line. */
static long ReadEarConfigurationNumber(const char *line, const char *key, int base) {
    return std::strtol(line + std::strlen(key), nullptr, base);
}

// The original reads the file through the CStdioFile of MFC: a read and write open becomes the
// text stream r+t, and a write open becomes a stream that writes where it seeks, as r+t does.
void __stdcall LoadEarConfiguration(uint32_t code) {
    char path[MAX_PATH];
    char line[EAR_CONFIGURATION_LINE + 1];
    // The digits of the code, then the key of the title; the bytes after the digits are read as
    // the stack holds them when the code has fewer than eight.
    char codeText[0x1c];
    uint8_t digits[8];
    char titleKeyAlphabets[2][12];
    GetSystemDirectoryA(path, MAX_PATH);
    std::strcat(path, g_szEarConfigFileSuffix);
    FILE *file = std::fopen(path, "r+t");
    if (!file) {
        return;
    }
    int32_t globalFound = 0;
    int endOfFile = 0;
    int endOfGlobal = 0;
    do {
        if (!std::fgets(line, EAR_CONFIGURATION_LINE, file)) {
            endOfFile = 1;
        } else if (IsEarConfigurationKey(line, "<GLOBAL>", "<GLOBAL>")) {
            globalFound = 1;
            do {
                if (!std::fgets(line, EAR_CONFIGURATION_LINE, file)) {
                    endOfFile = 1;
                    continue;
                }
                if (IsEarConfigurationKey(line, "[PTARGET]", "[PTARGET]")) {
                    g_wConfiguredProductTarget =
                        static_cast<uint16_t>(ReadEarConfigurationNumber(line, "[PTARGET]", 16));
                } else if (IsEarConfigurationKey(line, "[ITARGET]", "[ITARGET]")) {
                    g_wConfiguredInterfaceTarget =
                        static_cast<uint16_t>(ReadEarConfigurationNumber(line, "[ITARGET]", 16));
                }
                // The version is tested after either target, as in the original.
                if (IsEarConfigurationKey(line, "[VERSION]", "[VERSION]")) {
                    g_wConfiguredVersion =
                        static_cast<uint16_t>(ReadEarConfigurationNumber(line, "[VERSION]", 16));
                } else if (IsEarConfigurationKey(line, "[GNOIAS]", "[GNOIAS]")) {
                    g_bConfiguredGlobalNoIas = ReadEarConfigurationFlag(line, "[GNOIAS]");
                } else if (IsEarConfigurationKey(line, "[CDMIXER]", "[CDMIXER]")) {
                    g_uConfiguredCdMixerId =
                        static_cast<UINT>(ReadEarConfigurationNumber(line, "[CDMIXER]", 10));
                } else if (IsEarConfigurationKey(line, "[MIDIMIXER]", "[MIDIMIXER]")) {
                    g_uConfiguredMidiMixerId =
                        static_cast<UINT>(ReadEarConfigurationNumber(line, "[MIDIMIXER]", 10));
                } else if (IsEarConfigurationKey(line, "[DVDDRIVE]", "[DVDDRIVE]")) {
                    g_bConfiguredDvdDrive = ReadEarConfigurationFlag(line, "[DVDDRIVE]");
                } else if (IsEarConfigurationKey(line, "[CHANGER]", "[CHANGER]")) {
                    g_bConfiguredCdChanger = ReadEarConfigurationFlag(line, "[CHANGER]");
                } else if (IsEarConfigurationKey(line, "[MULTI]", "[MULTI]")) {
                    g_bConfiguredMultiDisc = ReadEarConfigurationFlag(line, "[MULTI]");
                } else if (IsEarConfigurationKey(line, "[CDLINE]", "[CDLINE]")) {
                    g_dwConfiguredCdMixerLineId =
                        static_cast<DWORD>(ReadEarConfigurationNumber(line, "[CDLINE]", 10));
                } else if (IsEarConfigurationKey(line, "[MIDILINE]", "[MIDILINE]")) {
                    g_dwConfiguredMidiMixerLineId =
                        static_cast<DWORD>(ReadEarConfigurationNumber(line, "[MIDILINE]", 10));
                } else if (IsEarConfigurationKey(line, "<END GLOBAL>", "<END GLOBAL>")) {
                    endOfGlobal = 1;
                }
            } while (!endOfFile && !endOfGlobal);
        }
    } while (!endOfFile && !globalFound);
    g_bGlobalConfigurationFound = globalFound;
    if (code == EAR_GLOBAL_ONLY_CODE) {
        std::fclose(file);
        return;
    }
    _itoa(static_cast<int>(code), codeText, 10);
    for (uint8_t digit = 0; digit < 8; ++digit) {
        char single[2];
        std::strncpy(single, &codeText[digit], 1);
        single[1] = '\0';
        digits[digit] = static_cast<uint8_t>(std::strtol(single, nullptr, 10));
    }
    std::memcpy(titleKeyAlphabets[0], g_abTitleKeyAlphabet0, sizeof(g_abTitleKeyAlphabet0));
    titleKeyAlphabets[0][11] = '\0';
    std::memcpy(titleKeyAlphabets[1], g_abTitleKeyAlphabet1, sizeof(g_abTitleKeyAlphabet1));
    titleKeyAlphabets[1][11] = '\0';
    // An even fourth digit picks the second alphabet.
    uint16_t alphabet = digits[3] % 2 == 0;
    codeText[0] = '<';
    for (int digit = 0; digit < 8; ++digit) {
        codeText[digit + 1] = titleKeyAlphabets[alphabet][digits[digit]];
    }
    codeText[9] = '>';
    codeText[10] = '\0';
    _strupr(codeText);
    int32_t titleFound = 0;
    int endOfTitle = 0;
    long launchCountPosition = 0;
    if (!endOfFile) {
        do {
            if (!std::fgets(line, EAR_CONFIGURATION_LINE, file)) {
                endOfFile = 1;
            } else if (IsEarConfigurationKey(line, codeText, codeText)) {
                titleFound = 1;
                do {
                    if (!std::fgets(line, EAR_CONFIGURATION_LINE, file)) {
                        endOfFile = 1;
                        continue;
                    }
                    if (IsEarConfigurationKey(line, "[TITLE]", "[TITLE]")) {
                        // The line break is dropped and the title is not terminated; an empty
                        // title copies with a count of -1, as in the original.
                        const char *title = line + std::strlen("[TITLE]");
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
                        std::strncpy(g_szConfiguredTitle, title, std::strlen(title) - 1);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
                    } else if (IsEarConfigurationKey(line, "[NOIAS]", "[NOIAS]")) {
                        g_bConfiguredTitleNoIas = ReadEarConfigurationFlag(line, "[NOIAS]");
                    } else if (IsEarConfigurationKey(line, "[ISDVD]", "[ISDVD]")) {
                        g_bConfiguredTitleIsDvd = ReadEarConfigurationFlag(line, "[ISDVD]");
                    } else if (IsEarConfigurationKey(line, "[TIMES]", "[TIMES]")) {
                        g_dwConfiguredLaunchCount =
                            static_cast<uint16_t>(ReadEarConfigurationNumber(line, "[TIMES]", 10));
                        g_dwConfiguredLaunchCount =
                            (g_dwConfiguredLaunchCount + 1) % EAR_LAUNCH_COUNT_LIMIT;
                        launchCountPosition = std::ftell(file);
                    } else if (IsEarConfigurationKey(line, "<END TITLE", "<END TITLE>")) {
                        endOfTitle = 1;
                    }
                } while (!endOfFile && !endOfTitle);
            }
        } while (!endOfFile && !titleFound);
    }
    g_bTitleConfigurationFound = titleFound;
    std::fclose(file);
    if (!launchCountPosition || !g_bTitleConfigurationFound) {
        return;
    }
    file = std::fopen(path, "r+t");
    if (!file) {
        return;
    }
    std::fseek(file, launchCountPosition - EAR_LAUNCH_COUNT_LINE_BYTES, SEEK_SET);
    _itoa(static_cast<int>(g_dwConfiguredLaunchCount), codeText, 10);
    std::strcpy(line, "[TIMES]");
    for (unsigned zero = 0; zero < 4 - std::strlen(codeText); ++zero) {
        std::strcat(line, "0");
    }
    std::strcat(line, codeText);
    std::strcat(line, "\n");
    std::fputs(line, file);
    std::fclose(file);
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
