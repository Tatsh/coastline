#ifndef COASTLINE_N64_RACE_INTERNAL_H
#define COASTLINE_N64_RACE_INTERNAL_H

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef float f32;
typedef double f64;

#define NULL 0

typedef struct {
    unsigned int w0;
    unsigned int w1;
} Gwords;

typedef union {
    Gwords words;
    long long force_structure_alignment;
} Gfx;

typedef struct {
    int m[16];
} Mtx;

typedef struct {
    short ob[3];
    unsigned short flag;
    short tc[2];
    unsigned char cn[4];
} Vtx_t;

typedef union {
    Vtx_t v;
    long long force_structure_alignment;
} Vtx;

typedef struct {
    short vscale[4];
    short vtrans[4];
} Vp_t;

typedef union {
    Vp_t vp;
    long long force_structure_alignment;
} Vp;

#define G_MTX 0x01
#define G_MOVEMEM 0x03
#define G_SETOTHERMODE_L 0xb9
#define G_SETOTHERMODE_H 0xba
#define G_RDPPIPESYNC 0xe7
#define G_FILLRECT 0xf6
#define G_SETPRIMCOLOR 0xfa
#define G_SETPRIMDEPTH 0xee
#define G_SETFILLCOLOR 0xf7
#define G_SETCIMG 0xff
#define G_SETTIMG 0xfd
#define G_SETTILE 0xf5
#define G_LOADBLOCK 0xf3
#define G_LOADTLUT 0xf0
#define G_SETTILESIZE 0xf2
#define G_TEXTURE 0xbb
#define G_RDPLOADSYNC 0xe6
#define G_RDPTILESYNC 0xe8
#define G_TX_LDBLK_MAX_TXL 2047
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#define G_TEXRECT 0xe4
#define G_RDPHALF_1 0xb4
#define G_RDPHALF_2 0xb3
#define G_IM_FMT_RGBA 0
#define G_IM_SIZ_16b 2
#define G_MAXFBZ 0x3fff
#define GPACK_ZDZ(z, dz) ((z) << 2 | (dz))
#define G_SETCOMBINE 0xfc
#define G_SETSCISSOR 0xed
#define G_SC_NON_INTERLACE 0
#define G_DL 0x06
#define G_MOVEWORD 0xbc

#define G_MTX_NOPUSH 0x00
#define G_MTX_PUSH 0x04
#define G_MTX_MUL 0x00
#define G_MTX_LOAD 0x02
#define G_MTX_MODELVIEW 0x00
#define G_MTX_PROJECTION 0x01

#define G_DL_PUSH 0x00
#define G_DL_NOPUSH 0x01

#define G_MW_PERSPNORM 0x0e
#define G_MV_VIEWPORT 0x80

#define G_MDSFT_RENDERMODE 3
#define G_MDSFT_RGBDITHER 6
#define G_MDSFT_CYCLETYPE 20

#define G_CYC_1CYCLE (0 << G_MDSFT_CYCLETYPE)
#define G_CYC_FILL (3 << G_MDSFT_CYCLETYPE)
#define G_CD_DISABLE (3 << G_MDSFT_RGBDITHER)

/* render modes used so far, as libultra composes them */
#define G_RM_CLD_SURF 0x00404340
#define G_RM_CLD_SURF2 0x00104340
#define G_RM_OPA_SURF 0x0c084000
#define G_RM_OPA_SURF2 0x03024000

#define GPACK_RGBA5551(r, g, b, a)                                                                 \
    ((((r) << 8) & 0xf800) | (((g) << 3) & 0x7c0) | (((b) >> 2) & 0x3e) | ((a) & 0x1))

#define _SHIFTL(v, s, w) ((unsigned int)(((unsigned int)(v) & ((0x01 << (w)) - 1)) << (s)))

#define gDma1p(pkt, c, s, l, p)                                                                    \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = (_SHIFTL((c), 24, 8) | _SHIFTL((p), 16, 8) | _SHIFTL((l), 0, 16));          \
        _g->words.w1 = (unsigned int)(s);                                                          \
    }

#define gMoveWd(pkt, index, offset, data)                                                          \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 =                                                                             \
            (_SHIFTL(G_MOVEWORD, 24, 8) | _SHIFTL((offset), 8, 16) | _SHIFTL((index), 0, 8));      \
        _g->words.w1 = (unsigned int)(data);                                                       \
    }

#define gDPNoParam(pkt, cmd)                                                                       \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = _SHIFTL(cmd, 24, 8);                                                        \
        _g->words.w1 = 0;                                                                          \
    }

#define gSPSetOtherMode(pkt, cmd, sft, len, data)                                                  \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = (_SHIFTL(cmd, 24, 8) | _SHIFTL(sft, 8, 8) | _SHIFTL(len, 0, 8));            \
        _g->words.w1 = (unsigned int)(data);                                                       \
    }

