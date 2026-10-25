#pragma once

/**
 * @file
 * The DirectDraw and Direct3D renderer of a window, and the host that the game reaches it through.
 */

#include <d3d.h>
#include <ddraw.h>
#include <windows.h>

/** A renderer told to take the screen back from GDI that had not given it. */
#define RENDERER_E_NOT_IN_GDI_MODE static_cast<HRESULT>(0x88762711)

/** A renderer that is missing its window or its host. */
#define RENDERER_E_NOT_CREATED static_cast<HRESULT>(0x88762712)

/** A renderer that is not ready to draw. */
#define RENDERER_E_NOT_READY static_cast<HRESULT>(0x88762713)

/** The message a renderer sends its window after it creates its objects, with itself. */
#define WM_RENDERER_CREATED 0xbd0

/** The message a renderer sends its window before it releases its objects, with itself. */
#define WM_RENDERER_DESTROYING 0xbd1

/** Query the window for whether its renderer can draw. */
#define WM_RENDERER_IS_READY 0x7e8

/** Query the window for its renderer. */
#define WM_RENDERER_GET 0x7e9

/** Query the window for the back buffer of its renderer. */
#define WM_RENDERER_GET_BACK_BUFFER 0x7ea

/** The bits of DirectDrawRenderer::dwRenderStateFlags of a renderer that can draw. */
#define RENDERER_STATE_READY 0x1f

/** A DirectDraw driver usable by the game. */
#define DIRECTDRAW_DRIVER_USABLE 0x1

/** The primary DirectDraw driver, selected without a GUID. */
#define DIRECTDRAW_DRIVER_PRIMARY 0x2

/** A DirectDraw driver the game found. The layout after pNext is not recovered yet. */
class DirectDrawDriver {
public:
    unsigned dwFlags;        /*!< DIRECTDRAW_DRIVER_USABLE and DIRECTDRAW_DRIVER_PRIMARY. */
    GUID guid;               /*!< The GUID of the driver. */
    char abUnknown14[0x31c]; /* +0x14 */
    DirectDrawDriver *pNext; /*!< The next driver found. */

    /**
     * Whether a GUID selects the driver, a usable driver alone. No GUID selects the primary.
     *
     * @param pGuid The GUID, or `nullptr` for the primary driver.
     * @return Whether the GUID selects the driver.
     * @ghidraAddress BRD3D: 0x1001b2d0
     */
    bool CompareDirectDrawDriverGuid(const GUID *pGuid);

    /**
     * The GUID that selects the driver.
     *
     * @return The GUID, or `nullptr` for the primary driver.
     * @ghidraAddress BRD3D: 0x1001b960
     */
    GUID *GetDirectDrawDriverGuid();
};

class RenderHost;

/** The DirectDraw and Direct3D objects of a window. */
class DirectDrawRenderer {
public:
    int anUnknown0[3];                   /* +0x00 */
    int dwDeviceInit;                    /*!< Whether the device is initialised. */
    int dwSize;                          /*!< The size of the renderer. */
    int dwFlags;                         /*!< Options of the renderer. Bit 2 is set by default. */
    int nUnknown18;                      /* +0x18 */
    int dwRenderStateFlags;              /*!< The objects created, RENDERER_STATE_READY for all. */
    HWND hWnd;                           /*!< The window drawn into. */
    int nGdiModeRefCount;                /*!< How many times the screen was given to GDI. */
    int nUnknown28;                      /* +0x28 */
    DirectDrawDriver *pDriver;           /*!< The driver selected. */
    void *pDevice;                       /*!< The Direct3D device selected. */
    void *pMode;                         /*!< The display mode selected. */
    IDirectDraw *pDirectDraw;            /*!< The DirectDraw object of the driver. */
    IDirectDraw2 *pDirectDraw2;          /*!< The second interface of pDirectDraw. */
    IDirect3D2 *pDirect3D2;              /*!< The Direct3D interface of pDirectDraw. */
    IDirectDrawSurface *pPrimarySurface; /*!< The front buffer. */
    IDirectDrawPalette *pPrimaryPalette; /*!< The palette of the front buffer. */
    int nPaletteEntryCount;              /*!< The entries of the two palette buffers. */
    void *pPaletteEntries;               /*!< A buffer of palette entries. */
    void *pSavedPaletteEntries;          /*!< A second buffer of palette entries. */
    IDirectDrawSurface *pBackBuffer;     /*!< The surface the device draws into. */
    IDirectDrawSurface *pZBuffer;        /*!< The depth buffer attached to pBackBuffer. */
    IDirect3DDevice2 *pD3DDevice;        /*!< The Direct3D device. */
    IDirect3DViewport2 *pViewport;       /*!< The viewport of pD3DDevice. */
    int anUnknown68[4];                  /* +0x68 */
    RECT rcViewport;                     /*!< The rectangle of the viewport. */
    RenderHost *pHost;                   /*!< The host told when the device changes. */

