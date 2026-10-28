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

/**
 * @param code The code the game passes.
 * @return Whether the code is valid.
 * @ghidraAddress EAR: 0x10003d15
 */
uint32_t __stdcall ValidateEarNumericCode(uint32_t code);

/**
 * @param code The code the game passes.
 * @ghidraAddress EAR: 0x100051a7
 */
void __stdcall LoadEarConfiguration(uint32_t code);

/**
 * Validate the code of the game and load the configuration it selects.
 *
 * @param code The code the game passes.
 * @return Whether the code is valid.
 * @ghidraAddress EAR: 0x1000ba60
 */
int __stdcall EAR_DLL_AAA_Validate(uint32_t code);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_CONFIGURATION_H */
