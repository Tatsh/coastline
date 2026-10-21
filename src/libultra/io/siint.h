#ifndef LIBULTRA_IO_SIINT_H
#define LIBULTRA_IO_SIINT_H

/**
 * @file
 * The SI access queue and the raw SI routines.
 */

#include <ultra64.h>

/**
 * @ghidraAddress USA: 0x802a6280
 * @ghidraAddress JPN: 0x802a7860
 * @ghidraAddress EUR: 0x802a7d70
 */
extern u32 __osSiAccessQueueEnabled;

/**
 * @ghidraAddress USA: 0x80381888
 * @ghidraAddress JPN: 0x80382438
 * @ghidraAddress EUR: 0x803837c8
 */
extern OSMesgQueue __osSiAccessQueue;

/**
 * @ghidraAddress USA: 0x80381880
 * @ghidraAddress JPN: 0x80382430
 * @ghidraAddress EUR: 0x803837c0
 */
extern OSMesg siAccessBuf[1];

/**
 * @ghidraAddress USA: 0x80269410
 * @ghidraAddress JPN: 0x8026a530
 * @ghidraAddress EUR: 0x8026aa80
 */
void __osSiCreateAccessQueue(void);

/**
 * @ghidraAddress USA: 0x80269460
 * @ghidraAddress JPN: 0x8026a580
 * @ghidraAddress EUR: 0x8026aad0
 */
void __osSiGetAccess(void);

/**
 * @ghidraAddress USA: 0x802694a4
 * @ghidraAddress JPN: 0x8026a5c4
 * @ghidraAddress EUR: 0x8026ab14
 */
void __osSiRelAccess(void);

/**
 * @ghidraAddress USA: 0x802694d0
 * @ghidraAddress JPN: 0x8026add0
 * @ghidraAddress EUR: 0x8026b320
 */
s32 __osSiRawStartDma(s32 direction, void *dramAddr);

/**
 * @ghidraAddress USA: 0x8026ba20
 * @ghidraAddress JPN: 0x8026d4a0
 * @ghidraAddress EUR: 0x8026d9f0
 */
s32 __osSiRawReadIo(u32 devAddr, u32 *data);

/**
 * @ghidraAddress USA: 0x8026ba70
 * @ghidraAddress JPN: 0x8026d4f0
 * @ghidraAddress EUR: 0x8026da40
 */
s32 __osSiRawWriteIo(u32 devAddr, u32 data);

/**
 * @ghidraAddress USA: 0x8026dbb0
 * @ghidraAddress JPN: 0x8026f630
 * @ghidraAddress EUR: 0x8026fb80
 */
int __osSiDeviceBusy(void);

#endif
