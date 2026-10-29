#include "win32/d3d/renderer.h"

#include <concepts>
#include <cstdlib>
#include <cstring>
#include <new>

#include "common/rallyconfig.h"
#include "common/screen.h"
#include "common/settings.h"
#include "win32/app.h"
#include "win32/d3d/texture.h"
#include "win32/render.h"
#include "win32/renderinit.h"

namespace {
/**
 * Whether the render target was cleared since the last flip.
 *
 * @ghidraAddress BRD3D: 0x102776a0
 */
int g_bRenderTargetCleared;
/**
 * The fourth value of the display mode set.
 *
 * @ghidraAddress BRD3D: 0x106c6644
 */
int g_dwCurrentDisplayModeBpp;
} // namespace

/** @ghidraAddress BRD3D: 0x10aa2860 */
int g_dwActiveDisplayDeviceIndex;
/** @ghidraAddress BRD3D: 0x10aa2a2c */
int g_dwSelectedDisplayDeviceIndex;
/** @ghidraAddress BRD3D: 0x118ac238 */
DirectDrawDriver *g_pSelectedDisplayDriver;

namespace {
// @ghidraAddress BRD3D: 0x10277680
D3DTexture g_LoadingScreenImageState;
} // namespace

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
        (pRenderer->GetRenderStateFlags() & RENDERER_STATE_READY) != RENDERER_STATE_READY) {
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
        (pRenderer->GetRenderStateFlags() & RENDERER_STATE_READY) != RENDERER_STATE_READY ||
        pRenderer->GetDirect3D2() == nullptr || pRenderer->GetD3DDevice() == nullptr ||
        pRenderer->GetViewport() == nullptr) {
        return E_FAIL;
    }
    return S_OK;
}

// The size of the texture of the loading screen, whatever the size of its image.
#define LOADING_SCREEN_TEXTURE_SIZE 256
// The pixel format of the texture of the loading screen.
#define LOADING_SCREEN_PIXEL_FORMAT 0xb
// The frames the loading screen is drawn and flipped in.
#define LOADING_SCREEN_FRAME_COUNT 2
// The specular colour of the vertices of the loading screen.
#define LOADING_SCREEN_SPECULAR 0xff0000ff

// A call of the device is made again after each restore of lost surfaces.
template <typename Call>
    requires requires(Call call) {
        { call() } -> std::same_as<HRESULT>;
    }
static inline HRESULT RetryWhileSurfacesLost(DirectDrawRenderer *pRenderer, Call call) {
    HRESULT hResult = call();

    while (hResult == DDERR_SURFACELOST) {
        while (pRenderer->RestoreLostSurfaces() == DDERR_SURFACELOST) {
        }
        hResult = call();
    }
    return hResult;
}

static inline void
SetLoadingScreenVertex(D3DTLVERTEX *pVertex, float fX, float fY, float fU, float fV) {
    pVertex->sx = fX;
    pVertex->sy = fY;
    pVertex->sz = 0.0f;
    pVertex->rhw = 1.0f;
    pVertex->color = 0xffffffff;
    pVertex->specular = LOADING_SCREEN_SPECULAR;
    pVertex->tu = fU;
    pVertex->tv = fV;
}

