#ifndef COASTLINE_EAR_PLAYBACKSMOOTHING_H
#define COASTLINE_EAR_PLAYBACKSMOOTHING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @ghidraAddress EAR: 0x100049ab */
void __stdcall ConfigurePlaybackSmoothingPasses(uint32_t reduced);
/** @ghidraAddress EAR: 0x100360b8 */
extern uint16_t g_wPlaybackSmoothingPassCount;
/** @ghidraAddress EAR: 0x1003b7b8 */
extern int g_earPlaybackSmoothingEnabled;

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_PLAYBACKSMOOTHING_H */
