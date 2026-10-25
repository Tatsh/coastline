#include "platform/win/d3d/renderer.h"

#include <cstring>
#include <new>

#include "platform/win/boot/app.h"

RenderHost *g_pRenderHost;
IDirect3D2 *g_pDirect3D2;
IDirect3DDevice2 *g_pD3DDevice2;

RenderHost::RenderHost() : pRenderer(nullptr) {
}

HRESULT RenderHost::SetRenderer(DirectDrawRenderer *pNewRenderer) {
    if (pNewRenderer == nullptr) {
        return RENDERER_E_NOT_CREATED;
    }
    pRenderer = pNewRenderer;
    return S_OK;
}

HRESULT RenderHost::ValidateRenderDeviceState() {
    if (pRenderer == nullptr ||
        (pRenderer->dwRenderStateFlags & RENDERER_STATE_READY) != RENDERER_STATE_READY) {
        return RENDERER_E_NOT_READY;
    }
    return ValidateObjectRenderResources();
}

int RenderHost::OnDestroyDevice() {
    ReturnZero();
    return 0;
}

int RenderHost::ReloadTextureSurfacesAfterRestore() {
    InitializeAllTextureSurfaces();
    if (g_dwNullStubEnable) {
        NullStub();
    }
    return 0;
}

HRESULT RenderHost::ValidateObjectRenderResources() {
    if (pRenderer == nullptr ||
        (pRenderer->dwRenderStateFlags & RENDERER_STATE_READY) != RENDERER_STATE_READY ||
        pRenderer->pDirect3D2 == nullptr || pRenderer->pD3DDevice == nullptr ||
        pRenderer->pViewport == nullptr) {
        return E_FAIL;
    }
    return S_OK;
}

void D3DMaterial::Release() {
    if (pMaterial != nullptr) {
        pMaterial->Release();
        pMaterial = nullptr;
    }
}

DirectDrawRenderer::DirectDrawRenderer() {
    memset(this, 0, sizeof(*this));
    dwSize = sizeof(*this);
    dwFlags |= 4;
}

void DirectDrawRenderer::Cleanup() {
    DestroyDirectDrawRendererObjects();
    pDriver = nullptr;
    pMode = nullptr;
    pDevice = nullptr;
    hWnd = nullptr;
    pHost = nullptr;
}

HRESULT DirectDrawRenderer::InitializeWindow(HWND hNewWnd,
                                             GUID *pDriverGuid,
                                             int nWidth,
                                             int nHeight,
                                             int nBitDepth,
                                             void *pSurfaceFormat,
                                             GUID *pDeviceGuid,
                                             bool bOption) {
    HRESULT hr;

    if (hNewWnd == nullptr || !IsWindow(hNewWnd)) {
        return RENDERER_E_NOT_CREATED;
    }
    hWnd = hNewWnd;
    if (bOption) {
        dwFlags |= 4;
    } else {
        dwFlags &= ~4;
    }
    hr = SelectDriverDeviceAndModeFromConfig(pDriverGuid,
                                             nWidth,
                                             nHeight,
                                             nBitDepth,
                                             pSurfaceFormat,
                                             pDeviceGuid,
                                             true,
                                             &pDriver,
                                             &pMode,
                                             &pDevice);
    if (FAILED(hr)) {
        return hr;
    }
    hr = CreateDirectDrawObjects();
    if (FAILED(hr)) {
        return hr;
    }
    // The steps after the DirectDraw objects release everything when one fails.
    if (FAILED(hr = SetCooperativeLevel()) || FAILED(hr = SetDisplayMode()) ||
        FAILED(hr = CreatePrimaryFlipChain()) || FAILED(hr = InitializeRenderDevice())) {
        DestroyDirectDrawRendererObjects();
        return hr;
    }
    SendMessageA(hWnd, WM_RENDERER_CREATED, 0, reinterpret_cast<LPARAM>(this));
    return S_OK;
}

