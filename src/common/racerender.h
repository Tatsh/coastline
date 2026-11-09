#ifndef COASTLINE_RACE_RENDER_H
#define COASTLINE_RACE_RENDER_H

#include "common/racers.h"
#include "common/render.h"
#include "common/vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_BOSS_RALLY
/** Render the race view of player @p index. */
void RenderRaceView(ViewportRect *views, const RacerSlot *slots, Vehicle *vehicles, s32 index);
/** Draw the pause menu over the active race views. */
void DrawPauseMenu(ViewportRect *views, const RacerSlot *slots, Vehicle *vehicles);
#else
/** Render the race view of player @p index. */
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
