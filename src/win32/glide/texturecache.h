#ifndef COASTLINE_PLATFORM_WIN_GLIDE_TEXTURECACHE_H
#define COASTLINE_PLATFORM_WIN_GLIDE_TEXTURECACHE_H

/**
 * @file
 * The texture memory of the texture mapping units of the Glide renderer, and the textures the game
 * downloads into it.
 */

#include <glide.h>
#include <windows.h>

/** The number of entries of g_aGlideTextureSlots. */
#define GLIDE_TEXTURE_SLOT_COUNT 1024

/** The number of entries of g_aGlideTmuMemory. */
#define GLIDE_TMU_COUNT 2

/** The address of texture memory where the upper region of a texture mapping unit begins. */
#define GLIDE_TMU_UPPER_REGION_ADDRESS 0x200000

/** A texture in the memory of a texture mapping unit. */
typedef struct GlideTextureSlot {
    int nUnknown0;        /* +0x00 */
    int bInUse;           /*!< Whether the slot holds a texture. */
    int nWidth;           /*!< The width of the texture, in texels. */
    int nHeight;          /*!< The height of the texture, in texels. */
    int nAspectRatio;     /*!< The aspect ratio, a GrAspectRatio_t. */
    const void *pData;    /*!< The texels downloaded last. */
    int nFormat;          /*!< The format of the texels, a GrTextureFormat_t. */
    int nMipMapMode;      /*!< The mipmapping mode, a GrMipMapMode_t. */
    int nMagnifyFilter;   /*!< The magnification filter, a GrTextureFilterMode_t. */
    int nMinifyFilter;    /*!< The minification filter, a GrTextureFilterMode_t. */
    int nClampS;          /*!< The clamping of s, a GrTextureClampMode_t. */
    int nClampT;          /*!< The clamping of t, a GrTextureClampMode_t. */
    int nUnknown30;       /* +0x30 */
    int nUnknown34;       /* +0x34 */
    unsigned dwLodBias;   /*!< The level of detail bias, in quarters. */
    int nSmallLod;        /*!< The smallest level of detail, a GrLOD_t. */
    int nLargeLod;        /*!< The largest level of detail, a GrLOD_t. */
    int nTmu;             /*!< The texture mapping unit, a GrChipID_t. */
    int nEvenOdd;         /*!< The levels of detail present, a GR_MIPMAPLEVELMASK_* value. */
    FxU32 dwStartAddress; /*!< The address of the texture in the memory of the unit. */
    int nLodBlend;        /*!< Whether the levels of detail blend, an FxBool. */
    unsigned char abUnknown54[0x70]; /* +0x54 */
    GrTexInfo info;                  /*!< The description of the texture that Glide downloads. */
} GlideTextureSlot;

/** The texture memory of a texture mapping unit that the textures have not taken. */
typedef struct GlideTmuMemory {
    FxU32 dwNextAddress;  /*!< The next free address below #GLIDE_TMU_UPPER_REGION_ADDRESS. */
    FxU32 dwUpperAddress; /*!< The next free address from #GLIDE_TMU_UPPER_REGION_ADDRESS up. */
} GlideTmuMemory;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The texture memory of each texture mapping unit not yet taken.
 *
 * @ghidraAddress BRGlide: 0x10661830
 */
extern GlideTmuMemory g_aGlideTmuMemory[GLIDE_TMU_COUNT];

/**
 * The textures in the memory of the texture mapping units.
 *
 * @ghidraAddress BRGlide: 0x10661840
 */
extern GlideTextureSlot g_aGlideTextureSlots[GLIDE_TEXTURE_SLOT_COUNT];

/**
 * The number of entries of g_aGlideTextureSlots in use.
 *
 * @ghidraAddress BRGlide: 0x105d17ec
 */
extern unsigned g_dwGlideTextureSlotCount;

/**
 * The texture mapping unit the texture combine was set for last, or -1 for none.
 *
 * @ghidraAddress BRGlide: 0x105e1820
 */
extern int g_nBoundTextureTmu;

/**
 * Empty the texture memory of every texture mapping unit.
 *
 * @ghidraAddress BRGlide: 0x100281c0
 */
void ResetGlideTmuTextureAllocator(void);

/**
 * Take the texture memory for a texture and record how it is sampled.
 *
 * @param nTmu The texture mapping unit.
 * @param nEvenOdd The levels of detail present, a GR_MIPMAPLEVELMASK_* value.
 * @param nWidth The width of the texture, in texels.
 * @param nHeight The height of the texture, in texels.
 * @param nFormat The format of the texels, a GrTextureFormat_t.
 * @param nMipMapMode The mipmapping mode, a GrMipMapMode_t.
 * @param nSmallLod The smallest level of detail, a GrLOD_t.
 * @param nLargeLod The largest level of detail, a GrLOD_t.
 * @param nAspectRatio The aspect ratio, a GrAspectRatio_t.
 * @param nClampS The clamping of s, a GrTextureClampMode_t.
 * @param nClampT The clamping of t, a GrTextureClampMode_t.
 * @param nMinifyFilter The minification filter, a GrTextureFilterMode_t.
 * @param nMagnifyFilter The magnification filter, a GrTextureFilterMode_t.
 * @param fLodBias The level of detail bias.
 * @param nLodBlend Whether the levels of detail blend, an FxBool.
 * @return The entry of g_aGlideTextureSlots, or -1 when the slots or the memory run out.
 * @ghidraAddress BRGlide: 0x10028200
 */
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
                            int nLodBlend);

/**
 * Download the texels of a texture into the memory of its texture mapping unit.
 *
 * @param nSlot The entry of g_aGlideTextureSlots.
 * @param pData The texels.
 * @param nUnused Not read.
 * @ghidraAddress BRGlide: 0x100283c0
 */
void DownloadGlideTextureMipMap(unsigned nSlot, const void *pData, int nUnused);

/**
 * Make a texture the source of its texture mapping unit, with its clamping, filters, level of
 * detail bias, and mipmapping.
 *
 * @param nSlot The entry of g_aGlideTextureSlots.
 * @ghidraAddress BRGlide: 0x10028420
 */
void BindGlideTextureSource(unsigned nSlot);

/**
 * Append a message and a space to g_szDebugOverlayText, unless the text already has the message.
 *
 * @param pszMessage The message.
 * @ghidraAddress BRGlide: 0x1006ff50
 */
void AppendUniqueGlideErrorMessage(const char *pszMessage);

/**
 * Create a texture from texels already in a format of Glide.
 *
 * @param pPixels The texels.
 * @param nWidth The width, in texels.
 * @param nHeight The height, in texels.
 * @param nFormat The format of the texels, a GrTextureFormat_t.
 * @return The entry of g_pTextureEntries.
 * @ghidraAddress BRGlide: 0x10027fb0
 */
int CreateGlideTextureFromRawImage(unsigned char *pPixels, int nWidth, int nHeight, int nFormat);

/**
 * Convert pixels of eight bits a channel to ARGB1555, with the high bit of alpha.
 *
 * @param pDest Receives the converted pixels.
 * @param pSource The pixels to convert.
 * @param nBytes The bytes of the pixels to convert.
 * @return The bytes written.
 * @ghidraAddress BRGlide: 0x10023cb0
 */
int ConvertRgba32ToArgb1555(unsigned char *pDest, const unsigned char *pSource, int nBytes);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_PLATFORM_WIN_GLIDE_TEXTURECACHE_H */
