#include "platform/win/glide/texturecache.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "platform/win/d3d/renderer.h"
#include "platform/win/displaylist.h"
#include "platform/win/frontend.h"
#include "platform/win/glide/renderer.h"
#include "platform/win/glide/texture.h"
#include "platform/win/paintshop.h"
#include "platform/win/render.h"
#include "platform/win/settings.h"
#include "platform/win/texturesampling.h"
#include "platform/win/texturescan.h"
#include "platform/win/textureworkspace.h"

GlideTmuMemory g_aGlideTmuMemory[GLIDE_TMU_COUNT];
GlideTextureSlot g_aGlideTextureSlots[GLIDE_TEXTURE_SLOT_COUNT];
unsigned g_dwGlideTextureSlotCount;
int g_nBoundTextureTmu;

// @ghidraAddress BRGlide: 0x100a9e5c
static const char g_szOutOfTexMem[] = "Out of TexMem";

int ConvertRgba32ToArgb1555(unsigned char *pDest, const unsigned char *pSource, int nBytes) {
    unsigned char *output = pDest;
    if (nBytes > 0) {
        for (unsigned count = static_cast<unsigned>(nBytes + 3) >> 2; count; count--) {
            unsigned red = pSource[0] >> 3;
            unsigned green = pSource[1] >> 3;
            unsigned blue = pSource[2];
            unsigned alpha = pSource[3] >> 7;
            pSource += 4;
            auto pixel =
                static_cast<unsigned short>((((alpha << 5 | red) << 5 | green) << 5) | blue >> 3);
            std::memcpy(output, &pixel, sizeof(pixel));
            output += sizeof(pixel);
        }
    }
    return static_cast<int>(output - pDest);
}

void ResetGlideTmuTextureAllocator(void) {
    for (GlideTextureSlot &slot : g_aGlideTextureSlots) {
        slot.bInUse = 0;
    }
    g_dwGlideTextureSlotCount = 0;
    for (int tmu = 0; tmu < GLIDE_TMU_COUNT; tmu++) {
        g_aGlideTmuMemory[tmu].dwNextAddress = grTexMinAddress(tmu);
        g_aGlideTmuMemory[tmu].dwUpperAddress = GLIDE_TMU_UPPER_REGION_ADDRESS;
    }
}

int AllocateGlideTmuTexture(int nTmu,
                            unsigned char nEvenOdd,
                            int nWidth,
                            int nHeight,
                            int nFormat,
                            int nMipMapMode,
                            int nSmallLod,
                            int nLargeLod,
                            int nAspectRatio,
                            int nClampS,
                            int nClampT,
                            int nMinifyFilter,
                            int nMagnifyFilter,
                            float fLodBias,
                            int nLodBlend) {
    if (g_dwGlideTextureSlotCount >= GLIDE_TEXTURE_SLOT_COUNT) {
        return -1;
    }
    unsigned nSlot = g_dwGlideTextureSlotCount;
    GlideTextureSlot &slot = g_aGlideTextureSlots[nSlot];
    slot.info.smallLod = static_cast<GrLOD_t>(nSmallLod);
    slot.info.largeLod = static_cast<GrLOD_t>(nLargeLod);
    slot.info.aspectRatio = static_cast<GrAspectRatio_t>(nAspectRatio);
    slot.info.format = static_cast<GrTextureFormat_t>(nFormat);
    slot.info.data = nullptr;
    FxU32 size = grTexTextureMemRequired(nEvenOdd, &slot.info);
    GlideTmuMemory &memory = g_aGlideTmuMemory[nTmu];
    FxU32 start = memory.dwNextAddress;
    FxU32 end = size + start;
    if (end < grTexMaxAddress(nTmu) && end < GLIDE_TMU_UPPER_REGION_ADDRESS) {
        memory.dwNextAddress = end;
    } else {
        start = memory.dwUpperAddress;
        end = size + start;
        if (end >= grTexMaxAddress(nTmu)) {
            return -1;
        }
        memory.dwUpperAddress = end;
    }
    slot.nUnknown0 = 0;
    slot.bInUse = 1;
    slot.nWidth = nWidth;
    slot.nHeight = nHeight;
    slot.nAspectRatio = nAspectRatio;
    slot.pData = nullptr;
    slot.nFormat = nFormat;
    slot.nMipMapMode = nMipMapMode;
    slot.nMagnifyFilter = nMagnifyFilter;
    slot.nMinifyFilter = nMinifyFilter;
    slot.nClampS = nClampS;
    slot.nClampT = nClampT;
    slot.nUnknown30 = 0;
    slot.nUnknown34 = 0;
    slot.dwLodBias = static_cast<unsigned>(fLodBias * 4.0f);
    slot.nSmallLod = nSmallLod;
    slot.nLargeLod = nLargeLod;
    slot.nTmu = nTmu;
    slot.nEvenOdd = nEvenOdd;
    slot.dwStartAddress = start;
    slot.nLodBlend = nLodBlend;
    g_dwGlideTextureSlotCount++;
    return static_cast<int>(nSlot);
}

