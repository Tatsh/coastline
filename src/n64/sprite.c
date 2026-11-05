#include "sprite.h"

#include <PR/gbi.h>

#include "common/render.h"
#include "common/system.h"
#include "common/texrect.h"
#include "pakmenu.h"

/* One texel in the 5.10 fixed point of a texture coordinate step. */
#define TEXEL_STEP 0x400

/* The bits of a texel of each depth of sprite. */
#define SPRITE_DEPTH_4 4
#define SPRITE_DEPTH_8 8
#define SPRITE_DEPTH_16 16

/* The texels of a strip for DrawSpriteStrip(), followed by no palette in the releases that take
 * one. */
#define STRIP_PIXELS(pixels) WITH_PALETTE(pixels, NULL)

/* The format DrawSpriteStrip() loads a strip in: the one chosen by the depth and the palette in
 * the releases with palettes, and the value given otherwise. */
#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
#define STRIP_FORMAT(value) format
#else
#define STRIP_FORMAT(value) (value)
#endif

/* The bytes of a sprite, and of each strip, into total and stripBytes. */
#define SPRITE_STRIP_BYTES(sprite, total, stripBytes)                                              \
    if ((sprite)->bitsPerTexel == SPRITE_DEPTH_4) {                                                \
        total = ((sprite)->width * (sprite)->height) >> 1;                                         \
        stripBytes = ((sprite)->width * (sprite)->stripHeight) >> 1;                               \
    } else {                                                                                       \
        total = (sprite)->width * (sprite)->height * ((sprite)->bitsPerTexel >> 3);                \
        stripBytes = (sprite)->width * (sprite)->stripHeight * ((sprite)->bitsPerTexel >> 3);      \
    }

/* Draw every strip of a sprite over a rectangle of the screen, the last strip in memory first. The
 * original reads the position and width again for every strip, and the height once. */
