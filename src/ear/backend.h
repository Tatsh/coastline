#ifndef COASTLINE_EAR_BACKEND_H
#define COASTLINE_EAR_BACKEND_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x1001010b */
int __stdcall GetDirectSoundVersion(void);

/**
 * @ghidraAddress EAR: 0x1000fc00
 */
int __stdcall QueryDirectSoundProductVersion(void);

/** @ghidraAddress EAR: 0x10010115 */
int __stdcall ValidateEarStereoSolution(uint32_t enabled);
/** @ghidraAddress EAR: 0x10010132 */
int __stdcall SetReservedEarInitializationOption(uint32_t option);
/** @ghidraAddress EAR: 0x100100fd */
int RejectUnsupportedInitializationFeature4000(void);
/** @ghidraAddress EAR: 0x10010104 */
int RejectUnsupportedInitializationFeature2000(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_BACKEND_H */
