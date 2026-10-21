/*
 * Host check of src/racemenu.c against the branches and words of the disassembly of 09900.s.
 * Build with -no-pie so that the globals, which the display lists hold as 32-bit words, sit below
 * 4 GB.
 */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "racemenu.c"

/* The libc of the original clashes with stdlib.h, so only this is taken from it. */
char *getenv(const char *name);

/* The state the file reads. */
Gfx *gDisplayListPtr;
static Gfx gfx[1024];
f32 g_afMvpMatrix[4][4], g_afScratchMatrix[4][4];
u32 g_dwTextureFilterMode = 0x2000, g_dwColorDitherMode = 0x40, g_dwAlphaDitherMode = 0x10;
s32 g_dwTrackMirrored, g_dwViewportMirrored, g_bSmoothShading = 1;
u32 g_dwGameMode, g_dwSelectedTrackId, g_dwNumLocalPlayers = 1, g_dwCurrentWeather;
s32 g_dwNumTracks = 10;
Vehicle g_aVehicles[MAX_VEHICLES];
static PlayerProfile profile;
static PlayerInputState inputs[MAX_VEHICLES];
PlayerProfile *g_pActiveProfile = &profile;
s32 g_dwDrawModelListNoZBuffer;
u32 g_adwModelShadeColors[4] = {0x11111100, 0x22222200, 0x33333300, 0x44444400};
s32 g_dwScreenWidth = 320, g_dwScreenHeight = 240;
f32 g_fFrameDeltaTime = 1.0f / 30.0f;
Season g_SeasonTable[SEASONS] = {
    {"First Season", "Spring", 3, 10, {1, 2}, {0, 0}, {{0, 0}, {1, 1}, {2, 2}}},
    {"Second Season", "Summer", 4, 20, {3, 4}, {0, 0}, {{3, 0}, {4, 1}, {0, 2}, {1, 3}}},
};

u8 g_abGhostRecording[16];
Gfx g_aMenuChoiceMarkerDisplayList[1], g_aMenuSpotlightDisplayList[1];
Vtx g_aMenuSpotlightVertices[31];
PlayerInputState g_aPlayerInputState[MAX_PLAYERS];
MenuItem *g_apTrackMenuItems[16], *g_apWeatherMenuItems[8];
Texture g_ArrowLeftTextureDesc, g_ArrowRightTextureDesc, g_BgTextureDesc, *g_pMenuTitleImage;
MixerChannel g_aSoundEffectChannels[SOUND_EFFECT_CHANNELS];
VehicleModelLoad g_aVehicleModelSlots[VEHICLE_MODEL_SLOTS];
ViewportRect g_aViewportRects[4];
s32 g_bMenuSpotlightBuilt, g_bScreenItemChosen, g_bScreenStateInitFlag, g_bSeasonFirstRace;
s32 g_bTrackMenuChampionship, g_bTrackMenuPakScreen, g_dwContinueFromResults, g_dwHudDebugLevel;
s32 g_dwMenuChoosingPlayer, g_dwMenuInputPlayerMask, g_dwMenuTitleImageHeight;
s32 g_dwMenuTitleImageWidth, g_dwMenuTitleImageX, g_dwMenuTitleImageY, g_dwScreenPlayerIdx;
s32 g_dwScreenScaleShift, g_dwTotalLaps, g_dwTrackMenuInitState, g_dwTrackMenuItemCount;
s32 g_dwTrackMenuSelection, g_dwVehicleViewModeIndex;
s32 g_nPrevScreenSelection, g_nScreenCursor, g_nScreenLastActiveIndex, g_nScreenPendingSelection;
u32 g_dwGhostRecordingSize, g_dwKeepScreenBlanked, g_dwNightMode, g_dwRainMode, g_dwRandomSeed;
u32 g_dwScreenScrollSoundSize = 4, g_dwScreenSelectSoundSize = 4, g_dwSnowMode, g_dwWeatherActive;
f32 g_fScreenIdleTime, g_fScreenIdleTimeout, g_fScreenRotationAngle, g_fScreenSlideOffset;
f32 g_fSeasonRecordsBlinkTime;
Ambient g_MenuAmbientLight;
Light g_MenuDirectionalLight;
Mtx g_MenuChoiceMarkerMtx;
MenuPostDrawCallback g_pfnScreenPostDrawCallback;
MenuPreDrawCallback g_pfnScreenPreDrawCallback;
s8 *g_pScreenScrollSoundBuf, *g_pScreenSelectSoundBuf;
s8 g_SilenceSampleData[4];
char g_szTrackMenuTextBuf[128], g_szTrackSelectPointsTextBuf[128],
    g_szTrackSelectRecordTextBuf[128];

/* Stubs for the routines the file calls. */
static int fadingOut, fadeComplete, sounds;
s32 IsTrackIndexValid(s32 track) {
    return track < 5;
}
LookAt *AllocLightBlock(void) {
    static LookAt l[4];
    return &l[0];
}
DisplayListBlock *AllocDisplayListBlock(void) {
    static DisplayListBlock m[64];
    static int i;
    return &m[i++ & 63];
}
s16 FindFreeSoundChannel(void) {
    return 0;
}
void InitSoundChannel(s16 channel, s8 *sample, u32 length, u32 loopLength) {
    sounds++;
}
s32 IsFadingOut(void) {
    return fadingOut;
}
s32 IsFadeComplete(void) {
    return fadeComplete;
}
void TransferFlags(PlayerInputState *input, u32 flags) {
}
void UpdateStickRepeat(PlayerInputState *input) {
}

