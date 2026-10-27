#include "platform/win/texturescan.h"

#include <cstdint>
#include <cstring>

#include <zlib.h>

#include "platform/win/boot/app.h"
#include "platform/win/d3d/renderer.h"
#include "platform/win/paintshop.h"
#include "platform/win/render.h"
#include "platform/win/settings.h"
#include "vehiclemodel.h"

int g_dwTexScanLoadState;
GbiCommand *g_pTexScanCmdBlockStart;
int g_dwTexScanTileIndex;
int g_dwTexScanMipLevels;
int g_dwTexScanImageSiz;
const void *g_pTexScanImageAddress;
const void *g_pTexScanTlutAddress;
GbiCommand *g_pTexScanSetTextureImageCmd;
unsigned char g_abTmemShadow[TMEM_SHADOW_SIZE];
unsigned char g_abTmemShadowTlut[TMEM_SHADOW_TLUT_SIZE];
unsigned char *g_pTlutLoadList = g_abTmemShadowTlut;
const void *g_pTexScanLoadBlockSource;
int g_dwTexScanLoadBlockSize;
TexScanTile g_aTexScanTiles[TEX_SCAN_TILE_COUNT];
int g_bTexScanRenderModeBlendBit16;

// The shift of the render mode in the low word of the other modes.
#define TEX_SCAN_MODE_SHIFT_RENDER_MODE 0x300
// The render modes whose bit 16 the scan does not record.
#define TEX_SCAN_RENDER_MODE_A 0x504f50
#define TEX_SCAN_RENDER_MODE_B 0xc184240
#define TEX_SCAN_RENDER_MODE_C 0x504240
// The bits of a render mode that let the scan record its bit 16.
#define TEX_SCAN_RENDER_MODE_BLEND_BITS 0x1800

static inline int TexScanField12(unsigned dwWord, int nShift) {
    return dwWord >> nShift & 0xfff;
}

void MarkTextureCommandBlockStart(GbiCommand *pCommand) {
    if (g_pTexScanCmdBlockStart == nullptr) {
        g_pTexScanCmdBlockStart = pCommand;
    }
}

void ScanGbiTexture(GbiCommand *pCommand) {
    g_dwTexScanTileIndex = pCommand->w0 >> 8 & 7;
    g_dwTexScanMipLevels = pCommand->w0 >> 11 & 7;
}

void ScanGbiSetTextureImage(GbiCommand *pCommand) {
    int nState = g_dwTexScanLoadState;

    if (nState != TEX_SCAN_IDLE && nState != TEX_SCAN_BLOCK_LOADED &&
        nState != TEX_SCAN_TILE_SIZED) {
        return;
    }
    g_dwTexScanImageSiz = pCommand->w0 >> 19 & 3;
    g_pTexScanImageAddress = pCommand->pData;
    g_pTexScanTlutAddress = nullptr;
    if (nState == TEX_SCAN_IDLE) {
        g_pTexScanSetTextureImageCmd = pCommand;
        g_pTexScanCmdBlockStart = nullptr;
    }
    g_dwTexScanLoadState = TEX_SCAN_IMAGE_SET;
}

void ScanGbiLoadTlut(GbiCommand *pCommand) {
    if (g_dwTexScanLoadState != TEX_SCAN_IMAGE_SET) {
        return;
    }
    int nWidth = TexScanField12(pCommand->w1, 0) - TexScanField12(pCommand->w0, 0) + 1;
    int nHeight = TexScanField12(pCommand->w1, 12) - TexScanField12(pCommand->w0, 12) + 1;
    g_pTexScanTlutAddress = g_pTexScanImageAddress;
    memcpy(g_pTlutLoadList, g_pTexScanImageAddress, nWidth * nHeight * sizeof(unsigned short));
    g_dwTexScanLoadState = TEX_SCAN_TLUT_LOADED;
}

void ScanGbiLoadSync(GbiCommand *pCommand) {
    if (g_dwTexScanLoadState == TEX_SCAN_IMAGE_SET) {
        g_dwTexScanLoadState = TEX_SCAN_LOAD_SYNCED;
    }
}

