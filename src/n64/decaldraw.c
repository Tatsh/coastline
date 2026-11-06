#include "decaldraw.h"

#include <PR/gbi.h>
#include <PR/os_libc.h>

#include "common/render.h"
#include "common/system.h"
#include "decaleditor.h"

/* The bytes of the largest decal. The flood fill copies them whole. */
#define DECAL_BYTES 0x800

/* The runs the flood fill may have pending. */
#define FILL_SPANS 400

/* The texel of the font that is ink; every other is clear. */
#define FONT_INK 15

/* The texels of a cell of the font. */
#define FONT_CELL 16

/* The dashes repeat every 8 steps, 4 in each color. */
#define DASH_PERIOD 8
#define DASH_LENGTH 4

/* A run of texels the flood fill has still to look beyond, on the row dy away. */
typedef struct {
    s32 y;  /*!< The row of the run. */
    s32 x1; /*!< The first column of the run. */
    s32 x2; /*!< The last column of the run. */
    s32 dy; /*!< The step to the row to look at, 1 or -1. */
} FillSpan;

/* The texels of a rectangle given by two corners on the screen, each edge included. */
typedef struct {
    s32 minX; /*!< The leftmost column. */
    s32 minY; /*!< The top row. */
    s32 maxX; /*!< The rightmost column. */
    s32 maxY; /*!< The bottom row. */
} TexelBox;

/* The middle of two coordinates, worked out again at each use as the original does. */
#define MIDPOINT(a, b) (((a) + (b)) >> 1)
#define BOX_CENTER_X(box) MIDPOINT((box).minX, (box).maxX)
#define BOX_CENTER_Y(box) MIDPOINT((box).minY, (box).maxY)

#define ScreenToTexelX(screenX) (((screenX) - g_rcDecalCanvas.x) >> 2)

#define ScreenToTexelY(screenY) ((g_rcDecalCanvas.y + g_rcDecalCanvas.height - (screenY)) >> 2)

/* The byte of a texel. The words of an odd row are swapped, as the RDP expects of a 4-bit
 * texture loaded in one block. */
#define TexelOffset(x, y) ((((((y) & 1) << 3) ^ (x)) >> 1) + (y) * (g_bDecalWidth >> 1))

#define SWAP(a, b, swap)                                                                           \
    swap = a;                                                                                      \
    a = b;                                                                                         \
    b = swap

/* Turn a point on the screen into a texel in place. */
#define ScreenToTexel(x, y)                                                                        \
    x = ScreenToTexelX(x);                                                                         \
    y = ScreenToTexelY(y)

/* Paint the texels from (x0, y0) up to (x1, y1), excluding the far column and row. */
#define FillTexels(x, y, x0, y0, x1, y1)                                                           \
    for (y = (y0); y < (y1); y++) {                                                                \
        for (x = (x0); x < (x1); x++) {                                                            \
            SetPixel4Bit(x, y, g_bDecalColor);                                                     \
        }                                                                                          \
    }

#define FillTexelRow(x, x0, x1, y)                                                                 \
    for (x = (x0); x < (x1); x++) {                                                                \
        SetPixel4Bit(x, y, g_bDecalColor);                                                         \
    }

/* Paint the texels from x0 to x1 of a row, both included. */
#define FillTexelSpan(x, x0, x1, y)                                                                \
    for (x = (x0); x <= (x1); x++) {                                                               \
        SetPixel4Bit(x, y, g_bDecalColor);                                                         \
    }

#define FillTexelColumn(y, x, y0, y1)                                                              \
    for (y = (y0); y < (y1); y++) {                                                                \
        SetPixel4Bit(x, y, g_bDecalColor);                                                         \
    }

/* Paint a run of texels from a point in a direction, as long as the brush is wide, or the point
 * alone for the smallest brush. */
#define PaintRun(i, x, y, dx, dy, half)                                                            \
    if ((half) == 0) {                                                                             \
        SetPixel4Bit(x, y, g_bDecalColor);                                                         \
    } else {                                                                                       \
        for (i = 0; i < (half) * 2; i++) {                                                         \
            SetPixel4Bit((x) + i * (dx), (y) + i * (dy), g_bDecalColor);                           \
        }                                                                                          \
    }

/* Paint a run of texels from a point in a direction, as long as the brush. */
#define PaintBrushRun(i, x, y, dx, dy, brush)                                                      \
    for (i = 0; i < (brush); i++) {                                                                \
        SetPixel4Bit((x) + i * (dx), (y) + i * (dy), g_bDecalColor);                               \
    }

#define PushSpan(top, y_, x1_, x2_, dy_)                                                           \
    (top)->y = (y_);                                                                               \
    (top)->x1 = (x1_);                                                                             \
    (top)->x2 = (x2_);                                                                             \
    (top)->dy = (dy_);                                                                             \
    (top)++

/* The space from a glyph to the next. */
#define GlyphAdvance(glyph, next)                                                                  \
    ((glyph)->width -                                                                              \
     ((glyph)->kernRight < (next)->kernLeft ? (glyph)->kernRight : (next)->kernLeft) + 2)

/* A texel of the font, read without the swap of odd rows. */
#define FontTexel(column, row)                                                                     \
    (((column) & 1) ?                                                                              \
         (g_texDecalFont.pixels[((column) >> 1) + (row) * (g_texDecalFont.width >> 1)] & 0xF) :    \
         (g_texDecalFont.pixels[((column) >> 1) + (row) * (g_texDecalFont.width >> 1)] >> 4))

/* Count a frame of the dashes. Their colors swap every 8 frames. */
#define AdvanceDashes()                                                                            \
    g_wScreenFrameCount++;                                                                         \
    if ((g_wScreenFrameCount & 7) == 0) {                                                          \
        SwapBytes(&g_bDashColorA, &g_bDashColorB);                                                 \
    }

#define DashColor(phase) ((phase) < DASH_LENGTH ? g_bDashColorA : g_bDashColorB)

#define AlternateDashColor(phase) ((phase) ? g_bDashColorA : g_bDashColorB)

u8 GetPixel4Bit(s32 x, s32 y) {
    s32 offset = TexelOffset(x, y);
    u8 texel;

    if (x & 1) {
        texel = g_pDecalTexels[offset] & 0xF;
    } else {
        texel = g_pDecalTexels[offset] >> 4;
    }
    return texel;
}

u8 ReadPixel4Bit(u8 *texels, s32 x, s32 y) {
    s32 offset = TexelOffset(x, y);
    u8 texel;

    if (x & 1) {
        texel = texels[offset] & 0xF;
    } else {
        texel = texels[offset] >> 4;
    }
    return texel;
}