#define gDPSetCombine(pkt, muxs0, muxs1)                                                           \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = _SHIFTL(G_SETCOMBINE, 24, 8) | _SHIFTL(muxs0, 0, 24);                       \
        _g->words.w1 = (unsigned int)(muxs1);                                                      \
    }

#define gDPSetColor(pkt, c, d)                                                                     \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = _SHIFTL(c, 24, 8);                                                          \
        _g->words.w1 = (unsigned int)(d);                                                          \
    }

#define gDPSetPrimDepth(pkt, z, dz)                                                                \
    gDPSetColor(pkt, G_SETPRIMDEPTH, _SHIFTL(z, 16, 16) | _SHIFTL(dz, 0, 16))

#define gDPSetPrimColor(pkt, m, l, r, g, b, a)                                                     \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = (_SHIFTL(G_SETPRIMCOLOR, 24, 8) | _SHIFTL(m, 8, 8) | _SHIFTL(l, 0, 8));     \
        _g->words.w1 =                                                                             \
            (_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8));         \
    }

#define gDPFillRectangle(pkt, ulx, uly, lrx, lry)                                                  \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 =                                                                             \
            (_SHIFTL(G_FILLRECT, 24, 8) | _SHIFTL((lrx), 14, 10) | _SHIFTL((lry), 2, 10));         \
        _g->words.w1 = (_SHIFTL((ulx), 14, 10) | _SHIFTL((uly), 2, 10));                           \
    }

#define gDPSetScissor(pkt, mode, ulx, uly, lrx, lry)                                               \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = _SHIFTL(G_SETSCISSOR, 24, 8) |                                              \
                       _SHIFTL((int)((float)(ulx) * 4.0f), 12, 12) |                               \
                       _SHIFTL((int)((float)(uly) * 4.0f), 0, 12);                                 \
        _g->words.w1 = _SHIFTL(mode, 24, 2) | _SHIFTL((int)((float)(lrx) * 4.0f), 12, 12) |        \
                       _SHIFTL((int)((float)(lry) * 4.0f), 0, 12);                                 \
    }

#define gDPPipeSync(pkt) gDPNoParam(pkt, G_RDPPIPESYNC)
#define gDPSetFillColor(pkt, d) gDPSetColor(pkt, G_SETFILLCOLOR, (d))
#define gSetImage(pkt, cmd, fmt, siz, width, i)                                                    \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = _SHIFTL(cmd, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) |           \
                       _SHIFTL((width) - 1, 0, 12);                                                \
        _g->words.w1 = (unsigned int)(i);                                                          \
    }
#define gDPSetColorImage(pkt, f, s, w, i) gSetImage(pkt, G_SETCIMG, f, s, w, i)
#define gDPSetTextureImage(pkt, f, s, w, i) gSetImage(pkt, G_SETTIMG, f, s, w, i)
#define gDPLoadSync(pkt) gDPNoParam(pkt, G_RDPLOADSYNC)
#define gDPTileSync(pkt) gDPNoParam(pkt, G_RDPTILESYNC)
#define gDPSetTile(                                                                                \
    pkt, fmt, siz, line, tmem, tile, palette, cmt, maskt, shiftt, cms, masks, shifts)              \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = _SHIFTL(G_SETTILE, 24, 8) | _SHIFTL(fmt, 21, 3) | _SHIFTL(siz, 19, 2) |     \
                       _SHIFTL(line, 9, 9) | _SHIFTL(tmem, 0, 9);                                  \
        _g->words.w1 = _SHIFTL(tile, 24, 3) | _SHIFTL(palette, 20, 4) | _SHIFTL(cmt, 18, 2) |      \
                       _SHIFTL(maskt, 14, 4) | _SHIFTL(shiftt, 10, 4) | _SHIFTL(cms, 8, 2) |       \
                       _SHIFTL(masks, 4, 4) | _SHIFTL(shifts, 0, 4);                               \
    }
#define gDPLoadBlock(pkt, tile, uls, ult, lrs, dxt)                                                \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = (_SHIFTL(G_LOADBLOCK, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12)); \
        _g->words.w1 = (_SHIFTL(tile, 24, 3) | _SHIFTL((MIN(lrs, G_TX_LDBLK_MAX_TXL)), 12, 12) |   \
                        _SHIFTL(dxt, 0, 12));                                                      \
    }
#define gDPLoadTLUTCmd(pkt, tile, count)                                                           \
    {                                                                                              \
        Gfx *_g = (Gfx *)pkt;                                                                      \
                                                                                                   \
        _g->words.w0 = _SHIFTL(G_LOADTLUT, 24, 8);                                                 \
        _g->words.w1 = _SHIFTL((tile), 24, 3) | _SHIFTL((count), 14, 10);                          \
    }
#define gDPLoadTileGeneric(pkt, c, tile, uls, ult, lrs, lrt)                                       \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = _SHIFTL(c, 24, 8) | _SHIFTL(uls, 12, 12) | _SHIFTL(ult, 0, 12);             \
        _g->words.w1 = _SHIFTL(tile, 24, 3) | _SHIFTL(lrs, 12, 12) | _SHIFTL(lrt, 0, 12);          \
    }
