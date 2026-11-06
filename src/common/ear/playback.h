#ifndef COASTLINE_EAR_PLAYBACK_H
#define COASTLINE_EAR_PLAYBACK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Advance the gain of each channel towards its target, and the timers of each event of each
 * channel that is not paused, then retire the events that completed, start those that may start,
 * and update the others. A channel waiting to shut down shuts down once its gain arrives.
 *
 * @ghidraAddress EAR: 0x10004d3b
 */
void __stdcall UpdateEarPlaybackEngine(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_EAR_PLAYBACK_H */
