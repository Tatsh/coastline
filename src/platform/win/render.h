#pragma once

/**
 * @file
 * The renderer of the PC port: its mode, the routines that start and stop it, and the pool of the
 * data of the models it draws.
 */

#include <PR/gbi.h>
#include <windows.h>

#include "../../render.h"
#include "convertedvertex.h"
#include "platform/win/fontdata.h"
#include "platform/win/textureupload.h"
#include "vecmath.h"

struct GbiCommand;
struct TextureImageData;
struct TexScanTile;

/**
 * @ghidraAddress BRD3D: 0x10b501d0
 */
extern void (*g_pfnRenderBackendSubmitDisplayList)(GbiCommand *pDisplayList);

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

/**
 * Set the level of damage of a texture the game loaded.
 *
 * @param nEntry The entry of the texture.
 * @param nDamageLevel The level of damage.
 * @ghidraAddress BRD3D: 0x10025800
 * @ghidraAddress BRGlide: 0x10024e30
 * @stub{BRGlide}
 */
void SetTextureEntryDamageLevel(int nEntry, int nDamageLevel);

/**
 * Load the data of a texture the game loaded again, set by InitializeTextureSubsystem().
 *
 * @ghidraAddress BRD3D: 0x118aa0c0
 * @ghidraAddress BRGlide: 0x118ed1d8
 */
extern void (*g_pfnReloadTextureEntryData)(int nEntry);

/** The highest level of damage of IncrementDamageTextureLevel(). */
#define DAMAGE_TEXTURE_MAX_LEVEL 3

/** The number of damage textures of IncrementDamageTextureLevel(). */
#define DAMAGE_TEXTURE_COUNT 8

/**
 * Raise the level of damage of a damage texture by one, up to #DAMAGE_TEXTURE_MAX_LEVEL, and load
 * the texture again.
 *
 * @param nTexture The damage texture, below #DAMAGE_TEXTURE_COUNT.
 * @param pnEntries The entry of each damage texture.
 * @param pnLevels The level of damage of each damage texture.
 * @ghidraAddress BRD3D: 0x10074ed0
 * @ghidraAddress BRGlide: 0x1006e130
 */
void IncrementDamageTextureLevel(int nTexture, const int *pnEntries, int *pnLevels);

/**
 * Raise the level of damage of a damage texture or of one of its two neighbours, wrapping at the
 * ends, chosen at random from those below #DAMAGE_TEXTURE_MAX_LEVEL.
 *
 * @param nTexture The damage texture, below #DAMAGE_TEXTURE_COUNT.
 * @param pnEntries The entry of each damage texture.
 * @param pnLevels The level of damage of each damage texture.
 * @ghidraAddress BRD3D: 0x10074e40
 * @ghidraAddress BRGlide: 0x1006e0a0
 */
void ApplyRandomAdjacentDamageTexture(int nTexture, const int *pnEntries, int *pnLevels);

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
 * @stub{BRGlide}
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

#if TGR_RENDERER == TGR_RENDERER_D3D
/**
 * Get the bytes of the levels of a texture of four bytes a texel from one level of detail to
 * another, each level half the width and the height of the one before.
 *
 * @param nSmallLod The level of detail of the smallest level, as CalculateTextureDimensions()
 * reads it.
 * @param nLargeLod The level of detail of the largest level.
 * @param nAspectRatio The aspect ratio, as CalculateTextureDimensions() reads it.
 * @return The bytes, or 0 when the smallest level is larger than the largest.
 * @ghidraAddress BRD3D: 0x10029e10
 */
int ComputeMipChainByteSize(int nSmallLod, int nLargeLod, int nAspectRatio);
#endif

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
extern ConvertedModelVertex g_afModelVertexPool[];

/**
 * The next free entry of g_afModelVertexPool.
 *
 * @ghidraAddress BRD3D: 0x100a81c8
 * @ghidraAddress BRGlide: 0x100a751c
 */
extern ConvertedModelVertex *g_pModelDataPoolCursor;

/**
 * Make the size of the frame the size of the framebuffer of the configuration.
 *
 * @ghidraAddress BRD3D: 0x1003128c
 * @ghidraAddress BRGlide: 0x1002a93c
 */
void CopyConfigFramebufferSize(void);

/** A run of vertices of the console converted to floats. */
struct VertexConvCacheEntry {
    const Vtx *pSource;                /*!< The vertices of the console. */
    int nVertexCount;                  /*!< The number of vertices. */
    ConvertedModelVertex *pFloatVerts; /*!< The vertices converted. */
};

/** The number of entries of g_aVertexConvCache. */
#define VERTEX_CONV_CACHE_SIZE 0x800

/**
 * The runs of vertices converted.
 *
 * @ghidraAddress BRD3D: 0x10675548
 * @ghidraAddress BRGlide: 0x105b16f0
 */
extern VertexConvCacheEntry g_aVertexConvCache[VERTEX_CONV_CACHE_SIZE];

/**
 * The number of entries of g_aVertexConvCache in use.
 *
 * @ghidraAddress BRD3D: 0x1067b54c
 * @ghidraAddress BRGlide: 0x105b76f4
 */
