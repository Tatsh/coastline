#include "platform/win/render.h"

#include <bit>
#include <cstdlib>
#include <cstring>
#include <new>

#include "displaylistops.h"
#include "platform/win/boot/app.h"
#include "platform/win/d3d/renderer.h"
#include "platform/win/d3d/texture.h"
#include "platform/win/displaylist.h"
#include "platform/win/fileio.h"
#include "platform/win/settings.h"
#include "screen.h"
#include "weather.h"

int g_dwRenderBackendMode;

#if TGR_RENDERER == TGR_RENDERER_D3D
GbiVertexSource g_aClipVertexPool[GBI_CLIP_VERTEX_POOL_SIZE];
GbiVertexSource *g_pVertexFreeList;
int g_bRdpBlendStateOverride;

void InitGbiD3DRenderer(void) {
    g_pfnRenderBackendSubmitDisplayList = RenderGbiDisplayListFrame;
    g_pfnRenderBackendUpdateFog = UpdateD3DFogRenderState;
    g_pfnRenderBackendShutdown = NullStub;
    GbiVertexSource *pNext = nullptr;
    for (int i = GBI_CLIP_VERTEX_POOL_SIZE - 1; i >= 0; --i) {
        g_aClipVertexPool[i].pNext = pNext;
        pNext = &g_aClipVertexPool[i];
    }
    g_pVertexFreeList = pNext;
    g_dwGbiDisplayListStackDepth = 0;
    g_dwGbiMatrixStackDepth = 1;
    for (auto &vertex : g_aGbiVertexCache) {
        vertex.nRenderBufferIndex = -1;
    }
    for (auto &vertex : g_renderBuffer) {
        vertex.specular = 0;
    }
    g_bRdpBlendStateOverride = 0;
}

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

VertexConvCacheEntry g_aVertexConvCache[VERTEX_CONV_CACHE_SIZE];
int g_dwVertexConvCacheCount;
void *g_apRelocatedPointers[RELOCATED_POINTER_COUNT];
int g_dwRelocatedPointerCount;

void ResetVertexConversionCache(void) {
    g_dwVertexConvCacheCount = 0;
    g_dwRelocatedPointerCount = 0;
}

void AddVertexConversionCacheEntry(const Vtx *pSource,
                                   int nVertexCount,
                                   ConvertedModelVertex *pFloatVerts) {
    if (g_dwVertexConvCacheCount >= VERTEX_CONV_CACHE_SIZE) {
        return;
    }
    VertexConvCacheEntry &entry = g_aVertexConvCache[g_dwVertexConvCacheCount++];
    entry.pSource = pSource;
    entry.nVertexCount = nVertexCount;
    entry.pFloatVerts = pFloatVerts;
}

#if TGR_RENDERER == TGR_RENDERER_D3D
static inline unsigned short SwapVertexHalfword(unsigned short nValue) {
    return nValue >> 8 | nValue << 8;
}

void ByteSwapVertexArray(Vtx *pVertices, int nCount) {
    for (int nVertex = 0; nVertex < nCount; nVertex++) {
        for (int nCoordinate = 0; nCoordinate < 3; nCoordinate++) {
            pVertices[nVertex].v.ob[nCoordinate] =
                SwapVertexHalfword(pVertices[nVertex].v.ob[nCoordinate]);
        }
        pVertices[nVertex].v.flag = SwapVertexHalfword(pVertices[nVertex].v.flag);
        for (int nCoordinate = 0; nCoordinate < 2; nCoordinate++) {
            pVertices[nVertex].v.tc[nCoordinate] =
                SwapVertexHalfword(pVertices[nVertex].v.tc[nCoordinate]);
        }
    }
}