#define DRAW_SPRITE_STRIPS(sprite, x, y, drawWidth, drawHeight, red, green, blue)                  \
    {                                                                                              \
        u32 total, stripBytes, offset;                                                             \
        s32 strips, stripDrawHeight, top, i;                                                       \
                                                                                                   \
        SPRITE_STRIP_BYTES(sprite, total, stripBytes);                                             \
        strips = (sprite)->height / (sprite)->stripHeight;                                         \
        stripDrawHeight = (drawHeight) / strips;                                                   \
        for (i = 1, offset = stripBytes, top = 0; i <= strips;                                     \
             i++, offset += stripBytes, top += stripDrawHeight) {                                  \
            DrawSpriteStrip((sprite),                                                              \
                            STRIP_PIXELS((sprite)->pixels + total - offset),                       \
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
    }

#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
void DrawSpriteAtOrigin(Sprite *sprite, u16 *palette) {
#else
void DrawSpriteAtOrigin(Sprite *sprite) {
#endif
    u32 total, stripBytes;
    s32 strips, stripDrawHeight, i;

    SPRITE_STRIP_BYTES(sprite, total, stripBytes);
    strips = sprite->height / sprite->stripHeight;
    stripDrawHeight = sprite->drawHeight / strips;
    for (i = 1; i <= strips; i++) {
        DrawSpriteStrip(sprite,
                        WITH_PALETTE(sprite->pixels + total - stripBytes * i, palette),
                        sprite->width,
                        sprite->stripHeight,
                        sprite->originX,
                        sprite->originY + stripDrawHeight * (i - 1),
                        sprite->drawWidth,
                        stripDrawHeight,
                        0,
                        0,
                        0);
    }
}

void DrawTexturedSprite(Sprite *sprite, s32 x, s32 y) {
    u32 total, stripBytes, offset;
    s32 strips, stripDrawHeight, row, i;

    SPRITE_STRIP_BYTES(sprite, total, stripBytes);
    strips = sprite->height / sprite->stripHeight;
    stripDrawHeight = sprite->drawHeight / strips;
    for (i = 1, offset = stripBytes, row = y; i <= strips;
         i++, offset += stripBytes, row += stripDrawHeight) {
        DrawSpriteStrip(sprite,
                        STRIP_PIXELS(sprite->pixels + total - offset),
                        sprite->width,
                        sprite->stripHeight,
                        x,
                        row,
                        sprite->drawWidth,
                        stripDrawHeight,
                        0,
                        0,
                        0);
    }
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
    s32 stripDrawHeight;
    u32 total;
    u32 stripBytes;
    s32 strips;
    s32 i;

    SPRITE_STRIP_BYTES(sprite, total, stripBytes);
    strips = sprite->height / sprite->stripHeight;
    stripDrawHeight = height / strips;
    for (i = 1; i <= strips; i++) {
        DrawSpriteStrip(sprite,
                        STRIP_PIXELS(sprite->pixels + total - stripBytes * i),
                        sprite->width,
                        sprite->stripHeight,
                        x,
                        y + stripDrawHeight * (i - 1),
                        width,
                        stripDrawHeight,
                        red,
                        green,
                        blue);
    }
}

void DrawSpriteStrip(Sprite *sprite,
                     u8 *pixels,
#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
                     u16 *palette,
#endif
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
#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
#ifdef ENABLE_PATCHES
    /* A depth the switch does not list is drawn without a palette. */
    s32 format = G_IM_FMT_RGBA;
#else
    /* A depth the switch does not list leaves the format unset where the palette test reads
     * it. */
    s32 format;
#endif /* ENABLE_PATCHES */
#endif

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
#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
        /* A strip with a palette is drawn opaque from palette indices. */
        if (palette != NULL) {
            format = G_IM_FMT_CI;
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
            gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
        } else {
            format = G_IM_FMT_I;
#endif /* TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU */
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
#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
        }
#endif
        gDPLoadTextureBlock_4b(gDisplayListPtr++,
                               pixels,
                               STRIP_FORMAT(G_IM_FMT_I),
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
#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
        format = G_IM_FMT_IA;
#endif
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
#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
        format = sprite == &g_texLogo ? G_IM_FMT_IA : G_IM_FMT_RGBA;
#endif
        gDPSetCombineLERP(
            gDisplayListPtr++, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0);
        gDPSetRenderMode(gDisplayListPtr++, G_RM_XLU_SURF, G_RM_XLU_SURF2);
        gDPLoadTextureBlock(gDisplayListPtr++,
                            pixels,
                            STRIP_FORMAT(sprite == &g_texLogo ? G_IM_FMT_IA : G_IM_FMT_RGBA),
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
#if TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU
    if (format == G_IM_FMT_CI) {
        gDPLoadTLUT_pal16(gDisplayListPtr++, 0, palette);
        gDPSetTextureLUT(gDisplayListPtr++, G_TT_RGBA16);
    } else {
        gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
    }
#else
    gDPSetTextureLUT(gDisplayListPtr++, G_TT_NONE);
#endif /* TGR_REGION == TGR_REGION_JP || TGR_REGION == TGR_REGION_EU */
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
    s32 right = srcX + srcWidth;
    s32 bottom = srcY + srcHeight;

    gDisplayListWords(gDisplayListPtr++, 0xe7000000, 0);
    gDisplayListWords(gDisplayListPtr++, 0xba001402, 0);
    gDisplayListWords(gDisplayListPtr++, 0xba001001, 0);
    gDisplayListWords(gDisplayListPtr++, 0xba000c02, g_dwTextureFilterMode);
    gDisplayListWords(gDisplayListPtr++, 0xfcffffff, 0xfffdf2f9);
    gDisplayListWords(gDisplayListPtr++, 0xb900031d, 0x504240);
    gDisplayListWords(gDisplayListPtr++,
                      0xfd880000 | _SHIFTL((sprite->width >> 1) - 1, 0, 12),
                      (u32)sprite->pixels);
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);

        _g->words.w0 = 0xf5880000 | _SHIFTL((((right - srcX + 1) >> 1) + 7) >> 3, 9, 9);
        _g->words.w1 = 0x7080200;
    }
    gDisplayListWords(gDisplayListPtr++, 0xe6000000, 0);
    gDisplayListWords(gDisplayListPtr++,
                      0xf4000000 | _SHIFTL(srcX << 1, 12, 12) | _SHIFTL(srcY << 2, 0, 12),
                      0x7000000 | _SHIFTL(right << 1, 12, 12) | _SHIFTL(bottom << 2, 0, 12));
    gDisplayListWords(gDisplayListPtr++, 0xe7000000, 0);
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);

        _g->words.w0 = 0xf5800000 | _SHIFTL((((right - srcX + 1) >> 1) + 7) >> 3, 9, 9);
        _g->words.w1 = 0x80200;
    }
    gDisplayListWords(gDisplayListPtr++,
                      0xf2000000 | _SHIFTL(srcX << 2, 12, 12) | _SHIFTL(srcY << 2, 0, 12),
                      _SHIFTL(right << 2, 12, 12) | _SHIFTL(bottom << 2, 0, 12));
    gDisplayListWords(gDisplayListPtr++, 0xba000e02, 0);
    gDisplayListWords(gDisplayListPtr++, 0xba001301, 0);
    if (!g_dwScreenScaleShift) {
        x >>= 1;
        y >>= 1;
        width >>= 1;
        height >>= 1;
    }
    gDPSetPrimColor(gDisplayListPtr++, 0xFF, 0xFF, red, green, blue, 0xFF);
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);

        _g->words.w0 =
            0xe4000000 | _SHIFTL((x + width) << 2, 12, 12) | _SHIFTL((y + height) << 2, 0, 12);
        _g->words.w1 = _SHIFTL(x << 2, 12, 12) | _SHIFTL(y << 2, 0, 12);
    }
    {
        Gfx *_g = (Gfx *)(gDisplayListPtr++);

        _g->words.w0 = _SHIFTL(G_RDPHALF_1, 24, 8);
        _g->words.w1 = _SHIFTL((srcX - 1) << 5, 16, 16) | _SHIFTL((bottom - 1) << 5, 0, 16);
    }
    gImmp1(gDisplayListPtr++,
           G_RDPHALF_2,
           _SHIFTL(((srcWidth << 10) - 0x400) / width, 16, 16) |
               _SHIFTL((1024 - srcHeight * 1024) / height, 0, 16));
    gDisplayListWords(gDisplayListPtr++, 0xe7000000, 0);
    gDisplayListWords(gDisplayListPtr++, 0xba001301, 0x80000);
}

/* Start filling rectangles in a colour, opaque. */
#define BeginFill(red, green, blue)                                                                \
    gDPPipeSync(gDisplayListPtr++);                                                                \
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);                                                \
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);                            \
    gDPSetFillColor(gDisplayListPtr++,                                                             \
                    GPACK_RGBA5551(red, green, blue, 1) << 16 |                                    \
                        GPACK_RGBA5551(red, green, blue, 1))

/* Return to drawing one cycle at a time after the rectangles. */
#define EndFill()                                                                                  \
    gDPPipeSync(gDisplayListPtr++);                                                                \
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE)

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
