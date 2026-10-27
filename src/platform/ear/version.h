#ifndef COASTLINE_EAR_VERSION_H
#define COASTLINE_EAR_VERSION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x1000c8c0 */
uint32_t __stdcall EarGetVersionExportWrapper(void);

/** @ghidraAddress EAR: 0x1000c8ca */
uint32_t __stdcall EAR_DLL_GetVersion(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_VERSION_H */