extern int g_dwVertexConvCacheCount;

/**
 * Add a run of vertices converted to g_aVertexConvCache while it has room.
 *
 * @param pSource The vertices of the console.
 * @param nVertexCount The number of vertices.
 * @param pFloatVerts The vertices converted.
 * @ghidraAddress BRD3D: 0x1002bf00
 * @ghidraAddress BRGlide: 0x10018fc0
 * @stub{BRGlide}
 */
void AddVertexConversionCacheEntry(const Vtx *pSource,
                                   int nVertexCount,
                                   ConvertedModelVertex *pFloatVerts);

union ModelVertexData;

/**
 * @ghidraAddress BRD3D: 0x1002bdd0
 */
void ByteSwapVertexArray(Vtx *pVertices, int nCount);

/**
 * @ghidraAddress BRD3D: 0x1002be30
 */
ConvertedModelVertex *ConvertVerticesToFloat(const Vtx *pVertices, int nCount);

/**
 * @ghidraAddress BRD3D: 0x1002bd50
 */
void ConvertN64VerticesCached(ModelVertexData *pVertices, int nCount);

/**
 * @ghidraAddress BRD3D: 0x1002c150
 */
void RelocateGbiVertexCommand(GbiCommand *pCommand);

/**
 * @ghidraAddress BRD3D: 0x1002bf80
 */
void ByteSwapAndRelocateDisplayList(GbiCommand *pCommands);

/**
 * @ghidraAddress BRD3D: 0x10690bec
 */
extern unsigned char *g_pTrackTextureTable;

/**
 * @ghidraAddress BRD3D: 0x1002bc90
 */
void ByteSwapTextureVariantTable(TextureAnimation *pAnimation);

/**
 * @ghidraAddress BRD3D: 0x1002baa0
 */
void ByteSwapAndRelocateTextureDescriptor(ModelTexture *pTexture);

/**
 * @ghidraAddress BRD3D: 0x1002ba80
 */
void ByteSwapTextureDescriptorArray(ModelTexture *pTextures, int nCount);

/**
 * @ghidraAddress BRD3D: 0x10075330
 */
void NullStub2(void *pMemory);

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
 * Report whether a pointer was relocated already.
 *
 * @param pPointer The pointer.
 * @return Whether g_apRelocatedPointers includes it, and true for a null pointer.
 * @ghidraAddress BRD3D: 0x1002bf40
 * @ghidraAddress BRGlide: 0x10019000
 */
bool IsPointerInRelocatedList(void *pPointer);

/**
 * Move a pointer from g_dwOldBaseAddress to g_dwNewBaseAddress. A pointer below the old base
 * becomes null, and a null pointer is left null.
 *
 * @param pdwPointer The pointer, as a word.
 * @ghidraAddress BRD3D: 0x1002b970
 * @ghidraAddress BRGlide: 0x100189e0
 */
void RelocatePointerValue(unsigned *pdwPointer);

/**
 * Swap the two bytes of each of a run of halfwords.
 *
 * @param pData The halfwords.
 * @param nCount The number of halfwords.
 * @ghidraAddress BRD3D: 0x1002b9e0
 * @ghidraAddress BRGlide: 0x10018a50
 */
void ByteSwapArray16(unsigned short *pData, int nCount);

/**
 * Swap the two bytes of each of four halfwords.
 *
 * @param pData The halfwords.
 * @ghidraAddress BRD3D: 0x1002ba20
 * @ghidraAddress BRGlide: 0x10018a90
 */
void ByteSwapFourShorts(unsigned short *pData);

/**
 * Swap the bytes of each halfword of a run of groups of four halfwords.
 *
 * @param pData The groups.
 * @param nCount The number of groups.
 * @ghidraAddress BRD3D: 0x1002ba00
 * @ghidraAddress BRGlide: 0x10018a70
 */
void ByteSwapShortQuadArray(unsigned short *pData, int nCount);

/**
 * Reverse the four bytes of each of three words.
 *
 * @param pData The words.
 * @ghidraAddress BRD3D: 0x100383c0
 * @ghidraAddress BRGlide: 0x10018af0
 */
void ByteSwap3DWords(unsigned char *pData);

/** A texture of the console that a texture of another size replaces, from the texture map. */
struct TextureMapEntry {
    unsigned nId;       /*!< The number of the entry in the texture map. */
    unsigned dwAddress; /*!< The address of the texture of the console. */
    int nWidth;         /*!< The width of the replacement. */
    int nHeight;        /*!< The height of the replacement. */
};

/** The number of entries of g_aTextureMapEntries. */
#define TEXTURE_MAP_ENTRY_COUNT 256

/**
 * The entries the texture map file lists.
 *
 * @ghidraAddress BRD3D: 0x106c7e78
 * @ghidraAddress BRGlide: 0x106eef08
 */
extern TextureMapEntry g_aTextureMapEntries[TEXTURE_MAP_ENTRY_COUNT];