int DirectDrawRenderer::DestroyDirectDrawRendererObjects() {
    SendMessageA(hWnd, WM_RENDERER_DESTROYING, 0, reinterpret_cast<LPARAM>(this));
    DestroyD3DDevice();
    ReleasePrimarySurface();
    RestoreDisplayMode();
    ReleaseDirectDrawObjects();
    return 0;
}

HRESULT DirectDrawRenderer::CreateDirectDrawObjects() {
    HRESULT hr;

    if (pDriver == nullptr) {
        pDriver = FindDirectDrawDriverOrDefault(nullptr);
        if (pDriver == nullptr) {
            return RENDERER_E_NOT_READY;
        }
    }
    hr = DirectDrawCreate(pDriver->GetDirectDrawDriverGuid(), &pDirectDraw, nullptr);
    if (FAILED(hr)) {
        ReleaseDirectDrawObjects();
        return hr;
    }
    hr = pDirectDraw->QueryInterface(IID_IDirectDraw2, reinterpret_cast<void **>(&pDirectDraw2));
    if (FAILED(hr)) {
        MessageBoxA(hWnd, "This game requires DirectX 6.", "Boss Rally Error", MB_ICONERROR);
        ReleaseDirectDrawObjects();
        return hr;
    }
    hr = pDirectDraw->QueryInterface(IID_IDirect3D2, reinterpret_cast<void **>(&pDirect3D2));
    if (FAILED(hr)) {
        ReleaseDirectDrawObjects();
        return hr;
    }
    g_pDirect3D2 = pDirect3D2;
    dwRenderStateFlags |= 1;
    return S_OK;
}

int DirectDrawRenderer::ReleaseDirectDrawObjects() {
    dwRenderStateFlags &= ~1;
    if (pDirect3D2 != nullptr) {
        pDirect3D2->Release();
        pDirect3D2 = nullptr;
        g_pDirect3D2 = nullptr;
    }
    if (pDirectDraw2 != nullptr) {
        pDirectDraw2->Release();
        pDirectDraw2 = nullptr;
    }
    if (pDirectDraw != nullptr) {
        pDirectDraw->Release();
        pDirectDraw = nullptr;
    }
    return 0;
}

HRESULT DirectDrawRenderer::SetCooperativeLevel() {
    HRESULT hr;

    if (hWnd == nullptr || !IsWindow(hWnd)) {
        return RENDERER_E_NOT_READY;
    }
    hr = pDirectDraw2->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
    return FAILED(hr) ? hr : S_OK;
}

// The mode a display falls back to, and the depth after that.
#define FALLBACK_MODE_WIDTH 640
#define FALLBACK_MODE_HEIGHT 480
#define FALLBACK_MODE_BIT_DEPTH 16

// Record the display mode that was set and the viewport that covers it.
static inline void
RecordDisplayMode(DirectDrawRenderer *pRenderer, int nWidth, int nHeight, int nBitDepth) {
    pRenderer->rcViewport.left = 0;
    pRenderer->rcViewport.top = 0;
    pRenderer->rcViewport.right = nWidth;
    pRenderer->rcViewport.bottom = nHeight;
    g_dwConfigFramebufferWidth = nWidth;
    g_dwScreenWidth = nWidth;
    g_dwConfigFramebufferHeight = nHeight;
    g_dwScreenHeight = nHeight;
    g_dwScreenBitDepth = nBitDepth;
    pRenderer->dwRenderStateFlags |= 2;
}