void DownloadGlideTextureMipMap(unsigned nSlot, const void *pData, int nUnused) {
    if (nSlot >= g_dwGlideTextureSlotCount || !g_aGlideTextureSlots[nSlot].bInUse) {
        return;
    }
    GlideTextureSlot &slot = g_aGlideTextureSlots[nSlot];
    // Glide takes the texels through a pointer it does not write to.
    slot.info.data = const_cast<void *>(pData);
    slot.pData = pData;
    grTexDownloadMipMap(slot.nTmu, slot.dwStartAddress, slot.nEvenOdd, &slot.info);
}

void BindGlideTextureSource(unsigned nSlot) {
    if (nSlot >= g_dwGlideTextureSlotCount || !g_aGlideTextureSlots[nSlot].bInUse) {
        return;
    }
    GlideTextureSlot &slot = g_aGlideTextureSlots[nSlot];
    grTexSource(slot.nTmu, slot.dwStartAddress, slot.nEvenOdd, &slot.info);
    grTexClampMode(slot.nTmu, slot.nClampS, slot.nClampT);
    grTexFilterMode(slot.nTmu, slot.nMinifyFilter, slot.nMagnifyFilter);
    grTexLodBiasValue(slot.nTmu, static_cast<float>(slot.dwLodBias) * 0.25f);
    grTexMipMapMode(slot.nTmu, slot.nMipMapMode, slot.nLodBlend);
}

void BindTextureEntry(int nEntry) {
    if (nEntry == g_nBoundTextureEntry) {
        return;
    }
    BindGlideTextureSource(g_pTextureEntries[nEntry].nTextureId);
    int tmu = g_pTextureEntries[nEntry].data.nOption1;
    if (g_nBoundTextureTmu != tmu) {
        if (tmu == GR_TMU1) {
            grTexCombine(GR_TMU1,
                         GR_COMBINE_FUNCTION_LOCAL,
                         GR_COMBINE_FACTOR_NONE,
                         GR_COMBINE_FUNCTION_LOCAL,
                         GR_COMBINE_FACTOR_NONE,
                         FXFALSE,
                         FXFALSE);
            grTexCombine(GR_TMU0,
                         GR_COMBINE_FUNCTION_SCALE_OTHER,
                         GR_COMBINE_FACTOR_ONE,
                         GR_COMBINE_FUNCTION_SCALE_OTHER,
                         GR_COMBINE_FACTOR_ONE,
                         FXFALSE,
                         FXFALSE);
        } else {
            grTexCombine(GR_TMU0,
                         GR_COMBINE_FUNCTION_LOCAL,
                         GR_COMBINE_FACTOR_NONE,
                         GR_COMBINE_FUNCTION_LOCAL,
                         GR_COMBINE_FACTOR_NONE,
                         FXFALSE,
                         FXFALSE);
        }
        g_nBoundTextureTmu = g_pTextureEntries[nEntry].data.nOption1;
    }
    const TextureEntry &entry = g_pTextureEntries[nEntry];
    g_fTexCoordScaleS = entry.fTextureScaleS;
    g_fTexCoordScaleT = entry.fTextureScaleT;
    const TexScanTile &tile = entry.data.aTiles[entry.data.nTileIndex];
    g_dwTileSizeUls = tile.nUls;
    g_dwTileSizeUlt = tile.nUlt;
    g_dwTileSizeLrs = tile.nLrs;
    g_dwTileSizeLrt = tile.nLrt;
    g_nBoundTextureEntry = nEntry;
}

