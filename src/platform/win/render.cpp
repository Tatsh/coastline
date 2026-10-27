#include "platform/win/render.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

#include "displaylistops.h"
#include "platform/win/boot/app.h"
#include "platform/win/d3d/renderer.h"
#include "platform/win/d3d/texture.h"
#include "platform/win/displaylist.h"
#include "platform/win/fileio.h"
#include "platform/win/paintshop.h"
#include "platform/win/settings.h"
#include "platform/win/texturescan.h"
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

int FindMatchingTextureEntry(const TextureImageData *pImage) {
    for (unsigned i = 0; i < g_dwTextureEntryCount; i++) {
        const TextureImageData &candidate = g_pTextureEntries[i].data;
        if (candidate.pImage != pImage->pImage || candidate.pPalette != pImage->pPalette) {
            continue;
        }
        if (candidate.bColorCombine != 1 || pImage->bColorCombine != 1 ||
            (std::memcmp(candidate.abEnvColor, pImage->abEnvColor, 4) == 0 &&
             std::memcmp(candidate.abPrimColor, pImage->abPrimColor, 4) == 0)) {
            return i;
        }
    }
    return -1;
}

static long double TextureCoordinateShiftFactor(int shift) {
    if (static_cast<unsigned>(shift) <= 10) {
        return static_cast<long double>(1024 >> shift) * (1.0f / 1024.0f);
    }
    int factor = static_cast<int>(1u << ((16u - static_cast<unsigned>(shift)) & 31));
    return factor;
}

unsigned RegisterTextureEntry(const TextureImageData *pImage, int nTextureId) {
    unsigned nEntry = AllocateTextureEntry();
    TextureEntry &entry = g_pTextureEntries[nEntry];
    entry.data = *pImage;
    entry.nTextureId = nTextureId;
    entry.fTextureScaleS = (1.0L / 32.0L) / entry.data.nDecodedWidth;
    entry.fTextureScaleT = (1.0L / 32.0L) / entry.data.nDecodedHeight;
    const TexScanTile &tile = pImage->aTiles[pImage->nTileIndex];
    if (tile.nShiftS) {
        entry.fTextureScaleS = TextureCoordinateShiftFactor(tile.nShiftS) * entry.fTextureScaleS;
    }
    if (tile.nShiftT) {
        entry.fTextureScaleT = TextureCoordinateShiftFactor(tile.nShiftT) * entry.fTextureScaleT;
    }
    return nEntry;
}

void SampleTexturePixelBilinear(unsigned char *pDest,
                                const unsigned char *pTopLeft,
                                const unsigned char *pTopRight,
                                const unsigned char *pBottomLeft,
                                const unsigned char *pBottomRight,
                                float x,
                                float y) {
    unsigned char result[4];
    const int channels[4] = {3, 0, 1, 2};
    for (int channel : channels) {
        long double top = pTopLeft[channel] +
                          (static_cast<long double>(pTopRight[channel]) - pTopLeft[channel]) * x;
        long double bottom =
            pBottomLeft[channel] +
            (static_cast<long double>(pBottomRight[channel]) - pBottomLeft[channel]) * x;
        long double value = top + (bottom - top) * y;
        long double rounded = floorl(static_cast<double>(value + 0.5L));
        if (!(rounded >= 0.0L)) {
            result[channel] = 0;
        } else if (rounded > 255.0L) {
            result[channel] = 255;
        } else {
            result[channel] = static_cast<unsigned char>(rounded);
        }
    }
    std::memcpy(pDest, result, sizeof(result));
}

static int FloorTextureSampleCoordinate(float coordinate) {
    long double value = floorl(static_cast<double>(coordinate));
    if (!(value >= -0x1p63L && value < 0x1p63L)) {
        return 0;
    }
    return static_cast<int>(static_cast<long long>(value));
}

