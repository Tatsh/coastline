#include "texrect.h"

#include <PR/gbi.h>

#include "render.h"
#include "system.h"

/* One texel in the 5.10 fixed point of a texture coordinate step. */
#define TEXEL_STEP 0x400

/* The red of the tinted rectangle. */
#define TINT_RED 0xFF
#define TINT_GREEN 0x58
#define TINT_BLUE 0x58

/* The bytes of texture memory a strip may load, half of it for a texture with a palette. */
#define TMEM_BYTES 0x1000
#define TMEM_BYTES_WITH_PALETTE 0x800

/* The blending of a translucent rectangle over the picture. */
#define RENDER_MODE_XLU_C1                                                                         \
    (FORCE_BL | CVG_DST_FULL | IM_RD | GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA))
#define RENDER_MODE_XLU_C2 GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_1MA)

void AllocAndDmaLoadAsset(Texture *texture, AssetAllocator alloc) {
    texture->pixels = alloc(texture->romEnd - texture->romStart);
    DmaRead(texture->pixels, texture->romStart, texture->romEnd - texture->romStart);
}

void LoadAndDecompressAsset(Texture *texture, AssetAllocator alloc) {
    texture->pixels = alloc(DmaReadAssetSize(texture->romStart));
    DecompressFromRom(texture->pixels, texture->romStart, NULL);
}

