#ifndef COASTLINE_EAR_PRESET_H
#define COASTLINE_EAR_PRESET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @param source The path of the wave file, or the wave file in memory.
 * @param control The handle of the preset in the low word, 0 for a new one, and the kind of the
 * source in the high word.
 * @return The handle of the preset, or 0.
 * @ghidraAddress EAR: 0x1000eca0
 */
uint32_t __stdcall EarRegisterPresetExportWrapper(const void *source, uint32_t control);

/**
 * Load a wave sample from a file or from memory and register it as a preset.
 *
 * @param source The path of the wave file, or the wave file in memory.
 * @param control The handle of the preset in the low word, 0 for a new one, and the kind of the
 * source in the high word, 0 for a file and 2 for memory.
 * @return The handle of the preset, or 0.
 * @ghidraAddress EAR: 0x1000ecb4
 */
uint32_t __stdcall EAR_DLL_RegisterPreset(const void *source, uint32_t control);

/**
 * @param preset The preset, or 0 for every preset.
 * @return Whether the preset was shut down.
 * @ghidraAddress EAR: 0x1000f710
 */
int __stdcall EarShutDownPresetExportWrapper(uint32_t preset);

/**
 * Shut down a preset and its samples, or every preset.
 *
 * @param preset The preset, or 0 for every preset.
 * @return Whether the preset was shut down.
 * @ghidraAddress EAR: 0x1000f720
 */
int __stdcall EAR_DLL_ShutDownPreset(uint32_t preset);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_PRESET_H */
