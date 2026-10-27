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

/** @ghidraAddress EAR: 0x1003b7b4 */
extern int32_t g_earAudioComponentsValidated;

/** @ghidraAddress EAR: 0x1003b798 */
extern uint16_t g_earAudioComponentValidationFlags;

#ifdef __cplusplus
}
#endif

#endif
