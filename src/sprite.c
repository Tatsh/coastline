#include "sprite.h"

#include <PR/gbi.h>

#include "pakmenu.h"
#include "render.h"
#include "system.h"
#include "texrect.h"

/* One texel in the 5.10 fixed point of a texture coordinate step. */
#define TEXEL_STEP 0x400

/* The bits of a texel of each depth of sprite. */
#define SPRITE_DEPTH_4 4
#define SPRITE_DEPTH_8 8
#define SPRITE_DEPTH_16 16

/* Draw every strip of a sprite over a rectangle of the screen, the last strip in memory first. The
 * original reads the position and width again for every strip, and the height once. */
#define DRAW_SPRITE_STRIPS(sprite, x, y, drawWidth, drawHeight, red, green, blue)                  \
    do {                                                                                           \
        u32 total, stripBytes, offset;                                                             \
        s32 strips, stripDrawHeight, top, i;                                                       \
                                                                                                   \
        if ((sprite)->bitsPerTexel == SPRITE_DEPTH_4) {                                            \
            total = ((sprite)->width * (sprite)->height) >> 1;                                     \
            stripBytes = ((sprite)->width * (sprite)->stripHeight) >> 1;                           \
        } else {                                                                                   \
            total = (sprite)->width * (sprite)->height * ((sprite)->bitsPerTexel >> 3);            \
            stripBytes = (sprite)->width * (sprite)->stripHeight * ((sprite)->bitsPerTexel >> 3);  \
        }                                                                                          \
        strips = (sprite)->height / (sprite)->stripHeight;                                         \
        stripDrawHeight = (drawHeight) / strips;                                                   \
        for (i = 1, offset = stripBytes, top = 0; i <= strips;                                     \
             i++, offset += stripBytes, top += stripDrawHeight) {                                  \
            DrawSpriteStrip((sprite),                                                              \
                            (sprite)->pixels + total - offset,                                     \
                            (sprite)->width,                                                       \
                            (sprite)->stripHeight,                                                 \
                            (x),                                                                   \
                            (y) + top,                                                             \
                            (drawWidth),                                                           \
                            stripDrawHeight,                                                       \
                            (red),                                                                 \
                            (green),                                                               \
                            (blue));                                                               \
        }                                                                                          \
    } while (0)

void DrawSpriteAtOrigin(Sprite *sprite) {
    DRAW_SPRITE_STRIPS(
        sprite, sprite->originX, sprite->originY, sprite->drawWidth, sprite->drawHeight, 0, 0, 0);
}

void DrawTexturedSprite(Sprite *sprite, s32 x, s32 y) {
    DRAW_SPRITE_STRIPS(sprite, x, y, sprite->drawWidth, sprite->drawHeight, 0, 0, 0);
}

void DrawSpriteTinted(Sprite *sprite, u8 red, u8 green, u8 blue) {
    DRAW_SPRITE_STRIPS(sprite,
                       sprite->originX,
                       sprite->originY,
                       sprite->drawWidth,
                       sprite->drawHeight,
                       red,
                       green,
                       blue);
}

void DrawSpriteRegion(
    Sprite *sprite, s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue) {
    DRAW_SPRITE_STRIPS(sprite, x, y, width, height, red, green, blue);
}