void AppendUniqueGlideErrorMessage(const char *pszMessage) {
    if (!std::strstr(g_szDebugOverlayText, pszMessage)) {
        std::strcat(g_szDebugOverlayText, pszMessage);
        std::strcat(g_szDebugOverlayText, g_szSpace);
    }
}

int CreateGlideTextureFromRawImage(unsigned char *pPixels, int nWidth, int nHeight, int nFormat) {
    int widthBits = GetMipLevelFromDimension(nWidth);
    int heightBits = GetMipLevelFromDimension(nHeight);
    int paddedWidth = 1 << widthBits;
    int paddedHeight = 1 << heightBits;
    TextureImageData image;
    image.nOption1 = g_nGlideNumTmus > 1;
    image.nOption2 = GR_MIPMAPLEVELMASK_BOTH;
    image.nSourceWidth = image.nWidth = nWidth;
    image.nSourceHeight = image.nHeight = nHeight;
    image.nDecodedWidth = paddedWidth;
    image.nDecodedHeight = paddedHeight;
    image.nOption3 = GR_MIPMAP_DISABLE;
    CalculateMipMapLevel(&image.nLargestLod, paddedWidth, paddedHeight);
    image.nLod = image.nLargestLod;
    image.nTileIndex = 0;
    TexScanTile &tile = image.aTiles[0];
    tile.nTmem = 0;
    tile.nMaskS = GetMipLevelFromDimension(nWidth);
    tile.nMaskT = GetMipLevelFromDimension(nHeight);
    tile.nLineBytes = nWidth;
    tile.nSize = G_IM_SIZ_8b;
    tile.nFormat = G_IM_FMT_CI;
    tile.nShiftS = 0;
    tile.nShiftT = 0;
    tile.nUls = 2;
    tile.nUlt = 2;
    tile.nLrs = static_cast<int>((static_cast<unsigned>(nWidth) << 2) - 2);
    tile.nLrt = static_cast<int>((static_cast<unsigned>(nHeight) << 2) - 2);
    image.nTileLimit = 1;
    image.nPixelFormatId = nFormat;
    image.nPixelBytes = GetGlideTexFormatBytesPerPixel(nFormat) * paddedHeight * paddedWidth;
    image.pImage = pPixels;
    image.pPalette = nullptr;
    image.bMirrorS = 0;
    image.bMirrorT = 0;
    image.bClampS = 1;
    image.bClampT = 1;
    CalculateAspectRatioLevel(&image.nAspectRatio, paddedWidth, paddedHeight);
    image.nOption4 = GR_TEXTUREFILTER_BILINEAR;
    image.nOption5 = GR_TEXTUREFILTER_BILINEAR;
    image.dwOption6 = 0xc0000000;
    image.nOption7 = 0;
    // Flag 0x10 skips decoding, and the decode buffer is uploaded in place of pPixels.
    image.dwFlags = 0x10;
    image.bColorCombine = 0;
    image.bOwnsPaintBuffers = 0;
    unsigned char *pixels = GetTextureImagePixels(&image);
    return CreateTextureFromImage(&image, pixels);
}

