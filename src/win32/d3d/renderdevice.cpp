#include "common/renderdevice.h"

#include "win32/d3d/renderer.h"

void SetDeviceRenderState(u32 state, u32 value) {
    g_pD3DDevice2->SetRenderState(static_cast<D3DRENDERSTATETYPE>(state), value);
}

void DrawDeviceTriangles(DeviceVertex *vertices, u32 count) {
    g_pD3DDevice2->DrawPrimitive(
        D3DPT_TRIANGLELIST, D3DVT_TLVERTEX, vertices, count, D3DDP_WAIT | D3DDP_DONOTUPDATEEXTENTS);
}

void DrawDeviceIndexedTriangles(DeviceVertex *vertices,
                                u32 vertexCount,
                                u16 *indices,
                                u32 indexCount) {
    g_pD3DDevice2->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
                                        D3DVT_TLVERTEX,
                                        vertices,
                                        vertexCount,
                                        indices,
                                        indexCount,
                                        D3DDP_DONOTCLIP | D3DDP_DONOTUPDATEEXTENTS);
}
