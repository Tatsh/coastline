#ifndef COASTLINE_UTIL_H
#define COASTLINE_UTIL_H

/**
 * @file
 * Walks of the track model, the blocks of display data allocated afresh each frame, and the sort
 * the game uses, which follows the qsort() of the Microsoft C runtime.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "track.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The blocks of each kind a frame can allocate before they run out. */
#define FRAME_DISPLAY_LIST_BLOCKS 256 /*!< The display list blocks of a frame. */
#define FRAME_HILITE_BLOCKS 20        /*!< The specular highlight blocks of a frame. */
#define FRAME_LIGHT_BLOCKS 20         /*!< The light blocks of a frame. */

/** A block of eight display list commands. */
typedef struct {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    union {
        u64 commands[8];
        f32 matrix[4][4];
    };
#else
    u64 commands[8]; /*!< The commands, one 64-bit word each. */
#endif
} DisplayListBlock;

/**
 * The display list blocks of the two frames. The entry after the last of each frame takes every
 * allocation beyond it.
 *
 * @ghidraAddress USA: 0x8036ae50
 * @ghidraAddress EUR: 0x8036d350
 * @ghidraAddress JPN: 0x8036bfc0
 * @ghidraAddress BRD3D: 0x10af9bc0
 */
extern DisplayListBlock g_aDisplayListBlocks[2][FRAME_DISPLAY_LIST_BLOCKS + 1];

/**
 * @ghidraAddress USA: 0x80372ed0
 * @ghidraAddress EUR: 0x803753d0
 * @ghidraAddress JPN: 0x80374040
 * @ghidraAddress BRD3D: 0x10b02190
 */
extern Hilite g_aHiliteBlocks[2][FRAME_HILITE_BLOCKS + 1];

/**
 * @ghidraAddress USA: 0x80373170
 * @ghidraAddress EUR: 0x80375670
 * @ghidraAddress JPN: 0x803742e0
 * @ghidraAddress BRD3D: 0x10b01c50
 */
extern LookAt g_aLightBlocks[2][FRAME_LIGHT_BLOCKS + 1];

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/**
 * The blocks the current frame has allocated.
 *
 * @ghidraAddress USA: 0x8028dfb0
 * @ghidraAddress EUR: 0x80290770
 * @ghidraAddress JPN: 0x80290210
 * @ghidraAddress BRD3D: 0x10b01c40
 */
extern s32 g_dwDisplayListBlockCount;
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/**
 * @ghidraAddress USA: 0x8028dfb4
 * @ghidraAddress EUR: 0x80290774
 * @ghidraAddress JPN: 0x80290214
 * @ghidraAddress BRD3D: 0x10b01c48
 */
extern s32 g_dwHiliteBlockCount;
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/**
 * @ghidraAddress USA: 0x8028dfb8
 * @ghidraAddress EUR: 0x80290778
 * @ghidraAddress JPN: 0x80290218
 * @ghidraAddress BRD3D: 0x10b01c44
 */
extern s32 g_dwLightBlockCount;
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */

/**
 * The frame buffer the current frame draws into.
 *
 * @ghidraAddress USA: 0x8028a85c
 * @ghidraAddress EUR: 0x8028ca5c
 * @ghidraAddress JPN: 0x8028c52c
 * @ghidraAddress BRD3D: 0x106c65ec
 */
extern s32 g_dwFrameBufferIndex;

/** A routine that orders two elements as strcmp() orders strings. */
typedef s32 (*CompareFunc)(const void *a, const void *b);

/** A routine that exchanges two elements of a size. */
typedef void (*SwapFunc)(void *a, void *b, u32 size);

/**
 * Mark every section of a tree of the route as visited, skipping sections already visited and
 * alternate branches. On the strip mine and its mirrored version, a section whose grid starts at
 * row 2 starts at row 0.
 *
 * @param section The first section of the tree.
 * @ghidraAddress USA: 0x80255ba0
 * @ghidraAddress EUR: 0x80257170
 * @ghidraAddress JPN: 0x80256b00
 * @ghidraAddress BRD3D: 0x10061660
 * @ghidraAddress BRGlide: 0x1005a6e0
 */
void MarkTrackSections(TrackSection *section);

/**
 * Clear the visited mark of every section of a tree of the route.
 *
 * @param section The first section of the tree.
 * @ghidraAddress USA: 0x80255c50
 * @ghidraAddress EUR: 0x80257220
 * @ghidraAddress JPN: 0x80256bb0
 * @ghidraAddress BRD3D: 0x100616c0
 * @ghidraAddress BRGlide: 0x1005a740
 */