HRESULT DirectDrawRenderer::SetDisplayMode() {
    int nWidth;
    int nHeight;
    int nBitDepth;
    int nRefreshRate;
    HRESULT hr;

    if (pMode == nullptr || pDirectDraw2 == nullptr) {
        return RENDERER_E_NOT_READY;
    }
    pMode->GetSurfaceDescDimensions(&nWidth, &nHeight, &nBitDepth, &nRefreshRate);
    hr = pDirectDraw2->SetDisplayMode(
        nWidth,
        nHeight,
        nBitDepth,
        nRefreshRate,
        nWidth == 320 && nHeight == 200 && nBitDepth == 8 ? DDSDM_STANDARDVGAMODE : 0);
    if (SUCCEEDED(hr)) {
        RecordDisplayMode(this, nWidth, nHeight, nBitDepth);
        return hr;
    }
    // A mode that is not set falls back to 640 by 480 at its depth, then to its size at 16 bits.
    // The mode of the renderer is replaced by each mode tried.
    if (nWidth != FALLBACK_MODE_WIDTH || nHeight != FALLBACK_MODE_HEIGHT) {
        nWidth = FALLBACK_MODE_WIDTH;
        nHeight = FALLBACK_MODE_HEIGHT;
        pMode = FindDisplayModeWithFallback(pDriver, nWidth, nHeight, nBitDepth, 0, pDevice);
        if (pMode != nullptr) {
            HRESULT hrFallback = pDirectDraw2->SetDisplayMode(nWidth, nHeight, nBitDepth, 0, 0);

            if (SUCCEEDED(hrFallback)) {
                RecordDisplayMode(this, nWidth, nHeight, nBitDepth);
                return hrFallback;
            }
            hr = hrFallback;
        }
    }
    if (nBitDepth == FALLBACK_MODE_BIT_DEPTH) {
        return hr;
    }
    nBitDepth = FALLBACK_MODE_BIT_DEPTH;
    pMode = FindDisplayModeWithFallback(pDriver, nWidth, nHeight, nBitDepth, 0, pDevice);
    if (pMode == nullptr) {
        return hr;
    }
    hr = pDirectDraw2->SetDisplayMode(nWidth, nHeight, nBitDepth, 0, 0);
    if (SUCCEEDED(hr)) {
        RecordDisplayMode(this, nWidth, nHeight, nBitDepth);
    }
    return hr;
}

int DirectDrawRenderer::RestoreDisplayMode() {
    dwRenderStateFlags &= ~2;
    if (pDirectDraw2 != nullptr) {
        pDirectDraw2->RestoreDisplayMode();
    }
    return 0;
}

HRESULT DirectDrawRenderer::CreatePrimaryFlipChain() {
    DDSURFACEDESC desc;
    HRESULT hr;

    if (pMode == nullptr || pDirectDraw2 == nullptr) {
        return RENDERER_E_NOT_READY;
    }
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
    desc.ddsCaps.dwCaps =
        DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
    desc.dwBackBufferCount = 1;
    hr = pDirectDraw2->CreateSurface(&desc, &pPrimarySurface, nullptr);
    if (FAILED(hr)) {
        return hr;
    }
    hr = InitializePrimaryPalette();
    if (FAILED(hr)) {
        return hr;
    }
    dwRenderStateFlags |= 4;
    return S_OK;
}

int DirectDrawRenderer::ReleasePrimarySurface() {
    dwRenderStateFlags &= ~4;
    ReleasePrimaryPalette();
    if (pPrimarySurface != nullptr) {
        pPrimarySurface->Release();
        pPrimarySurface = nullptr;
    }
    return 0;
}

// The most entries a palette of the front buffer has.
#define MAX_PALETTE_ENTRIES 256

// Mark the entries of a palette, from the first to the one before the last.
static inline void MarkPaletteEntries(PALETTEENTRY *pEntries, int nFirst, int nEnd, BYTE nFlags) {
    for (int i = nFirst; i < nEnd; i++) {
        pEntries[i].peFlags = nFlags;
    }
}

