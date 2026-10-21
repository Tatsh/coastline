#ifndef PR_GBI_H
#define PR_GBI_H

/**
 * @file
 * The graphics binary interface types the gu routines fill in for the RSP microcode.
 */

#include <PR/ultratypes.h>

/** A 4 by 4 matrix in s15.16 fixed point, with the integer halves before the fractions. */
typedef union {
    s32 m[4][4];
    long long force_structure_alignment;
} Mtx;

/** A directional light with its colour, a copy of the colour and its direction. */
typedef struct {
    u8 col[3];
    s8 pad1;
    u8 colc[3];
    s8 pad2;
    s8 dir[3];
    s8 pad3;
} Light_t;

typedef union {
    Light_t l;
    long long force_structure_alignment[2];
} Light;

/** The two directions the RSP uses to generate texture coordinates for reflection mapping. */
typedef struct {
    Light l[2];
} LookAt;

/** The texture coordinates of the two specular highlights. */
typedef struct {
    s32 x1, y1, x2, y2;
} Hilite_t;

typedef union {
    Hilite_t h;
    s32 force_structure_alignment[4];
} Hilite;

/** The two words of a display list command. */
typedef struct {
    u32 w0;
    u32 w1;
} Gwords;

/** A display list command for the Fast3D microcode. */
typedef union {
    Gwords words;
    long long force_structure_alignment;
} Gfx;

/** Place the low siz bits of v at bit s. */
#define _SHIFTL(v, s, siz) (((u32)(v) & ((0x01 << (siz)) - 1)) << (s))

/* The command opcodes of the F3DEX microcode, which keeps those of Fast3D. */
#define G_MTX 0x01
#define G_DL 0x06
#define G_TEXTURE 0xBB
#define G_POPMTX 0xBD
#define G_SETGEOMETRYMODE 0xB7
#define G_CLEARGEOMETRYMODE 0xB6
#define G_MOVEWORD 0xBC
#define G_SETOTHERMODE_L 0xB9
#define G_SETOTHERMODE_H 0xBA
#define G_RDPPIPESYNC 0xE7
#define G_RDPTILESYNC 0xE8
#define G_SETTILE 0xF5
#define G_SETENVCOLOR 0xFB
#define G_SETCOMBINE 0xFC
#define G_RDPHALF_2 0xB3
#define G_RDPHALF_1 0xB4
#define G_TEXRECT 0xE4
#define G_RDPLOADSYNC 0xE6
#define G_SETTILESIZE 0xF2
#define G_LOADBLOCK 0xF3
#define G_SETPRIMCOLOR 0xFA
#define G_SETTIMG 0xFD
#define G_FILLRECT 0xF6
#define G_SETFILLCOLOR 0xF7
#define G_SETCIMG 0xFF

/* The parameters of gSPMatrix(). */
#define G_MTX_MODELVIEW 0x00
#define G_MTX_PROJECTION 0x01
#define G_MTX_MUL 0x00
#define G_MTX_LOAD 0x02
#define G_MTX_NOPUSH 0x00
#define G_MTX_PUSH 0x04

/* The geometry modes. */
#define G_ZBUFFER 0x00000001
#define G_SHADE 0x00000004
#define G_SHADING_SMOOTH 0x00000200
#define G_CULL_FRONT 0x00001000
#define G_CULL_BACK 0x00002000
#define G_FOG 0x00010000
#define G_LIGHTING 0x00020000
#define G_TEXTURE_GEN 0x00040000
#define G_TEXTURE_GEN_LINEAR 0x00080000

/* The fields of the other modes, by their lowest bit. */
#define G_MDSFT_RENDERMODE 3
#define G_MDSFT_TEXTFILT 12
#define G_MDSFT_TEXTLUT 14
#define G_MDSFT_TEXTLOD 16
#define G_MDSFT_TEXTPERSP 19
#define G_MDSFT_CYCLETYPE 20

