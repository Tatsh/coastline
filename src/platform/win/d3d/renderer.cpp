#include "platform/win/d3d/renderer.h"

#include <cstring>

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