void FloodFill4Bit(s32 x, s32 y) {
    u8 old;
    s32 dy, count, left, x1, x2;
    FillSpan stack[FILL_SPANS];
    u8 *mask, *texels;

    x = ScreenToTexelX(x), y = ScreenToTexelY(y);
    old = GetPixel4Bit(x, y);
    if (old != g_bDecalColor) {
        stack[0].y = y;
        stack[0].x1 = x;
        stack[0].x2 = x;
        stack[0].dy = 1;
        stack[1].y = y + 1;
        stack[1].x1 = x;
        stack[1].x2 = x;
        stack[1].dy = -1, count = 2;

        /* The fill runs in a copy with the mask off, then passes through the mask. */
        mask = g_pDecalMask;
        g_pDecalMask = NULL;
        memcpy(g_pDecalScratchTexels, g_pDecalTexels, DECAL_BYTES);
        texels = g_pDecalTexels;
        g_pDecalTexels = g_pDecalScratchTexels;

        /* The original does not bound individual pushes onto the stack. */
        while (count > 0 && count < FILL_SPANS) {
            dy = stack[--count].dy;
            y = stack[count].y + dy;
            x = x1 = stack[count].x1;
            x2 = stack[count].x2;
            /* The scan to the left does not check the width. */
            while (x >= 0 && y >= 0 && y < g_bDecalHeight && GetPixel4Bit(x, y) == old) {
                SetPixel4Bit(x--, y, g_bDecalColor);
            }
            if (x >= x1) {
                goto skip;
            }
            left = x + 1;
            if (left < x1) {
                stack[count].y = y;
                stack[count].x1 = left;
                stack[count].x2 = x1 - 1;
                stack[count].dy = -dy;
                count++;
            }
            x = x1 + 1;
            do {
                while (x < g_bDecalWidth && y >= 0 && y < g_bDecalHeight &&
                       GetPixel4Bit(x, y) == old) {
                    SetPixel4Bit(x++, y, g_bDecalColor);
                }
                stack[count].y = y;
                stack[count].x1 = left;
                stack[count].x2 = x - 1;
                stack[count].dy = dy;
                count++;
                if (x > x2 + 1) {
                    stack[count].y = y;
                    stack[count].x1 = x2 + 1;
                    stack[count].x2 = x - 1;
                    stack[count].dy = -dy;
                    count++;
                }
            skip:
                for (x++; x <= x2 && x < g_bDecalWidth && y >= 0 && y < g_bDecalHeight &&
                          GetPixel4Bit(x, y) != old;
                     x++) {
                }
                left = x;
            } while (x <= x2);
        }

        g_pDecalMask = mask;
        for (y = 0; y < g_bDecalHeight; y++) {
            for (x = 0; x < g_bDecalWidth; x++) {
                old = GetPixel4Bit(x, y);
                g_pDecalTexels = texels;
                SetPixel4Bit(x, y, old);
                g_pDecalTexels = g_pDecalScratchTexels;
            }
        }
        g_pDecalTexels = texels;
    }
}

void StampTextOnDecal(s32 x, s32 y) {
    /* IDO reserves this slot in the original stack frame. */
    s32 unused;
    s32 column;
    s32 i;
    s32 entry;
    s32 kern;
#ifdef ENABLE_PATCHES
    s32 glyphIndex = 0;
#else
    s32 glyphIndex;
#endif /* ENABLE_PATCHES */
    s32 row;
    s32 offset;
    s32 firstColumn;
    s32 lastColumn;
    s32 penX;
    s32 texelX;

    x = ScreenToTexelX(x) + 1;
    y = ScreenToTexelY(y);
    penX = x - (g_bDecalTextWidth >> 1);
    for (i = 0; i < g_bDecalTextLength; i++) {
        for (entry = 0; entry < DECAL_GLYPHS; entry++) {
            if (g_apDecalTextGlyphs[i]->character == g_aDecalGlyphs[entry].character) {
                glyphIndex = entry;
                break;
            }
        }
        firstColumn = g_apDecalTextGlyphs[i]->cellX + glyphIndex * FONT_CELL;
        lastColumn = g_apDecalTextGlyphs[i]->width + firstColumn;

        if (g_texPaintToolText.style != 0) {
            for (row = 0; row < g_texDecalFont.height; row++) {
                for (column = firstColumn; column <= lastColumn; column++) {
                    texelX = column - firstColumn + penX;
                    if (texelX >= 0 && texelX < g_bDecalWidth && row + y >= 0 &&
                        row + y < g_bDecalHeight) {
                        offset = (column >> 1) + row * (g_texDecalFont.width >> 1);
                        if (column & 1) {
                            if ((g_texDecalFont.pixels[offset] & 0xF) == FONT_INK) {
                                SetPixel4Bit(texelX + 1, row + y - 1, g_bTextShadowColor);
                            }
                        } else if (g_texDecalFont.pixels[offset] >> 4 == FONT_INK) {
                            SetPixel4Bit(texelX + 1, row + y - 1, g_bTextShadowColor);
                        }
                    }
                }
            }
        }
        for (row = 0; row < g_texDecalFont.height; row++) {
            for (column = firstColumn; column <= lastColumn; column++) {
                texelX = column - firstColumn + penX;
                if (texelX >= 0 && texelX < g_bDecalWidth && row + y >= 0 &&
                    row + y < g_bDecalHeight) {
                    offset = (column >> 1) + row * (g_texDecalFont.width >> 1);
                    if (column & 1) {
                        if ((g_texDecalFont.pixels[offset] & 0xF) == FONT_INK) {
                            SetPixel4Bit(texelX, row + y, g_bDecalColor);
                        }
                    } else if (g_texDecalFont.pixels[offset] >> 4 == FONT_INK) {
                        SetPixel4Bit(texelX, row + y, g_bDecalColor);
                    }
                }
            }
        }

        if (i < g_bDecalTextLength - 1) {
            kern = g_apDecalTextGlyphs[i]->kernRight < g_apDecalTextGlyphs[i + 1]->kernLeft ?
                       g_apDecalTextGlyphs[i]->kernRight :
                       g_apDecalTextGlyphs[i + 1]->kernLeft;
            penX += g_apDecalTextGlyphs[i]->width - kern + 2;
        }
    }
}

