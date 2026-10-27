#include "platform/win/texturescan.h"

#include <cstring>

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
