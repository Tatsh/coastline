#include "platform/win/earsound.h"

#include <cstdlib>
#include <type_traits>

#include <windows.h>

#include "platform/win/boot/strings.h"
#include "platform/win/sound.h"

// The key _EAR_DLL_AAA_Validate checks.
#define EAR_VALIDATION_KEY 0x9be9c9
// The error of the EAR library the game reports with its own message. Its meaning is not
// recovered yet.
#define EAR_ERROR_REPORTED_BY_GAME 3

HMODULE g_hEarSoundDll;
int g_bEarSoundDllLoadedByUs;
int g_bEarSoundDllUsingPds;
int g_bEarSoundPdsRequested;
int g_dwEarSoundDllMode = 1;
int g_dwEarSoundInitRefCount;
UINT g_uEarSoundWindowMessage;

EarProc1 g_pfnEarAaaValidate;
EarAssignHwndProc g_pfnEarAssignHwnd;
EarProc2 g_pfnEarChangeChannelControl;
EarProc2 g_pfnEarClearChannel;
EarProc0 g_pfnEarEarInactive;
EarProc2 g_pfnEarGetEventStatus;
EarProc0 g_pfnEarGetLastError;
EarProc0 g_pfnEarGetVersion;
EarProc1 g_pfnEarInitializeEar;
EarMixEventProc g_pfnEarMixEvent;
EarProc1 g_pfnEarMoveEvent;
EarProc2 g_pfnEarRegisterBank;
EarProc4 g_pfnEarRegisterChannel;
EarProc1 g_pfnEarRegisterEnvironment;
EarProc1 g_pfnEarRegisterMatrix;
EarProc2 g_pfnEarRegisterPreset;
EarProc0 g_pfnEarResetEar;
EarProc2 g_pfnEarSetAttenuationLevel;
EarProc2 g_pfnEarSetUserDistanceUnit;
EarProc0 g_pfnEarShowLastError;
EarProc1 g_pfnEarShutDownBank;
EarProc1 g_pfnEarShutDownChannel;
EarProc0 g_pfnEarShutDownEar;
EarProc1 g_pfnEarShutDownEnvironment;
EarProc2 g_pfnEarShutDownEvent;
EarProc1 g_pfnEarShutDownMatrix;
EarProc1 g_pfnEarShutDownPreset;
EarProc1 g_pfnEarStartEvent;
EarProc0 g_pfnEarStartTimer;
EarProc0 g_pfnEarShutDownTimer;
EarProc0 g_pfnEarUpdateEar;

template <typename T>
    requires(std::is_pointer_v<T> && std::is_function_v<std::remove_pointer_t<T>>)
static inline void FindEarProc(T &pfnProc, const char *pszName) {
    pfnProc = reinterpret_cast<T>(
        reinterpret_cast<void (*)(void)>(GetProcAddress(g_hEarSoundDll, pszName)));
}

