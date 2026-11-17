#ifndef COASTLINE_RACE_RENDER_H
#define COASTLINE_RACE_RENDER_H

/**
 * @file
 * The drawing of the race view of each player, the pause menu, and the captions of the credits.
 */

#include "common/racers.h"
#include "common/render.h"
#include "common/vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_BOSS_RALLY
/**
 * Render the race view of player @p index.
 *
 * @param views The viewports of the race views, indexed by player.
 * @param slots The racers of the frame drawn.
 * @param vehicles The vehicles of the frame drawn.
 * @param index The player whose view is drawn.
 */
void RenderRaceView(ViewportRect *views, const RacerSlot *slots, Vehicle *vehicles, s32 index);
/**
 * Draw the pause menu over the active race views.
 *
 * @param views The viewports of the race views, indexed by player.
 * @param slots The racers of the frame drawn.
 * @param vehicles The vehicles of the frame drawn.
 */
void DrawPauseMenu(ViewportRect *views, const RacerSlot *slots, Vehicle *vehicles);
#else
/**
 * Render the race view of player @p index.
 *
 * @param index The player whose view is drawn.
 */
void RenderRaceView(s32 index);
/** Draw the pause menu over the active race view. */
void DrawPauseMenu(void);
#endif

/** Draw the current caption over the credits race. */
void DrawCreditCaption(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACE_RENDER_H */
