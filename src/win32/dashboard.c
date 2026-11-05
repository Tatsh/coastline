#include "common/camera.h"
#include "common/cameraprojection.h"
#include "common/game.h"
#include "common/hud.h"
#include "common/pause.h"
#include "common/race.h"
#include "common/render.h"
#include "common/screen.h"
#include "common/settings.h"
#include "common/util.h"
#include "common/vecmath.h"
#include "common/vehicleaccess.h"
#include "common/weather.h"
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "win32/directplay.h"
#endif
#include <math.h>

#include "common/combine.h"
#include "common/hudcanvas.h"
#include "common/hudquad.h"
#include "common/renderbackend.h"

#if defined(ENABLE_PATCHES)
#define g_dwConfigFramebufferWidth GetHudCanvasWidth()
#define g_dwConfigFramebufferHeight GetHudCanvasHeight()
#define g_dwScreenWidth GetHudCanvasWidth()
#define g_dwScreenHeight GetHudCanvasHeight()
#endif

#define DASHBOARD_ADD(a, b) ((s32)((u32)(a) + (u32)(b)))
#define DASHBOARD_AXIS_0 x
#define DASHBOARD_AXIS_1 y
#define DASHBOARD_AXIS_2 z
#define DASHBOARD_VERTEX(vertex, axis) ((vertex).position.DASHBOARD_AXIS_##axis)
#define DASHBOARD_COORD(value) ((s16)(s64)(value))
#define DASHBOARD_COS cosl
#define DASHBOARD_SIN sinl
namespace {
    /** @ghidraAddress BRD3D: 0x104afd20 */
    ConvertedModelVertex g_aGaugeNeedleVerts[2][DASHBOARD_VIEWS][4];
}
s32 g_adwDashboardLoadedGear[DASHBOARD_VIEWS] = {-1, -1};
s32 g_bHudShowSpeedometer = 1;

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

#if TGR_PLATFORM == TGR_PLATFORM_SDL
static void DrawSplitScreenDashboard(
    const ViewportRect *view, const VehicleModel *car, s32 gear, s32 x, s32 y) {
    s32 lit = 15;
    PackCombineMode(gDisplayListPtr++, 0, 0, 0, 1003, 0, 0, 0, 1003, 0, 0, 0, 1003, 0, 0, 0, 1003);
    gDisplayListWords(gDisplayListPtr++, 0xb900031d, 0x0c184240);
    gDisplayListWords(gDisplayListPtr++, 0xb900031d, 0x0c193078);
    gSPClearGeometryMode(gDisplayListPtr++, G_ZBUFFER);
    gDisplayListWords(gDisplayListPtr++, 0xde000000, 0x3f800000);
    gDisplayListWords(gDisplayListPtr++, 0xdf000000, 0xbf800000);
    if (car->tachometer == TACHOMETER_BAR_DIMMED || car->tachometer == TACHOMETER_BAR_HIDDEN) {
        s32 jitter = g_dwRacePauseState ? NEEDLE_JITTER_PAUSED : AdvanceRng() & NEEDLE_JITTER_MASK;
        lit = (s32)((((f32)jitter + VEHICLE_GET(g_pActiveCameraVehicle, EngineRpm)) - 1000.0f) *
                        (1.0f / 7000.0f) * (f32)(car->barSegments + 1) -
                    0.5f);
        lit = lit < 0 ? 0 : lit > 15 ? 15 : lit;
    }
    EmitIndexedQuadDrawCommand(view->dashboardTextureEntries[15 - lit],
                               x,
                               y,
                               car->dashboardWidth * 3 / 4,
                               car->dashboardHeight * 3 / 4);
    if (gear != g_adwDashboardLoadedGear[g_dwCurrentSplitScreenIndex]) {
        GbiCommand *command = gDisplayListPtr++;
        command->w0 = 0xdd000000u | ((u32)view->dashboardGearTextureEntry & 0xffffffu);
        command->pData = &car->dashboardImages[(u32)gear * (u32)car->gearImageBytes];
        g_adwDashboardLoadedGear[g_dwCurrentSplitScreenIndex] = gear;
    }
    EmitIndexedQuadDrawCommand(view->dashboardGearTextureEntry,
                               x + car->gearX * 3 / 4,
                               y + car->gearY * 3 / 4,
                               car->gearWidth * 3 / 4,
                               car->gearHeight * 3 / 4);
}
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */

