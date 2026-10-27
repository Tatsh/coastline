#ifndef COASTLINE_EAR_INITIALIZATION_H
#define COASTLINE_EAR_INITIALIZATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x1000c980 */
int __stdcall EarInitializeEarExportWrapper(uint32_t flags);
/** @ghidraAddress EAR: 0x1000c990 */
int __stdcall EAR_DLL_InitializeEar(uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif
