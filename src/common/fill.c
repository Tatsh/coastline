#include <PR/gbi.h>

#include "common/coordinates.h"
#include "common/displaylistptr.h"
#include "common/render.h"
#include "common/screen.h"
#include "common/screendraw.h"

void ClearScreen(s32 red, s32 green, s32 blue) {
#if !TGR_BOSS_RALLY
    u16 color;
    u32 *p;
#endif

    gDPPipeSync(gDisplayListPtr++);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
#if !TGR_BOSS_RALLY
    p = (u32 *)gDisplayListPtr++;
    p[0] = 0xba001402;
    p[1] = G_CYC_FILL;
#else
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
#endif
#if !TGR_BOSS_RALLY
    color = GPACK_RGBA5551(red, green, blue, 1);
    gDPSetFillColor(gDisplayListPtr++, color | color << 16);
#else
    gDPSetFillColor(gDisplayListPtr++, FILL_COLOR(red, green, blue));
#endif
    gDPFillRectangle(gDisplayListPtr++,
                     0,
                     0,
                     COORD_SUB(COORD_SHIFT(g_dwScreenWidth, g_dwScreenScaleShift), 1),
                     COORD_SUB(COORD_SHIFT(g_dwScreenHeight, g_dwScreenScaleShift), 1));
    gDPPipeSync(gDisplayListPtr++);
#if !TGR_BOSS_RALLY
    {
        u32 *q = (u32 *)gDisplayListPtr++;
        q[0] = 0xba001402;
        q[1] = G_CYC_1CYCLE;
    }
#else
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
#endif
}

void DrawFilledRect(s32 x, s32 y, s32 width, s32 height, s32 red, s32 green, s32 blue) {
#if !TGR_BOSS_RALLY
    u16 color;
    u32 *p;
#endif

    if (g_dwScreenScaleShift) {
        x = COORD_DOUBLE(x);
        y = COORD_DOUBLE(y);
        width = COORD_DOUBLE(width);
        height = COORD_DOUBLE(height);
    }
#if !TGR_BOSS_RALLY
    color = GPACK_RGBA5551(red, green, blue, 1);
#endif
    gDPPipeSync(gDisplayListPtr++);
    gDPSetRenderMode(gDisplayListPtr++, G_RM_OPA_SURF, G_RM_OPA_SURF2);
#if !TGR_BOSS_RALLY
    p = (u32 *)gDisplayListPtr++;
    p[0] = 0xba001402;
    p[1] = G_CYC_FILL;
    gDPSetFillColor(gDisplayListPtr++, color | color << 16);
#else
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetFillColor(gDisplayListPtr++, FILL_COLOR(red, green, blue));
#endif
    /* The lower right corner is doubled a second time in the high resolution. */
    gDPFillRectangle(gDisplayListPtr++,
                     x,
                     y,
                     COORD_SUB(COORD_SHIFT(COORD_ADD(x, width), g_dwScreenScaleShift), 1),
                     COORD_SUB(COORD_SHIFT(COORD_ADD(y, height), g_dwScreenScaleShift), 1));
    gDPPipeSync(gDisplayListPtr++);
#if !TGR_BOSS_RALLY
    p = (u32 *)gDisplayListPtr++;
    p[0] = 0xba001402;
    p[1] = G_CYC_1CYCLE;
#else
    gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
#endif
}
