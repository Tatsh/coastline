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

DirectDrawRenderer::DirectDrawRenderer() {
    memset(this, 0, sizeof(*this));
    dwSize = sizeof(*this);
    dwFlags |= 4;
}

void DirectDrawRenderer::Cleanup() {
    DestroyDirectDrawRendererObjects();
    pDriver = nullptr;
    pDevice = nullptr;
    pMode = nullptr;
    hWnd = nullptr;
    pHost = nullptr;
}

HRESULT DirectDrawRenderer::InitializeWindow(HWND hNewWnd,
                                             GUID *pDriverGuid,
                                             GUID *pDeviceGuid,
                                             int nWidth,
                                             int nHeight,
                                             int nBitDepth,
                                             int nModeFlags,
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
                                             pDeviceGuid,
                                             nWidth,
                                             nHeight,
                                             nBitDepth,
                                             nModeFlags,
                                             1,
                                             &pDriver,
                                             &pDevice,
                                             &pMode);
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

    if (pDevice == nullptr || pDirectDraw2 == nullptr) {
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
    if (pSavedPaletteEntries != nullptr) {
        operator delete(pSavedPaletteEntries);
        pSavedPaletteEntries = nullptr;
    }
    if (pPaletteEntries != nullptr) {
        operator delete(pPaletteEntries);
        pPaletteEntries = nullptr;
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