/**
 * The entries of g_aTextureMapEntries in use.
 *
 * @ghidraAddress BRD3D: 0x10a99778
 * @ghidraAddress BRGlide: 0x10ac0808
 */
extern int g_dwTextureMapEntryCount;

/**
 * The amount added to an address before it is looked up in g_aTextureMapEntries.
 *
 * @ghidraAddress BRD3D: 0x10a9977c
 * @ghidraAddress BRGlide: 0x10ac080c
 */
extern unsigned g_dwTextureMapAddressBias;

/**
 * Get the size of the replacement of a texture of the console, searching the texture map from its
 * last entry.
 *
 * @param dwAddress The address of the texture, before g_dwTextureMapAddressBias.
 * @param pnWidth Receives the width of the replacement.
 * @param pnHeight Receives the height of the replacement.
 * @return Whether the texture map lists the texture.
 * @ghidraAddress BRD3D: 0x10037930
 * @ghidraAddress BRGlide: 0x10030fd0
 */
bool LookupTextureMapEntry(unsigned dwAddress, int *pnWidth, int *pnHeight);

/**
 * Reverse the bytes of each word of a run of groups of three words.
 *
 * @param pData The groups.
 * @param nCount The number of groups.
 * @ghidraAddress BRD3D: 0x1002ba60
 * @ghidraAddress BRGlide: 0x10018ad0
 */
void ByteSwap3DWordsArray(unsigned char *pData, int nCount);

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

/** The number of entries of g_aTextureUploadQueue. */
#define TEXTURE_UPLOAD_QUEUE_SIZE 256

/**
 * The textures to upload, a ring from g_dwTextureUploadQueueTail to g_dwTextureUploadQueueHead.
 *
 * @ghidraAddress BRD3D: 0x118a9880
 * @ghidraAddress BRGlide: 0x118ec998
 */
extern TextureUploadRequest g_aTextureUploadQueue[TEXTURE_UPLOAD_QUEUE_SIZE];

/**
 * The entry of g_aTextureUploadQueue the next texture to queue takes.
 *
 * @ghidraAddress BRD3D: 0x118a9878
 * @ghidraAddress BRGlide: 0x118ec990
 */
extern int g_dwTextureUploadQueueHead;

/**
 * The entry of g_aTextureUploadQueue of the next texture to upload.
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
 * Empty the queue of textures to upload and close its mutex.
 *
 * @ghidraAddress BRD3D: 0x10074f40
 * @ghidraAddress BRGlide: 0x1006e1a0
 */
void DestroyTextureUploadQueue(void);

/**
 * Whether the data of a texture is copied when its pointers are relocated.
 *
 * @ghidraAddress BRD3D: 0x10675540
 * @ghidraAddress BRGlide: 0x105b16e8
 */
extern int g_dwCopyTextureDataOnRelocate;

/**
 * Set whether the data of a texture is copied when its pointers are relocated.
 *
 * @param bCopy Whether to copy.
 * @ghidraAddress BRD3D: 0x1002b9d0
 * @ghidraAddress BRGlide: 0x10018a40
 */
void SetCopyTextureDataOnRelocate(int bCopy);

/**
 * The category of the textures loaded next. Its values are not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x118aa088
 * @ghidraAddress BRGlide: 0x118ed1a0
 */
extern int g_dwTextureLoadCategory;

/**
 * Set the category of the textures loaded next.
 *
 * @param nCategory The category.
 * @ghidraAddress BRD3D: 0x10074dc0
 * @ghidraAddress BRGlide: 0x1006e020
 */
void SetTextureLoadCategory(int nCategory);

/** The number of entries of g_adwDamageTextureLevels. */
#define DAMAGE_TEXTURE_LEVEL_COUNT 8

/**
 * The levels of damage of the textures of the vehicles.
 *
 * @ghidraAddress BRD3D: 0x11829850
 * @ghidraAddress BRGlide: 0x1186c968
 */
extern DWORD g_adwDamageTextureLevels[DAMAGE_TEXTURE_LEVEL_COUNT];

/**
 * Clear g_adwDamageTextureLevels.
 *
 * @ghidraAddress BRD3D: 0x10074e00
 * @ghidraAddress BRGlide: 0x1006e060
 */
void ClearDamageTextureLevels(void);

/**
 * Copy g_adwDamageTextureLevels.
 *
 * @param pdwLevels Receives the #DAMAGE_TEXTURE_LEVEL_COUNT levels.
 * @ghidraAddress BRD3D: 0x10074e20
 * @ghidraAddress BRGlide: 0x1006e080
 */
void CopyDamageTextureLevels(DWORD *pdwLevels);

/**
 * Get the texels of a word of 8 bytes of the texture memory of the console for a size of texel.
 *
 * @param nSize The size of the texels, from 0 for 4 bits to 3 for 32 bits.
 * @return 16, 8, 4, or 2.
 * @ghidraAddress BRD3D: 0x10028c70
 * @ghidraAddress BRGlide: 0x10027f80
 */
int GetTexelsPerLoadWord(int nSize);