/** The words of the display list from its start. */
static u32 word(int i) {
    return ((u32 *)gfx)[i];
}

static void CheckTrackUnlocked(void) {
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    g_dwSelectedTrackId = 3;
    g_bSeasonFirstRace = TRUE;
    assert(IsTrackUnlocked(3) && !IsTrackUnlocked(10));
    g_bSeasonFirstRace = FALSE;
    assert(IsTrackUnlocked(3) && IsTrackUnlocked(10) && !IsTrackUnlocked(2));
    g_dwGameMode = GAME_MODE_TIME_ATTACK;
    profile.unlockedTracks = 0x0006;
    assert(IsTrackUnlocked(10) && IsTrackUnlocked(12) && !IsTrackUnlocked(11));
    assert(IsTrackUnlocked(1) && IsTrackUnlocked(2) && !IsTrackUnlocked(0));
    /* An unlocked track must also be on the cartridge. */
    profile.unlockedTracks = 0x0060;
    assert(!IsTrackUnlocked(5) && !IsTrackUnlocked(6));
}

static void CheckValidate(void) {
    g_aVehicles[0].profile = &profile;
    profile.championshipFlags = -1;
    profile.season = 1;
    profile.round = 2;
    profile.seasonPoints[0] = 7;
    profile.seasonPoints[1] = 9;
    ValidateChampionshipProgress();
    assert(profile.championshipFlags == 0 && profile.season == 0 && profile.round == 0);
    assert(profile.seasonPoints[0] == 0 && profile.seasonPoints[1] == 9);
    /* A round past the races of its season starts the season again. */
    profile.season = 0;
    profile.round = 3;
    profile.seasonPoints[0] = 5;
    ValidateChampionshipProgress();
    assert(profile.round == 0 && profile.seasonPoints[0] == 0);
    /* A season out of range starts the championship again. */
    profile.season = 9;
    profile.round = 1;
    ValidateChampionshipProgress();
    assert(profile.season == 0 && profile.round == 0);
    /* Valid progress is kept. */
    profile.season = 1;
    profile.round = 3;
    profile.seasonPoints[1] = 4;
    ValidateChampionshipProgress();
    assert(profile.season == 1 && profile.round == 3 && profile.seasonPoints[1] == 4);
}

static void CheckDrawModelList(void) {
    static Gfx partList[1];
    static union {
        Model model;
        u8 bytes[sizeof(Model) + 2 * sizeof(ModelPart)];
    } u;
    f32 m[4][4];
    int i, n;

    u.model.partCount = 2;
    u.model.parts[0].displayList = partList;
    u.model.parts[0].flags = MODEL_PART_TRANSLUCENT;
    u.model.parts[1].displayList = partList;
    u.model.parts[1].flags = MODEL_PART_RAMPED | MODEL_PART_TWO_SIDED | MODEL_PART_FLAT | 2;
    guMtxIdentF(m);
    gDisplayListPtr = gfx;
    DrawModelList(&u.model, m);
    n = (int)(gDisplayListPtr - gfx) * 2;
    if (getenv("DUMP") != NULL) {
        for (i = 0; i < n; i += 2) {
            fprintf(stderr, "%3d %08X %08X\n", i, word(i), word(i + 1));
        }
    }

    assert(word(0) == 0xE7000000 && word(1) == 0);
    assert(word(2) == 0xBA001402 && word(3) == 0x00100000);
    assert(word(4) == 0xB900031D && word(5) == 0x0C192038);
    assert(word(6) == 0xFC26A004 && word(7) == 0x1FFC93F8);
    assert(word(16) == 0xBA000C02 && word(17) == 0x2000 && word(28) == 0xBA001001);
    assert(word(29) == 0x00010000);
    assert(word(34) == 0xB6000000 && word(35) == 0x00853200);
    assert(word(36) == 0xB7000000 && word(37) == 0x000A2205);
    assert(word(38) == 0x03840010 && word(40) == 0x03820010);
    assert(word(41) == word(39) + 0x10);
    assert(word(42) == 0x01040040 && word(44) == 0xBB000001 && word(45) == 0xFFFFFFFF);
    assert(word(46) == 0xB6000000 && word(47) == 0x000C0000);
    assert(word(50) == 0xF5100000 && word(51) == 0x07000000);
    assert(word(52) == 0xF50001F0 && word(53) == 0x06000000);
    assert(word(54) == 0xF5000100 && word(55) == 0x05000000);
    /* The ramped part comes first, in the opaque pass. */
    i = 56;
    assert(word(i) == 0xBC00000A && word(i + 1) == 0);
    assert(word(i + 2) == 0xBC00040A && word(i + 3) == 0);
    assert(word(i + 4) == 0xBC00200A && word(i + 5) == 0x33333300);
    assert(word(i + 6) == 0xBC00240A && word(i + 7) == 0x33333300);
    assert(word(i + 8) == 0xB6000000 && word(i + 9) == 0x3000);
    assert(word(i + 10) == 0xB6000000 && word(i + 11) == 0x200);
    assert(word(i + 12) == 0xBB000001 && word(i + 14) == 0xE8000000);
    assert(word(i + 16) == 0x01040040 && word(i + 18) == 0x06000000);
    assert(word(i + 19) == (u32)(unsigned long)partList);
    assert(word(i + 20) == 0xBD000000 && word(i + 22) == 0xB7000000 && word(i + 23) == 0x200);
    assert(word(i + 24) == 0xB7000000 && word(i + 25) == 0x2000);
    assert(word(i + 26) == 0xBC00000A && word(i + 27) == 0xFFFFFF00);
    assert(word(i + 30) == 0xBC00200A && word(i + 31) == 0x40404000);
    /* Then the translucent one, with none of the changes. */
    i += 34;
    assert(word(i) == 0xBB000001 && word(i + 2) == 0xE8000000 && word(i + 4) == 0x01040040);
    assert(word(i + 6) == 0x06000000 && word(i + 8) == 0xBD000000);
    assert(word(i + 10) == 0xBD000000 && n == i + 12);

    /* Without the depth buffer, once. */
    g_dwDrawModelListNoZBuffer = TRUE;
    gDisplayListPtr = gfx;
    DrawModelList(&u.model, m);
    assert(word(5) == 0x0C192008 && !g_dwDrawModelListNoZBuffer);
}

