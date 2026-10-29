// The race screen of the Nintendo 64 front end in the SDL game. The front end chooses a race as
// the console does, and the game code of the PC release runs it.

#include "common/frontendscreens.h"
#include "common/race.h"
#include "common/screen.h"
#include "sdl/pcgamerun.h"

void ProcessRacingScreen(void) {
    coastline::RunPcRaceFromFrontEnd();
    // ponytail: every race returns to the main menu; the results, the season, and the attract
    // loop of the console follow once the PC race reports how it ended.
    SetCurrentScreenCallback(ProcessMainMenu);
}
