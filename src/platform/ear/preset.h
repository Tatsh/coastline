#ifndef COASTLINE_EAR_PRESET_H
#define COASTLINE_EAR_PRESET_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Shut down a preset, or every preset.
 *
 * @param preset The preset, or 0 for every preset.
 * @return Whether the preset was shut down.
 * @ghidraAddress EAR: 0x1000f710
 * @stub
 */
int __stdcall EarShutDownPresetExportWrapper(uint32_t preset);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_PRESET_H */
