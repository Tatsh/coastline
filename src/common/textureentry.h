#pragma once

#include "common/renderbackend.h"
#include "common/texturetile.h"

/** The bits of g_dwDirtyRenderStateMask, one for each render state the game caches. */
enum RenderStateDirtyBit {
    RENDER_STATE_DIRTY_Z_WRITE_ENABLE = 0x1,       /*!< g_dwPendingRS_ZWriteEnable. */
    RENDER_STATE_DIRTY_ALPHA_TEST_ENABLE = 0x2,    /*!< g_dwPendingRS_AlphaTestEnable. */
    RENDER_STATE_DIRTY_SRC_BLEND = 0x4,            /*!< g_dwPendingRS_SrcBlend. */
    RENDER_STATE_DIRTY_DEST_BLEND = 0x8,           /*!< g_dwPendingRS_DestBlend. */
    RENDER_STATE_DIRTY_CULL_MODE = 0x10,           /*!< g_dwPendingRS_CullMode. */
    RENDER_STATE_DIRTY_Z_FUNC = 0x20,              /*!< g_dwPendingRS_ZFunc. */
    RENDER_STATE_DIRTY_ALPHA_REF = 0x40,           /*!< g_dwPendingRS_AlphaRef. */
    RENDER_STATE_DIRTY_ALPHA_FUNC = 0x80,          /*!< g_dwPendingRS_AlphaFunc. */
    RENDER_STATE_DIRTY_ADDRESS_U = 0x100,          /*!< g_dwPendingRS_AddressU. */
    RENDER_STATE_DIRTY_ADDRESS_V = 0x200,          /*!< g_dwPendingRS_AddressV. */
    RENDER_STATE_DIRTY_ALPHA_BLEND_ENABLE = 0x400, /*!< g_dwPendingRS_AlphaBlendEnable. */
};
/** Image metadata shared by texture entries and temporary texture conversions. */
struct TextureImageData {
    int nOption1;           /*!< Initialized to 0 by image construction. */
    unsigned char nOption2; /*!< Initialized to 3 by image construction. */
    char abUnknown5[3];
    int nWidth;
    int nHeight;
    int nPixelFormatId;
    int nOption3; /*!< Initialized to 0 for direct images and 2 for captured texture scans. */
    int nLargestLod;
    int nLod;           /*!< The level of detail, as CalculateTextureDimensions() reads it. */
    int nAspectRatio;   /*!< The aspect ratio, as CalculateTextureDimensions() reads it. */
    int bClampS;        /*!< Whether texture binding clamps s. */
    int bClampT;        /*!< Whether texture binding clamps t. */
    int nOption4;       /*!< Initialized to 1 by image construction. */
    int nOption5;       /*!< Initialized to 1 by image construction. */
    unsigned dwOption6; /*!< Initialized to 0xc0000000 by image construction. */
    int nOption7;       /*!< Initialized to 0 by image construction. */
#if TGR_BRD3D
    unsigned nColorKey; /*!< The colour key of the texture, or 0 for none. */
#endif
    int nPixelBytes;
    int nDecodedWidth;
    int nDecodedHeight;
    unsigned char *pImage;
    const unsigned char *pPalette;
    int bMirrorS;
    int bMirrorT;
    int nTileIndex; /*!< The active tile of aTiles. */
    int nTileLimit;
    TexScanTile aTiles[TEX_SCAN_TILE_COUNT];
    unsigned dwFlags;
    int bColorCombine;     /*!< Whether texture matching compares the captured colors. */
    int bOwnsPaintBuffers; /*!< Whether paint buffer memory is released with this image. */
    int nPaintGroup;
    int nPaintSet;
    int nDamageLevel; /*!< The level of damage, from 0 to 3. */
    int bHasTintPixels;
    unsigned char *apPaintBuffers[4]; /*!< Pixel buffers for the four damage levels. */
    unsigned nPaintPixelBytes;
    unsigned char abEnvColor[4];
    unsigned char abPrimColor[4];
    unsigned nRowSwapMask;
    int nDecodedBytes;
    int nSourceWidth;
    int nSourceHeight;
};

/** A loaded texture, its image metadata and texture-coordinate scales. */
struct TextureEntry {
    int nTextureId; /*!< The entry of g_ppTextureIdTable of the texture. */
    TextureImageData data;
    float fTextureScaleS;
    float fTextureScaleT;
};

/**
 * Get the size of a texture the game loaded.
 *
 * @param pWidth Receives the width.
 * @param pHeight Receives the height.
 * @param nEntry The entry of g_pTextureEntries.
 * @ghidraAddress BRD3D: 0x100290a0
 */
void GetTextureEntryDimensions(int *pWidth, int *pHeight, int nEntry);

/**
 * Make a texture the game loaded draw with the texture of another.
 *
 * @param nEntry The entry of g_pTextureEntries that takes the texture.
 * @param nSourceEntry The entry whose texture it takes.
 * @ghidraAddress BRD3D: 0x10024ab0
 * @ghidraAddress BRGlide: 0x10023d20
 */
void AliasTextureEntrySurface(int nEntry, int nSourceEntry);

/**
 * The number of entries of g_pTextureEntries.
 *
 * @ghidraAddress BRD3D: 0x105553f0
 * @ghidraAddress BRGlide: 0x10697a58
 */
extern unsigned g_dwTextureEntryCount;

/**
 * The textures the game loaded.
 *
 * @ghidraAddress BRD3D: 0x1057543c
 * @ghidraAddress BRGlide: 0x106b7aa0
 */
extern TextureEntry *g_pTextureEntries;

/**
 * @ghidraAddress BRD3D: 0x10024b60
 * @ghidraAddress BRGlide: 0x10027a10
 */
unsigned AllocateTextureEntry(void);