    /**
     * Clear the renderer.
     *
     * @ghidraAddress BRD3D: 0x1000a190
     */
    DirectDrawRenderer();

    /**
     * Release every object of the renderer and forget its window.
     *
     * @ghidraAddress BRD3D: 0x1000a1c0
     */
    void Cleanup();

    /**
     * Select a driver, a device, and a mode, and create the objects that draw into a window. A
     * failure releases what was created.
     *
     * @param hWnd The window.
     * @param pDriverGuid The DirectDraw driver, or `nullptr` for the default.
     * @param pDeviceGuid The Direct3D device, or `nullptr` for the default.
     * @param nWidth The width of the mode, or 0.
     * @param nHeight The height of the mode, or 0.
     * @param nBitDepth The bits of a pixel of the mode, or 0.
     * @param nModeFlags Options of the selection, or 0.
     * @param bOption Whether bit 2 of dwFlags is set. Its meaning is not recovered yet.
     * @return `S_OK`, or the error of the step that failed.
     * @ghidraAddress BRD3D: 0x1000a1e0
     */
    HRESULT InitializeWindow(HWND hWnd,
                             GUID *pDriverGuid,
                             GUID *pDeviceGuid,
                             int nWidth,
                             int nHeight,
                             int nBitDepth,
                             int nModeFlags,
                             bool bOption);

    /**
     * Tell the window that the objects are going and release them.
     *
     * @return 0.
     * @ghidraAddress BRD3D: 0x1000a350
     */
    int DestroyDirectDrawRendererObjects();

    /**
     * Create the DirectDraw object of the driver and its second and Direct3D interfaces.
     *
     * @return `S_OK`, or the error of the step that failed.
     * @ghidraAddress BRD3D: 0x1000a480
     */
    HRESULT CreateDirectDrawObjects();

    /**
     * Release the DirectDraw object and its interfaces.
     *
     * @return 0.
     * @ghidraAddress BRD3D: 0x1000a540
     */
    int ReleaseDirectDrawObjects();

    /**
     * Take the screen for the window alone.
     *
     * @return `S_OK`, the error of DirectDraw, or RENDERER_E_NOT_READY without a window.
     * @ghidraAddress BRD3D: 0x1000a590
     */
    HRESULT SetCooperativeLevel();

    /**
     * Set the display mode selected.
     *
     * @return `S_OK`, or the error of DirectDraw.
     * @ghidraAddress BRD3D: 0x1000a5d0
     */
    HRESULT SetDisplayMode();

    /**
     * Give the display back its mode.
     *
     * @return 0.
     * @ghidraAddress BRD3D: 0x1000a800
     */
    int RestoreDisplayMode();

    /**
     * Create the front buffer and its back buffers.
     *
     * @return `S_OK`, or the error of DirectDraw.
     * @ghidraAddress BRD3D: 0x1000a820
     */
    HRESULT CreatePrimaryFlipChain();

    /**
     * Create the palette of the front buffer when the mode has one.
     *
     * @return `S_OK`, or the error of DirectDraw.
     * @ghidraAddress BRD3D: 0x1000a8d0
     */
    HRESULT InitializePrimaryPalette();

    /**
     * Release the front buffer and its palette.
     *
     * @return 0.
     * @ghidraAddress BRD3D: 0x1000a8a0
     */
    int ReleasePrimarySurface();

