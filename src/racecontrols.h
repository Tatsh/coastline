#ifndef COASTLINE_RACE_CONTROLS_H
#define COASTLINE_RACE_CONTROLS_H
#include "types.h"
/** Process the pause menu and return its requested pause state. */
s32 ProcessPauseMenu(s32 paused);
/** Process race input and return its requested pause state. */
s32 ProcessRaceButtons(s32 paused);
#endif
