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

void AddPointerToRelocatedList(void *pPointer) {
    if (g_dwRelocatedPointerCount < RELOCATED_POINTER_COUNT) {
        g_apRelocatedPointers[g_dwRelocatedPointerCount++] = pPointer;
    }
}

void ByteSwapArray16(unsigned short *pData, int nCount) {
    for (int i = 0; i < nCount; i++) {
        pData[i] = static_cast<unsigned short>(pData[i] << 8 | pData[i] >> 8);
    }
}

int GetMipLevelFromDimension(int nDimension) {
    if (nDimension <= 1) {
        return 0;
    }
    if (nDimension <= 2) {
        return 1;
    }
    if (nDimension <= 4) {
        return 2;
    }
    if (nDimension <= 8) {
        return 3;
    }
    if (nDimension <= 16) {
        return 4;
    }
    if (nDimension <= 32) {
        return 5;
    }
    if (nDimension <= 64) {
        return 6;
    }
    return nDimension <= 128 ? 7 : 8;
}

void ResetRenderCommandBufferStack(void) {
    ShadowClipVertex *pNext = nullptr;

    for (int i = SHADOW_CLIP_VERTEX_COUNT - 1; i >= 0; i--) {
        g_aShadowClipVertexPool[i].pNext = pNext;
        pNext = &g_aShadowClipVertexPool[i];
    }
    g_pShadowClipVertexFreeList = pNext;
}

void ClearSplitScreenRenderState(void) {
    memset(g_anSkidMarkHead, 0, sizeof(g_anSkidMarkHead));
    memset(g_anSkidMarkTail, 0, sizeof(g_anSkidMarkTail));
}

void InitializeTextureUploadQueue(void) {
    g_dwTextureUploadQueueHead = 0;
    g_dwTextureUploadQueueTail = 0;
    g_hTextureUploadQueueMutex = CreateMutexA(nullptr, false, nullptr);
}

void ResetModelDataPool(void) {
    g_dwConvertedVertexCount = 0;
    g_pModelDataPoolCursor = g_afModelVertexPool;
}
