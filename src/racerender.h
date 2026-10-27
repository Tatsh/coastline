#ifndef COASTLINE_RACE_RENDER_H
#define COASTLINE_RACE_RENDER_H

#include "racers.h"
#include "render.h"
#include "vehicle.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
void RenderRaceView(ViewportRect *views, const RacerSlot *slots, Vehicle *vehicles, s32 index);
void DrawPauseMenu(ViewportRect *views, const RacerSlot *slots, Vehicle *vehicles);
#else
void RenderRaceView(s32 index);
void DrawPauseMenu(void);
#endif

void DrawCreditCaption(void);

#endif
