#ifndef COASTLINE_EAR_CONFIGURATION_H
#define COASTLINE_EAR_CONFIGURATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @ghidraAddress EAR: 0x10005d66
 * @stub
 */
int __stdcall ValidateEarTitleConfiguration(uint32_t required);
/**
 * @ghidraAddress EAR: 0x10005c48
 * @stub
 */
int ValidateEarAudioComponents(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_CONFIGURATION_H */
