#include "platform/win/render.h"

#include <cstring>

#include "platform/win/boot/app.h"
#include "platform/win/displaylist.h"
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

void RelocatePointerValue(unsigned *pdwPointer) {
    unsigned dwPointer = *pdwPointer;
    if (dwPointer == 0) {
        return;
    }
    // The comparison is signed, as in the game.
    if (static_cast<int>(dwPointer) < static_cast<int>(g_dwOldBaseAddress)) {
        *pdwPointer = 0;
        return;
    }
    *pdwPointer = g_dwNewBaseAddress - g_dwOldBaseAddress + dwPointer;
}

void ByteSwapArray16(unsigned short *pData, int nCount) {
    for (int i = 0; i < nCount; i++) {
        pData[i] = static_cast<unsigned short>(pData[i] << 8 | pData[i] >> 8);
    }
}

// The halfwords of a group ByteSwapFourShorts() swaps, and the words of a group ByteSwap3DWords()
// reverses.
#define SHORT_QUAD_COUNT 4
#define WORD_TRIPLE_COUNT 3

void ByteSwapShortQuadArray(unsigned short *pData, int nCount) {
    for (int i = 0; i < nCount; i++) {
        ByteSwapFourShorts(pData + i * SHORT_QUAD_COUNT);
    }
}

void ByteSwapFourShorts(unsigned short *pData) {
    for (int i = 0; i < SHORT_QUAD_COUNT; i++) {
        pData[i] = static_cast<unsigned short>(pData[i] << 8 | pData[i] >> 8);
    }
}

void ByteSwap3DWordsArray(unsigned char *pData, int nCount) {
    for (int i = 0; i < nCount; i++) {
        ByteSwap3DWords(pData + i * WORD_TRIPLE_COUNT * sizeof(unsigned));
    }
}

void ByteSwap3DWords(unsigned char *pData) {
    for (int i = 0; i < WORD_TRIPLE_COUNT; i++) {
        unsigned char *pWord = pData + i * sizeof(unsigned);
        unsigned char bByte = pWord[3];
        pWord[3] = pWord[0];
        pWord[0] = bByte;
        bByte = pWord[2];
        pWord[2] = pWord[1];
        pWord[1] = bByte;
    }
}

#if TGR_RENDERER == TGR_RENDERER_D3D
static inline DWORD WidenColorChannel5(DWORD dwChannel) {
    // The low bits are repeated, not the high bits.
    return dwChannel << 3 | (dwChannel & 7);
}

DWORD ConvertColor16To32(DWORD dwColor16) {
    DWORD dwSwapped = (dwColor16 & 0xff) << 8 | (dwColor16 & 0xffff) >> 8;
    DWORD dwAlpha = (dwSwapped & 1) ? 0xff : 0;
    return dwAlpha << 24 | WidenColorChannel5(dwSwapped >> 1 & 0x1f) << 16 |
           WidenColorChannel5(dwSwapped >> 6 & 0x1f) << 8 | WidenColorChannel5(dwSwapped >> 11);
}
#endif

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

// The texels of the side of a texture at level of detail 0.
#define TEXTURE_LOD_0_SIZE 256
// The highest level of detail, of a single texel.
#define TEXTURE_LOD_MAX 8

void CalculateTextureDimensions(int *pWidth, int *pHeight, int nLod, int nAspectRatio) {
    *pWidth = static_cast<unsigned>(nLod) <= TEXTURE_LOD_MAX ? TEXTURE_LOD_0_SIZE >> nLod : 1;
    switch (nAspectRatio) {
    case 0:
        *pHeight = *pWidth / 8;
        break;
    case 1:
        *pHeight = *pWidth / 4;
        break;
    case 2:
        *pHeight = *pWidth / 2;
        break;
    case 3:
        *pHeight = *pWidth;
        break;
    case 4:
        *pHeight = *pWidth;
        *pWidth /= 2;
        break;
    case 5:
        *pHeight = *pWidth;
        *pWidth /= 4;
        break;
    case 6:
        *pHeight = *pWidth;
        *pWidth /= 8;
        break;
    default:
        break;
    }
}

#if TGR_RENDERER == TGR_RENDERER_D3D
// The bytes of a texel of the textures of Direct3D.
#define RGBA32_BYTES_PER_TEXEL 4

