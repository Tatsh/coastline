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
    int nOption1;           /*!< The Glide TMU selector; direct images can select the second TMU. */
    unsigned char nOption2; /*!< The mipmap-level mask, initialised to both odd and even levels. */
    int nWidth;             /*!< The texture width used during conversion, in texels. */
    int nHeight;            /*!< The texture height used during conversion, in texels. */
    int nPixelFormatId;     /*!< The selected backend pixel-format identifier. */
    int nOption3;           /*!< The mipmap mode, disabled for direct images. */
    int nLargestLod;        /*!< The final mipmap endpoint used with nLod to size the chain. */
    int nLod;               /*!< The level of detail, as CalculateTextureDimensions() reads it. */
    int nAspectRatio;       /*!< The aspect ratio, as CalculateTextureDimensions() reads it. */
    int bClampS;            /*!< Whether texture binding clamps s. */
    int bClampT;            /*!< Whether texture binding clamps t. */
    int nOption4;           /*!< The minification filter, initialised to bilinear filtering. */
    int nOption5;           /*!< The magnification filter, initialised to bilinear filtering. */
    unsigned dwOption6;     /*!< Backend texture options, initialised to 0xc0000000. */
    int nOption7;           /*!< An unreconstructed backend option, initialised to zero. */
#if TGR_BRD3D
    unsigned nColorKey; /*!< The colour key of the texture, or 0 for none. */
#endif
    int nPixelBytes;       /*!< The byte capacity calculated for the selected pixel format. */
    int nDecodedWidth;     /*!< The decoded image width, including required padding, in texels. */
    int nDecodedHeight;    /*!< The decoded image height, including required padding, in texels. */
    unsigned char *pImage; /*!< The source image bytes used during conversion. */
    const unsigned char *pPalette; /*!< The source palette, or null for direct-colour images. */
    int bMirrorS;   /*!< Whether conversion mirrors the source along its horizontal axis. */
    int bMirrorT;   /*!< Whether conversion mirrors the source along its vertical axis. */
    int nTileIndex; /*!< The active tile of aTiles. */
    int nTileLimit; /*!< The exclusive upper bound of tiles contributing mipmap levels. */
    TexScanTile aTiles[TEX_SCAN_TILE_COUNT]; /*!< Captured RDP descriptors for each texture tile. */
    unsigned dwFlags;  /*!< Texture-load category flags captured when the image is constructed. */
    int bColorCombine; /*!< Whether texture matching compares the captured colours. */
    int bOwnsPaintBuffers; /*!< Whether paint buffer memory is released with this image. */
    int nPaintGroup;    /*!< The paint-image group used to obtain custom car colours and decals. */
    int nPaintSet;      /*!< The paint set within nPaintGroup. */
    int nDamageLevel;   /*!< The level of damage, from 0 to 3. */
    int bHasTintPixels; /*!< Whether the paint rectangle includes colour-tinted pixels. */
    unsigned char *apPaintBuffers[4]; /*!< Pixel buffers for the four damage levels. */
    unsigned nPaintPixelBytes;        /*!< The capacity of each allocated paint buffer, in bytes. */
    unsigned char abEnvColor[4];      /*!< Captured environment colour, in RGBA order. */
    unsigned char abPrimColor[4];     /*!< Captured primitive colour, in RGBA order. */
    unsigned nRowSwapMask; /*!< Row-index bits that select the RDP's four-byte row address swap. */
    int nDecodedBytes;     /*!< The decoded pixel byte count before backend upload. */
    int nSourceWidth;      /*!< The original image width before padding or scaling, in texels. */
    int nSourceHeight;     /*!< The original image height before padding or scaling, in texels. */
};

/** A loaded texture, its image metadata and texture-coordinate scales. */
struct TextureEntry {
    int nTextureId;        /*!< The entry of g_ppTextureIdTable of the texture. */
    TextureImageData data; /*!< Source, decoded-image, paint, and backend texture metadata. */
    float fTextureScaleS;  /*!< The horizontal texture-coordinate multiplier for this entry. */
    float fTextureScaleT;  /*!< The vertical texture-coordinate multiplier for this entry. */
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
 * Append an uninitialised texture entry, expanding storage in blocks of 256 entries as needed.
 *
 * @return The new zero-based entry index. Existing entry pointers can be invalidated by growth.
 * @ghidraAddress BRD3D: 0x10024b60
 * @ghidraAddress BRGlide: 0x10027a10
 */
unsigned AllocateTextureEntry(void);