void ResizeTextureImage(unsigned char *pDest,
                        int width,
                        int height,
                        const unsigned char *pSource,
                        int sourceWidth,
                        int sourceHeight,
                        int nPixelFormatId) {
    int pixelSize = GetGlideTexFormatBytesPerPixel(nPixelFormatId);
    decltype(&BilinearInterpolateArgb1555) sample;
    switch (nPixelFormatId) {
    case GR_TEXFMT_ALPHA_8:
    case GR_TEXFMT_INTENSITY_8:
        sample = BilinearInterpolate8bpp;
        break;
    case GR_TEXFMT_ARGB_4444:
        sample = BilinearInterpolateArgb4444;
        break;
    default:
        sample = BilinearInterpolateArgb1555;
        break;
    }
    long double verticalRatio = static_cast<long double>(sourceHeight) / height;
    float yStep = verticalRatio;
    float xStep = static_cast<long double>(sourceWidth) / width;
    float y = verticalRatio * 0.5L - 0.5L;
    if (height <= 0) {
        return;
    }
    float xStart = static_cast<long double>(xStep) * 0.5L - 0.5L;
    int maxY = sourceHeight - 1;
    for (int row = height; row; row--) {
        int iy = static_cast<int>(std::floor(static_cast<double>(y)));
        float fractionY = static_cast<long double>(y) - iy;
        int y0 = iy > 0 ? iy : 0;
        int y1 = iy + 1;
        if (y1 >= maxY) {
            y1 = maxY;
        }
        float x = xStart;
        int maxX = sourceWidth - 1;
        for (int column = width; column > 0; column--) {
            int ix = static_cast<int>(std::floor(static_cast<double>(x)));
            float fractionX = static_cast<long double>(x) - ix;
            int x0 = ix > 0 ? ix : 0;
            int x1 = ix + 1;
            if (x1 >= maxX) {
                x1 = maxX;
            }
            sample(pDest,
                   pSource + (x0 + y0 * sourceWidth) * pixelSize,
                   pSource + (x1 + y0 * sourceWidth) * pixelSize,
                   pSource + (x0 + y1 * sourceWidth) * pixelSize,
                   pSource + (x1 + y1 * sourceWidth) * pixelSize,
                   fractionX,
                   fractionY);
            pDest += pixelSize;
            x = static_cast<long double>(x) + xStep;
        }
        y = static_cast<long double>(y) + yStep;
    }
}

unsigned ResizeTextureMipChain(unsigned char *pDest,
                               const unsigned char *pSource,
                               TextureImageData *pImage) {
    unsigned bytes = 0;
    int pixelSize = GetGlideTexFormatBytesPerPixel(pImage->nPixelFormatId);
    int sourceWidth = pImage->nSourceWidth;
    int sourceHeight = pImage->nSourceHeight;
    int width = pImage->nWidth;
    int height = pImage->nHeight;
    if (sourceWidth >= sourceHeight) {
        pImage->nDecodedHeight = width * sourceHeight / height;
    } else {
        pImage->nDecodedWidth = height * sourceWidth / width;
    }
    for (int level = g_dwTexScanRenderTileIndex; level < pImage->nTileLimit; level++) {
        ResizeTextureImage(
            pDest, width, height, pSource, sourceWidth, sourceHeight, pImage->nPixelFormatId);
        int outputBytes = height * width * pixelSize;
        bytes += outputBytes;
        pDest += outputBytes;
        pSource += sourceHeight * sourceWidth * pixelSize;
        width >>= 1;
        height >>= 1;
        sourceWidth >>= 1;
        sourceHeight >>= 1;
    }
    return bytes;
}

void ModulateTextureWithIntensityPlane(const TextureImageData *pImage, unsigned char *pPixels) {
    int sourceWidth = pImage->nSourceWidth;
    int sourceHeight = pImage->nSourceHeight;
    ResizeTextureImage(g_abTextureResampleBuffer,
                       sourceWidth,
                       sourceHeight,
                       pPixels + sourceWidth * sourceHeight * 2,
                       sourceWidth / 2,
                       sourceHeight / 2,
                       GR_TEXFMT_ARGB_1555);
    const unsigned char *intensity = g_abTextureResampleBuffer;
    for (int row = 0; row < pImage->nSourceHeight; row++) {
        for (int column = 0; column < pImage->nSourceWidth; column++) {
            unsigned short color;
            unsigned short multiplier;
            std::memcpy(&color, pPixels, sizeof(color));
            std::memcpy(&multiplier, intensity, sizeof(multiplier));
            unsigned red = (color >> 10 & 0x1f) * multiplier / 15;
            unsigned green = (color >> 5 & 0x1f) * multiplier / 15;
            unsigned blue = (color & 0x1f) * multiplier / 15;
            auto result = static_cast<unsigned short>(
                (((static_cast<unsigned>(color) >> 15 << 5 | red) << 5 | green) << 5) | blue);
            std::memcpy(pPixels, &result, sizeof(result));
            pPixels += sizeof(result);
            intensity += sizeof(result);
        }
    }
}

