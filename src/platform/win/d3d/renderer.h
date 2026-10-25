#pragma once

/**
 * @file
 * The DirectDraw and Direct3D renderer of a window, and the host that the game reaches it through.
 */

#include <cstring>

#include <d3d.h>
#include <ddraw.h>
#include <windows.h>

/** A renderer told to take the screen back from GDI that had not given it. */
#define RENDERER_E_NOT_IN_GDI_MODE static_cast<HRESULT>(0x88762711)

/** A renderer that ran out of memory. */
#define RENDERER_E_OUT_OF_MEMORY static_cast<HRESULT>(0x88762714)

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

class D3DDeviceNode;
class DisplayMode;

/** A DirectDraw driver the game found. */
class DirectDrawDriver {
public:
    unsigned dwFlags;               /*!< DIRECTDRAW_DRIVER_USABLE and DIRECTDRAW_DRIVER_PRIMARY. */
    GUID guid;                      /*!< The GUID of the driver. */
    char *pszName;                  /*!< The name of the driver, allocated. */
    char *pszDescription;           /*!< The description of the driver, allocated. */
    DDCAPS hardwareCaps;            /*!< What the hardware of the driver does. */
    DDCAPS softwareCaps;            /*!< What the emulation of the driver does. */
    int nDisplayModeCount;          /*!< The display modes of the driver. */
    DisplayMode *pDisplayModesHead; /*!< The first display mode. */
    DisplayMode *pDisplayModesTail; /*!< The last display mode. */
    int nD3DDeviceCount;            /*!< The Direct3D devices of the driver. */
    D3DDeviceNode *pD3DDevicesHead; /*!< The first Direct3D device. */
    D3DDeviceNode *pD3DDevicesTail; /*!< The last Direct3D device. */
    DirectDrawDriver *pPrev;        /*!< The driver found before. */
    DirectDrawDriver *pNext;        /*!< The next driver found. */

    /** Start a driver that is not usable, with empty abilities. */
    DirectDrawDriver() : pPrev(nullptr), pNext(nullptr) {
        dwFlags &= ~DIRECTDRAW_DRIVER_USABLE;
        memset(&hardwareCaps, 0, sizeof(hardwareCaps));
        hardwareCaps.dwSize = sizeof(hardwareCaps);
        memset(&softwareCaps, 0, sizeof(softwareCaps));
        softwareCaps.dwSize = sizeof(softwareCaps);
        nDisplayModeCount = 0;
        pDisplayModesHead = nullptr;
        pDisplayModesTail = nullptr;
        nD3DDeviceCount = 0;
        pD3DDevicesHead = nullptr;
        pD3DDevicesTail = nullptr;
    }

    /**
     * Find the display modes of the driver.
     *
     * @param pDirectDraw2 The DirectDraw interface of the driver.
     * @return `S_OK`, or the error of DirectDraw.
     * @ghidraAddress BRD3D: 0x1001b320
     */
    HRESULT EnumerateDisplayModes(IDirectDraw2 *pDirectDraw2);

    /**
     * Free the display modes of the driver.
     *
     * @ghidraAddress BRD3D: 0x1001b3b0
     */
    void FlushDirectDrawModeEnumerationList();

    /**
     * Find the Direct3D devices of the driver.
     *
     * @param pDirect3D2 The Direct3D interface of the driver.
     * @return `S_OK`, or the error of Direct3D.
     * @ghidraAddress BRD3D: 0x1001b510
     */
    HRESULT EnumerateD3DDevices(IDirect3D2 *pDirect3D2);

    /**
     * Free the Direct3D devices of the driver.
     *
     * @return 0.
     * @ghidraAddress BRD3D: 0x1001b5a0
     */
    int FlushDirect3DDeviceList();

    /**
     * Add a display mode to the end of the modes of the driver.
     *
     * @param pMode The mode.
     * @return `S_OK`, or `E_INVALIDARG` without a mode.
     * @ghidraAddress BRD3D: 0x1001b430
     */
    HRESULT EnqueueDirectDrawModeNode(DisplayMode *pMode);