#define G_CYC_1CYCLE (0 << G_MDSFT_CYCLETYPE)
#define G_CYC_FILL (3 << G_MDSFT_CYCLETYPE)
#define G_TL_TILE (0 << G_MDSFT_TEXTLOD)
#define G_TT_NONE (0 << G_MDSFT_TEXTLUT)
#define G_TP_NONE (0 << G_MDSFT_TEXTPERSP)
#define G_TP_PERSP (1 << G_MDSFT_TEXTPERSP)
#define G_TT_RGBA16 (2 << G_MDSFT_TEXTLUT)

/* The coverage, depth and blending bits of a render mode. */
#define AA_EN 0x0008
#define Z_CMP 0x0010
#define Z_UPD 0x0020
#define IM_RD 0x0040
#define CLR_ON_CVG 0x0080
#define CVG_DST_CLAMP 0x0000
#define CVG_DST_WRAP 0x0100
#define CVG_DST_FULL 0x0200
#define CVG_DST_SAVE 0x0300
#define ZMODE_OPA 0x0000
#define CVG_X_ALPHA 0x1000
#define ALPHA_CVG_SEL 0x2000
#define FORCE_BL 0x4000

/* The inputs of the blender. */
#define G_BL_CLR_IN 0
#define G_BL_CLR_MEM 1
#define G_BL_A_IN 0
#define G_BL_1MA 0
#define G_BL_1 2
#define G_BL_0 3

/** The blender equation of the first cycle of a render mode. */
#define GBL_c1(m1a, m1b, m2a, m2b) ((m1a) << 30 | (m1b) << 26 | (m2a) << 22 | (m2b) << 18)

/** The blender equation of the second cycle of a render mode. */
#define GBL_c2(m1a, m1b, m2a, m2b) ((m1a) << 28 | (m1b) << 24 | (m2a) << 20 | (m2b) << 16)

/* The inputs of the colour combiner, named for G_CCMUX_ and G_ACMUX_ by gDPSetCombineLERP(). */
#define G_CCMUX_COMBINED 0
#define G_CCMUX_TEXEL0 1
#define G_CCMUX_TEXEL1 2
#define G_CCMUX_PRIMITIVE 3
#define G_CCMUX_SHADE 4
#define G_CCMUX_ENVIRONMENT 5
#define G_CCMUX_TEXEL0_ALPHA 8
#define G_CCMUX_PRIMITIVE_ALPHA 10
#define G_CCMUX_SHADE_ALPHA 11
#define G_CCMUX_ENV_ALPHA 12
#define G_CCMUX_1 6
#define G_CCMUX_0 31

#define G_ACMUX_COMBINED 0
#define G_ACMUX_TEXEL0 1
#define G_ACMUX_TEXEL1 2
#define G_ACMUX_PRIMITIVE 3
#define G_ACMUX_SHADE 4
#define G_ACMUX_ENVIRONMENT 5
#define G_ACMUX_1 6
#define G_ACMUX_0 7

/* The texture formats, sizes and tiles. */
#define G_IM_FMT_RGBA 0
#define G_IM_FMT_CI 2
#define G_IM_FMT_I 4
#define G_IM_SIZ_4b 0
#define G_IM_SIZ_8b 1
#define G_IM_SIZ_16b 2
#define G_TX_RENDERTILE 0
#define G_TX_LOADTILE 7
#define G_TX_WRAP 0
#define G_TX_CLAMP 2
#define G_TX_NOLOD 0
#define G_ON 1

/* The fixed-point fractions of texture coordinates and of the load stride. */
#define G_TEXTURE_IMAGE_FRAC 2
#define G_TX_DXT_FRAC 11

/** The most texels one load block takes, less one. */
#define G_TX_LDBLK_MAX_TXL 2047

/** The 64-bit words of a row of a 4-bit texture, at least one. */
#define TXL2WORDS_4b(txls) ((txls) / 16 > 1 ? (txls) / 16 : 1)

