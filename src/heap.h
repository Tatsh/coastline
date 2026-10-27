#ifndef COASTLINE_HEAP_H
#define COASTLINE_HEAP_H

/**
 * @file
 * The interface heap that screens load their data into, and the clearing of the frame buffers
 * between screens.
 */

#include <PR/ultratypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The next free byte of the interface heap.
 *
 * @ghidraAddress USA: 0x80369b70
 * @ghidraAddress EUR: 0x8036c070
 * @ghidraAddress JPN: 0x8036ace0
 * @ghidraAddress BRD3D: 0x106c7c38
 */
extern u8 *g_pHeapCurrent;

/**
 * Empty the heap.
 *
 * @ghidraAddress USA: 0x80242940
 * @ghidraAddress EUR: 0x80243e50
 * @ghidraAddress JPN: 0x80243950
 * @ghidraAddress BRD3D: 0x10036020
 */
void ResetHeap(void);

/**
 * Return the next free byte of the heap, to free what follows it later.
 *
 * @return The next free byte.
 * @ghidraAddress USA: 0x80242954
 * @ghidraAddress EUR: 0x80243e64
 * @ghidraAddress JPN: 0x80243964
 */
u8 *GetHeapCurrentAddress(void);

/**
 * Free everything after an address GetHeapCurrentAddress() returned.
 *
 * @param address The new next free byte.
 * @ghidraAddress USA: 0x80242960
 * @ghidraAddress EUR: 0x80243e70
 * @ghidraAddress JPN: 0x80243970
 */
void SetHeapCurrentAddress(u8 *address);

/**
 * Allocate memory aligned to 8 bytes. Running out is reported on the debug display, and the
 * allocation succeeds regardless.
 *
 * @param size The size in bytes.
 * @return The start of the allocation.
 * @ghidraAddress USA: 0x8024296c
 * @ghidraAddress EUR: 0x80243e7c
 * @ghidraAddress JPN: 0x8024397c
 */
void *AllocHeapAligned(u32 size);

/**
 * Blank the display and clear the frame buffers in the wide display mode.
 *
 * @ghidraAddress USA: 0x802429ec
 * @ghidraAddress EUR: 0x80243efc
 * @ghidraAddress JPN: 0x802439fc
 */
void ClearFramebuffersForWideMode(void);

/**
 * Blank the display and clear the frame buffers in the normal display mode.
 *
 * @ghidraAddress USA: 0x80242a7c
 * @ghidraAddress EUR: 0x80243f88
 * @ghidraAddress JPN: 0x80243a88
 */
void ClearFramebuffersForNormalMode(void);

#ifdef __cplusplus
}
#endif

#endif
