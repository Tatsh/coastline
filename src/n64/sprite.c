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

/* IDO schedules the minimum row size differently when one is the first operand. */
#define SPRITE_ROW_WORDS_4B(width) (1 > (width) / 16 ? 1 : (width) / 16)
#define SPRITE_ROW_WORDS(width, bytes) (1 > (width) * (bytes) / 8 ? 1 : (width) * (bytes) / 8)
#define SPRITE_DXT_4B(width)                                                                       \
    (((1 << G_TX_DXT_FRAC) + SPRITE_ROW_WORDS_4B(width) - 1) / SPRITE_ROW_WORDS_4B(width))
#define SPRITE_DXT(width, bytes)                                                                   \
    (((1 << G_TX_DXT_FRAC) + SPRITE_ROW_WORDS(width, bytes) - 1) / SPRITE_ROW_WORDS(width, bytes))

/* The texels of a strip for DrawSpriteStrip(), followed by no palette in the releases that take
 * one. */
#define STRIP_PIXELS(pixels) WITH_PALETTE(pixels, NULL)

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
    int maskBitsS;
    int maskBitsT;
    int scaleS;
    int scaleT;
    int mirrored;

#if (defined(ENABLE_PATCHES) || !defined(__sgi)) &&                                                \
    (TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP)
    int format = G_IM_FMT_RGBA;
#else
    int format;
#endif

    mirrored = 0;
    if (drawWidth < 0) {
        drawWidth = -drawWidth;
        mirrored = 1;
    }
    GetTextureScaleAndMask(width, &scaleS, &maskBitsS);
    GetTextureScaleAndMask(height, &scaleT, &maskBitsT);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gSPTexture(gDisplayListPtr++, scaleS, scaleT, 0, 0, 1);
    gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 12, 2, g_dwTextureFilterMode);
    switch (sprite->bitsPerTexel) {
    case 4:
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
        if (palette != 0) {
            format = 2;
            gDisplayListWords(gDisplayListPtr++, 0xfc000000 | 0xffffff, 0xfffcf279);
            gDPSetRenderMode(gDisplayListPtr++, 0x0f0a4000, 0);
        } else {
#endif
            format = 4;
            if (sprite == &g_texBackground) {
                gDisplayListWords(gDisplayListPtr++, 0xfc000000 | 0x309861, 0x5532ff7f);
                gDPSetRenderMode(gDisplayListPtr++, 0x0f0a4000, 0);
            } else {
                gDisplayListWords(gDisplayListPtr++, 0xfc000000 | 0xffffff, 0xfffdf2f9);
                gDPSetRenderMode(gDisplayListPtr++, 0x00504240, 0);
            }
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
        }
#endif
        gDPSetTextureImage(gDisplayListPtr++, format, 2, 1, pixels);
        gDPSetTile(gDisplayListPtr++, format, 2, 0, 0, 7, 0, 0, maskBitsT, 0, 0, maskBitsS, 0);
        gDPLoadSync(gDisplayListPtr++);
        gDPLoadBlock(
            gDisplayListPtr++, 7, 0, 0, ((width * height + 3) >> 2) - 1, SPRITE_DXT_4B(width));
        gDPPipeSync(gDisplayListPtr++);
        gDPSetTile(gDisplayListPtr++,
                   format,
                   0,
                   ((width >> 1) + 7) >> 3,
                   0,
                   0,
                   0,
                   0,
                   maskBitsT,
                   0,
                   0,
                   maskBitsS,
                   0);
        {
            Gfx *_g = (Gfx *)(gDisplayListPtr++);

            _g->words.w0 = _SHIFTL(G_SETTILESIZE, 24, 8);
            _g->words.w1 = _SHIFTL((width - 1) << 2, 12, 12) | _SHIFTL((height - 1) << 2, 0, 12);
        }
        break;
    case 8:
        format = 3;
        if (sprite->style == 'c') {
            gDisplayListWords(gDisplayListPtr++, 0xfc000000 | 0x11fe23, 0xfffff3f9);
        } else {
            gDisplayListWords(gDisplayListPtr++, 0xfc000000 | 0xffffff, 0xfffcf279);
        }
        gDPSetRenderMode(gDisplayListPtr++, 0x00504240, 0);
        gDPSetTextureImage(gDisplayListPtr++, format, 2, 1, pixels);
        gDPSetTile(gDisplayListPtr++, format, 2, 0, 0, 7, 0, 0, maskBitsT, 0, 0, maskBitsS, 0);
        gDPLoadSync(gDisplayListPtr++);
        gDPLoadBlock(
            gDisplayListPtr++, 7, 0, 0, ((width * height + 1) >> 1) - 1, SPRITE_DXT(width, 1));
        gDPPipeSync(gDisplayListPtr++);
        gDPSetTile(gDisplayListPtr++,
                   format,
                   1,
                   (width * 1 + 7) >> 3,
                   0,
                   0,
                   0,
                   0,
                   maskBitsT,
                   0,
                   0,
                   maskBitsS,
                   0);
        {
            Gfx *_g = (Gfx *)(gDisplayListPtr++);

            _g->words.w0 = _SHIFTL(G_SETTILESIZE, 24, 8);
            _g->words.w1 = _SHIFTL((width - 1) << 2, 12, 12) | _SHIFTL((height - 1) << 2, 0, 12);
        }
        break;
    case 16:
        if (sprite == &g_texLogo) {
            format = 3;
        } else {
            format = 0;
        }
        gDisplayListWords(gDisplayListPtr++, 0xfc000000 | 0xffffff, 0xfffcf279);
        gDPSetRenderMode(gDisplayListPtr++, 0x00504240, 0);
        gDPSetTextureImage(gDisplayListPtr++, format, 2, 1, pixels);
        gDPSetTile(gDisplayListPtr++, format, 2, 0, 0, 7, 0, 0, maskBitsT, 0, 0, maskBitsS, 0);
        gDPLoadSync(gDisplayListPtr++);
        gDPLoadBlock(gDisplayListPtr++, 7, 0, 0, width * height - 1, SPRITE_DXT(width, 2));
        gDPPipeSync(gDisplayListPtr++);
        gDPSetTile(gDisplayListPtr++,
                   format,
                   2,
                   (width * 2 + 7) >> 3,
                   0,
                   0,
                   0,
                   0,
                   maskBitsT,
                   0,
                   0,
                   maskBitsS,
                   0);
        {
            Gfx *_g = (Gfx *)(gDisplayListPtr++);

            _g->words.w0 = _SHIFTL(G_SETTILESIZE, 24, 8);
            _g->words.w1 = _SHIFTL((width - 1) << 2, 12, 12) | _SHIFTL((height - 1) << 2, 0, 12);
        }
        break;
    }