#define gDPSetTileSize(pkt, t, uls, ult, lrs, lrt)                                                 \
    gDPLoadTileGeneric(pkt, G_SETTILESIZE, t, uls, ult, lrs, lrt)
#define gSPTexture(pkt, s, t, level, tile, on)                                                     \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = (_SHIFTL(G_TEXTURE, 24, 8) | _SHIFTL((level), 11, 3) |                      \
                        _SHIFTL((tile), 8, 3) | _SHIFTL((on), 0, 8));                              \
        _g->words.w1 = (_SHIFTL((s), 16, 16) | _SHIFTL((t), 0, 16));                               \
    }
#define gImmp1(pkt, c, p0)                                                                         \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = _SHIFTL(c, 24, 8);                                                          \
        _g->words.w1 = (unsigned int)(p0);                                                         \
    }
#define gDPSetCycleType(pkt, type)                                                                 \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_CYCLETYPE, 2, type)
#define gDPSetColorDither(pkt, mode)                                                               \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_RGBDITHER, 2, mode)
#define gDPSetRenderMode(pkt, c0, c1)                                                              \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_L, G_MDSFT_RENDERMODE, 29, (c0) | (c1))

/* one command given as its two words (a machine-converted draft; named
 * macros replace these as each command is identified) */
#define gRaw(pkt, a, b)                                                                            \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = (unsigned int)(a);                                                          \
        _g->words.w1 = (unsigned int)(b);                                                          \
    }

#define gSPMatrix(pkt, m, p) gDma1p(pkt, G_MTX, m, sizeof(Mtx), p)
#define gSPViewport(pkt, v) gDma1p((pkt), G_MOVEMEM, (v), sizeof(Vp), G_MV_VIEWPORT)
#define gSPDisplayList(pkt, dl) gDma1p(pkt, G_DL, dl, 0, G_DL_PUSH)
#define gSPPerspNormalize(pkt, s) gMoveWd(pkt, G_MW_PERSPNORM, 0, (s))

#define G_SETGEOMETRYMODE 0xb7
#define G_CLEARGEOMETRYMODE 0xb6
#define G_POPMTX 0xbd
#define G_SETBLENDCOLOR 0xf9
#define G_MW_LIGHTCOL 0x0a
#define G_MV_LOOKATY 0x82
#define G_MV_LOOKATX 0x84
#define G_MDSFT_ALPHACOMPARE 0
#define G_MDSFT_ALPHADITHER 4
#define G_MDSFT_TEXTFILT 12
#define G_MDSFT_TEXTLUT 14
#define G_MDSFT_TEXTLOD 16
#define G_MDSFT_TEXTDETAIL 17

#define gSPSetGeometryMode(pkt, word) gImmp1(pkt, G_SETGEOMETRYMODE, word)
#define gSPClearGeometryMode(pkt, word) gImmp1(pkt, G_CLEARGEOMETRYMODE, word)
#define gSPPopMatrix(pkt, n) gImmp1(pkt, G_POPMTX, n)
#define gDPSetBlendColor(pkt, r, g, b, a)                                                          \
    gDPSetColor(pkt,                                                                               \
                G_SETBLENDCOLOR,                                                                   \
                (_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8)))
/* a light's colour goes to both of its copies (0x20 bytes per light) */
#define gSPLightColor(pkt, n, col)                                                                 \
    {                                                                                              \
        gMoveWd(pkt, G_MW_LIGHTCOL, ((n) - 1) * 0x20, col);                                        \
        gMoveWd(pkt, G_MW_LIGHTCOL, ((n) - 1) * 0x20 + 4, col);                                    \
    }
#define gSPLookAtX(pkt, l) gDma1p(pkt, G_MOVEMEM, l, 16, G_MV_LOOKATX)
#define gSPLookAtY(pkt, l) gDma1p(pkt, G_MOVEMEM, l, 16, G_MV_LOOKATY)
#define gDPSetAlphaCompare(pkt, type)                                                              \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_L, G_MDSFT_ALPHACOMPARE, 2, type)
#define gDPSetAlphaDither(pkt, mode)                                                               \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_ALPHADITHER, 2, mode)
#define gDPSetTextureFilter(pkt, type)                                                             \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTFILT, 2, type)
#define gDPSetTextureLUT(pkt, type) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTLUT, 2, type)
#define gDPSetTextureLOD(pkt, type) gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTLOD, 1, type)
#define gDPSetTextureDetail(pkt, type)                                                             \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTDETAIL, 2, type)