HRESULT DirectDrawRenderer::InitializePrimaryPalette() {
    DDSURFACEDESC desc;
    HRESULT hr;
    HDC hdc;
    DWORD dwCaps;

    ReleasePrimaryPalette();
    if (pDirectDraw2 == nullptr || pPrimarySurface == nullptr) {
        return RENDERER_E_NOT_READY;
    }
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    hr = pPrimarySurface->GetSurfaceDesc(&desc);
    if (FAILED(hr)) {
        return hr;
    }
    if (!IsPixelFormatPalettized(&desc.ddpfPixelFormat)) {
        return S_OK;
    }
    // The palette starts as the palette of the system, and a copy of it is saved for GDI.
    hdc = GetDC(nullptr);
    nPaletteEntryCount = GetDeviceCaps(hdc, SIZEPALETTE);
    if (nPaletteEntryCount != 0) {
        if (nPaletteEntryCount > MAX_PALETTE_ENTRIES) {
            nPaletteEntryCount = MAX_PALETTE_ENTRIES;
        }
        pGdiPaletteEntries = static_cast<PALETTEENTRY *>(operator new(
            nPaletteEntryCount * sizeof(PALETTEENTRY), std::nothrow));
        if (pGdiPaletteEntries == nullptr) {
            ReleaseDC(nullptr, hdc);
            ReleasePrimaryPalette();
            return RENDERER_E_OUT_OF_MEMORY;
        }
        pGamePaletteEntries = static_cast<PALETTEENTRY *>(operator new(
            nPaletteEntryCount * sizeof(PALETTEENTRY), std::nothrow));
        if (pGamePaletteEntries == nullptr) {
            ReleaseDC(nullptr, hdc);
            ReleasePrimaryPalette();
            return RENDERER_E_OUT_OF_MEMORY;
        }
        GetSystemPaletteEntries(hdc, 0, nPaletteEntryCount, pGdiPaletteEntries);
        memcpy(pGamePaletteEntries, pGdiPaletteEntries, nPaletteEntryCount * sizeof(PALETTEENTRY));
    }
    ReleaseDC(nullptr, hdc);
    // The entries the game may change are reserved, and the colours Windows reserves at the ends of
    // a palette of 4 or 8 bits are read-only. A display without a palette leaves the buffers
    // unallocated, and the original writes the marks regardless.
    if (desc.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED1) {
        MarkPaletteEntries(pGamePaletteEntries, 0, 2, PC_RESERVED);
        dwCaps = DDPCAPS_1BIT;
    } else if (desc.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED2) {
        MarkPaletteEntries(pGamePaletteEntries, 0, 4, PC_RESERVED);
        dwCaps = DDPCAPS_2BIT;
    } else if (desc.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4) {
        pGamePaletteEntries[0].peFlags = D3DPAL_READONLY;
        pGamePaletteEntries[15].peFlags = D3DPAL_READONLY;
        MarkPaletteEntries(pGamePaletteEntries, 1, 15, PC_RESERVED);
        dwCaps = DDPCAPS_4BIT;
    } else if (desc.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) {
        MarkPaletteEntries(pGamePaletteEntries, 0, 10, D3DPAL_READONLY);
        MarkPaletteEntries(pGamePaletteEntries, 246, 256, D3DPAL_READONLY);
        MarkPaletteEntries(pGamePaletteEntries, 10, 246, PC_RESERVED);
        dwCaps = DDPCAPS_8BIT;
    } else {
        ReleasePrimaryPalette();
        return RENDERER_E_NOT_IN_GDI_MODE;
    }
    hr = pDirectDraw2->CreatePalette(dwCaps, pGamePaletteEntries, &pPrimaryPalette, nullptr);
    if (SUCCEEDED(hr)) {
        hr = pPrimarySurface->SetPalette(pPrimaryPalette);
    }
    if (FAILED(hr)) {
        ReleasePrimaryPalette();
        return hr;
    }
    return S_OK;
}

int DirectDrawRenderer::ReleasePrimaryPalette() {
    if (pPrimaryPalette != nullptr) {
        pPrimaryPalette->Release();
        pPrimaryPalette = nullptr;
    }
    if (pGamePaletteEntries != nullptr) {
        operator delete(pGamePaletteEntries);
        pGamePaletteEntries = nullptr;
    }
    if (pGdiPaletteEntries != nullptr) {
        operator delete(pGdiPaletteEntries);
        pGdiPaletteEntries = nullptr;
    }
    nPaletteEntryCount = 0;
    return 0;
}

