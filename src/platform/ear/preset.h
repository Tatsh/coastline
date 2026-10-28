#ifndef COASTLINE_EAR_PRESET_H
#define COASTLINE_EAR_PRESET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