    /**
     * Add a Direct3D device to the end of the devices of the driver.
     *
     * @param pDevice The device.
     * @return `S_OK`, or `E_INVALIDARG` without a device.
     * @ghidraAddress BRD3D: 0x1001b630
     */
    HRESULT AppendDirect3DDeviceNode(D3DDeviceNode *pDevice);

    /**
     * Read a driver DirectDraw reports, with its devices and its modes.
     *
     * @param pGuid The GUID of the driver, or `nullptr` for the primary.
     * @param pszName The name of the driver.
     * @param pszDescription The description of the driver.
     * @return `S_OK`, or the error of DirectDraw.
     * @ghidraAddress BRD3D: 0x1001b090
     */
    HRESULT InitializeDirectDrawDriver(GUID *pGuid, LPSTR pszName, LPSTR pszDescription);

    /**
     * Free the devices and the modes of the driver.
     *
     * @ghidraAddress BRD3D: 0x1001b280
     */
    void ShutdownDirectDrawDriver();

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

/** A Direct3D device of a driver. The layout after softwareDesc is not recovered yet. */
class D3DDeviceNode {
public:
    unsigned dwFlags;           /*!< Flags of the device, of which bits 0 and 1 start clear. */
    GUID guid;                  /*!< The GUID of the device. */
    char *pszDescription;       /*!< The description of the device, allocated. */
    char *pszName;              /*!< The name of the device, allocated. */
    D3DDEVICEDESC hardwareDesc; /*!< What the hardware of the device does. */
    D3DDEVICEDESC softwareDesc; /*!< What the emulation of the device does. */
    int nTextureFormatCount;    /*!< The texture formats of the device. */
    DisplayMode *pTextureFormatsHead; /*!< The first texture format, a surface description. */
    DisplayMode *pTextureFormatsTail; /*!< The last texture format. */
    D3DDeviceNode *pPrev;             /*!< The device of the driver before. */
    D3DDeviceNode *pNext;             /*!< The next device of the driver. */

    /**
     * Release what the device holds before it is freed.
     *
     * @ghidraAddress BRD3D: 0x1001aee0
     */
    void ReleaseD3DDeviceNode();

    /** Start a device without texts, with empty descriptions. */
    D3DDeviceNode()
        : pszDescription(nullptr), pszName(nullptr), nTextureFormatCount(0),
          pTextureFormatsHead(nullptr), pTextureFormatsTail(nullptr), pPrev(nullptr),
          pNext(nullptr) {
        dwFlags &= ~3u;
        memset(&hardwareDesc, 0, sizeof(hardwareDesc));
        hardwareDesc.dwSize = sizeof(hardwareDesc);
        memset(&softwareDesc, 0, sizeof(softwareDesc));
        softwareDesc.dwSize = sizeof(softwareDesc);
    }

    /**
     * Read a device Direct3D reports.
     *
     * @param pGuid The GUID of the device.
     * @param pszNewDescription The description of the device.
     * @param pszNewName The name of the device.
     * @param pHardwareDesc What the hardware of the device does.
     * @param pSoftwareDesc What the emulation of the device does.
     * @return `S_OK`, or an error for a device that is not read.
     * @ghidraAddress BRD3D: 0x1001ade0
     */
    HRESULT InitD3DDeviceNode(GUID *pGuid,
                              LPSTR pszNewDescription,
                              LPSTR pszNewName,
                              D3DDEVICEDESC *pHardwareDesc,
                              D3DDEVICEDESC *pSoftwareDesc);

    /**
     * Free the texture formats of the device.
     *
     * @ghidraAddress BRD3D: 0x1001afb0
     */
    void FlushTextureFormatList();

    /**
     * Add a texture format to the end of the formats of the device.
     *
     * @param pFormat The format.
     * @return `S_OK`, or `E_INVALIDARG` without a format.
     * @ghidraAddress BRD3D: 0x1001b030
     */
    HRESULT EnqueueTextureFormatNode(DisplayMode *pFormat);