#define G_MW_NUMLIGHT 0x02
#define G_MV_L0 0x86
#define NUMLIGHTS(n) (0x80000000 | (((n) + 1) * 32))
#define gSPNumLights(pkt, n) gMoveWd(pkt, G_MW_NUMLIGHT, 0, NUMLIGHTS(n))
#define gSPLight(pkt, l, n) gDma1p(pkt, G_MOVEMEM, l, 16, ((n) - 1) * 2 + G_MV_L0)

#define G_MW_FOG 0x08
#define G_SETFOGCOLOR 0xf8
#define gSPFogFactor(pkt, fm, fo)                                                                  \
    gMoveWd(pkt, G_MW_FOG, 0, (_SHIFTL(fm, 16, 16) | _SHIFTL(fo, 0, 16)))
#define gSPFogPosition(pkt, min, max)                                                              \
    gSPFogFactor(pkt, (500 * 0x100) / ((max) - (min)), (500 - (min)) * 0x100 / ((max) - (min)))
#define gDPSetFogColor(pkt, r, g, b, a)                                                            \
    gDPSetColor(pkt,                                                                               \
                G_SETFOGCOLOR,                                                                     \
                (_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8)))

#define G_MW_CLIP 0x04
#define G_SETENVCOLOR 0xfb
#define gSPClipRatio1(pkt)                                                                         \
    {                                                                                              \
        gMoveWd(pkt, G_MW_CLIP, 0x04, 1);                                                          \
        gMoveWd(pkt, G_MW_CLIP, 0x0c, 1);                                                          \
        gMoveWd(pkt, G_MW_CLIP, 0x14, 0xffff);                                                     \
        gMoveWd(pkt, G_MW_CLIP, 0x1c, 0xffff);                                                     \
    }
#define gSPClipRatio(pkt, r)                                                                       \
    {                                                                                              \
        gMoveWd(pkt, G_MW_CLIP, 0x04, r);                                                          \
        gMoveWd(pkt, G_MW_CLIP, 0x0c, r);                                                          \
        gMoveWd(pkt, G_MW_CLIP, 0x14, (unsigned short)-(r));                                       \
        gMoveWd(pkt, G_MW_CLIP, 0x1c, (unsigned short)-(r));                                       \
    }
#define gDPSetEnvColor(pkt, r, g, b, a)                                                            \
    gDPSetColor(pkt,                                                                               \
                G_SETENVCOLOR,                                                                     \
                (_SHIFTL(r, 24, 8) | _SHIFTL(g, 16, 8) | _SHIFTL(b, 8, 8) | _SHIFTL(a, 0, 8)))

#define G_LOADTILE 0xf4
#define G_IM_FMT_IA 3
#define G_IM_SIZ_8b 1
#define G_MDSFT_TEXTPERSP 19
#define G_TP_NONE (0 << G_MDSFT_TEXTPERSP)
#define G_TP_PERSP (1 << G_MDSFT_TEXTPERSP)
#define G_CYC_2CYCLE (1 << G_MDSFT_CYCLETYPE)
#define G_RM_PASS 0x0c080000
#define G_RM_XLU_SURF2 0x00104240
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#define gDPSetTexturePersp(pkt, type)                                                              \
    gSPSetOtherMode(pkt, G_SETOTHERMODE_H, G_MDSFT_TEXTPERSP, 1, type)
#define gDPLoadTile(pkt, t, uls, ult, lrs, lrt)                                                    \
    gDPLoadTileGeneric(pkt, G_LOADTILE, t, uls, ult, lrs, lrt)
#define gSPTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy)                           \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(xh, 12, 12) | _SHIFTL(yh, 0, 12));     \
        _g->words.w1 = (_SHIFTL(tile, 24, 3) | _SHIFTL(xl, 12, 12) | _SHIFTL(yl, 0, 12));          \
        gImmp1(pkt, G_RDPHALF_1, (_SHIFTL(s, 16, 16) | _SHIFTL(t, 0, 16)));                        \
        gImmp1(pkt, G_RDPHALF_2, (_SHIFTL(dsdx, 16, 16) | _SHIFTL(dtdy, 0, 16)));                  \
    }
/* the rectangle clamped at the top-left edges, its texture coordinates
 * advanced by what the clamp cut off */