/** The step of the row counter of a load block for a 4-bit texture of a width. */
#define CALC_DXT_4b(width) (((1 << G_TX_DXT_FRAC) + TXL2WORDS_4b(width) - 1) / TXL2WORDS_4b(width))

/** The 64-bit words of a row of a texture with texels of b_txl bytes, at least one. */
#define TXL2WORDS(txls, b_txl) ((txls) * (b_txl) / 8 > 1 ? (txls) * (b_txl) / 8 : 1)

/** The step of the row counter of a load block for a texture with texels of b_txl bytes. */
#define CALC_DXT(width, b_txl)                                                                     \
    (((1 << G_TX_DXT_FRAC) + TXL2WORDS(width, b_txl) - 1) / TXL2WORDS(width, b_txl))

/* The words of the clipping ratio. */
#define G_MW_CLIP 0x04
#define G_MWO_CLIP_RNX 0x04
#define G_MWO_CLIP_RNY 0x0C
#define G_MWO_CLIP_RPX 0x14
#define G_MWO_CLIP_RPY 0x1C
#define FRUSTRATIO_1 1

/** Write a command of two words at pkt, which is evaluated once. */
#define gDisplayListWords(pkt, c0, c1)                                                             \
    do {                                                                                           \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
        _g->words.w0 = (c0);                                                                       \
        _g->words.w1 = (c1);                                                                       \
    } while (0)

#define gDPNoParam(pkt, cmd) gDisplayListWords(pkt, _SHIFTL(cmd, 24, 8), 0)
#define gImmp1(pkt, cmd, data) gDisplayListWords(pkt, _SHIFTL(cmd, 24, 8), (u32)(data))
#define gMoveWd(pkt, index, offset, data)                                                          \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_MOVEWORD, 24, 8) | _SHIFTL(offset, 8, 16) | _SHIFTL(index, 0, 8),  \
                      (u32)(data))
#define gSPSetOtherMode(pkt, cmd, sft, len, data)                                                  \
    gDisplayListWords(                                                                             \
        pkt, _SHIFTL(cmd, 24, 8) | _SHIFTL(sft, 8, 8) | _SHIFTL(len, 0, 8), (u32)(data))

#define gSPMatrix(pkt, m, p)                                                                       \
    gDisplayListWords(                                                                             \
        pkt, _SHIFTL(G_MTX, 24, 8) | _SHIFTL(p, 16, 8) | _SHIFTL(sizeof(Mtx), 0, 16), (u32)(m))
#define gSPPopMatrix(pkt, n) gImmp1(pkt, G_POPMTX, n)
#define gSPDisplayList(pkt, dl) gDisplayListWords(pkt, _SHIFTL(G_DL, 24, 8), (u32)(dl))
#define gSPSetGeometryMode(pkt, word) gImmp1(pkt, G_SETGEOMETRYMODE, word)
#define gSPClearGeometryMode(pkt, word) gImmp1(pkt, G_CLEARGEOMETRYMODE, word)
#define gSPTexture(pkt, s, t, level, tile, on)                                                     \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_TEXTURE, 24, 8) | _SHIFTL(level, 11, 3) | _SHIFTL(tile, 8, 3) |    \
                          _SHIFTL(on, 0, 8),                                                       \
                      _SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16))

/* The ratio of the guard band to the screen, which takes four commands. */
#define gSPClipRatio(pkt, r)                                                                       \
    do {                                                                                           \
        gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RNX, (u16)(r));                                         \
        gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RNY, (u16)(r));                                         \
        gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RPX, (u16) - (r));                                      \
        gMoveWd(pkt, G_MW_CLIP, G_MWO_CLIP_RPY, (u16) - (r));                                      \
    } while (0)