    /**
     * Whether the device is a device of the hardware, one with a colour model of its own.
     *
     * @return Whether the device is a device of the hardware.
     * @ghidraAddress BRD3D: 0x1001ad90
     */
    bool IsHardwareDevice();

    /**
     * Find the texture formats a device created from the node supports.
     *
     * @param pD3DDevice The device created.
     * @return `S_OK`, or the error of Direct3D.
     * @ghidraAddress BRD3D: 0x1001af20
     */
    HRESULT EnumerateDeviceTextureFormats(IDirect3DDevice2 *pD3DDevice);
};

/** A display mode of a driver. The layout after desc is not recovered yet. */
class DisplayMode {
public:
    DDSURFACEDESC desc; /*!< The description of the mode. */
    DisplayMode *pPrev; /*!< The mode of the driver before. */
    DisplayMode *pNext; /*!< The next mode of the driver. */

    /** Start an empty mode. */
    DisplayMode() : pPrev(nullptr), pNext(nullptr) {
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
    }

    /**
     * The size and the bits of a pixel of the mode.
     *
     * @param pnWidth Receives the width.
     * @param pnHeight Receives the height.
     * @param pnBitDepth Receives the bits of a pixel.
     * @param pnRefreshRate Receives the refresh rate, 0 for the default.
     * @return `S_OK`, or `E_FAIL` for a description without the size or the pixel format.
     * @ghidraAddress BRD3D: 0x1001ac80
     */
    HRESULT
    GetSurfaceDescDimensions(int *pnWidth, int *pnHeight, int *pnBitDepth, int *pnRefreshRate);
};

/** A Direct3D material and the description it was set from. */
class D3DMaterial {
public:
    D3DMATERIAL desc;              /*!< The description of the material. */
    D3DMATERIALHANDLE hMaterial;   /*!< The handle of the material on the device. */
    IDirect3DMaterial2 *pMaterial; /*!< The material of Direct3D. */
    int nUnknown58;                /*!< Cleared when the background is set. */

    /**
     * Create an opaque material of Direct3D and its handle on a device.
     *
     * @param pDirect3D2 The Direct3D interface that creates the material.
     * @param pD3DDevice The device the handle belongs to.
     * @ghidraAddress BRD3D: 0x1000a100
     */
    D3DMaterial(IDirect3D2 *pDirect3D2, IDirect3DDevice2 *pD3DDevice);