/**
 * Encode an input of the colour combiner for a field of 5 bits: 0 for zero becomes selector 31, 1
 * for one becomes selector 6, and 1000 more than a selector becomes the selector.
 *
 * @param nInput The input.
 * @return The selector.
 * @ghidraAddress BRD3D: 0x1002faf0
 * @ghidraAddress BRGlide: 0x1001d180
 */
int EncodeCombinerSelectorWide(int nInput);

/**
 * Encode an input of the colour combiner for a field of 3 bits: 0 for zero becomes selector 7, 1
 * for one becomes selector 6, 1013 becomes selector 0, and 1000 more than a selector becomes the
 * selector.
 *
 * @param nInput The input.
 * @return The selector.
 * @ghidraAddress BRD3D: 0x1002fac0
 * @ghidraAddress BRGlide: 0x1001d150
 */
int EncodeCombinerSelectorNarrow(int nInput);

struct GbiCommand;

/**
 * Build the command that sets the combine mode of two cycles from the inputs of the colour
 * combiner, as EncodeCombinerSelectorWide() and EncodeCombinerSelectorNarrow() take them, packed as
 * the console packs them.
 *
 * @param pCommand Receives the command.
 * @param nColorA0 The input a of the colour of the first cycle.
 * @param nColorB0 The input b of the colour of the first cycle.
 * @param nColorC0 The input c of the colour of the first cycle.
 * @param nColorD0 The input d of the colour of the first cycle.
 * @param nAlphaA0 The input a of the alpha of the first cycle.
 * @param nAlphaB0 The input b of the alpha of the first cycle.
 * @param nAlphaC0 The input c of the alpha of the first cycle.
 * @param nAlphaD0 The input d of the alpha of the first cycle.
 * @param nColorA1 The input a of the colour of the second cycle.
 * @param nColorB1 The input b of the colour of the second cycle.
 * @param nColorC1 The input c of the colour of the second cycle.
 * @param nColorD1 The input d of the colour of the second cycle.
 * @param nAlphaA1 The input a of the alpha of the second cycle.
 * @param nAlphaB1 The input b of the alpha of the second cycle.
 * @param nAlphaC1 The input c of the alpha of the second cycle.
 * @param nAlphaD1 The input d of the alpha of the second cycle.
 * @ghidraAddress BRD3D: 0x1002f900
 * @ghidraAddress BRGlide: 0x1001cf90
 */
void PackCombineMode(GbiCommand *pCommand,
                     int nColorA0,
                     int nColorB0,
                     int nColorC0,
                     int nColorD0,
                     int nAlphaA0,
                     int nAlphaB0,
                     int nAlphaC0,
                     int nAlphaD0,
                     int nColorA1,
                     int nColorB1,
                     int nColorC1,
                     int nColorD1,
                     int nAlphaA1,
                     int nAlphaB1,
                     int nAlphaC1,
                     int nAlphaD1);

/**
 * Load the texture of the overlay of the display of the race.
 *
 * @ghidraAddress BRD3D: 0x100737e0
 * @ghidraAddress BRGlide: 0x1006c750
 * @stub{BRGlide}
 */
void LoadHudOverlayTexture(void);

/**
 * Load the textures of the sets of glyphs of the display of the race.
 *
 * @ghidraAddress BRD3D: 0x10073820
 * @ghidraAddress BRGlide: 0x1006c790
 * @stub{BRGlide}
 */
void LoadHudGlyphTextureSets(void);

/**
 * @ghidraAddress BRD3D: 0x11829844
 */
extern unsigned g_dwAvailableTextureMemory;
/**
 * @ghidraAddress BRD3D: 0x11829848
 */
extern unsigned g_dwHiResTextureMemoryThreshold;
/**
 * @ghidraAddress BRD3D: 0x100a7dfc
 */
extern int g_dwMaxTexWidthHighQuality;
/**
 * @ghidraAddress BRD3D: 0x100a7e00
 */
extern int g_dwMaxTexHeightHighQuality;
/**
 * @ghidraAddress BRD3D: 0x100a7e04
 */
extern int g_dwMaxTexWidthLowQuality;
/**
 * @ghidraAddress BRD3D: 0x100a7e08
 */
extern int g_dwMaxTexHeightLowQuality;

/**
 * @ghidraAddress BRD3D: 0x1002a5a0
 */
void SelectTextureQualityFromVideoMemory(void);
/**
 * @ghidraAddress BRD3D: 0x1009b4c8
 */
extern unsigned char g_abHudFont0GlyphPixels[];
/**
 * @ghidraAddress BRD3D: 0x100946c8
 */
extern unsigned char g_abHudFont1GlyphPixels[];
/**
 * @ghidraAddress BRD3D: 0x100a4170
 */
extern unsigned char g_abHudFont2GlyphPixels[];
/**
 * @ghidraAddress BRD3D: 0x100a22d0
 */
extern unsigned char g_abHudFont3GlyphPixels[];
/**
 * @ghidraAddress BRD3D: 0x118aa0ac
 */
