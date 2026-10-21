#ifndef COASTLINE_HEAP_H
#define COASTLINE_HEAP_H

/**
 * @file
 * The interface heap that screens load their data into, and the clearing of the frame buffers
 * between screens.
 */

#include <PR/ultratypes.h>

/**
 * The next free byte of the interface heap.
 *
 * @ghidraAddress USA: 0x80369b70
 * @ghidraAddress JPN: 0x8036ace0
 * @ghidraAddress EUR: 0x8036c070
 */
extern u8 *g_pHeapCurrent;

/**
 * Empty the heap.
 *
 * @ghidraAddress USA: 0x80242940
 * @ghidraAddress JPN: 0x80243950
 * @ghidraAddress EUR: 0x80243e50
 */
void ResetHeap(void);

/**
 * Return the next free byte of the heap, to free what follows it later.
 *
 * @return The next free byte.
 * @ghidraAddress USA: 0x80242954
 */
u8 *GetHeapCurrentAddress(void);

/**
 * Free everything after an address GetHeapCurrentAddress() returned.
 *
 * @param address The new next free byte.
 * @ghidraAddress USA: 0x80242960
 */
void SetHeapCurrentAddress(u8 *address);

/**
 * Allocate memory aligned to 8 bytes. Running out is reported on the debug display, and the
 * allocation succeeds regardless.
 *
 * @param size The size in bytes.
 * @return The start of the allocation.
 * @ghidraAddress USA: 0x8024296c
 * @ghidraAddress JPN: 0x8024397c
 * @ghidraAddress EUR: 0x80243e7c
 */
void *AllocHeapAligned(u32 size);

/**
 * Blank the display and clear the frame buffers in the wide display mode.
 *
 * @ghidraAddress USA: 0x802429ec
 * @ghidraAddress JPN: 0x802439fc (stub)
 * @ghidraAddress EUR: 0x80243efc (stub)
 */
void ClearFramebuffersForWideMode(void);

/**
 * Blank the display and clear the frame buffers in the normal display mode.
 *
 * @ghidraAddress USA: 0x80242a7c
 * @ghidraAddress JPN: 0x80243a88 (stub)
 * @ghidraAddress EUR: 0x80243f88 (stub)
 */
void ClearFramebuffersForNormalMode(void);

#endif