void ResizeTextureImage(unsigned char *pDest,
                        int width,
                        int height,
                        const unsigned char *pSource,
                        int sourceWidth,
                        int sourceHeight,
                        int nPixelFormatId) {
    int pixelSize = GetTexturePixelSize(nPixelFormatId);
    long double verticalRatio = static_cast<long double>(sourceHeight) / height;
    float yStep = verticalRatio;
    float xStep = static_cast<long double>(sourceWidth) / width;
    float y = verticalRatio * 0.5L - 0.5L;
    float xStart = static_cast<long double>(xStep) * 0.5L - 0.5L;
    int maxY = static_cast<int>(static_cast<unsigned>(sourceHeight) - 1);
    int maxX = static_cast<int>(static_cast<unsigned>(sourceWidth) - 1);
    auto pixel = [pSource, sourceWidth, pixelSize](int x, int y) {
        unsigned index = static_cast<unsigned>(y) * static_cast<unsigned>(sourceWidth) +
                         static_cast<unsigned>(x);
        int offset = static_cast<int>(index * static_cast<unsigned>(pixelSize));
        return pSource + offset;
    };
    for (int row = 0; row < height; row++) {
        int iy = FloorTextureSampleCoordinate(y);
        float fractionY = static_cast<long double>(y) - iy;
        int y0 = iy > 0 ? iy : 0;
        int y1 = static_cast<int>(static_cast<unsigned>(iy) + 1);
        if (y1 >= maxY) {
            y1 = maxY;
        }
        float x = xStart;
        for (int column = 0; column < width; column++) {
            int ix = FloorTextureSampleCoordinate(x);
            float fractionX = static_cast<long double>(x) - ix;
            int x0 = ix > 0 ? ix : 0;
            int x1 = static_cast<int>(static_cast<unsigned>(ix) + 1);
            if (x1 >= maxX) {
                x1 = maxX;
            }
            SampleTexturePixelBilinear(pDest,
                                       pixel(x0, y0),
                                       pixel(x1, y0),
                                       pixel(x0, y1),
                                       pixel(x1, y1),
                                       fractionX,
                                       fractionY);
            pDest += pixelSize;
            x = static_cast<long double>(x) + xStep;
        }
        y = static_cast<long double>(y) + yStep;
    }
}

void ReloadTextureEntryData(int nEntry) {
    unsigned char *pixels = GetTextureImagePixels(&g_pTextureEntries[nEntry].data);
    g_pfnUploadTextureEntryToSurface(nEntry, TextureUploadData(pixels));
}

unsigned ResizeTextureMipChain(unsigned char *pDest,
                               const unsigned char *pSource,
                               TextureImageData *pImage) {
    int pixelSize = GetTexturePixelSize(pImage->nPixelFormatId);
    int sourceWidth = pImage->nSourceWidth;
    int sourceHeight = pImage->nSourceHeight;
    int width = pImage->nWidth;
    int height = pImage->nHeight;
    pImage->nDecodedWidth = sourceWidth;
    pImage->nDecodedHeight = sourceHeight;
    unsigned bytes = 0;
    for (int level = g_dwTexScanRenderTileIndex; level < pImage->nTileIndex; level++) {
        ResizeTextureImage(
            pDest, width, height, pSource, sourceWidth, sourceHeight, pImage->nPixelFormatId);
        unsigned outputBytes = static_cast<unsigned>(width) * static_cast<unsigned>(height) *
                               static_cast<unsigned>(pixelSize);
        unsigned sourceBytes = static_cast<unsigned>(sourceWidth) *
                               static_cast<unsigned>(sourceHeight) *
                               static_cast<unsigned>(pixelSize);
        bytes += outputBytes;
        pDest += static_cast<int>(outputBytes);
        pSource += static_cast<int>(sourceBytes);
        width >>= 1;
        height >>= 1;
        sourceWidth >>= 1;
        sourceHeight >>= 1;
    }
    return bytes;
}

void ModulateTextureWithIntensityPlane(const TextureImageData *pImage, unsigned char *pPixels) {
    unsigned imageBytes = static_cast<unsigned>(pImage->nSourceWidth) *
                          static_cast<unsigned>(pImage->nSourceHeight) * 4;
    ResizeTextureImage(g_abTextureResampleBuffer,
                       pImage->nSourceWidth,
                       pImage->nSourceHeight,
                       pPixels + static_cast<int>(imageBytes),
                       pImage->nSourceWidth / 2,
                       pImage->nSourceHeight / 2,
                       0xb);
    const unsigned char *intensity = g_abTextureResampleBuffer;
    for (int row = 0; row < pImage->nSourceHeight; row++) {
        for (int column = 0; column < pImage->nSourceWidth; column++) {
            unsigned color;
            unsigned multiplier;
            std::memcpy(&color, pPixels, sizeof(color));
            std::memcpy(&multiplier, intensity, sizeof(multiplier));
            auto scale = [multiplier](unsigned channel) {
                unsigned product = channel * multiplier;
                return static_cast<unsigned>(
                    (static_cast<unsigned long long>(product) * 0x88888889u) >> 35);
            };
            unsigned result = (scale(color >> 16 & 0xff) << 16) | (scale(color >> 8 & 0xff) << 8) |
                              scale(color & 0xff) | (color & 0xff000000);
            std::memcpy(pPixels, &result, sizeof(result));
            pPixels += 4;
            intensity += 4;
        }
    }
}

