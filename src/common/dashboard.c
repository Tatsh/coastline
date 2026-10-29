#include "common/camera.h"
#include "common/cameraprojection.h"
#include "common/game.h"
#include "common/hud.h"
#include "common/pause.h"
#include "common/race.h"
#include "common/render.h"
#include "common/screen.h"
#include "common/util.h"
#include "common/vecmath.h"
#include "common/vehicleaccess.h"
#include "common/weather.h"

#if TGR_PC
#include "win32/directplay.h"
#include "win32/hudcanvas.h"
#include "win32/hudquad.h"
#include "win32/render.h"
#include "win32/settings.h"
#else
#include <PR/gu.h>
#include <PR/os_libc.h>

#include "common/input.h"
#include "common/system.h"
#include "common/text.h"
#include "options.h"
#endif /* TGR_PC */

#if TGR_PC && defined(ENABLE_PATCHES)
#define g_dwConfigFramebufferWidth GetHudCanvasWidth()
#define g_dwConfigFramebufferHeight GetHudCanvasHeight()
#define g_dwScreenWidth GetHudCanvasWidth()
#define g_dwScreenHeight GetHudCanvasHeight()
#endif

#if TGR_PC
#define DASHBOARD_ADD(a, b) ((s32)((u32)(a) + (u32)(b)))
#define DASHBOARD_AXIS_0 x
#define DASHBOARD_AXIS_1 y
#define DASHBOARD_AXIS_2 z
#define DASHBOARD_VERTEX(vertex, axis) ((vertex).position.DASHBOARD_AXIS_##axis)
#define DASHBOARD_COORD(value) ((s16)(s64)(value))
#define DASHBOARD_COS DashboardCos
#define DASHBOARD_SIN DashboardSin
static inline long double DashboardCos(long double angle) {
    long double result;
    __asm__("fcos" : "=t"(result) : "0"(angle));
    return result;
}
static inline long double DashboardSin(long double angle) {
    long double result;
    __asm__("fsin" : "=t"(result) : "0"(angle));
    return result;
}
namespace {
    /** @ghidraAddress BRD3D: 0x104afd20 */
    ConvertedModelVertex g_aGaugeNeedleVerts[2][DASHBOARD_VIEWS][4];
}
s32 g_adwDashboardLoadedGear[DASHBOARD_VIEWS] = {-1, -1};
s32 g_bHudShowSpeedometer = 1;
#else

#define DASHBOARD_ADD(a, b) ((a) + (b))
#define DASHBOARD_VERTEX(vertex, axis) ((vertex).v.ob[axis])
#define DASHBOARD_COORD(value) ((s32)(value))
#define DASHBOARD_COS cosf
#define DASHBOARD_SIN sinf
#endif /* TGR_PC */

/* The right edge of the times, the speed and the dashboard. */
#define HUD_RIGHT 296

/* The gap below the dashboard and the speed. */
#define HUD_BOTTOM_MARGIN 4

/* The palettes come first in the dashboard on the cartridge, one for day and one for night. */
#define DASHBOARD_PALETTE_BYTES (DASHBOARD_PALETTE_COLORS * 2)
#define DASHBOARD_PALETTES_BYTES (DASHBOARD_PALETTE_BYTES * 2)

/* The revolutions for the full needle angle and the final bar segment. */
#define TACHOMETER_FULL_RPM 8000.0f
#define TACHOMETER_FULL_RPM_D 8000.0

/* The half-widths of the tip and the base of the needle, in radians. */
#define NEEDLE_TIP_HALF 0.05f
#define NEEDLE_BASE_HALF 0.3f

/* The lengths of the tip and the base of the needle for one view and for two. */
#define NEEDLE_TIP_LENGTH 20.0f
#define NEEDLE_BASE_LENGTH 7.0f
#define NEEDLE_TIP_LENGTH_SPLIT 15.0f
#define NEEDLE_BASE_LENGTH_SPLIT 5.0f

/* The needle shakes by up to this many revolutions, half of it while paused. */
#define NEEDLE_JITTER_MASK 0x7F
#define NEEDLE_JITTER_PAUSED 0x40

/* The height of the low-resolution screen, which the needle's vertices count up from. */
#define SCREEN_HEIGHT 240

