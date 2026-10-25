#pragma once

/**
 * @file
 * The renderer of the PC port: its mode, the routines that start and stop it, and the pool of the
 * data of the models it draws.
 */

#include <windows.h>

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

/**
 * Clear the target and the depth buffer of the main window once after each flip, when
 * g_dwD3DClearZBuffer is set. Without it, only g_bRenderTargetCleared is set.
 *
 * @ghidraAddress BRD3D: 0x100244e0
 */
void ClearRenderTargetOnce(void);

/**
 * Convert a colour of the console, RGBA 5551 with its bytes swapped, to RGBA 8888. Each channel of
 * five bits is widened by repeating its low three bits, and the alpha bit becomes 0 or 255.
 *
 * @param dwColor16 The colour in the low 16 bits.
 * @return The colour, red in the low byte.
 * @ghidraAddress BRD3D: 0x10027b10
 */
DWORD ConvertColor16To32(DWORD dwColor16);
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
 * Get the exponent of the smallest power of two from 1 to 256 not below a dimension of a texture.
 *
 * @param nDimension The dimension.
 * @return The exponent, from 0 to 8, and 8 for a dimension above 256.
 * @ghidraAddress BRD3D: 0x10027c00
 * @ghidraAddress BRGlide: 0x10027290
 */
int GetMipLevelFromDimension(int nDimension);

/**
 * Get the size of a texture from the codes of Glide for its level of detail and its aspect ratio.
 *
 * @param pWidth Receives the width.
 * @param pHeight Receives the height. It is not written for an aspect ratio above 6.
 * @param nLod The level of detail, from 0 for 256 texels to 8 for 1, and 1 texel above 8.
 * @param nAspectRatio The aspect ratio, from 0 for eight times as wide as high to 6 for eight times
 * as high as wide.
 * @ghidraAddress BRD3D: 0x10028720
 * @ghidraAddress BRGlide: 0x10029290
 */
void CalculateTextureDimensions(int *pWidth, int *pHeight, int nLod, int nAspectRatio);

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
 * Make the size of the frame the size of the framebuffer of the configuration.
 *
 * @ghidraAddress BRD3D: 0x1003128c
 * @ghidraAddress BRGlide: 0x1002a93c
 */
void CopyConfigFramebufferSize(void);

/**
 * The number of vertices of the cache of converted vertices.
 *
 * @ghidraAddress BRD3D: 0x1067b54c
 * @ghidraAddress BRGlide: 0x105b76f4
 */
extern int g_dwVertexConvCacheCount;

/**
 * The number of pointers relocated.
 *
 * @ghidraAddress BRD3D: 0x1067b548
 * @ghidraAddress BRGlide: 0x105b76f0
 */
extern int g_dwRelocatedPointerCount;

/**
 * The address the pointers to relocate are based at.
 *
 * @ghidraAddress BRD3D: 0x1057553c
 * @ghidraAddress BRGlide: 0x104b16e4
 */
extern unsigned g_dwOldBaseAddress;

/**
 * The address the pointers are relocated to.
 *
 * @ghidraAddress BRD3D: 0x10575538
 * @ghidraAddress BRGlide: 0x104b16e0
 */
extern unsigned g_dwNewBaseAddress;

/** The most entries of g_apRelocatedPointers. */
#define RELOCATED_POINTER_COUNT 0x800

/**
 * The pointers relocated, g_dwRelocatedPointerCount of them.
 *
 * @ghidraAddress BRD3D: 0x1067b550
 * @ghidraAddress BRGlide: 0x105b76f8
 */
extern void *g_apRelocatedPointers[RELOCATED_POINTER_COUNT];

/**
 * Add a pointer to g_apRelocatedPointers while it has room.
 *
 * @param pPointer The pointer.
 * @ghidraAddress BRD3D: 0x1002c1f0
 * @ghidraAddress BRGlide: 0x100192b0
 */
void AddPointerToRelocatedList(void *pPointer);

/**
 * Swap the two bytes of each of a run of halfwords.
 *
 * @param pData The halfwords.
 * @param nCount The number of halfwords.
 * @ghidraAddress BRD3D: 0x1002b9e0
 * @ghidraAddress BRGlide: 0x10018a50
 */
void ByteSwapArray16(unsigned short *pData, int nCount);

/** A vertex of the pool of the clipping of shadows. The layout past the link is not recovered yet.
 */
