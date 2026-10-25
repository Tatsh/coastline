#pragma once

/**
 * @file
 * The scan of the display lists of the game in the PC port for the textures they load.
 */

#include "platform/win/displaylist.h"

/** The states of the scan of a load of a texture, by the command that sets each. */
enum TexScanLoadState {
    TEX_SCAN_IDLE = 0,         /*!< No load is being scanned. */
    TEX_SCAN_IMAGE_SET = 1,    /*!< ScanGbiSetTextureImage(). */
    TEX_SCAN_LOAD_SYNCED = 2,  /*!< ScanGbiLoadSync(). */
    TEX_SCAN_BLOCK_LOADED = 3, /*!< ScanGbiLoadBlock(). */
    TEX_SCAN_TILE_SYNCED = 4,  /*!< ScanGbiTileSync(). */
    TEX_SCAN_TILE_SIZED = 6,   /*!< ScanGbiSetTileSize(). */
    TEX_SCAN_TLUT_LOADED = 7,  /*!< ScanGbiLoadTlut(). */
    TEX_SCAN_PIPE_SYNCED = 8,  /*!< ScanGbiPipeSync(). */
};

/** A tile the scan records. The layout is not recovered past its edges. */
struct TexScanTile {
    char abUnknown0[0x30]; /* +0x00 */
    int nUls;              /*!< The left edge, in texels with two bits of fraction. */
    int nUlt;              /*!< The top edge, in texels with two bits of fraction. */
    int nLrs;              /*!< The right edge, in texels with two bits of fraction. */
    int nLrt;              /*!< The bottom edge, in texels with two bits of fraction. */
};

/** The number of entries of g_aTexScanTiles. */
#define TEX_SCAN_TILE_COUNT 8

/** The size of g_abTmemShadow. */
#define TMEM_SHADOW_SIZE 0x800

/** The size of g_abTmemShadowTlut. */
#define TMEM_SHADOW_TLUT_SIZE 0xf800

/**
 * The state of the scan of the load of a texture.
 *
 * @ghidraAddress BRD3D: 0x104d51ac
 * @ghidraAddress BRGlide: 0x105e17fc
 */
extern int g_dwTexScanLoadState;

/**
 * The first command of the commands that load a texture, or `nullptr` before one is marked.
 *
 * @ghidraAddress BRD3D: 0x10575438
 * @ghidraAddress BRGlide: 0x106b7a9c
 */
extern GbiCommand *g_pTexScanCmdBlockStart;

/**
 * The tile the last texture command chose.
 *
 * @ghidraAddress BRD3D: 0x105553e8
 * @ghidraAddress BRGlide: 0x10697a50
 */
extern int g_dwTexScanTileIndex;

/**
 * The levels of detail the last texture command gave.
 *
 * @ghidraAddress BRD3D: 0x105553e0
 * @ghidraAddress BRGlide: 0x10697a48
 */
extern int g_dwTexScanMipLevels;

/**
 * The size of the texels of the texture image set last.
 *
 * @ghidraAddress BRD3D: 0x104d51b4
 * @ghidraAddress BRGlide: 0x105e1804
 */
extern int g_dwTexScanImageSiz;

/**
 * The texels of the texture image set last.
 *
 * @ghidraAddress BRD3D: 0x104d51bc
 * @ghidraAddress BRGlide: 0x105e180c
 */
extern const void *g_pTexScanImageAddress;

/**
 * The texels of the palette loaded last, `nullptr` after a texture image without one.
 *
 * @ghidraAddress BRD3D: 0x10575434
 * @ghidraAddress BRGlide: 0x106b7a98
 */
extern const void *g_pTexScanTlutAddress;

/**
 * The command that set the texture image when the scan was idle.
 *
 * @ghidraAddress BRD3D: 0x105553fc
 * @ghidraAddress BRGlide: 0x10697a64
 */
extern GbiCommand *g_pTexScanSetTextureImageCmd;

/**
 * The copy of the texture memory of the console that the blocks load into.
 *
 * @ghidraAddress BRD3D: 0x104c51a8
 * @ghidraAddress BRGlide: 0x105d17f8
 */
extern unsigned char g_abTmemShadow[TMEM_SHADOW_SIZE];

/**
 * The copy of the palettes the scan loads.
 *
 * @ghidraAddress BRD3D: 0x104c59a8
 * @ghidraAddress BRGlide: 0x105d1ff8
 */
