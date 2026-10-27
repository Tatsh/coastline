#ifndef COASTLINE_LAP_TIME_H
#define COASTLINE_LAP_TIME_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write a time as minutes, seconds, and hundredths.
 *
 * @param out The buffer to write to.
 * @param seconds The time, in seconds.
 * @ghidraAddress USA: 0x8020cf44
 * @ghidraAddress EUR: 0x8020cd24
 * @ghidraAddress JPN: 0x8020c91c
 * @ghidraAddress BRD3D: 0x100020d0
 */
void FormatLapTime(char *out, f32 seconds);

#ifdef __cplusplus
}
#endif

#endif
