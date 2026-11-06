#include "common/debugtext.h"

#include <PR/os.h>

#include "common/system.h"
#include "common/timing.h"
#include "common/track.h"
#include "common/util.h"

/* The colour of the outline around each pixel of a glyph: opaque black. */
#define OUTLINE_COLOR 0x0001

/* The rows a newline moves down before the shift of the frame buffer is added. */
#define NEWLINE_ROWS 8

/* The margin of the bitmap text, before the shift of the frame buffer is applied. */
#define TEXT_MARGIN 16

/* The microseconds of a frame at 60 Hz. */
#define FRAME_MICROSECONDS 16667

#define DRAW_BITMAP_DOT(index)                                                                     \
    if (rows[index]) {                                                                             \
        dst[index] = dst[index + 1] = dst[index + 2] = dst[stride + index] =                       \
            dst[stride + index + 2] = dst[stride + stride + index] =                               \
                dst[stride + stride + index + 1] = dst[stride + stride + index + 2] =              \
                    OUTLINE_COLOR;                                                                 \
    }

void DebugFrameStubA(void) {
}

void DrawBitmapTextToFramebuffer(const char *text) {
    u8 c;
    s32 stride;
    s16 *dst;
    s32 y;
    u16 color;
    u8 glyph;
    s32 x;
    s16 *rows;
    s32 i;

    stride = g_dwScreenWidth << g_dwScreenScaleShift;
    y = g_dwBitmapTextY;
    x = g_dwBitmapTextX;
    dst = (s16 *)g_apFramebuffers[g_dwFrameBufferIndex ^ 1];
    dst += y * stride;
    color = g_wBitmapTextColor;
    dst += x;
    for (; (c = *text); text++) {
        if (c >= 0x20 && c < 0x7f) {
            if (c >= 'a' && c <= 'z') {
                c -= 0x20;
            } else if (c > 'z' && c < 0x7f) {
                c -= 0x1a;
            }
            glyph = c - 0x20;
        } else if (c == '\n') {
            x = TEXT_MARGIN << (g_dwScreenScaleShift << 1);
            y += g_dwScreenScaleShift + NEWLINE_ROWS;
            dst += (g_dwScreenScaleShift + NEWLINE_ROWS) * stride;
            continue;
        } else {
            glyph = 0;
        }
        if (g_bBitmapTextNarrow) {
            rows = g_aBitmapFontNarrow[glyph][0];
            for (i = 0; i != 8; i++, dst += stride, rows += 3) {
                if (i < 7) {
                    DRAW_BITMAP_DOT(0)
                    DRAW_BITMAP_DOT(1)
                    DRAW_BITMAP_DOT(2)
                }
                if (i != 0) {
                    rows -= 3;
                    if (rows[0]) {
                        dst[1] = rows[0];
                    }
                    if (rows[1]) {
                        dst[2] = rows[1];
                    }
                    if (rows[2]) {
                        dst[3] = rows[2];
                    }
                    rows += 3;
                }
            }
            dst -= stride * 8 - g_dwScreenScaleShift - 4;
        } else {
            rows = g_aBitmapFontWide[glyph][0];
            for (i = 0; i != 8; i++, dst += stride, rows += 5) {
                if (i < 7) {
                    DRAW_BITMAP_DOT(0)
                    DRAW_BITMAP_DOT(1)
                    DRAW_BITMAP_DOT(2)
                    DRAW_BITMAP_DOT(3)
                    DRAW_BITMAP_DOT(4)
                }
                if (i != 0) {
                    rows -= 5;
                    if (rows[0]) {
                        dst[1] = color;
                    }
                    if (rows[1]) {
                        dst[2] = color;
                    }
                    if (rows[2]) {
                        dst[3] = color;
                    }
                    if (rows[3]) {
                        dst[4] = color;
                    }
                    if (rows[4]) {
                        dst[5] = color;
                    }
                    rows += 5;
                }
            }
            dst -= stride * 8 - g_dwScreenScaleShift - 6;
        }
    }
    g_dwBitmapTextX = x;
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