extern unsigned (*g_pfnCreateTextureFromN64Image)(const unsigned char *,
                                                  const unsigned char *,
                                                  int,
                                                  int,
                                                  int,
                                                  int,
                                                  int,
                                                  int,
                                                  int,
                                                  int,
                                                  int,
                                                  int,
                                                  int,
                                                  int,
                                                  int);

/**
 * @ghidraAddress BRD3D: 0x118aa0b0
 */
extern unsigned (*g_pfnCreateTextureFromN64ImageAutoLoadSize)(const unsigned char *,
                                                              const unsigned char *,
                                                              int,
                                                              int,
                                                              int,
                                                              int,
                                                              int,
                                                              int,
                                                              int,
                                                              int,
                                                              int,
                                                              int,
                                                              int,
                                                              int);

/**
 * @ghidraAddress BRD3D: 0x10028bf0
 */
unsigned CreateTextureFromN64ImageAutoLoadSize(const unsigned char *pImage,
                                               const unsigned char *pPalette,
                                               int nWidth,
                                               int nHeight,
                                               int nSize,
                                               int nFormat,
                                               int nOption6,
                                               int nOption7,
                                               int nOption8,
                                               int nOption9,
                                               int nOption10,
                                               int nOption11,
                                               int nOption12,
                                               int nOption13);

/**
 * @ghidraAddress BRD3D: 0x118aa0d8
 */
extern unsigned char g_abDaySkyPalette[32];
/**
 * @ghidraAddress BRD3D: 0x118aa8f8
 */
extern unsigned char g_abDaySkyTexture[2048];
/**
 * @ghidraAddress BRD3D: 0x118ab0f8
 */
extern unsigned char g_abNightSkyPalette[32];
/**
 * @ghidraAddress BRD3D: 0x118aa0f8
 */
extern unsigned char g_abNightSkyTexture[2048];
/**
 * @ghidraAddress BRD3D: 0x100a6498
 */
extern unsigned g_apVehicleReflectionTextures[3];

/**
 * @ghidraAddress BRD3D: 0x10073ac0
 */
void CreateDaySkyTexture(void);
/**
 * @ghidraAddress BRD3D: 0x10073b00
 */
void CreateNightSkyTexture(void);

/**
 * @ghidraAddress BRD3D: 0x100baac8
 */
extern unsigned char g_abHudOverlayTextureData[];
/**
 * @ghidraAddress BRD3D: 0x100b6ca8
 */
extern unsigned char g_abHudTexture64AData[];
/**
 * @ghidraAddress BRD3D: 0x100b9cb0
 */
extern unsigned char g_abHudTexture32x64Data[];
/**
 * @ghidraAddress BRD3D: 0x100b94a8
 */
extern unsigned char g_abHudTexture64BData[];
/**
 * @ghidraAddress BRD3D: 0x100b8ca0
 */
extern unsigned char g_abFallbackEnvTextureData[];
/**
 * @ghidraAddress BRD3D: 0x11829110
 */
extern unsigned g_dwHudOverlayTextureId;
/**
 * @ghidraAddress BRD3D: 0x1182931c
 */
extern unsigned g_dwHudOverlayHalfTextureId;
/**
 * @ghidraAddress BRD3D: 0x11829328
 */
extern unsigned g_dwParticleTextureId;
/**
 * @ghidraAddress BRD3D: 0x11829108
 */
extern unsigned g_dwTireTrailTextureId;
/**
 * @ghidraAddress BRD3D: 0x11829314
 */
extern unsigned g_dwVehicleEnvMapTextureId;
/**
 * @ghidraAddress BRD3D: 0x11829324
 */
extern unsigned g_dwFallbackEnvMapTextureId;

/**
 * @ghidraAddress BRD3D: 0x10024ae0
 */
unsigned DuplicateTextureEntryHalfScale(unsigned nTexture);

/**
 * @ghidraAddress BRD3D: 0x11829330
 */
extern unsigned char g_abAlphaRampRgba[16][4];
/**
 * @ghidraAddress BRD3D: 0x11829118
 */
extern unsigned char g_abAlphaRampPixelStrip[64];
/**
 * @ghidraAddress BRD3D: 0x11829318
 */
extern int g_dwAlphaRampStripRowCount;

/**
 * Create the texture of the particles, 64 by 64 texels.
 *
 * @ghidraAddress BRD3D: 0x10073950
 * @ghidraAddress BRGlide: 0x1006c800
 * @stub{BRGlide}
 */
void LoadParticleTexture(void);

/**
 * Create the texture of the trails of the tyres, 32 by 64 texels.
 *
 * @ghidraAddress BRD3D: 0x10073980
 * @ghidraAddress BRGlide: 0x1006c820
 * @stub{BRGlide}
 */
void LoadTireTrailTexture(void);

/**
 * Create the map of the environment of the vehicles, 64 by 64 texels.
 *
 * @ghidraAddress BRD3D: 0x100739b0
 * @ghidraAddress BRGlide: 0x1006c850
 * @stub{BRGlide}
 */
void LoadVehicleEnvMapTexture(void);