void DrawLine4Bit(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 dx, dy, radius, x, y, stepX, stepY, error, swap;

    x0 = ScreenToTexelX(x0), y0 = ScreenToTexelY(y0);
    x1 = ScreenToTexelX(x1), y1 = ScreenToTexelY(y1);
    radius = g_aBrushSizeRects[g_bBrushSize].width >> 1;
    dx = x1 - x0 < 0 ? -(x1 - x0) : x1 - x0;
    dy = y1 - y0 < 0 ? -(y1 - y0) : y1 - y0;
    /* An exact diagonal drawn downwards is not swapped, and draws nothing. */
    if ((dy < dx && x1 < x0) || (dx < dy && y1 < y0)) {
        swap = x0;
        x0 = x1;
        x1 = swap;
        swap = y0;
        y0 = y1;
        y1 = swap;
    }
    stepY = y1 < y0 ? -1 : 1;
    stepX = x1 < x0 ? -1 : 1;

    if (dy < dx) {
        x = x0;
        y = y0;
        error = 0;
        for (; x <= x1; x++) {
            if (error >= dx) {
                error -= dx;
                y += stepY;
            }
            if (g_bBrushSize == 0) {
                SetPixel4Bit(x, y, g_bDecalColor);
            } else {
                FillCircle4Bit(x, y, radius, FALSE);
            }
            error += dy;
        }
    } else {
        x = x0;
        y = y0;
        error = 0;
        for (; y <= y1; y++) {
            if (error >= dy) {
                error -= dy;
                x += stepX;
            }
            if (g_bBrushSize == 0) {
                SetPixel4Bit(x, y, g_bDecalColor);
            } else {
                FillCircle4Bit(x, y, radius, FALSE);
            }
            error += dx;
        }
    }
}

/* Write a texel at its byte. The color is not masked to 4 bits for the low nibble. */
#define PutTexel(offset, x, color)                                                                 \
    if ((x) & 1) {                                                                                 \
        g_pDecalTexels[offset] = (g_pDecalTexels[offset] & 0xF0) | (color);                        \
    } else {                                                                                       \
        g_pDecalTexels[offset] = (g_pDecalTexels[offset] & 0x0F) | ((color) << 4);                 \
    }

void SetPixel4Bit(s32 x, s32 y, u8 color) {
    s32 offset, maskByte;

    if (x >= 0 && x < g_bDecalWidth && y >= 0 && y < g_bDecalHeight) {
        offset = TexelOffset(x, y);
        /* The mask is a bit a texel, the first texel of a byte in its highest bit. */
        if (g_pDecalMask == NULL) {
            PutTexel(offset, x, color);
        } else {
            maskByte = (x >> 3) + y * ((g_bDecalWidth + 7) >> 3);
            if (!(g_pDecalMask[maskByte] & (1 << ((x ^ 7) & 7)))) {
                PutTexel(offset, x, color);
            }
        }
    }
}

void FillRect4Bit(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 swap, x, y;

    ScreenToTexel(x0, y0);
    ScreenToTexel(x1, y1);
    if (x1 < x0) {
        SWAP(x0, x1, swap);
    }
    if (y1 < y0) {
        SWAP(y0, y1, swap);
    }
    FillTexels(x, y, x0, y0, x1, y1);
}

void StrokeDecalRectangle(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 swap, x, half, y;

    ScreenToTexel(x0, y0);
    ScreenToTexel(x1, y1);
    if (x1 < x0) {
        SWAP(x0, x1, swap);
    }
    if (y1 < y0) {
        SWAP(y0, y1, swap);
    }
    half = g_aBrushSizeRects[g_bBrushSize].width >> 1;
    if (half == 0) {
        /* Only the edge at y1 includes the corner at x1. */
        FillTexelColumn(y, x0, y0, y1);
        FillTexelSpan(x, x0, x1, y1);
        FillTexelColumn(y, x1, y0, y1);
        FillTexelRow(x, x0, x1, y0);
        return;
    }
    FillTexels(x, y, x0 - half, y0 - half, x0 + half, y1 + half);
    FillTexels(x, y, x0, y1 - half, x1, y1 + half);
    FillTexels(x, y, x1 - half, y0 - half, x1 + half, y1 + half);
    FillTexels(x, y, x0, y0 - half, x1, y0 + half);
}

void FillDecalRoundRect(s32 x0, s32 y0, s32 screenX1, s32 y1) {
    s32 x1;
    s32 swap;
    s32 radius;
    s32 y;
    s32 x;
    s32 i;
    s32 j;
    s32 end;
    s32 start;
    s32 error;

    x0 = ScreenToTexelX(x0), y0 = ScreenToTexelY(y0);
    x1 = ScreenToTexelX(screenX1), y1 = ScreenToTexelY(y1);
    if (x1 < x0) {
        SWAP(x0, x1, swap);
    }
    if (y1 < y0) {
        SWAP(y0, y1, swap);
    }
    radius = (x1 - x0 < y1 - y0 ? x1 - x0 : y1 - y0) >> 2;
    for (y = y0 + radius; y < y1 - radius; y++) {
        for (x = x0; x < x1; x++) {
            SetPixel4Bit(x, y, g_bDecalColor);
        }
    }

    x0 += radius;
    y0 += radius;
    x1 -= radius;
    y1 -= radius;
    /* The corners are stepped at twice the resolution and drawn on the even steps. Most rows are
     * drawn twice. */
    j = radius * 2;
    i = 0;
    error = -j;
    for (; i <= j;) {
        if (!(i & 1)) {
            start = x0 - (i >> 1);
            end = (i >> 1) + x1;
            for (x = start; x < end; x++) {
                SetPixel4Bit(x, y0 - (j >> 1), g_bDecalColor);
            }
            start = x0 - (j >> 1);
            end = (j >> 1) + x1;
            for (x = start; x < end; x++) {
                SetPixel4Bit(x, y0 - (i >> 1), g_bDecalColor);
            }
            start = x0 - (i >> 1);
            end = (i >> 1) + x1;
            for (x = start; x < end; x++) {
                SetPixel4Bit(x, y1 + (j >> 1), g_bDecalColor);
            }
            start = x0 - (j >> 1);
            end = (j >> 1) + x1;
            for (x = start; x < end; x++) {
                SetPixel4Bit(x, y1 + (i >> 1), g_bDecalColor);
            }
        }
        error += i++;
        if (error >= 0) {
            error -= j--;
        }
    }
}

