#ifndef COASTLINE_SDL_PENDING_H
#define COASTLINE_SDL_PENDING_H

/**
 * @file
 * The routines of the Windows libraries the shared race engine calls, and the SDL game does not
 * implement yet. Each records its first call in the log and does nothing.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Record the duration of the preceding game loop. */
void UpdateRaceFrameStatistics(void);

/** Render interpolated snapshots until the next game frame is due. */
void PaceRaceFrames(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SDL_PENDING_H */