// Create a depth buffer of the size of the back buffer and attach it. Return whether it was.
static inline HRESULT CreateDepthBuffer(DirectDrawRenderer *pRenderer,
                                        const D3DDEVICEDESC *pDesc,
                                        DWORD dwMemoryCaps,
                                        bool *pbAttached) {
    DDSURFACEDESC backDesc;
    DDSURFACEDESC depthDesc;
    HRESULT hr;

    *pbAttached = false;
    memset(&backDesc, 0, sizeof(backDesc));
    backDesc.dwSize = sizeof(backDesc);
    hr = pRenderer->pBackBuffer->GetSurfaceDesc(&backDesc);
    if (FAILED(hr)) {
        return hr;
    }
    memset(&depthDesc, 0, sizeof(depthDesc));
    depthDesc.dwSize = sizeof(depthDesc);
    depthDesc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_ZBUFFERBITDEPTH;
    depthDesc.dwHeight = backDesc.dwHeight;
    depthDesc.dwWidth = backDesc.dwWidth;
    depthDesc.dwZBufferBitDepth = DDBDToBitDepth(pDesc->dwDeviceZBufferBitDepth);
    depthDesc.ddsCaps.dwCaps = dwMemoryCaps | DDSCAPS_ZBUFFER;
    // A depth buffer that is not created or not attached leaves the device without one.
    if (SUCCEEDED(
            pRenderer->pDirectDraw2->CreateSurface(&depthDesc, &pRenderer->pZBuffer, nullptr))) {
        if (FAILED(pRenderer->pBackBuffer->AddAttachedSurface(pRenderer->pZBuffer))) {
            if (pRenderer->pZBuffer != nullptr) {
                pRenderer->pZBuffer->Release();
                pRenderer->pZBuffer = nullptr;
            }
        } else {
            *pbAttached = true;
        }
    }
    return S_OK;
}

HRESULT DirectDrawRenderer::InitializeRenderDevice() {
    DDSCAPS caps;
    const D3DDEVICEDESC *pDesc;
    DWORD dwMemoryCaps;
    HRESULT hr;
    bool bAttached;

    if (hWnd == nullptr || !IsWindow(hWnd) || pDevice == nullptr || pMode == nullptr ||
        pDirectDraw2 == nullptr || pDirect3D2 == nullptr || pPrimarySurface == nullptr) {
        return RENDERER_E_NOT_READY;
    }
    caps.dwCaps = DDSCAPS_BACKBUFFER;
    hr = pPrimarySurface->GetAttachedSurface(&caps, &pBackBuffer);
    if (FAILED(hr)) {
        return hr;
    }
    // A device of the hardware puts its depth buffer in video memory.
    if (pDevice->IsHardwareDevice()) {
        pDesc = &pDevice->hardwareDesc;
        dwMemoryCaps = DDSCAPS_VIDEOMEMORY;
    } else {
        pDesc = &pDevice->softwareDesc;
        dwMemoryCaps = DDSCAPS_SYSTEMMEMORY;
    }
    dwDeviceInit = 0;
    if ((dwFlags & 4) != 0 && pDesc->dwDeviceZBufferBitDepth != 0) {
        hr = CreateDepthBuffer(this, pDesc, dwMemoryCaps, &bAttached);
        if (FAILED(hr)) {
            return hr;
        }
        dwDeviceInit = bAttached;
    }
    hr = pDirect3D2->CreateDevice(pDevice->guid, pBackBuffer, &pD3DDevice);
    if (FAILED(hr)) {
        return hr;
    }
    g_pD3DDevice2 = pD3DDevice;
    QueryD3DDeviceCaps(pD3DDevice);
    pDevice->EnumerateDeviceTextureFormats(pD3DDevice);
    dwRenderStateFlags |= 8;
    hr = CreateAndAttachViewport();
    if (FAILED(hr)) {
        return hr;
    }
    if (pHost != nullptr) {
        pHost->ValidateRenderDeviceState();
    }
    EnumerateTextureFormats(g_pD3DDevice2);
    return S_OK;
}