/**
 * Create the texture of the environment used when none is loaded.
 *
 * @ghidraAddress BRD3D: 0x100739e0
 * @ghidraAddress BRGlide: 0x1006c880
 * @stub{BRGlide}
 */
void CreateFallbackEnvironmentTexture(void);

/**
 * Create the textures of the ramps of alpha and of precipitation.
 *
 * @ghidraAddress BRD3D: 0x10073a10
 * @ghidraAddress BRGlide: 0x1006c8b0
 * @stub{BRGlide}
 */
void CreateAlphaRampTextures(void);

/**
 * Load and create the textures every race uses: the overlay and the glyphs of the display of the
 * race, the particles, the trails of the tyres, the maps of the environment, and the ramps of alpha
 * and precipitation.
 *
 * @ghidraAddress BRD3D: 0x10074dd0
 * @ghidraAddress BRGlide: 0x1006e030
 */
void CreateGlobalEffectTextures(void);

/**
 * Upload the next texture of the queue of textures to upload.
 *
 * @ghidraAddress BRD3D: 0x10074fc0
 * @ghidraAddress BRGlide: 0x1006e220
 */
void ProcessTextureUploadQueueEntry(void);

/**
 * Queue a texture to upload, under the mutex of the queue.
 *
 * @param nEntry The entry of the texture the game loaded.
 * @param payload The second argument of g_pfnUploadTextureEntryToSurface.
 * @ghidraAddress BRD3D: 0x10074f70
 * @ghidraAddress BRGlide: 0x1006e1d0
 */
void EnqueueTextureUploadEntry(int nEntry, TextureUploadData payload);

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

/** @ghidraAddress BRD3D: 0x104d51c0 */
extern unsigned char g_abWeatherShadeTexture[16];

/** @ghidraAddress BRD3D: 0x105553e4 */
extern unsigned g_dwWeatherShadeTextureId;

/**
 * Create a uniform intensity texture for the chosen weather.
 *
 * @ghidraAddress BRD3D: 0x1002a740
 */
void CreateWeatherShadeTexture(void);

/**
 * Query the free video memory available for textures on the active DirectDraw device.
 *
 * @return The free bytes reported by DirectDraw.
 * @ghidraAddress BRD3D: 0x1003e220
 */
unsigned QueryAvailableTextureMemory(void);

/**
 * Stop texture uploads and release texture surfaces and owned paint buffers.
 *
 * @ghidraAddress BRD3D: 0x1002a7a0
 */
void ShutdownTextureSubsystem(void);

/**
 * Copy a supplied pixel buffer into the device texture of a loaded entry.
 *
 * @param nEntry The loaded texture entry.
 * @param payload The pixel buffer.
 * @ghidraAddress BRD3D: 0x10028e00
 */
void UploadTextureEntryToSurface(int nEntry, TextureUploadData payload);

/**
 * Replace a static model texture's image with its loaded device texture entry.
 *
 * @param pTexture The model texture descriptor.
 * @param nOption13 The final texture creation option.
 * @ghidraAddress BRD3D: 0x1002a280
 */
void CreateDeviceTextureFromImageHeader(ModelTexture *pTexture, int nOption13);

/**
 * Derive a texture level of detail from its larger dimension.
 *
 * @param pLod Receives the level, clamped to zero for dimensions above 256.
 * @param width The width.
 * @param height The height.
 * @return Whether the larger dimension is at most 256.
 * @ghidraAddress BRD3D: 0x100251a0
 */
int CalculateTextureLodFromDimensions(int *pLod, int width, int height);

/**
 * Select a device pixel format from N64 texture size, format and combine state.
 *
 * @param nSize The N64 texel size.
 * @param nFormat The N64 image format.
 * @param bColorCombine The scan's color combine state.
 * @return The device pixel format identifier.
 * @ghidraAddress BRD3D: 0x10027b90
 */
int ChooseTexturePixelFormat(int nSize, int nFormat, int bColorCombine);

/**
 * Get the decoded bytes per texture pixel.
 *
 * @param nPixelFormatId The device pixel format identifier, ignored.
 * @return Four bytes.
 * @ghidraAddress BRD3D: 0x100257f0
 */
int GetTexturePixelSize(int nPixelFormatId);

/**
 * Find an entry with the same image, palette and applicable captured colors.
 *
 * @param pImage The image metadata to match.
 * @return The first matching entry, or -1.
 * @ghidraAddress BRD3D: 0x10028630
 */
int FindMatchingTextureEntry(const TextureImageData *pImage);

/**
 * Append image metadata and a device texture identifier to the loaded entries.
 *
 * @param pImage The image metadata.
 * @param nTextureId The device texture identifier.
 * @return The loaded texture entry.
 * @ghidraAddress BRD3D: 0x100284a0
 */
unsigned RegisterTextureEntry(const TextureImageData *pImage, int nTextureId);

/**
 * Bilinearly sample four decoded four-byte pixels.
 *
 * @param pDest Receives the sampled pixel.
 * @param pTopLeft The top-left pixel.
 * @param pTopRight The top-right pixel.
 * @param pBottomLeft The bottom-left pixel.
 * @param pBottomRight The bottom-right pixel.
 * @param x The horizontal interpolation fraction.
 * @param y The vertical interpolation fraction.
 * @ghidraAddress BRD3D: 0x100254f0
 */