unsigned char *GetTextureImagePixels(TextureImageData *pImage) {
    if (pImage->dwFlags & 0x40) {
        g_dwTexturePixelByteCount = pImage->nDecodedBytes;
        return pImage->pImage;
    }
    unsigned char *pixels = GetTextureDecodeBuffer();
    if (pImage->bOwnsPaintBuffers) {
        if (pImage->apPaintBuffers[pImage->nDamageLevel]) {
            g_dwTexturePixelByteCount = pImage->nPaintPixelBytes;
            return pImage->apPaintBuffers[pImage->nDamageLevel];
        }
        g_dwTexturePixelByteCount = pImage->nPaintPixelBytes;
        return pImage->apPaintBuffers[0];
    }
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
    if ((pImage->dwFlags & 2) && (pImage->dwFlags & 0x80)) {
        pImage->nLargestLod = pImage->nLod;
        ModulateTextureWithIntensityPlane(pImage, pixels);
    }
    if (pImage->nSourceWidth != pImage->nWidth || pImage->nSourceHeight != pImage->nHeight) {
        pImage->nPixelBytes = ResizeTextureMipChain(g_abTextureResampleBuffer, pixels, pImage);
        g_dwTexturePixelByteCount = pImage->nPixelBytes;
        return g_abTextureResampleBuffer;
    }
    return pixels;
}

int PreparePaintTexture(int *pWidth, int *pHeight, TextureImageData *pImage) {
    pImage->bOwnsPaintBuffers = 0;
    if (g_dwTextureLoadCategory != 2) {
        return 0;
    }
    g_nPaintTextureSetIndex++;
    void *paint = GetPaintImage(g_nPaintTextureGroup, g_nPaintTextureSetIndex, 0);
    if (!paint) {
        return 0;
    }
    int left;
    int right;
    int top;
    int bottom;
    GetPaintRectangle(g_nPaintTextureGroup, g_nPaintTextureSetIndex, &left, &right, &top, &bottom);
    if (!(left || right || top || bottom)) {
        left = 0;
        top = 0;
        right = pImage->nSourceWidth << 1;
        bottom = pImage->nSourceHeight << 1;
    }
    int sourceWidth = pImage->nSourceWidth << 1;
    int sourceHeight = pImage->nSourceHeight << 1;
    unsigned char *decoded = GetTextureDecodeBuffer();
    auto save = [&](int damage) {
        pImage->apPaintBuffers[damage] =
            static_cast<unsigned char *>(std::malloc(pImage->nPaintPixelBytes));
        std::memcpy(pImage->apPaintBuffers[damage], decoded, pImage->nPaintPixelBytes);
    };
    auto saveAll = [&](auto blitAndConvert) {
        if (GetSelectedPaintOption() < 0) {
            for (int damage = 3; damage > 0; damage--) {
                pImage->apPaintBuffers[damage] = nullptr;
            }
            save(0);
            return;
        }
        for (int damage = 3; damage >= 0; damage--) {
            paint = GetPaintImage(g_nPaintTextureGroup, g_nPaintTextureSetIndex, damage);
            if (paint) {
                blitAndConvert(paint);
                save(damage);
            } else {
                pImage->apPaintBuffers[damage] = nullptr;
            }
        }
    };
    if (pImage->nSourceWidth == pImage->nWidth && pImage->nSourceHeight == pImage->nHeight &&
        g_dwTextureQualityLevel <= 0) {
        if (!BlitPaintImageToPixels(
                paint, left, right, top, bottom, decoded, sourceWidth, sourceHeight)) {
            return 1;
        }
        pImage->nWidth <<= 1;
        pImage->nHeight <<= 1;
        pImage->nPixelBytes <<= 2;
        CalculateMipMapLevel(&pImage->nLod, pImage->nWidth, pImage->nHeight);
        pImage->nLargestLod = pImage->nLod;
        *pWidth <<= 1;
        *pHeight <<= 1;
        pImage->bOwnsPaintBuffers = 1;
        pImage->nPaintGroup = g_nPaintTextureGroup;
        pImage->nPaintSet = g_nPaintTextureSetIndex;
        pImage->nDamageLevel = 0;
        pImage->bHasTintPixels = HasPaintTintPixels(paint, left, right, top, bottom);
        auto convert = [&]() {
            ConvertRgba32ToArgb1555(
                decoded, decoded, pImage->nSourceHeight * pImage->nSourceWidth * 16);
            pImage->nPaintPixelBytes = pImage->nSourceHeight * pImage->nSourceWidth * 8;
        };
        convert();
        saveAll([&](void *image) {
            if (BlitPaintImageToPixels(
                    image, left, right, top, bottom, decoded, sourceWidth, sourceHeight)) {
                convert();
            }
        });
        return 1;
    }
    if (!BlitPaintImageToPixels(paint,
                                left,
                                right,
                                top,
                                bottom,
                                g_abTextureResampleBuffer,
                                sourceWidth,
                                sourceHeight)) {
        return 1;
    }
    pImage->bOwnsPaintBuffers = 1;
    pImage->nPaintGroup = g_nPaintTextureGroup;
    pImage->nPaintSet = g_nPaintTextureSetIndex;
    pImage->nDamageLevel = 0;
    pImage->bHasTintPixels = HasPaintTintPixels(paint, left, right, top, bottom);
    auto convert = [&]() {
        ConvertRgba32ToArgb1555(g_abTextureResampleBuffer,
                                g_abTextureResampleBuffer,
                                pImage->nSourceHeight * pImage->nSourceWidth * 16);
        pImage->nPaintPixelBytes = pImage->nHeight * pImage->nWidth * 2;
        ResizeTextureImage(decoded,
                           pImage->nWidth,
                           pImage->nHeight,
                           g_abTextureResampleBuffer,
                           pImage->nSourceWidth << 1,
                           pImage->nSourceHeight << 1,
                           pImage->nPixelFormatId);
    };
    convert();
    saveAll([&](void *image) {
        if (BlitPaintImageToPixels(image,
                                   left,
                                   right,
                                   top,
                                   bottom,
                                   g_abTextureResampleBuffer,
                                   sourceWidth,
                                   sourceHeight)) {
            convert();
        }
    });
    return 1;
}

