#include "decaldraw.h"

#include <PR/gbi.h>
#include <PR/os_libc.h>

#include "decaleditor.h"
#include "render.h"
#include "system.h"

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
    s32 y;
    s32 x1;
    s32 x2;
    s32 dy;
} FillSpan;

/* The texels of a rectangle given by two corners on the screen, each edge included. */
typedef struct {
    s32 minX;
    s32 minY;
    s32 maxX;
    s32 maxY;
} TexelBox;

/* The middle of two coordinates, worked out again at each use as the original does. */
#define MIDPOINT(a, b) (((a) + (b)) >> 1)
#define BOX_CENTER_X(box) MIDPOINT((box).minX, (box).maxX)
#define BOX_CENTER_Y(box) MIDPOINT((box).minY, (box).maxY)

static inline s32 ScreenToTexelX(s32 x) {
    return (x - g_rcDecalCanvas.x) >> 2;
}

static inline s32 ScreenToTexelY(s32 y) {
    return (g_rcDecalCanvas.y + g_rcDecalCanvas.height - y) >> 2;
}

/* The byte of a texel. The words of an odd row are swapped, as the RDP expects of a 4-bit
 * texture loaded in one block. */
static inline s32 TexelOffset(s32 x, s32 y) {
    return ((((y & 1) << 3) ^ x) >> 1) + y * (g_bDecalWidth >> 1);
}

static inline void ToTexelBox(TexelBox *box, s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 swap;

    box->minX = ScreenToTexelX(x0);
    box->maxX = ScreenToTexelX(x1);
    box->minY = ScreenToTexelY(y0);
    box->maxY = ScreenToTexelY(y1);
    if (box->maxX < box->minX) {
        swap = box->minX;
        box->minX = box->maxX;
        box->maxX = swap;
    }
    if (box->maxY < box->minY) {
        swap = box->minY;
        box->minY = box->maxY;
        box->maxY = swap;
    }
}

/* Paint the texels from (x0, y0) up to (x1, y1), excluding the far column and row. */
static void FillTexels(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 x, y;

    for (y = y0; y < y1; y++) {
        for (x = x0; x < x1; x++) {
            SetPixel4Bit(x, y, g_bDecalColor);
        }
    }
}

static void FillTexelRow(s32 x0, s32 x1, s32 y) {
    s32 x;

    for (x = x0; x < x1; x++) {
        SetPixel4Bit(x, y, g_bDecalColor);
    }
}

/* Paint the texels from x0 to x1 of a row, both included. */
static void FillTexelSpan(s32 x0, s32 x1, s32 y) {
    s32 x;

    for (x = x0; x <= x1; x++) {
        SetPixel4Bit(x, y, g_bDecalColor);
    }
}

static void FillTexelColumn(s32 x, s32 y0, s32 y1) {
    s32 y;

    for (y = y0; y < y1; y++) {
        SetPixel4Bit(x, y, g_bDecalColor);
    }
}

/* Paint a run of texels from a point in a direction, as long as the brush is wide, or the point
 * alone for the smallest brush. */
static void PaintRun(s32 x, s32 y, s32 dx, s32 dy, s32 half) {
    s32 i;

    if (half == 0) {
        SetPixel4Bit(x, y, g_bDecalColor);
        return;
    }
    for (i = 0; i < half * 2; i++) {
        SetPixel4Bit(x + i * dx, y + i * dy, g_bDecalColor);
    }
}

/* Paint a run of texels from a point in a direction, as long as the brush. */
static void PaintBrushRun(s32 x, s32 y, s32 dx, s32 dy, s32 brush) {
    s32 i;

    for (i = 0; i < brush; i++) {
        SetPixel4Bit(x + i * dx, y + i * dy, g_bDecalColor);
    }
}

static inline FillSpan *PushSpan(FillSpan *top, s32 y, s32 x1, s32 x2, s32 dy) {
    top->y = y;
    top->x1 = x1;
    top->x2 = x2;
    top->dy = dy;
    return top + 1;
}

/* The space from a glyph to the next. */
static inline s32 GlyphAdvance(DecalGlyph *glyph, DecalGlyph *next) {
    return glyph->width - (glyph->kernRight < next->kernLeft ? glyph->kernRight : next->kernLeft) +
           2;
}

