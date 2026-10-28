#include <concepts>
#include <cstdlib>
#include <cstring>
#include <new>

#include "platform/win/d3d/renderer.h"

namespace {
/**
 * The number of DirectDraw drivers found.
 *
 * @ghidraAddress BRD3D: 0x104bbe1c
 */
int g_dwDirectDrawDriverCount;
} // namespace

/** @ghidraAddress BRD3D: 0x104bbe18 */
unsigned char g_bDirectDrawDriversEnumerated;

DirectDrawDriver *g_pDirectDrawDriverListHead;

namespace {
// @ghidraAddress BRD3D: 0x104bbe24
DirectDrawDriver *g_pDirectDrawDriverListTail;
} // namespace

// The name and the description a driver or a device gets when none is reported.
#define UNKNOWN_DRIVER_TEXT "UNKNOWN"

// Copy a text of a driver or a device into memory of its own.
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

// Take an element off a list linked through pPrev and pNext, with the head and the tail of the
// list.
template <typename T>
concept LinkedDriverNode = requires(T *node) {
    { node->GetPrevious() } -> std::same_as<T *>;
    { node->GetNext() } -> std::same_as<T *>;
    { node->SetPrevious(node) } -> std::same_as<void>;
    { node->SetNext(node) } -> std::same_as<void>;
};

template <LinkedDriverNode T>
static inline void UnlinkListNode(T *pNode, T **ppHead, T **ppTail) {
    if (pNode->GetPrevious() != nullptr) {
        pNode->GetPrevious()->SetNext(pNode->GetNext());
    } else {
        *ppHead = pNode->GetNext();
    }
    if (pNode->GetNext() != nullptr) {
        pNode->GetNext()->SetPrevious(pNode->GetPrevious());
    } else {
        *ppTail = pNode->GetPrevious();
    }
    pNode->SetPrevious(nullptr);
    pNode->SetNext(nullptr);
}

// Add an element to the end of a list linked through pPrev and pNext, and count it.
template <LinkedDriverNode T>
static inline HRESULT AppendListNode(T *pNode, T **ppHead, T **ppTail, int *pnCount) {
    if (pNode == nullptr) {
        return E_INVALIDARG;
    }
    pNode->SetNext(nullptr);
    pNode->SetPrevious(*ppTail);
    if (*ppTail != nullptr) {
        (*ppTail)->SetNext(pNode);
    }
    *ppTail = pNode;
    if (*ppHead == nullptr) {
        *ppHead = pNode;
    }
    (*pnCount)++;
    return S_OK;
}

DirectDrawDriver *FindDirectDrawDriverOrDefault(GUID *pGuid) {
    DirectDrawDriver *pPrimary;
    DirectDrawDriver *pDriver = FindDirectDrawDriverByGuid(pGuid, &pPrimary, nullptr);

    return pDriver != nullptr ? pDriver : pPrimary;
}

DisplayMode *FindDisplayModeWithFallback(DirectDrawDriver *pDriver,
                                         int nWidth,
                                         int nHeight,
                                         int nBitDepth,
                                         int nUnknown,
                                         D3DDeviceNode *pDevice) {
    DisplayMode *pFallback;
    DisplayMode *pMode;

    if (pDriver == nullptr) {
        return nullptr;
    }
    if (pDevice == nullptr) {
        pMode = pDriver->FindDisplayMode(
            nWidth, nHeight, nBitDepth, reinterpret_cast<void *>(nUnknown), &pFallback, nullptr);
    } else {
        pMode = pDriver->FindDisplayModeForDevice(nWidth,
                                                  nHeight,
                                                  nBitDepth,
                                                  reinterpret_cast<void *>(nUnknown),
                                                  pDevice,
                                                  &pFallback,
                                                  nullptr);
    }
    return pMode != nullptr ? pMode : pFallback;
}

