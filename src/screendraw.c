#include "screendraw.h"

#include <PR/gbi.h>

#include "displaylistptr.h"
#include "fade.h"
#include "screen.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/gbicommand.h"

s32 g_dwScreenScaleShift;
#endif

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/* Screen coordinate operations retain the original 32-bit wrap. */
#define COORD_ADD(a, b) ((s32)((u32)(a) + (u32)(b)))
#define COORD_SUB(a, b) ((s32)((u32)(a) - (u32)(b)))
#define COORD_DOUBLE(value) ((s32)((u32)(value) * 2u))
#define COORD_SHIFT(value, shift) ((s32)((u32)(value) << ((u32)(shift) & 31u)))
#define FILL_COLOR(red, green, blue)                                                               \
    ((u32)(u16)GPACK_RGBA5551((u32)(red), (u32)(green), (u32)(blue), 1) * 0x10001u)
#else
#define COORD_ADD(a, b) ((a) + (b))
#define COORD_SUB(a, b) ((a) - (b))
#define COORD_DOUBLE(value) ((value) * 2)
#define COORD_SHIFT(value, shift) ((value) << (shift))
/* A fill of the frame buffer with a colour of 8-bit components, as two pixels. */
#define FILL_COLOR(red, green, blue)                                                               \
    ((u16)GPACK_RGBA5551(red, green, blue, 1) | (u16)GPACK_RGBA5551(red, green, blue, 1) << 16)
#endif

/* A blend of the colour of the frame buffer and the colour drawn, by the alpha of the fog. */
#define RENDER_MODE_FOG_BLEND_C1                                                                   \
    (IM_RD | CVG_DST_FULL | ZMODE_OPA | FORCE_BL |                                                 \
     GBL_c1(G_BL_CLR_MEM, G_BL_A_FOG, G_BL_CLR_IN, G_BL_1MA))
#define RENDER_MODE_FOG_BLEND_C2 GBL_c2(G_BL_CLR_MEM, G_BL_A_FOG, G_BL_CLR_IN, G_BL_1MA)

void ClearScreen(s32 red, s32 green, s32 blue) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetFillColor(gDisplayListPtr++, FILL_COLOR(red, green, blue));
    gDPFillRectangle(gDisplayListPtr++,
                     0,
                     0,
                     COORD_SUB(COORD_SHIFT(g_dwScreenWidth, g_dwScreenScaleShift), 1),
                     COORD_SUB(COORD_SHIFT(g_dwScreenHeight, g_dwScreenScaleShift), 1));
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
}

void DrawFilledRect(s32 x, s32 y, s32 width, s32 height, s32 red, s32 green, s32 blue) {
    if (g_dwScreenScaleShift) {
        x = COORD_DOUBLE(x);
        y = COORD_DOUBLE(y);
        width = COORD_DOUBLE(width);
        height = COORD_DOUBLE(height);
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetFillColor(gDisplayListPtr++, FILL_COLOR(red, green, blue));
    /* The lower right corner is doubled a second time in the high resolution. */
    gDPFillRectangle(gDisplayListPtr++,
                     x,
                     y,
                     COORD_SUB(COORD_SHIFT(COORD_ADD(x, width), g_dwScreenScaleShift), 1),
                     COORD_SUB(COORD_SHIFT(COORD_ADD(y, height), g_dwScreenScaleShift), 1));
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
}

void SetScissorRect(s32 x, s32 y, s32 width, s32 height) {
    if (x < g_dwFadeRectLeft) {
        width = COORD_ADD(COORD_SUB(width, g_dwFadeRectLeft), x);
        x = g_dwFadeRectLeft;
    }
    if (g_dwFadeRectRight < COORD_ADD(x, width)) {
        width = COORD_SUB(g_dwFadeRectRight, x);
    }
    if (width < 0) {
        width = 0;
    }
    if (y < g_dwFadeRectTop) {
        height = COORD_ADD(COORD_SUB(height, g_dwFadeRectTop), y);
        y = g_dwFadeRectTop;
    }
    if (g_dwFadeRectBottom < COORD_ADD(y, height)) {
        height = COORD_SUB(g_dwFadeRectBottom, y);
    }
    if (height < 0) {
        height = 0;
    }
    if (g_dwScreenScaleShift) {
        x = COORD_DOUBLE(x);
        y = COORD_DOUBLE(y);
        width = COORD_DOUBLE(width);
        height = COORD_DOUBLE(height);
    }
    gDPPipeSync(gDisplayListPtr++);
    gDPSetScissor(
        gDisplayListPtr++, G_SC_NON_INTERLACE, x, y, COORD_ADD(x, width), COORD_ADD(y, height));
}

void DrawRdpOutlineRect(s32 x, s32 y, s32 width, s32 height) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
    gDPPipeSync(gDisplayListPtr++);
    /* With the alpha of the fog at 255, the blend retains the colour of the frame buffer. */
    gDPSetRenderMode(gDisplayListPtr++, RENDER_MODE_FOG_BLEND_C1, RENDER_MODE_FOG_BLEND_C2);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetFogColor(gDisplayListPtr++, 0, 0, 0, 0xFF);
    gDPFillRectangle(gDisplayListPtr++, x, y, COORD_ADD(x, width), COORD_ADD(y, 1));
    gDPFillRectangle(gDisplayListPtr++,
                     x,
                     COORD_SUB(COORD_ADD(y, height), 1),
                     COORD_ADD(x, width),
                     COORD_ADD(y, height));
    gDPFillRectangle(gDisplayListPtr++, x, y, COORD_ADD(x, 1), COORD_ADD(y, height));
    gDPFillRectangle(gDisplayListPtr++,
                     COORD_SUB(COORD_ADD(x, width), 1),
                     y,
                     COORD_ADD(x, width),
                     COORD_ADD(y, height));
#endif
}