/* A texel of the font, read without the swap of odd rows. */
static inline u8 FontTexel(s32 column, u32 row) {
    u8 pair = g_texDecalFont.pixels[(column >> 1) + row * (g_texDecalFont.width >> 1)];

    return (column & 1) ? (pair & 0xF) : (pair >> 4);
}

/* Count a frame of the dashes. Their colors swap every 8 frames. */
static inline void AdvanceDashes(void) {
    g_wScreenFrameCount++;
    if ((g_wScreenFrameCount & 7) == 0) {
        SwapBytes(&g_bDashColorA, &g_bDashColorB);
    }
}

static inline u8 DashColor(s32 phase) {
    return phase < DASH_LENGTH ? g_bDashColorA : g_bDashColorB;
}

static inline u8 AlternateDashColor(s32 phase) {
    return phase ? g_bDashColorA : g_bDashColorB;
}

u8 GetPixel4Bit(s32 x, s32 y) {
    s32 offset = TexelOffset(x, y);

    if (x & 1) {
        return g_pDecalTexels[offset] & 0xF;
    }
    return g_pDecalTexels[offset] >> 4;
}

u8 ReadPixel4Bit(u8 *texels, s32 x, s32 y) {
    s32 offset = TexelOffset(x, y);

    if (x & 1) {
        return texels[offset] & 0xF;
    }
    return texels[offset] >> 4;
}

