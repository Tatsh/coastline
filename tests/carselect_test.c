/* Test of src/carselect.c against the branches and words of the disassembly of 0D460.s. */
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "carselect.c"

int strcmp(const char *a, const char *b);

Gfx *gDisplayListPtr;
static Gfx gfx[256];
f32 g_afMvpMatrix[4][4], g_afScratchMatrix[4][4];
f32 g_fCameraFovRadians, g_fCameraProjectionScale = 400.0f, g_fFrameDeltaTime = 1.0f / 30.0f;
CameraFrame *g_pActiveCamera;
Vehicle *g_pActiveCameraVehicle;
Vehicle g_aVehicles[MAX_VEHICLES];
VehicleModel g_aVehicleModels[2];
VehicleModelLoad g_aVehicleModelSlots[4];
CarModel g_aCarModels[CAR_MODELS];
s32 g_dwCarCount = 5, g_dwScreenWidth = 320, g_dwScreenHeight = 240, g_dwHudDebugLevel;
s32 g_dwVehicleViewModeIndex, g_dwNumActiveVehicles, g_dwContinueFromResults, g_dwTotalLaps = 3;
s32 g_dwIsPaintMode, g_dwScreenPlayerIdx, g_dwSaveAvailable, g_dwSeasonResultMsgLineCount;
s32 g_dwElapsedTimeMs;
u32 g_dwRandomSeed;
u32 g_dwNumLocalPlayers = 1, g_dwCurrentWeather, g_dwSelectedTrackId;
s32 g_dwGameMode;
u32 g_dwWeatherActive, g_dwNightMode, g_dwRainMode, g_dwSnowMode;
u8 g_bSaveLoadEntryPoint, g_abRacePositionPoints[8] = {9, 6, 5, 3, 2, 1};
PlayerProfile *g_pActiveProfile;
static PlayerProfile profile;
static PlayerInputState inputs[2];
ViewportRect g_aViewportRects[4];
Texture g_BgTextureDesc, g_ArrowLeftTextureDesc, g_ArrowRightTextureDesc, g_TitleBarTextureDesc;
MixerChannel g_aSoundEffectChannels[SOUND_EFFECT_CHANNELS];
s8 g_SilenceSampleData[4];
const char *g_apszPlaceNames[8], *g_apszSeasonResultMsgLines[8], *g_apszEngineLayouts[4];
const char *g_apszDriveTypes[4];
MenuItem *g_apTrackMenuItems[16], *g_apWeatherMenuItems[8];
Season g_SeasonTable[SEASONS];
Ambient g_CameraViewAmbientLight;
Light g_CameraViewLight1, g_CameraViewLight2, g_CameraViewLight3, g_CameraViewLight4;

s32 g_dwIsReviewMode, g_bCarSelectInitialized, g_dwCarSelectStartTime, g_adwCarSelectModelSlots[2];
f32 g_afCarSelectSlide[2], g_afCarSelectAngle[2];
MenuItem **g_aapCarSelectOptions[2];
s32 g_anCarSelectCursor[2], g_anCarSelectPrevCursor[2], g_anCarSelectOptionCount[2];
const char *g_apszCarSelectCategory[2];
s32 g_bCarSelectDone, g_dwTitleRedrawFrames, g_adwCarSelectState[2];
s8 *g_pScrollSoundBuf, *g_pSelectSoundBuf;
u32 g_dwScrollSoundSize, g_dwSelectSoundSize;
s32 g_dwPakDialogActive, g_dwPakDialogPlayerIdx, g_dwPakDialogResult;
u8 g_bPakDialogDone;
s32 g_dwResultsSeason, g_dwResultsRound, g_dwResultsChampionshipFlags, g_dwResultsTrack;
s32 g_dwCarSelectWeather, g_bSeasonResultsShown, g_dwResultsPage;
char g_szResultsBuf[0x58];
static MenuItem items[5] = {{"A"}, {"B"}, {"C"}, {"D"}, {"E"}};
MenuItem *g_apHandlingOptions[] = {&items[0], &items[1], NULL};
MenuItem *g_apTransmissionOptions[] = {&items[0], &items[1], &items[2], NULL};
MenuItem *g_apTiresOptions[] = {&items[0], &items[1], &items[2], NULL};
MenuItem *g_apSuspensionOptions[] = {&items[0], &items[1], NULL};
MenuItem *g_apDecalOptions[] = {&items[0], &items[1], &items[2], NULL};
s32 g_dwHandlingOptionCount = 2, g_dwTransmissionOptionCount = 3, g_dwTiresOptionCount = 3;
s32 g_dwSuspensionOptionCount = 2, g_dwDecalListCount = 3;

