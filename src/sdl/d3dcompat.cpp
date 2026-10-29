#include "d3dcompat.h"

#include <vector>

IDirect3DDevice2 *g_pD3DDevice2;
D3DDEVICEDESC g_D3DHalDeviceDesc;
D3DDEVICEDESC g_D3DHelDeviceDesc;

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
    m_renderer.EndScene(m_capture);
    return D3D_OK;
}