void FloodFill4Bit(s32 x, s32 y) {
    FillSpan stack[FILL_SPANS];
    FillSpan *top;
    s32 count, left, x1, x2, dy;
    u8 *mask, *texels;
    u8 old, color;

    x = ScreenToTexelX(x);
    y = ScreenToTexelY(y);
    old = GetPixel4Bit(x, y);
    if (g_bDecalColor == old) {
        return;
    }
    stack[0].y = y;
    stack[0].x1 = x;
    stack[0].x2 = x;
    stack[0].dy = 1;
    stack[1].y = y + 1;
    stack[1].x1 = x;
    stack[1].x2 = x;
    stack[1].dy = -1;
    count = 2;

    /* The fill runs in a copy with the mask off, and is copied back through the mask. */
    mask = g_pDecalMask;
    g_pDecalMask = NULL;
    memcpy(g_pDecalScratchTexels, g_pDecalTexels, DECAL_BYTES);
    texels = g_pDecalTexels;
    g_pDecalTexels = g_pDecalScratchTexels;

    /* The pushes are not bounded, and more runs than the stack has room for overrun it. */
    top = &stack[count];
    do {
        top--;
        count--;
        /* The original reads the direction by the count and the rest through the pointer. */
        dy = stack[count].dy;
        y = top->y + dy;
        x1 = top->x1;
        x2 = top->x2;
        /* The scan to the left does not check the width. */
        for (x = x1; x >= 0 && y >= 0 && y < g_bDecalHeight && GetPixel4Bit(x, y) == old; x--) {
            SetPixel4Bit(x, y, g_bDecalColor);
        }
        if (x >= x1) {
            goto skip;
        }
        left = x + 1;
        if (left < x1) {
            top = PushSpan(top, y, left, x1 - 1, -dy);
            count++;
        }
        x = x1 + 1;
        do {
            for (; x < g_bDecalWidth && y >= 0 && y < g_bDecalHeight && GetPixel4Bit(x, y) == old;
                 x++) {
                SetPixel4Bit(x, y, g_bDecalColor);
            }
            top = PushSpan(top, y, left, x - 1, dy);
            count++;
            if (x2 + 1 < x) {
                top = PushSpan(top, y, x2 + 1, x - 1, -dy);
                count++;
            }
        skip:
            for (x++; x <= x2 && x < g_bDecalWidth && y >= 0 && y < g_bDecalHeight &&
                      GetPixel4Bit(x, y) != old;
                 x++) {
            }
            left = x;
        } while (x <= x2);
    } while (count > 0 && count < FILL_SPANS);

    g_pDecalMask = mask;
    for (y = 0; y < g_bDecalHeight; y++) {
        for (x = 0; x < g_bDecalWidth; x++) {
            color = GetPixel4Bit(x, y);
            g_pDecalTexels = texels;
            SetPixel4Bit(x, y, color);
            g_pDecalTexels = g_pDecalScratchTexels;
        }
    }
    g_pDecalTexels = texels;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuninitialized"
void StampTextOnDecal(s32 x, s32 y) {
    DecalGlyph **glyphs;
    DecalGlyph *glyph;
    s32 penX, baseY, i, entry, glyphIndex, column, firstColumn, lastColumn, texelX, texelY;
    u32 row;

    penX = ScreenToTexelX(x) + 1 - (g_bDecalTextWidth >> 1);
    baseY = ScreenToTexelY(y);
    for (i = 0, glyphs = g_apDecalTextGlyphs; i < g_bDecalTextLength; i++, glyphs++) {
        glyph = *glyphs;
        /* A character missing from the font reuses the index of the glyph before, unset for the
         * first, as in the original. */
        for (entry = 0; entry < DECAL_GLYPHS; entry++) {
            if (glyph->character == g_aDecalGlyphs[entry].character) {
                glyphIndex = entry;
                break;
            }
        }
        firstColumn = glyph->cellX + glyphIndex * FONT_CELL;
        lastColumn = glyph->width + firstColumn;

        /* The bounds are checked before the shadow is moved off by one texel. */
        if (g_texPaintToolText.style) {
            for (row = 0; row < g_texDecalFont.height; row++) {
                for (column = firstColumn; column <= lastColumn; column++) {
                    texelX = column - firstColumn + penX;
                    texelY = row + baseY;
                    if (texelX < 0 || texelX >= g_bDecalWidth || texelY < 0 ||
                        texelY >= g_bDecalHeight) {
                        continue;
                    }
                    if (FontTexel(column, row) == FONT_INK) {
                        SetPixel4Bit(texelX + 1, texelY - 1, g_bTextShadowColor);
                    }
                }
            }
        }
        for (row = 0; row < g_texDecalFont.height; row++) {
            for (column = firstColumn; column <= lastColumn; column++) {
                texelX = column - firstColumn + penX;
                texelY = row + baseY;
                if (texelX < 0 || texelX >= g_bDecalWidth || texelY < 0 ||
                    texelY >= g_bDecalHeight) {
                    continue;
                }
                if (FontTexel(column, row) == FONT_INK) {
                    SetPixel4Bit(texelX, texelY, g_bDecalColor);
                }
            }
        }

        if (i < g_bDecalTextLength - 1) {
            penX = penX + glyphs[0]->width -
                   (glyphs[0]->kernRight < glyphs[1]->kernLeft ? glyphs[0]->kernRight :
                                                                 glyphs[1]->kernLeft) +
                   2;
        }
    }
}
#pragma GCC diagnostic pop

void DrawLine4Bit(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 radius, dx, dy, stepX, stepY, x, y, error, swap;

    radius = g_aBrushSizeRects[g_bBrushSize].width >> 1;
    x0 = ScreenToTexelX(x0);
    y0 = ScreenToTexelY(y0);
    x1 = ScreenToTexelX(x1);
    y1 = ScreenToTexelY(y1);
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
    /* The original sets the start here and again for the first loop. */
    x = x0;
    y = y0;

    error = 0;
    if (dy < dx) {
        for (x = x0, y = y0; x <= x1; x++) {
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

void SetPixel4Bit(s32 x, s32 y, u8 color) {
    u8 *texel;
    s32 offset;

    if (x < 0 || x >= g_bDecalWidth || y < 0 || y >= g_bDecalHeight) {
        return;
    }
    offset = TexelOffset(x, y);
    /* The mask is a bit a texel, the first texel of a byte in its highest bit. */
    if (g_pDecalMask != NULL &&
        (g_pDecalMask[(x >> 3) + y * ((g_bDecalWidth + 7) >> 3)] & (1 << ((x ^ 7) & 7)))) {
        return;
    }
    texel = &g_pDecalTexels[offset];
    if (x & 1) {
        /* The color is not masked to 4 bits for the low nibble. */
        *texel = (*texel & 0xF0) | color;
    } else {
        *texel = (*texel & 0x0F) | (color << 4);
    }
}

void FillRect4Bit(s32 x0, s32 y0, s32 x1, s32 y1) {
    TexelBox box;

    ToTexelBox(&box, x0, y0, x1, y1);
    FillTexels(box.minX, box.minY, box.maxX, box.maxY);
}

void StrokeDecalRectangle(s32 x0, s32 y0, s32 x1, s32 y1) {
    TexelBox box;
    s32 half;

    ToTexelBox(&box, x0, y0, x1, y1);
    half = g_aBrushSizeRects[g_bBrushSize].width >> 1;
    if (half == 0) {
        /* Only the edge at maxY includes the corner at maxX. */
        FillTexelColumn(box.minX, box.minY, box.maxY);
        FillTexelRow(box.minX, box.maxX + 1, box.maxY);
        FillTexelColumn(box.maxX, box.minY, box.maxY);
        FillTexelRow(box.minX, box.maxX, box.minY);
        return;
    }
    FillTexels(box.minX - half, box.minY - half, box.minX + half, box.maxY + half);
    FillTexels(box.minX, box.maxY - half, box.maxX, box.maxY + half);
    FillTexels(box.maxX - half, box.minY - half, box.maxX + half, box.maxY + half);
    FillTexels(box.minX, box.minY - half, box.maxX, box.minY + half);
}

void FillDecalRoundRect(s32 x0, s32 y0, s32 x1, s32 y1) {
    TexelBox box;
    s32 radius, i, j, error, a, b;

    ToTexelBox(&box, x0, y0, x1, y1);
    radius =
        (box.maxX - box.minX < box.maxY - box.minY ? box.maxX - box.minX : box.maxY - box.minY) >>
        2;
    FillTexels(box.minX, box.minY + radius, box.maxX, box.maxY - radius);

    box.minX += radius;
    box.maxX -= radius;
    box.minY += radius;
    box.maxY -= radius;
    /* The corners are stepped at twice the resolution and drawn on the even steps. Most rows are
     * drawn twice. */
    j = radius * 2;
    i = 0;
    error = -j;
    if (j < 0) {
        return;
    }
    do {
        if ((i & 1) == 0) {
            a = i >> 1;
            b = j >> 1;
            FillTexelRow(box.minX - a, box.maxX + a, box.minY - b);
            FillTexelRow(box.minX - b, box.maxX + b, box.minY - a);
            FillTexelRow(box.minX - a, box.maxX + a, box.maxY + b);
            FillTexelRow(box.minX - b, box.maxX + b, box.maxY + a);
        }
        error += i;
        i++;
        if (error >= 0) {
            error -= j;
            j--;
        }
    } while (j >= i);
}

void DrawDecalRoundedRectOutline(s32 x0, s32 y0, s32 x1, s32 y1) {
    TexelBox box;
    s32 half, quarter, radius, i, j, error, a, b, x, y;

    ToTexelBox(&box, x0, y0, x1, y1);
    half = g_aBrushSizeRects[g_bBrushSize].width >> 1;
    quarter = half >> 1;
    radius =
        ((box.maxX - box.minX < box.maxY - box.minY ? box.maxX - box.minX : box.maxY - box.minY) >>
         2) +
        quarter;
    if (half == 0) {
        FillTexelColumn(box.minX, box.minY + radius, box.maxY - radius);
        FillTexelColumn(box.maxX, box.minY + radius, box.maxY - radius);
        FillTexelRow(box.minX + radius, box.maxX - radius, box.maxY);
        FillTexelRow(box.minX + radius, box.maxX - radius, box.minY);
    } else {
        /* The thick edges extend one texel further than the thin ones. */
        for (y = box.minY + radius; y <= box.maxY - radius; y++) {
            FillTexelRow(box.minX - half, box.minX + half, y);
        }
        for (y = box.minY + radius; y <= box.maxY - radius; y++) {
            FillTexelRow(box.maxX - half, box.maxX + half, y);
        }
        for (y = box.maxY - half; y < box.maxY + half; y++) {
            for (x = box.minX + radius; x <= box.maxX - radius; x++) {
                SetPixel4Bit(x, y, g_bDecalColor);
            }
        }
        for (y = box.minY - half; y < box.minY + half; y++) {
            for (x = box.minX + radius; x <= box.maxX - radius; x++) {
                SetPixel4Bit(x, y, g_bDecalColor);
            }
        }
    }

    box.minX += radius;
    box.maxX -= radius;
    box.minY += radius;
    box.maxY -= radius;
    /* The two largest brushes move the corners on the left and at minY by a texel. */
    if (g_bBrushSize >= 3) {
        box.minX--;
        box.minY--;
    }
    radius += quarter;
    j = radius * 2;
    i = 0;
    error = -j;
    if (j < 0) {
        return;
    }
    do {
        if ((i & 1) == 0) {
            a = i >> 1;
            b = j >> 1;
            PaintRun(box.minX - a, box.minY - b, 0, 1, half);
            PaintRun(box.minX - a, box.maxY + b, 0, -1, half);
            PaintRun(box.maxX + a, box.maxY + b, 0, -1, half);
            PaintRun(box.maxX + a, box.minY - b, 0, 1, half);
            PaintRun(box.maxX + b, box.maxY + a, -1, 0, half);
            PaintRun(box.maxX + b, box.minY - a, -1, 0, half);
            PaintRun(box.minX - b, box.minY - a, 1, 0, half);
            PaintRun(box.minX - b, box.maxY + a, 1, 0, half);
        }
        error += i;
        i++;
        if (error >= 0) {
            error -= j;
            j--;
        }
    } while (j >= i);
}

void DrawDecalFilledEllipse(s32 x0, s32 y0, s32 x1, s32 y1) {
    TexelBox box;
    s32 radiusX, radiusY, squareX, squareY, stepSquareX, stepSquareY, x, y, error, stepX, stepY;
    s32 increment;

    ToTexelBox(&box, x0, y0, x1, y1);
    radiusY = (box.maxY - box.minY) >> 1;
    if (radiusY < 2) {
        radiusY = 1;
    }
    radiusX = (box.maxX - box.minX) >> 1;
    if (radiusX < 2) {
        radiusX = 1;
    }
    /* The steps use their own copies of the squares, as in the original. */
    squareY = stepSquareY = radiusY * radiusY;
    squareX = stepSquareX = radiusX * radiusX;

    /* Both regions step at twice the resolution and draw on the even steps. The step adds the
     * square of the radius where the midpoint method adds twice it. */
    x = 0;
    y = radiusY * 2;
    error = -squareX * (radiusY * 2);
    if (squareX * (radiusY * 2) >= 0) {
        stepY = stepSquareX * (radiusY * 2);
        stepX = 0;
        increment = 0;
        do {
            if ((x & 1) == 0) {
                FillTexelRow(BOX_CENTER_X(box) - (x >> 1),
                             BOX_CENTER_X(box) + (x >> 1),
                             (y >> 1) + BOX_CENTER_Y(box));
                FillTexelRow(BOX_CENTER_X(box) - (x >> 1),
                             BOX_CENTER_X(box) + (x >> 1),
                             BOX_CENTER_Y(box) - (y >> 1));
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
            FillTexelRow(BOX_CENTER_X(box) - (x >> 1),
                         BOX_CENTER_X(box) + (x >> 1),
                         (y >> 1) + BOX_CENTER_Y(box));
            FillTexelRow(BOX_CENTER_X(box) - (x >> 1),
                         BOX_CENTER_X(box) + (x >> 1),
                         BOX_CENTER_Y(box) - (y >> 1));
        }
        error += increment;
        y++;
        stepY += stepSquareX;
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

void DrawDecalEllipseOutline(s32 x0, s32 y0, s32 x1, s32 y1) {
    TexelBox box;
    s32 brush, radiusX, radiusY, squareX, squareY, stepSquareX, stepSquareY, x, y, error, stepX;
    s32 stepY, increment, swap;

    box.minX = ScreenToTexelX(x0);
    box.maxX = ScreenToTexelX(x1);
    box.minY = ScreenToTexelY(y0);
    box.maxY = ScreenToTexelY(y1);
    /* The corners are ordered by the sign of their difference here. */
    if (box.maxX - box.minX < 0) {
        swap = box.minX;
        box.minX = box.maxX;
        box.maxX = swap;
    }
    if (box.maxY - box.minY < 0) {
        swap = box.minY;
        box.minY = box.maxY;
        box.maxY = swap;
    }
    brush = g_aBrushSizeRects[g_bBrushSize].width;
    radiusY = (box.maxY - box.minY + brush) >> 1;
    if (radiusY < 2) {
        radiusY = 1;
    }
    radiusX = (box.maxX - box.minX + brush) >> 1;
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
                    BOX_CENTER_X(box) + (x >> 1), BOX_CENTER_Y(box) + (y >> 1), 0, -1, brush);
                PaintBrushRun(
                    BOX_CENTER_X(box) + (x >> 1), BOX_CENTER_Y(box) - (y >> 1), 0, 1, brush);
                PaintBrushRun(
                    BOX_CENTER_X(box) - (x >> 1), BOX_CENTER_Y(box) - (y >> 1), 0, 1, brush);
                PaintBrushRun(
                    BOX_CENTER_X(box) - (x >> 1), BOX_CENTER_Y(box) + (y >> 1), 0, -1, brush);
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
            PaintBrushRun(BOX_CENTER_X(box) + (x >> 1), BOX_CENTER_Y(box) + (y >> 1), -1, 0, brush);
            PaintBrushRun(BOX_CENTER_X(box) + (x >> 1), BOX_CENTER_Y(box) - (y >> 1), -1, 0, brush);
            PaintBrushRun(BOX_CENTER_X(box) - (x >> 1), BOX_CENTER_Y(box) - (y >> 1), 1, 0, brush);
            PaintBrushRun(BOX_CENTER_X(box) - (x >> 1), BOX_CENTER_Y(box) + (y >> 1), 1, 0, brush);
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
    s32 i, j, error, a, b;

    if (fromScreen) {
        x = ScreenToTexelX(x);
        y = ScreenToTexelY(y);
    }
    i = 0;
    j = radius * 2;
    error = -j;
    if (j < 0) {
        return;
    }
    do {
        if ((i & 1) == 0) {
            a = i >> 1;
            b = j >> 1;
            FillTexelSpan(x - a, x + a, y + b);
            FillTexelSpan(x - b, x + b, y + a);
            FillTexelSpan(x - a, x + a, y - b);
            FillTexelSpan(x - b, x + b, y - a);
        }
        error += i;
        i++;
        if (error >= 0) {
            error -= j;
            j--;
        }
    } while (i <= j);
}

void DrawDecalCircleOutline(s32 x, s32 y, s32 radius) {
    s32 brush, i, j, error, a, b;

    brush = g_aBrushSizeRects[g_bBrushSize].width;
    x = ScreenToTexelX(x);
    y = ScreenToTexelY(y);
    j = (radius + (brush >> 1)) * 2;
    i = 0;
    error = -j;
    if (j < 0) {
        return;
    }
    do {
        if ((i & 1) == 0) {
            a = i >> 1;
            b = j >> 1;
            PaintBrushRun(x + a, y + b, 0, -1, brush);
            PaintBrushRun(x + a, y - b, 0, 1, brush);
            PaintBrushRun(x - a, y - b, 0, 1, brush);
            PaintBrushRun(x - a, y + b, 0, -1, brush);
            PaintBrushRun(x + b, y + a, -1, 0, brush);
            PaintBrushRun(x + b, y - a, -1, 0, brush);
            PaintBrushRun(x - b, y - a, 1, 0, brush);
            PaintBrushRun(x - b, y + a, 1, 0, brush);
        }
        error += i;
        i++;
        if (error >= 0) {
            error -= j;
            j--;
        }
    } while (i <= j);
}

void DrawDashedLine(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 dx, dy, startX, startY, endX, endY, stepX, stepY, x, y, error, phase;

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

    /* The original sets the start here and again for the first loop. */
    x = startX;

    /* Only the odd pixels along the line are drawn. */
    error = 0;
    if (dx >= dy) {
        for (x = startX, y = startY; x <= endX; x++) {
            if (error >= dx) {
                error -= dx;
                y += stepY;
            }
            phase = (phase + 1) % DASH_PERIOD;
            if (x & 1) {
                DrawPixel(x, y, DashColor(phase));
            }
            error += dy;
        }
    } else {
        for (y = startY; y <= endY; y++) {
            if (error >= dy) {
                error -= dy;
                x += stepX;
            }
            phase = (phase + 1) % DASH_PERIOD;
            if (y & 1) {
                DrawPixel(x, y, DashColor(phase));
            }
            error += dx;
        }
    }
}

/* With no dash drawn, the corners read the unset colour, as the original does. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuninitialized"
void DrawDashedRectangle(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 phase, x, y, swap;
    u8 color;

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
    for (x = x0; x < x1 - 4; x += 4) {
        phase = (phase + 1) & 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x, y0, 4, 1, color, color, color);
    }
    for (y = y0; y < y1 - 4; y += 4) {
        phase = (phase + 1) & 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x1, y, 1, 4, color, color, color);
    }
    DrawFilledRectHiRes(x1, y1 - 1, 1, 2, color, color, color);
    for (x = x0; x < x1 - 4; x += 4) {
        phase = (phase + 1) & 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x, y1, 4, 1, color, color, color);
    }
    for (y = y0; y < y1 - 4; y += 4) {
        phase = (phase + 1) & 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x0, y, 1, 4, color, color, color);
    }
    DrawFilledRectHiRes(x0, y1 - 1, 1, 2, color, color, color);
}
#pragma GCC diagnostic pop

void DrawDashedRoundedRect(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 radius, i, j, error, a, b, x, y, swap;
    u8 phase, color;

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
        phase = (phase + 1) & 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x0, y, 1, 4, color, color, color);
    }
    for (y = y0 + radius; y <= y1 - radius; y += 4) {
        phase = (phase + 1) & 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x1, y, 1, 4, color, color, color);
    }
    for (x = x0 + radius; x <= x1 - radius; x += 4) {
        phase = (phase + 1) & 1;
        color = AlternateDashColor(phase);
        DrawFilledRectHiRes(x, y0, 4, 1, color, color, color);
    }
    for (x = x0 + radius; x <= x1 - radius; x += 4) {
        phase = (phase + 1) & 1;
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
    if (j < 0) {
        return;
    }
    do {
        if ((i & 1) == 0) {
            phase = (u8)(phase + 1) % DASH_PERIOD;
            color = DashColor(phase);
            a = i >> 1;
            b = j >> 1;
            if ((x0 - a) & 1) {
                DrawPixel(x0 - a, y0 - b, color);
            }
            if ((x0 - a) & 1) {
                DrawPixel(x0 - a, y1 + b, color);
            }
            if ((x1 + a) & 1) {
                DrawPixel(x1 + a, y1 + b, color);
            }
            if ((x1 + a) & 1) {
                DrawPixel(x1 + a, y0 - b, color);
            }
            if ((y1 + a) & 1) {
                DrawPixel(x1 + b, y1 + a, color);
            }
            if ((y0 - a) & 1) {
                DrawPixel(x1 + b, y0 - a, color);
            }
            if ((y0 - a) & 1) {
                DrawPixel(x0 - b, y0 - a, color);
            }
            if ((y1 + a) & 1) {
                DrawPixel(x0 - b, y1 + a, color);
            }
        }
        error += i;
        i++;
        if (error >= 0) {
            error -= j;
            j--;
        }
    } while (i <= j);
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

/* Draw a point of the dashed circle where it is odd along the arc and over the decal. */
static inline void
DrawDashedCirclePoint(s32 x, s32 y, s32 along, u8 color, s32 left, s32 top, s32 right, s32 bottom) {
    if ((along & 1) && x >= left && x < right && y >= top && y < bottom) {
        DrawPixel(x, y, color);
    }
}

void DrawDashedCircle(s32 x, s32 y, s32 radius) {
    s32 phase, i, j, error, a, b, left, top, right, bottom;
    u8 color;

    /* The bounds of the decal are read once, before the dashes advance. */
    left = g_rcDecalCanvas.x;
    top = g_rcDecalCanvas.y;
    right = left + g_rcDecalCanvas.width;
    bottom = top + g_rcDecalCanvas.height;
    phase = 0;
    AdvanceDashes();
    i = 0;
    j = radius * 2;
    error = -j;
    if (j < 0) {
        return;
    }
    do {
        if ((i & 1) == 0) {
            phase = (phase + 1) % DASH_PERIOD;
            color = DashColor(phase);
            a = i >> 1;
            b = j >> 1;
            DrawDashedCirclePoint(x + a, y + b, x + a, color, left, top, right, bottom);
            DrawDashedCirclePoint(x + a, y - b, x + a, color, left, top, right, bottom);
            DrawDashedCirclePoint(x - a, y - b, x - a, color, left, top, right, bottom);
            DrawDashedCirclePoint(x - a, y + b, x - a, color, left, top, right, bottom);
            DrawDashedCirclePoint(x + b, y + a, y + a, color, left, top, right, bottom);
            DrawDashedCirclePoint(x + b, y - a, y - a, color, left, top, right, bottom);
            DrawDashedCirclePoint(x - b, y - a, y - a, color, left, top, right, bottom);
            DrawDashedCirclePoint(x - b, y + a, y + a, color, left, top, right, bottom);
        }
        error += i;
        i++;
        if (error >= 0) {
            error -= j;
            j--;
        }
    } while (i <= j);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#pragma GCC diagnostic ignored "-Wuninitialized"
void DrawDecalTextPreview(s32 x, s32 y) {
    DecalGlyph *glyph;
    PaletteSwatch *swatch;
    s32 left, bottom, i, entry, glyphIndex;

    left = x - ((g_bDecalTextWidth >> 1) << 2);
    bottom = y - (g_texDecalFont.height << 2);
    for (i = 0; i < g_bDecalTextLength; i++) {
        glyph = g_apDecalTextGlyphs[i];
        /* A character missing from the font reuses the index of the glyph before, unset for the
         * first, as in the original. */
        for (entry = 0; entry < DECAL_GLYPHS; entry++) {
            if (glyph->character == g_aDecalGlyphs[entry].character) {
                glyphIndex = entry;
                break;
            }
        }
        /* The glyph is tested at its cell moved right, but drawn moved left; the test at the
         * top of the decal is unsigned. */
        if (g_rcDecalCanvas.x < left + (glyph->cellX << 2) + (glyph->width << 2) &&
            left + (glyph->cellX << 2) < g_rcDecalCanvas.x + g_rcDecalCanvas.width &&
            (u32)g_rcDecalCanvas.y < bottom + (g_texDecalFont.height << 2) &&
            bottom < g_rcDecalCanvas.y + g_rcDecalCanvas.height) {
            if (g_texPaintToolText.style) {
                swatch = &g_aDecalSwatches[g_bTextShadowColor];
                DrawScaledTexturedRect(&g_texDecalFont,
                                       glyphIndex * FONT_CELL,
                                       0,
                                       FONT_CELL,
                                       FONT_CELL,
                                       left - (g_apDecalTextGlyphs[i]->cellX << 2) + 4,
                                       bottom + 4,
                                       FONT_CELL * 4,
                                       FONT_CELL * 4,
                                       swatch->color[0],
                                       swatch->color[1],
                                       swatch->color[2]);
            }
            swatch = &g_aDecalSwatches[g_bDecalColor];
            DrawScaledTexturedRect(&g_texDecalFont,
                                   glyphIndex * FONT_CELL,
                                   0,
                                   FONT_CELL,
                                   FONT_CELL,
                                   left - (g_apDecalTextGlyphs[i]->cellX << 2),
                                   bottom,
                                   FONT_CELL * 4,
                                   FONT_CELL * 4,
                                   swatch->color[0],
                                   swatch->color[1],
                                   swatch->color[2]);
        }
        if (i < g_bDecalTextLength - 1) {
            left += GlyphAdvance(g_apDecalTextGlyphs[i], g_apDecalTextGlyphs[i + 1]) << 2;
        }
    }
}
#pragma GCC diagnostic pop

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
    gSPTextureRectangle(gDisplayListPtr++,
                        x << 2,
                        y << 2,
                        (x + width) << 2,
                        (y + height) << 2,
                        G_TX_RENDERTILE,
                        0,
                        textureHeight << 5,
                        (textureWidth << 10) / width,
                        -(textureHeight << 10) / height);
    gDPPipeSync(gDisplayListPtr++);
    gDPSetTexturePersp(gDisplayListPtr++, G_TP_PERSP);
}
