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
                                            D3DDeviceNode **ppDevice) {
    DirectDrawDriver *pDriver;
    DisplayMode *pMode;
    D3DDeviceNode *pDevice;
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

int DDBDToBitDepth(DWORD dwMask) {
    if (dwMask & DDBD_1) {
        return 1;
    }
    if (dwMask & DDBD_2) {
        return 2;
    }
    if (dwMask & DDBD_4) {
        return 4;
    }
    if (dwMask & DDBD_8) {
        return 8;
    }
    if (dwMask & DDBD_16) {
        return 16;
    }
    if (dwMask & DDBD_24) {
        return 24;
    }
    return (dwMask & DDBD_32) ? 32 : 0;
}

bool IsPixelFormatPalettized(const DDPIXELFORMAT *pFormat) {
    return pFormat != nullptr &&
           (pFormat->dwFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED4 |
                                DDPF_PALETTEINDEXED8)) != 0;
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

bool D3DDeviceNode::IsHardwareDevice() {
    return hardwareDesc.dcmColorModel != 0;
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

HRESULT EnsureDirectDrawDriversEnumerated(void) {
    HRESULT hr;

    if ((g_bDirectDrawDriversEnumerated & 1) == 0) {
        g_dwDirectDrawDriverCount = 0;
        hr = EnumerateDirectDrawDrivers();
        if (FAILED(hr)) {
            return hr;
        }
        g_bDirectDrawDriversEnumerated |= 1;
    }
    return S_OK;
}

HRESULT EnumerateDirectDrawDrivers(void) {
    DriverEnumContext context = {true, 0, 0};
    HRESULT hr;

    hr = DirectDrawEnumerateA(EnumDirectDrawDriversCallback, &context);
    if (FAILED(hr)) {
        return hr;
    }
    // Each driver DirectDraw reported must be in the list.
    if (context.bSucceeded && context.nFound != 0 && g_dwDirectDrawDriverCount == context.nFound) {
        return S_OK;
    }
    return RENDERER_E_NOT_IN_GDI_MODE;
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