extern unsigned char g_abTmemShadowTlut[TMEM_SHADOW_TLUT_SIZE];

/**
 * Where ScanGbiLoadTlut() copies a palette.
 *
 * @ghidraAddress BRD3D: 0x100a7df0
 * @ghidraAddress BRGlide: 0x100a9e58
 */
extern unsigned char *g_pTlutLoadList;

/**
 * The texels the last block was loaded from.
 *
 * @ghidraAddress BRD3D: 0x104c51a4
 * @ghidraAddress BRGlide: 0x105d17f0
 */
extern const void *g_pTexScanLoadBlockSource;

/**
 * The bytes of the last block loaded.
 *
 * @ghidraAddress BRD3D: 0x105553ec
 * @ghidraAddress BRGlide: 0x10697a54
 */
extern int g_dwTexScanLoadBlockSize;

/**
 * The tiles the scan records.
 *
 * @ghidraAddress BRD3D: 0x105551d8
 * @ghidraAddress BRGlide: 0x10697840
 */
extern TexScanTile g_aTexScanTiles[TEX_SCAN_TILE_COUNT];

/**
 * Bit 16 of the render mode set last when bit 11 or 12 of it is set, and 0 otherwise and for
 * three render modes the scan knows.
 *
 * @ghidraAddress BRD3D: 0x10575414
 * @ghidraAddress BRGlide: 0x106b7a7c
 */
extern int g_bTexScanRenderModeBlendBit16;

/**
 * Mark the first command of the commands that load a texture, when none is marked.
 *
 * @param pCommand The command.
 * @ghidraAddress BRD3D: 0x10029e60
 * @ghidraAddress BRGlide: 0x100293d0
 */
void MarkTextureCommandBlockStart(GbiCommand *pCommand);

/**
 * Record the tile and the levels of detail of a texture command.
 *
 * @param pCommand The command.
 * @ghidraAddress BRD3D: 0x10029e80
 * @ghidraAddress BRGlide: 0x100293f0
 */
void ScanGbiTexture(GbiCommand *pCommand);

/**
 * Record the texture image of a command, when the scan is idle or after a block or a tile size.
 *
 * @param pCommand The command.
 * @ghidraAddress BRD3D: 0x10029eb0
 * @ghidraAddress BRGlide: 0x10029420
 */
void ScanGbiSetTextureImage(GbiCommand *pCommand);

/**
 * Copy the palette of a command from the texture image into g_pTlutLoadList, after a texture
 * image.
 *
 * @param pCommand The command.
 * @ghidraAddress BRD3D: 0x10029f10
 * @ghidraAddress BRGlide: 0x10029480
 */
void ScanGbiLoadTlut(GbiCommand *pCommand);

/**
 * Mark the sync of a load, after a texture image.
 *
 * @param pCommand The command.
 * @ghidraAddress BRD3D: 0x10029f80
 * @ghidraAddress BRGlide: 0x100294f0
 */
void ScanGbiLoadSync(GbiCommand *pCommand);

/**
 * Copy the block of a command from the texture image into g_abTmemShadow, after the sync of a load.
 *
 * @param pCommand The command.
 * @ghidraAddress BRD3D: 0x10029fa0
 * @ghidraAddress BRGlide: 0x10029510
 */
void ScanGbiLoadBlock(GbiCommand *pCommand);

/**
 * Mark the sync of the pipeline, after a palette.
 *
 * @param pCommand The command.
 * @ghidraAddress BRD3D: 0x1002a000
 * @ghidraAddress BRGlide: 0x10029570
 */
void ScanGbiPipeSync(GbiCommand *pCommand);

/**
 * Mark the sync of a tile, after a block or a palette.
 *
 * @param pCommand The command.
 * @ghidraAddress BRD3D: 0x1002a020
 * @ghidraAddress BRGlide: 0x10029590
 */
void ScanGbiTileSync(GbiCommand *pCommand);

/**
 * Record the edges of the tile of a command.
 *
 * @param pCommand The command.
 * @ghidraAddress BRD3D: 0x1002a140
 * @ghidraAddress BRGlide: 0x100296b0
 */
void ScanGbiSetTileSize(GbiCommand *pCommand);

/**
 * Record the blending of the render mode of a command of the low word of the other modes.
 *
 * @param pCommand The command.
 * @ghidraAddress BRD3D: 0x1002a1a0
 * @ghidraAddress BRGlide: 0x10029710
 */
void ScanGbiSetOtherModeL(GbiCommand *pCommand);