bool FindDesktopDisplayModeAndDevice(DirectDrawDriver *pDriver,
                                     GUID *pDeviceGuid,
                                     DisplayMode **ppMode,
                                     D3DDeviceNode **ppDevice) {
    HWND hDesktop;
    HDC hdc;
    int nWidth;
    int nHeight;
    int nBitDepth;
    DisplayMode *pMode;
    D3DDeviceNode *pDevice;
    D3DDeviceNode *pFallback;

    if (pDriver == nullptr || ppMode == nullptr || ppDevice == nullptr) {
        return false;
    }
    hDesktop = GetDesktopWindow();
    hdc = GetDC(hDesktop);
    nWidth = GetDeviceCaps(hdc, HORZRES);
    nHeight = GetDeviceCaps(hdc, VERTRES);
    nBitDepth = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
    ReleaseDC(hDesktop, hdc);
    pMode = pDriver->FindDisplayMode(nWidth, nHeight, nBitDepth, nullptr, nullptr, nullptr);
    if (pMode == nullptr) {
        return false;
    }
    pDevice = pDriver->FindRenderDevice(pDeviceGuid, pMode, &pFallback, nullptr);
    if (pDevice == nullptr) {
        pDevice = pFallback;
        if (pDevice == nullptr) {
            return false;
        }
    }
    *ppMode = pMode;
    *ppDevice = pDevice;
    return true;
}

D3DDeviceNode *
ResolveRenderDeviceOrFind(DirectDrawDriver *pDriver, GUID *pDeviceGuid, DisplayMode *pMode) {
    if (pDriver == nullptr) {
        return nullptr;
    }
    union {
        GUID *guid;
        D3DDeviceNode *device;
    } fallback = {pDeviceGuid};
    D3DDeviceNode *device;

    if (pMode == nullptr) {
        device = pDriver->SelectPreferredD3DDevice(pDeviceGuid, &fallback.device, nullptr);
    } else {
        device = pDriver->FindRenderDevice(pDeviceGuid, pMode, &fallback.device, nullptr);
    }
    /* The fallback reuses the GUID argument slot, including when the query does not write it. */
    return device != nullptr ? device : fallback.device;
}

// The mode a renderer takes when it is not given a size and a depth.
#define DEFAULT_MODE_WIDTH 640
#define DEFAULT_MODE_HEIGHT 480
#define DEFAULT_MODE_BIT_DEPTH 16

