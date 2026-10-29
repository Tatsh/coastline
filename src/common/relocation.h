#ifndef COASTLINE_RELOCATION_H
#define COASTLINE_RELOCATION_H

/**
 * @file
 * The byte swapping and the relocation of the pointers of the data the game loads from files.
 */

#include "common/gbicommand.h"
#include "common/render.h"
#include "common/track.h"
#include "common/vecmath.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress BRD3D: 0x1002bf80
 * @ghidraAddress BRGlide: 0x10019040
 */
void ByteSwapAndRelocateDisplayList(GbiCommand *pCommands);

/**
 * @ghidraAddress BRD3D: 0x10690bec
 * @ghidraAddress BRGlide: 0x106b7c7c
 */
extern unsigned char *g_pTrackTextureTable;

/**
 * @ghidraAddress BRD3D: 0x1002bc90
 * @ghidraAddress BRGlide: 0x10018d50
 */
void ByteSwapTextureVariantTable(TextureAnimation *pAnimation);

/**
 * @ghidraAddress BRD3D: 0x1002baa0
 * @ghidraAddress BRGlide: 0x10018b60
 */
void ByteSwapAndRelocateTextureDescriptor(ModelTexture *pTexture);

/**
 * @ghidraAddress BRD3D: 0x1002ba80
 * @ghidraAddress BRGlide: 0x10018b40
 */
void ByteSwapTextureDescriptorArray(ModelTexture *pTextures, int nCount);

/**
 * The number of pointers relocated.
 *
 * @ghidraAddress BRD3D: 0x1067b548
 * @ghidraAddress BRGlide: 0x105b76f0
 */
extern int g_dwRelocatedPointerCount;

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
int IsPointerInRelocatedList(void *pPointer);

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
 * Swap the two bytes of each of the four halfwords of a triangle of the track.
 *
 * @param pData The triangle.
 * @ghidraAddress BRD3D: 0x1002ba20
 * @ghidraAddress BRGlide: 0x10018a90
 */
void ByteSwapFourShorts(TrackSurface *pData);

/**
 * Swap the bytes of each halfword of a run of groups of four halfwords.
 *
 * @param pData The groups.
 * @param nCount The number of groups.
 * @ghidraAddress BRD3D: 0x1002ba00
 * @ghidraAddress BRGlide: 0x10018a70
 */
void ByteSwapShortQuadArray(TrackSurface *pData, int nCount);

/**
 * Reverse the four bytes of each component of a vector.
 *
 * @param pData The vector.
 * @ghidraAddress BRD3D: 0x100383c0
 * @ghidraAddress BRGlide: 0x10018af0
 */
void ByteSwap3DWords(Vec3f *pData);

/**
 * Reverse the bytes of each component of a run of vectors.
 *
 * @param pData The vectors.
 * @param nCount The number of vectors.
 * @ghidraAddress BRD3D: 0x1002ba60
 * @ghidraAddress BRGlide: 0x10018ad0
 */
void ByteSwap3DWordsArray(Vec3f *pData, int nCount);

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

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RELOCATION_H */