int CreateTextureFromImage(TextureImageData *pImage, unsigned char *pPixels) {
    if (!g_bForceTextureClone) {
        int entry = FindMatchingTextureEntry(pImage);
        if (entry != -1) {
            return entry;
        }
    }
    // The size is never computed before PreparePaintTexture() doubles it, and is not read after.
    int width;
    int height;
    unsigned char *pixels =
        PreparePaintTexture(&width, &height, pImage) ? GetTextureDecodeBuffer() : pPixels;
    auto allocate = [pImage](int tmu) {
        return AllocateGlideTmuTexture(tmu,
                                       pImage->nOption2,
                                       pImage->nWidth,
                                       pImage->nHeight,
                                       pImage->nPixelFormatId,
                                       pImage->nOption3,
                                       pImage->nLargestLod,
                                       pImage->nLod,
                                       pImage->nAspectRatio,
                                       pImage->bClampS,
                                       pImage->bClampT,
                                       pImage->nOption4,
                                       pImage->nOption5,
                                       std::bit_cast<float>(pImage->dwOption6),
                                       pImage->nOption7);
    };
    int slot = allocate(pImage->nOption1);
    while (slot == -1) {
        if (pImage->nOption1 != GR_TMU1) {
            unsigned short gray[16];
            for (unsigned short &texel : gray) {
                texel = 0x800f;
            }
            DownloadGlideTextureMipMap(g_dwWeatherShadeTextureId, gray, 0);
            AppendUniqueGlideErrorMessage(g_szOutOfTexMem);
            return static_cast<int>(g_dwWeatherShadeTextureId);
        }
        pImage->nOption1 = GR_TMU0;
        slot = allocate(GR_TMU0);
    }
    DownloadGlideTextureMipMap(slot, pixels, 0);
    return static_cast<int>(RegisterTextureEntry(pImage, slot));
}

