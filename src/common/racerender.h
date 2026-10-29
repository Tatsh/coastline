#ifndef COASTLINE_RACE_RENDER_H
#define COASTLINE_RACE_RENDER_H

#include "common/racers.h"
#include "common/render.h"
#include "common/vehicle.h"

#ifdef __cplusplus
extern "C" {
#endif

#if TGR_PC
void RenderRaceView(ViewportRect *views, const RacerSlot *slots, Vehicle *vehicles, s32 index);
void DrawPauseMenu(ViewportRect *views, const RacerSlot *slots, Vehicle *vehicles);
#else
void RenderRaceView(s32 index);
void DrawPauseMenu(void);
#endif

void DrawCreditCaption(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACE_RENDER_H */
