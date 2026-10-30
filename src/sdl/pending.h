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

/** Clear both states of the mouse. */
void ResetMouseState(void);

/** Set the volumes of the music and the sound effects from the levels of g_GlobalRallyConfig. */
void ApplyAudioVolumeSettings(void);

/** Record the duration of the preceding game loop. */
void UpdateRaceFrameStatistics(void);

/** Render interpolated snapshots until the next game frame is due. */
void PaceRaceFrames(void);

/**
 * Set the multipliers of the tint of the paint.
 *
 * @param nRed The multiplier of the red.
 * @param nGreen The multiplier of the green.
 * @param nBlue The multiplier of the blue.
 */
void SetColorTintMultipliers(int nRed, int nGreen, int nBlue);

/**
 * Raise the level of damage of a damage texture or of one of its two neighbours.
 *
 * @param nTexture The damage texture.
 * @param pnEntries The entry of each damage texture.
 * @param pnLevels The level of damage of each damage texture.
 */
void ApplyRandomAdjacentDamageTexture(int nTexture, const int *pnEntries, int *pnLevels);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SDL_PENDING_H */