void DrawDecalRoundedRectOutline(int x0, int y0, int x1, int y1) {
    int swap;
    int halfBrush;
    int unusedFrameSlot; /* Retains the original stack frame size. */
    int x;
    int y;
    int radius;
    int stepX;
    int stepY;
    int run;
    int err;

    ScreenToTexel(x0, y0);
    ScreenToTexel(x1, y1);
    if (x1 < x0) {
        swap = x0;
        x0 = x1;
        x1 = swap;
    }
    if (y1 < y0) {
        swap = y0;
        y0 = y1;
        y1 = swap;
    }
    halfBrush = g_aBrushSizeRects[g_bBrushSize].width >> 1;
    radius = ((x1 - x0 < y1 - y0 ? x1 - x0 : y1 - y0) >> 2) + (halfBrush >> 1);
    if (halfBrush == 0) {
        for (y = y0 + radius; y < y1 - radius; y++) {
            SetPixel4Bit(x0, y, g_bDecalColor);
        }
        for (y = y0 + radius; y < y1 - radius; y++) {
            SetPixel4Bit(x1, y, g_bDecalColor);
        }
        for (x = x0 + radius; x < x1 - radius; x++) {
            SetPixel4Bit(x, y1, g_bDecalColor);
        }
        for (x = x0 + radius; x < x1 - radius; x++) {
            SetPixel4Bit(x, y0, g_bDecalColor);
        }
    } else {
        for (y = y0 + radius; y <= y1 - radius; y++) {
            for (x = x0 - halfBrush; x < x0 + halfBrush; x++) {
                SetPixel4Bit(x, y, g_bDecalColor);
            }
        }
        for (y = y0 + radius; y <= y1 - radius; y++) {
            for (x = x1 - halfBrush; x < x1 + halfBrush; x++) {
                SetPixel4Bit(x, y, g_bDecalColor);
            }
        }
        for (y = y1 - halfBrush; y < y1 + halfBrush; y++) {
            for (x = x0 + radius; x <= x1 - radius; x++) {
                SetPixel4Bit(x, y, g_bDecalColor);
            }
        }
        for (y = y0 - halfBrush; y < y0 + halfBrush; y++) {
            for (x = x0 + radius; x <= x1 - radius; x++) {
                SetPixel4Bit(x, y, g_bDecalColor);
            }
        }
    }
    x0 += radius;
    x1 -= radius;
    y0 += radius;
    y1 -= radius;
    if (g_bBrushSize > 2) {
        x0--;
        y0--;
    }
    radius += halfBrush >> 1;
    stepY = radius * 2;
    stepX = 0;
    err = -stepY;
    for (; stepX <= stepY;) {
        if (!(stepX & 1)) {
            if (halfBrush == 0) {
                SetPixel4Bit(x0 - (stepX >> 1), y0 - (stepY >> 1), g_bDecalColor);
            } else {
                for (run = 0; run < halfBrush * 2; run++) {
                    SetPixel4Bit(x0 - (stepX >> 1), y0 - (stepY >> 1) + run, g_bDecalColor);
                }
            }
            if (halfBrush == 0) {
                SetPixel4Bit(x0 - (stepX >> 1), (stepY >> 1) + y1, g_bDecalColor);
            } else {
                for (run = 0; run < halfBrush * 2; run++) {
                    SetPixel4Bit(x0 - (stepX >> 1), (stepY >> 1) + y1 - run, g_bDecalColor);
                }
            }
            if (halfBrush == 0) {
                SetPixel4Bit((stepX >> 1) + x1, (stepY >> 1) + y1, g_bDecalColor);
            } else {
                for (run = 0; run < halfBrush * 2; run++) {
                    SetPixel4Bit((stepX >> 1) + x1, (stepY >> 1) + y1 - run, g_bDecalColor);
                }
            }
            if (halfBrush == 0) {
                SetPixel4Bit((stepX >> 1) + x1, y0 - (stepY >> 1), g_bDecalColor);
            } else {
                for (run = 0; run < halfBrush * 2; run++) {
                    SetPixel4Bit((stepX >> 1) + x1, y0 - (stepY >> 1) + run, g_bDecalColor);
                }
            }
            if (halfBrush == 0) {
                SetPixel4Bit((stepY >> 1) + x1, (stepX >> 1) + y1, g_bDecalColor);
            } else {
                for (run = 0; run < halfBrush * 2; run++) {
                    SetPixel4Bit((stepY >> 1) + x1 - run, (stepX >> 1) + y1, g_bDecalColor);
                }
            }
            if (halfBrush == 0) {
                SetPixel4Bit((stepY >> 1) + x1, y0 - (stepX >> 1), g_bDecalColor);
            } else {
                for (run = 0; run < halfBrush * 2; run++) {
                    SetPixel4Bit((stepY >> 1) + x1 - run, y0 - (stepX >> 1), g_bDecalColor);
                }
            }
            if (halfBrush == 0) {
                SetPixel4Bit(x0 - (stepY >> 1), y0 - (stepX >> 1), g_bDecalColor);
            } else {
                for (run = 0; run < halfBrush * 2; run++) {
                    SetPixel4Bit(x0 - (stepY >> 1) + run, y0 - (stepX >> 1), g_bDecalColor);
                }
            }
            if (halfBrush == 0) {
                SetPixel4Bit(x0 - (stepY >> 1), (stepX >> 1) + y1, g_bDecalColor);
            } else {
                for (run = 0; run < halfBrush * 2; run++) {
                    SetPixel4Bit(x0 - (stepY >> 1) + run, (stepX >> 1) + y1, g_bDecalColor);
                }
            }
        }
        err += stepX++;
        if (err >= 0) {
            err -= stepY--;
        }
    }
}

