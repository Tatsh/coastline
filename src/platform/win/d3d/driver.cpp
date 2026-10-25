#include <cstdlib>
#include <cstring>
#include <new>

#include "platform/win/d3d/renderer.h"

DirectDrawDriver *g_pDirectDrawDriverListHead;
DirectDrawDriver *g_pDirectDrawDriverListTail;

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

BOOL WINAPI EnumDirectDrawDriversCallback(GUID *pGuid,
                                          LPSTR pszDescription,
                                          LPSTR pszName,
                                          LPVOID pContext) {
    DriverEnumContext *pEnum = static_cast<DriverEnumContext *>(pContext);
    DirectDrawDriver *pDriver;

    if (pEnum == nullptr) {
        return DDENUMRET_OK;
    }
    pDriver = new (std::nothrow) DirectDrawDriver;
    if (pDriver == nullptr) {
        return DDENUMRET_OK;
    }
    // A driver that is not read or not added is freed, and the search goes on without it.
    if (FAILED(pDriver->InitializeDirectDrawDriver(pGuid, pszName, pszDescription)) ||
        FAILED(AppendDirectDrawDriverNode(pDriver))) {
        pDriver->ShutdownDirectDrawDriver();
        delete pDriver;
        return DDENUMRET_OK;
    }
    pEnum->nFound++;
    return DDENUMRET_OK;
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

// The name and the description a driver gets when DirectDraw reports none.
#define UNKNOWN_DRIVER_TEXT "UNKNOWN"

// Copy a text of a driver into memory of its own.
static inline char *CopyDriverText(const char *pszText) {
    size_t nLength;
    char *pszCopy;

    if (pszText == nullptr) {
        pszText = UNKNOWN_DRIVER_TEXT;
    }
    nLength = strlen(pszText);
    pszCopy = static_cast<char *>(malloc(nLength + 1));
    if (pszCopy != nullptr) {
        strncpy(pszCopy, pszText, nLength);
        pszCopy[nLength] = '\0';
    }
    return pszCopy;
}

HRESULT DirectDrawDriver::InitializeDirectDrawDriver(GUID *pGuid,
                                                     LPSTR pszNewName,
                                                     LPSTR pszNewDescription) {
    IDirectDraw *pDirectDraw = nullptr;
    IDirectDraw2 *pDirectDraw2 = nullptr;
    IDirect3D2 *pDirect3D2 = nullptr;
    HRESULT hr;

    if (dwFlags & DIRECTDRAW_DRIVER_USABLE) {
        return S_OK;
    }
    if (pGuid == nullptr) {
        dwFlags |= DIRECTDRAW_DRIVER_PRIMARY;
    } else {
        guid = *pGuid;
    }
    pszName = CopyDriverText(pszNewName);
    pszDescription = CopyDriverText(pszNewDescription);
    if (SUCCEEDED(hr = DirectDrawCreate(pGuid, &pDirectDraw, nullptr)) &&
        SUCCEEDED(hr = pDirectDraw->QueryInterface(IID_IDirectDraw2,
                                                   reinterpret_cast<void **>(&pDirectDraw2))) &&
        SUCCEEDED(hr = pDirectDraw->QueryInterface(IID_IDirect3D2,
                                                   reinterpret_cast<void **>(&pDirect3D2)))) {
        g_pDirect3D2 = pDirect3D2;
        hardwareCaps.dwSize = sizeof(hardwareCaps);
        softwareCaps.dwSize = sizeof(softwareCaps);
        if (SUCCEEDED(hr = pDirectDraw2->GetCaps(&hardwareCaps, &softwareCaps))) {
            // A driver whose hardware has no depth buffer cannot draw in 3D.
            if (hardwareCaps.dwZBufferBitDepths == 0) {
                hr = DDERR_NO3D;
            } else {
                dwFlags &= ~8;
                nDisplayModeCount = 0;
                if (SUCCEEDED(hr = EnumerateDisplayModes(pDirectDraw2))) {
                    dwFlags &= ~0x10;
                    nD3DDeviceCount = 0;
                    if (SUCCEEDED(hr = EnumerateD3DDevices(pDirect3D2))) {
                        dwFlags |= DIRECTDRAW_DRIVER_USABLE;
                        hr = S_OK;
                    }
                }
            }
        }
    }
    // The interfaces are only for the search.
    if (pDirect3D2 != nullptr) {
        pDirect3D2->Release();
        g_pDirect3D2 = nullptr;
    }
    if (pDirectDraw2 != nullptr) {
        pDirectDraw2->Release();
    }
    if (pDirectDraw != nullptr) {
        pDirectDraw->Release();
    }
    return hr;
}

void DirectDrawDriver::ShutdownDirectDrawDriver() {
    FlushDirect3DDeviceList();
    FlushDirectDrawModeEnumerationList();
    if (pszDescription != nullptr) {
        free(pszDescription);
        pszDescription = nullptr;
    }
    if (pszName != nullptr) {
        free(pszName);
        pszName = nullptr;
    }
    pPrev = nullptr;
    pNext = nullptr;
    dwFlags &= ~DIRECTDRAW_DRIVER_USABLE;
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

// Take an element off a list linked through pPrev and pNext, with the head and the tail of the
// list.
template <typename T>
static inline void UnlinkListNode(T *pNode, T **ppHead, T **ppTail) {
    if (pNode->pPrev != nullptr) {
        pNode->pPrev->pNext = pNode->pNext;
    } else {
        *ppHead = pNode->pNext;
    }
    if (pNode->pNext != nullptr) {
        pNode->pNext->pPrev = pNode->pPrev;
    } else {
        *ppTail = pNode->pPrev;
    }
    pNode->pPrev = nullptr;
    pNode->pNext = nullptr;
}

HRESULT DirectDrawDriver::EnumerateDisplayModes(IDirectDraw2 *pDirectDraw2) {
    DriverEnumContext context = {true, 0, this};
    HRESULT hr;

    if (dwFlags & 8) {
        return S_OK;
    }
    if (pDirectDraw2 == nullptr) {
        return RENDERER_E_NOT_CREATED;
    }
    hr = pDirectDraw2->EnumDisplayModes(0, nullptr, &context, EnumDisplayModesCallback_AddMode);
    if (FAILED(hr)) {
        return hr;
    }
    // Each mode DirectDraw reported must be in the list.
    if (!context.bSucceeded || context.nFound == 0 || nDisplayModeCount != context.nFound) {
        return RENDERER_E_NOT_IN_GDI_MODE;
    }
    dwFlags |= 8;
    return S_OK;
}

void DirectDrawDriver::FlushDirectDrawModeEnumerationList() {
    DisplayMode *pMode = pDisplayModesHead;

    while (pMode != nullptr) {
        DisplayMode *pNextMode = pMode->pNext;

        UnlinkListNode(pMode, &pDisplayModesHead, &pDisplayModesTail);
        delete pMode;
        pMode = pNextMode;
    }
    nDisplayModeCount = 0;
    pDisplayModesHead = nullptr;
    pDisplayModesTail = nullptr;
    dwFlags &= ~8;
}

HRESULT DirectDrawDriver::EnumerateD3DDevices(IDirect3D2 *pDirect3D2) {
    DriverEnumContext context = {true, 0, this};
    HRESULT hr;

    if (dwFlags & 0x10) {
        return S_OK;
    }
    if (pDirect3D2 == nullptr) {
        return RENDERER_E_NOT_CREATED;
    }
    hr = pDirect3D2->EnumDevices(EnumD3DDevicesCallback, &context);
    if (FAILED(hr)) {
        return hr;
    }
    // Each device Direct3D reported must be in the list.
    if (!context.bSucceeded || context.nFound == 0 || nD3DDeviceCount != context.nFound) {
        return RENDERER_E_NOT_IN_GDI_MODE;
    }
    dwFlags |= 0x10;
    return S_OK;
}

int DirectDrawDriver::FlushDirect3DDeviceList() {
    D3DDeviceNode *pDevice = pD3DDevicesHead;

    while (pDevice != nullptr) {
        D3DDeviceNode *pNextDevice = pDevice->pNext;

        UnlinkListNode(pDevice, &pD3DDevicesHead, &pD3DDevicesTail);
        pDevice->ReleaseD3DDeviceNode();
        delete pDevice;
        pDevice = pNextDevice;
    }
    nD3DDeviceCount = 0;
    pD3DDevicesHead = nullptr;
    pD3DDevicesTail = nullptr;
    dwFlags &= ~0x10;
    return 0;
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
    DriverEnumContext context = {true, 0, nullptr};
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

HRESULT AppendDirectDrawDriverNode(DirectDrawDriver *pDriver) {
    if (pDriver == nullptr) {
        return E_INVALIDARG;
    }
    pDriver->pNext = nullptr;
    pDriver->pPrev = g_pDirectDrawDriverListTail;
    if (g_pDirectDrawDriverListTail != nullptr) {
        g_pDirectDrawDriverListTail->pNext = pDriver;
    }
    g_pDirectDrawDriverListTail = pDriver;
    if (g_pDirectDrawDriverListHead == nullptr) {
        g_pDirectDrawDriverListHead = pDriver;
    }
    g_dwDirectDrawDriverCount++;
    return S_OK;
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
