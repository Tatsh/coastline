#include "dashboardtextures.h"

#include <string.h>

#include <common/hud.h>

#include "common/strings.h"
#include "common/vehiclemodel.h"
#include "common/weather.h"
#include "renderbackend.h"
#include "win32/fatalerror.h"

void InitializeDashboardTextures(void) {
    for (s32 index = 0; index < g_dwNumViewports; index++) {
        ViewportRect *view = &g_aViewportRects[index];
        VehicleModel *model = &g_aVehicleModels[view->vehicleIndex];
        u16 *palette = model->dashboardPalettes[g_dwCurrentWeather == 4];
        u8 *paletteBytes = (u8 *)palette;
        view->dashboardTextureEntries[0] = g_pfnCreateTextureFromN64Image(model->dashboardImages,
                                                                          paletteBytes,
                                                                          model->dashboardWidth,
                                                                          model->dashboardHeight,
                                                                          model->dashboardWidth,
                                                                          1,
                                                                          2,
                                                                          0,
                                                                          0,
                                                                          1,
                                                                          1,
                                                                          0,
                                                                          0,
                                                                          0,
                                                                          0);
        if (model->tachometer == TACHOMETER_BAR_DIMMED ||
            model->tachometer == TACHOMETER_BAR_HIDDEN) {
            g_bForceTextureClone = 1;
            for (s32 segment = 0; segment < 15; segment++) {
                u16 *color = &palette[255 - segment];
                if (model->tachometer == TACHOMETER_BAR_DIMMED) {
                    u16 dimmed = ((*color & 0x3000) | 0x0800) >> 11 | (*color & 0xc6) << 5;
                    *color = (u16)(dimmed << 8 | dimmed >> 8);
                } else if (model->tachometer == TACHOMETER_BAR_HIDDEN) {
                    *color &= 0xfeff;
                }
                view->dashboardTextureEntries[segment + 1] =
                    g_pfnCreateTextureFromN64Image(model->dashboardImages,
                                                   paletteBytes,
                                                   model->dashboardWidth,
                                                   model->dashboardHeight,
                                                   model->dashboardWidth,
                                                   1,
                                                   2,
                                                   0,
                                                   0,
                                                   1,
                                                   1,
                                                   0,
                                                   0,
                                                   0,
                                                   0);
            }
            g_bForceTextureClone = 0;
        }
        u8 *gearImages = &model->dashboardImages[model->dashboardWidth * model->dashboardHeight];
        view->dashboardGearTextureEntry = g_pfnCreateTextureFromN64Image(gearImages,
                                                                         paletteBytes,
                                                                         model->gearWidth,
                                                                         model->gearHeight,
                                                                         model->gearWidth,
                                                                         1,
                                                                         2,
                                                                         0,
                                                                         0,
                                                                         1,
                                                                         1,
                                                                         0,
                                                                         0,
                                                                         1,
                                                                         0);
        s32 imageBytes = (model->gearCount + 2) * model->gearWidth * model->gearHeight;
        u8 *source = &model->dashboardImages[sizeof(model->dashboardImages) - imageBytes];
        u8 *destination = model->dashboardImages;
        // The gear images can overlap their new place at the end of the buffer. BRGlide calls
        // memmove, and the static memcpy of BRD3D's runtime copies overlapping ranges the same way.
        memmove(source, gearImages, imageBytes);
        for (s32 gear = 0; gear < model->gearCount + 2; gear++) {
            s32 decoded = g_pfnDecodeN64TextureToPixelBuffer(destination,
                                                             source,
                                                             paletteBytes,
                                                             model->gearWidth,
                                                             model->gearHeight,
                                                             model->gearWidth,
                                                             1,
                                                             2);
            destination += decoded;
            model->gearImageBytes = decoded;
            if (source < destination) {
                ShowFatalErrorScreen(GetStringResource(300));
            }
            source += model->gearWidth * model->gearHeight;
            if (destination > &model->dashboardImages[sizeof(model->dashboardImages)]) {
                ShowFatalErrorScreen(GetStringResource(301));
            }
        }
        g_adwDashboardLoadedGear[index] = -1;
    }
}