void DrawDecalFilledEllipse(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 swap, radiusY, radiusX, x, y, error, centerX, centerY, endX, column;
    s32 squareY, squareX, stepSquareY, stepSquareX;

    ScreenToTexel(x0, y0);
    ScreenToTexel(x1, y1);
    if (x1 < x0) {
        SWAP(x0, x1, swap);
    }
    if (y1 < y0) {
        SWAP(y0, y1, swap);
    }
    radiusY = (y1 - y0) >> 1 >= 2 ? (y1 - y0) >> 1 : 1;
    radiusX = (x1 - x0) >> 1 >= 2 ? (x1 - x0) >> 1 : 1;
    squareY = stepSquareY = radiusY * radiusY;
    squareX = stepSquareX = radiusX * radiusX;
    y = radiusY * 2;
    x = 0;
    error = -squareX * y;
    while (stepSquareY * x <= stepSquareX * y) {
        /* The dead product sets the order of IDO's strength-reduced sums. */
        swap = stepSquareY * x;
        centerX = MIDPOINT(x0, x1);
        centerY = MIDPOINT(y0, y1);
        if (!(x & 1)) {
            endX = (x >> 1) + centerX;
            for (column = centerX - (x >> 1); column < endX; column++) {
                SetPixel4Bit(column, (y >> 1) + centerY, g_bDecalColor);
            }
            for (column = centerX - (x >> 1); column < endX; column++) {
                SetPixel4Bit(column, centerY - (y >> 1), g_bDecalColor);
            }
        }
        error += squareY * x;
        x++;
        error += squareY * x;
        if (error > 0) {
            error -= squareX * y;
            y--;
            error -= squareX * y;
        }
    }
    y = 0;
    x = radiusX * 2;
    error = -squareY * x;
    if (squareX * y <= x * squareY) {
        do {
            centerX = MIDPOINT(x0, x1);
            centerY = MIDPOINT(y0, y1);
            if (!(y & 1)) {
                endX = (x >> 1) + centerX;
                for (column = centerX - (x >> 1); column < endX; column++) {
                    SetPixel4Bit(column, (y >> 1) + centerY, g_bDecalColor);
                }
                for (column = centerX - (x >> 1); column < endX; column++) {
                    SetPixel4Bit(column, centerY - (y >> 1), g_bDecalColor);
                }
            }
            error += squareX * y;
            y++;
            error += squareX * y;
            if (error > 0) {
                error -= squareY * x;
                x--;
                error -= squareY * x;
            }
        } while (stepSquareX * y <= stepSquareY * x);
    }
}

void DrawDecalEllipseOutline(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 brush, radiusX, radiusY, squareX, squareY, stepSquareX, stepSquareY, x, y, error, stepX;
    s32 stepY, increment, swap, run;

    x0 = ScreenToTexelX(x0);
    x1 = ScreenToTexelX(x1);
    y0 = ScreenToTexelY(y0);
    y1 = ScreenToTexelY(y1);
    /* The corners are ordered by the sign of their difference here. */
    if (x1 - x0 < 0) {
        swap = x0;
        x0 = x1;
        x1 = swap;
    }
    if (y1 - y0 < 0) {
        swap = y0;
        y0 = y1;
        y1 = swap;
    }
    brush = g_aBrushSizeRects[g_bBrushSize].width;
    radiusY = (y1 - y0 + brush) >> 1;
    if (radiusY < 2) {
        radiusY = 1;
    }
    radiusX = (x1 - x0 + brush) >> 1;
    if (radiusX < 2) {
        radiusX = 1;
    }
    /* The steps use their own copies of the squares, as in the original. */
    squareY = stepSquareY = radiusY * radiusY;
    squareX = stepSquareX = radiusX * radiusX;

    /* The runs of the brush point into the ellipse. */
    x = 0;
    y = radiusY * 2;
    error = -squareX * (radiusY * 2);
    if (squareX * (radiusY * 2) >= 0) {
        stepY = stepSquareX * (radiusY * 2);
        stepX = 0;
        increment = 0;
        do {
            if ((x & 1) == 0) {
                PaintBrushRun(
                    run, MIDPOINT(x0, x1) + (x >> 1), MIDPOINT(y0, y1) + (y >> 1), 0, -1, brush);
                PaintBrushRun(
                    run, MIDPOINT(x0, x1) + (x >> 1), MIDPOINT(y0, y1) - (y >> 1), 0, 1, brush);
                PaintBrushRun(
                    run, MIDPOINT(x0, x1) - (x >> 1), MIDPOINT(y0, y1) - (y >> 1), 0, 1, brush);
                PaintBrushRun(
                    run, MIDPOINT(x0, x1) - (x >> 1), MIDPOINT(y0, y1) + (y >> 1), 0, -1, brush);
            }
            error += increment;
            stepX += stepSquareY;
            x++;
            increment += radiusY * radiusY;
            error += increment;
            if (error > 0) {
                error -= squareX * y;
                y--;
                stepY -= stepSquareX;
                error -= squareX * y;
            }
        } while (stepY >= stepX);
    }

    y = 0;
    x = radiusX * 2;
    error = -squareY * x;
    if (x * squareY < 0) {
        return;
    }
    stepX = stepSquareY * x;
    stepY = 0;
    increment = 0;
    do {
        if ((y & 1) == 0) {
            PaintBrushRun(
                run, MIDPOINT(x0, x1) + (x >> 1), MIDPOINT(y0, y1) + (y >> 1), -1, 0, brush);
            PaintBrushRun(
                run, MIDPOINT(x0, x1) + (x >> 1), MIDPOINT(y0, y1) - (y >> 1), -1, 0, brush);
            PaintBrushRun(
                run, MIDPOINT(x0, x1) - (x >> 1), MIDPOINT(y0, y1) - (y >> 1), 1, 0, brush);
            PaintBrushRun(
                run, MIDPOINT(x0, x1) - (x >> 1), MIDPOINT(y0, y1) + (y >> 1), 1, 0, brush);
        }
        error += increment;
        stepY += stepSquareX;
        y++;
        increment += radiusX * radiusX;
        error += increment;
        if (error > 0) {
            error -= squareY * x;
            x--;
            stepX -= stepSquareY;
            error -= squareY * x;
        }
    } while (stepX >= stepY);
}

void FillCircle4Bit(s32 x, s32 y, s32 radius, u8 fromScreen) {
    s32 i, end, unused, start, a, b, error;

    if (fromScreen) {
        x = ScreenToTexelX(x), y = ScreenToTexelY(y);
    }
    b = radius * 2;
    a = 0;
    error = -b;
    for (; a <= b;) {
        if (!(a & 1)) {
            start = x - (a >> 1);
            end = x + (a >> 1);
            for (i = start; i <= end; i++) {
                SetPixel4Bit(i, y + (b >> 1), g_bDecalColor);
            }
            start = x - (b >> 1);
            end = x + (b >> 1);
            for (i = start; i <= end; i++) {
                SetPixel4Bit(i, y + (a >> 1), g_bDecalColor);
            }
            start = x - (a >> 1);
            end = x + (a >> 1);
            for (i = start; i <= end; i++) {
                SetPixel4Bit(i, y - (b >> 1), g_bDecalColor);
            }
            start = x - (b >> 1);
            end = x + (b >> 1);
            for (i = start; i <= end; i++) {
                SetPixel4Bit(i, y - (a >> 1), g_bDecalColor);
            }
        }
        error += a++;
        if (error >= 0) {
            error -= b--;
        }
    }
}