void ScanGbiLoadBlock(GbiCommand *pCommand) {
    if (g_dwTexScanLoadState != TEX_SCAN_LOAD_SYNCED) {
        return;
    }
    g_pTexScanLoadBlockSource = g_pTexScanImageAddress;
    g_dwTexScanLoadBlockSize =
        (TexScanField12(pCommand->w1, 12) - TexScanField12(pCommand->w0, 12)) * 2 + 2;
    memcpy(g_abTmemShadow, g_pTexScanImageAddress, g_dwTexScanLoadBlockSize);
    g_dwTexScanLoadState = TEX_SCAN_BLOCK_LOADED;
}

void ScanGbiPipeSync(GbiCommand *pCommand) {
    if (g_dwTexScanLoadState != TEX_SCAN_IDLE) {
        if (g_dwTexScanLoadState == TEX_SCAN_TLUT_LOADED) {
            g_dwTexScanLoadState = TEX_SCAN_PIPE_SYNCED;
        }
    }
}

void ScanGbiTileSync(GbiCommand *pCommand) {
    if (g_dwTexScanLoadState == TEX_SCAN_BLOCK_LOADED ||
        g_dwTexScanLoadState == TEX_SCAN_TLUT_LOADED) {
        g_dwTexScanLoadState = TEX_SCAN_TILE_SYNCED;
    }
}

void ScanGbiSetTileSize(GbiCommand *pCommand) {
    TexScanTile &tile = g_aTexScanTiles[pCommand->w1 >> 24 & 7];

    tile.nUls = TexScanField12(pCommand->w0, 12);
    tile.nUlt = TexScanField12(pCommand->w0, 0);
    tile.nLrs = TexScanField12(pCommand->w1, 12);
    g_dwTexScanLoadState = TEX_SCAN_TILE_SIZED;
    tile.nLrt = TexScanField12(pCommand->w1, 0);
}

int g_dwTexScanTlutMode;
int g_dwTexScanRenderTileIndex;
int g_dwTexScanMaxTileIndex;

// The palettes of the textures of the console, and the formats of the palettes.
#define TEX_SCAN_TLUT_RGBA16 0x8000
#define TEX_SCAN_TLUT_IA16 0xc000
#define TEX_SCAN_TLUT_FORMAT_RGBA 0
#define TEX_SCAN_TLUT_FORMAT_IA 3
// The shifts of the modes of the high word of the other modes the scan reads.
#define TEX_SCAN_MODE_SHIFT_TEXTURE_LUT 0xe00
#define TEX_SCAN_MODE_SHIFT_TEXTURE_DETAIL 0x1100
// The detail of the textures that uses detail textures.
#define TEX_SCAN_TEXTURE_DETAIL_DETAIL 0x40000

void ScanGbiSetTile(GbiCommand *pCommand) {
    int nTile = pCommand->w1 >> 24 & 7;
    TexScanTile &tile = g_aTexScanTiles[nTile];

    tile.nFormat = pCommand->w0 >> 21 & 7;
    tile.nSize = pCommand->w0 >> 19 & 3;
    tile.nLineBytes = (pCommand->w0 >> 9 & 0x1ff) << 3;
    tile.nTmem = pCommand->w0 & 0x1ff;
    tile.bMirrorS = pCommand->w1 >> 8 & 1;
    tile.bClampS = pCommand->w1 >> 9 & 1;
    tile.bMirrorT = pCommand->w1 >> 18 & 1;
    tile.bClampT = pCommand->w1 >> 19 & 1;
    tile.nMaskS = pCommand->w1 >> 4 & 0xf;
    tile.nMaskT = pCommand->w1 >> 14 & 0xf;
    tile.nShiftS = pCommand->w1 & 0xf;
    tile.nShiftT = pCommand->w1 >> 10 & 0xf;
    if (g_dwTexScanLoadState == TEX_SCAN_BLOCK_LOADED ||
        g_dwTexScanLoadState == TEX_SCAN_TILE_SYNCED ||
        g_dwTexScanLoadState == TEX_SCAN_TLUT_LOADED || nTile > g_dwTexScanMaxTileIndex) {
        g_dwTexScanMaxTileIndex = nTile;
    }
    g_dwTexScanLoadState = TEX_SCAN_TILE_SET;
}

