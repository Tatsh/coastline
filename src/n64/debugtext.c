#include "debugtext.h"

#include <PR/os.h>

#include "system.h"
#include "timing.h"
#include "track.h"
#include "util.h"

/* The colour of the outline around each pixel of a glyph: opaque black. */
#define OUTLINE_COLOR 0x0001

/* The rows a newline moves down before the shift of the frame buffer is added. */
#define NEWLINE_ROWS 8

/* The margin of the bitmap text, before the shift of the frame buffer is applied. */
#define TEXT_MARGIN 16

/* The microseconds of a frame at 60 Hz. */
#define FRAME_MICROSECONDS 16667

void DebugFrameStubA(void) {
}

/*
 * Each pixel of a glyph takes the 3 by 3 block of outline around it, and the centre of the block is
 * drawn a row later so that the outline of the next row cannot cover it.
 */
void DrawBitmapTextToFramebuffer(const char *text) {
    s32 stride = g_dwScreenWidth << g_dwScreenScaleShift;
    s32 lineX = g_dwBitmapTextX;
    s32 y = g_dwBitmapTextY;
    u16 color = g_wBitmapTextColor;
    s16 *dst = (s16 *)g_apFramebuffers[g_dwFrameBufferIndex ^ 1] + y * stride + lineX;
    const u8 *p;
    const s16 *rows;
    u8 c;
    u8 glyph;
    s32 row;
    s32 column;

    for (p = (const u8 *)text; (c = *p) != '\0'; p++) {
        if (c >= ' ' && c < 0x7F) {
            if (c >= 'a' && c <= 'z') {
                glyph = c - ('a' - 'A' + ' ');
            } else {
                /* The font keeps {|}~ in the places of the lower case letters. The second
                 * comparison is always true here. */
                if (c >= '{' && c < 0x7F) {
                    c -= '{' - 'a';
                }
                glyph = c - ' ';
            }
        } else if (c == '\n') {
            /* The column goes back to the margin, but the drawing carries on from where it was. */
            lineX = TEXT_MARGIN << (g_dwScreenScaleShift * 2);
            y += g_dwScreenScaleShift + NEWLINE_ROWS;
            dst += (g_dwScreenScaleShift + NEWLINE_ROWS) * stride;
            continue;
        } else {
            glyph = 0;
        }

        if (g_bBitmapTextNarrow) {
            rows = g_aBitmapFontNarrow[glyph][0];
            for (row = 0; row <= BITMAP_FONT_ROWS; row++) {
                if (row < BITMAP_FONT_ROWS) {
                    for (column = 0; column < BITMAP_FONT_NARROW_COLUMNS; column++) {
                        if (rows[column] != 0) {
                            dst[column] = dst[column + 1] = dst[column + 2] = dst[stride + column] =
                                dst[stride + column + 2] = dst[stride * 2 + column] =
                                    dst[stride * 2 + column + 1] = dst[stride * 2 + column + 2] =
                                        OUTLINE_COLOR;
                        }
                    }
                }
                if (row != 0) {
                    for (column = 0; column < BITMAP_FONT_NARROW_COLUMNS; column++) {
                        if (rows[column - BITMAP_FONT_NARROW_COLUMNS] != 0) {
                            dst[column + 1] = rows[column - BITMAP_FONT_NARROW_COLUMNS];
                        }
                    }
                }
                dst += stride;
                rows += BITMAP_FONT_NARROW_COLUMNS;
            }
            dst -= stride * (BITMAP_FONT_ROWS + 1) - g_dwScreenScaleShift -
                   (BITMAP_FONT_NARROW_COLUMNS + 1);
        } else {
            rows = g_aBitmapFontWide[glyph][0];
            for (row = 0; row <= BITMAP_FONT_ROWS; row++) {
                if (row < BITMAP_FONT_ROWS) {
                    for (column = 0; column < BITMAP_FONT_WIDE_COLUMNS; column++) {
                        if (rows[column] != 0) {
                            dst[column] = dst[column + 1] = dst[column + 2] = dst[stride + column] =
                                dst[stride + column + 2] = dst[stride * 2 + column] =
                                    dst[stride * 2 + column + 1] = dst[stride * 2 + column + 2] =
                                        OUTLINE_COLOR;
                        }
                    }
                }
                if (row != 0) {
                    for (column = 0; column < BITMAP_FONT_WIDE_COLUMNS; column++) {
                        if (rows[column - BITMAP_FONT_WIDE_COLUMNS] != 0) {
                            dst[column + 1] = color;
                        }
                    }
                }
                dst += stride;
                rows += BITMAP_FONT_WIDE_COLUMNS;
            }
            dst -= stride * (BITMAP_FONT_ROWS + 1) - g_dwScreenScaleShift -
                   (BITMAP_FONT_WIDE_COLUMNS + 1);
        }
    }

    g_dwBitmapTextX = lineX;
    g_dwBitmapTextY = y;
}

void InitBitmapText(void) {
    s32 shift = g_dwScreenScaleShift;

    g_wBitmapTextColor = 0xFFFF;
    g_dwBitmapTextX = TEXT_MARGIN << (shift * 2);
    g_dwBitmapTextY = TEXT_MARGIN << shift;
    g_bBitmapTextNarrow = FALSE;
}

void BeginDebugStats(void) {
    s32 frameMicroseconds;
    f32 cyclesPerMillisecond;

    AddTimingMarker(0, 0xFF, 0xFF, 0xFF, 0xFF);

    /* The original computes these for a display that is no longer drawn, and discards them. */
    frameMicroseconds = g_dwFrameDeltaMs * 1000;
    while (frameMicroseconds >= 1000) {
        frameMicroseconds -= FRAME_MICROSECONDS;
    }
    cyclesPerMillisecond = (f32)(1000 * osClockRate / 1000000);
    cyclesPerMillisecond = (f32)(1000 * osClockRate / 1000000);
    (void)frameMicroseconds;
    (void)cyclesPerMillisecond;

    InitBitmapText();
}

void DebugFrameStubB(void) {
}
