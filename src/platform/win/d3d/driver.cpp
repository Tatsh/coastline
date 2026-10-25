#include <cstring>

#include "platform/win/d3d/renderer.h"

DirectDrawDriver *g_pDirectDrawDriverListHead;

DirectDrawDriver *FindDirectDrawDriverOrDefault(GUID *pGuid) {
    DirectDrawDriver *pPrimary;
    DirectDrawDriver *pDriver = FindDirectDrawDriverByGuid(pGuid, &pPrimary, nullptr);

    return pDriver != nullptr ? pDriver : pPrimary;
}

HRESULT SelectDriverDeviceAndModeFromConfig(GUID *pDriverGuid,
                                            int nWidth,
                                            int nHeight,
                                            int nBitDepth,
                                            void *pSurfaceFormat,
                                            GUID *pDeviceGuid,
                                            bool bMatchMode,
                                            DirectDrawDriver **ppDriver,
                                            DisplayMode **ppMode,
                                            void **ppDevice) {
    DirectDrawDriver *pDriver;
    DisplayMode *pMode;
    void *pDevice;
    HRESULT hr;
    bool bFound;

    if (ppDriver == nullptr || ppMode == nullptr || ppDevice == nullptr) {
        return RENDERER_E_NOT_CREATED;
    }
    if ((g_bDirectDrawDriversEnumerated & 1) == 0) {
        hr = EnsureDirectDrawDriversEnumerated();
        if (FAILED(hr)) {
            return hr;
        }
    }
    pDriver = FindDirectDrawDriverOrDefault(pDriverGuid);
    if (pDriver == nullptr) {
        return RENDERER_E_NOT_CREATED;
    }
    if (bMatchMode) {
        bFound = SelectDeviceAndDisplayMode(
            pDriver, pDeviceGuid, nWidth, nHeight, nBitDepth, pSurfaceFormat, &pMode, &pDevice);
    } else {
        bFound = FindDesktopDisplayModeAndDevice(pDriver, pDeviceGuid, &pMode, &pDevice);
    }
    if (!bFound) {
        return RENDERER_E_NOT_IN_GDI_MODE;
    }
    *ppDriver = pDriver;
    *ppMode = pMode;
    *ppDevice = pDevice;
    return S_OK;
}

HRESULT DisplayMode::GetSurfaceDescDimensions(int *pnWidth,
                                              int *pnHeight,
                                              int *pnBitDepth,
                                              int *pnRefreshRate) {
    if (desc.dwSize != sizeof(desc) || (desc.dwFlags & DDSD_WIDTH) == 0 ||
        (desc.dwFlags & DDSD_HEIGHT) == 0 || (desc.dwFlags & DDSD_PIXELFORMAT) == 0 ||
        desc.ddpfPixelFormat.dwSize != sizeof(desc.ddpfPixelFormat)) {
        return E_FAIL;
    }
    *pnWidth = desc.dwWidth;
    *pnHeight = desc.dwHeight;
    *pnBitDepth = desc.ddpfPixelFormat.dwRGBBitCount;
    *pnRefreshRate = 0;
    return S_OK;
}

bool DirectDrawDriver::CompareDirectDrawDriverGuid(const GUID *pGuid) {
    if ((dwFlags & DIRECTDRAW_DRIVER_USABLE) == 0) {
        return false;
    }
    if (pGuid == nullptr) {
        return (dwFlags & DIRECTDRAW_DRIVER_PRIMARY) != 0;
    }
    return memcmp(&guid, pGuid, sizeof(guid)) == 0;
}

GUID *DirectDrawDriver::GetDirectDrawDriverGuid() {
    return (dwFlags & DIRECTDRAW_DRIVER_PRIMARY) != 0 ? nullptr : &guid;
}

DirectDrawDriver *
FindDirectDrawDriverByGuid(GUID *pGuid, DirectDrawDriver **ppPrimary, DirectDrawDriver *pFirst) {
    DirectDrawDriver *pDriver = pFirst != nullptr ? pFirst : g_pDirectDrawDriverListHead;

    if (ppPrimary != nullptr) {
        *ppPrimary = pDriver;
    }
    while (pDriver != nullptr) {
        DirectDrawDriver *pNext = pDriver->pNext;

        if (pDriver->CompareDirectDrawDriverGuid(pGuid)) {
            return pDriver;
        }
        if ((pDriver->dwFlags & DIRECTDRAW_DRIVER_PRIMARY) != 0 && ppPrimary != nullptr) {
            *ppPrimary = pDriver;
        }
        pDriver = pNext;
    }
    return nullptr;
}