ConvertedModelVertex *ConvertVerticesToFloat(const Vtx *pVertices, int nCount) {
    ConvertedModelVertex *pFirst = g_pModelDataPoolCursor;
    for (int nVertex = 0; nVertex < nCount; nVertex++) {
        g_pModelDataPoolCursor->position.x = pVertices[nVertex].n.ob[0];
        g_pModelDataPoolCursor->position.y = pVertices[nVertex].n.ob[1];
        g_pModelDataPoolCursor->position.z = pVertices[nVertex].n.ob[2];
        g_pModelDataPoolCursor->texture.x = pVertices[nVertex].n.tc[0];
        g_pModelDataPoolCursor->texture.y = pVertices[nVertex].n.tc[1];
        g_pModelDataPoolCursor->normal.x = pVertices[nVertex].n.n[0] * (1.0f / 128.0f);
        g_pModelDataPoolCursor->normal.y = pVertices[nVertex].n.n[1] * (1.0f / 128.0f);
        g_pModelDataPoolCursor->normal.z = pVertices[nVertex].n.n[2] * (1.0f / 128.0f);
        g_pModelDataPoolCursor++;
        g_dwConvertedVertexCount++;
    }
    return pFirst;
}

void ConvertN64VerticesCached(ModelVertexData *pVertices, int nCount) {
    Vtx *pSource = pVertices->pSource;
    if (pSource == nullptr || nCount == 0) {
        return;
    }
    for (int nEntry = 0; nEntry < g_dwVertexConvCacheCount; nEntry++) {
        if (g_aVertexConvCache[nEntry].pSource == pSource &&
            g_aVertexConvCache[nEntry].nVertexCount == nCount) {
            pVertices->pConverted = g_aVertexConvCache[nEntry].pFloatVerts;
            return;
        }
    }
    ByteSwapVertexArray(pSource, nCount);
    ConvertedModelVertex *pConverted = ConvertVerticesToFloat(pSource, nCount);
    AddVertexConversionCacheEntry(pSource, nCount, pConverted);
    pVertices->pConverted = pConverted;
}

void RelocateGbiVertexCommand(GbiCommand *pCommand) {
    pCommand->w1 = (g_dwOldBaseAddress & 0xff000000) | (pCommand->w1 & 0xffffff);
    RelocatePointerValue(&pCommand->w1);
    ConvertN64VerticesCached(&pCommand->vertexData, pCommand->w0 >> 10 & 0x3f);
}

static inline unsigned SwapDisplayListWord(unsigned dwWord) {
    return dwWord << 24 | (dwWord << 8 & 0xff0000) | (dwWord >> 8 & 0xff00) | dwWord >> 24;
}

template <typename T>
static inline void SwapAndRelocateTexturePointer(T *&pPointer) {
    unsigned dwAddress = SwapDisplayListWord(std::bit_cast<unsigned>(pPointer));
    RelocatePointerValue(&dwAddress);
    pPointer = std::bit_cast<T *>(dwAddress);
}

void ByteSwapTextureVariantTable(TextureAnimation *pAnimation) {
    if (pAnimation == nullptr) {
        return;
    }
    pAnimation->frameCount = SwapVertexHalfword(pAnimation->frameCount);
    unsigned dwHeaderWord;
    memcpy(&dwHeaderWord, pAnimation->unreconstructed4, sizeof(dwHeaderWord));
    dwHeaderWord = SwapDisplayListWord(dwHeaderWord);
    memcpy(pAnimation->unreconstructed4, &dwHeaderWord, sizeof(dwHeaderWord));
    for (int nFrame = 0; nFrame < pAnimation->frameCount; nFrame++) {
        pAnimation->frames[nFrame].time = SwapDisplayListWord(pAnimation->frames[nFrame].time);
        pAnimation->frames[nFrame].imageRom =
            SwapDisplayListWord(pAnimation->frames[nFrame].imageRom);
        pAnimation->frames[nFrame].paletteRom =
            SwapDisplayListWord(pAnimation->frames[nFrame].paletteRom);
    }
}

void NullStub2(void *pMemory) {
}