    /**
     * Release the palette of the front buffer and free its entries.
     *
     * @return 0.
     * @ghidraAddress BRD3D: 0x1000ab20
     */
    int ReleasePrimaryPalette();

    /**
     * Create the Direct3D device and its viewport.
     *
     * @return `S_OK`, or the error of Direct3D.
     * @ghidraAddress BRD3D: 0x1000ab70
     */
    HRESULT InitializeRenderDevice();

    /**
     * Release the Direct3D device, its viewport, and the back and depth buffers, telling the host
     * first.
     *
     * @return 0.
     * @ghidraAddress BRD3D: 0x1000ad70
     */
    int DestroyD3DDevice();

    /**
     * Release the viewport of the device and its material.
     *
     * @return 0.
     * @ghidraAddress BRD3D: 0x1000aea0
     */
    int ReleaseViewportAndMaterial();

    /**
     * Give the renderer the host that draws through it, and give the host the renderer.
     *
     * @param pRenderHost The host.
     * @return `S_OK`, or `E_INVALIDARG` without a host.
     * @ghidraAddress BRD3D: 0x1000b280
     */
    HRESULT SetDeviceNotificationCallback(RenderHost *pRenderHost);

    /**
     * Give the screen to GDI, with its palette, so a menu or a dialog can show.
     *
     * @return `S_OK`, or the error of DirectDraw.
     * @ghidraAddress BRD3D: 0x1000b170
     */
    HRESULT RestoreGdiPaletteAndFlipToGdi();

    /**
     * Take the screen back from GDI with the palette of the game.
     *
     * @return `S_OK`, or the error of DirectDraw.
     * @ghidraAddress BRD3D: 0x1000b1f0
     */
    HRESULT RestoreGamePaletteEntries();

    /**
     * Give the screen to GDI or take it back. The screen goes to GDI on the first request and
     * comes back on the last.
     *
     * @param bEnter Whether the screen goes to GDI.
     * @return `S_OK`, the error of the change, or RENDERER_E_NOT_IN_GDI_MODE for a return that
     * was not requested.
     * @ghidraAddress BRD3D: 0x1000b220
     */
    HRESULT AdjustGdiModeRefCount(bool bEnter);

    /**
     * Restore the front, back, and depth buffers that were lost, and the textures of the host.
     *
     * @return `S_OK`, the error of a restore, or RENDERER_E_NOT_READY.
     * @ghidraAddress BRD3D: 0x1000b2c0
     */
    HRESULT RestoreLostSurfaces();

    /**
     * The rectangle of the viewport.
     *
     * @param pRect Receives the rectangle.
     * @return `S_OK`, or RENDERER_E_NOT_READY.
     * @ghidraAddress BRD3D: 0x1000b350
     */
    HRESULT GetDeviceViewportRect(RECT *pRect);
};

/** What the game draws through: the renderer of the main window. */
class RenderHost {
public:
    DirectDrawRenderer *pRenderer; /*!< The renderer, set by SetRenderer(). */

    /**
     * Start without a renderer.
     *
     * @ghidraAddress BRD3D: 0x10001130
     */
    RenderHost();

    /**
     * Prepare for the device of the renderer to go.
     *
     * @return 0.
     * @ghidraAddress BRD3D: 0x10001180
     */
    int OnDestroyDevice();

    /**
     * Load the textures again after the surfaces of the renderer were restored.
     *
     * @return 0.
     * @ghidraAddress BRD3D: 0x10001190
     */
    int ReloadTextureSurfacesAfterRestore();

    /**
     * Draw through a renderer.
     *
     * @param pNewRenderer The renderer.
     * @return `S_OK`, or RENDERER_E_NOT_CREATED without a renderer.
     * @ghidraAddress BRD3D: 0x10001140
     */
    HRESULT SetRenderer(DirectDrawRenderer *pNewRenderer);

    /**
     * Show an image of the game while the game loads.
     *
     * @param pWindowRenderer The renderer of the window.
     * @param pszImage The name of the image.
     * @param nImageKey A constant the game passes with the image, 0x2ac7e58b.
     * @ghidraAddress BRD3D: 0x10008e30
     */
    void RenderLoadingScreen(DirectDrawRenderer *pWindowRenderer,
                             const char *pszImage,
                             unsigned nImageKey);