#define gDPPipeSync(pkt) gDPNoParam(pkt, G_RDPPIPESYNC)
#define gDPTileSync(pkt) gDPNoParam(pkt, G_RDPTILESYNC)
#define gDPSetCycleType(pkt, type)                                                                 \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_CYCLETYPE, 2, type)
#define gDPSetTextureFilter(pkt, type)                                                             \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTFILT, 2, type)
#define gDPSetTextureLOD(pkt, type) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTLOD, 1, type)
#define gDPSetRenderMode(pkt, c0, c1)                                                              \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, (c0) | (c1))
#define gDPSetEnvColor(pkt, r, g, b, a)                                                            \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETENVCOLOR, 24, 8),                                               \
                      _SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8))
#define gDPSetTile(                                                                                \
    pkt, fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts)              \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETTILE, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) |      \
                          _SHIFTL(line, 9, 9) | _SHIFTL(tmem, 0, 9),                               \
                      _SHIFTL(tile, 24, 3) | _SHIFTL(palette, 20, 4) | _SHIFTL(cmt, 18, 2) |       \
                          _SHIFTL(maskt, 14, 4) | _SHIFTL(shiftt, 10, 4) | _SHIFTL(cms, 8, 2) |    \
                          _SHIFTL(masks, 4, 4) | _SHIFTL(shifts, 0, 4))