void DrawDecalCircleOutline(s32 x, s32 y, s32 radius) {
    /* The unused local preserves the original frame layout. */
    s32 unused;
    s32 brush;
    s32 centerX;
    s32 centerY;
    s32 circleY;
    s32 circleX;
    s32 error;
    s32 run;

    centerX = ScreenToTexelX(x), centerY = ScreenToTexelY(y);
    brush = g_aBrushSizeRects[g_bBrushSize].width;
    radius += brush >> 1;
    circleY = radius * 2;
    circleX = 0;
    error = -circleY;
    while (circleX <= circleY) {
        if ((circleX & 1) == 0) {
            for (run = 0; run < brush; run++) {
                SetPixel4Bit(
                    (circleX >> 1) + centerX, (circleY >> 1) + centerY - run, g_bDecalColor);
            }
            for (run = 0; run < brush; run++) {
                SetPixel4Bit(
                    (circleX >> 1) + centerX, centerY - (circleY >> 1) + run, g_bDecalColor);
            }
            for (run = 0; run < brush; run++) {
                SetPixel4Bit(
                    centerX - (circleX >> 1), centerY - (circleY >> 1) + run, g_bDecalColor);
            }
            for (run = 0; run < brush; run++) {
                SetPixel4Bit(
                    centerX - (circleX >> 1), (circleY >> 1) + centerY - run, g_bDecalColor);
            }
            for (run = 0; run < brush; run++) {
                SetPixel4Bit(
                    (circleY >> 1) + centerX - run, (circleX >> 1) + centerY, g_bDecalColor);
            }
            for (run = 0; run < brush; run++) {
                SetPixel4Bit(
                    (circleY >> 1) + centerX - run, centerY - (circleX >> 1), g_bDecalColor);
            }
            for (run = 0; run < brush; run++) {
                SetPixel4Bit(
                    centerX - (circleY >> 1) + run, centerY - (circleX >> 1), g_bDecalColor);
            }
            for (run = 0; run < brush; run++) {
                SetPixel4Bit(
                    centerX - (circleY >> 1) + run, (circleX >> 1) + centerY, g_bDecalColor);
            }
        }
        error += circleX;
        circleX++;
        if (error >= 0) {
            error -= circleY;
            circleY--;
        }
    }
}

void DrawDashedLine(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 dx, dy, error, phase, x, y, startX, endX, startY, endY, stepX, stepY;
    /* The unused local preserves the original frame layout. */
    s32 unused;
    u8 color;

    phase = 0;
    startX = x0;
    startY = y0;
    endX = x1;
    endY = y1;
    dx = x1 - x0 > 0 ? x1 - x0 : -(x1 - x0);
    dy = y1 - y0 > 0 ? y1 - y0 : -(y1 - y0);
    AdvanceDashes();
    if ((dx >= dy && x1 < x0) || (dx < dy && y1 < y0)) {
        startX = x1;
        endX = x0;
        startY = y1;
        endY = y0;
    }
    stepY = endY - startY < 0 ? -1 : 1;
    stepX = endX - startX < 0 ? -1 : 1;

    /* Only the odd pixels along the line are drawn. */
    if (dx >= dy) {
        x = startX;
        y = startY;
        error = 0;
        for (; x <= endX; x++) {
            if (error >= dx) {
                error -= dx;
                y += stepY;
            }
            phase = (phase + 1) % DASH_PERIOD;
            color = DashColor(phase);
            if (x & 1) {
                DrawPixel(x, y, color);
            }
            error += dy;
        }
    } else {
        x = startX;
        y = startY;
        error = 0;
        for (; y <= endY; y++) {
            if (error >= dy) {
                error -= dy;
                x += stepX;
            }
            phase = (phase + 1) % DASH_PERIOD;
            color = DashColor(phase);
            if (y & 1) {
                DrawPixel(x, y, color);
            }
            error += dx;
        }
    }
    (void)unused;
}

void DrawDashedRectangle(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 x;
    u8 phase;
    s32 y;
    /* The unused bytes place the colour at its original stack offset. */
    u8 unusedA, unusedB;
    u8 color;
    s32 swap;

    phase = 0;
    AdvanceDashes();
#ifdef ENABLE_PATCHES
    /* With no dash drawn, the corners take the colour of the first phase. */
    color = AlternateDashColor(phase);
#else
    /* With no dash drawn, the corners read the unset colour, as the original does. */
#endif /* ENABLE_PATCHES */
    if (x1 < x0) {
        swap = x0;
        x0 = x1;
        x1 = swap;
    }
    if (y1 < y0) {
        swap = y0;
        y0 = y1;
        y1 = swap;
    }
    swap = y1 - 4;
    for (x = x0; x < x1 - 4; x += 4) {
        phase++;
        phase &= 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x, y0, 4, 1, color, color, color);
    }
    for (y = y0; y < y1 - 4; y += 4) {
        phase++;
        phase &= 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x1, y, 1, 4, color, color, color);
    }
    swap = y1 - 1;
    DrawFilledRectHiRes(x1, swap, 1, 2, color, color, color);
    for (x = x0; x < x1 - 4; x += 4) {
        phase++;
        phase &= 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x, y1, 4, 1, color, color, color);
    }
    for (y = y0; y < y1 - 4; y += 4) {
        phase++;
        phase &= 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x0, y, 1, 4, color, color, color);
    }
    DrawFilledRectHiRes(x0, swap, 1, 2, color, color, color);
    (void)unusedA;
    (void)unusedB;
}

