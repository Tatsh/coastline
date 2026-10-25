#pragma once

/**
 * @file
 * The renderer of the PC port: its mode, the routines that start and stop it, and the pool of the
 * data of the models it draws.
 */

/** The mode of the renderer of the Direct3D library. */
#define RENDER_BACKEND_MODE_D3D 1
/** The mode of the renderer of the Glide library. */
#define RENDER_BACKEND_MODE_GLIDE 3

/** The mode of the renderer of the library being built. */
#if TGR_RENDERER == TGR_RENDERER_D3D
#define RENDER_BACKEND_MODE RENDER_BACKEND_MODE_D3D
#else
#define RENDER_BACKEND_MODE RENDER_BACKEND_MODE_GLIDE
#endif

/**
 * The mode of the renderer, 0 before one is set.
 *
 * @ghidraAddress BRD3D: 0x10b502e4
 * @ghidraAddress BRGlide: 0x10b73644
 */
extern int g_dwRenderBackendMode;

/**
 * Reset the render device, set by SetRenderBackendMode().
 *
 * @ghidraAddress BRD3D: 0x10690a20
 * @ghidraAddress BRGlide: 0x106b7ab4
 */
extern int (*g_pfnRenderDeviceReset)(void);

/**
 * Start the renderer of the backend, set by SetRenderBackendMode().
 *
 * @ghidraAddress BRD3D: 0x10b501c8
 * @ghidraAddress BRGlide: 0x10b73528
 */
extern void (*g_pfnRenderBackendInit)(void);

/**
 * Start the texture subsystem, set by SetRenderBackendMode().
 *
 * @ghidraAddress BRD3D: 0x100b8c94
 * @ghidraAddress BRGlide: 0x100b849c
 */
extern void (*g_pfnInitializeTextureSubsystem)(void);

/**
 * Release the renderer of the backend, if one is set.
 *
 * @ghidraAddress BRD3D: 0x10b501cc
 * @ghidraAddress BRGlide: 0x10b7352c
 */
extern void (*g_pfnRenderBackendShutdown)(void);

/**
 * Release the texture subsystem, if one is set.
 *
 * @ghidraAddress BRD3D: 0x118aa0d0
 * @ghidraAddress BRGlide: 0x118ed1e8
 */
extern void (*g_pfnShutdownTextureSubsystem)(void);

/**
 * Release the render device, if one is set. The Glide library does not call it.
 *
 * @ghidraAddress BRD3D: 0x10690a28
 * @ghidraAddress BRGlide: 0x106b7abc
 */
extern void (*g_pfnRenderDeviceShutdown)(void);

/**
 * The renderer the game was set to use.
 *
 * @return g_dwRenderBackendMode.
 * @ghidraAddress BRD3D: 0x1006a980
 */
int GetRenderBackendMode(void);

/**
 * Record the mode of the renderer. The mode of the renderer of the library sets the routines that
 * reset its device and start the renderer and the texture subsystem.
 *
 * @param nMode The mode.
 * @ghidraAddress BRD3D: 0x1006a990
 * @ghidraAddress BRGlide: 0x10063940
 */
void SetRenderBackendMode(int nMode);

/**
 * Start the renderer of a mode: reset the device, start the texture subsystem and the renderer, and
 * empty the pool of the data of the models.
 *
 * @param nMode The mode of the renderer.
 * @ghidraAddress BRD3D: 0x1006aa20
 * @ghidraAddress BRGlide: 0x100639d0
 */
void InitializeRenderSubsystems(int nMode);

/**
 * Shut the renderer down when one runs, and start the renderer of a mode in a display mode.
 *
 * @param nMode The mode of the renderer.
 * @param nWidth The width of the display mode.
 * @param nHeight The height of the display mode.
 * @param nBitDepth The bits of a pixel of the display mode.
 * @param nExtra The fourth value of the display mode.
 * @ghidraAddress BRD3D: 0x1006a9c0
 * @ghidraAddress BRGlide: 0x10063970
 */
void ResetRenderSubsystems(int nMode, int nWidth, int nHeight, int nBitDepth, int nExtra);

/**
 * Set a display mode, falling back to 640 by 480 with 16 bits when the device is lost, and reset
 * the routines of the device.
 *
 * @param nWidth The width of the display mode.
 * @param nHeight The height of the display mode.
 * @param nBitDepth The bits of a pixel of the display mode.
 * @param nExtra The fourth value of the display mode.
 * @ghidraAddress BRD3D: 0x1001bb00
 * @ghidraAddress BRGlide: 0x1001e130
 * @stub{BRGlide}
 */
void SetDisplayModeWithFallback(int nWidth, int nHeight, int nBitDepth, int nExtra);

#if TGR_RENDERER == TGR_RENDERER_D3D
/**
 * Set the routines of the Direct3D device: the flip of the main window shows a frame, and the
 * shutdown of the device does nothing.
 *
 * @return Always true.
 * @ghidraAddress BRD3D: 0x1001bae0
 */
int ResetRenderDeviceCallbacks(void);

/**
 * Start the renderer of the display lists of the game with Direct3D.
 *
 * @ghidraAddress BRD3D: 0x10024460
 */
void InitGbiD3DRenderer(void);
#else
/**
 * Start the Glide device.
 *
 * @return The result of the start. Its meaning is not recovered yet.
 * @ghidraAddress BRGlide: 0x1001e080
 */
int InitGlideRenderDevice(void);

/**
 * Start the renderer of the display lists of the game with Glide.
 *
 * @ghidraAddress BRGlide: 0x10023b10
 */
void InitGbiGlideRenderer(void);
#endif

/**
 * Start the texture subsystem.
 *
 * @ghidraAddress BRD3D: 0x1002a640
 * @ghidraAddress BRGlide: 0x10029b50
 */
void InitializeTextureSubsystem(void);

/**
 * The number of vertices converted into g_afModelVertexPool.
 *
 * @ghidraAddress BRD3D: 0x1067d550
 * @ghidraAddress BRGlide: 0x105b96f8
 */
extern int g_dwConvertedVertexCount;

/**
 * The pool of the data of the models the renderer draws.
 *
 * @ghidraAddress BRD3D: 0x10575540
 * @ghidraAddress BRGlide: 0x104b16e8
 */
extern float g_afModelVertexPool[];

/**
 * The next free entry of g_afModelVertexPool.
 *
 * @ghidraAddress BRD3D: 0x100a81c8
 * @ghidraAddress BRGlide: 0x100a751c
 */
extern float *g_pModelDataPoolCursor;

/**
 * Empty the pool of the data of the models.
 *
 * @ghidraAddress BRD3D: 0x1002b950
 * @ghidraAddress BRGlide: 0x100189c0
 */
void ResetModelDataPool(void);