unsigned g_dwTexturePixelByteCount;
int g_bForceTextureClone;
const char g_szTidFromTextureAppendAmid[] = "TIDFromTextureAppend: amid";

unsigned char *GetTextureImagePixels(TextureImageData *pImage) {
    if (pImage->dwFlags & 0x40) {
        g_dwTexturePixelByteCount = pImage->nDecodedBytes;
        return pImage->pImage;
    }
    if (pImage->bOwnsPaintBuffers) {
        unsigned char *pixels = pImage->apPaintBuffers[pImage->nDamageLevel];
        g_dwTexturePixelByteCount = pImage->nPaintPixelBytes;
        return pixels ? pixels : pImage->apPaintBuffers[0];
    }
    unsigned char *pixels = g_abDecodedTexturePixels;
    if (!(pImage->dwFlags & 0x10)) {
        const TexScanTile &tile = pImage->aTiles[pImage->nTileIndex];
        DecodeTexturePixels(pixels,
                            pImage->nDecodedBytes,
                            tile.nSize,
                            pImage->pImage,
                            pImage->pPalette,
                            tile.nFormat,
                            pImage->bMirrorS,
                            pImage->bMirrorT,
                            pImage->nTileIndex,
                            pImage->nTileLimit,
                            pImage->aTiles,
                            pImage->dwFlags,
                            pImage->bColorCombine,
                            pImage->abEnvColor[0],
                            pImage->abEnvColor[1],
                            pImage->abEnvColor[2],
                            pImage->abEnvColor[3],
                            pImage->abPrimColor[0],
                            pImage->abPrimColor[1],
                            pImage->abPrimColor[2],
                            pImage->abPrimColor[3],
                            pImage->nRowSwapMask);
    }
    g_dwTexturePixelByteCount = pImage->nDecodedBytes;
    if ((pImage->dwFlags & 0x82) == 0x82) {
        pImage->nLargestLod = pImage->nLod;
        ModulateTextureWithIntensityPlane(pImage, pixels);
    }
    if (pImage->nSourceWidth != pImage->nWidth || pImage->nSourceHeight != pImage->nHeight) {
        pImage->nPixelBytes = ResizeTextureMipChain(g_abTextureResampleBuffer, pixels, pImage);
        g_dwTexturePixelByteCount = pImage->nPixelBytes;
        pixels = g_abTextureResampleBuffer;
    }
    if (pImage->nColorKey) {
        g_dwTexturePixelByteCount = ConvertRGBAToPixelFormatWithColorKey(
            pixels, pixels, pImage->nPixelBytes, pImage->nPixelFormatId, &pImage->nColorKey);
    } else {
        g_dwTexturePixelByteCount =
            ConvertRGBAToPixelFormat(pixels, pixels, pImage->nPixelBytes, pImage->nPixelFormatId);
    }
    return pixels;
}

int CloneTextureEntryWithNewImageData(int nEntry,
                                      unsigned char *pImage,
                                      const unsigned char *pPalette) {
    g_bForceTextureClone = 1;
    TextureImageData image = g_pTextureEntries[nEntry].data;
    image.pImage = pImage;
    image.pPalette = pPalette;
    if (image.bOwnsPaintBuffers) {
        image.nWidth /= 2;
        image.nHeight /= 2;
        image.nPixelBytes /= 4;
        CalculateTextureLodFromDimensions(&image.nLod, image.nWidth, image.nHeight);
        image.nLargestLod = image.nLod;
        image.bOwnsPaintBuffers = 0;
    }
    unsigned char *pixels = GetTextureImagePixels(&image);
    int entry = CreateTextureFromImage(&image, pixels);
    g_bForceTextureClone = 0;
    return entry;
}