bool SelectDeviceAndDisplayMode(DirectDrawDriver *pDriver,
                                GUID *pDeviceGuid,
                                int nWidth,
                                int nHeight,
                                int nBitDepth,
                                void *pSurfaceFormat,
                                DisplayMode **ppMode,
                                D3DDeviceNode **ppDevice) {
    D3DDeviceNode *pDevice;
    D3DDeviceNode *pPreferred;
    DisplayMode *pMode;
    DisplayMode *pFallback;

    if (pDriver == nullptr || ppMode == nullptr || ppDevice == nullptr) {
        return false;
    }
    pDevice = pDriver->SelectPreferredD3DDevice(pDeviceGuid, &pPreferred, nullptr);
    if (pDevice == nullptr) {
        pDevice = pPreferred;
        if (pDevice == nullptr) {
            return false;
        }
    }
    if (nWidth == 0 || nHeight == 0 || nBitDepth == 0) {
        nWidth = DEFAULT_MODE_WIDTH;
        nHeight = DEFAULT_MODE_HEIGHT;
        nBitDepth = DEFAULT_MODE_BIT_DEPTH;
    }
    pMode = pDriver->FindDisplayModeForDevice(
        nWidth, nHeight, nBitDepth, pSurfaceFormat, pDevice, &pFallback, nullptr);
    if (pMode == nullptr) {
        pMode = pFallback;
        if (pMode == nullptr) {
            return false;
        }
    }
    *ppMode = pMode;
    *ppDevice = pDevice;
    return true;
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

DWORD BitDepthToDDBD(int nBitDepth) {
    switch (nBitDepth) {
    case 1:
        return DDBD_1;
    case 2:
        return DDBD_2;
    case 4:
        return DDBD_4;
    case 8:
        return DDBD_8;
    case 16:
        return DDBD_16;
    case 24:
        return DDBD_24;
    case 32:
        return DDBD_32;
    default:
        return 0;
    }
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

HRESULT WINAPI EnumDisplayModesCallback_AddMode(DDSURFACEDESC *pDesc, LPVOID pContext) {
    DriverEnumContext *pEnum = static_cast<DriverEnumContext *>(pContext);
    DisplayMode *pMode;

    if (pEnum == nullptr || pEnum->pOwner == nullptr) {
        return DDENUMRET_OK;
    }
    // A description of another size stops the search.
    if (pDesc == nullptr || pDesc->dwSize != sizeof(DDSURFACEDESC)) {
        return DDENUMRET_CANCEL;
    }
    pMode = new (std::nothrow) DisplayMode;
    if (pMode == nullptr) {
        return DDENUMRET_OK;
    }
    pMode->SetDescription(*pDesc);
    // A mode that is not added is not freed.
    if (SUCCEEDED(
            static_cast<DirectDrawDriver *>(pEnum->pOwner)->EnqueueDirectDrawModeNode(pMode))) {
        pEnum->nFound++;
    }
    return DDENUMRET_OK;
}

HRESULT WINAPI EnumDeviceTextureFormatsCallback_AddFormat(DDSURFACEDESC *pDesc, LPVOID pContext) {
    DriverEnumContext *pEnum = static_cast<DriverEnumContext *>(pContext);
    DisplayMode *pFormat;

    if (pEnum == nullptr || pEnum->pOwner == nullptr) {
        return D3DENUMRET_OK;
    }
    // A description of another size stops the search.
    if (pDesc == nullptr || pDesc->dwSize != sizeof(DDSURFACEDESC)) {
        return D3DENUMRET_CANCEL;
    }
    pFormat = new (std::nothrow) DisplayMode;
    if (pFormat == nullptr) {
        return D3DENUMRET_OK;
    }
    pFormat->SetDescription(*pDesc);
    // A format that is not added is not freed.
    if (SUCCEEDED(static_cast<D3DDeviceNode *>(pEnum->pOwner)->EnqueueTextureFormatNode(pFormat))) {
        pEnum->nFound++;
    }
    return D3DENUMRET_OK;
}

HRESULT WINAPI EnumD3DDevicesCallback(GUID *pGuid,
                                      LPSTR pszDescription,
                                      LPSTR pszName,
                                      D3DDEVICEDESC *pHardwareDesc,
                                      D3DDEVICEDESC *pSoftwareDesc,
                                      LPVOID pContext) {
    DriverEnumContext *pEnum = static_cast<DriverEnumContext *>(pContext);
    D3DDeviceNode *pDevice;

    if (pEnum == nullptr || pEnum->pOwner == nullptr) {
        return D3DENUMRET_OK;
    }
    pDevice = new (std::nothrow) D3DDeviceNode;
    if (pDevice == nullptr) {
        return D3DENUMRET_OK;
    }
    // A device that is not read or not added is not freed.
    if (SUCCEEDED(pDevice->InitD3DDeviceNode(
            pGuid, pszDescription, pszName, pHardwareDesc, pSoftwareDesc)) &&
        SUCCEEDED(
            static_cast<DirectDrawDriver *>(pEnum->pOwner)->AppendDirect3DDeviceNode(pDevice))) {
        pEnum->nFound++;
    }
    return D3DENUMRET_OK;
}

int DisplayMode::GetSurfaceDescBitDepth() {
    if (desc.dwSize != sizeof(desc) || (desc.dwFlags & DDSD_PIXELFORMAT) == 0 ||
        desc.ddpfPixelFormat.dwSize != sizeof(desc.ddpfPixelFormat)) {
        return 0;
    }
    return desc.ddpfPixelFormat.dwRGBBitCount;
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

bool DisplayMode::IsDisplayModeSupportedByDevice(D3DDeviceNode *pDevice) {
    DWORD dwDepth;

    if (pDevice == nullptr) {
        return false;
    }
    dwDepth = BitDepthToDDBD(GetSurfaceDescBitDepth());
    if (pDevice->IsHardwareDevice()) {
        return (pDevice->GetHardwareDescription()->dwDeviceRenderBitDepth & dwDepth) != 0;
    }
    return (pDevice->GetSoftwareDescription()->dwDeviceRenderBitDepth & dwDepth) != 0;
}

bool DisplayMode::IsDisplayModeMatch(int nWidth, int nHeight, int nBitDepth) {
    return desc.dwSize == sizeof(desc) && static_cast<int>(desc.dwWidth) == nWidth &&
           static_cast<int>(desc.dwHeight) == nHeight &&
           static_cast<int>(desc.ddpfPixelFormat.dwRGBBitCount) == nBitDepth;
}

bool D3DDeviceNode::IsHardwareDevice() {
    return hardwareDesc.dcmColorModel != 0;
}

bool D3DDeviceNode::CompareD3DDeviceGuid(const GUID *pGuid) {
    return pGuid != nullptr && (dwFlags & 1) != 0 && memcmp(&guid, pGuid, sizeof(guid)) == 0;
}

HRESULT D3DDeviceNode::InitD3DDeviceNode(GUID *pGuid,
                                         LPSTR pszNewDescription,
                                         LPSTR pszNewName,
                                         D3DDEVICEDESC *pHardwareDesc,
                                         D3DDEVICEDESC *pSoftwareDesc) {
    if (pGuid == nullptr) {
        return E_INVALIDARG;
    }
    guid = *pGuid;
    pszDescription = CopyDriverText(pszNewDescription);
    pszName = CopyDriverText(pszNewName);
    if (pHardwareDesc != nullptr) {
        hardwareDesc = *pHardwareDesc;
    }
    if (pSoftwareDesc != nullptr) {
        softwareDesc = *pSoftwareDesc;
    }
    // Bit 0 marks a device that was read, and bit 1 one whose formats were found.
    dwFlags = (dwFlags & ~2u) | 1;
    nTextureFormatCount = 0;
    return S_OK;
}

void D3DDeviceNode::ReleaseD3DDeviceNode() {
    FlushTextureFormatList();
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
}

HRESULT D3DDeviceNode::EnumerateDeviceTextureFormats(IDirect3DDevice2 *pD3DDevice) {
    DriverEnumContext context = {true, 0, this};
    HRESULT hr;

    if (dwFlags & 2) {
        return S_OK;
    }
    if (pD3DDevice == nullptr) {
        return RENDERER_E_NOT_CREATED;
    }
    hr = pD3DDevice->EnumTextureFormats(EnumDeviceTextureFormatsCallback_AddFormat, &context);
    if (FAILED(hr)) {
        return hr;
    }
    // Each format Direct3D reported must be in the list.
    if (!context.bSucceeded || context.nFound == 0 || nTextureFormatCount != context.nFound) {
        return RENDERER_E_NOT_IN_GDI_MODE;
    }
    dwFlags |= 2;
    return S_OK;
}

void D3DDeviceNode::FlushTextureFormatList() {
    DisplayMode *pFormat;

    if ((dwFlags & 2) == 0) {
        return;
    }
    pFormat = pTextureFormatsHead;
    while (pFormat != nullptr) {
        DisplayMode *pNextFormat = pFormat->GetNext();

        UnlinkListNode(pFormat, &pTextureFormatsHead, &pTextureFormatsTail);
        delete pFormat;
        pFormat = pNextFormat;
    }
    nTextureFormatCount = 0;
    pTextureFormatsHead = nullptr;
    pTextureFormatsTail = nullptr;
    dwFlags &= ~2u;
}

HRESULT D3DDeviceNode::EnqueueTextureFormatNode(DisplayMode *pFormat) {
    return AppendListNode(
        pFormat, &pTextureFormatsHead, &pTextureFormatsTail, &nTextureFormatCount);
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
        DisplayMode *pNextMode = pMode->GetNext();

        UnlinkListNode(pMode, &pDisplayModesHead, &pDisplayModesTail);
        delete pMode;
        pMode = pNextMode;
    }
    nDisplayModeCount = 0;
    pDisplayModesHead = nullptr;
    pDisplayModesTail = nullptr;
    dwFlags &= ~8;
}

HRESULT DirectDrawDriver::EnqueueDirectDrawModeNode(DisplayMode *pMode) {
    return AppendListNode(pMode, &pDisplayModesHead, &pDisplayModesTail, &nDisplayModeCount);
}

// The mode the search of a mode falls back to.
#define FALLBACK_SEARCH_WIDTH 640
#define FALLBACK_SEARCH_HEIGHT 480
#define FALLBACK_SEARCH_BIT_DEPTH 8

DisplayMode *DirectDrawDriver::FindDisplayMode(int nWidth,
                                               int nHeight,
                                               int nBitDepth,
                                               void *pUnused,
                                               DisplayMode **ppFallback,
                                               DisplayMode *pFirst) {
    DisplayMode *pMode = pFirst != nullptr ? pFirst : pDisplayModesHead;

    if (ppFallback != nullptr) {
        *ppFallback = pMode;
    }
    while (pMode != nullptr) {
        DisplayMode *pNextMode = pMode->GetNext();

        if (pMode->IsDisplayModeMatch(nWidth, nHeight, nBitDepth)) {
            return pMode;
        }
        if (pMode->IsDisplayModeMatch(
                FALLBACK_SEARCH_WIDTH, FALLBACK_SEARCH_HEIGHT, FALLBACK_SEARCH_BIT_DEPTH) &&
            ppFallback != nullptr) {
            *ppFallback = pMode;
        }
        pMode = pNextMode;
    }
    return nullptr;
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
        D3DDeviceNode *pNextDevice = pDevice->GetNext();

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

HRESULT DirectDrawDriver::AppendDirect3DDeviceNode(D3DDeviceNode *pDevice) {
    return AppendListNode(pDevice, &pD3DDevicesHead, &pD3DDevicesTail, &nD3DDeviceCount);
}

D3DDeviceNode *DirectDrawDriver::SelectPreferredD3DDevice(GUID *pGuid,
                                                          D3DDeviceNode **ppPreferred,
                                                          D3DDeviceNode *pFirst) {
    D3DDeviceNode *pDevice;
    D3DDeviceNode *pHardware = nullptr;
    D3DDeviceNode *pMmx = nullptr;
    D3DDeviceNode *pRgb = nullptr;

    if (ppPreferred != nullptr) {
        *ppPreferred = nullptr;
    }
    pFirst = pFirst != nullptr ? pFirst : pD3DDevicesHead;
    for (pDevice = pFirst; pDevice != nullptr;) {
        D3DDeviceNode *pNextDevice = pDevice->GetNext();

        if (pDevice->CompareD3DDeviceGuid(pGuid)) {
            return pDevice;
        }
        if (pDevice->IsHardwareDevice() && pHardware == nullptr) {
            pHardware = pDevice;
        }
        if (IsEqualGUID(*pDevice->GetGuid(), IID_IDirect3DRGBDevice) && pRgb == nullptr) {
            pRgb = pDevice;
        }
        if (IsEqualGUID(*pDevice->GetGuid(), IID_IDirect3DMMXDevice) && pMmx == nullptr) {
            pMmx = pDevice;
        }
        pDevice = pNextDevice;
    }
    if (ppPreferred != nullptr) {
        if (pHardware != nullptr) {
            *ppPreferred = pHardware;
        } else if (pRgb != nullptr) {
            *ppPreferred = pRgb;
        } else if (pMmx != nullptr) {
            *ppPreferred = pMmx;
        } else if (pFirst != nullptr) {
            *ppPreferred = pFirst;
        }
    }
    return nullptr;
}

D3DDeviceNode *DirectDrawDriver::FindRenderDevice(GUID *pGuid,
                                                  DisplayMode *pMode,
                                                  D3DDeviceNode **ppFallback,
                                                  D3DDeviceNode *pFirst) {
    D3DDeviceNode *pDevice;

    if (pMode == nullptr) {
        if (ppFallback != nullptr) {
            *ppFallback = nullptr;
        }
        return nullptr;
    }
    pDevice = pFirst != nullptr ? pFirst : pD3DDevicesHead;
    if (ppFallback != nullptr && pMode->IsDisplayModeSupportedByDevice(pDevice)) {
        *ppFallback = pDevice;
    }
    while (pDevice != nullptr) {
        D3DDeviceNode *pNextDevice = pDevice->GetNext();

        if (pDevice->CompareD3DDeviceGuid(pGuid)) {
            if (pMode->IsDisplayModeSupportedByDevice(pDevice)) {
                return pDevice;
            }
        } else if (pDevice->IsHardwareDevice()) {
            // A hardware device tests the fallback before the mode, and a software device after.
            if (ppFallback != nullptr && pMode->IsDisplayModeSupportedByDevice(pDevice)) {
                *ppFallback = pDevice;
            }
        } else if (pMode->IsDisplayModeSupportedByDevice(pDevice) && ppFallback != nullptr) {
            *ppFallback = pDevice;
        }
        pDevice = pNextDevice;
    }
    return nullptr;
}

DisplayMode *DirectDrawDriver::FindDisplayModeForDevice(int nWidth,
                                                        int nHeight,
                                                        int nBitDepth,
                                                        void *pUnused,
                                                        D3DDeviceNode *pDevice,
                                                        DisplayMode **ppFallback,
                                                        DisplayMode *pFirst) {
    DisplayMode *pMode;

    if (pDevice == nullptr) {
        if (ppFallback != nullptr) {
            *ppFallback = nullptr;
        }
        return nullptr;
    }
    pMode = pFirst != nullptr ? pFirst : pDisplayModesHead;
#ifdef ENABLE_PATCHES
    if (ppFallback != nullptr && pMode != nullptr &&
        pMode->IsDisplayModeSupportedByDevice(pDevice)) {
        *ppFallback = pMode;
    }
#else
    // The original reads the first mode of an empty list as well.
    if (ppFallback != nullptr && pMode->IsDisplayModeSupportedByDevice(pDevice)) {
        *ppFallback = pMode;
    }
#endif
    while (pMode != nullptr) {
        DisplayMode *pNextMode = pMode->GetNext();

        if (pMode->IsDisplayModeMatch(nWidth, nHeight, nBitDepth)) {
            if (pMode->IsDisplayModeSupportedByDevice(pDevice)) {
                return pMode;
            }
        } else if (pMode->IsDisplayModeMatch(640, 480, 16)) {
            // The two fallback branches differ only in the order of their tests.
            if (ppFallback != nullptr && pMode->IsDisplayModeSupportedByDevice(pDevice)) {
                *ppFallback = pMode;
            }
        } else if (pMode->IsDisplayModeSupportedByDevice(pDevice) && ppFallback != nullptr) {
            *ppFallback = pMode;
        }
        pMode = pNextMode;
    }
    return nullptr;
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
    return AppendListNode(pDriver,
                          &g_pDirectDrawDriverListHead,
                          &g_pDirectDrawDriverListTail,
                          &g_dwDirectDrawDriverCount);
}

DirectDrawDriver *
FindDirectDrawDriverByGuid(GUID *pGuid, DirectDrawDriver **ppPrimary, DirectDrawDriver *pFirst) {
    DirectDrawDriver *pDriver = pFirst != nullptr ? pFirst : g_pDirectDrawDriverListHead;

    if (ppPrimary != nullptr) {
        *ppPrimary = pDriver;
    }
    while (pDriver != nullptr) {
        DirectDrawDriver *pNext = pDriver->GetNext();

        if (pDriver->CompareDirectDrawDriverGuid(pGuid)) {
            return pDriver;
        }
        if ((pDriver->GetFlags() & DIRECTDRAW_DRIVER_PRIMARY) != 0 && ppPrimary != nullptr) {
            *ppPrimary = pDriver;
        }
        pDriver = pNext;
    }
    return nullptr;
}