struct ShadowClipVertex {
    ShadowClipVertex *pNext; /*!< The next free vertex. */
    char abUnknown4[0x24];   /* +0x04 */
};

/** The number of entries of g_aShadowClipVertexPool. */
#define SHADOW_CLIP_VERTEX_COUNT 64

/**
 * The pool of the vertices of the clipping of shadows.
 *
 * @ghidraAddress BRD3D: 0x102e54c0
 * @ghidraAddress BRGlide: 0x102e0ca8
 */
extern ShadowClipVertex g_aShadowClipVertexPool[SHADOW_CLIP_VERTEX_COUNT];

/**
 * The first free vertex of g_aShadowClipVertexPool.
 *
 * @ghidraAddress BRD3D: 0x102e5ecc
 * @ghidraAddress BRGlide: 0x102e16b4
 */
extern ShadowClipVertex *g_pShadowClipVertexFreeList;

/**
 * Link every vertex of g_aShadowClipVertexPool into its free list, in order.
 *
 * @ghidraAddress BRD3D: 0x1000f460
 * @ghidraAddress BRGlide: 0x1000c9c0
 */
void ResetRenderCommandBufferStack(void);

/** The number of entries of g_anSkidMarkHead and g_anSkidMarkTail. */
#define SKID_MARK_TRACK_COUNT 32

/**
 * The first skid mark of each track of skid marks.
 *
 * @ghidraAddress BRD3D: 0x10364308
 * @ghidraAddress BRGlide: 0x1035faf0
 */
extern int g_anSkidMarkHead[SKID_MARK_TRACK_COUNT];

/**
 * The last skid mark of each track of skid marks.
 *
 * @ghidraAddress BRD3D: 0x10363f68
 * @ghidraAddress BRGlide: 0x1035f750
 */
extern int g_anSkidMarkTail[SKID_MARK_TRACK_COUNT];

/**
 * Clear the tracks of skid marks.
 *
 * @ghidraAddress BRD3D: 0x1000f620
 * @ghidraAddress BRGlide: 0x1000cb80
 */
void ClearSplitScreenRenderState(void);

/**
 * The first texture of the queue of textures to upload.
 *
 * @ghidraAddress BRD3D: 0x118a9878
 * @ghidraAddress BRGlide: 0x118ec990
 */
extern int g_dwTextureUploadQueueHead;

/**
 * The last texture of the queue of textures to upload.
 *
 * @ghidraAddress BRD3D: 0x118aa098
 * @ghidraAddress BRGlide: 0x118ed1b0
 */
extern int g_dwTextureUploadQueueTail;

/**
 * The mutex of the queue of textures to upload.
 *
 * @ghidraAddress BRD3D: 0x118aa0a0
 * @ghidraAddress BRGlide: 0x118ed1b8
 */
extern HANDLE g_hTextureUploadQueueMutex;

/**
 * Empty the queue of textures to upload and create its mutex.
 *
 * @ghidraAddress BRD3D: 0x10074f20
 * @ghidraAddress BRGlide: 0x1006e180
 */
void InitializeTextureUploadQueue(void);

/**
 * Empty the cache of converted vertices and the count of relocated pointers.
 *
 * @ghidraAddress BRD3D: 0x1002b9c0
 * @ghidraAddress BRGlide: 0x10018a30
 */
void ResetVertexConversionCache(void);

/**
 * Empty the cache of converted vertices and record the address the pointers to relocate are based
 * at and the address they move to.
 *
 * @param dwOldBase The address the pointers are based at.
 * @param dwNewBase The address the pointers move to.
 * @ghidraAddress BRD3D: 0x1002b9a0
 * @ghidraAddress BRGlide: 0x10018a10
 */
void SetPointerRelocationBase(unsigned dwOldBase, unsigned dwNewBase);

/**
 * Copy eight words.
 *
 * @param pDest Receives the words.
 * @param pSrc The words.
 * @ghidraAddress BRD3D: 0x1002b930
 * @ghidraAddress BRGlide: 0x100189a0
 */
void Copy8Dwords(void *pDest, const void *pSrc);

/**
 * Empty the pool of the data of the models.
 *
 * @ghidraAddress BRD3D: 0x1002b950
 * @ghidraAddress BRGlide: 0x100189c0
 */
void ResetModelDataPool(void);