#define gSPScisTextureRectangle(pkt, xl, yl, xh, yh, tile, s, t, dsdx, dtdy)                       \
    {                                                                                              \
        Gfx *_g = (Gfx *)(pkt);                                                                    \
                                                                                                   \
        _g->words.w0 = (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL(MAX((s16)(xh), 0), 12, 12) |           \
                        _SHIFTL(MAX((s16)(yh), 0), 0, 12));                                        \
        _g->words.w1 = (_SHIFTL((tile), 24, 3) | _SHIFTL(MAX((s16)(xl), 0), 12, 12) |              \
                        _SHIFTL(MAX((s16)(yl), 0), 0, 12));                                        \
        gImmp1(                                                                                    \
            pkt,                                                                                   \
            G_RDPHALF_1,                                                                           \
            (_SHIFTL(((s) - (((s16)(xl) < 0) ? (((s16)(dsdx) < 0) ?                                \
                                                    (MAX((((s16)(xl) * (s16)(dsdx)) >> 7), 0)) :   \
                                                    (MIN((((s16)(xl) * (s16)(dsdx)) >> 7), 0))) :  \
                                               0)),                                                \
                     16,                                                                           \
                     16) |                                                                         \
             _SHIFTL(((t) - (((yl) < 0) ? (((s16)(dtdy) < 0) ?                                     \
                                               (MAX((((s16)(yl) * (s16)(dtdy)) >> 7), 0)) :        \
                                               (MIN((((s16)(yl) * (s16)(dtdy)) >> 7), 0))) :       \
                                          0)),                                                     \
                     0,                                                                            \
                     16)));                                                                        \
        gImmp1(pkt, G_RDPHALF_2, (_SHIFTL((dsdx), 16, 16) | _SHIFTL((dtdy), 0, 16)));              \
    }

typedef struct N64Vec3 {
    float x, y, z;
} N64Vec3;

float sqrtf(float x);

typedef struct N64Season {
    int state;                 /* 0x00 */
    unsigned char round;       /* 0x04 */
    unsigned char race;        /* 0x05 */
    unsigned char place[6][4]; /* 0x06  finishing position per round and race */
    unsigned short points[7];  /* 0x1E  points per round */
    float times[6][4];         /* 0x2C  lap clock per round and race */
    float x8c[16];             /* 0x8C  one per car */
    unsigned short unlocked;   /* 0xCC  bit n: car n may be picked */
    unsigned short xce;        /* 0xCE */
    unsigned short xd0;        /* 0xD0 */
    short padd2;
    int xd4;       /* 0xD4 */
    int xd8;       /* 0xD8 */
    int xdc;       /* 0xDC */
    int xe0;       /* 0xE0 */
    int xe4;       /* 0xE4 */
    float xe8[16]; /* 0xE8  one per car */
} N64Season;

/* One championship round's rules (0x1C bytes), a table at 0x8028B944. */
typedef struct N64Round {
    int x0;                    /* 0x00 */
    int x4;                    /* 0x04 */
    int x8;                    /* 0x08 */
    int xc;                    /* 0x0C */
    unsigned char kinds[2];    /* 0x10  car kind for the first cars */
    unsigned short kindMask;   /* 0x12  bit n: kind n allowed */
    unsigned char races[4][2]; /* 0x14  per race: track, weather */
} N64Round;

extern N64Round g_SeasonTable[];

/* A rigid-body state (0x44 bytes): the car keeps three (current, and two
 * copies the integrator steps between). */
typedef struct N64RbState {
    N64Vec3 pos;    /* 0x00 */
    N64Vec3 vel;    /* 0x0C */
    float q[4];     /* 0x18  orientation */
    N64Vec3 angVel; /* 0x28 */
    char pad34[0x10];
} N64RbState;

/* A car's camera: its matrix and field of view (0x44 bytes). */
typedef struct N64CarCam {
    float mtx[4][4];
    float fov; /* 0x40  radians */
} N64CarCam;

/* A car kind's handling numbers (0x4C bytes each), a table at 0x8028B330. */
typedef struct N64CarKindParams {
    char x00[0x1c]; /* copied whole to the car at 0xDF8 */
    float x1c[5];   /* to 0xE14.. */
    int x30[2];     /* to 0xE28.. */
    float x38[4];   /* to 0x324.. */
    int x48;        /* to 0xE34 */
} N64CarKindParams;

/* The record a car's +0xED0 points at; only its flags are known. */
typedef struct N64CarLink {
    char pad00[0x68];
    unsigned int flags; /* 0x68  bits 0-1: the car is out of the race */
    char pad6c[0x74 - 0x6C];
    int x74; /* 0x74  the entrant's row in the AI pace table */
} N64CarLink;

/* A body of the car (0x208 bytes): the car itself is one, at 0x1C0 (its
 * fields named on N64Car), and each wheel another. */
typedef struct N64CarBody {
    char pad000[0x74];
    float pos[3]; /* 0x74  where it hangs off the car body */
    char pad080[0x128 - 0x80];
    unsigned char surface; /* 0x128  the surface under it (read when x13c is set) */
    char pad129[0x13C - 0x129];
    int x13c; /* 0x13C */
    char pad140[0x148 - 0x140];
    float steer; /* 0x148  radians */
    char pad14c[0x15C - 0x14C];
    float spin; /* 0x15C  degrees */
    char pad160[0x208 - 0x160];
} N64CarBody;

/* A skid-mark vertex (an F3DEX Vtx, 16 bytes) and point (0x18 bytes):
 * a point's half-width is drawn while non-zero. */
typedef struct N64SkidVtx {
    short ob[3];
    unsigned short flag;
    short tc[2];
    unsigned char cn[4];
} N64SkidVtx;
typedef struct N64SkidPt {
    short pos[3];
    short half[3]; /* 0x06 */
    char pad0c[0x18 - 0x0C];
} N64SkidPt;