    /**
     * Whether the renderer is ready and has its Direct3D objects.
     *
     * @return `S_OK`, RENDERER_E_NOT_READY for a renderer not ready, or `E_FAIL`.
     * @ghidraAddress BRD3D: 0x10001160
     */
    HRESULT ValidateRenderDeviceState();

    /**
     * Whether the renderer is ready and has Direct3D, a device, and a viewport.
     *
     * @return `S_OK`, or `E_FAIL`.
     * @ghidraAddress BRD3D: 0x100011b0
     */
    HRESULT ValidateObjectRenderResources();
};

/**
 * The host the game draws through.
 *
 * @ghidraAddress BRD3D: 0x118abe08
 */
extern RenderHost *g_pRenderHost;

/**
 * The Direct3D interface of the renderer that created it last.
 *
 * @ghidraAddress BRD3D: 0x10277478
 */
extern IDirect3D2 *g_pDirect3D2;

/**
 * The first DirectDraw driver the game found with a GUID, or the default.
 *
 * @param pGuid The GUID of the driver, or `nullptr` for the default.
 * @return The driver, or `nullptr` when none was found.
 * @ghidraAddress BRD3D: 0x1001a550
 */
DirectDrawDriver *FindDirectDrawDriverOrDefault(GUID *pGuid);

/**
 * The DirectDraw drivers the game found, linked by DirectDrawDriver::pNext.
 *
 * @ghidraAddress BRD3D: 0x104bbe20
 */
extern DirectDrawDriver *g_pDirectDrawDriverListHead;

/**
 * The Direct3D device of the renderer that created it last.
 *
 * @ghidraAddress BRD3D: 0x10277368
 */
extern IDirect3DDevice2 *g_pD3DDevice2;

/**
 * Find the driver a GUID selects, and the primary driver on the way.
 *
 * @param pGuid The GUID, or `nullptr` for the primary driver.
 * @param ppPrimary Receives the primary driver, or the first driver searched, unless `nullptr`.
 * @param pFirst The driver the search starts at, or `nullptr` for the first driver found.
 * @return The driver, or `nullptr` when the GUID selects none.
 * @ghidraAddress BRD3D: 0x1001ba70
 */
DirectDrawDriver *
FindDirectDrawDriverByGuid(GUID *pGuid, DirectDrawDriver **ppPrimary, DirectDrawDriver *pFirst);

/**
 * Do nothing.
 *
 * @return 0.
 * @ghidraAddress BRD3D: 0x1007a1c0
 */
int ReturnZero(void);

/**
 * Create the surfaces of every texture.
 *
 * @ghidraAddress BRD3D: 0x10024bb0
 */
void InitializeAllTextureSurfaces(void);

/**
 * Whether NullStub() is called after the textures are loaded again.
 *
 * @ghidraAddress BRD3D: 0x100ac300
 */
extern bool g_dwNullStubEnable;

/**
 * Select the driver, device, and mode of a renderer from the configuration.
 *
 * @param pDriverGuid The DirectDraw driver, or `nullptr` for the default.
 * @param pDeviceGuid The Direct3D device, or `nullptr` for the default.
 * @param nWidth The width of the mode, or 0.
 * @param nHeight The height of the mode, or 0.
 * @param nBitDepth The bits of a pixel of the mode, or 0.
 * @param nModeFlags Options of the selection, or 0.
 * @param nBehavior Always 1.
 * @param ppDriver Receives the driver.
 * @param ppDevice Receives the device.
 * @param ppMode Receives the mode.
 * @return `S_OK`, or the error of the selection.
 * @ghidraAddress BRD3D: 0x1001a790
 */
HRESULT SelectDriverDeviceAndModeFromConfig(GUID *pDriverGuid,
                                            GUID *pDeviceGuid,
                                            int nWidth,
                                            int nHeight,
                                            int nBitDepth,
                                            int nModeFlags,
                                            int nBehavior,
                                            DirectDrawDriver **ppDriver,
                                            void **ppDevice,
                                            void **ppMode);
