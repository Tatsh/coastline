#include "d3dcompat.h"

#include <numeric>
#include <vector>

IDirect3DDevice2 *g_pD3DDevice2;
D3DTexture **g_ppTextureIdTable;
D3DDEVICEDESC g_D3DHalDeviceDesc;
D3DDEVICEDESC g_D3DHelDeviceDesc;

namespace {

coastline::CompareFunction Compare(DWORD value) {
    return value >= D3DCMP_NEVER && value <= D3DCMP_ALWAYS ?
               static_cast<coastline::CompareFunction>(value) :
               coastline::CompareFunction::Always;
}

coastline::BlendFactor Blend(DWORD value) {
    return value >= D3DBLEND_ZERO && value <= D3DBLEND_INVDESTCOLOR ?
               static_cast<coastline::BlendFactor>(value) :
               coastline::BlendFactor::One;
}

} // namespace

HRESULT IDirect3DDevice2::SetRenderState(D3DRENDERSTATETYPE state, DWORD value) {
    auto &states = m_renderer.States();
    switch (state) {
    case D3DRENDERSTATE_TEXTUREHANDLE:
        m_renderer.SetTexture(value);
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
    return D3D_OK;
}

HRESULT IDirect3DDevice2::DrawPrimitive(
    D3DPRIMITIVETYPE type, D3DVERTEXTYPE vertexType, void *vertices, DWORD count, DWORD flags) {
    std::vector<std::uint16_t> indices(count / 3 * 3);
    std::iota(indices.begin(), indices.end(), std::uint16_t{0});
    m_renderer.DrawIndexed({static_cast<const D3DTLVERTEX *>(vertices), count}, indices);
    return D3D_OK;
}

HRESULT IDirect3DDevice2::DrawIndexedPrimitive(D3DPRIMITIVETYPE type,
                                               D3DVERTEXTYPE vertexType,
                                               void *vertices,
                                               DWORD vertexCount,
                                               WORD *indices,
                                               DWORD indexCount,
                                               DWORD flags) {
    m_renderer.DrawIndexed({static_cast<const D3DTLVERTEX *>(vertices), vertexCount},
                           {indices, indexCount});
    return D3D_OK;
}

namespace {

// The pixels of a colour-keyed texture with the alpha of the key: 0 below half, and opaque above.
std::vector<unsigned char> ApplyColorKey(const unsigned char *pixels, int width, int height) {
    std::vector<unsigned char> keyed(pixels, pixels + static_cast<std::size_t>(width) * height * 4);
    for (std::size_t i = 3; i < keyed.size(); i += 4) {
        keyed[i] = keyed[i] < 0x80 ? 0 : 0xFF;
    }
    return keyed;
}

} // namespace

bool D3DTexture::CreateD3DTexture(IDirect3DDevice2 *device,
                                  const unsigned char *pixels,
                                  int width,
                                  int height,
                                  int pixelFormat,
                                  unsigned colorKey) {
    m_colorKey = true;
    auto keyed = ApplyColorKey(pixels, width, height);
    m_handle = device->Renderer().CreateTexture(
        static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), keyed, true);
    return m_handle != 0;
}

bool D3DTexture::CreateD3DTextureDefault(
    IDirect3DDevice2 *device, const unsigned char *pixels, int width, int height, int pixelFormat) {
    m_colorKey = false;
    m_handle =
        device->Renderer().CreateTexture(static_cast<std::uint32_t>(width),
                                         static_cast<std::uint32_t>(height),
                                         {pixels, static_cast<std::size_t>(width) * height * 4},
                                         true);
    return m_handle != 0;
}

void D3DTexture::UploadSurfaceToTexture(const void *pixels, int width, int height) {
    if (m_handle == 0 || g_pD3DDevice2 == nullptr) {
        return;
    }
    const auto *bytes = static_cast<const unsigned char *>(pixels);
    if (m_colorKey) {
        auto keyed = ApplyColorKey(bytes, width, height);
        g_pD3DDevice2->Renderer().UpdateTexture(
            m_handle, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), keyed);
    } else {
        g_pD3DDevice2->Renderer().UpdateTexture(
            m_handle,
            static_cast<std::uint32_t>(width),
            static_cast<std::uint32_t>(height),
            {bytes, static_cast<std::size_t>(width) * height * 4});
    }
}

HRESULT IDirect3DDevice2::SetClipStatus(D3DCLIPSTATUS *status) {
    return D3D_OK;
}

HRESULT IDirect3DDevice2::BeginScene() {
    m_renderer.BeginScene(m_clearColor);
    return D3D_OK;
}

HRESULT IDirect3DDevice2::EndScene() {
    m_renderer.EndScene();
    return D3D_OK;
}