typedef struct N64Car {
    float mtx0[4][4];        /* 0x000  body matrix */
    float wheelMtx[4][4][4]; /* 0x040  one per wheel: the body's rotation, the wheel's position */
    int slot;                /* 0x140  index in the car array */
    short sndHits;           /* 0x144  impact sounds in a row (to 16) */
    char pad146[0x14C - 0x146];
    struct N64CarWheel *wheel[4]; /* 0x14C  the wheels' rigid bodies */
    char pad15c[0x1C0 - 0x15C];
    N64RbState st;     /* 0x1C0  the body's state */
    float stMtx[4][4]; /* 0x204  st's orientation as a matrix */
    char pad244[0x25C - 0x244];
    N64RbState stA; /* 0x25C */
    N64RbState stB; /* 0x2A0 */
    char pad2e4[0x31C - 0x2E4];
    float spin; /* 0x31C  the body's roll angle, degrees */
    char pad320[0x324 - 0x320];
    float x324[4];         /* 0x324  from the kind table */
    N64Vec3 x334;          /* 0x334  where the HUD arrow points */
    int x340;              /* 0x340  negative: the car is off the track */
    unsigned char x344;    /* 0x344  a pending HUD arrow (0 = none) */
    unsigned char x345;    /* 0x345 */
    unsigned char sndHitA; /* 0x346  pending one-shot sounds: loudness, 0 none */
    unsigned char sndHitB; /* 0x347 */
    char pad348;
    unsigned char hitAge;    /* 0x349  frames since the last car-to-car hit */
    unsigned char x34a;      /* 0x34A */
    unsigned char sndHitC;   /* 0x34B */
    unsigned char sndImpact; /* 0x34C  the surface impact level */
    char pad34d[0x3C8 - 0x34D];
    N64CarBody wheels[4]; /* 0x3C8  the wheels' bodies (wheel[] points at them) */
    char padbe8[0xDF0 - 0xBE8];
    float xdf0;      /* 0xDF0 */
    float xdf4;      /* 0xDF4  scales the exhaust smoke (0.001 per unit) */
    char xdf8[0x1c]; /* 0xDF8  from the kind table */
    float xe14;      /* 0xE14  from the kind table: the torque curve's rpm^3 term */
    float xe18;      /* 0xE18  rpm^2 term */
    float xe1c;      /* 0xE1C  rpm term */
    float xe20;      /* 0xE20  constant term */
    float xe24;      /* 0xE24  the wheel-to-engine rpm scale */
    int xe28[2];     /* 0xE28  from the kind table */
    int xe30;        /* 0xE30 */
    int xe34;        /* 0xE34  from the kind table */
    float xe38;      /* 0xE38 */
    float xe3c;      /* 0xE3C */
    int xe40;        /* 0xE40 */
    char pade44[0xE58 - 0xE44];
    int xe58;          /* 0xE58 */
    N64Season *season; /* 0xE5C  the player's season, 0 for others */
    int xe60;          /* 0xE60  from the season (or the ghost's header) */
    int xe64;          /* 0xE64 */
    int xe68;          /* 0xE68 */
    int xe6c;          /* 0xE6C */
    int xe70[4];       /* 0xE70 */
    char pade80[0xED0 - 0xE80];
    struct N64CarLink *link;   /* 0xED0 */
    int xed4;                  /* 0xED4  a countdown, one per frame */
    int xed8;                  /* 0xED8  the car's control function (camera step, AI) */
    N64Vec3 aim;               /* 0xEDC  the AI's smoothed aim point */
    N64Vec3 linePos;           /* 0xEE8  the nearest point on the racing line */
    N64Vec3 lineDir;           /* 0xEF4  the line's direction there */
    N64Vec3 lineSide;          /* 0xF00  across the line */
    N64Vec3 lineUp;            /* 0xF0C */
    float lineOffAbs;          /* 0xF18  |lineOff| */
    N64Vec3 lineRel;           /* 0xF1C  the car relative to linePos */
    float lineOff;             /* 0xF28  the car's signed distance across the line */
    N64Vec3 posStart;          /* 0xF2C  where the race started */
    int xf38;                  /* 0xF38 */
    int xf3c;                  /* 0xF3C */
    int xf40;                  /* 0xF40 */
    float xf44;                /* 0xF44 */
    int xf48;                  /* 0xF48  camera mode */
    int xf4c;                  /* 0xF4C  the camera keeps the car's up axis */
    N64Vec3 posPrev;           /* 0xF50  last frame's position */
    int xf5c;                  /* 0xF5C  the path segment the car is on */
    int xf60;                  /* 0xF60  and the point in it */
    float xf64;                /* 0xF64  the heading's atan2 arguments at the restart point */
    float xf68;                /* 0xF68 */
    float xf6c;                /* 0xF6C */
    int xf70;                  /* 0xF70 */
    int xf74;                  /* 0xF74  -1 for a player's car */
    int laps;                  /* 0xF78  laps completed */
    int xf7c;                  /* 0xF7C  -1 for a player's car */
    float raceTime;            /* 0xF80  race clock, seconds */
    float lapTimes[5];         /* 0xF84  each lap's time */
    float xf98;                /* 0xF98 */
    int xf9c;                  /* 0xF9C */
    float lapTime;             /* 0xFA0  current lap clock */
    float xfa4;                /* 0xFA4  a countdown (mode 1 only) */
    float xfa8;                /* 0xFA8  grid row distance back, the ranking key */
    int xfac;                  /* 0xFAC */
    char *msgA;                /* 0xFB0  first message and its timer */
    float msgATime;            /* 0xFB4 */
    char *msgB;                /* 0xFB8  second message and its timer */
    float msgBTime;            /* 0xFBC */
    char xfc0[0x14];           /* 0xFC0  text for msgB (a formatted time) */
    int xfd4;                  /* 0xFD4 */
    N64Vec3 velfd8;            /* 0xFD8  another velocity copy */
    float xfe4[11];            /* 0xFE4 */
    float x1010;               /* 0x1010  zeroed when the particle pool is reset */
    N64Vec3 smokeAt;           /* 0x1014  where the last smoke particle started */
    float sprayTime[4];        /* 0x1020  per wheel: the spray emit timer */
    N64Vec3 sprayAt[4];        /* 0x1030  per wheel: where its last spray particle started */
    float skidLife[4];         /* 0x1060  per wheel: frames the mark lasts off the ground */
    int skidSurf[4];           /* 0x1070  the surface it was last on */
    float skidClock[4];        /* 0x1080  lays a mark point past 0.75 */
    int skidEmit[4];           /* 0x1090  a point was laid this frame */
    N64Vec3 skidAt[4];         /* 0x10A0  where the last point went */
    N64SkidVtx skidVtx[4][36]; /* 0x10D0  each wheel's trail, newest first */
    N64SkidPt skidPt[4][9];    /* 0x19D0 */
    short skidKind[4][9];      /* 0x1D30  each point's surface */
    N64Vec3 pos1d78;           /* 0x1D78  another position copy */
    char pad1d84[0x1D88 - 0x1D84];
    int mtx[16]; /* 0x1D88 */
    char pad1dc8[0x1DCC - 0x1DC8];
    float heading; /* 0x1DCC  of the camera, radians */
    char pad1dd0[0x1DD4 - 0x1DD0];
    float x1dd4;       /* 0x1DD4  rotation rates while free-flying */
    float x1dd8;       /* 0x1DD8 */
    float x1ddc;       /* 0x1DDC  speed while free-flying */
    float x1de0;       /* 0x1DE0 */
    float fog;         /* 0x1DE4  fog amount at the car */
    N64CarCam *cam;    /* 0x1DE8  the camera in use */
    N64CarCam *cam2;   /* 0x1DEC  the camera it switches back to */
    N64CarCam cams[4]; /* 0x1DF0 */
    char pad1f00[0x1F44 - 0x1F00];
    N64CarCam cam4;    /* 0x1F44 */
    float camSpeed;    /* 0x1F88  the chase camera's smoothed speed */
    float camSpin;     /* 0x1F8C  and spin */
    float x1f90;       /* 0x1F90 */
    N64Vec3 camTarget; /* 0x1F94  where the camera looks */
    N64Vec3 camPosA;   /* 0x1FA0  copies of the chase camera's start position */
    float x1fac;       /* 0x1FAC */
    char pad1fb0[0x1FB4 - 0x1FB0];
    N64Vec3 camPosB;          /* 0x1FB4 */
    unsigned short x1fc0[32]; /* 0x1FC0  trigger ids the car has passed */
    int x2000;                /* 0x2000  entries in x1fc0 */
    int x2004[16];            /* 0x2004 */
    int x2044;                /* 0x2044 */
    float x2048;              /* 0x2048  lifts the spray particles (half of it) */
    int x204c;                /* 0x204C */
    char pad2050[0x2058 - 0x2050];
    int x2058;               /* 0x2058  the kind it was given (copied to kind) */
    int kind;                /* 0x205C */
    unsigned char colour[4]; /* 0x2060  body colour r, g, b and a fourth byte */
    float x2064;             /* 0x2064 */
    int x2068;               /* 0x2068 */
    int wrongWay;            /* 0x206C  frames spent facing backwards */
    unsigned char cellX;     /* 0x2070  the 32-unit track grid cell it is in (0..63) */
    unsigned char cellY;     /* 0x2071 */
    char pad2072[0x2074 - 0x2072];
    unsigned int *pad; /* 0x2074  the slot's pad record */
    char *model;       /* 0x2078  the slot's model buffer */
    short dent[8];     /* 0x207C  how far each side of the body has been pushed in */
    short dentWobble;  /* 0x208C  cycles -4..3, offsets the dent's vertex sample */
    char pad208e[0x2090 - 0x208E];
} N64Car;

