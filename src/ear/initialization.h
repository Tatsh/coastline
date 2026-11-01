#ifndef COASTLINE_EAR_INITIALIZATION_H
#define COASTLINE_EAR_INITIALIZATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Forward library initialisation to EAR_DLL_InitializeEar().
 * @param flags The initialisation controls accepted by EAR_DLL_InitializeEar().
 * @return The underlying initialisation result, one for an accepted request or zero on failure.
 * @ghidraAddress EAR: 0x1000c980
 */
int __stdcall EarInitializeEarExportWrapper(uint32_t flags);
/**
 * Initialise audio output, registries, notification delivery, and the default environment.
 * Numeric-code validation must succeed first. Fatal initialisation failures shut down partial
 * resources while preserving the last EAR error. Post-initialisation timer and registration
 * results do not change the success return; inspect the error API when required.
 *
 * @param flags Controls comprising reserved option (1), unsupported stereo request (2), default
 * channel/matrix/timer setup (4), required title configuration (8), automatic error dialogs (0x40),
 * manual updates without a timer (0x80), component validation (0x100), and reduced smoothing
 * (0x200).
 * Flags 0x2000 and 0x4000 request unsupported features. A high word of 0xdeaf instead only sets
 * mixer/configuration-validation bypass markers for set low bits 1/2, without initialising audio.
 * Clear low bits do not reset established bypass markers.
 * @return One for bypass configuration, already-initialised components, or completed core setup;
 * zero for failed validation or fatal initialisation failure.
 * @ghidraAddress EAR: 0x1000c990
 */
int __stdcall EAR_DLL_InitializeEar(uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_INITIALIZATION_H */
