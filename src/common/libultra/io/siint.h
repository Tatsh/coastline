#ifndef LIBULTRA_IO_SIINT_H
#define LIBULTRA_IO_SIINT_H

/**
 * @file
 * The SI access queue and the raw SI routines.
 */

#include <ultra64.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress USA: 0x802a6280
 * @ghidraAddress EUR: 0x802a7d70
 * @ghidraAddress JPN: 0x802a7860
 */
extern u32 __osSiAccessQueueEnabled;

/**
 * @ghidraAddress USA: 0x80381888
 * @ghidraAddress EUR: 0x803837c8
 * @ghidraAddress JPN: 0x80382438
 */
extern OSMesgQueue __osSiAccessQueue;

/**
 * @ghidraAddress USA: 0x80381880
 * @ghidraAddress EUR: 0x803837c0
 * @ghidraAddress JPN: 0x80382430
 */
extern OSMesg siAccessBuf[1];

/**
 * @ghidraAddress USA: 0x80269410
 * @ghidraAddress EUR: 0x8026aa80
 * @ghidraAddress JPN: 0x8026a530
 */
void __osSiCreateAccessQueue(void);

/**
 * @ghidraAddress USA: 0x80269460
 * @ghidraAddress EUR: 0x8026aad0
 * @ghidraAddress JPN: 0x8026a580
 */
void __osSiGetAccess(void);

/**
 * @ghidraAddress USA: 0x802694a4
 * @ghidraAddress EUR: 0x8026ab14
 * @ghidraAddress JPN: 0x8026a5c4
 */
void __osSiRelAccess(void);

/**
 * @ghidraAddress USA: 0x802694d0
 * @ghidraAddress EUR: 0x8026b320
 * @ghidraAddress JPN: 0x8026add0
 */
s32 __osSiRawStartDma(s32 direction, void *dramAddr);

/**
 * @ghidraAddress USA: 0x8026ba20
 * @ghidraAddress EUR: 0x8026d9f0
 * @ghidraAddress JPN: 0x8026d4a0
 */
s32 __osSiRawReadIo(u32 devAddr, u32 *data);

/**
 * @ghidraAddress USA: 0x8026ba70
 * @ghidraAddress EUR: 0x8026da40
 * @ghidraAddress JPN: 0x8026d4f0
 */
s32 __osSiRawWriteIo(u32 devAddr, u32 data);

/**
 * @ghidraAddress USA: 0x8026dbb0
 * @ghidraAddress EUR: 0x8026fb80
 * @ghidraAddress JPN: 0x8026f630
 */
int __osSiDeviceBusy(void);

#ifdef __cplusplus
}
#endif

#endif /* LIBULTRA_IO_SIINT_H */