int CreateTextureFromImage(TextureImageData *pImage, unsigned char *pPixels) {
    if (!g_bForceTextureClone) {
        int entry = FindMatchingTextureEntry(pImage);
        if (entry != -1) {
            return entry;
        }
    }
    unsigned textureId = ++g_dwTextureIdCount;
    if (textureId >= g_dwTextureIdCapacity) {
        g_dwTextureIdCapacity += 256;
        g_ppTextureIdTable = static_cast<D3DTexture **>(
            CHK_ReAllocateMemory(g_ppTextureIdTable,
                                 g_dwTextureIdCapacity * sizeof(*g_ppTextureIdTable),
                                 g_szTidFromTextureAppendAmid));
    }
    g_ppTextureIdTable[textureId] = new (std::nothrow) D3DTexture{};
    int width;
    int height;
    CalculateTextureDimensions(&width, &height, pImage->nLod, pImage->nAspectRatio);
    g_bTexScanPrimColorSet = 0;
    g_bTexScanEnvColorSet = 0;
    PreparePaintTexture(&width, &height, pImage);
    if (pImage->nColorKey) {
        g_ppTextureIdTable[textureId]->CreateD3DTexture(
            g_pD3DDevice2, pPixels, width, height, pImage->nPixelFormatId, pImage->nColorKey);
    } else {
        g_ppTextureIdTable[textureId]->CreateD3DTextureDefault(
            g_pD3DDevice2, pPixels, width, height, pImage->nPixelFormatId);
    }
    return RegisterTextureEntry(pImage, textureId);
}

void UploadTrackTextureVariants(ModelTexture *pTextures, int nCount) {
    if (nCount <= 0) {
        return;
    }
    unsigned entryCount = g_dwTextureEntryCount;
    for (int i = 0; i < nCount; i++) {
        ModelTexture &texture = pTextures[i];
        if (!texture.image || !(texture.textureFlags & 0x100000)) {
            continue;
        }
        TextureAnimation *animation = texture.source.animation;
        if (animation->frameCount == 2 && animation->frames[0].time == 0xffffffff) {
            continue;
        }
        unsigned sourceEntry = 0;
        while (sourceEntry < entryCount &&
               g_pTextureEntries[sourceEntry].data.pImage != texture.image) {
            sourceEntry++;
        }
        if (!animation->frameCount) {
            continue;
        }
        for (int frame = 0; frame < animation->frameCount; frame++) {
            TextureImageData image = g_pTextureEntries[sourceEntry].data;
            image.pImage = g_pTrackTextureTable + animation->frames[frame].imageRom;
            unsigned paletteOffset = animation->frames[frame].paletteRom;
            if (static_cast<int>(paletteOffset) > 0) {
                image.pPalette = g_pTrackTextureTable + paletteOffset;
                std::memcpy(g_pTlutLoadList, image.pPalette, 512);
            }
            g_bTexScanPrimColorSet = 0;
            g_bTexScanEnvColorSet = 0;
            unsigned char *pixels = GetTextureImagePixels(&image);
            unsigned entry = CreateTextureFromImage(&image, pixels);
            animation->frames[frame].imageRom = entry | (sourceEntry << 16);
        }
        entryCount = g_dwTextureEntryCount;
    }
}

int DecodeN64TextureToPixelBuffer(void *pDest,
                                  const unsigned char *pImage,
                                  const unsigned char *pPalette,
                                  int nWidth,
                                  int nHeight,
                                  int nLineBytes,
                                  int nSize,
                                  int nFormat) {
    int widthBits = GetMipLevelFromDimension(nWidth);
    int heightBits = GetMipLevelFromDimension(nHeight);
    unsigned width = 1u << widthBits;
    unsigned height = 1u << heightBits;
    TexScanTile tiles[TEX_SCAN_TILE_COUNT];
    tiles[0].nTmem = 0;
    tiles[0].nMaskS = widthBits;
    tiles[0].nMaskT = heightBits;
    tiles[0].nLineBytes = nLineBytes;
    tiles[0].nSize = nSize;
    tiles[0].nFormat = nFormat;
    g_bTexScanPrimColorSet = 0;
    g_bTexScanEnvColorSet = 0;
    int format = ChooseTexturePixelFormat(nSize, nFormat, 0);
    int size = static_cast<int>(GetTexturePixelSize(format) * width * height);
    DecodeTexturePixels(g_abDecodedTexturePixels,
                        size,
                        nSize,
                        pImage,
                        pPalette,
                        nFormat,
                        0,
                        0,
                        0,
                        1,
                        tiles,
                        g_dwTextureLoadCategory,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0,
                        0);
    return ConvertRGBAToPixelFormat(pDest, g_abDecodedTexturePixels, size, format);
}

