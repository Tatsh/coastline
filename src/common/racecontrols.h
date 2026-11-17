#ifndef COASTLINE_RACE_CONTROLS_H
#define COASTLINE_RACE_CONTROLS_H

/**
 * @file
 * The buttons of the players during a race and in its pause menu.
 */

#include "common/types.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * Process the pause menu and return its requested pause state.
 *
 * @param paused The pause state of the race, from g_dwRacePauseState.
 * @return The requested pause state, 0 to resume the race. The PC release returns 2 when the menu
 * moves to the confirmation of the exit.
 */
s32 ProcessPauseMenu(s32 paused);
/**
 * Process race input and return its requested pause state.
 *
 * @param paused The pause state of the race, from g_dwRacePauseState.
 * @return The requested pause state, non-zero when a player paused the race.
 */
s32 ProcessRaceButtons(s32 paused);

#ifdef __cplusplus
}
#endif
#endif