    /**
     * Release the material of Direct3D.
     *
     * @ghidraAddress BRD3D: 0x1000a170
     */
    void Release();
};

/** The DirectDraw and Direct3D objects of a window. */
class DirectDrawRenderer {
public:
    int anUnknown0[2];                   /* +0x00 */
    D3DMaterial *pMaterial;              /*!< The material of the viewport. */
    int dwDeviceInit;                    /*!< Whether the device has a depth buffer. */
    int dwSize;                          /*!< The size of the renderer. */
    int dwFlags;                         /*!< Options of the renderer. Bit 2 is set by default. */
    int nUnknown18;                      /* +0x18 */
    int dwRenderStateFlags;              /*!< The objects created, RENDERER_STATE_READY for all. */
    HWND hWnd;                           /*!< The window drawn into. */
    int nGdiModeRefCount;                /*!< How many times the screen was given to GDI. */
    int nUnknown28;                      /* +0x28 */
    DirectDrawDriver *pDriver;           /*!< The driver selected. */
    DisplayMode *pMode;                  /*!< The display mode selected. */
    D3DDeviceNode *pDevice;              /*!< The Direct3D device selected. */
    IDirectDraw *pDirectDraw;            /*!< The DirectDraw object of the driver. */
    IDirectDraw2 *pDirectDraw2;          /*!< The second interface of pDirectDraw. */
    IDirect3D2 *pDirect3D2;              /*!< The Direct3D interface of pDirectDraw. */
    IDirectDrawSurface *pPrimarySurface; /*!< The front buffer. */
    IDirectDrawPalette *pPrimaryPalette; /*!< The palette of the front buffer. */
    int nPaletteEntryCount;              /*!< The entries of the two palette buffers. */
    PALETTEENTRY *pGdiPaletteEntries;    /*!< The palette of GDI, set while GDI has the screen. */
    PALETTEENTRY *pGamePaletteEntries;   /*!< The palette of the game, saved for GDI. */
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
     * @param nWidth The width of the mode, or 0.
     * @param nHeight The height of the mode, or 0.
     * @param nBitDepth The bits of a pixel of the mode, or 0.
     * @param pSurfaceFormat The format of the surfaces, or `nullptr`.
     * @param pDeviceGuid The Direct3D device, or `nullptr` for the default.
     * @param bOption Whether bit 2 of dwFlags is set. Its meaning is not recovered yet.
     * @return `S_OK`, or the error of the step that failed.
     * @ghidraAddress BRD3D: 0x1000a1e0
     */
    HRESULT InitializeWindow(HWND hWnd,
                             GUID *pDriverGuid,
                             int nWidth,
                             int nHeight,
                             int nBitDepth,
                             void *pSurfaceFormat,
                             GUID *pDeviceGuid,
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
     * Create the viewport of the device and attach it, with its material.
     *
     * @return `S_OK`, or the error of Direct3D.
     * @ghidraAddress BRD3D: 0x1000adf0
     */
    HRESULT CreateAndAttachViewport();

    /**
     * Set the viewport of the device and the state it draws with.
     *
     * @return `S_OK`, or the error of Direct3D.
     * @ghidraAddress BRD3D: 0x1000afb0
     */
    HRESULT InitializeViewportAndRenderContext();

    /**
     * Create the material of the background of the viewport.
     *
     * @return `S_OK`, or the error of Direct3D.
     * @ghidraAddress BRD3D: 0x1000af00
     */
    HRESULT BeginRenderScene();

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
     * Give the palette to the front buffer.
     *
     * @return `S_OK`, or the error of DirectDraw.
     * @ghidraAddress BRD3D: 0x1000b150
     */
    HRESULT SetPrimarySurfacePalette();

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
 * The last DirectDraw driver the game found.
 *
 * @ghidraAddress BRD3D: 0x104bbe24
 */
extern DirectDrawDriver *g_pDirectDrawDriverListTail;

/**
 * Add a driver to the end of the list of drivers.
 *
 * @param pDriver The driver.
 * @return `S_OK`, or `E_INVALIDARG` without a driver.
 * @ghidraAddress BRD3D: 0x1001ba10
 */
HRESULT AppendDirectDrawDriverNode(DirectDrawDriver *pDriver);

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
 * Select the driver, device, and mode of a renderer, finding the drivers first if the game has
 * not. A mode is matched to the size asked for, or the mode of the desktop is taken.
 *
 * @param pDriverGuid The DirectDraw driver, or `nullptr` for the default.
 * @param nWidth The width of the mode, or 0.
 * @param nHeight The height of the mode, or 0.
 * @param nBitDepth The bits of a pixel of the mode, or 0.
 * @param pSurfaceFormat The format of the surfaces, or `nullptr`.
 * @param pDeviceGuid The Direct3D device, or `nullptr` for the default.
 * @param bMatchMode Whether a mode is matched to the size rather than taken from the desktop.
 * @param ppDriver Receives the driver.
 * @param ppMode Receives the mode.
 * @param ppDevice Receives the device.
 * @return `S_OK`, the error of the search for drivers, RENDERER_E_NOT_CREATED without a driver
 * or an output, or RENDERER_E_NOT_IN_GDI_MODE without a device and a mode.
 * @ghidraAddress BRD3D: 0x1001a790
 */
HRESULT SelectDriverDeviceAndModeFromConfig(GUID *pDriverGuid,
                                            int nWidth,
                                            int nHeight,
                                            int nBitDepth,
                                            void *pSurfaceFormat,
                                            GUID *pDeviceGuid,
                                            bool bMatchMode,
                                            DirectDrawDriver **ppDriver,
                                            DisplayMode **ppMode,
                                            D3DDeviceNode **ppDevice);

/**
 * Whether the DirectDraw drivers were found, in bit 0.
 *
 * @ghidraAddress BRD3D: 0x104bbe18
 */
extern unsigned char g_bDirectDrawDriversEnumerated;

/**
 * Find the DirectDraw drivers and their devices and modes, once.
 *
 * @return `S_OK`, or the error of DirectDraw.
 * @ghidraAddress BRD3D: 0x1001b970
 */
HRESULT EnsureDirectDrawDriversEnumerated(void);

/**
 * Select a device of a driver and a mode that matches a size.
 *
 * @param pDriver The driver.
 * @param pDeviceGuid The Direct3D device, or `nullptr` for the default.
 * @param nWidth The width of the mode.
 * @param nHeight The height of the mode.
 * @param nBitDepth The bits of a pixel of the mode.
 * @param pSurfaceFormat The format of the surfaces, or `nullptr`.
 * @param ppMode Receives the mode.
 * @param ppDevice Receives the device.
 * @return Whether a device and a mode were found.
 * @ghidraAddress BRD3D: 0x1001a6e0
 */
bool SelectDeviceAndDisplayMode(DirectDrawDriver *pDriver,
                                GUID *pDeviceGuid,
                                int nWidth,
                                int nHeight,
                                int nBitDepth,
                                void *pSurfaceFormat,
                                DisplayMode **ppMode,
                                D3DDeviceNode **ppDevice);

/**
 * Select a device of a driver and the mode of the desktop.
 *
 * @param pDriver The driver.
 * @param pDeviceGuid The Direct3D device, or `nullptr` for the default.
 * @param ppMode Receives the mode.
 * @param ppDevice Receives the device.
 * @return Whether a device and a mode were found.
 * @ghidraAddress BRD3D: 0x1001a610
 */
bool FindDesktopDisplayModeAndDevice(DirectDrawDriver *pDriver,
                                     GUID *pDeviceGuid,
                                     DisplayMode **ppMode,
                                     D3DDeviceNode **ppDevice);

/**
 * The bits of a depth from the `DDBD_` flags of the depths.
 *
 * @param dwMask The flags of the depths, of which the shallowest counts.
 * @return The bits, or 0 without a depth.
 * @ghidraAddress BRD3D: 0x1001a870
 */
int DDBDToBitDepth(DWORD dwMask);

/**
 * What the hardware of the device created last does.
 *
 * @ghidraAddress BRD3D: 0x10277480
 */
extern D3DDEVICEDESC g_D3DHalDeviceDesc;

/**
 * What the emulation of the device created last does.
 *
 * @ghidraAddress BRD3D: 0x10277580
 */
extern D3DDEVICEDESC g_D3DHelDeviceDesc;

/**
 * The number of DirectDraw drivers found.
 *
 * @ghidraAddress BRD3D: 0x104bbe1c
 */
extern int g_dwDirectDrawDriverCount;

/** What the search of the DirectDraw drivers found. */
struct DriverEnumContext {
    int bSucceeded; /*!< Whether each driver was recorded. */
    int nFound;     /*!< The drivers DirectDraw reported. */
    /** The driver whose modes or devices, or the device whose formats, are found, if any. */
    void *pOwner;
};

/**
 * Record a display mode of a driver that DirectDraw reports.
 *
 * @param pDesc The description of the mode.
 * @param pContext The DriverEnumContext of the search.
 * @return Whether the search goes on.
 * @ghidraAddress BRD3D: 0x1001aa70
 */
HRESULT WINAPI EnumDisplayModesCallback_AddMode(DDSURFACEDESC *pDesc, LPVOID pContext);

/**
 * Record a texture format of a device that Direct3D reports.
 *
 * @param pDesc The description of the format.
 * @param pContext The DriverEnumContext of the search.
 * @return Whether the search goes on.
 * @ghidraAddress BRD3D: 0x1001ab00
 */
HRESULT WINAPI EnumDeviceTextureFormatsCallback_AddFormat(DDSURFACEDESC *pDesc, LPVOID pContext);

/**
 * Record a Direct3D device of a driver that Direct3D reports.
 *
 * @param pGuid The GUID of the device.
 * @param pszDescription The description of the device.
 * @param pszName The name of the device.
 * @param pHardwareDesc What the hardware of the device does.
 * @param pSoftwareDesc What the emulation of the device does.
 * @param pContext The DriverEnumContext of the search.
 * @return Whether the search goes on.
 * @ghidraAddress BRD3D: 0x1001ab90
 */
HRESULT WINAPI EnumD3DDevicesCallback(GUID *pGuid,
                                      LPSTR pszDescription,
                                      LPSTR pszName,
                                      D3DDEVICEDESC *pHardwareDesc,
                                      D3DDEVICEDESC *pSoftwareDesc,
                                      LPVOID pContext);

/**
 * Record a DirectDraw driver that DirectDraw reports.
 *
 * @param pGuid The GUID of the driver, or `nullptr` for the primary.
 * @param pszDescription The description of the driver.
 * @param pszName The name of the driver.
 * @param pContext The DriverEnumContext of the search.
 * @return Whether the search goes on.
 * @ghidraAddress BRD3D: 0x1001a990
 */
BOOL WINAPI EnumDirectDrawDriversCallback(GUID *pGuid,
                                          LPSTR pszDescription,
                                          LPSTR pszName,
                                          LPVOID pContext);

/**
 * Find the DirectDraw drivers and their devices and modes.
 *
 * @return `S_OK`, the error of DirectDraw, or RENDERER_E_NOT_IN_GDI_MODE when a driver was not
 * recorded.
 * @ghidraAddress BRD3D: 0x1001b9a0
 */
HRESULT EnumerateDirectDrawDrivers(void);

/**
 * Read what a Direct3D device can do.
 *
 * @param pD3DDevice The device.
 * @ghidraAddress BRD3D: 0x10008d90
 */
void QueryD3DDeviceCaps(IDirect3DDevice2 *pD3DDevice);

/**
 * Find the texture formats of a Direct3D device for the game.
 *
 * @param pD3DDevice The device.
 * @ghidraAddress BRD3D: 0x10009bd0
 */
void EnumerateTextureFormats(IDirect3DDevice2 *pD3DDevice);

/**
 * Whether a pixel format indexes a palette of 1, 2, 4, or 8 bits.
 *
 * @param pFormat The pixel format, or `nullptr`.
 * @return Whether the format indexes a palette.
 * @ghidraAddress BRD3D: 0x1001a950
 */
bool IsPixelFormatPalettized(const DDPIXELFORMAT *pFormat);

/**
 * Find a mode of a driver for a size and a depth, for a device.
 *
 * @param pDriver The driver.
 * @param nWidth The width of the mode.
 * @param nHeight The height of the mode.
 * @param nBitDepth The bits of a pixel of the mode.
 * @param nUnknown Always 0. Its meaning is not recovered yet.
 * @param pDevice The device, or `nullptr`.
 * @return The mode, or `nullptr` when the driver has none.
 * @ghidraAddress BRD3D: 0x1001a570
 */
DisplayMode *FindDisplayModeWithFallback(DirectDrawDriver *pDriver,
                                         int nWidth,
                                         int nHeight,
                                         int nBitDepth,
                                         int nUnknown,
                                         D3DDeviceNode *pDevice);

/**
 * The width of the display mode set.
 *
 * @ghidraAddress BRD3D: 0x100a81c0
 */
extern int g_dwConfigFramebufferWidth;

/**
 * The height of the display mode set.
 *
 * @ghidraAddress BRD3D: 0x100a81c4
 */
extern int g_dwConfigFramebufferHeight;

/**
 * The bits of a pixel of the display mode set.
 *
 * @ghidraAddress BRD3D: 0x100aa8a8
 */
extern int g_dwScreenBitDepth;

/**
 * The width of the screen.
 *
 * @ghidraAddress BRD3D: 0x106c0684
 */
extern int g_dwScreenWidth;

/**
 * The height of the screen.
 *
 * @ghidraAddress BRD3D: 0x106c299c
 */
extern int g_dwScreenHeight;