decltype(g_pfnAliasTextureEntrySurface) g_pfnAliasTextureEntrySurface;
decltype(g_pfnDecodeN64TextureToPixelBuffer) g_pfnDecodeN64TextureToPixelBuffer;
decltype(g_pfnCreateTextureFromN64Image) g_pfnCreateTextureFromN64Image;
decltype(g_pfnCreateTextureFromN64ImageAutoLoadSize) g_pfnCreateTextureFromN64ImageAutoLoadSize;
decltype(g_pfnGetTextureEntryPixelsWithPaint) g_pfnGetTextureEntryPixelsWithPaint;
decltype(g_pfnCreateDeviceTextureFromImageHeader) g_pfnCreateDeviceTextureFromImageHeader;
decltype(g_pfnCloneTextureEntryWithNewImageData) g_pfnCloneTextureEntryWithNewImageData;
decltype(g_pfnUploadTrackTextureVariants) g_pfnUploadTrackTextureVariants;
decltype(g_pfnShutdownTextureSubsystem) g_pfnShutdownTextureSubsystem;

void InitializeTextureSubsystem(void) {
    g_pfnAliasTextureEntrySurface = AliasTextureEntrySurface;
    g_pfnDecodeN64TextureToPixelBuffer = DecodeN64TextureToPixelBuffer;
    g_pfnCreateTextureFromN64Image = CreateTextureFromN64Image;
    g_pfnCreateTextureFromN64ImageAutoLoadSize = CreateTextureFromN64ImageAutoLoadSize;
    g_pfnBindTextureEntry = BindTextureEntry;
    g_pfnUploadTextureEntryToSurface = UploadTextureEntryToSurface;
    g_pfnGetTextureEntryPixelsWithPaint = GetTextureEntryPixelsWithPaint;
    g_pfnReloadTextureEntryData = ReloadTextureEntryData;
    g_pfnPreprocessDisplayListTextures = PreprocessDisplayListTextures;
    g_pfnCreateDeviceTextureFromImageHeader = CreateDeviceTextureFromImageHeader;
    g_pfnCloneTextureEntryWithNewImageData = CloneTextureEntryWithNewImageData;
    g_pfnUploadTrackTextureVariants = UploadTrackTextureVariants;
    g_pfnShutdownTextureSubsystem = ShutdownTextureSubsystem;
    g_dwTextureIdCount = 0;
    g_dwTextureIdCapacity = 0;
    std::free(g_ppTextureIdTable);
    g_ppTextureIdTable = nullptr;
    g_dwAvailableTextureMemory = QueryAvailableTextureMemory();
    SelectTextureQualityFromVideoMemory();
    g_nBoundTextureEntry = -1;
    g_dwTextureEntryCount = 0;
    g_dwTextureEntryCapacity = 0;
    std::free(g_pTextureEntries);
    g_pTextureEntries = nullptr;
    g_dwTexScanTileIndex = 0;
    g_dwTexScanMipLevels = 0;
    g_bTexScanRenderModeBlendBit16 = 0;
    CreateWeatherShadeTexture();
    InitializeTextureUploadQueue();
}

unsigned char *GetTextureEntryPixelsWithPaint(int nEntry, unsigned *pSize) {
    TextureImageData &image = g_pTextureEntries[nEntry].data;
    if (!image.bOwnsPaintBuffers) {
        unsigned char *pixels = GetTextureImagePixels(&image);
        *pSize = g_dwTexturePixelByteCount;
        return pixels;
    }
    int right = nEntry;
    void *paint = GetPaintImage(image.nPaintGroup, image.nPaintSet, image.nDamageLevel);
    if (paint) {
        int left;
        int top;
        int bottom;
        GetPaintRectangle(image.nPaintGroup, image.nPaintSet, &left, &right, &top, &bottom);
        int sourceWidth = static_cast<int>(static_cast<unsigned>(image.nSourceWidth) << 1);
        int sourceHeight = static_cast<int>(static_cast<unsigned>(image.nSourceHeight) << 1);
        if (!(left || right || top || bottom)) {
            left = 0;
            right = sourceWidth;
            top = 0;
            bottom = sourceHeight;
        }
        if (BlitPaintImageToPixels(paint,
                                   left,
                                   right,
                                   top,
                                   bottom,
                                   g_abDecodedTexturePixels,
                                   sourceWidth,
                                   sourceHeight)) {
            unsigned char *pixels = g_abDecodedTexturePixels;
            if (sourceWidth != image.nWidth || sourceHeight != image.nHeight) {
                ResizeTextureImage(g_abTextureResampleBuffer,
                                   image.nWidth,
                                   image.nHeight,
                                   pixels,
                                   sourceWidth,
                                   sourceHeight,
                                   image.nPixelFormatId);
                pixels = g_abTextureResampleBuffer;
            }
            g_dwTexturePixelByteCount = image.nDecodedBytes;
            if (image.nColorKey) {
                g_dwTexturePixelByteCount = ConvertRGBAToPixelFormatWithColorKey(
                    pixels, pixels, image.nPixelBytes, image.nPixelFormatId, &image.nColorKey);
            } else {
                g_dwTexturePixelByteCount = ConvertRGBAToPixelFormat(
                    pixels, pixels, image.nPixelBytes, image.nPixelFormatId);
            }
            *pSize = g_dwTexturePixelByteCount;
            return pixels;
        }
    }
    *pSize = g_dwTexturePixelByteCount;
    // The failure path returns the argument slot reused for the right rectangle bound.
    return reinterpret_cast<unsigned char *>(
        static_cast<std::uintptr_t>(static_cast<unsigned>(right)));
}