unsigned char *GetTextureEntryPixelsWithPaint(int nEntry, unsigned *pSize) {
    TextureImageData &image = g_pTextureEntries[nEntry].data;
    if (!image.bOwnsPaintBuffers) {
        unsigned char *pixels = GetTextureImagePixels(&image);
        *pSize = g_dwTexturePixelByteCount;
        return pixels;
    }
    unsigned char *pixels = GetTextureDecodeBuffer();
    int right = nEntry;
    void *paint = GetPaintImage(image.nPaintGroup, image.nPaintSet, image.nDamageLevel);
    if (paint) {
        int left;
        int top;
        int bottom;
        GetPaintRectangle(image.nPaintGroup, image.nPaintSet, &left, &right, &top, &bottom);
        if (!(left || right || top || bottom)) {
            left = 0;
            right = image.nSourceWidth << 1;
            top = 0;
            bottom = image.nSourceHeight << 1;
        }
        if (BlitPaintImageToPixels(paint,
                                   left,
                                   right,
                                   top,
                                   bottom,
                                   pixels,
                                   image.nSourceWidth << 1,
                                   image.nSourceHeight << 1)) {
            ConvertRgba32ToArgb1555(pixels, pixels, image.nSourceHeight * image.nSourceWidth * 16);
            if (image.nSourceWidth << 1 != image.nWidth ||
                image.nSourceHeight << 1 != image.nHeight) {
                ResizeTextureImage(g_abTextureResampleBuffer,
                                   image.nWidth,
                                   image.nHeight,
                                   pixels,
                                   image.nSourceWidth << 1,
                                   image.nSourceHeight << 1,
                                   image.nPixelFormatId);
                pixels = g_abTextureResampleBuffer;
            }
            g_dwTexturePixelByteCount = image.nDecodedBytes;
            *pSize = g_dwTexturePixelByteCount;
            return pixels;
        }
    }
    *pSize = g_dwTexturePixelByteCount;
    // The failure path returns the argument slot reused for the right rectangle bound.
    return reinterpret_cast<unsigned char *>(
        static_cast<std::uintptr_t>(static_cast<unsigned>(right)));
}

namespace {

// Fill the fields of an image of one tile that CreateTextureFromN64Image() and
// DecodeN64TextureToPixelBuffer() both set.
void InitializeN64Image(TextureImageData &image,
                        const unsigned char *pImage,
                        const unsigned char *pPalette,
                        int nWidth,
                        int nHeight,
                        int nLineBytes,
                        int nSize,
                        int nFormat,
                        int paddedWidth,
                        int paddedHeight) {
    image.nOption1 = g_nGlideNumTmus > 1;
    image.nOption2 = GR_MIPMAPLEVELMASK_BOTH;
    image.nWidth = nWidth;
    image.nHeight = nHeight;
    image.nOption3 = GR_MIPMAP_DISABLE;
    CalculateMipMapLevel(&image.nLargestLod, paddedWidth, paddedHeight);
    image.nLod = image.nLargestLod;
    image.nTileIndex = 0;
    TexScanTile &tile = image.aTiles[0];
    tile.nTmem = 0;
    tile.nMaskS = GetMipLevelFromDimension(nWidth);
    tile.nMaskT = GetMipLevelFromDimension(nHeight);
    tile.nLineBytes = nLineBytes;
    tile.nSize = nSize;
    tile.nFormat = nFormat;
    image.nTileLimit = 1;
    image.bColorCombine = 0;
    image.bOwnsPaintBuffers = 0;
    std::memset(image.abEnvColor, 0, sizeof(image.abEnvColor));
    std::memset(image.abPrimColor, 0, sizeof(image.abPrimColor));
    g_bTexScanPrimColorSet = 0;
    g_bTexScanEnvColorSet = 0;
    image.nPixelFormatId = ChooseTexturePixelFormat(tile.nSize, tile.nFormat, 0);
    image.nPixelBytes =
        GetGlideTexFormatBytesPerPixel(image.nPixelFormatId) * paddedHeight * paddedWidth;
    image.pImage = const_cast<unsigned char *>(pImage);
    image.pPalette = pPalette;
}

} // namespace

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
    int paddedWidth = 1 << GetMipLevelFromDimension(nWidth);
    int paddedHeight = 1 << GetMipLevelFromDimension(nHeight);
    TextureImageData image;
    image.nSourceWidth = nWidth;
    image.nSourceHeight = nHeight;
    image.nDecodedWidth = paddedWidth;
    image.nDecodedHeight = paddedHeight;
    InitializeN64Image(image,
                       pImage,
                       pPalette,
                       nWidth,
                       nHeight,
                       nLineBytes,
                       nSize,
                       nFormat,
                       paddedWidth,
                       paddedHeight);
    TexScanTile &tile = image.aTiles[0];
    tile.nShiftS = nOption11;
    tile.nShiftT = nOption12;
    tile.nUls = 2;
    tile.nUlt = 2;
    tile.nLrs = static_cast<int>((static_cast<unsigned>(nWidth) << 2) - 2);
    tile.nLrt = static_cast<int>((static_cast<unsigned>(nHeight) << 2) - 2);
    image.nRowSwapMask = nOption14;
    image.bClampS = nOption9 != 0;
    image.bClampT = nOption10 != 0;
    image.bMirrorS = nOption7;
    image.bMirrorT = nOption8;
    CalculateAspectRatioLevel(&image.nAspectRatio, paddedWidth, paddedHeight);
    image.nOption4 = GR_TEXTUREFILTER_BILINEAR;
    image.nOption5 = GR_TEXTUREFILTER_BILINEAR;
    image.dwOption6 = 0xc0000000;
    image.nOption7 = 0;
    image.dwFlags = g_dwTextureLoadCategory;
    image.nDecodedBytes = image.nPixelBytes;
    unsigned char *pixels = GetTextureImagePixels(&image);
    return static_cast<unsigned>(CreateTextureFromImage(&image, pixels));
}

