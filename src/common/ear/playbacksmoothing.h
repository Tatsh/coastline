#ifndef COASTLINE_EAR_PLAYBACKSMOOTHING_H
#define COASTLINE_EAR_PLAYBACKSMOOTHING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Select one or two playback smoothing passes without resetting per-playback pass counters.
 * @param reduced Nonzero for one pass; zero for two passes.
 * @ghidraAddress EAR: 0x100049ab
 */
void __stdcall ConfigurePlaybackSmoothingPasses(uint32_t reduced);
/**
 * Configured smoothing pass count, initially two.
 * ConfigurePlaybackSmoothingPasses() sets one or two.
 * @ghidraAddress EAR: 0x100360b8
 */
/** @cond */
extern uint16_t g_wPlaybackSmoothingPassCount;
/**
 * Nonzero when an applied gain/pan smoothing update advances directly beyond the final pass.
 * @ghidraAddress EAR: 0x1003b7b8
 */
extern int g_earPlaybackSmoothingEnabled;
/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_PLAYBACKSMOOTHING_H */