void ByteSwapAndRelocateTextureDescriptor(ModelTexture *pTexture) {
    SwapAndRelocateTexturePointer(pTexture->image);
    SwapAndRelocateTexturePointer(pTexture->palette);
    pTexture->source.paletteRomIndex = SwapDisplayListWord(pTexture->source.paletteRomIndex);
    pTexture->width = SwapVertexHalfword(pTexture->width);
    pTexture->height = SwapVertexHalfword(pTexture->height);
    pTexture->tileT0 = SwapVertexHalfword(pTexture->tileT0);
    pTexture->tileS0 = SwapVertexHalfword(pTexture->tileS0);
    pTexture->tileT1 = SwapVertexHalfword(pTexture->tileT1);
    pTexture->tileS1 = SwapVertexHalfword(pTexture->tileS1);
    pTexture->textureFlags = SwapDisplayListWord(pTexture->textureFlags);
    if (g_dwCopyTextureDataOnRelocate) {
        unsigned nPaletteBytes = (pTexture->textureFlags & 0xf000000) == 0x1000000 ? 32 : 512;
        if (pTexture->textureFlags & 0x100000) {
            RelocatePointerValue(&pTexture->source.paletteRomIndex);
            TextureAnimation *pAnimation = pTexture->source.animation;
            ByteSwapTextureVariantTable(pAnimation);
            // The sentinel suppresses image copying even when the selected image exists.
            int nFrame = pAnimation->frameCount == 2 && pAnimation->frames[0].time == 0xffffffff;
            unsigned nImageOffset = pAnimation->frames[nFrame].imageRom;
            unsigned nPaletteOffset = pAnimation->frames[nFrame].paletteRom;
            unsigned nImageBytes = pTexture->textureFlags & 0x3ffff;
            if (nFrame == 0 && nImageBytes != 0 && nImageOffset != 0xffffffff) {
                memcpy(pTexture->image, g_pTrackTextureTable + nImageOffset, nImageBytes);
            }
            if (pTexture->palette != nullptr && nPaletteOffset != 0xffffffff) {
                memcpy(pTexture->palette, g_pTrackTextureTable + nPaletteOffset, nPaletteBytes);
            }
        } else if (pTexture->palette != nullptr) {
            unsigned nPaletteOffset = (pTexture->source.paletteRomIndex & 0xfff) << 5;
            memcpy(pTexture->palette, g_pTrackTextureTable + nPaletteOffset, nPaletteBytes);
        }
    }
    NullStub2(pTexture->palette);
}

void ByteSwapTextureDescriptorArray(ModelTexture *pTextures, int nCount) {
    for (int nTexture = 0; nTexture < nCount; nTexture++) {
        ByteSwapAndRelocateTextureDescriptor(&pTextures[nTexture]);
    }
}

void ByteSwapAndRelocateDisplayList(GbiCommand *pCommands) {
    if (pCommands == nullptr) {
        return;
    }
    AddPointerToRelocatedList(pCommands);
    for (GbiCommand *pCommand = pCommands;; pCommand++) {
        pCommand->w0 = SwapDisplayListWord(pCommand->w0);
        pCommand->w1 = SwapDisplayListWord(pCommand->w1);
        switch (pCommand->w0 >> 24) {
        case G_VTX:
            RelocateGbiVertexCommand(pCommand);
            break;
        case G_TRI2:
            HalveGbiTrianglePairVertexIndices(pCommand);
            break;
        case G_ENDDL:
            return;
        case G_TRI1:
            HalveGbiTriangleVertexIndices(pCommand);
            break;
        case G_SETTIMG:
            RelocatePointerValue(&pCommand->w1);
            break;
        }
    }
}
#endif