int PreparePaintTexture(int *pWidth, int *pHeight, TextureImageData *pImage) {
    pImage->bOwnsPaintBuffers = 0;
    if (g_dwTextureLoadCategory != 2) {
        return 0;
    }
    g_nPaintTextureSetIndex = static_cast<int>(static_cast<unsigned>(g_nPaintTextureSetIndex) + 1);
    void *paint = GetPaintImage(g_nPaintTextureGroup, g_nPaintTextureSetIndex, 0);
    if (!paint) {
        return 0;
    }
    int left;
    int right;
    int top;
    int bottom;
    GetPaintRectangle(g_nPaintTextureGroup, g_nPaintTextureSetIndex, &left, &right, &top, &bottom);
    int sourceWidth = static_cast<int>(static_cast<unsigned>(pImage->nSourceWidth) << 1);
    int sourceHeight = static_cast<int>(static_cast<unsigned>(pImage->nSourceHeight) << 1);
    if (!(left || right || top || bottom)) {
        left = 0;
        right = sourceWidth;
        top = 0;
        bottom = sourceHeight;
    }
    bool enlarge = pImage->nSourceWidth == pImage->nWidth &&
                   pImage->nSourceHeight == pImage->nHeight && g_dwTextureQualityLevel <= 0;
    unsigned char *paintPixels = enlarge ? g_abDecodedTexturePixels : g_abTextureResampleBuffer;
    auto blit = [&](const void *pixels) {
        return BlitPaintImageToPixels(
            pixels, left, right, top, bottom, paintPixels, sourceWidth, sourceHeight);
    };
    if (!blit(paint)) {
        return 1;
    }
    if (enlarge) {
        pImage->nWidth = static_cast<int>(static_cast<unsigned>(pImage->nWidth) << 1);
        pImage->nHeight = static_cast<int>(static_cast<unsigned>(pImage->nHeight) << 1);
        pImage->nPixelBytes = static_cast<int>(static_cast<unsigned>(pImage->nPixelBytes) << 2);
        CalculateTextureLodFromDimensions(&pImage->nLod, pImage->nWidth, pImage->nHeight);
        pImage->nLargestLod = pImage->nLod;
        *pWidth = static_cast<int>(static_cast<unsigned>(*pWidth) << 1);
        *pHeight = static_cast<int>(static_cast<unsigned>(*pHeight) << 1);
    }
    pImage->bOwnsPaintBuffers = 1;
    pImage->nPaintGroup = g_nPaintTextureGroup;
    pImage->nPaintSet = g_nPaintTextureSetIndex;
    pImage->nDamageLevel = 0;
    pImage->bHasTintPixels = HasPaintTintPixels(paint, left, right, top, bottom);
    auto convert = [&]() {
        if (!enlarge) {
            ResizeTextureImage(g_abDecodedTexturePixels,
                               pImage->nWidth,
                               pImage->nHeight,
                               g_abTextureResampleBuffer,
                               sourceWidth,
                               sourceHeight,
                               pImage->nPixelFormatId);
        }
        if (pImage->nColorKey) {
            pImage->nPaintPixelBytes =
                ConvertRGBAToPixelFormatWithColorKey(g_abDecodedTexturePixels,
                                                     g_abDecodedTexturePixels,
                                                     pImage->nPixelBytes,
                                                     pImage->nPixelFormatId,
                                                     &pImage->nColorKey);
        } else {
            pImage->nPaintPixelBytes = ConvertRGBAToPixelFormat(g_abDecodedTexturePixels,
                                                                g_abDecodedTexturePixels,
                                                                pImage->nPixelBytes,
                                                                pImage->nPixelFormatId);
        }
    };
    auto save = [&](int damage) {
        pImage->apPaintBuffers[damage] =
            static_cast<unsigned char *>(std::malloc(pImage->nPaintPixelBytes));
        std::memcpy(
            pImage->apPaintBuffers[damage], g_abDecodedTexturePixels, pImage->nPaintPixelBytes);
    };
    convert();
    if (GetSelectedPaintOption() < 0) {
        pImage->apPaintBuffers[3] = nullptr;
        pImage->apPaintBuffers[2] = nullptr;
        pImage->apPaintBuffers[1] = nullptr;
        save(0);
    } else {
        for (int damage = 3; damage >= 0; damage--) {
            paint = GetPaintImage(g_nPaintTextureGroup, g_nPaintTextureSetIndex, damage);
            if (paint) {
                if (blit(paint)) {
                    convert();
                }
                save(damage);
            } else {
                pImage->apPaintBuffers[damage] = nullptr;
            }
        }
    }
    return 1;
}

