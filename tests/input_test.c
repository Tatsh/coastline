/* Host check of src/n64/input.c. */
#include <assert.h>
#include <stdio.h>

#include <PR/os.h>

#include "input.h"
#include "race.h"
#include "system.h"
#include "vehicle.h"

PlayerInputState g_aPlayerInputState[MAX_PLAYERS];
OSContPad g_aContPadData[MAX_PLAYERS];
Vehicle g_aVehicles[MAX_VEHICLES];
s32 g_bShowDepthBuffer, g_dwMultiplayerMode, g_bBilinearFiltering, g_dwFrameDeltaMs;
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

/* The serial interface, the Paks, and the screens the probe of the controllers runs. */
s32 g_dwContReadPending;
u8 g_bPakPort0Initialized, g_bControllerWarningType, g_abSavePakIdKnown[2];
u8 g_aabSavePakIds[2][32];
s16 g_wSerialInterfaceIdle;
OSMesgQueue g_SiMesgQueue;
OSPfs g_aContPfs[MAXCONTROLLERS], g_aPakPfs[MAXCONTROLLERS];
s32 gControllerPakState;
ScreenCallback g_pfnCurrentScreen;
static OSContPad sNextPads[MAX_PLAYERS];
static s32 starts, receives, prints, pakMenuFrames, motorChannel = -1;
static u8 plugged, warnings[4], warningCount;
static s32 pakResults[MAXCONTROLLERS], enumerateResult;
s32 osContStartReadData(OSMesgQueue *mq) {
    assert(mq == &g_SiMesgQueue && g_wSerialInterfaceIdle == 0);
    starts++;
    return 0;
}
s32 osRecvMesg(OSMesgQueue *mq, OSMesg *msg, s32 flag) {
    assert(mq == &g_SiMesgQueue && msg == NULL && flag == OS_MESG_BLOCK && g_dwContReadPending);
    receives++;
    return 0;
}
void osContGetReadData(OSContPad *data) {
    s32 i;

    for (i = 0; i < MAX_PLAYERS; i++) {
        data[i] = sNextPads[i];
    }
}
s32 osPfsIsPlug(OSMesgQueue *queue, u8 *pattern) {
    *pattern = plugged;
    return 0;
}
s32 osPfsInitPak(OSMesgQueue *queue, OSPfs *pfs, int channel) {
    assert(pfs == &g_aPakPfs[channel]);
    pfs->id[0] = 0xA0 + channel;
    pfs->id[31] = 0xB0 + channel;
    return pakResults[channel];
}
s32 osMotorInit(OSMesgQueue *mq, OSPfs *pfs, int channel) {
    assert(pfs == &g_aContPfs[channel]);
    motorChannel = channel;
    return 0;
}
void osSyncPrintf(const char *fmt, ...) {
    prints++;
}
void SetCurrentScreenCallback(ScreenCallback callback) {
    g_pfnCurrentScreen = callback;
}
void ProcessControllerCheck(void) {
    assert(g_wSerialInterfaceIdle == 0);
    warnings[warningCount++] = g_bControllerWarningType;
    g_pfnCurrentScreen = NULL;
}
void ProcessPakMenuScreen(void) {
    if (++pakMenuFrames == 3) {
        g_pfnCurrentScreen = NULL;
    }
}
s32 InitPakAndEnumerateFiles(void) {
    return enumerateResult;
}

static void TestControllerReads(void) {
    s32 i;

    for (i = 0; i < MAX_PLAYERS; i++) {
        InitPlayerInputState(&g_aPlayerInputState[i]);
        g_aPlayerInputState[i].heldButtons = 0;
    }
    /* A second start while a read is pending does not start another. */
    StartControllerRead();
    StartControllerRead();
    assert(starts == 1 && g_dwContReadPending && g_wSerialInterfaceIdle == 0);
    ReadControllers();
    assert(starts == 1 && receives == 1 && !g_dwContReadPending && g_wSerialInterfaceIdle == 1);
    ReadControllers();
    assert(starts == 2 && receives == 2);

    /* Each of the four players is translated from its pad, and a held button is dropped. */
    sNextPads[0].button = A_BUTTON;
    sNextPads[3].button = Z_TRIG;
    sNextPads[2].errno = CONT_NO_RESPONSE_ERROR;
    g_aPlayerInputState[3].heldButtons = INPUT_Z;
    UpdateControllerState();
    assert(starts == 3 && receives == 3);
    assert(g_aPlayerInputState[0].buttons & INPUT_A);
    assert(g_aPlayerInputState[2].controllerMissing && !g_aPlayerInputState[0].controllerMissing);
    assert(!(g_aPlayerInputState[3].buttons & INPUT_Z));
}

static void TestControllerPaks(void) {
    /* A missing first controller is warned of first; both Paks mount, the second needs ids. */
    sNextPads[0].errno = CONT_NO_RESPONSE_ERROR;
    sNextPads[2].errno = 0;
    plugged = 3;
    pakResults[1] = PFS_ERR_ID_FATAL;
    enumerateResult = 1;
    InitControllersAndPaks();
    assert(warningCount == 1 && warnings[0] == 0);
    assert(g_bPakPort0Initialized && prints == 1 && gControllerPakState == PFS_ERR_ID_FATAL);
    assert(g_abSavePakIdKnown[0] && g_aabSavePakIds[0][0] == 0xA0 &&
           g_aabSavePakIds[0][31] == 0xB0);
    assert(!g_abSavePakIdKnown[1] && g_aabSavePakIds[1][0] == 0 && motorChannel == 1);
    assert(pakMenuFrames == 0 && g_wSerialInterfaceIdle == 1);

    /* The second Pak mounts; the first is not mounted again and its failure is warned of. */
    sNextPads[0].errno = 0;
    pakResults[1] = 0;
    gControllerPakState = PFS_ERR_ID_FATAL;
    plugged = 3;
    warningCount = 0;
    motorChannel = -1;
    InitControllersAndPaks();
    assert(prints == 1 && warningCount == 1 && warnings[0] == 1 && motorChannel == 0);
    assert(g_abSavePakIdKnown[1] && g_aabSavePakIds[1][0] == 0xA1 &&
           g_aabSavePakIds[1][31] == 0xB1);

    /* No Pak, start held for the Pak menu, then the data of another game. */
    sNextPads[0].button = START_BUTTON;
    plugged = 0;
    enumerateResult = 0;
    warningCount = 0;
    motorChannel = -1;
    InitControllersAndPaks();
    assert(warningCount == 2 && warnings[0] == 1 && warnings[1] == 2 && motorChannel == -1);
    assert(pakMenuFrames == 3 && !(g_aPlayerInputState[0].buttons & INPUT_START) &&
           g_aPlayerInputState[0].heldButtons == INPUT_START);
    assert(g_wSerialInterfaceIdle == 1);
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

    TestControllerReads();
    TestControllerPaks();
    puts("input ok");
    return 0;
}