void DrawRdpBorderRect(s32 x, s32 y, s32 width, s32 height) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetFillColor(gDisplayListPtr++, FILL_COLOR(0, 0, 0));
    gDPSetFogColor(gDisplayListPtr++, 0, 0, 0, 0xFF);
    gDPFillRectangle(
        gDisplayListPtr++, COORD_SUB(x, 1), COORD_SUB(y, 2), COORD_ADD(x, width), COORD_SUB(y, 1));
    gDPFillRectangle(gDisplayListPtr++,
                     COORD_SUB(x, 1),
                     COORD_ADD(y, height),
                     COORD_ADD(x, width),
                     COORD_ADD(COORD_ADD(y, height), 1));
    gDPFillRectangle(
        gDisplayListPtr++, COORD_SUB(x, 2), COORD_SUB(y, 1), COORD_SUB(x, 1), COORD_ADD(y, height));
    gDPFillRectangle(gDisplayListPtr++,
                     COORD_ADD(x, width),
                     COORD_SUB(y, 1),
                     COORD_ADD(COORD_ADD(x, width), 1),
                     COORD_ADD(y, height));
    gDPPipeSync(gDisplayListPtr++);
    gDPSetRenderMode(gDisplayListPtr++, RENDER_MODE_FOG_BLEND_C1, RENDER_MODE_FOG_BLEND_C2);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
    gDPSetFogColor(gDisplayListPtr++, 0, 0, 0, 0xFF);
    gDPFillRectangle(gDisplayListPtr++, x, y, COORD_ADD(x, width), COORD_ADD(y, 1));
    gDPFillRectangle(gDisplayListPtr++,
                     x,
                     COORD_SUB(COORD_ADD(y, height), 1),
                     COORD_ADD(x, width),
                     COORD_ADD(y, height));
    gDPFillRectangle(gDisplayListPtr++, x, y, COORD_ADD(x, 1), COORD_ADD(y, height));
    gDPFillRectangle(gDisplayListPtr++,
                     COORD_SUB(COORD_ADD(x, width), 1),
                     y,
                     COORD_ADD(x, width),
                     COORD_ADD(y, height));
    gDPFillRectangle(gDisplayListPtr++,
                     COORD_SUB(x, 3),
                     COORD_SUB(y, 3),
                     COORD_ADD(COORD_ADD(x, width), 3),
                     COORD_SUB(y, 2));
    gDPFillRectangle(gDisplayListPtr++,
                     COORD_SUB(x, 3),
                     COORD_ADD(COORD_ADD(y, height), 2),
                     COORD_ADD(COORD_ADD(x, width), 3),
                     COORD_ADD(COORD_ADD(y, height), 3));
    gDPFillRectangle(gDisplayListPtr++,
                     COORD_SUB(x, 3),
                     COORD_SUB(y, 3),
                     COORD_SUB(x, 2),
                     COORD_ADD(COORD_ADD(y, height), 3));
    gDPFillRectangle(gDisplayListPtr++,
                     COORD_ADD(COORD_ADD(x, width), 2),
                     COORD_SUB(y, 3),
                     COORD_ADD(COORD_ADD(x, width), 3),
                     COORD_ADD(COORD_ADD(y, height), 3));
}