void SampleTexturePixelBilinear(unsigned char *pDest,
                                const unsigned char *pTopLeft,
                                const unsigned char *pTopRight,
                                const unsigned char *pBottomLeft,
                                const unsigned char *pBottomRight,
                                float x,
                                float y);

/**
 * Resize a decoded texture with bilinear sampling at pixel centers.
 *
 * @param pDest The output pixels.
 * @param width The output width.
 * @param height The output height.
 * @param pSource The source pixels.
 * @param sourceWidth The source width.
 * @param sourceHeight The source height.
 * @param nPixelFormatId The decoded pixel format.
 * @ghidraAddress BRD3D: 0x10025350
 */
void ResizeTextureImage(unsigned char *pDest,
                        int width,
                        int height,
                        const unsigned char *pSource,
                        int sourceWidth,
                        int sourceHeight,
                        int nPixelFormatId);

/**
 * Decode an image descriptor into the texture pixel work buffers.
 *
 * @param pImage The image metadata.
 * @return The decoded pixels.
 * @ghidraAddress BRD3D: 0x10028860
 */
unsigned char *GetTextureImagePixels(TextureImageData *pImage);

/**
 * Decode a loaded entry again and upload the resulting pixels.
 *
 * @param nEntry The loaded texture entry.
 * @ghidraAddress BRD3D: 0x10029060
 */
void ReloadTextureEntryData(int nEntry);

/**
 * Resize consecutive decoded mip levels and record the decoded source dimensions.
 *
 * @param pDest The output mip pixels.
 * @param pSource The source mip pixels.
 * @param pImage The image metadata.
 * @return The output byte count.
 * @ghidraAddress BRD3D: 0x10028b20
 */
unsigned
ResizeTextureMipChain(unsigned char *pDest, const unsigned char *pSource, TextureImageData *pImage);

/**
 * The byte work buffer for resized decoded texture pixels.
 *
 * @ghidraAddress BRD3D: 0x104d51d0
 */
extern unsigned char g_abTextureResampleBuffer[];

/**
 * The byte work buffer for decoded texture pixels.
 *
 * @ghidraAddress BRD3D: 0x11829870
 */
extern unsigned char g_abDecodedTexturePixels[];

/**
 * Modulate decoded RGB by the half-sized intensity plane following its pixels.
 *
 * @param pImage The image metadata.
 * @param pPixels The decoded pixels and following intensity plane.
 * @ghidraAddress BRD3D: 0x10028a10
 */
void ModulateTextureWithIntensityPlane(const TextureImageData *pImage, unsigned char *pPixels);

/**
 * The byte count produced by the last decoded texture operation.
 *
 * @ghidraAddress BRD3D: 0x105553f8
 */
extern unsigned g_dwTexturePixelByteCount;

/**
 * Decode captured N64 texture tiles into four-byte pixels.
 *
 * @param pDest Receives decoded pixels.
 * @param nSize The output byte limit.
 * @param nTexelSize The N64 texel size.
 * @param pImage The captured image bytes.
 * @param pPalette The captured palette bytes.
 * @param nFormat The N64 image format.
 * @param bMirrorS Whether to append reflected columns to each row.
 * @param bMirrorT Whether to append reflected rows to each tile.
 * @param nTileIndex The first tile.
 * @param nTileLimit The exclusive tile limit.
 * @param pTiles The captured tile records.
 * @param flags The image control flags.
 * @param bColorCombine The captured combine state.
 * @param envR The environment red byte.
 * @param envG The environment green byte.
 * @param envB The environment blue byte.
 * @param envA The environment alpha byte.
 * @param primR The primitive red byte.
 * @param primG The primitive green byte.
 * @param primB The primitive blue byte.
 * @param primA The primitive alpha byte.
 * @param nRowSwapMask The row mask for swapped texture words.
 * @ghidraAddress BRD3D: 0x10025ab0
 */
void DecodeTexturePixels(unsigned char *pDest,
                         int nSize,
                         int nTexelSize,
                         const unsigned char *pImage,
                         const unsigned char *pPalette,
                         int nFormat,
                         int bMirrorS,
                         int bMirrorT,
                         int nTileIndex,
                         int nTileLimit,
                         const TexScanTile *pTiles,
                         unsigned flags,
                         int bColorCombine,
                         unsigned envR,
                         unsigned envG,
                         unsigned envB,
                         unsigned envA,
                         unsigned primR,
                         unsigned primG,
                         unsigned primB,
                         unsigned primA,
                         unsigned nRowSwapMask);

/**
 * Bypass texture metadata matching while creating a new device texture.
 *
 * @ghidraAddress BRD3D: 0x118aa09c
 */
extern int g_bForceTextureClone;

/**
 * Create a device texture from captured metadata and decoded pixels.
 *
 * @param pImage The image metadata.
 * @param pPixels The decoded pixels.
 * @return The registered texture entry.
 * @ghidraAddress BRD3D: 0x10028350
 */
