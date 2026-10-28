#include <cstring>

#include "win32/d3d/renderer.h"

D3DDEVICEDESC g_D3DHalDeviceDesc;
D3DDEVICEDESC g_D3DHelDeviceDesc;

void QueryD3DDeviceCaps(IDirect3DDevice2 *pD3DDevice) {
    memset(&g_D3DHalDeviceDesc, 0, sizeof(g_D3DHalDeviceDesc));
    memset(&g_D3DHelDeviceDesc, 0, sizeof(g_D3DHelDeviceDesc));
    g_D3DHalDeviceDesc.dwSize = sizeof(g_D3DHalDeviceDesc);
    g_D3DHalDeviceDesc.dpcTriCaps.dwSize = sizeof(g_D3DHalDeviceDesc.dpcTriCaps);
    g_D3DHelDeviceDesc.dwSize = sizeof(g_D3DHelDeviceDesc);
    g_D3DHelDeviceDesc.dpcTriCaps.dwSize = sizeof(g_D3DHelDeviceDesc.dpcTriCaps);
    pD3DDevice->GetCaps(&g_D3DHalDeviceDesc, &g_D3DHelDeviceDesc);
}
