#include "platform/win/render.h"

#include <cstring>

#include "platform/win/boot/app.h"
#include "platform/win/settings.h"
#include "screen.h"

int g_dwRenderBackendMode;

#if TGR_RENDERER == TGR_RENDERER_D3D
int GetRenderBackendMode(void) {
    return g_dwRenderBackendMode;
}
#endif

void SetRenderBackendMode(int nMode) {
    g_dwRenderBackendMode = nMode;
#if TGR_RENDERER == TGR_RENDERER_D3D
    if (nMode == RENDER_BACKEND_MODE_D3D) {
        g_pfnRenderDeviceReset = ResetRenderDeviceCallbacks;
        g_pfnRenderBackendInit = InitGbiD3DRenderer;
        g_pfnInitializeTextureSubsystem = InitializeTextureSubsystem;
    }
#else
    if (nMode == RENDER_BACKEND_MODE_GLIDE) {
        g_pfnRenderDeviceReset = InitGlideRenderDevice;
        g_pfnRenderBackendInit = InitGbiGlideRenderer;
        g_pfnInitializeTextureSubsystem = InitializeTextureSubsystem;
    }
#endif
}

void ResetRenderSubsystems(int nMode, int nWidth, int nHeight, int nBitDepth, int nExtra) {
    if (g_dwRenderBackendMode) {
        NullStub();
        g_pfnRenderBackendShutdown();
        g_pfnShutdownTextureSubsystem();
#if TGR_RENDERER == TGR_RENDERER_D3D
        g_pfnRenderDeviceShutdown();
#endif
    }
    SetRenderBackendMode(nMode);
    SetDisplayModeWithFallback(nWidth, nHeight, nBitDepth, nExtra);
    g_pfnInitializeTextureSubsystem();
    g_pfnRenderBackendInit();
    ResetModelDataPool();
}

void InitializeRenderSubsystems(int nMode) {
    SetRenderBackendMode(nMode);
    g_pfnRenderDeviceReset();
    g_pfnInitializeTextureSubsystem();
    g_pfnRenderBackendInit();
    ResetModelDataPool();
}

void CopyConfigFramebufferSize(void) {
    g_dwScreenWidth = g_dwConfigFramebufferWidth;
    g_dwScreenHeight = g_dwConfigFramebufferHeight;
}

// The number of words Copy8Dwords() copies.
#define COPY_WORD_COUNT 8

void ResetVertexConversionCache(void) {
    g_dwVertexConvCacheCount = 0;
    g_dwRelocatedPointerCount = 0;
}

void SetPointerRelocationBase(unsigned dwOldBase, unsigned dwNewBase) {
    ResetVertexConversionCache();
    g_dwOldBaseAddress = dwOldBase;
    g_dwNewBaseAddress = dwNewBase;
}

void Copy8Dwords(void *pDest, const void *pSrc) {
    memcpy(pDest, pSrc, COPY_WORD_COUNT * sizeof(unsigned));
}

void ResetModelDataPool(void) {
    g_dwConvertedVertexCount = 0;
    g_pModelDataPoolCursor = g_afModelVertexPool;
}