/** Set the colour combiner from the a, b, c and d inputs of (a - b) * c + d for each cycle. */
#define gDPSetCombineLERP(                                                                         \
    pkt, a0, b0, c0, d0, Aa0, Ab0, Ac0, Ad0, a1, b1, c1, d1, Aa1, Ab1, Ac1, Ad1)                   \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETCOMBINE, 24, 8) | _SHIFTL(G_CCMUX_##a0, 20, 4) |                \
                          _SHIFTL(G_CCMUX_##c0, 15, 5) | _SHIFTL(G_ACMUX_##Aa0, 12, 3) |           \
                          _SHIFTL(G_ACMUX_##Ac0, 9, 3) | _SHIFTL(G_CCMUX_##a1, 5, 4) |             \
                          _SHIFTL(G_CCMUX_##c1, 0, 5),                                             \
                      _SHIFTL(G_CCMUX_##b0, 28, 4) | _SHIFTL(G_CCMUX_##b1, 24, 4) |                \
                          _SHIFTL(G_ACMUX_##Aa1, 21, 3) | _SHIFTL(G_ACMUX_##Ac1, 18, 3) |          \
                          _SHIFTL(G_CCMUX_##d0, 15, 3) | _SHIFTL(G_ACMUX_##Ab0, 12, 3) |           \
                          _SHIFTL(G_ACMUX_##Ad0, 9, 3) | _SHIFTL(G_CCMUX_##d1, 6, 3) |             \
                          _SHIFTL(G_ACMUX_##Ab1, 3, 3) | _SHIFTL(G_ACMUX_##Ad1, 0, 3))

#define gDPLoadSync(pkt) gDPNoParam(pkt, G_RDPLOADSYNC)
#define gDPSetColorImage(pkt, f, s, w, i)                                                          \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETCIMG, 24, 8) | _SHIFTL(f, 21, 3) | _SHIFTL(s, 19, 2) |          \
                          _SHIFTL((w) - 1, 0, 12),                                                 \
                      (u32)(i))
#define gDPSetFillColor(pkt, d) gDisplayListWords(pkt, _SHIFTL(G_SETFILLCOLOR, 24, 8), (u32)(d))
#define gDPFillRectangle(pkt, ulx, uly, lrx, lry)                                                  \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_FILLRECT, 24, 8) | _SHIFTL(lrx, 14, 10) | _SHIFTL(lry, 2, 10),     \
                      _SHIFTL(ulx, 14, 10) | _SHIFTL(uly, 2, 10))
#define gDPSetTextureLUT(pkt, type) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTLUT, 2, type)
#define gDPSetTexturePersp(pkt, type)                                                              \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTPERSP, 1, type)
#define gDPSetPrimColor(pkt, m, l, r, g, b, a)                                                     \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETPRIMCOLOR, 24, 8) | _SHIFTL(m, 8, 8) | _SHIFTL(l, 0, 8),        \
                      _SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8))
#define gDPSetTextureImage(pkt, f, s, w, i)                                                        \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETTIMG, 24, 8) | _SHIFTL(f, 21, 3) | _SHIFTL(s, 19, 2) |          \
                          _SHIFTL((w) - 1, 0, 12),                                                 \
                      (u32)(i))
#define gDPLoadBlock(pkt, tile, uls, ult, lrs, dxt)                                                \
    gDisplayListWords(                                                                             \
        pkt,                                                                                       \
        _SHIFTL(G_LOADBLOCK, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12),                  \
        _SHIFTL(tile, 24, 3) |                                                                     \
            _SHIFTL((lrs) < G_TX_LDBLK_MAX_TXL ? (lrs) : G_TX_LDBLK_MAX_TXL, 12, 12) |             \
            _SHIFTL(dxt, 0, 12))
#define gDPSetTileSize(pkt, t, uls, ult, lrs, lrt)                                                 \
    gDisplayListWords(pkt,                                                                         \
                      _SHIFTL(G_SETTILESIZE, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12),  \
                      _SHIFTL(t, 24, 3) | _SHIFTL(lrs, 12, 12) | _SHIFTL(lrt, 0, 12))

/** Load a 4-bit texture into the texture memory and set up the render tile to draw it. */
#define gDPLoadTextureBlock_4b(                                                                    \
    pkt, timg, fmt, width, height, pal, cms, cmt, masks, maskt, shifts, shiftt)                    \
    do {                                                                                           \
        gDPSetTextureImage(pkt, fmt, G_IM_SIZ_16b, 1, timg);                                       \
        gDPSetTile(pkt,                                                                            \
                   fmt,                                                                            \
                   G_IM_SIZ_16b,                                                                   \
                   0,                                                                              \
                   0,                                                                              \
                   G_TX_LOADTILE,                                                                  \
                   0,                                                                              \
                   cmt,                                                                            \
                   maskt,                                                                          \
                   shiftt,                                                                         \
                   cms,                                                                            \
                   masks,                                                                          \
                   shifts);                                                                        \
        gDPLoadSync(pkt);                                                                          \
        gDPLoadBlock(                                                                              \
            pkt, G_TX_LOADTILE, 0, 0, (((width) * (height) + 3) >> 2) - 1, CALC_DXT_4b(width));    \
        gDPPipeSync(pkt);                                                                          \
        gDPSetTile(pkt,                                                                            \
                   fmt,                                                                            \
                   G_IM_SIZ_4b,                                                                    \
                   (((width) >> 1) + 7) >> 3,                                                      \
                   0,                                                                              \
                   G_TX_RENDERTILE,                                                                \
                   pal,                                                                            \
                   cmt,                                                                            \
                   maskt,                                                                          \
                   shiftt,                                                                         \
                   cms,                                                                            \
                   masks,                                                                          \
                   shifts);                                                                        \
        gDPSetTileSize(pkt,                                                                        \
                       G_TX_RENDERTILE,                                                            \
                       0,                                                                          \
                       0,                                                                          \
                       ((width) - 1) << G_TEXTURE_IMAGE_FRAC,                                      \
                       ((height) - 1) << G_TEXTURE_IMAGE_FRAC);                                    \
    } while (0)

/** Draw a textured rectangle in screen coordinates of 10.2 fixed point. */
#define gSPTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy)                           \
    do {                                                                                           \
        gDisplayListWords(pkt,                                                                     \
                          _SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(xh, 12, 12) | _SHIFTL(yh, 0, 12),    \
                          _SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) | _SHIFTL(yl, 0, 12));        \
        gImmp1(pkt, G_RDPHALF_1, _SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16));                          \
        gImmp1(pkt, G_RDPHALF_2, _SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16));                    \
    } while (0)

#endif
