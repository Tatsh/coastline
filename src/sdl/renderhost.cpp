#include "renderhost.h"

#include <cmath>
#include <cstdlib>
#include <functional>
#include <utility>

#include "common/screen.h"
#include "common/settings.h"
#include "win32/render.h"

namespace {

RenderHost g_renderHost;

std::uint32_t ColorChannel(float value) {
    return static_cast<std::uint32_t>(
        std::lround(std::fmin(std::fmax(value, 0.0f), 1.0f) * 255.0f));
}

} // namespace

RenderHost *g_pRenderHost = &g_renderHost;

void DisplayMode::GetSurfaceDescDimensions(int *pnWidth,
                                           int *pnHeight,
                                           int *pnBitDepth,
                                           int *pnRefreshRate) {
    // The menus of the PC release are not part of the SDL game, which reports their size.
    *pnWidth = 640;
    *pnHeight = 480;
    *pnBitDepth = 32;
    *pnRefreshRate = 60;
}

void D3DMaterial::SetDiffuseColor(float red, float green, float blue) {
    m_color = 0xFF000000 | ColorChannel(red) << 16 | ColorChannel(green) << 8 | ColorChannel(blue);
}

HRESULT D3DViewport::Clear(DWORD count, D3DRECT *rects, DWORD flags) {
    if (g_pD3DDevice2 == nullptr) {
        return D3D_OK;
    }
    coastline::TlRenderer &renderer = g_pD3DDevice2->Renderer();
    coastline::RenderStates saved = renderer.States();
    auto &states = renderer.States();
    states = {};
    states.cull = coastline::CullMode::None;
    states.depthFunction = coastline::CompareFunction::Always;
    states.depthWrite = (flags & D3DCLEAR_ZBUFFER) != 0;
    renderer.SetTexture(0);
    std::uint32_t color = g_pMainRenderWindow->GetMaterial()->Color();
    constexpr std::uint16_t QUAD[] = {0, 1, 2, 0, 2, 3};
    for (DWORD i = 0; i < count; ++i) {
        auto left = static_cast<float>(rects[i].x1);
        auto top = static_cast<float>(rects[i].y1);
        auto right = static_cast<float>(rects[i].x2);
        auto bottom = static_cast<float>(rects[i].y2);
        coastline::TlVertex quad[] = {{left, top, 1.0f, 1.0f, color, 0, 0, 0},
                                      {right, top, 1.0f, 1.0f, color, 0, 0, 0},
                                      {right, bottom, 1.0f, 1.0f, color, 0, 0, 0},
                                      {left, bottom, 1.0f, 1.0f, color, 0, 0, 0}};
        renderer.DrawIndexed(quad, QUAD);
    }
    renderer.States() = saved;
    return D3D_OK;
}

HRESULT RenderHost::RenderLoadingScreen(DirectDrawRenderer *pWindowRenderer,
                                        const char *pszImage,
                                        int nOption) {
    return D3D_OK;
}

namespace {

// The flag of d3d/renderer.cpp, set by the first clear of a frame and cleared by its present.
bool g_bRenderTargetCleared;

std::function<bool()> g_framePump;

void PresentFrame(void) {
    g_bRenderTargetCleared = false;
    if (g_framePump && !g_framePump()) {
        std::exit(EXIT_SUCCESS);
    }
}

} // namespace

void coastline::SetFramePump(std::function<bool()> pump) {
    g_framePump = std::move(pump);
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

int ResetRenderDeviceCallbacks(void) {
    g_pfnPresentFrame = PresentFrame;
    g_pfnRenderDeviceShutdown = NullStub;
    return true;
}

// The window of SDL has no display modes. The mode becomes the size of the screen the game draws,
// as RecordDisplayMode() of d3d/renderer.cpp records it, and the renderer scales the screen to the
// window.
void SetDisplayModeWithFallback(int nWidth, int nHeight, int nBitDepth, int nExtra) {
    g_dwConfigFramebufferWidth = nWidth;
    g_dwScreenWidth = nWidth;
    g_dwConfigFramebufferHeight = nHeight;
    g_dwScreenHeight = nHeight;
    ResetRenderDeviceCallbacks();
}
