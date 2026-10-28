/**
 * @file
 * The images of the menus of the Glide library, which the menus draw into a frame the size of the
 * frame buffer and write to the back buffer of Glide.
 */
#include <cstdio>

#include "win32/boot/app.h"
#include "win32/frontend.h"
#include "win32/glide/rgb565image.h"
#include "win32/render.h"
#include "win32/settings.h"

// The colour key of the menu images.
#define MENU_IMAGE_COLOR_KEY RGB(0, 0xff, 0)

Rgb565Image *g_pMenuFrameSurface;

bool InitMenuSurfaces(void) {
    char szMessage[0x100];

    if (g_pMenuFrameSurface == nullptr) {
        g_pMenuFrameSurface =
            CreateRgb565Image(g_dwConfigFramebufferWidth, g_dwConfigFramebufferHeight);
        if (g_pMenuFrameSurface == nullptr) {
            return ReleaseMenuSurfacesAndFail(g_hMainWindow);
        }
    }
    if (g_dwMenuSurfaceInitCount != 0) {
        g_aMenuImages[0].pImage = LoadBitmapToRgb565Image("images\\loading.bmp", 0, 0);
        if (g_aMenuImages[0].pszPath != nullptr && g_aMenuImages[0].pImage == nullptr) {
            // The message is formatted and not shown.
            sprintf(szMessage, "DDraw_DoInit: loading.bmp failed to load!");
            return ReleaseMenuSurfacesAndFail(g_hMainWindow);
        }
        g_pActiveMenuScreen->ClearBackBuffer();
        BlitSurfaceRegion(0, 0, 0, &g_aMenuSprites[0].rc, g_aMenuSprites[0].dwFlags);
        g_pActiveMenuScreen->FlipAndWait();
        if (g_aMenuImages[0].pImage != nullptr) {
            FreeRgb565Image(g_aMenuImages[0].pImage);
            g_aMenuImages[0].pImage = nullptr;
        }
    }
    g_dwMenuSurfaceInitCount++;
    for (int i = 0; i < MENU_IMAGE_COUNT; i++) {
        MenuImage &image = g_aMenuImages[i];

        if (image.pszPath == nullptr) {
            continue;
        }
        image.pImage = LoadBitmapToRgb565Image(image.pszPath, 0, 0);
        // The path is checked a second time, as in the original.
        if (image.pszPath != nullptr && image.pImage == nullptr) {
            sprintf(szMessage, "DDraw_DoInit: Bitmap %d failed to load!", i);
            return ReleaseMenuSurfacesAndFail(g_hMainWindow);
        }
        g_wMenuImageLoadedCount++;
        SetRgb565ImageColorKey(image.pImage, MENU_IMAGE_COLOR_KEY);
    }
    return true;
}

void ReleaseMenuSurfaces(void) {
    for (int i = 0; i < g_wMenuImageLoadedCount; i++) {
        if (g_aMenuImages[i].pImage != nullptr) {
            FreeRgb565Image(g_aMenuImages[i].pImage);
            g_aMenuImages[i].pImage = nullptr;
        }
    }
    if (g_pMenuFrameSurface != nullptr) {
        FreeRgb565Image(g_pMenuFrameSurface);
        g_pMenuFrameSurface = nullptr;
    }
}

void BlitSurfaceRegion(int nDestX, int nDestY, WORD wImage, const RECT *pRect, DWORD dwFlags) {
    BlitRgb565ImageRegion(
        g_pMenuFrameSurface, nDestX, nDestY, g_aMenuImages[wImage].pImage, pRect, dwFlags);
}

void MenuScreen::FlipAndWait() {
    WriteRgb565ImageToGlideLfb(g_pMenuFrameSurface);
    g_pfnPresentFrame();
}
