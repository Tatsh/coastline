#include <PR/gu.h>

#include "common/assetdma.h"
#include "common/camera.h"
#include "common/displaylistptr.h"
#include "common/hud.h"
#include "common/pause.h"
#include "common/render.h"
#include "common/texrect.h"
#include "common/util.h"
#include "common/vecmath.h"
#include "common/vehicle.h"
#include "common/weather.h"
void RenderDashboardGauges(void) {
    int x;
    int y;
    unsigned int key;
    int v;
    float a;
    int frame;
#ifdef __sgi
    float fx;
    float fy;
    int lit;
    unsigned short *src;
    unsigned short *dst;
#endif
    Texture *img;
    int lx;
    int ly;
    int lw;
    int lh;
    int dh;
#ifdef __sgi
    /* IDO reserves these unused locals in the original stack frame. */
    int u0[3];
#endif

    x = 296 - g_aDashboardTextures[g_dwCurrentSplitScreenIndex].width;
    y = g_aViewportRects[g_dwCurrentSplitScreenIndex].y +
        g_aViewportRects[g_dwCurrentSplitScreenIndex].height -
        g_aDashboardTextures[g_dwCurrentSplitScreenIndex].height - 4;
    if (0.0f <= g_pActiveCameraVehicle->throttle) {
        frame = g_pActiveCameraVehicle->gear + 1;
    } else {
        frame = 0;
    }
    key = g_pActiveCameraVehicle->carModel | g_dwNightMode << 8;
    if (key != (u32)g_adwDashboardLoadedKey[g_dwCurrentSplitScreenIndex]) {
        g_aDashboardTextures[g_dwCurrentSplitScreenIndex].width =
            g_aCarModels[g_pActiveCameraVehicle->carModel].dashboardWidth;
        g_aDashboardTextures[g_dwCurrentSplitScreenIndex].height =
            g_aCarModels[g_pActiveCameraVehicle->carModel].dashboardHeight;
        DmaRead(g_aDashboardTextures[g_dwCurrentSplitScreenIndex].pixels,
                g_aCarModels[g_pActiveCameraVehicle->carModel].dashboardRom + 0x400,
                g_aDashboardTextures[g_dwCurrentSplitScreenIndex].width *
                    g_aDashboardTextures[g_dwCurrentSplitScreenIndex].height);
        DmaRead(g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex],
                g_aCarModels[g_pActiveCameraVehicle->carModel].dashboardRom + g_dwNightMode * 0x200,
                0x200);
        g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex].width =
            g_aCarModels[g_pActiveCameraVehicle->carModel].gearWidth;
        g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex].height =
            g_aCarModels[g_pActiveCameraVehicle->carModel].gearHeight;
        g_adwDashboardLoadedKey[g_dwCurrentSplitScreenIndex] = key;
        goto read;
    }
    if (frame != g_adwDashboardLoadedGear[g_dwCurrentSplitScreenIndex]) {
    read:
        DmaRead(g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex].pixels,
                g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex].height *
                        g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex].width * frame +
                    (g_aCarModels[g_pActiveCameraVehicle->carModel].dashboardRom +
                     g_aDashboardTextures[g_dwCurrentSplitScreenIndex].width *
                         g_aDashboardTextures[g_dwCurrentSplitScreenIndex].height) +
                    0x400,
                g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex].height *
                    g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex].width);
        g_adwDashboardLoadedGear[g_dwCurrentSplitScreenIndex] = frame;
    }
    gDisplayListWords(gDisplayListPtr++, 0xE7000000, 0);
    gDisplayListWords(
        gDisplayListPtr++, 0xFD100000, (u32)g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex]);
    gDisplayListWords(gDisplayListPtr++, 0xE8000000, 0);
    gDisplayListWords(gDisplayListPtr++, 0xF5000100, 0x07000000);
    gDisplayListWords(gDisplayListPtr++, 0xE6000000, 0);
    gDisplayListWords(gDisplayListPtr++, 0xF0000000, 0x073FC000);
    gDisplayListWords(gDisplayListPtr++, 0xE7000000, 0);
    if (g_dwNumViewports == 1) {
        img = &g_aDashboardTextures[g_dwCurrentSplitScreenIndex];
        DrawTexturedRect(
            img, x, y, img->width, img->height, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0);
        lx = g_aCarModels[g_pActiveCameraVehicle->carModel].gearX + x;
        ly = g_aCarModels[g_pActiveCameraVehicle->carModel].gearY + y;
        lh = g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex].height;
        DrawTexturedRect(&g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex],
                         lx,
                         ly,
                         g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex].width,
                         lh,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0);
    } else {
        x += g_aDashboardTextures[g_dwCurrentSplitScreenIndex].width / 4;
        y += g_aDashboardTextures[g_dwCurrentSplitScreenIndex].height / 4;
        lw = g_aDashboardTextures[g_dwCurrentSplitScreenIndex].width * 3 / 4;
        dh = g_aDashboardTextures[g_dwCurrentSplitScreenIndex].height * 3 / 4;
        DrawTexturedRect(&g_aDashboardTextures[g_dwCurrentSplitScreenIndex],
                         x,
                         y,
                         lw,
                         dh,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0);
        lx = g_aCarModels[g_pActiveCameraVehicle->carModel].gearX * 3 / 4 + x;
        ly = g_aCarModels[g_pActiveCameraVehicle->carModel].gearY * 3 / 4 + y;
        lw = g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex].width * 3 / 4;
        lh = g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex].height * 3 / 4;
        DrawTexturedRect(&g_aDashboardGearTextures[g_dwCurrentSplitScreenIndex],
                         lx,
                         ly,
                         lw,
                         lh,
                         0,
                         0,
                         0,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0xFF,
                         0);
    }
    {
        float rev;
        Vtx *q;
        float tip;
        float base;

        if (g_dwRacePauseState != 0) {
            v = 0x40;
        } else {
            v = AdvanceRng() & 0x7F;
        }
        rev = (float)v + g_pActiveCameraVehicle->engineRpm;
        if (g_aCarModels[g_pActiveCameraVehicle->carModel].tachometer == 0) {
            q = g_aGaugeNeedleVerts[g_dwFrameBufferIndex][g_dwCurrentSplitScreenIndex];
            if (2 == g_dwNumViewports) {
                x += g_aCarModels[g_pActiveCameraVehicle->carModel].needleX * 3 / 4;
                y += g_aCarModels[g_pActiveCameraVehicle->carModel].needleY * 3 / 4;
                tip = 15.0f;
                base = 5.0f;
            } else {
                x += g_aCarModels[g_pActiveCameraVehicle->carModel].needleX;
                y += g_aCarModels[g_pActiveCameraVehicle->carModel].needleY;
                tip = 20.0f;
                base = 7.0f;
            }
            y = 240 - y;
            a = g_aCarModels[g_pActiveCameraVehicle->carModel].needleRestAngle;
            if (0.0f < rev) {
                a -= rev *
                     (g_aCarModels[g_pActiveCameraVehicle->carModel].needleRestAngle -
                      g_aCarModels[g_pActiveCameraVehicle->carModel].needleFullAngle) /
                     8000.0f;
            }
            q[0].v.ob[0] = cosf(a - 0.05f) * tip + (float)x;
            q[0].v.ob[1] = sinf(a - 0.05f) * tip + (float)y;
            q[0].v.ob[2] = 0;
            q[1].v.ob[0] = cosf(a + 0.05f) * tip + (float)x;
            q[1].v.ob[1] = sinf(a + 0.05f) * tip + (float)y;
            q[1].v.ob[2] = 0;
            q[2].v.ob[0] = cosf(a + 0.3f) * base + (float)x;
            q[2].v.ob[1] = sinf(a + 0.3f) * base + (float)y;
            q[2].v.ob[2] = 0;
            q[3].v.ob[0] = cosf(a - 0.3f) * base + (float)x;
            q[3].v.ob[1] = sinf(a - 0.3f) * base + (float)y;
            q[3].v.ob[2] = 0;
            q[0].v.cn[0] = 0;
            q[0].v.cn[1] = 0xFF;
            q[0].v.cn[2] = 0;
            q[0].v.cn[3] = 0xFF;
            q[1].v.cn[0] = 0;
            q[1].v.cn[1] = 0xFF;
            q[1].v.cn[2] = 0;
            q[1].v.cn[3] = 0xFF;
            q[2].v.cn[0] = 0;
            q[2].v.cn[1] = 0xFF;
            q[2].v.cn[2] = 0;
            q[2].v.cn[3] = 0xFF;
            q[3].v.cn[0] = 0;
            q[3].v.cn[1] = 0xFF;
            q[3].v.cn[2] = 0;
            q[3].v.cn[3] = 0xFF;
            gDisplayListWords(gDisplayListPtr++, 0xE7000000, 0);
            gDisplayListWords(gDisplayListPtr++, 0xBA001402, 0);
            gDisplayListWords(gDisplayListPtr++, 0xFCFFFFFF, 0xFFFE793C);
            gDisplayListWords(gDisplayListPtr++, 0xB900031D, 0x00552048);
            gDisplayListWords(gDisplayListPtr++, 0xB6000000, 0x00033000);
            gDisplayListWords(gDisplayListPtr++, 0xB7000000, 4);
            gDisplayListWords(gDisplayListPtr++, 0x0400103F, (u32)q);
            gDisplayListWords(gDisplayListPtr++, 0xB1000204, 0x0406);
            gDisplayListWords(gDisplayListPtr++, 0xE7000000, 0);
        } else if (g_aCarModels[g_pActiveCameraVehicle->carModel].tachometer == 1 ||
                   g_aCarModels[g_pActiveCameraVehicle->carModel].tachometer == 2) {
            int first;

            first = (0x100 - g_aCarModels[g_pActiveCameraVehicle->carModel].needleX) & ~3;
            DmaRead(&g_awDashboardBarPalette[first],
                    g_aCarModels[g_pActiveCameraVehicle->carModel].dashboardRom +
                        g_dwNightMode * 0x200 + first * 2,
                    (0x100 - first) * 2);
            ly = g_aCarModels[g_pActiveCameraVehicle->carModel].needleX;
            first = 0x100 - ly;
            ly = rev / 8000.0 * (float)(ly + 1) - 0.5f;
            if (ly < 0) {
                ly = 0;
            }
            ly += first;
            if (ly > 0x100) {
                ly = 0x100;
            }
            for (; first < ly; first++) {
                g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex][first] =
                    g_awDashboardBarPalette[first];
            }
            first = ly;
            if (g_aCarModels[g_pActiveCameraVehicle->carModel].tachometer == 1) {
                for (; first < 0x100; first++) {
                    g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex][first] =
                        ((g_awDashboardBarPalette[first] >> 3) & 0x18C6) | 1;
                }
            } else {
                for (; first < 0x100; first++) {
                    g_aawDashboardPalettes[g_dwCurrentSplitScreenIndex][first] =
                        g_awDashboardBarPalette[first] & 0xFFFE;
                }
            }
        }
    }
}