static int fadingOut, fadeComplete, seasonResults;
static void (*next)(void);
s32 IsFadingOut(void) {
    return fadingOut;
}
s32 IsFadeComplete(void) {
    return fadeComplete;
}
void SetCurrentScreenCallback(void (*callback)(void)) {
    next = callback;
}
void ProcessSeasonResults(void) {
    seasonResults++;
}
static int lastLoadSlot = -1, lastLoadCar = -1;
void LoadVehicleModel(VehicleModelLoad *load, s32 slot, s32 car, s32 player) {
    assert(load == &g_aVehicleModelSlots[slot]);
    lastLoadSlot = slot;
    lastLoadCar = car;
}

static u32 Word(int i) {
    return ((u32 *)gfx)[i];
}

static void Frame(u32 buttons) {
    inputs[0].buttons = buttons;
    gDisplayListPtr = gfx;
    ProcessCarSelectScreen();
}

static void CheckSmall(void) {
    char text[32];

    FormatLapTime(text, 83.456f);
    assert(strcmp(text, "1'23\"45") == 0);
    g_pActiveProfile = &profile;
    profile.unlocks = 0x0FFF;
    g_dwIsPaintMode = TRUE;
    assert(IsCarUnlocked(8) && !IsCarUnlocked(9));
    g_dwIsPaintMode = FALSE;
    assert(IsCarUnlocked(9) && !IsCarUnlocked(12));

    gDisplayListPtr = gfx;
    DrawProgressBar(10, 20, 100, 12, 0.5f);
    assert(Word(0) == 0xE7000000 && Word(2) == 0xBA001402 && Word(3) == 0x00300000);
    assert(Word(4) == 0xB900031D && Word(5) == 0 && Word(6) == 0xFCFFFFFF && Word(7) == 0xFFFDF6FB);
    assert(Word(8) == 0xF7000000 && Word(9) == 1 && Word(10) == 0xF61B8080 &&
           Word(11) == 0x00028050);
    assert(Word(14) == 0xF7000000 && Word(15) == 0x1C1 && Word(16) == 0xF61AC074);
    assert(Word(17) == 0x0003405C && Word(21) == 0x781 && Word(22) == 0xF60F0074);
    assert(Word(23) == 0x0003405C && gDisplayListPtr - gfx == 12);
}

