#ifndef COASTLINE_TIRE_SPRAY_H
#define COASTLINE_TIRE_SPRAY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Move the spray of every racer around its vehicle, advance each segment by its velocity, age the
 * segments by one when a wheel begins a new one, and fade the old.
 *
 * @ghidraAddress USA: 0x8023bf60
 * @ghidraAddress EUR: 0x8023d470
 * @ghidraAddress JPN: 0x8023cf70
 * @ghidraAddress BRD3D: 0x100397c0
 */
void MoveTireSpray(void);

#ifdef __cplusplus
}
#endif

#endif