int DirectDrawRenderer::DestroyD3DDevice() {
    if (pHost != nullptr) {
        pHost->OnDestroyDevice();
    }
    ReleaseViewportAndMaterial();
    dwRenderStateFlags &= ~8;
    if (pD3DDevice != nullptr) {
        pD3DDevice->Release();
        pD3DDevice = nullptr;
        g_pD3DDevice2 = nullptr;
    }
    if (pZBuffer != nullptr) {
        if (pBackBuffer != nullptr) {
            pBackBuffer->DeleteAttachedSurface(0, pZBuffer);
        }
        pZBuffer->Release();
        pZBuffer = nullptr;
    }
    if (pBackBuffer != nullptr) {
        pBackBuffer->Release();
        pBackBuffer = nullptr;
    }
    return 0;
}

HRESULT DirectDrawRenderer::CreateAndAttachViewport() {
    HRESULT hr;

    if (pDirect3D2 == nullptr || pD3DDevice == nullptr) {
        return RENDERER_E_NOT_READY;
    }
    hr = pDirect3D2->CreateViewport(&pViewport, nullptr);
    if (FAILED(hr)) {
        return hr;
    }
    hr = pD3DDevice->AddViewport(pViewport);
    // A step after the add that fails takes the viewport off the device before it is released.
    if (SUCCEEDED(hr)) {
        if (FAILED(hr = InitializeViewportAndRenderContext()) || FAILED(hr = BeginRenderScene())) {
            pD3DDevice->DeleteViewport(pViewport);
        }
    }
    if (FAILED(hr)) {
        pViewport->Release();
        pViewport = nullptr;
        return hr;
    }
    dwRenderStateFlags |= 0x10;
    return S_OK;
}

int DirectDrawRenderer::ReleaseViewportAndMaterial() {
    // The original sets bit 4 of the state rather than clearing a bit.
    dwRenderStateFlags |= 0x10;
    if (pMaterial != nullptr) {
        pMaterial->Release();
        delete pMaterial;
        pMaterial = nullptr;
    }
    if (pViewport != nullptr) {
        pD3DDevice->DeleteViewport(pViewport);
        pViewport->Release();
        pViewport = nullptr;
    }
    return 0;
}

HRESULT DirectDrawRenderer::BeginRenderScene() {
    D3DRECT rect;
    HRESULT hr;

    rect.x1 = 0;
    rect.y1 = 0;
    rect.x2 = g_dwConfigFramebufferWidth;
    rect.y2 = g_dwConfigFramebufferHeight;

    // The background of the viewport is a black material, cleared to and shown once.
    pMaterial->desc.dcvDiffuse.r = 0.0f;
    pMaterial->desc.dcvDiffuse.g = 0.0f;
    pMaterial->desc.dcvDiffuse.b = 0.0f;
    pMaterial->nUnknown58 = 1;
    pMaterial->nUnknown58 = 0;
    pMaterial->desc.dwRampSize = 1;
    pMaterial->pMaterial->SetMaterial(&pMaterial->desc);
    pViewport->SetBackground(pMaterial->hMaterial);
    pViewport->Clear(1, &rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    hr = pPrimarySurface->Flip(pBackBuffer, DDFLIP_WAIT);
    while (pPrimarySurface->GetFlipStatus(DDGFS_ISFLIPDONE) != DD_OK) {
    }
    return hr;
}

HRESULT DirectDrawRenderer::SetPrimarySurfacePalette() {
    HRESULT hr;

    if (pPrimarySurface == nullptr || pPrimaryPalette == nullptr) {
        return S_OK;
    }
    hr = pPrimarySurface->SetPalette(pPrimaryPalette);
    return FAILED(hr) ? hr : S_OK;
}

HRESULT DirectDrawRenderer::RestoreGdiPaletteAndFlipToGdi() {
    HRESULT hr;

    if (pPrimaryPalette != nullptr) {
        hr = pPrimaryPalette->GetEntries(0, 0, nPaletteEntryCount, pGamePaletteEntries);
        if (FAILED(hr)) {
            return hr;
        }
        hr = pPrimaryPalette->SetEntries(0, 0, nPaletteEntryCount, pGdiPaletteEntries);
        if (FAILED(hr)) {
            return hr;
        }
    }
    if (pDirectDraw2 != nullptr) {
        hr = pDirectDraw2->FlipToGDISurface();
        if (FAILED(hr)) {
            return hr;
        }
    }
    if (hWnd != nullptr && IsWindow(hWnd)) {
        DrawMenuBar(hWnd);
        RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME);
    }
    return S_OK;
}

