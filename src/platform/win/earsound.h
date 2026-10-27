#pragma once

/**
 * @file
 * The routines of the EAR library, earias.dll or earpds.dll, found when the library loads.
 * Parameters with unknown types use integer arguments.
 */

#include <windows.h>

/** A routine of the EAR library with no parameters. */
typedef int(WINAPI *EarProc0)(void);
/** A routine of the EAR library with one parameter. */
typedef int(WINAPI *EarProc1)(int nValue);
/** A routine of the EAR library with two parameters. */
typedef int(WINAPI *EarProc2)(int nValue, int nValue2);
/** A routine of the EAR library with four parameters. */
typedef int(WINAPI *EarProc4)(int nValue, int nValue2, int nValue3, int nValue4);
/** The routine of the EAR library that takes the window it posts its messages to. */
typedef int(WINAPI *EarAssignHwndProc)(HWND hWnd);

/** Parameters shared with the EAR event mixer. */
struct EarPlayParams {
    unsigned short wSize;
    unsigned short wType;
    int nTrack;
    WPARAM hEvent;
    unsigned short wChannel;
    unsigned short awUnknown0e[3]; /* +0x0e */
    unsigned adwUnknown14[2];      /* +0x14 */
    unsigned dwPlayFlags;
    unsigned adwUnknown20[2];      /* +0x20 */
    unsigned short awUnknown28[2]; /* +0x28 */
    unsigned adwUnknown2c[4];      /* +0x2c */
    unsigned short awUnknown3c[3]; /* +0x3c */
    unsigned short wPlaybackControl;
    unsigned short awUnknown44[2]; /* +0x44 */
    unsigned dwUnknown48;          /* +0x48 */
    unsigned short awUnknown4c[2]; /* +0x4c */
    unsigned adwUnknown50[8];      /* +0x50 */
    unsigned short awUnknown70[7]; /* +0x70 */
    unsigned short wUnknown7e;     /* +0x7e */
};

/** Mix an EAR event and return its event identifier. */
typedef WPARAM(WINAPI *EarMixEventProc)(EarPlayParams *pParams);

/**
 * @ghidraAddress BRD3D: 0x10220c50
 * @ghidraAddress BRGlide: 0x1021c780
 */
extern EarPlayParams g_EarPlayParams;

/**
 * The EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575464
 * @ghidraAddress BRGlide: 0x104b160c
 */
extern HMODULE g_hEarSoundDll;

/**
 * Whether LoadEarSoundDll() loaded the library, rather than finding it already loaded.
 *
 * @ghidraAddress BRD3D: 0x105754d4
 * @ghidraAddress BRGlide: 0x104b167c
 */
extern int g_bEarSoundDllLoadedByUs;

/**
 * Whether earpds.dll was loaded in place of earias.dll.
 *
 * @ghidraAddress BRD3D: 0x105754d8
 * @ghidraAddress BRGlide: 0x104b1680
 */
extern int g_bEarSoundDllUsingPds;

/**
 * Whether earpds.dll was requested.
 *
 * @ghidraAddress BRD3D: 0x105754dc
 * @ghidraAddress BRGlide: 0x104b1684
 */
extern int g_bEarSoundPdsRequested;

/**
 * The library InitializeEarSound() loads. 1 requests earpds.dll, and 0 requests earias.dll with
 * earpds.dll as the fallback.
 *
 * @ghidraAddress BRD3D: 0x100a81a8
 * @ghidraAddress BRGlide: 0x100a74fc
 */
extern int g_dwEarSoundDllMode;

/**
 * The number of calls to InitializeEarSound().
 *
 * @ghidraAddress BRD3D: 0x105754e0
 * @ghidraAddress BRGlide: 0x104b1688
 */
extern int g_dwEarSoundInitRefCount;

/**
 * Load an EAR library and find its routines.
 *
 * @param bUsePds Whether to load earpds.dll rather than earias.dll.
 * @return Whether the library loaded and every routine was found.
 * @ghidraAddress BRD3D: 0x1002a8a0
 * @ghidraAddress BRGlide: 0x10017910
 */
bool LoadEarSoundDll(int bUsePds);

/**
 * _EAR_DLL_AAA_Validate of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754b0
 * @ghidraAddress BRGlide: 0x104b1658
 */
extern EarProc1 g_pfnEarAaaValidate;

/**
 * _EAR_DLL_AssignHwnd of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x1057548c
 * @ghidraAddress BRGlide: 0x104b1634
 */
extern EarAssignHwndProc g_pfnEarAssignHwnd;

/**
 * _EAR_DLL_ChangeChannelControl of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575484
 * @ghidraAddress BRGlide: 0x104b162c
 */
extern EarProc2 g_pfnEarChangeChannelControl;

/**
 * _EAR_DLL_ClearChannel of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575480
 * @ghidraAddress BRGlide: 0x104b1628
 */
extern EarProc2 g_pfnEarClearChannel;

/**
 * _EAR_DLL_EarInactive of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575460
 * @ghidraAddress BRGlide: 0x104b1608
 */
extern EarProc0 g_pfnEarEarInactive;