void RenderDashboardGauges(const ViewportRect *viewports) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    /* A spectator of a network game has no car of its own. */
    if (!g_bHudShowSpeedometer || g_bNetSpectator) {
#else
    if (!g_bHudShowSpeedometer) {
#endif
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
#if TGR_PLATFORM == TGR_PLATFORM_SDL
    else {
        /* The PC release draws only the needle in a split screen. The SDL game draws the face and
           the gear of each player as the console does, three quarters of their size with the
           bottom right in place, from the textures of the view. */
        x = DASHBOARD_ADD(x, car->dashboardWidth / 4);
        y = DASHBOARD_ADD(y, car->dashboardHeight / 4);
        DrawSplitScreenDashboard(rect, car, gear, x, y);
    }
#endif /* TGR_PLATFORM == TGR_PLATFORM_SDL */
    jitter = g_dwRacePauseState ? NEEDLE_JITTER_PAUSED : AdvanceRng() & NEEDLE_JITTER_MASK;
    rpm = (long double)jitter + VEHICLE_GET(g_pActiveCameraVehicle, EngineRpm);
    if (car->tachometer == TACHOMETER_NEEDLE) {
        verts = g_aGaugeNeedleVerts[g_dwFrameBufferIndex][g_dwCurrentSplitScreenIndex];
        if (g_dwNumViewports == 2) {
            x = DASHBOARD_ADD(x, car->needleX * 3 / 4);
            y = DASHBOARD_ADD(y, car->needleY * 3 / 4);
            tipLength = NEEDLE_TIP_LENGTH_SPLIT;
            baseLength = NEEDLE_BASE_LENGTH_SPLIT;
        } else {
            x = DASHBOARD_ADD(x, car->needleX);
            y = DASHBOARD_ADD(y, car->needleY);
            tipLength = NEEDLE_TIP_LENGTH;
            baseLength = NEEDLE_BASE_LENGTH;
        }
        y = (s32)((u32)g_dwConfigFramebufferHeight - (u32)y);
        needleX = (f32)x;
        needleY = (f32)y;
#define DASHBOARD_NEEDLE_X needleX
#define DASHBOARD_NEEDLE_Y needleY
        angle = car->needleRestAngle;
        if (rpm > 0.0f) {
            angle = (long double)car->needleRestAngle -
                    ((long double)car->needleRestAngle - car->needleFullAngle) * rpm *
                        0.0001250000059371814f;
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
            verts[i].color.x = 0.0f;
            verts[i].color.y = 255.0f;
            verts[i].color.z = 0.0f;
        }
        gDPPipeSync(gDisplayListPtr++);
        gDPSetCycleType(gDisplayListPtr++, G_CYC_1CYCLE);
        PackCombineMode(
            gDisplayListPtr++, 0, 0, 0, 1004, 0, 0, 0, 1004, 0, 0, 0, 1004, 0, 0, 0, 1004);
        gDisplayListWords(gDisplayListPtr++, 0xb900031d, 0x00552048);
        gSPClearGeometryMode(gDisplayListPtr++, G_CULL_FRONT | G_CULL_BACK | G_FOG | G_LIGHTING);
        gSPSetGeometryMode(gDisplayListPtr++, G_SHADE);
        gDisplayListWords(gDisplayListPtr++, 0x0400107f, GBI_WORD1(verts));
        gSP2Triangles(gDisplayListPtr++, 0, 1, 2, 0, 0, 2, 3, 0);
        gSPSetGeometryMode(gDisplayListPtr++, G_ZBUFFER);
        gDPPipeSync(gDisplayListPtr++);
        return;
    }
}
