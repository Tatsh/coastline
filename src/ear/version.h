#ifndef COASTLINE_EAR_VERSION_H
#define COASTLINE_EAR_VERSION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @return The packed EAR build and version through EAR_DLL_GetVersion(), with its query effects.
 * @ghidraAddress EAR: 0x1000c8c0
 */
uint32_t __stdcall EarGetVersionExportWrapper(void);

/**
 * Clear the last EAR error and query the backend when component validation has been attempted.
 * Backend mismatch flags can change without changing the reported library version.
 * @return Build 0x1096 in the high word and major 1/minor 54 in the low word's high/low bytes.
 * @ghidraAddress EAR: 0x1000c8ca
 */
uint32_t __stdcall EAR_DLL_GetVersion(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_VERSION_H */