/* A color of a palette dimmed to a quarter, and the bit that makes it opaque. */
#define PALETTE_QUARTER_MASK 0x18C6
#define PALETTE_OPAQUE 0x0001

/* The dashboard of the current view and the car of the active camera, read afresh at every use. */
#define DASHBOARD_FACE g_aDashboardTextures[g_dwCurrentSplitScreenIndex]
#define DASHBOARD_GEAR g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex]
#define ACTIVE_CAR_MODEL g_aCarModels[g_pActiveCameraVehicle->carModel]

#if TGR_PC
void RenderDashboardGauges(const ViewportRect *viewports) {
    if (!g_bHudShowSpeedometer || g_bNetSpectator) {
        return;
    }
    const VehicleModel *car =
        &g_aVehicleModels[viewports[g_dwCurrentSplitScreenIndex].vehicleIndex];
    SetupMenuProjection((f32)g_dwScreenWidth, (f32)g_dwScreenHeight);
    const ViewportRect *rect = &viewports[g_dwCurrentSplitScreenIndex];
    ConvertedModelVertex *verts;
    s32 x = DASHBOARD_ADD(DASHBOARD_ADD(g_dwConfigFramebufferWidth, -car->dashboardWidth), -16);
    s32 y = DASHBOARD_ADD(DASHBOARD_ADD(rect->y, rect->height), -car->dashboardHeight - 4);
    s32 gear = 0, jitter, lit, i;
    f32 rpm, tipLength, baseLength, needleX, needleY;
    long double angle;
    if (VEHICLE_GET(g_pActiveCameraVehicle, Throttle) >= 0.0f) {
        gear = DASHBOARD_ADD(VEHICLE_GET(g_pActiveCameraVehicle, Gear), 1);
    }
    if (g_dwNumViewports == 1) {
        PackCombineMode(
            gDisplayListPtr++, 0, 0, 0, 1003, 0, 0, 0, 1003, 0, 0, 0, 1003, 0, 0, 0, 1003);
        gDisplayListWords(gDisplayListPtr++, 0xb900031d, 0x0c184240);
        gDisplayListWords(gDisplayListPtr++, 0xb900031d, 0x0c193078);
        gSPClearGeometryMode(gDisplayListPtr++, G_ZBUFFER);
        gDisplayListWords(gDisplayListPtr++, 0xde000000, 0x3f800000);
        gDisplayListWords(gDisplayListPtr++, 0xdf000000, 0xbf800000);
        lit = 15;
        if (car->tachometer == TACHOMETER_BAR_DIMMED || car->tachometer == TACHOMETER_BAR_HIDDEN) {
            jitter = g_dwRacePauseState ? NEEDLE_JITTER_PAUSED : AdvanceRng() & NEEDLE_JITTER_MASK;
            lit =
                (s32)(s64)((((long double)jitter + VEHICLE_GET(g_pActiveCameraVehicle, EngineRpm)) -
                            1000.0f) *
                               0.0001428571413271129f * (car->barSegments + 1) -
                           0.5f);
            if (lit < 0) {
                lit = 0;
            } else if (lit > 15) {
                lit = 15;
            }
        }
        /* The original selects texture entries from the first viewport record. */
        EmitIndexedQuadDrawCommand(viewports[0].dashboardTextureEntries[15 - lit],
                                   x,
                                   y,
                                   car->dashboardWidth,
                                   car->dashboardHeight);
        if (gear != g_adwDashboardLoadedGear[g_dwCurrentSplitScreenIndex]) {
            GbiCommand *command = gDisplayListPtr++;
            command->w0 = 0xdd000000u | ((u32)viewports[0].dashboardGearTextureEntry & 0xffffffu);
            command->pData = &car->dashboardImages[(u32)gear * (u32)car->gearImageBytes];
            g_adwDashboardLoadedGear[g_dwCurrentSplitScreenIndex] = gear;
        }
        EmitIndexedQuadDrawCommand(viewports[0].dashboardGearTextureEntry,
                                   DASHBOARD_ADD(x, car->gearX),
                                   DASHBOARD_ADD(y, car->gearY),
                                   car->gearWidth,
                                   car->gearHeight);
    }
#else
void RenderDashboardGauges(void) {
    ViewportRect *rect = &g_aViewportRects[g_dwCurrentSplitScreenIndex];
    CarModel *car;
    Vtx *verts;
    u16 *palette;
    s32 x, y, gear, key, reload, jitter, first, lit, i;
    f32 rpm, angle, tipLength, baseLength;

    x = HUD_RIGHT - DASHBOARD_FACE.width;
    y = rect->y + rect->height - DASHBOARD_FACE.height - HUD_BOTTOM_MARGIN;
    gear = 0;
    if (g_pActiveCameraVehicle->throttle >= 0.0f) {
        gear = g_pActiveCameraVehicle->gear + 1;
    }

    /* Load the face and the palette when the car or the time of day changes. */
    key = g_pActiveCameraVehicle->carModel | (g_dwNightMode << 8);
    reload = key != g_adwDashboardLoadedKey[g_dwCurrentSplitScreenIndex];
    if (reload) {
        DASHBOARD_FACE.width = ACTIVE_CAR_MODEL.dashboardWidth;
        DASHBOARD_FACE.height = ACTIVE_CAR_MODEL.dashboardHeight;
        DmaRead(DASHBOARD_FACE.pixels,
                ACTIVE_CAR_MODEL.dashboardRom + DASHBOARD_PALETTES_BYTES,
                DASHBOARD_FACE.height * DASHBOARD_FACE.width);
        DmaRead(g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex],
                ACTIVE_CAR_MODEL.dashboardRom + g_dwNightMode * DASHBOARD_PALETTE_BYTES,
                DASHBOARD_PALETTE_BYTES);
        DASHBOARD_GEAR.width = ACTIVE_CAR_MODEL.gearWidth;
        DASHBOARD_GEAR.height = ACTIVE_CAR_MODEL.gearHeight;
        g_adwDashboardLoadedKey[g_dwCurrentSplitScreenIndex] = key;
    }
    if (reload || gear != g_adwDashboardLoadedGear[g_dwCurrentSplitScreenIndex]) {
        DmaRead(DASHBOARD_GEAR.pixels,
                ACTIVE_CAR_MODEL.dashboardRom + DASHBOARD_PALETTES_BYTES +
                    DASHBOARD_FACE.height * DASHBOARD_FACE.width +
                    DASHBOARD_GEAR.height * DASHBOARD_GEAR.width * gear,
                DASHBOARD_GEAR.height * DASHBOARD_GEAR.width);
        g_adwDashboardLoadedGear[g_dwCurrentSplitScreenIndex] = gear;
    }

    gDPPipeSync(gDisplayListPtr++);
    gDPLoadTLUT_pal256(gDisplayListPtr++, g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex]);
    if (g_dwNumViewports == 1) {
        DrawTexturedRect(&DASHBOARD_FACE,
                         x,
                         y,
                         DASHBOARD_FACE.width,
                         DASHBOARD_FACE.height,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         TEXRECT_BLEND_TEXTURE);
        DrawTexturedRect(&DASHBOARD_GEAR,
                         ACTIVE_CAR_MODEL.gearX + x,
                         ACTIVE_CAR_MODEL.gearY + y,
                         DASHBOARD_GEAR.width,
                         DASHBOARD_GEAR.height,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         TEXRECT_BLEND_TEXTURE);
    } else {
        /* A smaller dashboard, three quarters of the size, with its bottom right in place. */
        x += DASHBOARD_FACE.width / 4;
        y += DASHBOARD_FACE.height / 4;
        DrawTexturedRect(&DASHBOARD_FACE,
                         x,
                         y,
                         DASHBOARD_FACE.width * 3 / 4,
                         DASHBOARD_FACE.height * 3 / 4,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         TEXRECT_BLEND_TEXTURE);
        DrawTexturedRect(&DASHBOARD_GEAR,
                         ACTIVE_CAR_MODEL.gearX * 3 / 4 + x,
                         ACTIVE_CAR_MODEL.gearY * 3 / 4 + y,
                         DASHBOARD_GEAR.width * 3 / 4,
                         DASHBOARD_GEAR.height * 3 / 4,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         TEXRECT_BLEND_TEXTURE);
    }