int CreateTextureFromImage(TextureImageData *pImage, unsigned char *pPixels);

/**
 * Clone texture metadata with replacement image and palette bytes.
 *
 * @param nEntry The source texture entry.
 * @param pImage The replacement image bytes.
 * @param pPalette The replacement palette bytes.
 * @return The new texture entry.
 * @ghidraAddress BRD3D: 0x1002a350
 */
int CloneTextureEntryWithNewImageData(int nEntry,
                                      unsigned char *pImage,
                                      const unsigned char *pPalette);

/**
 * Prepare paint texture metadata and adjust device dimensions.
 *
 * @param pWidth Receives the device width.
 * @param pHeight Receives the device height.
 * @param pImage The image metadata to prepare.
 * @return 0 when paint is unavailable, otherwise 1.
 * @ghidraAddress BRD3D: 0x10024bf0
 */
int PreparePaintTexture(int *pWidth, int *pHeight, TextureImageData *pImage);

/**
 * Allocation label for the device texture table.
 *
 * @ghidraAddress BRD3D: 0x100a7e20
 */
extern const char g_szTidFromTextureAppendAmid[];

/**
 * Create device textures for animated track frames and replace their image offsets with entries.
 *
 * @param pTextures The track texture descriptors.
 * @param nCount The descriptor count.
 * @ghidraAddress BRD3D: 0x1002a430
 */
void UploadTrackTextureVariants(ModelTexture *pTextures, int nCount);

/**
 * Decode an N64 image into the pixel format selected for its texel size and format.
 *
 * @param pDest Receives device pixels.
 * @param pImage The source image bytes.
 * @param pPalette The source palette bytes.
 * @param nWidth The source width.
 * @param nHeight The source height.
 * @param nLineBytes The captured tile line size.
 * @param nSize The N64 texel size.
 * @param nFormat The N64 image format.
 * @return The device pixel byte count.
 * @ghidraAddress BRD3D: 0x10025830
 */
int DecodeN64TextureToPixelBuffer(void *pDest,
                                  const unsigned char *pImage,
                                  const unsigned char *pPalette,
                                  int nWidth,
                                  int nHeight,
                                  int nLineBytes,
                                  int nSize,
                                  int nFormat);

/**
 * Select an aspect ratio code from the truncated ratio of the dimensions.
 *
 * @param pAspectRatio Receives the aspect ratio code.
 * @param nWidth The width, nonzero when it is the divisor.
 * @param nHeight The height, nonzero when it is the divisor.
 * @return Whether the ratio in eighths truncates to 8, 16, 32 or 64.
 * @ghidraAddress BRD3D: 0x10028200
 */
int CalculateTextureAspectRatio(int *pAspectRatio, int nWidth, int nHeight);

/**
 * Create a device texture from an N64 image and its load options.
 *
 * @ghidraAddress BRD3D: 0x10027c60
 */
unsigned CreateTextureFromN64Image(const unsigned char *pImage,
                                   const unsigned char *pPalette,
                                   int nWidth,
                                   int nHeight,
                                   int nLineBytes,
                                   int nSize,
                                   int nFormat,
                                   int nOption7,
                                   int nOption8,
                                   int nOption9,
                                   int nOption10,
                                   int nOption11,
                                   int nOption12,
                                   int nOption13,
                                   int nOption14);

/**
 * Decode texture pixels, applying the selected paint damage image when needed.
 *
 * @param nEntry The texture entry.
 * @param pSize Receives the decoded byte count.
 * @return The decoded pixels.
 * @ghidraAddress BRD3D: 0x10028e70
 */
unsigned char *GetTextureEntryPixelsWithPaint(int nEntry, unsigned *pSize);

/**
 * @ghidraAddress BRD3D: 0x118aa0a4
 */
extern void (*g_pfnAliasTextureEntrySurface)(int nEntry, int nSourceEntry);
/**
 * @ghidraAddress BRD3D: 0x118aa0a8
 */
extern decltype(&DecodeN64TextureToPixelBuffer) g_pfnDecodeN64TextureToPixelBuffer;
/**
 * @ghidraAddress BRD3D: 0x118aa0c8
 */
extern decltype(&CreateDeviceTextureFromImageHeader) g_pfnCreateDeviceTextureFromImageHeader;
/**
 * @ghidraAddress BRD3D: 0x118aa084
 */
extern decltype(&CloneTextureEntryWithNewImageData) g_pfnCloneTextureEntryWithNewImageData;
/**
 * @ghidraAddress BRD3D: 0x118aa0cc
 */
extern decltype(&UploadTrackTextureVariants) g_pfnUploadTrackTextureVariants;

/**
 * The width limit selected for the current texture category.
 *
 * @ghidraAddress BRD3D: 0x100a7df4
 */
extern int g_dwCurrentMaxTextureWidth;
/**
 * The height limit selected for the current texture category.
 *
 * @ghidraAddress BRD3D: 0x100a7df8
 */
extern int g_dwCurrentMaxTextureHeight;