void ScanGbiTextureLutMode(GbiCommand *pCommand) {
    unsigned dwMode = pCommand->w1;
    if (dwMode == 0) {
        return;
    }
    if (dwMode == TEX_SCAN_TLUT_RGBA16) {
        g_dwTexScanTlutMode = TEX_SCAN_TLUT_FORMAT_RGBA;
    } else if (dwMode == TEX_SCAN_TLUT_IA16) {
        g_dwTexScanTlutMode = TEX_SCAN_TLUT_FORMAT_IA;
    }
}

void ScanGbiSetOtherModeH(GbiCommand *pCommand) {
    unsigned dwShift = pCommand->w0 & 0xff00;
    if (dwShift == TEX_SCAN_MODE_SHIFT_TEXTURE_LUT) {
        ScanGbiTextureLutMode(pCommand);
    } else if (dwShift == TEX_SCAN_MODE_SHIFT_TEXTURE_DETAIL) {
        g_dwTexScanRenderTileIndex = pCommand->w1 == TEX_SCAN_TEXTURE_DETAIL_DETAIL;
    }
}

void ScanGbiSetOtherModeL(GbiCommand *pCommand) {
    if ((pCommand->w0 & 0xff00) != TEX_SCAN_MODE_SHIFT_RENDER_MODE) {
        return;
    }
    unsigned dwMode = pCommand->w1;
    if (dwMode == TEX_SCAN_RENDER_MODE_A || dwMode == TEX_SCAN_RENDER_MODE_B ||
        dwMode == TEX_SCAN_RENDER_MODE_C) {
        g_bTexScanRenderModeBlendBit16 = false;
    } else if (dwMode != 0 && (dwMode & TEX_SCAN_RENDER_MODE_BLEND_BITS)) {
        g_bTexScanRenderModeBlendBit16 = dwMode >> 16 & 1;
    } else {
        g_bTexScanRenderModeBlendBit16 = false;
    }
}

int g_bTexScanColorCombine;
int g_bTexScanPrimColorSet;
int g_bTexScanEnvColorSet;
unsigned char g_bTexScanPrimColorR;
unsigned char g_bTexScanPrimColorG;
unsigned char g_bTexScanPrimColorB;
unsigned char g_bTexScanPrimColorA;
unsigned char g_bTexScanEnvColorR;
unsigned char g_bTexScanEnvColorG;
unsigned char g_bTexScanEnvColorB;
unsigned char g_bTexScanEnvColorA;
void (*g_pfnPreprocessDisplayListTextures)(GbiCommand *pCommand);

void FlushPendingTextureLoad(GbiCommand *pCommand) {
    if (g_dwTexScanLoadState == TEX_SCAN_IDLE) {
        return;
    }
    if (!g_pTexScanCmdBlockStart) {
        g_pTexScanCmdBlockStart = pCommand;
    }
    int nEntry = CreateTextureFromTmemShadow(g_abTmemShadow);
    if (nEntry != -1) {
        g_pTexScanSetTextureImageCmd->w0 = (static_cast<unsigned>(nEntry) & 0xffffff) |
                                           (static_cast<unsigned>(GBI_OP_BIND_TEXTURE_ENTRY) << 24);
        g_pTexScanSetTextureImageCmd->w1 = g_pTexScanCmdBlockStart - g_pTexScanSetTextureImageCmd;
    }
    g_dwTexScanLoadState = TEX_SCAN_IDLE;
}