#if TGR_REGION == TGR_REGION_EU || TGR_REGION == TGR_REGION_JP
    if (format == 2) {
        /* Load the sixteen colours of a 4-bit palette as RGBA16. */
        gDPSetTextureImage(gDisplayListPtr++, 0, 2, 1, palette);
        gDPTileSync(gDisplayListPtr++);
        gDPSetTile(gDisplayListPtr++, 0, 0, 0, 0x100, 7, 0, 0, 0, 0, 0, 0, 0);
        gDPLoadSync(gDisplayListPtr++);
        gDPLoadTLUTCmd(gDisplayListPtr++, 7, 15);
        gDPPipeSync(gDisplayListPtr++);
        gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 14, 2, 0x8000);
    } else {
        gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 14, 2, 0);
    }
#else
    gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 14, 2, 0);
#endif
    gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 19, 1, 0);
    if (g_dwScreenScaleShift == 0) {
        x >>= 1;
        y >>= 1;
        drawWidth >>= 1;
        drawHeight >>= 1;
    }
    gDPSetPrimColor(gDisplayListPtr++, 0xff, 0xff, red, green, blue, 0xff);
    gDPSetEnvColor(gDisplayListPtr++, 0, 0, 0, 0xff);
    gSPTextureRectangle(gDisplayListPtr++,
                        x << 2,
                        y << 2,
                        (x + drawWidth) << 2,
                        (y + drawHeight) << 2,
                        0,
                        mirrored ? (width - 1) << 5 : 0,
                        (height - 1) << 5,
                        mirrored ? (TEXEL_STEP - (width << 10)) / drawWidth :
                                   ((width << 10) - TEXEL_STEP) / drawWidth,
                        (TEXEL_STEP - (height << 10)) / drawHeight);
    gDPPipeSync(gDisplayListPtr++);
    gSPSetOtherMode(gDisplayListPtr++, G_SETOTHERMODE_H, 19, 1, 0x80000);
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

#ifndef __sgi
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
#endif