static void CheckChoosing(void) {
    Vehicle *v = &g_aVehicles[0];

    g_bCarSelectInitialized = TRUE;
    g_dwNumLocalPlayers = 1;
    g_dwIsReviewMode = FALSE;
    v->input = &inputs[0];
    v->profile = &profile;
    profile.handling = 1;
    profile.transmission = 0;
    profile.tires = 2;
    profile.suspension = 1;
    v->selectedCar = 2;
    g_adwCarSelectModelSlots[0] = 0;
    g_adwCarSelectState[0] = CAR_SELECT_CAR;

    /* Left turns to the previous car, into the other slot. */
    Frame(INPUT_LEFT);
    assert(v->selectedCar == 1 && g_afCarSelectSlide[0] > 0.9f && g_adwCarSelectModelSlots[0] == 2);
    /* Nothing moves while the cars slide. */
    Frame(INPUT_RIGHT);
    assert(v->selectedCar == 1);
    g_afCarSelectSlide[0] = 0.0f;
    Frame(INPUT_RIGHT);
    assert(v->selectedCar == 2 && g_afCarSelectSlide[0] < -0.9f);
    g_afCarSelectSlide[0] = 0.0f;

    /* Choosing walks through the options, each from the profile. */
    Frame(INPUT_A);
    assert(v->carModel == 2 && g_adwCarSelectState[0] == CAR_SELECT_HANDLING);
    assert(g_anCarSelectCursor[0] == 1 && g_anCarSelectPrevCursor[0] == 1);
    assert(g_aapCarSelectOptions[0] == g_apHandlingOptions && g_anCarSelectOptionCount[0] == 2);
    assert(strcmp(g_apszCarSelectCategory[0], "Handling") == 0);
    Frame(INPUT_RIGHT);
    assert(g_anCarSelectCursor[0] == 0 && g_anCarSelectPrevCursor[0] == 1);
    g_afCarSelectSlide[0] = 0.0f;
    Frame(INPUT_A);
    assert(profile.handling == 0 && g_adwCarSelectState[0] == CAR_SELECT_TRANSMISSION);
    assert(g_anCarSelectOptionCount[0] == 3);
    Frame(INPUT_B);
    assert(g_adwCarSelectState[0] == CAR_SELECT_HANDLING && g_anCarSelectCursor[0] == 0);
    Frame(INPUT_A);
    Frame(INPUT_A);
    assert(g_adwCarSelectState[0] == CAR_SELECT_TIRES && g_anCarSelectCursor[0] == 2);
    Frame(INPUT_A);
    Frame(INPUT_A);
    assert(g_adwCarSelectState[0] == CAR_SELECT_DECALS && g_anCarSelectOptionCount[0] == 2);
    assert(g_aapCarSelectOptions[0] == g_apDecalOptions && g_adwCarSelectModelSlots[0] == 2);
    /* The custom decals, then everyone is ready and the screen fades out. */
    g_anCarSelectCursor[0] = DECALS_CUSTOM;
    Frame(INPUT_A);
    assert(v->customDecals && g_adwCarSelectState[0] == CAR_SELECT_READY && g_bCarSelectDone);
    fadingOut = fadeComplete = 1;
    Frame(0);
    assert(next == ProcessRacingScreen && !g_bCarSelectInitialized);
    fadingOut = fadeComplete = 0;
}

static void CheckResults(void) {
    g_bCarSelectInitialized = TRUE;
    g_dwIsReviewMode = TRUE;
    g_dwNumLocalPlayers = 1;
    g_dwGameMode = GAME_MODE_ARCADE;
    g_dwResultsPage = RESULTS_PAGE_PLAYER_1;
    g_adwCarSelectState[0] = CAR_SELECT_HANDLING;
    g_aapCarSelectOptions[0] = g_apHandlingOptions;
    g_anCarSelectCursor[0] = 0;
    g_anCarSelectPrevCursor[0] = 0;
    g_afCarSelectSlide[0] = 0.0f;
    g_apTrackMenuItems[0] = &items[0];
    g_apWeatherMenuItems[0] = &items[1];
    g_apszPlaceNames[0] = "1st";
    g_aVehicles[0].lap = 3;
    /* Past the last page of an arcade race, both are ready and the page stays. */
    Frame(INPUT_A);
    assert(g_dwResultsPage == RESULTS_PAGE_PLAYER_1 && g_adwCarSelectState[0] == CAR_SELECT_READY);
    assert(g_adwCarSelectState[1] == CAR_SELECT_READY && g_bCarSelectDone);

    /* A championship turns to the season, then its message, which may be empty. */
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    g_adwCarSelectState[0] = CAR_SELECT_HANDLING;
    g_dwResultsPage = RESULTS_PAGE_PLAYER_1;
    g_SeasonTable[0].raceCount = 2;
    Frame(INPUT_A);
    assert(g_dwResultsPage == RESULTS_PAGE_SEASON && g_adwCarSelectState[0] == CAR_SELECT_HANDLING);
    Frame(INPUT_A);
    assert(seasonResults == 1 && g_bSeasonResultsShown && g_dwResultsPage == RESULTS_PAGE_SEASON);
    assert(g_adwCarSelectState[0] == CAR_SELECT_READY);

    /* Leaving the results of a championship race in the same season saves. */
    g_pActiveProfile->season = 0;
    g_pActiveProfile->round = 1;
    g_dwResultsSeason = 0;
    fadingOut = fadeComplete = 1;
    Frame(0);
    assert(next == ProcessDataSaveLoadMenu && g_bSaveLoadEntryPoint == 1 &&
           g_dwContinueFromResults);
    fadingOut = fadeComplete = 0;
    g_dwIsReviewMode = FALSE;
}

int main(void) {
    CheckSmall();
    CheckChoosing();
    CheckResults();
    puts("carselect_check: ok");
    return 0;
}