void DrawSpriteStrip(Sprite *sprite,
                     u8 *pixels,
                     s32 width,
                     s32 height,
                     s32 x,
                     s32 y,
                     s32 drawWidth,
                     s32 drawHeight,
                     u8 red,
                     u8 green,
                     u8 blue) {
    s32 mirrored = FALSE;
    s32 scaleS, maskS, scaleT, maskT;

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
    switch (sprite->bitsPerTexel) {
    case SPRITE_DEPTH_4:
        if (sprite == &g_texBackground) {
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
            gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
        } else {
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
            gDPSetRenderMode(gDisplayListPtr++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        }
        gDPLoadTextureBlock_4b(gDisplayListPtr++,
                               pixels,
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
        break;
    case SPRITE_DEPTH_8:
        if (sprite->style == SPRITE_STYLE_TINTED) {
            gDPSetCombineLERP(gDisplayListPtr++,
                              TEXEL0,
                              0,
                              PRIMITIVE,
                              0,
                              0,
                              0,
                              0,
                              TEXEL0,
                              TEXEL0,
                              0,
                              PRIMITIVE,
                              0,
                              0,
                              0,
                              0,
                              TEXEL0);
        } else {
            gDPSetCombineLERP(gDisplayListPtr++,
                              0,
                              0,
                              0,
                              TEXEL0,
                              0,
                              0,
                              0,
                              TEXEL0,
                              0,
                              0,
                              0,
                              TEXEL0,
                              0,
                              0,
                              0,
                              TEXEL0);
        }
        gDPSetRenderMode(gDisplayListPtr++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPLoadTextureBlock(gDisplayListPtr++,
                            pixels,
                            G_IM_FMT_IA,
                            G_IM_SIZ_8b,
                            width,
                            height,
                            0,
                            G_TX_WRAP,
                            G_TX_WRAP,
                            maskS,
                            maskT,
                            G_TX_NOLOD,
                            G_TX_NOLOD);
        break;
    case SPRITE_DEPTH_16:
        gDPSetCombineLERP(
            gDisplayListPtr++, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0);
        gDPSetRenderMode(gDisplayListPtr++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPLoadTextureBlock(gDisplayListPtr++,
                            pixels,
                            sprite == &g_texLogo ? G_IM_FMT_IA : G_IM_FMT_RGBA,
                            G_IM_SIZ_16b,
                            width,
                            height,
                            0,
                            G_TX_WRAP,
                            G_TX_WRAP,
                            maskS,
                            maskT,
                            G_TX_NOLOD,
                            G_TX_NOLOD);
        break;
    }
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_NONE);
    if (!g_dwScreenScaleShift) {
        x >>= 1;
        y >>= 1;
        drawWidth >>= 1;
        drawHeight >>= 1;
    }
    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, red, green, blue, 0xFF);
    gDPSetEnvColor(gDisplayListPtr++, 0, 0, 0, 0xFF);
    /* The rows run from the bottom of the texture up. */
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

void DrawScaledTexturedRect(Sprite *sprite,
                            s32 srcX,
                            s32 srcY,
                            s32 srcWidth,
                            s32 srcHeight,
                            s32 x,
                            s32 y,
                            s32 width,
                            s32 height,
                            u8 red,
                            u8 green,
                            u8 blue) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetTextureLOD(gDisplayListPtr++, G_TL_TILE);
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
    gDPSetRenderMode(gDisplayListPtr++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
    /* The far edges are one texel past the inclusive ones the macro expects. */
    gDPLoadTextureTile_4b(gDisplayListPtr++,
                          sprite->pixels,
                          G_IM_FMT_I,
                          sprite->width,
                          0,
                          srcX,
                          srcY,
                          srcX + srcWidth,
                          srcY + srcHeight,
                          0,
                          G_TX_CLAMP,
                          G_TX_CLAMP,
                          0,
                          0,
                          G_TX_NOLOD,
                          G_TX_NOLOD);
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_NONE);
    if (!g_dwScreenScaleShift) {
        x >>= 1;
        y >>= 1;
        width >>= 1;
        height >>= 1;
    }
    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, red, green, blue, 0xFF);
    /* The first column drawn is the one before the source rectangle. */
    gSPTextureRectangle(gDisplayListPtr++,
                        x << 2,
                        y << 2,
                        (x + width) << 2,
                        (y + height) << 2,
                        G_TX_RENDERTILE,
                        (srcX - 1) << 5,
                        (srcY + srcHeight - 1) << 5,
                        ((srcWidth << 10) - TEXEL_STEP) / width,
                        (TEXEL_STEP - (srcHeight << 10)) / height);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
}

/* Start filling rectangles in a colour, opaque. */
static inline void BeginFill(u8 red, u8 green, u8 blue) {
    u32 color = GPACK_RGBA5551(red, green, blue, 1);

    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetFillColor(gDisplayListPtr++, color << 16 | color);
}

/* Return to drawing one cycle at a time after the rectangles. */
static inline void EndFill(void) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
}

void DrawPixel(s32 x, s32 y, u8 shade) {
    if (!g_dwScreenScaleShift) {
        x >>= 1;
        y >>= 1;
    }
    BeginFill(shade, shade, shade);
    gDPFillRectangle(gDisplayListPtr++, x, y, x + 1, y + 1);
    EndFill();
}

void DrawRectOutline(
    s32 x, s32 y, s32 width, s32 height, s32 thickness, u8 red, u8 green, u8 blue) {
    if (!g_dwScreenScaleShift) {
        x >>= 1;
        y >>= 1;
        width >>= 1;
        height >>= 1;
        thickness >>= 1;
    }
    if (thickness < 2) {
        thickness = 1;
    }
    BeginFill(red, green, blue);
    gDPFillRectangle(gDisplayListPtr++, x, y, x + width, y + thickness);
    gDPFillRectangle(gDisplayListPtr++, x, y + height - thickness, x + width, y + height);
    gDPFillRectangle(gDisplayListPtr++, x, y, x + thickness, y + height);
    gDPFillRectangle(gDisplayListPtr++, x + width - thickness, y, x + width, y + height);
    EndFill();
}

void DrawFilledRectHiRes(s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue) {
    if (!g_dwScreenScaleShift) {
        x >>= 1;
        y >>= 1;
        width >>= 1;
        height >>= 1;
    }
    if (width < 2) {
        width = 1;
    }
    if (height < 2) {
        height = 1;
    }
    BeginFill(red, green, blue);
    gDPFillRectangle(gDisplayListPtr++, x, y, x + width, y + height);
    EndFill();
}
