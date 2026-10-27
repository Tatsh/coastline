#ifndef COASTLINE_RACE_CONTROLS_H
#define COASTLINE_RACE_CONTROLS_H
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
/** Process the pause menu and return its requested pause state. */
s32 ProcessPauseMenu(s32 paused);
/** Process race input and return its requested pause state. */
s32 ProcessRaceButtons(s32 paused);

#ifdef __cplusplus
}
#endif
#endif
