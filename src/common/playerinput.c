#include "common/playerinput.h"

#if TGR_BOSS_RALLY
PlayerInputState g_aPlayerInputState[MAX_PLAYERS];
OSContPad g_aContPadData[MAXCONTROLLERS];
#endif

void ConsumeInputFlags(PlayerInputState *state) {
    u32 buttons = state->buttons;

    state->buttons ^= buttons & state->heldButtons;
    state->heldButtons &= buttons;
}

void TransferFlags(PlayerInputState *state, u32 mask) {
    state->heldButtons |= state->buttons & mask;
    state->buttons &= ~mask;
}

void InitPlayerInputState(PlayerInputState *state) {
    s32 player = state - g_aPlayerInputState;

    state->recordBuffers[1] = NULL;
    state->recordBuffers[0] = NULL;
    state->playbackBuffer = NULL;
    state->playerIndex = player;
    /* Windows slots above three receive addresses beyond controller pad storage. */
    state->pad = &g_aContPadData[player];
}