HRESULT DirectDrawRenderer::RestoreGamePaletteEntries() {
    HRESULT hr;

    if (pPrimaryPalette == nullptr) {
        return S_OK;
    }
    hr = pPrimaryPalette->SetEntries(0, 0, nPaletteEntryCount, pGamePaletteEntries);
    return FAILED(hr) ? hr : S_OK;
}

HRESULT DirectDrawRenderer::AdjustGdiModeRefCount(bool bEnter) {
    HRESULT hr;

    if (bEnter) {
        if (++nGdiModeRefCount == 1) {
            hr = RestoreGdiPaletteAndFlipToGdi();
            if (FAILED(hr)) {
                return hr;
            }
        }
        return S_OK;
    }
    if (nGdiModeRefCount == 0) {
        return RENDERER_E_NOT_IN_GDI_MODE;
    }
    if (nGdiModeRefCount == 1) {
        hr = RestoreGamePaletteEntries();
        if (FAILED(hr)) {
            return hr;
        }
    }
    nGdiModeRefCount--;
    return S_OK;
}

HRESULT DirectDrawRenderer::SetDeviceNotificationCallback(RenderHost *pRenderHost) {
    if (pRenderHost == nullptr) {
        return E_INVALIDARG;
    }
    pHost = pRenderHost;
    pHost->SetRenderer(this);
    if ((dwRenderStateFlags & RENDERER_STATE_READY) == RENDERER_STATE_READY) {
        pHost->ValidateRenderDeviceState();
    }
    return S_OK;
}

// Restore a surface that was lost.
static inline HRESULT RestoreSurfaceIfLost(IDirectDrawSurface *pSurface) {
    if (pSurface != nullptr && pSurface->IsLost() != DD_OK) {
        return pSurface->Restore();
    }
    return S_OK;
}

HRESULT DirectDrawRenderer::RestoreLostSurfaces() {
    HRESULT hr;

    if ((dwRenderStateFlags & RENDERER_STATE_READY) != RENDERER_STATE_READY) {
        return RENDERER_E_NOT_READY;
    }
    if (FAILED(hr = RestoreSurfaceIfLost(pPrimarySurface)) ||
        FAILED(hr = RestoreSurfaceIfLost(pZBuffer)) ||
        FAILED(hr = RestoreSurfaceIfLost(pBackBuffer))) {
        return hr;
    }
    if (pHost != nullptr) {
        hr = pHost->ReloadTextureSurfacesAfterRestore();
        if (FAILED(hr)) {
            return hr;
        }
    }
    return S_OK;
}

HRESULT DirectDrawRenderer::GetDeviceViewportRect(RECT *pRect) {
    if ((dwRenderStateFlags & RENDERER_STATE_READY) != RENDERER_STATE_READY) {
        return RENDERER_E_NOT_READY;
    }
    *pRect = rcViewport;
    return S_OK;
}