HRESULT RenderHost::RenderLoadingScreen(DirectDrawRenderer *pWindowRenderer,
                                        const char *pszImage,
                                        unsigned nImageKey) {
    IDirect3DDevice2 *pDevice;
    IDirect3DViewport2 *pViewport;
    void *pImage;
    int nWidth;
    int nHeight;
    float fLeft;
    float fTop;
    float fRight;
    float fBottom;
    D3DRECT rect;
    D3DTLVERTEX aVertices[3];
    HRESULT hResult;
    HRESULT hFlip = DD_OK;

    if (pWindowRenderer == nullptr ||
        (pWindowRenderer->GetRenderStateFlags() & RENDERER_STATE_READY) != RENDERER_STATE_READY) {
        return E_FAIL;
    }
    pDevice = pWindowRenderer->GetD3DDevice();
    pViewport = pWindowRenderer->GetViewport();
    if (pWindowRenderer->GetDirect3D2() == nullptr || pDevice == nullptr || pViewport == nullptr) {
        return E_FAIL;
    }
    // The result is discarded.
    pWindowRenderer->BeginRenderScene();
    // The pixels of the image are never freed.
    pImage = LoadRgba5551ImageFile(pszImage, nImageKey, &nWidth, &nHeight);
    g_LoadingScreenImageState.CreateD3DTextureDefault(pDevice,
                                                      pImage,
                                                      LOADING_SCREEN_TEXTURE_SIZE,
                                                      LOADING_SCREEN_TEXTURE_SIZE,
                                                      LOADING_SCREEN_PIXEL_FORMAT);
    for (int nFrame = 0; nFrame < LOADING_SCREEN_FRAME_COUNT; nFrame++) {
        hResult =
            RetryWhileSurfacesLost(pWindowRenderer, [pDevice] { return pDevice->BeginScene(); });
        if (hResult != D3D_OK) {
            return hResult;
        }
        // A state the device refuses skips the drawing and ends the scene.
        if (pDevice->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE) == D3D_OK &&
            pDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE) == D3D_OK &&
            pDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0) == D3D_OK &&
            pDevice->SetRenderState(D3DRENDERSTATE_DITHERENABLE, TRUE) == D3D_OK) {
            rect.x1 = 0;
            rect.y1 = 0;
            rect.x2 = g_dwConfigFramebufferWidth;
            rect.y2 = g_dwConfigFramebufferHeight;
            pViewport->Clear(1, &rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
            pDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
            pDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
            pDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
            pDevice->SetRenderState(D3DRENDERSTATE_WRAPU, FALSE);
            pDevice->SetRenderState(D3DRENDERSTATE_WRAPV, FALSE);
            pDevice->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
            pDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE,
                                    g_LoadingScreenImageState.GetTextureHandle());

            fLeft = static_cast<float>((g_dwConfigFramebufferWidth - nWidth) / 2);
            if (fLeft < 0.0f) {
                fLeft = 0.0f;
            }
            fTop = static_cast<float>((g_dwConfigFramebufferHeight - nHeight) / 2);
            if (fTop < 0.0f) {
                fTop = 0.0f;
            }
            fRight = fLeft + LOADING_SCREEN_TEXTURE_SIZE;
            fBottom = fTop + LOADING_SCREEN_TEXTURE_SIZE;

            SetLoadingScreenVertex(&aVertices[0], fLeft, fTop, 0.0f, 0.0f);
            SetLoadingScreenVertex(&aVertices[1], fRight, fBottom, 1.0f, 1.0f);
            SetLoadingScreenVertex(&aVertices[2], fLeft, fBottom, 0.0f, 1.0f);
            pDevice->DrawPrimitive(D3DPT_TRIANGLELIST,
                                   D3DVT_TLVERTEX,
                                   aVertices,
                                   3,
                                   D3DDP_WAIT | D3DDP_DONOTUPDATEEXTENTS);
            SetLoadingScreenVertex(&aVertices[0], fLeft, fTop, 0.0f, 0.0f);
            SetLoadingScreenVertex(&aVertices[1], fRight, fTop, 1.0f, 0.0f);
            SetLoadingScreenVertex(&aVertices[2], fRight, fBottom, 1.0f, 1.0f);
            pDevice->DrawPrimitive(D3DPT_TRIANGLELIST,
                                   D3DVT_TLVERTEX,
                                   aVertices,
                                   3,
                                   D3DDP_WAIT | D3DDP_DONOTUPDATEEXTENTS);
            // A device that refuses Gouraud shading ends the screen with its scene open and its
            // texture not released.
            if (pDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD) != D3D_OK) {
                return D3D_OK;
            }
        }
        hResult =
            RetryWhileSurfacesLost(pWindowRenderer, [pDevice] { return pDevice->EndScene(); });
        if (hResult != D3D_OK) {
            return hResult;
        }
        hFlip = pWindowRenderer->GetPrimarySurface()->Flip(pWindowRenderer->GetBackBuffer(),
                                                           DDFLIP_WAIT);
        while (pWindowRenderer->GetPrimarySurface()->GetFlipStatus(DDGFS_ISFLIPDONE) != DD_OK) {
        }
    }
    g_LoadingScreenImageState.ReleaseD3DTextureInterfaces();
    return hFlip;
}

// The colours a ramp of a material has.
#define MATERIAL_RAMP_SIZE 16

D3DMaterial::D3DMaterial(IDirect3D2 *pDirect3D2, IDirect3DDevice2 *pD3DDevice) {
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwRampSize = MATERIAL_RAMP_SIZE;
    desc.dcvDiffuse.a = 1.0f;
    pDirect3D2->CreateMaterial(&pMaterial, nullptr);
    pMaterial->SetMaterial(&desc);
    pMaterial->GetHandle(pD3DDevice, &hMaterial);
    bDirty = 0;
    pMaterial->SetMaterial(&desc);
}