int CalculateTextureAspectRatio(int *pAspectRatio, int nWidth, int nHeight) {
    bool wide = nWidth > nHeight;
    int ratio = wide ? static_cast<int>(static_cast<unsigned>(nWidth) << 3) / nHeight :
                       static_cast<int>(static_cast<unsigned>(nHeight) << 3) / nWidth;
    int level = ratio >= 64 ? 0 : ratio >= 32 ? 1 : ratio >= 16 ? 2 : 3;
    *pAspectRatio = wide ? level : 6 - level;
    return ratio == 64 || ratio == 32 || ratio == 16 || (!wide && ratio == 8);
}

int g_dwCurrentMaxTextureWidth;
int g_dwCurrentMaxTextureHeight;

unsigned CreateTextureFromN64Image(const unsigned char *pImage,
                                   const unsigned char *pPalette,
                                   int nWidth,
                                   int nHeight,
                                   int nLineBytes,
                                   int nSize,
                                   int nFormat,
                                   int nOption7,
                                   int nOption8,
                                   int nOption9,
                                   int nOption10,
                                   int nOption11,
                                   int nOption12,
                                   int nOption13,
                                   int nOption14) {
    int widthBits = GetMipLevelFromDimension(nWidth);
    int heightBits = GetMipLevelFromDimension(nHeight);
    int paddedWidth = 1 << widthBits;
    int paddedHeight = 1 << heightBits;
    TextureImageData image;
    std::memset(image.abUnknown0, 0, 4);
    image.abUnknown0[4] = 3;
    std::memset(image.abUnknown14, 0, sizeof(image.abUnknown14));
    image.nSourceWidth = image.nWidth = nWidth;
    image.nSourceHeight = image.nHeight = nHeight;
    image.nDecodedWidth = paddedWidth;
    image.nDecodedHeight = paddedHeight;
    CalculateTextureLodFromDimensions(&image.nLargestLod, paddedWidth, paddedHeight);
    image.nLod = image.nLargestLod;
    image.nTileIndex = 0;
    image.nTileLimit = 1;
    TexScanTile &tile = image.aTiles[0];
    tile.nTmem = 0;
    tile.nMaskS = widthBits;
    tile.nMaskT = heightBits;
    tile.nLineBytes = nLineBytes;
    tile.nSize = nSize;
    tile.nFormat = nFormat;
    tile.nShiftS = nOption11;
    tile.nShiftT = nOption12;
    tile.nUls = 2;
    tile.nUlt = 2;
    tile.nLrs = static_cast<int>((static_cast<unsigned>(nWidth) << 2) - 2);
    tile.nLrt = static_cast<int>((static_cast<unsigned>(nHeight) << 2) - 2);
    image.nRowSwapMask = nOption14;
    image.bColorCombine = 0;
    image.bOwnsPaintBuffers = 0;
    std::memset(image.abEnvColor, 0, sizeof(image.abEnvColor));
    std::memset(image.abPrimColor, 0, sizeof(image.abPrimColor));
    g_bTexScanPrimColorSet = 0;
    g_bTexScanEnvColorSet = 0;
    image.nPixelFormatId = ChooseTexturePixelFormat(nSize, nFormat, 0);
    image.nPixelBytes = GetTexturePixelSize(image.nPixelFormatId) * paddedWidth * paddedHeight;
    image.bClampS = nOption9 != 0;
    image.bClampT = nOption10 != 0;
    image.pImage = const_cast<unsigned char *>(pImage);
    image.pPalette = pPalette;
    image.bMirrorS = nOption7;
    image.bMirrorT = nOption8;
    if ((g_dwTextureLoadCategory & 2) && !g_dwNullStubEnable) {
        g_dwCurrentMaxTextureWidth = g_dwMaxTexWidthHighQuality;
        g_dwCurrentMaxTextureHeight = g_dwMaxTexHeightHighQuality;
    } else {
        g_dwCurrentMaxTextureWidth = g_dwMaxTexWidthLowQuality;
        g_dwCurrentMaxTextureHeight = g_dwMaxTexHeightLowQuality;
    }
    auto requiresSquare = []() {
        return g_dwD3DAlwaysSquareTextures || ((g_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps |
                                                g_D3DHelDeviceDesc.dpcTriCaps.dwTextureCaps) &
                                               D3DPTEXTURECAPS_SQUAREONLY);
    };
    bool exceedsDevice = (g_D3DHalDeviceDesc.dwMaxTextureWidth &&
                          static_cast<unsigned>(nWidth) > g_D3DHalDeviceDesc.dwMaxTextureWidth) ||
                         (g_D3DHalDeviceDesc.dwMaxTextureHeight &&
                          static_cast<unsigned>(nHeight) > g_D3DHalDeviceDesc.dwMaxTextureHeight);
    bool exceedsQuality =
        nOption13 && (nWidth > g_dwCurrentMaxTextureWidth || nHeight > g_dwCurrentMaxTextureHeight);
    if (!nOption7 && !nOption8 && !requiresSquare() && !exceedsDevice && !exceedsQuality) {
        CalculateTextureAspectRatio(&image.nAspectRatio, paddedWidth, paddedHeight);
    } else {
        int width = nWidth;
        int height = nHeight;
        auto update = [&]() {
            CalculateTextureLodFromDimensions(&image.nLod, width, height);
            CalculateTextureAspectRatio(&image.nAspectRatio, width, height);
            image.nLargestLod = static_cast<int>(static_cast<unsigned>(image.nLod) +
                                                 static_cast<unsigned>(g_dwTexScanMaxTileIndex));
        };
        if (nOption7) {
            width /= 2;
            update();
        }
        if (nOption8) {
            height /= 2;
            update();
        }
        if (g_D3DHalDeviceDesc.dwMaxTextureWidth &&
            static_cast<unsigned>(width) > g_D3DHalDeviceDesc.dwMaxTextureWidth) {
            width = static_cast<int>(g_D3DHalDeviceDesc.dwMaxTextureWidth);
            update();
        }
        if (g_D3DHalDeviceDesc.dwMaxTextureHeight &&
            static_cast<unsigned>(height) > g_D3DHalDeviceDesc.dwMaxTextureHeight) {
            height = static_cast<int>(g_D3DHalDeviceDesc.dwMaxTextureHeight);
            update();
        }
        if (nOption13 && width > g_dwCurrentMaxTextureWidth) {
            width = g_dwCurrentMaxTextureWidth;
            update();
        }
        if (nOption13 && height > g_dwCurrentMaxTextureHeight) {
            height = g_dwCurrentMaxTextureHeight;
            update();
        }
        if (requiresSquare() && width != height) {
            if (width >= height) {
                width = nOption13 && width >= g_dwCurrentMaxTextureWidth ?
                            g_dwCurrentMaxTextureWidth :
                            1 << GetMipLevelFromDimension(width);
                height = width;
            } else {
                height = nOption13 && height >= g_dwCurrentMaxTextureHeight ?
                             g_dwCurrentMaxTextureHeight :
                             1 << GetMipLevelFromDimension(height);
                width = height;
            }
            update();
        }
        image.nWidth = width;
        image.nHeight = height;
        CalculateTextureAspectRatio(&image.nAspectRatio, width, height);
    }
    image.nColorKey = 0;
    unsigned options[] = {1, 1, 0xc0000000u, 0};
    std::memcpy(image.abUnknown2c, options, sizeof(options));
    image.dwFlags = g_dwTextureLoadCategory;
    image.nDecodedBytes = image.nPixelBytes;
    unsigned char *pixels = GetTextureImagePixels(&image);
    return CreateTextureFromImage(&image, pixels);
}
