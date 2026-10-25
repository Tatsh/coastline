#include "platform/win/d3d/texture.h"

#include "platform/win/d3d/renderer.h"

unsigned GetAvailableTextureMemory(void) {
    DDSCAPS caps;
    DWORD dwTotal;
    DWORD dwFree;

    caps.dwCaps = DDSCAPS_TEXTURE;
    g_pRenderHost->pRenderer->pDirectDraw2->GetAvailableVidMem(&caps, &dwTotal, &dwFree);
    return dwFree;
}