bool IsPointerInRelocatedList(void *pPointer) {
    if (pPointer == nullptr) {
        return true;
    }
    for (int i = 0; i < g_dwRelocatedPointerCount; i++) {
        if (g_apRelocatedPointers[i] == pPointer) {
            return true;
        }
    }
    return false;
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

TextureMapEntry g_aTextureMapEntries[TEXTURE_MAP_ENTRY_COUNT];
int g_dwTextureMapEntryCount;
unsigned g_dwTextureMapAddressBias;

bool LookupTextureMapEntry(unsigned dwAddress, int *pnWidth, int *pnHeight) {
    unsigned dwKey = dwAddress + g_dwTextureMapAddressBias;

    for (int i = g_dwTextureMapEntryCount - 1; i >= 0; i--) {
        if (g_aTextureMapEntries[i].dwAddress == dwKey) {
            *pnWidth = g_aTextureMapEntries[i].nWidth;
            *pnHeight = g_aTextureMapEntries[i].nHeight;
            return true;
        }
    }
    return false;
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

void (*g_pfnReloadTextureEntryData)(int nEntry);

void IncrementDamageTextureLevel(int nTexture, const int *pnEntries, int *pnLevels) {
    if (nTexture < 0 || nTexture >= DAMAGE_TEXTURE_COUNT) {
        return;
    }
    if (pnLevels[nTexture] >= DAMAGE_TEXTURE_MAX_LEVEL) {
        return;
    }
    pnLevels[nTexture]++;
    SetTextureEntryDamageLevel(pnEntries[nTexture], pnLevels[nTexture]);
    g_pfnReloadTextureEntryData(pnEntries[nTexture]);
}

// The textures ApplyRandomAdjacentDamageTexture() chooses from: the texture and its neighbours.
#define DAMAGE_TEXTURE_CHOICES 3
// The range of rand().
#define RAND_RANGE 32768

void ApplyRandomAdjacentDamageTexture(int nTexture, const int *pnEntries, int *pnLevels) {
    int anChoices[DAMAGE_TEXTURE_CHOICES];
    int nChoices = 0;
    int nNeighbour;

    if (nTexture < 0 || nTexture >= DAMAGE_TEXTURE_COUNT) {
        return;
    }
    if (pnLevels[nTexture] < DAMAGE_TEXTURE_MAX_LEVEL) {
        anChoices[nChoices++] = nTexture;
    }
    nNeighbour = nTexture - 1;
    if (nNeighbour < 0) {
        nNeighbour += DAMAGE_TEXTURE_COUNT;
    }
    if (pnLevels[nNeighbour] < DAMAGE_TEXTURE_MAX_LEVEL) {
        anChoices[nChoices++] = nNeighbour;
    }
    nNeighbour = nTexture + 1;
    if (nNeighbour >= DAMAGE_TEXTURE_COUNT) {
        nNeighbour -= DAMAGE_TEXTURE_COUNT;
    }
    if (pnLevels[nNeighbour] < DAMAGE_TEXTURE_MAX_LEVEL) {
        anChoices[nChoices++] = nNeighbour;
    }
    if (nChoices == 0) {
        return;
    }
    IncrementDamageTextureLevel(anChoices[rand() * nChoices / RAND_RANGE], pnEntries, pnLevels);
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

// The opcode of the command that sets the combine mode.
#define GBI_SET_COMBINE_OPCODE 0xfcu

void PackCombineMode(GbiCommand *pCommand,
                     int nColorA0,
                     int nColorB0,
                     int nColorC0,
                     int nColorD0,
                     int nAlphaA0,
                     int nAlphaB0,
                     int nAlphaC0,
                     int nAlphaD0,
                     int nColorA1,
                     int nColorB1,
                     int nColorC1,
                     int nColorD1,
                     int nAlphaA1,
                     int nAlphaB1,
                     int nAlphaC1,
                     int nAlphaD1) {
    unsigned dwColorA0 = EncodeCombinerSelectorWide(nColorA0);
    unsigned dwColorB0 = EncodeCombinerSelectorWide(nColorB0);
    unsigned dwColorC0 = EncodeCombinerSelectorWide(nColorC0);
    unsigned dwColorD0 = EncodeCombinerSelectorWide(nColorD0);
    unsigned dwAlphaA0 = EncodeCombinerSelectorNarrow(nAlphaA0);
    unsigned dwAlphaB0 = EncodeCombinerSelectorNarrow(nAlphaB0);
    unsigned dwAlphaC0 = EncodeCombinerSelectorNarrow(nAlphaC0);
    unsigned dwAlphaD0 = EncodeCombinerSelectorNarrow(nAlphaD0);
    unsigned dwColorA1 = EncodeCombinerSelectorWide(nColorA1);
    unsigned dwColorB1 = EncodeCombinerSelectorWide(nColorB1);
    unsigned dwColorC1 = EncodeCombinerSelectorWide(nColorC1);
    unsigned dwColorD1 = EncodeCombinerSelectorWide(nColorD1);
    unsigned dwAlphaA1 = EncodeCombinerSelectorNarrow(nAlphaA1);
    unsigned dwAlphaB1 = EncodeCombinerSelectorNarrow(nAlphaB1);
    unsigned dwAlphaC1 = EncodeCombinerSelectorNarrow(nAlphaC1);
    unsigned dwAlphaD1 = EncodeCombinerSelectorNarrow(nAlphaD1);

    pCommand->w0 = GBI_SET_COMBINE_OPCODE << 24 | (dwColorA0 & 0xf) << 20 |
                   (dwColorC0 & 0x1f) << 15 | (dwAlphaA0 & 7) << 12 | (dwAlphaC0 & 7) << 9 |
                   (dwColorA1 & 0xf) << 5 | (dwColorC1 & 0x1f);
    pCommand->w1 = (dwColorB0 & 0xf) << 28 | (dwColorB1 & 0xf) << 24 | (dwAlphaA1 & 7) << 21 |
                   (dwAlphaC1 & 7) << 18 | (dwColorD0 & 7) << 15 | (dwAlphaB0 & 7) << 12 |
                   (dwAlphaD0 & 7) << 9 | (dwColorD1 & 7) << 6 | (dwAlphaB1 & 7) << 3 |
                   (dwAlphaD1 & 7);
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

#if TGR_RENDERER == TGR_RENDERER_D3D
void SelectTextureQualityFromVideoMemory(void) {
    if (g_dwAvailableTextureMemory > g_dwHiResTextureMemoryThreshold &&
        !g_dwD3DAlwaysSquareTextures) {
        g_dwTextureQualityLevel =
            g_dwTotalPhysicalMemory > 0x2000000 ? g_dwAvailableTextureMemory < 4000000 : 1;
        g_dwMaxTexWidthHighQuality = 256;
        g_dwMaxTexHeightHighQuality = 256;
        g_dwMaxTexWidthLowQuality = 256;
        g_dwMaxTexHeightLowQuality = 256;
    } else {
        g_dwTextureQualityLevel = 2;
        g_dwMaxTexWidthHighQuality = 64;
        g_dwMaxTexHeightHighQuality = 64;
        g_dwMaxTexWidthLowQuality = 32;
        g_dwMaxTexHeightLowQuality = 32;
    }
}

unsigned AllocateTextureEntry(void) {
    if (g_dwTextureEntryCount >= g_dwTextureEntryCapacity) {
        g_dwTextureEntryCapacity += 256;
        g_pTextureEntries = static_cast<TextureEntry *>(
            CHK_ReAllocateMemory(g_pTextureEntries,
                                 g_dwTextureEntryCapacity * sizeof(TextureEntry),
                                 g_szAppendTextureAtdb));
    }
    return g_dwTextureEntryCount++;
}

unsigned CreateTextureFromN64ImageAutoLoadSize(const unsigned char *pImage,
                                               const unsigned char *pPalette,
                                               int nWidth,
                                               int nHeight,
                                               int nSize,
                                               int nFormat,
                                               int nOption6,
                                               int nOption7,
                                               int nOption8,
                                               int nOption9,
                                               int nOption10,
                                               int nOption11,
                                               int nOption12,
                                               int nOption13) {
    int nLoadBytes = ((1 << GetMipLevelFromDimension(nWidth)) / GetTexelsPerLoadWord(nSize)) * 8;
    return g_pfnCreateTextureFromN64Image(pImage,
                                          pPalette,
                                          nWidth,
                                          nHeight,
                                          nLoadBytes,
                                          nSize,
                                          nFormat,
                                          nOption6,
                                          nOption7,
                                          nOption8,
                                          nOption9,
                                          nOption10,
                                          nOption11,
                                          nOption12,
                                          nOption13);
}

unsigned DuplicateTextureEntryHalfScale(unsigned nTexture) {
    unsigned nDuplicate = AllocateTextureEntry();
    g_pTextureEntries[nDuplicate] = g_pTextureEntries[nTexture];
    g_pTextureEntries[nDuplicate].fTextureScaleS *= 0.5f;
    g_pTextureEntries[nDuplicate].fTextureScaleT *= 0.5f;
    return nDuplicate;
}

void LoadHudOverlayTexture(void) {
    g_dwHudOverlayTextureId = g_pfnCreateTextureFromN64ImageAutoLoadSize(
        g_abHudOverlayTextureData, nullptr, 64, 64, 1, 4, 0, 0, 1, 1, 15, 15, 1, 0);
    g_dwHudOverlayHalfTextureId = DuplicateTextureEntryHalfScale(g_dwHudOverlayTextureId);
}

void LoadParticleTexture(void) {
    g_dwParticleTextureId = g_pfnCreateTextureFromN64ImageAutoLoadSize(
        g_abHudTexture64AData, nullptr, 64, 64, 1, 4, 0, 0, 1, 1, 0, 0, 1, 0);
}

void LoadTireTrailTexture(void) {
    g_dwTireTrailTextureId = g_pfnCreateTextureFromN64ImageAutoLoadSize(
        g_abHudTexture32x64Data, nullptr, 32, 64, 0, 4, 0, 0, 0, 0, 0, 0, 1, 0);
}

void LoadVehicleEnvMapTexture(void) {
    g_dwVehicleEnvMapTextureId = g_pfnCreateTextureFromN64ImageAutoLoadSize(
        g_abHudTexture64BData, nullptr, 64, 64, 0, 4, 0, 0, 0, 0, 0, 0, 1, 0);
}

void CreateFallbackEnvironmentTexture(void) {
    g_dwFallbackEnvMapTextureId = g_pfnCreateTextureFromN64ImageAutoLoadSize(
        g_abFallbackEnvTextureData, nullptr, 64, 64, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0);
}

void CreateAlphaRampTextures(void) {
    g_adwPrecipitationTextureIds[0] = g_pfnCreateTextureFromN64ImageAutoLoadSize(
        g_abPrecipitationTextures[0], nullptr, 64, 64, 1, 4, 0, 0, 1, 1, 0, 0, 1, 0);
    g_adwPrecipitationTextureIds[1] = g_pfnCreateTextureFromN64ImageAutoLoadSize(
        g_abPrecipitationTextures[1], nullptr, 64, 64, 1, 4, 0, 0, 1, 1, 0, 0, 1, 0);
    for (int nAlpha = 0; nAlpha < 16; nAlpha++) {
        g_abAlphaRampRgba[nAlpha][0] = 255;
        g_abAlphaRampRgba[nAlpha][1] = 255;
        g_abAlphaRampRgba[nAlpha][2] = 255;
        g_abAlphaRampRgba[nAlpha][3] = nAlpha << 4 | nAlpha;
    }
    g_dwAlphaRampStripRowCount =
        ConvertRGBAToPixelFormat(g_abAlphaRampPixelStrip, g_abAlphaRampRgba, 64, 2) / 16;
}

void LoadHudGlyphTextureSets(void) {
    if (g_dwTextureQualityLevel <= 1) {
        for (int nGlyph = 0; nGlyph < 27; nGlyph++) {
            g_adwLargeFontGlyphTexturesA[nGlyph] = g_pfnCreateTextureFromN64Image(
                g_abHudFont0GlyphPixels + g_adwHudFont0GlyphOffsets[nGlyph],
                nullptr,
                64,
                64,
                0x2c0,
                1,
                3,
                0,
                0,
                0,
                0,
                0,
                0,
                1,
                0);
        }
        for (int nGlyph = 0; nGlyph < 26; nGlyph++) {
            g_adwLargeFontGlyphTexturesB[nGlyph] = g_pfnCreateTextureFromN64Image(
                g_abHudFont1GlyphPixels + g_adwHudFont1GlyphOffsets[nGlyph],
                nullptr,
                64,
                64,
                0x2c0,
                1,
                3,
                0,
                0,
                0,
                0,
                0,
                0,
                1,
                0);
        }
    }
    for (int nGlyph = 0; nGlyph < 27; nGlyph++) {
        g_adwSmallFontGlyphTexturesA[nGlyph] = g_pfnCreateTextureFromN64Image(
            g_abHudFont2GlyphPixels + g_adwHudFont2GlyphOffsets[nGlyph],
            nullptr,
            32,
            32,
            0x188,
            1,
            3,
            0,
            0,
            0,
            0,
            0,
            0,
            1,
            0);
    }
    for (int nGlyph = 0; nGlyph < 26; nGlyph++) {
        g_adwSmallFontGlyphTexturesB[nGlyph] = g_pfnCreateTextureFromN64Image(
            g_abHudFont3GlyphPixels + g_adwHudFont3GlyphOffsets[nGlyph],
            nullptr,
            32,
            32,
            0x188,
            1,
            3,
            0,
            0,
            0,
            0,
            0,
            0,
            1,
            0);
    }
}

void CreateDaySkyTexture(void) {
    g_apVehicleReflectionTextures[0] = g_pfnCreateTextureFromN64ImageAutoLoadSize(
        g_abDaySkyTexture, g_abDaySkyPalette, 32, 128, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0);
}

void CreateNightSkyTexture(void) {
    unsigned nTexture = g_pfnCreateTextureFromN64ImageAutoLoadSize(
        g_abNightSkyTexture, g_abNightSkyPalette, 32, 128, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0);
    g_apVehicleReflectionTextures[2] = nTexture;
    g_apVehicleReflectionTextures[1] = nTexture;
}
#endif

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
        g_pfnUploadTextureEntryToSurface(request.nEntry, request.payload);
        if (++g_dwTextureUploadQueueTail >= TEXTURE_UPLOAD_QUEUE_SIZE) {
            g_dwTextureUploadQueueTail = 0;
        }
    }
    ReleaseMutex(g_hTextureUploadQueueMutex);
}

void EnqueueTextureUploadEntry(int nEntry, TextureUploadData payload) {
    WaitForSingleObject(g_hTextureUploadQueueMutex, INFINITE);
    g_aTextureUploadQueue[g_dwTextureUploadQueueHead].nEntry = nEntry;
    g_aTextureUploadQueue[g_dwTextureUploadQueueHead].payload = payload;
    // A full ring overwrites the textures still waiting.
    if (++g_dwTextureUploadQueueHead >= TEXTURE_UPLOAD_QUEUE_SIZE) {
        g_dwTextureUploadQueueHead = 0;
    }
    ReleaseMutex(g_hTextureUploadQueueMutex);
}

void ResetModelDataPool(void) {
    g_dwConvertedVertexCount = 0;
    g_pModelDataPoolCursor = g_afModelVertexPool;
}

unsigned char g_abWeatherShadeTexture[16];
unsigned g_dwWeatherShadeTextureId;

void CreateWeatherShadeTexture(void) {
    unsigned char intensity = g_dwChosenWeather == 2 || g_dwChosenWeather == 3 ? 0x20 : 0x80;
    std::memset(g_abWeatherShadeTexture, intensity, sizeof(g_abWeatherShadeTexture));
    g_dwWeatherShadeTextureId = g_pfnCreateTextureFromN64ImageAutoLoadSize(
        g_abWeatherShadeTexture, nullptr, 4, 4, G_IM_SIZ_8b, G_IM_FMT_I, 0, 0, 1, 1, 0, 0, 1, 0);
}

unsigned QueryAvailableTextureMemory(void) {
    DDSCAPS caps;
    caps.dwCaps = DDSCAPS_TEXTURE;
    DWORD total;
    DWORD available;
    g_pRenderHost->pRenderer->pDirectDraw2->GetAvailableVidMem(&caps, &total, &available);
    return available;
}

unsigned g_dwTextureIdCount;
unsigned g_dwTextureIdCapacity;

void ShutdownTextureSubsystem(void) {
    DestroyTextureUploadQueue();
    for (unsigned i = 1; i <= g_dwTextureIdCount; i++) {
        g_ppTextureIdTable[i]->ReleaseD3DTextureInterfaces();
        ::operator delete(g_ppTextureIdTable[i]);
    }
    std::free(g_ppTextureIdTable);
    g_dwTextureIdCount = 0;
    g_dwTextureIdCapacity = 0;
    g_ppTextureIdTable = nullptr;
    for (unsigned i = 0; i < g_dwTextureEntryCount; i++) {
        TextureEntry &entry = g_pTextureEntries[i];
        if (entry.data.bOwnsPaintBuffers) {
            for (unsigned char *&pixels : entry.data.apPaintBuffers) {
                if (pixels) {
                    std::free(pixels);
                    pixels = nullptr;
                }
            }
        }
    }
    g_dwTextureEntryCount = 0;
    g_dwTextureEntryCapacity = 0;
    std::free(g_pTextureEntries);
    g_pTextureEntries = nullptr;
}

void UploadTextureEntryToSurface(int nEntry, TextureUploadData payload) {
    if (nEntry == static_cast<int>(g_dwWeatherShadeTextureId)) {
        return;
    }
    const TextureEntry &entry = g_pTextureEntries[nEntry];
    int width;
    int height;
    CalculateTextureDimensions(&width, &height, entry.data.nLod, entry.data.nAspectRatio);
    g_ppTextureIdTable[entry.nTextureId]->UploadSurfaceToTexture(payload.pixels, width, height);
}

void CreateDeviceTextureFromImageHeader(ModelTexture *pTexture, int nOption13) {
    if (!pTexture->image || (pTexture->textureFlags & 0x100000)) {
        return;
    }
    unsigned flags = pTexture->textureFlags;
    unsigned format = flags >> 24 & 0xf;
    int size;
    int n64Format;
    if (format == TEXTURE_FORMAT_CI4) {
        size = G_IM_SIZ_4b;
        n64Format = G_IM_FMT_CI;
    } else if (format == TEXTURE_FORMAT_I8) {
        size = G_IM_SIZ_8b;
        n64Format = G_IM_FMT_I;
    } else {
        size = G_IM_SIZ_16b;
        n64Format = G_IM_FMT_RGBA;
    }
    int width = 1 << GetMipLevelFromDimension(pTexture->width);
    int height = 1 << GetMipLevelFromDimension(pTexture->height);
    pTexture->textureEntry = g_pfnCreateTextureFromN64ImageAutoLoadSize(
        pTexture->image,
        reinterpret_cast<const unsigned char *>(pTexture->palette),
        width,
        height,
        size,
        n64Format,
        flags >> 31,
        flags >> 30 & 1,
        flags >> 29 & 1,
        flags >> 28 & 1,
        0,
        0,
        1,
        nOption13);
}

int CalculateTextureLodFromDimensions(int *pLod, int width, int height) {
    int dimension = width > height ? width : height;
    *pLod = TEXTURE_LOD_MAX - GetMipLevelFromDimension(dimension);
    return dimension <= TEXTURE_LOD_0_SIZE;
}

int ChooseTexturePixelFormat(int nSize, int nFormat, int bColorCombine) {
    if (nSize == G_IM_SIZ_4b && nFormat == G_IM_FMT_I) {
        return bColorCombine == 1 ? 0xb : 2;
    }
    if (nSize == G_IM_SIZ_8b) {
        if (nFormat == G_IM_FMT_IA) {
            return bColorCombine == 1 ? 0xc : 4;
        }
        if (nFormat == G_IM_FMT_I) {
            return 2;
        }
    }
    return 0xb;
}

int GetTexturePixelSize(int nPixelFormatId) {
    return 4;
}
