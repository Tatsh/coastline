#include "common/texrect.h"

#include <PR/gbi.h>

#include "common/render.h"
#include "common/system.h"

/* One texel in the 5.10 fixed point of a texture coordinate step. */
#define TEXEL_STEP 0x400

/* The red of the tinted rectangle. */
#define TINT_RED 0xFF
#define TINT_GREEN 0x58
#define TINT_BLUE 0x58

#define TEXRECT_MAX(a, b) ((a) > (b) ? (a) : (b))
#define TEXRECT_TXL2WORDS(texels, bytes) TEXRECT_MAX(1, ((texels) * (bytes) / 8))
#define TEXRECT_TXL2WORDS_4B(texels) TEXRECT_MAX(1, ((texels) / 16))
#define TEXRECT_CALC_DXT(width, bytes)                                                             \
    (((1 << 11) + TEXRECT_TXL2WORDS(width, bytes) - 1) / (unsigned)TEXRECT_TXL2WORDS(width, bytes))
#define TEXRECT_CALC_DXT_4B(width)                                                                 \
    (((1 << 11) + TEXRECT_TXL2WORDS_4B(width) - 1) / (unsigned)TEXRECT_TXL2WORDS_4B(width))
#define TEXRECT_COMBINE_WORDS(command, high, low)                                                  \
    gDisplayListWords(command, 0xFC000000 | (high), low)

void AllocAndDmaLoadAsset(Texture *texture, AssetAllocator alloc) {
    texture->pixels = alloc(texture->romEnd - texture->romStart);
    DmaRead(texture->pixels, texture->romStart, texture->romEnd - texture->romStart);
}

void LoadAndDecompressAsset(Texture *texture, AssetAllocator alloc) {
    texture->pixels = alloc(DmaReadAssetSize(texture->romStart));
    DecompressFromRom(texture->pixels, texture->romStart, NULL);
}