int ComputeMipChainByteSize(int nSmallLod, int nLargeLod, int nAspectRatio) {
    int nWidth;
    int nHeight;
    int nBytes = 0;

    CalculateTextureDimensions(&nWidth, &nHeight, nLargeLod, nAspectRatio);
    for (int nLevel = nLargeLod; nLevel <= nSmallLod; nLevel++) {
        nBytes += nHeight * nWidth * RGBA32_BYTES_PER_TEXEL;
        nWidth >>= 1;
        nHeight >>= 1;
    }
    return nBytes;
}
#endif

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

int g_dwCopyTextureDataOnRelocate;

void SetCopyTextureDataOnRelocate(int bCopy) {
    g_dwCopyTextureDataOnRelocate = bCopy;
}

int g_dwTextureLoadCategory;

void SetTextureLoadCategory(int nCategory) {
    g_dwTextureLoadCategory = nCategory;
}

DWORD g_adwDamageTextureLevels[DAMAGE_TEXTURE_LEVEL_COUNT];

void ClearDamageTextureLevels(void) {
    memset(g_adwDamageTextureLevels, 0, sizeof(g_adwDamageTextureLevels));
}

void CopyDamageTextureLevels(DWORD *pdwLevels) {
    memcpy(pdwLevels, g_adwDamageTextureLevels, sizeof(g_adwDamageTextureLevels));
}

int GetTexelsPerLoadWord(int nSize) {
    switch (nSize) {
    case 0:
        return 16;
    case 1:
        return 8;
    case 2:
        return 4;
    default:
        return 2;
    }
}

// The inputs of the colour combiner that are not selectors, and the base of those that are.
#define COMBINER_INPUT_ZERO 0
#define COMBINER_INPUT_ONE 1
#define COMBINER_INPUT_SELECTOR_BASE 1000
// The input the narrow fields take as selector 0.
#define COMBINER_INPUT_NARROW_SELECTOR_0 1013

int EncodeCombinerSelectorNarrow(int nInput) {
    switch (nInput) {
    case COMBINER_INPUT_ZERO:
        return 7;
    case COMBINER_INPUT_ONE:
        return 6;
    case COMBINER_INPUT_NARROW_SELECTOR_0:
        return 0;
    default:
        return nInput - COMBINER_INPUT_SELECTOR_BASE;
    }
}

int EncodeCombinerSelectorWide(int nInput) {
    switch (nInput) {
    case COMBINER_INPUT_ZERO:
        return 31;
    case COMBINER_INPUT_ONE:
        return 6;
    default:
        return nInput - COMBINER_INPUT_SELECTOR_BASE;
    }
}

void CreateGlobalEffectTextures(void) {
    LoadHudOverlayTexture();
    LoadHudGlyphTextureSets();
    LoadParticleTexture();
    LoadTireTrailTexture();
    LoadVehicleEnvMapTexture();
    CreateFallbackEnvironmentTexture();
    CreateAlphaRampTextures();
}

TextureUploadRequest g_aTextureUploadQueue[TEXTURE_UPLOAD_QUEUE_SIZE];
int g_dwTextureUploadQueueHead;
int g_dwTextureUploadQueueTail;
HANDLE g_hTextureUploadQueueMutex;

void InitializeTextureUploadQueue(void) {
    g_dwTextureUploadQueueHead = 0;
    g_dwTextureUploadQueueTail = 0;
    g_hTextureUploadQueueMutex = CreateMutexA(nullptr, false, nullptr);
}

void DestroyTextureUploadQueue(void) {
    g_dwTextureUploadQueueHead = 0;
    g_dwTextureUploadQueueTail = 0;
    CloseHandle(g_hTextureUploadQueueMutex);
    g_hTextureUploadQueueMutex = nullptr;
}

void ProcessTextureUploadQueueEntry(void) {
    WaitForSingleObject(g_hTextureUploadQueueMutex, INFINITE);
    if (g_dwTextureUploadQueueTail != g_dwTextureUploadQueueHead) {
        const TextureUploadRequest &request = g_aTextureUploadQueue[g_dwTextureUploadQueueTail];
        g_pfnUploadTextureEntryToSurface(request.nEntry, request.dwData);
        if (++g_dwTextureUploadQueueTail >= TEXTURE_UPLOAD_QUEUE_SIZE) {
            g_dwTextureUploadQueueTail = 0;
        }
    }
    ReleaseMutex(g_hTextureUploadQueueMutex);
}

void ResetModelDataPool(void) {
    g_dwConvertedVertexCount = 0;
    g_pModelDataPoolCursor = g_afModelVertexPool;
}