extern N64Car g_aVehicles[4];

/* A car's loaded model (the head of its model buffer). */
typedef struct N64CarModelPart {
    void *a;
    unsigned short *b; /* 0x04  palette (RGBA5551) for a paletted texture */
    char pad08[0x18];
    unsigned char fmt; /* 0x20  low nibble 1: paletted */
    char pad21[3];
} N64CarModelPart;
typedef struct N64CarModel {
    char pad00[0x10];
    int nParts;              /* 0x10 */
    N64CarModelPart *parts;  /* 0x14 */
    unsigned int *dl[3][10]; /* 0x18 */
    void *x90;
    void *x94;
    char pad98[0xbc - 0x98];
    unsigned int *dl2[3][3]; /* 0xBC  wheel display lists (0 when the model has none) */
    float wheel[4][3];       /* 0xE0  wheel positions in the body's frame */
    unsigned char
        decalPart[12]; /* 0x110  six paint slots, two parts each; [2] carries the body paint */
    void **x11c;       /* 0x11C */
} N64CarModel;

/* A car's model file (0x60 bytes each), a table at 0x8028AE0C. */
typedef struct N64CarModelRec {
    char pad00[0x14];
    int rom;           /* 0x14  ROM address of the model file */
    int present;       /* 0x18  non-zero when the car is in this build */
    unsigned int size; /* 0x1C  bytes, set when it is loaded */
    int dialRom;       /* 0x20  ROM address of the dashboard art: the dial
                          face, then its 0x200-byte palettes (one per
                          weather) from +0x400... */
    char pad24[4];
    unsigned char dialW;    /* 0x28  the rev counter's size */
    unsigned char dialH;    /* 0x29 */
    signed char lampX;      /* 0x2A  the rev lamps' offset in the dial */
    signed char lampY;      /* 0x2B */
    unsigned char lampW;    /* 0x2C  and size (frames follow the dial face) */
    unsigned char lampH;    /* 0x2D */
    unsigned char dialMode; /* 0x2E  0 a needle; 1, 2 a palette rev bar */
    unsigned char needleX;  /* 0x2F  the needle's centre in the dial; for a */
    unsigned char needleY;  /* 0x30  bar, needleX is its length in colours */
    char pad31[3];
    float needleMax;  /* 0x34  needle angle at full revs */
    float needleRest; /* 0x38  needle angle at rest */
    char pad3c[0x58 - 0x3C];
    float len; /* 0x58  body length, 1/256 units */
    float wid; /* 0x5C  body half-width */
} N64CarModelRec;

