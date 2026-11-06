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

/**
 * Nonzero when config.ear includes the global configuration section.
 * @ghidraAddress EAR: 0x1003b704
 */
extern int32_t g_bGlobalConfigurationFound;
/**
 * Configured hexadecimal product target from [PTARGET], compared with the version's high word.
 * @ghidraAddress EAR: 0x1003b708
 */
extern uint16_t g_wConfiguredProductTarget;
/**
 * Configured hexadecimal interface target from [ITARGET], compared with the version's high word.
 * @ghidraAddress EAR: 0x1003b70c
 */
extern uint16_t g_wConfiguredInterfaceTarget;
/**
 * Configured hexadecimal version from [VERSION], compared with the version's low word.
 * @ghidraAddress EAR: 0x1003b710
 */
extern uint16_t g_wConfiguredVersion;
/**
 * Nonzero when global [GNOIAS] requires the non-IAS interface target during title validation.
 * @ghidraAddress EAR: 0x1003b714
 */
extern int32_t g_bConfiguredGlobalNoIas;
/**
 * Whether the configuration records a DVD drive, from [DVDDRIVE].
 *
 * @ghidraAddress EAR: 0x1003b718
 */
extern int32_t g_bConfiguredDvdDrive;
/**
 * Whether the configuration records a CD changer, from [CHANGER].
 *
 * @ghidraAddress EAR: 0x1003b71c
 */
extern int32_t g_bConfiguredCdChanger;
/**
 * Whether the configuration records more than one disc, from [MULTI].
 *
 * @ghidraAddress EAR: 0x1003b720
 */
extern int32_t g_bConfiguredMultiDisc;
/**
 * Nonzero when config.ear includes the selected title configuration section.
 * @ghidraAddress EAR: 0x1003b724
 */
extern int32_t g_bTitleConfigurationFound;
/**
 * The title of the game in the configuration, from [TITLE], without its line break.
 *
 * @ghidraAddress EAR: 0x1003b728
 */
extern char g_szConfiguredTitle[80];
/**
 * Nonzero when title [NOIAS] requires the non-IAS interface target during title validation.
 * @ghidraAddress EAR: 0x1003b778
 */
extern int32_t g_bConfiguredTitleNoIas;
/**
 * Whether the title is on a DVD, from [ISDVD].
 *
 * @ghidraAddress EAR: 0x1003b77c
 */
extern int32_t g_bConfiguredTitleIsDvd;
/**
 * The times the title was launched, from [TIMES], counted up and written back at each load.
 *
 * @ghidraAddress EAR: 0x1003b780
 */
extern uint32_t g_dwConfiguredLaunchCount;
/**
 * Component mismatch bits, including target/version mismatch (1) and older/newer backend (8/2).
 * ValidateEarAudioComponents() clears the flags before testing configured components.
 * @ghidraAddress EAR: 0x1003b798
 */
extern uint16_t g_earAudioComponentValidationFlags;
/**
 * Nonzero once component validation is attempted, including failed validation.
 * Version queries then perform backend version checks.
 * @ghidraAddress EAR: 0x1003b7b4
 */
extern int32_t g_earAudioComponentsValidated;

/**
 * @param code The code the game passes.
 * @return Whether the code is valid.
 * @ghidraAddress EAR: 0x10003d15
 */
uint32_t __stdcall ValidateEarNumericCode(uint32_t code);

/**
 * Read the global section of config.ear in the system directory, then, unless the code reads only
 * that section, the section of the title the code selects, and write its launch count back one
 * higher.
 *
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