bool LoadEarSoundDll(int bUsePds) {
    const char *pszName;

    if (g_hEarSoundDll != nullptr && g_pfnEarUpdateEar != nullptr) {
        return true;
    }
    if (bUsePds) {
        g_bEarSoundPdsRequested = true;
        pszName = "earpds.dll";
    } else {
        pszName = "earias.dll";
    }
    g_hEarSoundDll = GetModuleHandleA(pszName);
    if (g_hEarSoundDll == nullptr) {
        g_bEarSoundDllLoadedByUs = true;
        g_hEarSoundDll = LoadLibraryA(pszName);
        // Only earias.dll falls back. A missing earpds.dll goes on with no library, and every
        // routine is then missing.
        if (g_hEarSoundDll == nullptr && !bUsePds) {
            g_bEarSoundDllLoadedByUs = false;
            g_bEarSoundDllUsingPds = true;
            g_hEarSoundDll = GetModuleHandleA("earpds.dll");
            if (g_hEarSoundDll == nullptr) {
                g_bEarSoundDllLoadedByUs = true;
                g_hEarSoundDll = LoadLibraryA("earpds.dll");
                if (g_hEarSoundDll == nullptr) {
                    return false;
                }
            }
        }
    }
    if (g_pfnEarUpdateEar != nullptr) {
        return true;
    }
    FindEarProc(g_pfnEarAaaValidate, "_EAR_DLL_AAA_Validate@4");
    FindEarProc(g_pfnEarAssignHwnd, "_EAR_DLL_AssignHwnd@4");
    FindEarProc(g_pfnEarChangeChannelControl, "_EAR_DLL_ChangeChannelControl@8");
    FindEarProc(g_pfnEarClearChannel, "_EAR_DLL_ClearChannel@8");
    FindEarProc(g_pfnEarEarInactive, "_EAR_DLL_EarInactive@0");
    FindEarProc(g_pfnEarGetEventStatus, "_EAR_DLL_GetEventStatus@8");
    FindEarProc(g_pfnEarGetLastError, "_EAR_DLL_GetLastError@0");
    FindEarProc(g_pfnEarGetVersion, "_EAR_DLL_GetVersion@0");
    FindEarProc(g_pfnEarInitializeEar, "_EAR_DLL_InitializeEar@4");
    FindEarProc(g_pfnEarMixEvent, "_EAR_DLL_MixEvent@4");
    FindEarProc(g_pfnEarMoveEvent, "_EAR_DLL_MoveEvent@4");
    FindEarProc(g_pfnEarRegisterBank, "_EAR_DLL_RegisterBank@8");
    FindEarProc(g_pfnEarRegisterChannel, "_EAR_DLL_RegisterChannel@16");
    FindEarProc(g_pfnEarRegisterEnvironment, "_EAR_DLL_RegisterEnvironment@4");
    FindEarProc(g_pfnEarRegisterMatrix, "_EAR_DLL_RegisterMatrix@4");
    FindEarProc(g_pfnEarRegisterPreset, "_EAR_DLL_RegisterPreset@8");
    FindEarProc(g_pfnEarResetEar, "_EAR_DLL_ResetEar@0");
    FindEarProc(g_pfnEarSetAttenuationLevel, "_EAR_DLL_SetAttenuationLevel@8");
    FindEarProc(g_pfnEarSetUserDistanceUnit, "_EAR_DLL_SetUserDistanceUnit@8");
    FindEarProc(g_pfnEarShowLastError, "_EAR_DLL_ShowLastError@0");
    FindEarProc(g_pfnEarShutDownBank, "_EAR_DLL_ShutDownBank@4");
    FindEarProc(g_pfnEarShutDownChannel, "_EAR_DLL_ShutDownChannel@4");
    FindEarProc(g_pfnEarShutDownEar, "_EAR_DLL_ShutDownEar@0");
    FindEarProc(g_pfnEarShutDownEnvironment, "_EAR_DLL_ShutDownEnvironment@4");
    FindEarProc(g_pfnEarShutDownEvent, "_EAR_DLL_ShutDownEvent@8");
    FindEarProc(g_pfnEarShutDownMatrix, "_EAR_DLL_ShutDownMatrix@4");
    FindEarProc(g_pfnEarShutDownPreset, "_EAR_DLL_ShutDownPreset@4");
    FindEarProc(g_pfnEarStartEvent, "_EAR_DLL_StartEvent@4");
    FindEarProc(g_pfnEarStartTimer, "_EAR_DLL_StartTimer@0");
    FindEarProc(g_pfnEarShutDownTimer, "_EAR_DLL_ShutDownTimer@0");
    FindEarProc(g_pfnEarUpdateEar, "_EAR_DLL_UpdateEar@0");
    if (g_pfnEarShutDownEnvironment == nullptr || g_pfnEarShutDownEar == nullptr ||
        g_pfnEarShutDownMatrix == nullptr || g_pfnEarShutDownEvent == nullptr ||
        g_pfnEarShutDownTimer == nullptr || g_pfnEarShutDownPreset == nullptr ||
        g_pfnEarStartTimer == nullptr || g_pfnEarStartEvent == nullptr ||
        g_pfnEarUpdateEar == nullptr || g_pfnEarGetVersion == nullptr ||
        g_pfnEarGetLastError == nullptr || g_pfnEarMixEvent == nullptr ||
        g_pfnEarInitializeEar == nullptr || g_pfnEarRegisterBank == nullptr ||
        g_pfnEarMoveEvent == nullptr || g_pfnEarRegisterEnvironment == nullptr ||
        g_pfnEarRegisterChannel == nullptr || g_pfnEarRegisterPreset == nullptr ||
        g_pfnEarRegisterMatrix == nullptr || g_pfnEarSetAttenuationLevel == nullptr ||
        g_pfnEarResetEar == nullptr || g_pfnEarShowLastError == nullptr ||
        g_pfnEarSetUserDistanceUnit == nullptr || g_pfnEarShutDownChannel == nullptr ||
        g_pfnEarShutDownBank == nullptr || g_pfnEarAssignHwnd == nullptr ||
        g_pfnEarAaaValidate == nullptr || g_pfnEarClearChannel == nullptr ||
        g_pfnEarChangeChannelControl == nullptr || g_pfnEarGetEventStatus == nullptr ||
        g_pfnEarEarInactive == nullptr) {
        return false;
    }
    g_uEarSoundWindowMessage = RegisterWindowMessageA("EAR Interactive Around-Sound");
    return true;
}

bool ShutdownEarSound(void) {
    if (--g_dwEarSoundInitRefCount == 0) {
        g_pfnEarShutDownEar();
    }
    return true;
}

bool InitializeEarSound(HWND hWnd) {
    if (++g_dwEarSoundInitRefCount != 1) {
        return true;
    }
    if (!LoadEarSoundDll(g_dwEarSoundDllMode)) {
        MessageBoxA(hWnd, GetStringResource(0xfe), GetStringResource(0xfd), MB_ICONERROR);
        exit(1);
    }
    g_pfnEarAaaValidate(EAR_VALIDATION_KEY);
    g_pfnEarAssignHwnd(hWnd);
    if (!g_pfnEarInitializeEar(0)) {
        if (g_pfnEarGetLastError() == EAR_ERROR_REPORTED_BY_GAME) {
            MessageBoxA(hWnd, GetStringResource(0x12e), GetStringResource(0xfd), MB_ICONERROR);
            exit(1);
        }
        g_pfnEarShowLastError();
        exit(1);
    }
    return true;
}