extern N64CarModelRec g_aCarModels[];

typedef struct N64PadRec {
    unsigned int pressed; /* 0x00  buttons that went down this frame */
    unsigned int held;    /* 0x04  buttons already acted on */
    int repeat[4];        /* 0x08  stick auto-repeat timers */
    float axis[2];        /* 0x18  stick axes */
    char pad20[5];
    unsigned char x25; /* 0x25  from the season (or the ghost's header) */
    char pad26[2];
    int absent;            /* 0x28  non-zero while no controller answers on this port */
    unsigned char *rec[2]; /* 0x2C  the lap being recorded, per player */
    int recLen[2];         /* 0x34  bytes recorded so far */
    int recKeep[2];        /* 0x3C  recLen kept when a replay starts */
    unsigned char *ghost;  /* 0x44  the recording being played back, 0 for none */
    int ghostPos;          /* 0x48  bytes played */
    int ghostLen;          /* 0x4C  bytes in it */
    char pad50[0x154 - 0x50];
    int index;  /* 0x154 */
    void *cont; /* 0x158  its OSContPad */
} N64PadRec;

extern N64PadRec g_aPlayerInputState[4];

/* One row of a menu screen (20 bytes, static in .data).  A screen is a
 * NULL-terminated array of MenuItem pointers. */
typedef struct MenuItem {
    const char *label;         /* 0x00  may carry %XY colour codes */
    unsigned int flags;        /* 0x04 */
    void *icon;                /* 0x08  decompressed icon, 0 in ROM */
    unsigned int iconRomStart; /* 0x0C */
    unsigned int iconRomEnd;   /* 0x10 */
} MenuItem;

/* A track (0x17C bytes), a table at 0x80270854; its menu row comes first. */
typedef struct N64Track {
    MenuItem item; /* 0x00 */
    int x14;       /* 0x14  ROM offset of the track data */
    int present;   /* 0x18  non-zero when the track's data is there */
    int x1c;       /* 0x1C  ROM offset of a 0x20-byte header + packed data (0x803736B0) */
    int x20;
    int x24; /* 0x24  the same for 0x80373ED0 */
    char pad28[0x17C - 0x28];
} N64Track;

extern N64Track g_aTrackDescriptors[];

#endif