#endif /* TGR_PC */
    jitter = g_dwRacePauseState ? NEEDLE_JITTER_PAUSED : AdvanceRng() & NEEDLE_JITTER_MASK;
#if TGR_PC
    rpm = (long double)jitter + VEHICLE_GET(g_pActiveCameraVehicle, EngineRpm);
#else
    rpm = (f32)jitter + VEHICLE_GET(g_pActiveCameraVehicle, EngineRpm);
    car = &g_aCarModels[g_pActiveCameraVehicle->carModel];
#endif
    if (car->tachometer == TACHOMETER_NEEDLE) {
        verts = g_aGaugeNeedleVerts[g_dwFrameBufferIndex][g_dwCurrentSplitScreenIndex];
        if (g_dwNumViewports == 2) {
#if TGR_PC
            x = DASHBOARD_ADD(x, car->needleX * 3 / 4);
            y = DASHBOARD_ADD(y, car->needleY * 3 / 4);
#else
            x += car->needleX * 3 / 4;
            y += car->needleY * 3 / 4;
#endif
            tipLength = NEEDLE_TIP_LENGTH_SPLIT;
            baseLength = NEEDLE_BASE_LENGTH_SPLIT;
        } else {
#if TGR_PC
            x = DASHBOARD_ADD(x, car->needleX);
            y = DASHBOARD_ADD(y, car->needleY);
#else
            x += car->needleX;
            y += car->needleY;
#endif
            tipLength = NEEDLE_TIP_LENGTH;
            baseLength = NEEDLE_BASE_LENGTH;
        }
#if TGR_PC
        y = (s32)((u32)g_dwConfigFramebufferHeight - (u32)y);
        needleX = (f32)x;
        needleY = (f32)y;
#define DASHBOARD_NEEDLE_X needleX
#define DASHBOARD_NEEDLE_Y needleY
#else
        y = SCREEN_HEIGHT - y;
#define DASHBOARD_NEEDLE_X x
#define DASHBOARD_NEEDLE_Y y
#endif /* TGR_PC */
        angle = car->needleRestAngle;
        if (rpm > 0.0f) {
#if TGR_PC
            angle = (long double)car->needleRestAngle -
                    ((long double)car->needleRestAngle - car->needleFullAngle) * rpm *
                        0.0001250000059371814f;
#else
            angle = car->needleRestAngle -
                    rpm * (car->needleRestAngle - car->needleFullAngle) / TACHOMETER_FULL_RPM;
#endif
        }

        /* A narrow quad from the tip on one side of the center to the base on the other. */
        DASHBOARD_VERTEX(verts[0], 0) = DASHBOARD_COORD(
            DASHBOARD_COS(angle - NEEDLE_TIP_HALF) * tipLength + DASHBOARD_NEEDLE_X);
        DASHBOARD_VERTEX(verts[0], 1) = DASHBOARD_COORD(
            DASHBOARD_SIN(angle - NEEDLE_TIP_HALF) * tipLength + DASHBOARD_NEEDLE_Y);
        DASHBOARD_VERTEX(verts[0], 2) = 0;
        DASHBOARD_VERTEX(verts[1], 0) = DASHBOARD_COORD(
            DASHBOARD_COS(angle + NEEDLE_TIP_HALF) * tipLength + DASHBOARD_NEEDLE_X);
        DASHBOARD_VERTEX(verts[1], 1) = DASHBOARD_COORD(
            DASHBOARD_SIN(angle + NEEDLE_TIP_HALF) * tipLength + DASHBOARD_NEEDLE_Y);
        DASHBOARD_VERTEX(verts[1], 2) = 0;
        DASHBOARD_VERTEX(verts[2], 0) = DASHBOARD_COORD(
            DASHBOARD_COS(angle + NEEDLE_BASE_HALF) * baseLength + DASHBOARD_NEEDLE_X);
        DASHBOARD_VERTEX(verts[2], 1) = DASHBOARD_COORD(
            DASHBOARD_SIN(angle + NEEDLE_BASE_HALF) * baseLength + DASHBOARD_NEEDLE_Y);
        DASHBOARD_VERTEX(verts[2], 2) = 0;
        DASHBOARD_VERTEX(verts[3], 0) = DASHBOARD_COORD(
            DASHBOARD_COS(angle - NEEDLE_BASE_HALF) * baseLength + DASHBOARD_NEEDLE_X);
        DASHBOARD_VERTEX(verts[3], 1) = DASHBOARD_COORD(
            DASHBOARD_SIN(angle - NEEDLE_BASE_HALF) * baseLength + DASHBOARD_NEEDLE_Y);
        DASHBOARD_VERTEX(verts[3], 2) = 0;
        for (i = 0; i < 4; i++) {
#if TGR_PC
            verts[i].color.x = 0.0f;
            verts[i].color.y = 255.0f;
            verts[i].color.z = 0.0f;
#else
            verts[i].v.cn[0] = 0;
            verts[i].v.cn[1] = 0xFF;
            verts[i].v.cn[2] = 0;
            verts[i].v.cn[3] = 0xFF;
#endif /* TGR_PC */
        }
        gDPPipeSync(gDisplayListPtr++);
        gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
#if TGR_PC
        PackCombineMode(
            gDisplayListPtr++, 0, 0, 0, 1004, 0, 0, 0, 1004, 0, 0, 0, 1004, 0, 0, 0, 1004);
        gDisplayListWords(gDisplayListPtr++, 0xb900031d, 0x00552048);
#else
        gDPSetCombineLERP(
            gDisplayListPtr++, 0, 0, 0, SHADE, 0, 0, 0, SHADE, 0, 0, 0, SHADE, 0, 0, 0, SHADE);
        gDPSetRenderMode(gDisplayListPtr++,
                         AA_EN | IM_RD | ALPHA_CVG_SEL |
                             GBL_c1(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM),
                         GBL_c2(G_BL_CLR_IN, G_BL_A_IN, G_BL_CLR_MEM, G_BL_A_MEM));
#endif /* TGR_PC */
        gSPClearGeometryMode(gDisplayListPtr++, G_CULL_FRONT | G_CULL_BACK | G_FOG | G_LIGHTING);
        gSPSetGeometryMode(gDisplayListPtr++, G_SHADE);
#if TGR_PC
        gDisplayListWords(gDisplayListPtr++, 0x0400107f, GBI_WORD1(verts));
#else
        gSPVertex(gDisplayListPtr++, verts, 4, 0);
#endif
        gSP2Triangles(gDisplayListPtr++, 0, 1, 2, 0, 0, 2, 3, 0);
#if TGR_PC
        gSPSetGeometryMode(gDisplayListPtr++, G_ZBUFFER);
#endif
        gDPPipeSync(gDisplayListPtr++);
        return;
    }
