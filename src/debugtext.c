#include "debugtext.h"

#include <PR/os.h>

#include "system.h"
#include "timing.h"
#include "track.h"
#include "util.h"

/* The colour of the outline around each pixel of a glyph: opaque black. */
#define OUTLINE_COLOUR 0x0001

/* The rows a newline moves down before the shift of the frame buffer is added. */
#define NEWLINE_ROWS 8

/* The margin of the bitmap text, before the shift of the frame buffer is applied. */
#define TEXT_MARGIN 16

/* The microseconds of a frame at 60 Hz. */
#define FRAME_MICROSECONDS 16667

void DebugFrameStubA(void) {
}

/*
 * Draw one glyph with its top-left outline corner at dst. Each pixel of the glyph takes the 3 by 3
 * block of outline around it, and the centre of the block is drawn a row later so that the outline
 * of the next row cannot cover it.
 */
static void
DrawBitmapGlyph(u16 *dst, s32 stride, const u16 *rows, s32 columns, s32 ownColours, u16 colour) {
    s32 row, column;
    u16 *block;

    for (row = 0; row <= BITMAP_FONT_ROWS; row++) {
        if (row < BITMAP_FONT_ROWS) {
            for (column = 0; column < columns; column++) {
                if (rows[column] != 0) {
                    block = dst + column;
                    block[0] = block[1] = block[2] = OUTLINE_COLOUR;
                    block[stride] = block[stride + 2] = OUTLINE_COLOUR;
                    block[stride * 2] = block[stride * 2 + 1] = block[stride * 2 + 2] =
                        OUTLINE_COLOUR;
                }
            }
        }
        if (row != 0) {
            for (column = 0; column < columns; column++) {
                if (rows[column - columns] != 0) {
                    dst[column + 1] = ownColours ? rows[column - columns] : colour;
                }
            }
        }
        dst += stride;
        rows += columns;
    }
}

void DrawBitmapTextToFramebuffer(const char *text) {
    s32 shift = g_dwScreenScaleShift;
    s32 stride = g_dwScreenWidth << shift;
    s32 lineX = g_dwBitmapTextX;
    s32 y = g_dwBitmapTextY;
    u16 colour = g_wBitmapTextColour;
    u16 *dst = g_apFramebuffers[g_dwFrameBufferIndex ^ 1] + y * stride + lineX;
    const u8 *p;
    u32 c;
    s32 glyph;

    for (p = (const u8 *)text; (c = *p) != '\0'; p++) {
        if (c >= ' ' && c < 0x7F) {
            if (c >= 'a' && c <= 'z') {
                glyph = c - 'a' + 'A' - ' ';
            } else if (c >= '{') {
                /* The font keeps {|}~ in the places of the lower case letters. */
                glyph = c - ('{' - 'a') - ' ';
            } else {
                glyph = c - ' ';
            }
        } else if (c == '\n') {
            /* The column goes back to the margin, but the drawing carries on from where it was. */
            lineX = TEXT_MARGIN << (shift * 2);
            y += NEWLINE_ROWS + shift;
            dst += (NEWLINE_ROWS + shift) * stride;
            continue;
        } else {
            glyph = 0;
        }

        if (g_bBitmapTextNarrow) {
            DrawBitmapGlyph(dst,
                            stride,
                            &g_aBitmapFontNarrow[glyph][0][0],
                            BITMAP_FONT_NARROW_COLUMNS,
                            TRUE,
                            colour);
            dst += BITMAP_FONT_NARROW_COLUMNS + 1 + shift;
        } else {
            DrawBitmapGlyph(dst,
                            stride,
                            &g_aBitmapFontWide[glyph][0][0],
                            BITMAP_FONT_WIDE_COLUMNS,
                            FALSE,
                            colour);
            dst += BITMAP_FONT_WIDE_COLUMNS + 1 + shift;
        }
    }

    g_dwBitmapTextX = lineX;
    g_dwBitmapTextY = y;
}

void InitBitmapText(void) {
    s32 shift = g_dwScreenScaleShift;

    g_wBitmapTextColour = 0xFFFF;
    g_dwBitmapTextX = TEXT_MARGIN << (shift * 2);
    g_dwBitmapTextY = TEXT_MARGIN << shift;
    g_bBitmapTextNarrow = FALSE;
}

void StepDebugHiddenInstance(void) {
    s32 step = g_nDebugInstanceCycleStep;
    s32 count, index, visible, i;

    if (step == 0) {
        return;
    }
    count = g_LoadedTrack.instanceCount;
    index = g_nDebugHiddenInstanceIndex;
    for (;;) {
        index += step;
        g_nDebugHiddenInstanceIndex = index;
        if (index >= count) {
            index = 0;
            g_nDebugHiddenInstanceIndex = 0;
        }
        if (index < 0) {
            index = count - 1;
            g_nDebugHiddenInstanceIndex = index;
        }
        if (index == 0) {
            break;
        }
        visible = g_nOpaqueVisibleInstanceCount;
        for (i = 0; i < visible && g_awVisibleInstanceList[i] != index; i++) {
        }
        if (i < visible) {
            break;
        }
    }
    g_nDebugInstanceCycleStep = 0;
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