void UnmarkTrackSections(TrackSection *section);

/**
 * Adjust the route of the loaded track for the current track.
 *
 * @ghidraAddress USA: 0x80255ca0
 * @ghidraAddress EUR: 0x80257270
 * @ghidraAddress JPN: 0x80256c00
 * @ghidraAddress BRD3D: 0x10061700
 * @ghidraAddress BRGlide: 0x1005a780
 */
void PrepareTrackSections(void);

/**
 * Allocate a block of display list commands for the current frame.
 *
 * @return The block, or the spare block after the last once every block is taken.
 * @ghidraAddress USA: 0x80255cd0
 * @ghidraAddress EUR: 0x802572a0
 * @ghidraAddress JPN: 0x80256c30
 * @ghidraAddress BRD3D: 0x10069490
 * @ghidraAddress BRGlide: 0x10062500
 */
DisplayListBlock *AllocDisplayListBlock(void);

/**
 * Allocate a block of highlight coordinates for the current frame.
 *
 * @return The block, or the spare block after the last once every block is taken.
 * @ghidraAddress USA: 0x80255d4c
 * @ghidraAddress EUR: 0x8025731c
 * @ghidraAddress JPN: 0x80256cac
 * @ghidraAddress BRD3D: 0x100694e0
 * @ghidraAddress BRGlide: 0x10062550
 */
Hilite *AllocHiliteBlock(void);

/**
 * Allocate a block of reflection directions for the current frame.
 *
 * @return The block, or the spare block after the last once every block is taken.
 * @ghidraAddress USA: 0x80255dd8
 * @ghidraAddress EUR: 0x802573a8
 * @ghidraAddress JPN: 0x80256d38
 * @ghidraAddress BRD3D: 0x10069530
 * @ghidraAddress BRGlide: 0x100625a0
 */
LookAt *AllocLightBlock(void);

/**
 * Start the allocations of a new frame.
 *
 * @ghidraAddress USA: 0x80255e64
 * @ghidraAddress EUR: 0x80257434
 * @ghidraAddress JPN: 0x80256dc4
 * @ghidraAddress BRD3D: 0x10069580
 * @ghidraAddress BRGlide: 0x100625f0
 */
void ResetFrameBlocks(void);

/**
 * Sort count elements of a size with a comparison.
 *
 * @param base The first element.
 * @param count The number of elements.
 * @param size The size of each element, in bytes.
 * @param compare The comparison that orders two elements.
 * @ghidraAddress USA: 0x80255e80
 * @ghidraAddress EUR: 0x80257450
 * @ghidraAddress JPN: 0x80256de0
 */
void QuickSort(void *base, u32 count, u32 size, CompareFunc compare);

/**
 * Sort the elements from lo to hi by selection, for the short ranges of QuickSort().
 *
 * @param lo The first element.
 * @param hi The last element, included in the sort.
 * @param size The size of each element, in bytes.
 * @param compare The comparison that orders two elements.
 * @param swap The routine that exchanges two elements.
 * @ghidraAddress USA: 0x802560dc
 * @ghidraAddress EUR: 0x802576ac
 * @ghidraAddress JPN: 0x8025703c
 */
void ShortSort(u8 *lo, u8 *hi, u32 size, CompareFunc compare, SwapFunc swap);

/**
 * Exchange two buffers byte by byte.
 *
 * @param a The first buffer.
 * @param b The second buffer.
 * @param size The size of each buffer, in bytes.
 * @ghidraAddress USA: 0x802561a8
 * @ghidraAddress EUR: 0x80257778
 * @ghidraAddress JPN: 0x80257108
 */
void ExchangeByteBuffers(u8 *a, u8 *b, u32 size);

/**
 * Exchange two buffers halfword by halfword.
 *
 * @param a The first buffer.
 * @param b The second buffer.
 * @param size The size of each buffer, in bytes.
 * @ghidraAddress USA: 0x802561e4
 * @ghidraAddress EUR: 0x802577b4
 * @ghidraAddress JPN: 0x80257144
 */
void ExchangeHalfwordBuffers(s16 *a, s16 *b, u32 size);

/**
 * Exchange two buffers word by word.
 *
 * @param a The first buffer.
 * @param b The second buffer.
 * @param size The size of each buffer, in bytes.
 * @ghidraAddress USA: 0x80256228
 * @ghidraAddress EUR: 0x802577f8
 * @ghidraAddress JPN: 0x80257188
 */
void ExchangeWordBuffers(u32 *a, u32 *b, u32 size);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_UTIL_H */