/**
 * _EAR_DLL_GetEventStatus of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754b4
 * @ghidraAddress BRGlide: 0x104b165c
 */
extern EarProc2 g_pfnEarGetEventStatus;

/**
 * _EAR_DLL_GetLastError of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754c4
 * @ghidraAddress BRGlide: 0x104b166c
 */
extern EarProc0 g_pfnEarGetLastError;

/**
 * _EAR_DLL_GetVersion of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575450
 * @ghidraAddress BRGlide: 0x104b15f8
 */
extern EarProc0 g_pfnEarGetVersion;

/**
 * _EAR_DLL_InitializeEar of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754c0
 * @ghidraAddress BRGlide: 0x104b1668
 */
extern EarProc1 g_pfnEarInitializeEar;

/**
 * _EAR_DLL_MixEvent of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x1057546c
 * @ghidraAddress BRGlide: 0x104b1614
 */
extern EarMixEventProc g_pfnEarMixEvent;

/**
 * _EAR_DLL_MoveEvent of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575488
 * @ghidraAddress BRGlide: 0x104b1630
 */
extern EarProc1 g_pfnEarMoveEvent;

/**
 * _EAR_DLL_RegisterBank of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754cc
 * @ghidraAddress BRGlide: 0x104b1674
 */
extern EarProc2 g_pfnEarRegisterBank;

/**
 * _EAR_DLL_RegisterChannel of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754a0
 * @ghidraAddress BRGlide: 0x104b1648
 */
extern EarProc4 g_pfnEarRegisterChannel;

/**
 * _EAR_DLL_RegisterEnvironment of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754ac
 * @ghidraAddress BRGlide: 0x104b1654
 */
extern EarProc1 g_pfnEarRegisterEnvironment;

/**
 * _EAR_DLL_RegisterMatrix of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754d0
 * @ghidraAddress BRGlide: 0x104b1678
 */
extern EarProc1 g_pfnEarRegisterMatrix;

/**
 * _EAR_DLL_RegisterPreset of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754c8
 * @ghidraAddress BRGlide: 0x104b1670
 */
extern EarProc2 g_pfnEarRegisterPreset;

/**
 * _EAR_DLL_ResetEar of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575494
 * @ghidraAddress BRGlide: 0x104b163c
 */
extern EarProc0 g_pfnEarResetEar;

/**
 * _EAR_DLL_SetAttenuationLevel of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575454
 * @ghidraAddress BRGlide: 0x104b15fc
 */
extern EarProc2 g_pfnEarSetAttenuationLevel;

/**
 * _EAR_DLL_SetUserDistanceUnit of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754b8
 * @ghidraAddress BRGlide: 0x104b1660
 */
extern EarProc2 g_pfnEarSetUserDistanceUnit;

/**
 * _EAR_DLL_ShowLastError of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754a8
 * @ghidraAddress BRGlide: 0x104b1650
 */
extern EarProc0 g_pfnEarShowLastError;

/**
 * _EAR_DLL_ShutDownBank of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575458
 * @ghidraAddress BRGlide: 0x104b1600
 */
extern EarProc1 g_pfnEarShutDownBank;

/**
 * _EAR_DLL_ShutDownChannel of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575470
 * @ghidraAddress BRGlide: 0x104b1618
 */
extern EarProc1 g_pfnEarShutDownChannel;

/**
 * _EAR_DLL_ShutDownEar of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575474
 * @ghidraAddress BRGlide: 0x104b161c
 */
extern EarProc0 g_pfnEarShutDownEar;

/**
 * _EAR_DLL_ShutDownEnvironment of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754a4
 * @ghidraAddress BRGlide: 0x104b164c
 */
extern EarProc1 g_pfnEarShutDownEnvironment;

/**
 * _EAR_DLL_ShutDownEvent of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575498
 * @ghidraAddress BRGlide: 0x104b1640
 */
extern EarProc2 g_pfnEarShutDownEvent;

/**
 * _EAR_DLL_ShutDownMatrix of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x1057545c
 * @ghidraAddress BRGlide: 0x104b1604
 */
extern EarProc1 g_pfnEarShutDownMatrix;

/**
 * _EAR_DLL_ShutDownPreset of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575490
 * @ghidraAddress BRGlide: 0x104b1638
 */
extern EarProc1 g_pfnEarShutDownPreset;

/**
 * _EAR_DLL_StartEvent of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x1057547c
 * @ghidraAddress BRGlide: 0x104b1624
 */
extern EarProc1 g_pfnEarStartEvent;

/**
 * _EAR_DLL_StartTimer of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x1057549c
 * @ghidraAddress BRGlide: 0x104b1644
 */
extern EarProc0 g_pfnEarStartTimer;

/**
 * _EAR_DLL_ShutDownTimer of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x10575468
 * @ghidraAddress BRGlide: 0x104b1610
 */
extern EarProc0 g_pfnEarShutDownTimer;

/**
 * _EAR_DLL_UpdateEar of the EAR library.
 *
 * @ghidraAddress BRD3D: 0x105754bc
 * @ghidraAddress BRGlide: 0x104b1664
 */
extern EarProc0 g_pfnEarUpdateEar;
