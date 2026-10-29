#include "common/renderdevice.h"

#include <numeric>
#include <vector>

#include "sdl/d3dcompat.h"
#include "sdl/renderdevice.h"

namespace {

coastline::TlRenderer *g_renderer;

coastline::CompareFunction Compare(u32 value) {
    return value >= D3DCMP_NEVER && value <= D3DCMP_ALWAYS ?
               static_cast<coastline::CompareFunction>(value) :
               coastline::CompareFunction::Always;
}

coastline::BlendFactor Blend(u32 value) {
    return value >= D3DBLEND_ZERO && value <= D3DBLEND_INVDESTCOLOR ?
               static_cast<coastline::BlendFactor>(value) :
               coastline::BlendFactor::One;
}

} // namespace

void coastline::SetDeviceRenderer(TlRenderer *renderer) {
    g_renderer = renderer;
}

void SetDeviceRenderState(u32 state, u32 value) {
    auto &states = g_renderer->States();
    switch (state) {
    case D3DRENDERSTATE_TEXTUREHANDLE:
        g_renderer->SetTexture(value);
        break;
    case D3DRENDERSTATE_ZENABLE:
        states.depthTest = value != 0;
        break;
    case D3DRENDERSTATE_ZWRITEENABLE:
        states.depthWrite = value != 0;
        break;
    case D3DRENDERSTATE_ZFUNC:
        states.depthFunction = Compare(value);
        break;
    case D3DRENDERSTATE_ALPHATESTENABLE:
        states.alphaTest = value != 0;
        break;
    case D3DRENDERSTATE_ALPHAFUNC:
        states.alphaFunction = Compare(value);
        break;
    case D3DRENDERSTATE_ALPHAREF:
        states.alphaReference = static_cast<std::uint8_t>(value);
        break;
    case D3DRENDERSTATE_ALPHABLENDENABLE:
        states.blend = value != 0;
        break;
    case D3DRENDERSTATE_SRCBLEND:
        states.sourceBlend = Blend(value);
        break;
    case D3DRENDERSTATE_DESTBLEND:
        states.destinationBlend = Blend(value);
        break;
    case D3DRENDERSTATE_CULLMODE:
        states.cull = value >= D3DCULL_NONE && value <= D3DCULL_CCW ?
                          static_cast<coastline::CullMode>(value) :
                          coastline::CullMode::None;
        break;
    case D3DRENDERSTATE_FOGENABLE:
        states.fog = value != 0;
        break;
    case D3DRENDERSTATE_FOGCOLOR:
        states.fogColor = value;
        break;
    case D3DRENDERSTATE_SPECULARENABLE:
        states.specular = value != 0;
        break;
    case D3DRENDERSTATE_COLORKEYENABLE:
        states.colorKey = value != 0;
        break;
    case D3DRENDERSTATE_TEXTUREMAG:
    case D3DRENDERSTATE_TEXTUREMIN:
        states.linearFilter = value == D3DFILTER_LINEAR;
        break;
    case D3DRENDERSTATE_TEXTUREADDRESSU:
        states.wrapU = value != D3DTADDRESS_CLAMP;
        break;
    case D3DRENDERSTATE_TEXTUREADDRESSV:
        states.wrapV = value != D3DTADDRESS_CLAMP;
        break;
    default:
        // Perspective correction is always on, and the renderer neither dithers nor shades flat.
        break;
    }
}

void DrawDeviceTriangles(DeviceVertex *vertices, u32 count) {
    std::vector<std::uint16_t> indices(count / 3 * 3);
    std::iota(indices.begin(), indices.end(), std::uint16_t{0});
    g_renderer->DrawIndexed({vertices, count}, indices);
}

void DrawDeviceIndexedTriangles(DeviceVertex *vertices,
                                u32 vertexCount,
                                u16 *indices,
                                u32 indexCount) {
    g_renderer->DrawIndexed({vertices, vertexCount}, {indices, indexCount});
}
