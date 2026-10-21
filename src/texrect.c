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