HRESULT D3DMaterial::CommitMaterialIfDirty() {
    if (!bDirty) {
        return 0;
    }
    bDirty = false;
    return pMaterial->SetMaterial(&desc);
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

HRESULT DirectDrawRenderer::InitializeDirectDraw() {
    HRESULT hr;

    if (hWnd == nullptr || !IsWindow(hWnd)) {
        return RENDERER_E_NOT_READY;
    }
    hr = SelectDriverDeviceAndMode();
    if (FAILED(hr)) {
        return hr;
    }
    if (FAILED(hr = CreateDirectDrawObjects()) || FAILED(hr = SetCooperativeLevel()) ||
        FAILED(hr = SetDisplayMode()) || FAILED(hr = CreatePrimaryFlipChain()) ||
        FAILED(hr = InitializeRenderDevice())) {
        DestroyDirectDrawRendererObjects();
        return hr;
    }
    SendMessageA(hWnd, WM_RENDERER_CREATED, 0, reinterpret_cast<LPARAM>(this));
    return S_OK;
}

HRESULT DirectDrawRenderer::SelectDriverDeviceAndMode() {
    DirectDrawDriver *pNewDriver;
    GUID *pDeviceGuid;
    int nWidth;
    int nHeight;
    int nBitDepth;
    int nRefreshRate;
    DisplayMode *pNewMode;
    D3DDeviceNode *pNewDevice;
    HRESULT hr;

    if (!(g_bDirectDrawDriversEnumerated & 1)) {
        hr = EnsureDirectDrawDriversEnumerated();
        if (FAILED(hr)) {
            return hr;
        }
    }
    pDeviceGuid = pDevice != nullptr ? pDevice->GetGuid() : nullptr;
    pNewDriver = FindDirectDrawDriverOrDefault(
        pDriver != nullptr ? pDriver->GetDirectDrawDriverGuid() : nullptr);
    if (pNewDriver == nullptr) {
        return RENDERER_E_NOT_CREATED;
    }
    if (pMode != nullptr) {
        pMode->GetSurfaceDescDimensions(&nWidth, &nHeight, &nBitDepth, &nRefreshRate);
    } else {
        nWidth = 0;
        nHeight = 0;
        nBitDepth = 0;
        nRefreshRate = 0;
    }
    if (!SelectDeviceAndDisplayMode(pNewDriver,
                                    pDeviceGuid,
                                    nWidth,
                                    nHeight,
                                    nBitDepth,
                                    reinterpret_cast<void *>(nRefreshRate),
                                    &pNewMode,
                                    &pNewDevice)) {
        return RENDERER_E_NOT_IN_GDI_MODE;
    }
    pMode = pNewMode;
    pDriver = pNewDriver;
    pDevice = pNewDevice;
    return S_OK;
}

// The display mode a move to another driver takes without a mode.
#define DEFAULT_MODE_WIDTH 640
#define DEFAULT_MODE_HEIGHT 480
#define DEFAULT_MODE_BIT_DEPTH 16

HRESULT DirectDrawRenderer::ChangeDisplayModeAndReinitDirectDraw(GUID *pGuid,
                                                                 D3DDeviceNode *pNewDevice,
                                                                 DisplayMode *pNewMode) {
    DirectDrawDriver *pNewDriver;
    GUID *pDeviceGuid;
    int nWidth;
    int nHeight;
    int nBitDepth;
    int nRefreshRate;
    DisplayMode *pSelectedMode;
    D3DDeviceNode *pSelectedDevice;
    DirectDrawDriver *pOldDriver;
    DisplayMode *pOldMode;
    D3DDeviceNode *pOldDevice;
    HRESULT hr;

    pNewDriver = FindDirectDrawDriverOrDefault(pGuid);
    if (pNewDriver == nullptr) {
        return RENDERER_E_NOT_CREATED;
    }
    if (pNewDevice != nullptr) {
        pDeviceGuid = pNewDevice->GetGuid();
    } else if (pDevice != nullptr) {
        pDeviceGuid = pDevice->GetGuid();
    } else {
        pDeviceGuid = nullptr;
    }
    if (pNewMode != nullptr) {
        pNewMode->GetSurfaceDescDimensions(&nWidth, &nHeight, &nBitDepth, &nRefreshRate);
    } else {
        nWidth = DEFAULT_MODE_WIDTH;
        nHeight = DEFAULT_MODE_HEIGHT;
        nBitDepth = DEFAULT_MODE_BIT_DEPTH;
        nRefreshRate = 0;
    }
    if (!SelectDeviceAndDisplayMode(pNewDriver,
                                    pDeviceGuid,
                                    nWidth,
                                    nHeight,
                                    nBitDepth,
                                    reinterpret_cast<void *>(nRefreshRate),
                                    &pSelectedMode,
                                    &pSelectedDevice)) {
        return RENDERER_E_NOT_IN_GDI_MODE;
    }
    pOldDevice = pDevice;
    pOldDriver = pDriver;
    pOldMode = pMode;
    DestroyDirectDrawRendererObjects();
    pDriver = pNewDriver;
    pMode = pSelectedMode;
    pDevice = pSelectedDevice;
    hr = InitializeDirectDraw();
    if (FAILED(hr)) {
        // The objects of the old driver are created again, and the error of the new one reported.
        DestroyDirectDrawRendererObjects();
        pDriver = pOldDriver;
        pDevice = pOldDevice;
        pMode = pOldMode;
        InitializeDirectDraw();
        return hr;
    }
    SendMessageA(hWnd, WM_RENDERER_CREATED + 2, 0, 0);
    return S_OK;
}

int DirectDrawRenderer::SwitchDisplayModeAndRebuildDevice(int nWidth,
                                                          int nHeight,
                                                          int nBitDepth,
                                                          int nExtra) {
    DisplayMode *pOldMode;
    D3DDeviceNode *pOldDevice;
    DisplayMode *pNewMode;
    D3DDeviceNode *pNewDevice;
    D3DDeviceNode *pFallback;
    HRESULT hr;

    if (hWnd == nullptr || !IsWindow(hWnd)) {
        return RENDERER_E_NOT_READY;
    }
    pOldMode = pMode;
    pOldDevice = pDevice;
    // The fourth value of the mode is not passed on, as in the original.
    pNewMode = pDriver->FindDisplayMode(nWidth, nHeight, nBitDepth, nullptr, nullptr, nullptr);
    if (pNewMode == nullptr) {
        return RENDERER_E_NOT_CREATED;
    }
    if (pNewMode->IsDisplayModeSupportedByDevice(pOldDevice)) {
        pNewDevice = nullptr;
    } else {
        pNewDevice =
            pDriver->FindRenderDevice(pOldDevice->GetGuid(), pNewMode, &pFallback, nullptr);
        if (pNewDevice == nullptr) {
            pNewDevice = pFallback;
            if (pNewDevice == nullptr) {
                return RENDERER_E_NOT_IN_GDI_MODE;
            }
        }
    }
    DestroyD3DDevice();
    ReleasePrimarySurface();
    pMode = pNewMode;
    if (pNewDevice != nullptr) {
        pDevice = pNewDevice;
    }
    hr = SetDisplayMode();
    if (FAILED(hr)) {
        return hr;
    }
    hr = CreatePrimaryFlipChain();
    if (SUCCEEDED(hr)) {
        hr = InitializeRenderDevice();
        if (SUCCEEDED(hr)) {
            SendMessageA(hWnd, WM_RENDERER_CREATED + 3, 0, 0);
            return S_OK;
        }
        ReleasePrimarySurface();
    }
    // The old mode and device are set and created again, and the error is reported.
    pMode = pOldMode;
    pDevice = pOldDevice;
    SetDisplayMode();
    CreatePrimaryFlipChain();
    InitializeRenderDevice();
    return hr;
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
void DirectDrawRenderer::RecordDisplayMode(int nWidth, int nHeight, int nBitDepth) {
    rcViewport.left = 0;
    rcViewport.top = 0;
    rcViewport.right = nWidth;
    rcViewport.bottom = nHeight;
    g_dwConfigFramebufferWidth = nWidth;
    g_dwScreenWidth = nWidth;
    g_dwConfigFramebufferHeight = nHeight;
    g_dwScreenHeight = nHeight;
    g_dwScreenBitDepth = nBitDepth;
    dwRenderStateFlags |= 2;
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
        RecordDisplayMode(nWidth, nHeight, nBitDepth);
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
                RecordDisplayMode(nWidth, nHeight, nBitDepth);
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
        RecordDisplayMode(nWidth, nHeight, nBitDepth);
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
    nPaletteEntryCount = static_cast<unsigned>(GetDeviceCaps(hdc, SIZEPALETTE));
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
    // a palette of 4 or 8 bits are read-only. A display without a palette does not allocate the
    // buffers, and the original writes the marks regardless.
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
HRESULT DirectDrawRenderer::CreateDepthBuffer(const D3DDEVICEDESC *pDesc,
                                              DWORD dwMemoryCaps,
                                              bool *pbAttached) {
    DDSURFACEDESC backDesc;
    DDSURFACEDESC depthDesc;
    HRESULT hr;

    *pbAttached = false;
    memset(&backDesc, 0, sizeof(backDesc));
    backDesc.dwSize = sizeof(backDesc);
    hr = pBackBuffer->GetSurfaceDesc(&backDesc);
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
    // A depth buffer that is not created or not attached results in a device without one.
    if (SUCCEEDED(pDirectDraw2->CreateSurface(&depthDesc, &pZBuffer, nullptr))) {
        if (FAILED(pBackBuffer->AddAttachedSurface(pZBuffer))) {
            if (pZBuffer != nullptr) {
                pZBuffer->Release();
                pZBuffer = nullptr;
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
        pDesc = pDevice->GetHardwareDescription();
        dwMemoryCaps = DDSCAPS_VIDEOMEMORY;
    } else {
        pDesc = pDevice->GetSoftwareDescription();
        dwMemoryCaps = DDSCAPS_SYSTEMMEMORY;
    }
    dwDeviceInit = 0;
    if ((dwFlags & 4) != 0 && pDesc->dwDeviceZBufferBitDepth != 0) {
        hr = CreateDepthBuffer(pDesc, dwMemoryCaps, &bAttached);
        if (FAILED(hr)) {
            return hr;
        }
        dwDeviceInit = bAttached;
    }
    hr = pDirect3D2->CreateDevice(*pDevice->GetGuid(), pBackBuffer, &pD3DDevice);
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

// Make the material of the renderer black and the background of its viewport.
static inline void SetBlackBackground(DirectDrawRenderer *pRenderer) {
    D3DMaterial *pMaterial = pRenderer->GetMaterial();

    pMaterial->SetDiffuseColor(0.0f, 0.0f, 0.0f);
    pMaterial->SetRampSize(1);
    pMaterial->ApplyDescription();
    pRenderer->GetViewport()->SetBackground(pMaterial->GetMaterialHandle());
}

HRESULT DirectDrawRenderer::BeginRenderScene() {
    D3DRECT rect;
    HRESULT hr;

    rect.x1 = 0;
    rect.y1 = 0;
    rect.x2 = g_dwConfigFramebufferWidth;
    rect.y2 = g_dwConfigFramebufferHeight;

    // The viewport is cleared to its black background and shown once.
    SetBlackBackground(this);
    pViewport->Clear(1, &rect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER);
    hr = pPrimarySurface->Flip(pBackBuffer, DDFLIP_WAIT);
    while (pPrimarySurface->GetFlipStatus(DDGFS_ISFLIPDONE) != DD_OK) {
    }
    return hr;
}

HRESULT DirectDrawRenderer::InitializeViewportAndRenderContext() {
    D3DVIEWPORT2 viewport;
    float fAspect;
    int nWidth;
    int nHeight;
    HRESULT hr;

    if (pD3DDevice == nullptr || pViewport == nullptr) {
        return RENDERER_E_NOT_READY;
    }
    nWidth = abs(rcViewport.right - rcViewport.left);
    nHeight = abs(rcViewport.bottom - rcViewport.top);
    fAspect = nWidth != 0 ? static_cast<float>(nHeight) / nWidth : 1.0f;
    // The clip volume spans the width from -1 to 1 and the height in proportion.
    memset(&viewport, 0, sizeof(viewport));
    viewport.dwSize = sizeof(viewport);
    viewport.dwX = 0;
    viewport.dwY = 0;
    viewport.dwWidth = nWidth;
    viewport.dwHeight = nHeight;
    viewport.dvClipX = -1.0f;
    viewport.dvClipY = fAspect;
    viewport.dvClipWidth = 2.0f;
    viewport.dvClipHeight = fAspect + fAspect;
    viewport.dvMinZ = 0.0f;
    viewport.dvMaxZ = 1.0f;
    hr = pViewport->SetViewport2(&viewport);
    if (FAILED(hr)) {
        return hr;
    }
    hr = pD3DDevice->SetCurrentViewport(pViewport);
    if (FAILED(hr)) {
        return hr;
    }
    if (pMaterial != nullptr) {
        pMaterial->Release();
        delete pMaterial;
    }
    // The original writes through a material that was not allocated.
    pMaterial = new (std::nothrow) D3DMaterial(pDirect3D2, pD3DDevice);
    SetBlackBackground(this);
    return S_OK;
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

DirectDrawDriver *FindDisplayDeviceByGuid(GUID *pGuid) {
    int nIndex = 0;

    for (DirectDrawDriver *pDriver = g_pDirectDrawDriverListHead; pDriver != nullptr;
         pDriver = pDriver->GetNext(), nIndex++) {
        if (memcmp(pDriver->GetGuid(), pGuid, sizeof(GUID)) != 0) {
            continue;
        }
        g_pSelectedDisplayDriver = FindDirectDrawDriverOrDefault(pGuid);
        if (g_pSelectedDisplayDriver == nullptr) {
            break;
        }
        g_dwActiveDisplayDeviceIndex = nIndex;
        g_dwSelectedDisplayDeviceIndex = nIndex;
        return pDriver;
    }
    return g_pDirectDrawDriverListHead;
}

bool ApplySelectedDisplayMode(int nWidth, int nHeight, int nBitDepth, int nExtra) {
    DirectDrawRenderer *pRenderer = GetWindowRenderer(g_hMainWindow);
    DirectDrawDriver *pDriver;
    D3DDeviceNode *pDevice;
    DisplayMode *pMode;

    if (pRenderer == nullptr) {
        return false;
    }
    pDriver = FindDisplayDeviceByGuid(&g_GlobalRallyConfig.DisplayDriverGuid);
    if (pRenderer->GetDriver() == pDriver || pDriver == nullptr) {
        return false;
    }
    pDevice = pRenderer->GetDevice();
    if (pDevice == nullptr) {
        return false;
    }
    pMode = FindDisplayModeWithFallback(pDriver, nWidth, nHeight, nBitDepth, nExtra, pDevice);
    return pRenderer->ChangeDisplayModeAndReinitDirectDraw(
               &g_GlobalRallyConfig.DisplayDriverGuid, pDevice, pMode) == S_OK;
}

void DirectDrawRenderer::FlipPrimarySurfaceAndWait() {
    if (g_bD3DWaitCanFlip) {
        while (pPrimarySurface->GetFlipStatus(DDGFS_CANFLIP) != DD_OK) {
        }
    }
    pPrimarySurface->Flip(pBackBuffer, DDFLIP_WAIT);
    if (g_bD3DWaitFlipDone) {
        while (pPrimarySurface->GetFlipStatus(DDGFS_ISFLIPDONE) != DD_OK) {
        }
    }
    g_bRenderTargetCleared = false;
}

void ClearRenderTargetOnce(void) {
    if (g_bRenderTargetCleared) {
        return;
    }
    if (g_dwD3DClearZBuffer) {
        D3DRECT rcClear = {{0}, {0}, {g_dwConfigFramebufferWidth}, {g_dwConfigFramebufferHeight}};
        if (g_pMainRenderWindow->GetViewport()->Clear(
                1, &rcClear, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER) != D3D_OK) {
            return;
        }
    }
    g_bRenderTargetCleared = true;
}

void PresentFrameCallback_FlipSurfaces(void) {
    g_pMainRenderWindow->FlipPrimarySurfaceAndWait();
}

int ResetRenderDeviceCallbacks(void) {
    g_pfnPresentFrame = PresentFrameCallback_FlipSurfaces;
    g_pfnRenderDeviceShutdown = NullStub;
    return true;
}

// The display mode the game falls back to when the device is not created in the mode requested.
#define FALLBACK_MODE_WIDTH 640
#define FALLBACK_MODE_HEIGHT 480
#define FALLBACK_MODE_BIT_DEPTH 16

void SetDisplayModeWithFallback(int nWidth, int nHeight, int nBitDepth, int nExtra) {
    if (!ApplySelectedDisplayMode(nWidth, nHeight, nBitDepth, nExtra) &&
        (nWidth != g_dwScreenWidth || nHeight != g_dwScreenHeight ||
         nBitDepth != g_dwScreenBitDepth || nExtra != g_dwCurrentDisplayModeBpp)) {
        g_pMainRenderWindow->SwitchDisplayModeAndRebuildDevice(nWidth, nHeight, nBitDepth, nExtra);
        if (!g_pMainRenderWindow->GetDeviceInit()) {
            g_pMainRenderWindow->SwitchDisplayModeAndRebuildDevice(
                FALLBACK_MODE_WIDTH, FALLBACK_MODE_HEIGHT, FALLBACK_MODE_BIT_DEPTH, 0);
        }
    }
    ResetRenderDeviceCallbacks();
}