void DrawTexturedRect(Texture *img,
                      int x,
                      int y,
                      int w,
                      int h,
                      int pr,
                      int pg,
                      int pb,
                      int pa,
                      int er,
                      int eg,
                      int eb,
                      int ea,
                      int mode) {
    int bitsS;
    int bitsT;
    int maskS;
    int maskT;
#ifdef __sgi
    /* IDO reserves this unused word in the original stack frame. */
    int u0[1];
#endif
    int tpw;
    int dsdx;
    int dtdy;
    int lines;
    int left;
    int line;
    int row;
    int step;
    int yb;

    if (w < 0) {
        w = -w;
    }
    GetTextureScaleAndMask(img->width, &maskS, &bitsS);
    GetTextureScaleAndMask(img->height, &maskT, &bitsT);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);
        _g->words.w0 = 0xbb000001;
        _g->words.w1 = 0xffffffff;
    }
    gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 12, 2, g_dwTextureFilterMode);
    switch (mode) {
    case 1:
        TEXRECT_COMBINE_WORDS(gDisplayListPtr++, 0xffffff, 0xfffdf2f9);
        gDPSetPrimColor(gDisplayListPtr++, 0xff, 0xff, pr, pg, pb, pa);
        gDPSetRenderMode(gDisplayListPtr++, 0x00504240, 0);
        break;
    case 2:
        TEXRECT_COMBINE_WORDS(gDisplayListPtr++, 0xffffff, 0xfffcf279);
        gDPSetRenderMode(gDisplayListPtr++, 0x00504240, 0);
        break;
    case 3:
        TEXRECT_COMBINE_WORDS(gDisplayListPtr++, 0x50fea1, 0x33fdf2f9);
        gDPSetPrimColor(gDisplayListPtr++, 0xff, 0xff, pr, pg, pb, pa);
        gDPSetEnvColor(gDisplayListPtr++, er, eg, eb, ea);
        gDPSetRenderMode(gDisplayListPtr++, 0x00504240, 0);
        break;
    case 4:
        TEXRECT_COMBINE_WORDS(gDisplayListPtr++, 0x5098a1, 0x3331feff);
        gDPSetPrimColor(gDisplayListPtr++, 0xff, 0xff, pr, pg, pb, pa);
        gDPSetEnvColor(gDisplayListPtr++, er, eg, eb, ea);
        gDPSetRenderMode(gDisplayListPtr++, 0x0f0a4000, 0);
        break;
    case 0:
    default:
        TEXRECT_COMBINE_WORDS(gDisplayListPtr++, 0xffffff, 0xfffcf279);
        gDPSetRenderMode(gDisplayListPtr++, 0x00504240, 0);
        break;
    }
    if (img->format == 2) {
        gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 14, 2, 0x8000);
    } else {
        gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 14, 2, 0);
    }
    gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 19, 1, 0);
    if (g_dwScreenScaleShift != 0) {
        x <<= 1;
        y <<= 1;
        w <<= 1;
        h <<= 1;
    }
    switch (img->size) {
    case 0:
        tpw = 16;
        break;
    case 1:
        tpw = 8;
        break;
    case 2:
        tpw = 4;
        break;
#ifdef ENABLE_PATCHES
    default:
        tpw = 2;
        break;
#endif
    }
    line = (img->width + tpw - 1) / tpw * 8;
    if (img->format == 2) {
        lines = 0x800 / line;
    } else {
        lines = 0x1000 / line;
    }
    left = img->height;
    dsdx = (img->width << 10) / w;
    dtdy = (img->height << 10) / h;
    lines -= 1;
    yb = ((y + h) << 2) << 8;
    row = 0;
    while (left != 0) {
        if (left < lines) {
            lines = left;
        }
        left -= lines;
        step = ((lines << 10) / dtdy) << 10;
        yb -= step;
        switch (img->size) {
        case 0:
            gDPSetTextureImage(
                gDisplayListPtr++, img->format, G_IM_SIZ_16b, 1, img->pixels + line * row);
            gDPSetTile(
                gDisplayListPtr++, img->format, G_IM_SIZ_16b, 0, 0, 7, 0, 2, bitsT, 0, 2, bitsS, 0);
            gDPLoadSync(gDisplayListPtr++);
            gDPLoadBlock(gDisplayListPtr++,
                         7,
                         0,
                         0,
                         ((img->width * (lines + 1) + 3) >> 2) - 1,
                         TEXRECT_CALC_DXT_4B(img->width));
            gDPPipeSync(gDisplayListPtr++);
            gDPSetTile(gDisplayListPtr++,
                       img->format,
                       0,
                       ((img->width >> 1) + 7) >> 3,
                       0,
                       0,
                       0,
                       2,
                       bitsT,
                       0,
                       2,
                       bitsS,
                       0);
            gDPSetTileSize(gDisplayListPtr++, 0, 0, 0, (img->width - 1) << 2, lines << 2);
            break;
        case 1:
            gDPSetTextureImage(
                gDisplayListPtr++, img->format, G_IM_SIZ_16b, 1, img->pixels + line * row);
            gDPSetTile(
                gDisplayListPtr++, img->format, G_IM_SIZ_16b, 0, 0, 7, 0, 2, bitsT, 0, 2, bitsS, 0);
            gDPLoadSync(gDisplayListPtr++);
            gDPLoadBlock(gDisplayListPtr++,
                         7,
                         0,
                         0,
                         ((img->width * (lines + 1) + 1) >> 1) - 1,
                         TEXRECT_CALC_DXT(img->width, 1));
            gDPPipeSync(gDisplayListPtr++);
            gDPSetTile(gDisplayListPtr++,
                       img->format,
                       1,
                       (img->width + 7) >> 3,
                       0,
                       0,
                       0,
                       2,
                       bitsT,
                       0,
                       2,
                       bitsS,
                       0);
            gDPSetTileSize(gDisplayListPtr++, 0, 0, 0, (img->width - 1) << 2, lines << 2);
            break;
        case 2:
            gDPSetTextureImage(
                gDisplayListPtr++, img->format, G_IM_SIZ_16b, 1, img->pixels + line * row);
            gDPSetTile(
                gDisplayListPtr++, img->format, G_IM_SIZ_16b, 0, 0, 7, 0, 2, bitsT, 0, 2, bitsS, 0);
            gDPLoadSync(gDisplayListPtr++);
            gDPLoadBlock(gDisplayListPtr++,
                         7,
                         0,
                         0,
                         img->width * (lines + 1) - 1,
                         TEXRECT_CALC_DXT(img->width, 2));
            gDPPipeSync(gDisplayListPtr++);
            gDPSetTile(gDisplayListPtr++,
                       img->format,
                       2,
                       (img->width * 2 + 7) >> 3,
                       0,
                       0,
                       0,
                       2,
                       bitsT,
                       0,
                       2,
                       bitsS,
                       0);
            gDPSetTileSize(gDisplayListPtr++, 0, 0, 0, (img->width - 1) << 2, lines << 2);
            break;
        }
        {
            Gfx *_g = (Gfx *)(gDisplayListPtr++);

            _g->words.w0 = (_SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL((x + w) << 2, 12, 12) |
                            _SHIFTL((yb + step) >> 8, 0, 12));
            _g->words.w1 = (_SHIFTL(0, 24, 3) | _SHIFTL(x << 2, 12, 12) | _SHIFTL(yb >> 8, 0, 12));
            gImmp1(gDisplayListPtr++,
                   G_RDPHALF_1,
                   (_SHIFTL(0, 16, 16) | _SHIFTL((lines - 1) << 5, 0, 16)));
            gImmp1(gDisplayListPtr++, G_RDPHALF_2, (_SHIFTL(dsdx, 16, 16) | _SHIFTL(-dtdy, 0, 16)));
        }
        row += lines;
    }
    gDPPipeSync(gDisplayListPtr++);
    gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 19, 1, 0x80000);
}

