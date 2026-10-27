#include "playerinput.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
PlayerInputState g_aPlayerInputState[MAX_PLAYERS];
OSContPad g_aContPadData[MAX_PLAYERS];
#endif

void InitPlayerInputState(PlayerInputState *state) {
    s32 player = state - g_aPlayerInputState;

    state->recordBuffers[1] = NULL;
    state->recordBuffers[0] = NULL;
    state->playbackBuffer = NULL;
    state->playerIndex = player;
    state->pad = &g_aContPadData[player];
}
