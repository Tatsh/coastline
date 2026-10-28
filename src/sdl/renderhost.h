#pragma once

/**
 * @file
 * The members of the DirectDraw renderer of BRD3D.dll that the game code calls, on the SDL GPU
 * renderer.
 */

#include <functional>

#include "sdl/d3dcompat.h"
#include "win32/textureentry.h"

typedef DWORD D3DMATERIALHANDLE; /*!< A material of the device. */

/** A display mode, which on SDL is the size of the frame texture. */
class DisplayMode {
public:
    /**
     * Report the size of the frame.
     *
     * @param pnWidth The width in pixels.
     * @param pnHeight The height in pixels.
     * @param pnBitDepth 32.
     * @param pnRefreshRate 60.
     */
    void GetSurfaceDescDimensions(int *pnWidth, int *pnHeight, int *pnBitDepth, int *pnRefreshRate);
};

/** The material whose diffuse colour clears the viewport. */
class D3DMaterial {
public:
    /** @return The material, of which there is one. */
    D3DMATERIALHANDLE GetMaterialHandle() const {
        return 1;
    }

    /**
     * Set the diffuse colour, each component from 0 to 1.
     *
     * @param red The red.
     * @param green The green.
     * @param blue The blue.
     */
    void SetDiffuseColor(float red, float green, float blue);

    /**
     * Set the ramp size of the ramp driver, which the renderer does not have.
     *
     * @param size The size.
     */
    void SetRampSize(DWORD size) {
    }

    /** @return D3D_OK. */
    HRESULT CommitMaterialIfDirty() {
        return D3D_OK;
    }

    /** @return The diffuse colour, 0xAARRGGBB. */
    std::uint32_t Color() const {
        return m_color;
    }

private:
    std::uint32_t m_color = 0xFF000000;
};

/** The viewport of the device. */
class D3DViewport {
public:
    /**
     * Use a material for the clears of the viewport.
     *
     * @param material The material.
     * @return D3D_OK.
     */
    HRESULT SetBackground(D3DMATERIALHANDLE material) {
        return D3D_OK;
    }

    /**
     * Fill rectangles with the colour of the material and the far depth.
     *
     * @param count The rectangles.
     * @param rects The rectangles.
     * @param flags D3DCLEAR_TARGET and D3DCLEAR_ZBUFFER.
     * @return D3D_OK.
     */
    HRESULT Clear(DWORD count, D3DRECT *rects, DWORD flags);
};

/** The renderer of the window. */
class DirectDrawRenderer {
public:
    /** @return The display mode. */
    DisplayMode *GetMode() {
        return &m_mode;
    }

    /** @return The material of the clears. */
    D3DMaterial *GetMaterial() {
        return &m_material;
    }

    /** @return The viewport. */
    D3DViewport *GetViewport() {
        return &m_viewport;
    }

    /** @return D3D_OK; the surfaces of the GPU are not lost. */
    HRESULT RestoreLostSurfaces() {
        return D3D_OK;
    }

    /** @return D3D_OK. */
    HRESULT BeginRenderScene() {
        return D3D_OK;
    }

private:
    DisplayMode m_mode;
    D3DMaterial m_material;
    D3DViewport m_viewport;
};

/** The owner of the renderers, which shows the loading screen. */
class RenderHost {
public:
    /**
     * Show an image of the game data while the game loads.
     *
     * @param pWindowRenderer The renderer of the window.
     * @param pszImage The name of the image, such as loading.img.
     * @param nOption Unused.
     * @return D3D_OK.
     */
    HRESULT
    RenderLoadingScreen(DirectDrawRenderer *pWindowRenderer, const char *pszImage, int nOption);
};

/** The renderer of the window. */
extern "C" DirectDrawRenderer *g_pMainRenderWindow;

/** The owner of the renderers. */
extern RenderHost *g_pRenderHost;

namespace coastline {

/**
 * Run a routine after each frame the game presents, for code that runs its own loop, such as the
 * front end of the Nintendo 64 release. The game ends when the routine returns false.
 *
 * @param pump The routine, or an empty routine for none.
 */
void SetFramePump(std::function<bool()> pump);

} // namespace coastline
