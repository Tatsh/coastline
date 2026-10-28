#include <cstring>

#include <windows.h>
// The headers below need the types of windows.h.
#include <ddraw.h>

#include "win32/boot/app.h"

typedef HRESULT(WINAPI *DirectDrawCreateProc)(GUID *pGuid,
                                              IDirectDraw **ppDirectDraw,
                                              IUnknown *pOuter);

// Only whether DINPUT.DLL exports DirectInputCreateA is checked.
static bool HasDirectInput(void) {
    HMODULE hDirectInput;
    FARPROC pfnDirectInputCreate;

    hDirectInput = LoadLibraryA("DINPUT.DLL");
    if (hDirectInput == nullptr) {
        OutputDebugStringA("Couldn't LoadLibrary DInput\r\n");
        return false;
    }
    pfnDirectInputCreate = GetProcAddress(hDirectInput, "DirectInputCreateA");
    FreeLibrary(hDirectInput);
    if (pfnDirectInputCreate == nullptr) {
        OutputDebugStringA("Couldn't GetProcAddress DInputCreate\r\n");
        return false;
    }
    return true;
}

void GetDXVersion(unsigned *pdwVersion, unsigned *pdwPlatform) {
    OSVERSIONINFOA osVersion;
    HMODULE hDirectDraw;
    HMODULE hDirectInput;
    DirectDrawCreateProc pfnDirectDrawCreate;
    FARPROC pfnDirectInputCreate;
    IDirectDraw *pDirectDraw = nullptr;
    IDirectDraw2 *pDirectDraw2 = nullptr;
    IDirectDrawSurface *pSurface = nullptr;
    IDirectDrawSurface3 *pSurface3 = nullptr;
    IDirectDrawSurface4 *pSurface4 = nullptr;
    DDSURFACEDESC desc;

    osVersion.dwOSVersionInfoSize = sizeof(osVersion);
    if (!GetVersionExA(&osVersion)) {
        *pdwVersion = 0;
        *pdwPlatform = 0;
        return;
    }
    if (osVersion.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        *pdwPlatform = VER_PLATFORM_WIN32_NT;
        if (osVersion.dwMajorVersion < 4) {
            *pdwPlatform = 0;
            return;
        }
        // DirectDraw is not probed on NT 4. A DirectInput there marks DirectX 3.
        if (osVersion.dwMajorVersion == 4) {
            *pdwVersion = 0x200;
            if (HasDirectInput()) {
                *pdwVersion = 0x300;
            }
            return;
        }
    } else {
        *pdwPlatform = VER_PLATFORM_WIN32_WINDOWS;
    }

    hDirectDraw = LoadLibraryA("DDRAW.DLL");
    if (hDirectDraw == nullptr) {
        *pdwVersion = 0;
        *pdwPlatform = 0;
        // The handle is null here and the call does nothing.
        FreeLibrary(hDirectDraw);
        return;
    }
    pfnDirectDrawCreate = reinterpret_cast<DirectDrawCreateProc>(
        reinterpret_cast<void (*)(void)>(GetProcAddress(hDirectDraw, "DirectDrawCreate")));
    if (pfnDirectDrawCreate == nullptr) {
        *pdwVersion = 0;
        *pdwPlatform = 0;
        FreeLibrary(hDirectDraw);
        OutputDebugStringA("Couldn't LoadLibrary DDraw\r\n");
        return;
    }
    if (FAILED(pfnDirectDrawCreate(nullptr, &pDirectDraw, nullptr))) {
        *pdwVersion = 0;
        *pdwPlatform = 0;
        FreeLibrary(hDirectDraw);
        OutputDebugStringA("Couldn't create DDraw\r\n");
        return;
    }

    *pdwVersion = 0x100;
    if (FAILED(pDirectDraw->QueryInterface(IID_IDirectDraw2,
                                           reinterpret_cast<void **>(&pDirectDraw2)))) {
        pDirectDraw->Release();
        FreeLibrary(hDirectDraw);
        OutputDebugStringA("Couldn't QI DDraw2\r\n");
        return;
    }
    pDirectDraw2->Release();

    *pdwVersion = 0x200;
    hDirectInput = LoadLibraryA("DINPUT.DLL");
    if (hDirectInput == nullptr) {
        OutputDebugStringA("Couldn't LoadLibrary DInput\r\n");
        pDirectDraw->Release();
        FreeLibrary(hDirectDraw);
        return;
    }
    pfnDirectInputCreate = GetProcAddress(hDirectInput, "DirectInputCreateA");
    FreeLibrary(hDirectInput);
    if (pfnDirectInputCreate == nullptr) {
        // DDRAW.DLL is freed before the DirectDraw object is released.
        FreeLibrary(hDirectDraw);
        pDirectDraw->Release();
        OutputDebugStringA("Couldn't GetProcAddress DInputCreate\r\n");
        return;
    }

    *pdwVersion = 0x300;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    if (FAILED(pDirectDraw->SetCooperativeLevel(nullptr, DDSCL_NORMAL))) {
        pDirectDraw->Release();
        FreeLibrary(hDirectDraw);
        *pdwVersion = 0;
        OutputDebugStringA("Couldn't Set coop level\r\n");
        return;
    }
    if (FAILED(pDirectDraw->CreateSurface(&desc, &pSurface, nullptr))) {
        pDirectDraw->Release();
        FreeLibrary(hDirectDraw);
        *pdwVersion = 0;
        OutputDebugStringA("Couldn't CreateSurface\r\n");
        return;
    }
    // The third interface of the surface is never released, and the surface is released only when
    // it has a fourth.
    if (FAILED(pSurface->QueryInterface(IID_IDirectDrawSurface3,
                                        reinterpret_cast<void **>(&pSurface3)))) {
        pDirectDraw->Release();
        FreeLibrary(hDirectDraw);
        return;
    }

    *pdwVersion = 0x500;
    if (SUCCEEDED(pSurface->QueryInterface(IID_IDirectDrawSurface4,
                                           reinterpret_cast<void **>(&pSurface4)))) {
        *pdwVersion = 0x600;
        // The surface is released in place of its fourth interface.
        pSurface->Release();
    }
    pDirectDraw->Release();
    FreeLibrary(hDirectDraw);
}
