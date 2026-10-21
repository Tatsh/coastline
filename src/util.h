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

/** The blocks of each kind a frame can allocate before they run out. */
#define FRAME_DISPLAY_LIST_BLOCKS 256
#define FRAME_HILITE_BLOCKS 20
#define FRAME_LIGHT_BLOCKS 20

/** A block of eight display list commands. */
typedef struct {
    u64 commands[8]; /*!< The commands, one 64-bit word each. */
} DisplayListBlock;

/**
 * The display list blocks of the two frames. The entry after the last of each frame takes every
 * allocation beyond it.
 *
 * @ghidraAddress USA: 0x8036ae50
 */
extern DisplayListBlock g_aDisplayListBlocks[2][FRAME_DISPLAY_LIST_BLOCKS + 1];

/** @ghidraAddress USA: 0x80372ed0 */
extern Hilite g_aHiliteBlocks[2][FRAME_HILITE_BLOCKS + 1];

/** @ghidraAddress USA: 0x80373170 */
extern LookAt g_aLightBlocks[2][FRAME_LIGHT_BLOCKS + 1];

/** The blocks the current frame has allocated. @ghidraAddress USA: 0x8028dfb0 */
extern s32 g_dwDisplayListBlockCount;

/** @ghidraAddress USA: 0x8028dfb4 */
extern s32 g_dwHiliteBlockCount;

/** @ghidraAddress USA: 0x8028dfb8 */
extern s32 g_dwLightBlockCount;

/** The frame buffer the current frame draws into. @ghidraAddress USA: 0x8028a85c */
extern s32 g_dwFrameBufferIndex;

/** A routine that orders two elements as strcmp() orders strings. */
typedef s32 (*CompareFunc)(const void *a, const void *b);

/** A routine that exchanges two elements of a size. */
typedef void (*SwapFunc)(void *a, void *b, u32 size);

/**
 * Mark every section of a tree of the route as visited, skipping sections already visited and
 * alternate branches. On tracks 3 and 8, a section whose grid starts at row 2 starts at row 0.
 *
 * @param section The first section of the tree.
 * @ghidraAddress USA: 0x80255ba0
 */
void MarkTrackSections(TrackSection *section);

/**
 * Clear the visited mark of every section of a tree of the route.
 *
 * @param section The first section of the tree.
 * @ghidraAddress USA: 0x80255c50
 */
void UnmarkTrackSections(TrackSection *section);

/**
 * Adjust the route of the loaded track for the current track.
 *
 * @ghidraAddress USA: 0x80255ca0
 */
void PrepareTrackSections(void);

/**
 * Allocate a block of display list commands for the current frame.
 *
 * @return The block, or the spare block after the last once every block is taken.
 * @ghidraAddress USA: 0x80255cd0
 */
DisplayListBlock *AllocDisplayListBlock(void);

/**
 * Allocate a block of highlight coordinates for the current frame.
 *
 * @return The block, or the spare block after the last once every block is taken.
 * @ghidraAddress USA: 0x80255d4c
 */
Hilite *AllocHiliteBlock(void);

/**
 * Allocate a block of reflection directions for the current frame.
 *
 * @return The block, or the spare block after the last once every block is taken.
 * @ghidraAddress USA: 0x80255dd8
 */
LookAt *AllocLightBlock(void);

/**
 * Start the allocations of a new frame.
 *
 * @ghidraAddress USA: 0x80255e64
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
 */
void ShortSort(u8 *lo, u8 *hi, u32 size, CompareFunc compare, SwapFunc swap);

/**
 * Exchange two buffers byte by byte.
 *
 * @param a The first buffer.
 * @param b The second buffer.
 * @param size The size of each buffer, in bytes.
 * @ghidraAddress USA: 0x802561a8
 */
void ExchangeByteBuffers(void *a, void *b, u32 size);

/**
 * Exchange two buffers halfword by halfword.
 *
 * @param a The first buffer.
 * @param b The second buffer.
 * @param size The size of each buffer, in bytes.
 * @ghidraAddress USA: 0x802561e4
 */
void ExchangeHalfwordBuffers(void *a, void *b, u32 size);

/**
 * Exchange two buffers word by word.
 *
 * @param a The first buffer.
 * @param b The second buffer.
 * @param size The size of each buffer, in bytes.
 * @ghidraAddress USA: 0x80256228
 */
void ExchangeWordBuffers(void *a, void *b, u32 size);

#endif