void PreprocessDisplayListTextures(GbiCommand *pCommand) {
    if (!pCommand) {
        return;
    }
    g_dwTexScanRenderTileIndex = 0;
    g_dwTexScanMaxTileIndex = 0;
    g_bTexScanColorCombine = false;
    g_dwTexScanLoadState = TEX_SCAN_IDLE;
    g_pTexScanCmdBlockStart = nullptr;
    for (;; pCommand++) {
        switch (pCommand->w0 >> 24) {
        case GBI_OP_VERTEX:
        case GBI_OP_TRI2:
        case GBI_OP_TRI1:
            FlushPendingTextureLoad(pCommand);
            break;
        case GBI_OP_END_DISPLAY_LIST:
            return;
        case GBI_OP_SET_OTHER_MODE_L:
            ScanGbiSetOtherModeL(pCommand);
            MarkTextureCommandBlockStart(pCommand);
            break;
        case 0xba:
            ScanGbiSetOtherModeH(pCommand);
            break;
        case 0xbb:
            ScanGbiTexture(pCommand);
            break;
        case 0xe6:
            ScanGbiLoadSync(pCommand);
            break;
        case 0xe7:
            ScanGbiPipeSync(pCommand);
            break;
        case 0xe8:
            ScanGbiTileSync(pCommand);
            break;
        case 0xf0:
            ScanGbiLoadTlut(pCommand);
            break;
        case GBI_OP_SET_TILE_SIZE:
            ScanGbiSetTileSize(pCommand);
            break;
        case 0xf3:
            ScanGbiLoadBlock(pCommand);
            break;
        case 0xf5:
            ScanGbiSetTile(pCommand);
            break;
        case GBI_OP_SET_PRIM_COLOR:
            g_bTexScanPrimColorR = pCommand->w1 >> 24;
            g_bTexScanPrimColorG = pCommand->w1 >> 16;
            g_bTexScanPrimColorB = pCommand->w1 >> 8;
            g_bTexScanPrimColorA = pCommand->w1;
            g_bTexScanPrimColorSet = true;
            break;
        case GBI_OP_SET_ENV_COLOR:
            g_bTexScanEnvColorR = pCommand->w1 >> 24;
            g_bTexScanEnvColorG = pCommand->w1 >> 16;
            g_bTexScanEnvColorB = pCommand->w1 >> 8;
            g_bTexScanEnvColorA = pCommand->w1;
            g_bTexScanEnvColorSet = true;
            break;
        case GBI_OP_SET_COMBINE:
            g_bTexScanColorCombine = pCommand->w0 == 0xfc50fe04 && pCommand->w1 == 0x3ffdf3f8;
            break;
        case 0xfd:
            ScanGbiSetTextureImage(pCommand);
            break;
        default:
            MarkTextureCommandBlockStart(pCommand);
            break;
        }
    }
}

