#ifndef COASTLINE_EAR_CONFIGURATION_H
#define COASTLINE_EAR_CONFIGURATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Check that the configuration file configured the library and the game for the audio engine,
 * unless the check is disabled.
 *
 * @param required Whether a missing configuration shows a message and fails.
 * @return Whether the configuration is valid.
 * @ghidraAddress EAR: 0x10005d66
 */
int __stdcall ValidateEarTitleConfiguration(uint32_t required);

/**
 * Check that the loaded audio components are those the configuration records, and show a message
 * for each that is not.
 *
 * @return Whether every component is as configured.
 * @ghidraAddress EAR: 0x10005c48
 */
int ValidateEarAudioComponents(void);

/** @ghidraAddress EAR: 0x1003b704 */
extern int32_t g_bGlobalConfigurationFound;
/** @ghidraAddress EAR: 0x1003b708 */
extern uint16_t g_wConfiguredProductTarget;
/** @ghidraAddress EAR: 0x1003b70c */
extern uint16_t g_wConfiguredInterfaceTarget;
/** @ghidraAddress EAR: 0x1003b710 */
extern uint16_t g_wConfiguredVersion;
/** @ghidraAddress EAR: 0x1003b714 */
extern int32_t g_bConfiguredGlobalNoIas;
/** @ghidraAddress EAR: 0x1003b724 */
extern int32_t g_bTitleConfigurationFound;
/** @ghidraAddress EAR: 0x1003b778 */
extern int32_t g_bConfiguredTitleNoIas;
/** @ghidraAddress EAR: 0x1003b798 */
extern uint16_t g_earAudioComponentValidationFlags;
/** @ghidraAddress EAR: 0x1003b7b4 */
extern int32_t g_earAudioComponentsValidated;

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