void DrawRedIntensityRect(
    u8 *texture, s32 width, s32 height, s32 x, s32 y, s32 drawWidth, s32 drawHeight) {
    s32 scaleS, scaleT, maskS, maskT;

    if (drawWidth < 0) {
        drawWidth = -drawWidth;
    }
    GetTextureScaleAndMask(width, &scaleS, &maskS);
    GetTextureScaleAndMask(height, &scaleT, &maskT);

    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gSPTexture(gDisplayListPtr++, scaleS, scaleT, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetCombineLERP(gDisplayListPtr++,
                      PRIMITIVE,
                      ENVIRONMENT,
                      TEXEL0,
                      ENVIRONMENT,
                      TEXEL0,
                      0,
                      SHADE,
                      0,
                      PRIMITIVE,
                      ENVIRONMENT,
                      TEXEL0,
                      ENVIRONMENT,
                      TEXEL0,
                      0,
                      SHADE,
                      0);
    gDPSetRenderMode(gDisplayListPtr++,
                     FORCE_BL | GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                     GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
    gDPLoadTextureBlock_4b(gDisplayListPtr++,
                           texture,
                           G_IM_FMT_I,
                           width,
                           height,
                           0,
                           G_TX_WRAP,
                           G_TX_WRAP,
                           maskS,
                           maskT,
                           G_TX_NOLOD,
                           G_TX_NOLOD);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_NONE);

    if (g_dwScreenScaleShift) {
        x *= 2;
        y *= 2;
        drawWidth *= 2;
        drawHeight *= 2;
    }
    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, TINT_RED, TINT_GREEN, TINT_BLUE, 0xFF);
    gDPSetEnvColor(gDisplayListPtr++, 0, 0, 0, 0xFF);
    /* Start from the last row and step back up the texture. */
    gSPTextureRectangle(gDisplayListPtr++,
                        x << 2,
                        y << 2,
                        (x + drawWidth) << 2,
                        (y + drawHeight) << 2,
                        G_TX_RENDERTILE,
                        0,
                        (height - 1) << 5,
                        ((width << 10) - TEXEL_STEP) / drawWidth,
                        (TEXEL_STEP - (height << 10)) / drawHeight);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
}

void DrawColoredTexturedRect(u8 *texture,
                             s32 width,
                             s32 height,
                             s32 x,
                             s32 y,
                             s32 drawWidth,
                             s32 drawHeight,
                             s32 red,
                             s32 green,
                             s32 blue) {
    s32 scaleS, scaleT, maskS, maskT;
    s32 mirrored = FALSE;

    if (drawWidth < 0) {
        drawWidth = -drawWidth;
        mirrored = TRUE;
    }
    GetTextureScaleAndMask(width, &scaleS, &maskS);
    GetTextureScaleAndMask(height, &scaleT, &maskT);

    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gSPTexture(gDisplayListPtr++, scaleS, scaleT, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetCombineLERP(gDisplayListPtr++,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      TEXEL0,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      TEXEL0);
    gDPSetRenderMode(gDisplayListPtr++,
                     FORCE_BL | CVG_DST_FULL | IM_RD |
                         GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA),
                     GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA));
    gDPLoadTextureBlock_4b(gDisplayListPtr++,
                           texture,
                           G_IM_FMT_I,
                           width,
                           height,
                           0,
                           G_TX_WRAP,
                           G_TX_WRAP,
                           maskS,
                           maskT,
                           G_TX_NOLOD,
                           G_TX_NOLOD);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_NONE);

    if (g_dwScreenScaleShift) {
        x *= 2;
        y *= 2;
        drawWidth *= 2;
        drawHeight *= 2;
    }
    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, red, green, blue, 0xFF);
    /* Start from the last row, and from the last column when mirrored, and step back. */
    gSPTextureRectangle(gDisplayListPtr++,
                        x << 2,
                        y << 2,
                        (x + drawWidth) << 2,
                        (y + drawHeight) << 2,
                        G_TX_RENDERTILE,
                        mirrored ? (width - 1) << 5 : 0,
                        (height - 1) << 5,
                        mirrored ? (TEXEL_STEP - (width << 10)) / drawWidth :
                                   ((width << 10) - TEXEL_STEP) / drawWidth,
                        (TEXEL_STEP - (height << 10)) / drawHeight);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
}
