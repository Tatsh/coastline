#pragma once

/** A tile recorded by texture scans and loaded texture entries. */
typedef struct TexScanTile {
    int nFormat;    /*!< The format of the texels. */
    int nSize;      /*!< The size of the texels. */
    int nLineBytes; /*!< The bytes of a line of texels in the texture memory. */
    int nTmem;      /*!< The address of the tile in the texture memory, in words of 8 bytes. */
    int bMirrorS;   /*!< Whether s mirrors. */
    int bClampS;    /*!< Whether s clamps. */
    int bMirrorT;   /*!< Whether t mirrors. */
    int bClampT;    /*!< Whether t clamps. */
    int nMaskS;     /*!< The bits of s that wrap. */
    int nMaskT;     /*!< The bits of t that wrap. */
    int nShiftS;    /*!< The shift of s. */
    int nShiftT;    /*!< The shift of t. */
    int nUls;       /*!< The left edge, in texels with two bits of fraction. */
    int nUlt;       /*!< The top edge, in texels with two bits of fraction. */
    int nLrs;       /*!< The right edge, in texels with two bits of fraction. */
    int nLrt;       /*!< The bottom edge, in texels with two bits of fraction. */
} TexScanTile;

/** The number of tile records in a scan or loaded texture entry. */
#define TEX_SCAN_TILE_COUNT 8