static int Even(s32 item) {
    return (item & 1) == 0;
}

static void CheckScreen(void) {
    static MenuItem a = {"A", 0}, b = {"B", 0}, c = {"C", MENU_ITEM_ADJUSTABLE}, d = {"D", 0};
    static MenuItem *items[] = {&a, &b, &c, &d, NULL};
    static s8 scroll[2 * 4 + SOUND_TAIL_BYTES], select[2 * 4 + SOUND_TAIL_BYTES];
    s32 selection = 0, result;

    g_bScreenStateInitFlag = TRUE;
    g_pScreenScrollSoundBuf = scroll;
    g_pScreenSelectSoundBuf = select;
    g_dwScreenPlayerIdx = -1;
    g_aVehicles[0].input = &inputs[0];
    g_pfnScreenPreDrawCallback = NULL;
    g_pfnScreenPostDrawCallback = NULL;
    g_nScreenCursor = 0;
    g_nScreenPendingSelection = -1;
    g_fScreenSlideOffset = 0.0f;

    /* Left from the first item wraps to the last even item, past the odd ones. */
    gDisplayListPtr = gfx;
    inputs[0].buttons = INPUT_LEFT | INPUT_UP;
    sounds = 0;
    result = ProcessScreenState("T", 4, items, &selection, Even, 0, 0, 0, 0, 0, 0);
    assert(result == 0 && g_nScreenCursor == 2 && g_nPrevScreenSelection == 0);
    assert(g_fScreenSlideOffset == 1.0f && sounds == 1 && !(inputs[0].buttons & INPUT_UP));

    /* Up on an adjustable item reports it at once. */
    g_fScreenSlideOffset = 0.0f;
    gDisplayListPtr = gfx;
    inputs[0].buttons = INPUT_UP;
    result = ProcessScreenState("T", 4, items, &selection, Even, 0, 0, 0, 0, 0, 0);
    assert(result == MENU_RESULT_UP && selection == 2);

    /* Right with every other item filtered out stays, and nothing moves. */
    gDisplayListPtr = gfx;
    g_nScreenCursor = 0;
    inputs[0].buttons = INPUT_RIGHT;
    sounds = 0;
    result = ProcessScreenState("T", 1, items, &selection, NULL, 0, 0, 0, 0, 0, 0);
    assert(result == 0 && g_nScreenCursor == 0 && sounds == 0);

    /* A chooses: the screen fades out, then reports the choice. */
    gDisplayListPtr = gfx;
    inputs[0].buttons = INPUT_A;
    g_bScreenItemChosen = FALSE;
    result = ProcessScreenState("T", 4, items, &selection, Even, 0, 0, 0, 0, 0, 0);
    assert(result == 0 && g_bScreenItemChosen && g_dwMenuChoosingPlayer == 0);
    fadingOut = fadeComplete = 1;
    gDisplayListPtr = gfx;
    g_aSoundEffectChannels[3].volume = 9;
    result = ProcessScreenState("T", 4, items, &selection, Even, 0, 0, 0, 0, 0, 0);
    assert(result == MENU_RESULT_CHOSEN && selection == 0 && !g_bScreenStateInitFlag);
    assert(g_aSoundEffectChannels[3].volume == 0 &&
           g_aSoundEffectChannels[5].sample == g_SilenceSampleData);
    fadingOut = fadeComplete = 0;
}

int main(void) {
    CheckTrackUnlocked();
    CheckValidate();
    CheckDrawModelList();
    CheckScreen();
    puts("racemenu_check: ok");
    return 0;
}
