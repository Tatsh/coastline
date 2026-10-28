#include "renderhost.h"

#include <cmath>

namespace {

DirectDrawRenderer g_mainRenderWindow;
RenderHost g_renderHost;

std::uint32_t ColorChannel(float value) {
    return static_cast<std::uint32_t>(
        std::lround(std::fmin(std::fmax(value, 0.0f), 1.0f) * 255.0f));
}

} // namespace

DirectDrawRenderer *g_pMainRenderWindow = &g_mainRenderWindow;
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