/* A 32-bit texture reads the unset count of texels a word, as the original does. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuninitialized"
void DrawTexturedRect(Texture *texture,
                      s32 x,
                      s32 y,
                      s32 width,
                      s32 height,
                      s32 primRed,
                      s32 primGreen,
                      s32 primBlue,
                      s32 primAlpha,
                      s32 envRed,
                      s32 envGreen,
                      s32 envBlue,
                      s32 envAlpha,
                      s32 blendMode) {
    s32 scaleS, scaleT, maskS, maskT;
    s32 texelsPerWord, rowBytes, rows, remaining, done, dsdx, dtdy, bottom, stripHeight;

    if (width < 0) {
        width = -width;
    }
    GetTextureScaleAndMask(texture->width, &scaleS, &maskS);
    GetTextureScaleAndMask(texture->height, &scaleT, &maskT);

    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gSPTexture(gDisplayListPtr++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    switch (blendMode) {
    case TEXRECT_BLEND_COLOR_THROUGH_TEXTURE:
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
        gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, primRed, primGreen, primBlue, primAlpha);
        gDPSetRenderMode(gDisplayListPtr++, RENDER_MODE_XLU_C1, RENDER_MODE_XLU_C2);
        break;
    case TEXRECT_BLEND_TEXTURE_2:
        /* The original repeats the default case here. */
        gDPSetCombineLERP(
            gDisplayListPtr++, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0);
        gDPSetRenderMode(gDisplayListPtr++, RENDER_MODE_XLU_C1, RENDER_MODE_XLU_C2);
        break;
    case TEXRECT_BLEND_SHADED:
        gDPSetCombineLERP(gDisplayListPtr++,
                          ENVIRONMENT,
                          PRIMITIVE,
                          TEXEL0,
                          PRIMITIVE,
                          0,
                          0,
                          0,
                          TEXEL0,
                          ENVIRONMENT,
                          PRIMITIVE,
                          TEXEL0,
                          PRIMITIVE,
                          0,
                          0,
                          0,
                          TEXEL0);
        gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, primRed, primGreen, primBlue, primAlpha);
        gDPSetEnvColor(gDisplayListPtr++, envRed, envGreen, envBlue, envAlpha);
        gDPSetRenderMode(gDisplayListPtr++, RENDER_MODE_XLU_C1, RENDER_MODE_XLU_C2);
        break;
    case TEXRECT_BLEND_SHADED_OPAQUE:
        gDPSetCombineLERP(gDisplayListPtr++,
                          ENVIRONMENT,
                          PRIMITIVE,
                          TEXEL0,
                          PRIMITIVE,
                          TEXEL0,
                          0,
                          SHADE,
                          0,
                          ENVIRONMENT,
                          PRIMITIVE,
                          TEXEL0,
                          PRIMITIVE,
                          TEXEL0,
                          0,
                          SHADE,
                          0);
        gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, primRed, primGreen, primBlue, primAlpha);
        gDPSetEnvColor(gDisplayListPtr++, envRed, envGreen, envBlue, envAlpha);
        gDPSetRenderMode(gDisplayListPtr++,
                         FORCE_BL | GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                         GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
        break;
    default:
        gDPSetCombineLERP(
            gDisplayListPtr++, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0);
        gDPSetRenderMode(gDisplayListPtr++, RENDER_MODE_XLU_C1, RENDER_MODE_XLU_C2);
        break;
    }
    gDPSetTextureLUT(gDisplayListPtr++, texture->format == G_IM_FMT_CI ? G_TT_RGBA16 : G_TT_NONE);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_NONE);

    if (g_dwScreenScaleShift) {
        x *= 2;
        y *= 2;
        width *= 2;
        height *= 2;
    }

    switch (texture->size) {
    case G_IM_SIZ_4b:
        texelsPerWord = 16;
        break;
    case G_IM_SIZ_8b:
        texelsPerWord = 8;
        break;
    case G_IM_SIZ_16b:
        texelsPerWord = 4;
        break;
    default:
        /* A 32-bit texture does not set the count of texels a word, as in the original. */
        break;
    }
    rowBytes = (texture->width + texelsPerWord - 1) / texelsPerWord * 8;
    /* Each strip loads one row more than it draws, for the filter to blend with. */
    rows = (texture->format == G_IM_FMT_CI ? TMEM_BYTES_WITH_PALETTE / rowBytes :
                                             TMEM_BYTES / rowBytes) -
           1;
    dsdx = (texture->width << 10) / width;
    dtdy = (texture->height << 10) / height;
    /* The strips go up from the bottom of the rectangle, in 10.10 fixed point. */
    bottom = ((y + height) << 2) << 8;
    remaining = texture->height;
    done = 0;

    while (remaining != 0) {
        if (remaining < rows) {
            rows = remaining;
        }
        remaining -= rows;
        stripHeight = ((rows << 10) / dtdy) << 10;
        bottom -= stripHeight;

        switch (texture->size) {
        case G_IM_SIZ_4b:
            gDPSetTextureImage(gDisplayListPtr++,
                               texture->format,
                               G_IM_SIZ_16b,
                               1,
                               texture->pixels + rowBytes * done);
            gDPSetTile(gDisplayListPtr++,
                       texture->format,
                       G_IM_SIZ_16b,
                       0,
                       0,
                       G_TX_LOADTILE,
                       0,
                       G_TX_CLAMP,
                       maskT,
                       G_TX_NOLOD,
                       G_TX_CLAMP,
                       maskS,
                       G_TX_NOLOD);
            gDPLoadSync(gDisplayListPtr++);
            gDPLoadBlock(gDisplayListPtr++,
                         G_TX_LOADTILE,
                         0,
                         0,
                         ((texture->width * (rows + 1) + 3) >> 2) - 1,
                         CALC_DXT_4b(texture->width));
            gDPPipeSync(gDisplayListPtr++);
            gDPSetTile(gDisplayListPtr++,
                       texture->format,
                       G_IM_SIZ_4b,
                       ((texture->width >> 1) + 7) >> 3,
                       0,
                       G_TX_RENDERTILE,
                       0,
                       G_TX_CLAMP,
                       maskT,
                       G_TX_NOLOD,
                       G_TX_CLAMP,
                       maskS,
                       G_TX_NOLOD);
            gDPSetTileSize(
                gDisplayListPtr++, G_TX_RENDERTILE, 0, 0, (texture->width - 1) << 2, rows << 2);
            break;
        case G_IM_SIZ_8b:
            gDPSetTextureImage(gDisplayListPtr++,
                               texture->format,
                               G_IM_SIZ_16b,
                               1,
                               texture->pixels + rowBytes * done);
            gDPSetTile(gDisplayListPtr++,
                       texture->format,
                       G_IM_SIZ_16b,
                       0,
                       0,
                       G_TX_LOADTILE,
                       0,
                       G_TX_CLAMP,
                       maskT,
                       G_TX_NOLOD,
                       G_TX_CLAMP,
                       maskS,
                       G_TX_NOLOD);
            gDPLoadSync(gDisplayListPtr++);
            gDPLoadBlock(gDisplayListPtr++,
                         G_TX_LOADTILE,
                         0,
                         0,
                         ((texture->width * (rows + 1) + 1) >> 1) - 1,
                         CALC_DXT(texture->width, 1));
            gDPPipeSync(gDisplayListPtr++);
            gDPSetTile(gDisplayListPtr++,
                       texture->format,
                       G_IM_SIZ_8b,
                       (texture->width + 7) >> 3,
                       0,
                       G_TX_RENDERTILE,
                       0,
                       G_TX_CLAMP,
                       maskT,
                       G_TX_NOLOD,
                       G_TX_CLAMP,
                       maskS,
                       G_TX_NOLOD);
            gDPSetTileSize(
                gDisplayListPtr++, G_TX_RENDERTILE, 0, 0, (texture->width - 1) << 2, rows << 2);
            break;
        case G_IM_SIZ_16b:
            gDPSetTextureImage(gDisplayListPtr++,
                               texture->format,
                               G_IM_SIZ_16b,
                               1,
                               texture->pixels + rowBytes * done);
            gDPSetTile(gDisplayListPtr++,
                       texture->format,
                       G_IM_SIZ_16b,
                       0,
                       0,
                       G_TX_LOADTILE,
                       0,
                       G_TX_CLAMP,
                       maskT,
                       G_TX_NOLOD,
                       G_TX_CLAMP,
                       maskS,
                       G_TX_NOLOD);
            gDPLoadSync(gDisplayListPtr++);
            gDPLoadBlock(gDisplayListPtr++,
                         G_TX_LOADTILE,
                         0,
                         0,
                         texture->width * (rows + 1) - 1,
                         CALC_DXT(texture->width, 2));
            gDPPipeSync(gDisplayListPtr++);
            gDPSetTile(gDisplayListPtr++,
                       texture->format,
                       G_IM_SIZ_16b,
                       ((texture->width * 2) + 7) >> 3,
                       0,
                       G_TX_RENDERTILE,
                       0,
                       G_TX_CLAMP,
                       maskT,
                       G_TX_NOLOD,
                       G_TX_CLAMP,
                       maskS,
                       G_TX_NOLOD);
            gDPSetTileSize(
                gDisplayListPtr++, G_TX_RENDERTILE, 0, 0, (texture->width - 1) << 2, rows << 2);
            break;
        }

        /* The strip starts from its last row and steps back up the texture. */
        gSPTextureRectangle(gDisplayListPtr++,
                            x << 2,
                            bottom >> 8,
                            (x + width) << 2,
                            (bottom + stripHeight) >> 8,
                            G_TX_RENDERTILE,
                            0,
                            (rows - 1) << 5,
                            dsdx,
                            -dtdy);
        done += rows;
    }

    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
}
#pragma GCC diagnostic pop

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
