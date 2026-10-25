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