int CreateTextureFromTmemShadow(unsigned char *pTmem) {
    TextureImageData image;
    image.bColorCombine = g_bTexScanColorCombine;
    image.pImage =
        const_cast<unsigned char *>(static_cast<const unsigned char *>(g_pTexScanLoadBlockSource));
    image.pPalette = static_cast<const unsigned char *>(g_pTexScanTlutAddress);
    image.abEnvColor[0] = g_bTexScanEnvColorR;
    image.abEnvColor[1] = g_bTexScanEnvColorG;
    image.abEnvColor[2] = g_bTexScanEnvColorB;
    image.abEnvColor[3] = g_bTexScanEnvColorA;
    image.abPrimColor[0] = g_bTexScanPrimColorR;
    image.abPrimColor[1] = g_bTexScanPrimColorG;
    image.abPrimColor[2] = g_bTexScanPrimColorB;
    image.abPrimColor[3] = g_bTexScanPrimColorA;
    image.bOwnsPaintBuffers = 0;
    int existing = FindMatchingTextureEntry(&image);
    if (existing != -1) {
        return existing;
    }
    image.nTileIndex = g_dwTexScanRenderTileIndex;
    if (g_dwTexScanMaxTileIndex < image.nTileIndex) {
        g_dwTexScanMaxTileIndex = image.nTileIndex;
    }
    const TexScanTile &tile = g_aTexScanTiles[image.nTileIndex];
    int maskS = tile.nMaskS;
    int maskT = tile.nMaskT;
    for (int next = image.nTileIndex + 1; next <= g_dwTexScanMaxTileIndex; next++) {
        maskS--;
        if (g_aTexScanTiles[next].nMaskS != maskS || g_aTexScanTiles[next].nMaskT != --maskT) {
            g_dwTexScanMaxTileIndex = next - 1;
            break;
        }
    }
    image.nOption1 = 0;
    image.nOption2 = 3;
    image.nWidth = image.nDecodedWidth =
        static_cast<int>(1u << (static_cast<unsigned>(tile.nMaskS) & 31));
    image.nHeight = image.nDecodedHeight =
        static_cast<int>(1u << (static_cast<unsigned>(tile.nMaskT) & 31));
    int mirrorS = tile.bMirrorS;
    int mirrorT = tile.bMirrorT;
    if (mirrorS) {
        image.nWidth = static_cast<int>(static_cast<unsigned>(image.nWidth) << 1);
    }
    if (mirrorT) {
        image.nHeight = static_cast<int>(static_cast<unsigned>(image.nHeight) << 1);
    }
    image.nPixelFormatId = ChooseTexturePixelFormat(tile.nSize, tile.nFormat, image.bColorCombine);
    image.nOption3 = 2;
    image.bClampS = tile.bClampS != 0;
    image.bClampT = tile.bClampT != 0;
    int edgeWidth =
        static_cast<int>(static_cast<unsigned>(tile.nLrs) - static_cast<unsigned>(tile.nUls) + 4) >>
        2;
    int edgeHeight =
        static_cast<int>(static_cast<unsigned>(tile.nLrt) - static_cast<unsigned>(tile.nUlt) + 4) >>
        2;
    if (edgeWidth != image.nDecodedWidth && edgeWidth != image.nWidth && edgeWidth > image.nWidth &&
        edgeWidth % image.nWidth == 0) {
        image.bClampS = 0;
    }
    if (edgeHeight != image.nDecodedHeight && edgeHeight != image.nHeight &&
        edgeHeight > image.nHeight && edgeHeight % image.nHeight == 0) {
        image.bClampT = 0;
    }
    CalculateTextureLodFromDimensions(&image.nLod, image.nWidth, image.nHeight);
    bool invalidAspect =
        !CalculateTextureAspectRatio(&image.nAspectRatio, image.nWidth, image.nHeight);
    unsigned levelShift = static_cast<unsigned>(g_dwTexScanMaxTileIndex - image.nTileIndex) & 31;
    CalculateTextureLodFromDimensions(
        &image.nLargestLod, image.nWidth >> levelShift, image.nHeight >> levelShift);
    if (g_dwTextureQualityLevel > 1) {
        image.nLargestLod = image.nLod;
        g_dwTexScanMaxTileIndex = image.nTileIndex;
    }
    image.nOption4 = 1;
    image.nOption5 = 1;
    image.dwOption6 = 0xc0000000;
    image.nOption7 = 0;
    image.nPixelBytes = ComputeMipChainByteSize(
        image.nLargestLod, image.nLod, image.nAspectRatio, image.nPixelFormatId);
    if (image.nPixelBytes > 0x80000) {
        image.nPixelBytes = 0x80000;
    }
    std::memcpy(image.aTiles, g_aTexScanTiles, sizeof(image.aTiles));
    image.nTileLimit = g_dwTexScanMaxTileIndex + 1;
    image.bMirrorS = mirrorS;
    image.bMirrorT = mirrorT;
    image.nRowSwapMask = 1;
    image.nSourceWidth = image.nDecodedWidth = image.nWidth;
    image.nSourceHeight = image.nDecodedHeight = image.nHeight;
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
    bool exceedsDevice =
        (g_D3DHalDeviceDesc.dwMaxTextureWidth &&
         static_cast<unsigned>(image.nWidth) > g_D3DHalDeviceDesc.dwMaxTextureWidth) ||
        (g_D3DHalDeviceDesc.dwMaxTextureHeight &&
         static_cast<unsigned>(image.nHeight) > g_D3DHalDeviceDesc.dwMaxTextureHeight);
    if (invalidAspect || mirrorS || mirrorT || requiresSquare() || exceedsDevice ||
        image.nWidth > g_dwCurrentMaxTextureWidth || image.nHeight > g_dwCurrentMaxTextureHeight) {
        int width = image.nWidth;
        int height = image.nHeight;
        auto update = [&](int w, int h) {
            CalculateTextureLodFromDimensions(&image.nLod, w, h);
            int exact = CalculateTextureAspectRatio(&image.nAspectRatio, w, h);
            image.nLargestLod = image.nLod - image.nTileIndex + g_dwTexScanMaxTileIndex;
            return !exact;
        };
        if (mirrorS && g_dwTextureQualityLevel > 1) {
            width /= 2;
            invalidAspect = update(width, height);
        }
        if (mirrorT && g_dwTextureQualityLevel > 1) {
            height /= 2;
            invalidAspect = update(width, height);
        }
        if (invalidAspect) {
            int w = image.nWidth;
            int h = image.nHeight;
            if (w >= h && w > 1) {
                w /= 2;
                update(w, h);
            } else if (w < h && h > 1) {
                h /= 2;
                update(w, h);
            }
            CalculateTextureDimensions(&width, &height, image.nLod, image.nAspectRatio);
        }
        if (g_D3DHalDeviceDesc.dwMaxTextureWidth &&
            static_cast<unsigned>(width) > g_D3DHalDeviceDesc.dwMaxTextureWidth) {
            width = static_cast<int>(g_D3DHalDeviceDesc.dwMaxTextureWidth);
            update(width, height);
        }
        if (g_D3DHalDeviceDesc.dwMaxTextureHeight &&
            static_cast<unsigned>(height) > g_D3DHalDeviceDesc.dwMaxTextureHeight) {
            height = static_cast<int>(g_D3DHalDeviceDesc.dwMaxTextureHeight);
            update(width, height);
        }
        if (width > g_dwCurrentMaxTextureWidth) {
            width = g_dwCurrentMaxTextureWidth;
            update(width, height);
        }
        if (height > g_dwCurrentMaxTextureHeight) {
            height = g_dwCurrentMaxTextureHeight;
            update(width, height);
        }
        if (requiresSquare() && width != height) {
            if (width >= height) {
                width = width >= g_dwCurrentMaxTextureWidth ? g_dwCurrentMaxTextureWidth : width;
                height = width;
            } else {
                height =
                    height >= g_dwCurrentMaxTextureHeight ? g_dwCurrentMaxTextureHeight : height;
                width = height;
            }
            update(width, height);
        }
        image.nWidth = width;
        image.nHeight = height;
    }
    image.nColorKey = g_bTexScanRenderModeBlendBit16 && !g_bD3DAlphaCompare;
    if (g_dwTextureQualityLevel > 1 &&
        LookupTextureMapEntry(static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(image.pImage)),
                              &image.nWidth,
                              &image.nHeight)) {
        CalculateTextureLodFromDimensions(&image.nLod, image.nWidth, image.nHeight);
        CalculateTextureAspectRatio(&image.nAspectRatio, image.nWidth, image.nHeight);
        image.nLargestLod = image.nLod - image.nTileIndex + g_dwTexScanMaxTileIndex;
    }
    image.dwFlags = g_dwTextureLoadCategory;
    image.nDecodedBytes = image.nPixelBytes;
    if ((image.dwFlags & 2) && image.nTileLimit == 2) {
        image.dwFlags |= 0x80;
    }
    unsigned char *pixels = GetTextureImagePixels(&image);
    int entry = CreateTextureFromImage(&image, pixels);
    if (g_pTextureEntries[entry].data.bOwnsPaintBuffers) {
        int option = GetSelectedPaintOption();
        if (option >= 0 && option < DAMAGE_TEXTURE_LEVEL_COUNT) {
            g_adwDamageTextureLevels[option] = entry;
        }
    }
    if (image.dwFlags & 2) {
        if (image.bOwnsPaintBuffers) {
            if (image.bHasTintPixels) {
                AppendVehicleModelTextureEntry(entry);
            }
        } else if (IsVehicleMaterialPalette(image.pPalette)) {
            unsigned char *palette = const_cast<unsigned char *>(image.pPalette);
            unsigned firstColors;
            std::memcpy(&firstColors, palette, sizeof(firstColors));
            std::memset(palette, 0xff, sizeof(firstColors));
            pixels = GetTextureImagePixels(&image);
            g_pfnUploadTextureEntryToSurface(entry, pixels);
            unsigned checksum = adler32(adler32(0, nullptr, 0), pixels, image.nPixelBytes);
            std::memcpy(palette, &firstColors, sizeof(firstColors));
            pixels = GetTextureImagePixels(&image);
            g_pfnUploadTextureEntryToSurface(entry, pixels);
            if (adler32(adler32(0, nullptr, 0), pixels, image.nPixelBytes) != checksum) {
                AppendVehicleModelTextureEntry(entry);
            }
        }
    }
    return entry;
}
