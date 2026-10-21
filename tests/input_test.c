/* Host check of src/input.c. */
#include <assert.h>
#include <stdio.h>

#include "input.h"
#include "race.h"
#include "system.h"
#include "vehicle.h"

PlayerInputState g_aPlayerInputState[MAX_PLAYERS];
OSContPad g_aContPadData[MAX_PLAYERS];
Vehicle g_aVehicles[MAX_VEHICLES];
s32 g_bShowRamOnScreen, g_dwMultiplayerMode, g_bBilinearFiltering, g_dwFrameDeltaMs;
u32 g_dwRacePauseState;
static s32 applied, inRace = 1;
static void Apply(void) {
    applied++;
}
/* Pressing A, then B: the newest change comes first. */
static u16 sSequence[] = {INPUT_B, 0, INPUT_A, 0xFFFF};
CheatCode g_aCheatCodes[] = {{Apply, sSequence}, {NULL, NULL}};
void ProcessRacingScreen(void) {
}
s32 IsCurrentScreen(ScreenCallback callback) {
    return inRace;
}

static void Press(PlayerInputState *s, u16 button) {
    s->pad->button = button;
    TranslateControllerInput(s);
}

int main(void) {
    PlayerInputState *s = &g_aPlayerInputState[1];
    u8 recording[8] = {0x20, 0x01, 0xE0, 0x04};

    InitPlayerInputState(s);
    assert(s->pad == &g_aContPadData[1] && s->playerIndex == 1);
    s->history[0] = 0xFFFF;

    /* Layout 0: A accelerates, A and B is the e-brake, Z gears down. */
    s->controllerLayout = 0;
    Press(s, A_BUTTON | B_BUTTON | Z_TRIG);
    assert(s->buttons ==
           (INPUT_A | INPUT_B | INPUT_Z | INPUT_ACCELERATE | INPUT_EBRAKE | INPUT_GEAR_DOWN));
    /* Layout 2: the directional pad steers. */
    s->controllerLayout = 2;
    Press(s, L_JPAD);
    assert(s->steeringInput == -0x50 && s->steering < -1.0f + 0.9f);
    /* Layout 3: the stick accelerates. */
    s->controllerLayout = 3;
    s->pad->stick_y = 0x30;
    Press(s, 0);
    assert(s->buttons == INPUT_ACCELERATE);
    s->pad->stick_y = 0;

    /* The cheat: A, release, B, release. */
    s->controllerLayout = 5;
    applied = 0;
    Press(s, A_BUTTON);
    Press(s, 0);
    Press(s, B_BUTTON);
    assert(applied == 1);
    Press(s, B_BUTTON);
    assert(applied == 1);

    /* Playback replaces the steering and the actions until it runs out. */
    s->playbackBuffer = recording;
    s->playbackPosition = 0;
    s->playbackLimit = 4;
    Press(s, 0);
    assert(s->steeringInput == 0x20 && s->buttons == INPUT_ACCELERATE);
    Press(s, 0);
    assert(s->steeringInput == -0x20 && s->buttons == INPUT_BRAKE);
    Press(s, 0);
    assert(s->steeringInput == 0 && s->buttons == 0);

    /* The stick presses up at once, again after 500 ms, then every 200 ms. */
    s->buttons = 0;
    s->stickY = 1.0f;
    s->repeatUp = 0;
    g_dwFrameDeltaMs = 100;
    UpdateStickRepeat(s);
    assert(s->buttons == INPUT_UP && s->repeatUp == 500);
    s->buttons = 0;
    UpdateStickRepeat(s);
    UpdateStickRepeat(s);
    UpdateStickRepeat(s);
    UpdateStickRepeat(s);
    assert(s->buttons == 0);
    UpdateStickRepeat(s);
    assert(s->buttons == INPUT_UP && s->repeatUp == -200);
    s->buttons = 0;
    UpdateStickRepeat(s);
    assert(s->buttons == 0);
    UpdateStickRepeat(s);
    assert(s->buttons == INPUT_UP);

    /* A held button acts once. */
    s->buttons = INPUT_START;
    TransferFlags(s, INPUT_START);
    assert(s->buttons == 0 && s->heldButtons == INPUT_START);
    s->buttons = INPUT_START;
    ConsumeInputFlags(s);
    assert(s->buttons == 0 && s->heldButtons == INPUT_START);
    s->buttons = 0;
    ConsumeInputFlags(s);
    assert(s->heldButtons == 0);
    puts("input ok");
    return 0;
}