void DrawDashedRoundedRect(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 swap, radius, x, y, i, j, error, a, unused;
    u8 phase, color;

    (void)unused;

    phase = 0;
    AdvanceDashes();
    if (x1 < x0) {
        swap = x0;
        x0 = x1;
        x1 = swap;
    }
    if (y1 < y0) {
        swap = y0;
        y0 = y1;
        y1 = swap;
    }
    radius = (x1 - x0 < y1 - y0 ? x1 - x0 : y1 - y0) >> 2;
    for (y = y0 + radius; y <= y1 - radius; y += 4) {
        phase++;
        phase &= 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x0, y, 1, 4, color, color, color);
    }
    for (y = y0 + radius; y <= y1 - radius; y += 4) {
        phase++;
        phase &= 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x1, y, 1, 4, color, color, color);
    }
    for (x = x0 + radius; x <= x1 - radius; x += 4) {
        phase++;
        phase &= 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x, y0, 4, 1, color, color, color);
    }
    for (x = x0 + radius; x <= x1 - radius; x += 4) {
        phase++;
        phase &= 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x, y1, 4, 1, color, color, color);
    }

    x0 += radius;
    x1 -= radius;
    y0 += radius;
    y1 -= radius;
    /* Each corner point is drawn only where its coordinate along the arc is odd. */
    j = radius * 2;
    i = 0;
    error = -j;
    while (i <= j) {
        if ((i & 1) == 0) {
            phase++;
            phase %= DASH_PERIOD;
            color = DashColor(phase);
            a = i >> 1;
            if ((x0 - a) & 1) {
                DrawPixel(x0 - a, y0 - (j >> 1), color);
            }
            if ((x0 - a) & 1) {
                DrawPixel(x0 - a, (j >> 1) + y1, color);
            }
            if ((a + x1) & 1) {
                DrawPixel(a + x1, (j >> 1) + y1, color);
            }
            if ((a + x1) & 1) {
                DrawPixel(a + x1, y0 - (j >> 1), color);
            }
            if ((a + y1) & 1) {
                DrawPixel((j >> 1) + x1, a + y1, color);
            }
            if ((y0 - a) & 1) {
                DrawPixel((j >> 1) + x1, y0 - a, color);
            }
            if ((y0 - a) & 1) {
                DrawPixel(x0 - (j >> 1), y0 - a, color);
            }
            if ((a + y1) & 1) {
                DrawPixel(x0 - (j >> 1), a + y1, color);
            }
        }
        error += i;
        i++;
        if (error >= 0) {
            error -= j;
            j--;
        }
    }
}

void DrawDashedEllipse(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 radiusX, radiusY, squareX, squareY, stepSquareX, stepSquareY, x, y, error, stepX, stepY;
    s32 increment, phase, swap;
    u8 color;

    phase = 0;
    if (x1 - x0 < 0) {
        swap = x0;
        x0 = x1;
        x1 = swap;
    }
    if (y1 - y0 < 0) {
        swap = y0;
        y0 = y1;
        y1 = swap;
    }
    radiusY = (y1 - y0) >> 1;
    if (radiusY < 2) {
        radiusY = 1;
    }
    radiusX = (x1 - x0) >> 1;
    if (radiusX < 2) {
        radiusX = 1;
    }
    /* The steps use their own copies of the squares, as in the original. */
    squareY = stepSquareY = radiusY * radiusY;
    squareX = stepSquareX = radiusX * radiusX;

    /* The points are drawn where X is odd in the first region and where Y is odd in the second. */
    x = 0;
    y = radiusY * 2;
    error = -squareX * y;
    AdvanceDashes();
    if (squareX * y >= 0) {
        stepY = stepSquareX * y;
        stepX = x * stepSquareY;
        increment = squareY * x;
        do {
            if ((x & 1) == 0) {
                phase = (phase + 1) % DASH_PERIOD;
                color = DashColor(phase);
                if ((MIDPOINT(x0, x1) + (x >> 1)) & 1) {
                    DrawPixel(MIDPOINT(x0, x1) + (x >> 1), MIDPOINT(y0, y1) + (y >> 1), color);
                }
                if ((MIDPOINT(x0, x1) + (x >> 1)) & 1) {
                    DrawPixel(MIDPOINT(x0, x1) + (x >> 1), MIDPOINT(y0, y1) - (y >> 1), color);
                }
                if ((MIDPOINT(x0, x1) - (x >> 1)) & 1) {
                    DrawPixel(MIDPOINT(x0, x1) - (x >> 1), MIDPOINT(y0, y1) - (y >> 1), color);
                }
                if ((MIDPOINT(x0, x1) - (x >> 1)) & 1) {
                    DrawPixel(MIDPOINT(x0, x1) - (x >> 1), MIDPOINT(y0, y1) + (y >> 1), color);
                }
            }
            error += increment;
            stepX += stepSquareY;
            x++;
            increment += radiusY * radiusY;
            error += increment;
            if (error > 0) {
                error -= squareX * y;
                y--;
                stepY -= stepSquareX;
                error -= squareX * y;
            }
        } while (stepY >= stepX);
    }

    y = 0;
    x = radiusX * 2;
    error = -squareY * x;
    if (x * squareY < 0) {
        return;
    }
    stepY = 0;
    increment = 0;
    stepX = x * stepSquareY;
    do {
        if ((y & 1) == 0) {
            phase = (phase + 1) % DASH_PERIOD;
            color = DashColor(phase);
            if ((MIDPOINT(y0, y1) + (y >> 1)) & 1) {
                DrawPixel(MIDPOINT(x0, x1) + (x >> 1), MIDPOINT(y0, y1) + (y >> 1), color);
            }
            if ((MIDPOINT(y0, y1) - (y >> 1)) & 1) {
                DrawPixel(MIDPOINT(x0, x1) + (x >> 1), MIDPOINT(y0, y1) - (y >> 1), color);
            }
            if ((MIDPOINT(y0, y1) - (y >> 1)) & 1) {
                DrawPixel(MIDPOINT(x0, x1) - (x >> 1), MIDPOINT(y0, y1) - (y >> 1), color);
            }
            if ((MIDPOINT(y0, y1) + (y >> 1)) & 1) {
                DrawPixel(MIDPOINT(x0, x1) - (x >> 1), MIDPOINT(y0, y1) + (y >> 1), color);
            }
        }
        error += increment;
        stepY += stepSquareX;
        y++;
        increment += radiusX * radiusX;
        error += increment;
        if (error > 0) {
            error -= squareY * x;
            x--;
            stepX -= stepSquareY;
            error -= squareY * x;
        }
    } while (stepX >= stepY);
}