#if TGR_PLATFORM == TGR_PLATFORM_N64
    if (car->tachometer != TACHOMETER_BAR_DIMMED && car->tachometer != TACHOMETER_BAR_HIDDEN) {
        return;
    }

    /* Bar segments occupy final palette entries. Segments above engine speed dim or disappear. */
    first = (DASHBOARD_PALETTE_COLORS - car->barSegments) & ~3;
    DmaRead(&g_awDashboardBarPalette[first],
            car->dashboardRom + g_dwNightMode * DASHBOARD_PALETTE_BYTES + first * 2,
            (DASHBOARD_PALETTE_COLORS - first) * 2);
    car = &g_aCarModels[g_pActiveCameraVehicle->carModel];
    lit = (s32)((f64)rpm / TACHOMETER_FULL_RPM_D * (f64)(f32)(car->barSegments + 1) - 0.5);
    first = DASHBOARD_PALETTE_COLORS - car->barSegments;
    if (lit < 0) {
        lit = 0;
    }
    lit += first;
    if (lit > DASHBOARD_PALETTE_COLORS) {
        lit = DASHBOARD_PALETTE_COLORS;
    }
    palette = g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex];
    for (i = first; i < lit; i++) {
        palette[i] = g_awDashboardBarPalette[i];
    }
    if (ACTIVE_CAR_MODEL.tachometer == TACHOMETER_BAR_DIMMED) {
        for (i = lit; i < DASHBOARD_PALETTE_COLORS; i++) {
            palette[i] =
                ((g_awDashboardBarPalette[i] >> 3) & PALETTE_QUARTER_MASK) | PALETTE_OPAQUE;
        }
    } else {
        for (i = lit; i < DASHBOARD_PALETTE_COLORS; i++) {
            palette[i] = g_awDashboardBarPalette[i] & ~PALETTE_OPAQUE;
        }
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_N64 */
}