int DecodeN64TextureToPixelBuffer(void *pDest,
                                  const unsigned char *pImage,
                                  const unsigned char *pPalette,
                                  int nWidth,
                                  int nHeight,
                                  int nLineBytes,
                                  int nSize,
                                  int nFormat) {
    int paddedWidth = 1 << GetMipLevelFromDimension(nWidth);
    int paddedHeight = 1 << GetMipLevelFromDimension(nHeight);
    TextureImageData image;
    image.nDecodedWidth = nWidth;
    image.nDecodedHeight = nHeight;
    InitializeN64Image(image,
                       pImage,
                       pPalette,
                       nWidth,
                       nHeight,
                       nLineBytes,
                       nSize,
                       nFormat,
                       paddedWidth,
                       paddedHeight);
    image.nRowSwapMask = 0;
    image.bMirrorS = 0;
    image.bMirrorT = 0;
    image.dwFlags = g_dwTextureLoadCategory;
    const TexScanTile &tile = image.aTiles[image.nTileIndex];
    DecodeTexturePixels(static_cast<unsigned char *>(pDest),
                        image.nPixelBytes,
                        tile.nSize,
                        image.pImage,
                        image.pPalette,
                        tile.nFormat,
                        image.bMirrorS,
                        image.bMirrorT,
                        image.nTileIndex,
                        image.nTileLimit,
                        image.aTiles,
                        image.dwFlags,
                        image.bColorCombine,
                        image.abEnvColor[0],
                        image.abEnvColor[1],
                        image.abEnvColor[2],
                        image.abEnvColor[3],
                        image.abPrimColor[0],
                        image.abPrimColor[1],
                        image.abPrimColor[2],
                        image.abPrimColor[3],
                        image.nRowSwapMask);
    return image.nPixelBytes;
}

void DecodeTexturePixels(unsigned char *pDest,
                         int nSize,
                         int nTexelSize,
                         const unsigned char *pImage,
                         const unsigned char *pPalette,
                         int nFormat,
                         int bMirrorS,
                         int bMirrorT,
                         int nTileIndex,
                         int nTileLimit,
                         const TexScanTile *pTiles,
                         unsigned flags,
                         int bColorCombine,
                         unsigned envR,
                         unsigned envG,
                         unsigned envB,
                         unsigned envA,
                         unsigned primR,
                         unsigned primG,
                         unsigned primB,
                         unsigned primA,
                         unsigned nRowSwapMask) {
}

int CreateTextureFromTmemShadow(unsigned char *pTmem) {
    return static_cast<int>(g_dwWeatherShadeTextureId);
}