void DrawDashedCircle(s32 x, s32 y, s32 radius) {
    s32 phase;
    u8 color;
    s32 j, i, left, top, right, bottom, px, error, py;

    /* The bounds of the decal are read once, before the dashes advance. */
    left = g_rcDecalCanvas.x;
    top = g_rcDecalCanvas.y;
    right = g_rcDecalCanvas.x + g_rcDecalCanvas.width;
    bottom = g_rcDecalCanvas.y + g_rcDecalCanvas.height;
    phase = 0;
    if ((++g_wScreenFrameCount & 7) == 0) {
        SwapBytes(&g_bDashColorA, &g_bDashColorB);
    }
    j = radius * 2;
    i = 0;
    error = -j;
    while (i <= j) {
        if ((i & 1) == 0) {
            phase++;
            phase %= DASH_PERIOD;
            color = DashColor(phase);
            px = (i >> 1) + x;
            py = (j >> 1) + y;
            if ((px & 1) && px >= left && px < right && py >= top && py < bottom) {
                DrawPixel(px, py, color);
            }
            px = (i >> 1) + x;
            py = y - (j >> 1);
            if ((px & 1) && px >= left && px < right && py >= top && py < bottom) {
                DrawPixel(px, py, color);
            }
            px = x - (i >> 1);
            py = y - (j >> 1);
            if ((px & 1) && px >= left && px < right && py >= top && py < bottom) {
                DrawPixel(px, py, color);
            }
            px = x - (i >> 1);
            py = (j >> 1) + y;
            if ((px & 1) && px >= left && px < right && py >= top && py < bottom) {
                DrawPixel(px, py, color);
            }
            px = (j >> 1) + x;
            py = (i >> 1) + y;
            if ((py & 1) && px >= left && px < right && py >= top && py < bottom) {
                DrawPixel(px, py, color);
            }
            px = (j >> 1) + x;
            py = y - (i >> 1);
            if ((py & 1) && px >= left && px < right && py >= top && py < bottom) {
                DrawPixel(px, py, color);
            }
            px = x - (j >> 1);
            py = y - (i >> 1);
            if ((py & 1) && px >= left && px < right && py >= top && py < bottom) {
                DrawPixel(px, py, color);
            }
            px = x - (j >> 1);
            py = (i >> 1) + y;
            if ((py & 1) && px >= left && px < right && py >= top && py < bottom) {
                DrawPixel(px, py, color);
            }
        }
        error += i;
        i++;
        if (error >= 0) {
            error -= j;
            j--;
        }
    }
}

void DrawDecalTextPreview(s32 x, s32 y) {
#ifdef ENABLE_PATCHES
    /* A first character missing from the font is drawn with the first glyph. */
    s32 i, entry, glyphIndex = 0, left, bottom, glyphLeft, advance;
#else
    s32 i, entry, glyphIndex, left, bottom, glyphLeft, advance;
#endif /* ENABLE_PATCHES */

    left = x - ((g_bDecalTextWidth >> 1) << 2);
    bottom = y - (g_texDecalFont.height << 2);
    for (i = 0; i < g_bDecalTextLength; i++) {
        /* A character missing from the font reuses the index of the glyph before, unset for the
         * first, as in the original. */
        for (entry = 0; entry < DECAL_GLYPHS; entry++) {
            if (g_apDecalTextGlyphs[i]->character == g_aDecalGlyphs[entry].character) {
                glyphIndex = entry;
                break;
            }
        }
        /* The glyph is tested at its cell moved right, but drawn moved left; the test at the
         * top of the decal is unsigned. */
        glyphLeft = left + (g_apDecalTextGlyphs[i]->cellX << 2);
        if (g_rcDecalCanvas.x < glyphLeft + (g_apDecalTextGlyphs[i]->width << 2) &&
            glyphLeft < g_rcDecalCanvas.x + g_rcDecalCanvas.width &&
            (u32)g_rcDecalCanvas.y < bottom + (g_texDecalFont.height << 2) &&
            bottom < g_rcDecalCanvas.y + g_rcDecalCanvas.height) {
            if (g_texPaintToolText.style) {
                DrawScaledTexturedRect(&g_texDecalFont,
                                       glyphIndex * FONT_CELL,
                                       0,
                                       FONT_CELL,
                                       FONT_CELL,
                                       left - (g_apDecalTextGlyphs[i]->cellX << 2) + 4,
                                       bottom + 4,
                                       FONT_CELL * 4,
                                       FONT_CELL * 4,
                                       g_aDecalSwatches[g_bTextShadowColor].color[0],
                                       g_aDecalSwatches[g_bTextShadowColor].color[1],
                                       g_aDecalSwatches[g_bTextShadowColor].color[2]);
            }
            DrawScaledTexturedRect(&g_texDecalFont,
                                   glyphIndex * FONT_CELL,
                                   0,
                                   FONT_CELL,
                                   FONT_CELL,
                                   left - (g_apDecalTextGlyphs[i]->cellX << 2),
                                   bottom,
                                   FONT_CELL * 4,
                                   FONT_CELL * 4,
                                   g_aDecalSwatches[g_bDecalColor].color[0],
                                   g_aDecalSwatches[g_bDecalColor].color[1],
                                   g_aDecalSwatches[g_bDecalColor].color[2]);
        }
        if (i < g_bDecalTextLength - 1) {
            advance = GlyphAdvance(g_apDecalTextGlyphs[i], g_apDecalTextGlyphs[i + 1]);
            left += advance << 2;
        }
    }
}

void SwapBytes(u8 *a, u8 *b) {
    u8 swap = *a;

    *a = *b;
    *b = swap;
}

void EmitScaledTextureRect(
    s32 textureWidth, s32 textureHeight, s32 x, s32 y, s32 width, s32 height) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetTextureFilter(gDisplayListPtr++, g_dwTextureFilterMode);
    gDPSetCombineLERP(
        gDisplayListPtr++, 0, 0, 0, TEXEL0, 0, 0, 0, SHADE, 0, 0, 0, TEXEL0, 0, 0, 0, SHADE);
    gDPSetRenderMode(gDisplayListPtr++,
                     FORCE_BL | CVG_DST_CLAMP | ZMODE_OPA |
                         GBL_c1(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1),
                     GBL_c2(G_BL_CLR_IN, G_BL_0, G_BL_CLR_IN, G_BL_1));
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_NONE);
    if (g_dwScreenScaleShift == 0) {
        x >>= 1;
        y >>= 1;
        width >>= 1;
        height >>= 1;
    }
    /* The rows are read from the last upwards, as the decal is stored. */
    /* IDO schedules the rectangle words correctly when emitted in a separate block. */
    {
        Gfx *rect = gDisplayListPtr++;

        rect->words.w0 = _SHIFTL(G_TEXRECT, 24, 8) | _SHIFTL((x + width) << 2, 12, 12) |
                         _SHIFTL((y + height) << 2, 0, 12);
        rect->words.w1 =
            _SHIFTL(G_TX_RENDERTILE, 24, 3) | _SHIFTL(x << 2, 12, 12) | _SHIFTL(y << 2, 0, 12);
    }
    gImmp1(gDisplayListPtr++, G_RDPHALF_1, _SHIFTL(0, 16, 16) | _SHIFTL(textureHeight << 5, 0, 16));
    gImmp1(gDisplayListPtr++,
           G_RDPHALF_2,
           _SHIFTL((textureWidth << 10) / width, 16, 16) |
               _SHIFTL(-(textureHeight << 10) / height, 0, 16));
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
}
