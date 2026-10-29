/* Test of src/n64/race.c against the branches of the disassembly of 01320.s. */
#include <assert.h>
#include <stdio.h>

#include "common/racephases.c"
#include "common/racerun.c"
#include "race.c"

int strcmp(const char *a, const char *b);

/* The globals race.c reads. */
u32 g_dwRacePauseState;
s32 g_dwPauseMenuSelection;
u32 g_adwTrackMusicRom[12];
Model *g_pStartLightsModel;
s32 g_dwStartLightsAnimation, g_bStartLightsVisible;
f32 g_fStartLightsDrop;
s32 g_dwAirplaneInstance, g_dwAirplaneTrigger, g_dwAirplaneTriggerLap, g_bAirplaneFlying;
s32 g_dwAirplaneNode, g_dwAirplaneNodeCount;
f32 g_fAirplaneScale, g_fAirplaneDistance, g_fAirplaneSegmentLength;
Vec3f *g_pAirplanePathLeft, *g_pAirplanePathRight;
Vec3f g_vAirplaneDirPrev, g_vAirplaneDir, g_vAirplaneDirNext;
RaceCaption g_aCreditCaptions[4];
CreditLine g_aCreditScrollLines[4] = {{0, 40, 16, "A"}, {0, 20, 0, "B"}, {0, 15, 0, NULL}};
s32 g_bReplayAfterRace, g_dwSavedResultsValid;
f32 g_afStartLightThresholds[START_LIGHT_STEPS] = {2.15f, 1.5f, 0.85f, 0.2f, 0.0f};
s32 g_dwSaveAvailable;
u8 g_abRacePositionPoints[20] = {9, 6, 5, 3, 2, 1};
s32 g_dwSeasonResultMsgLineCount, g_dwWaterfallCount, g_dwRaceInitialized, g_dwRaceExitState;
RacePhaseStep g_aRacePhaseTable[RACE_PHASES] = {
    {0, 0.0f}, {1, 2.2f}, {2, 2.3f}, {3, 2.0f}, {4, 0.0f}, {5, 2.0f}, {6, 2.0f}, {7, 0.0f}};
s32 g_dwRaceFinishOrderCounter, g_dwTotalLaps = 3;
f32 g_fRacePhaseTimer;
s32 g_dwStartLightsStep, g_dwRacePhaseIndex;
s16 g_wRaceScreenLoopExited;
s32 g_adwSavedResultPositions[2], g_adwSavedResultBestLaps[2], g_adwSavedResultLaps[2];
f32 g_afSavedResultTimes[2], g_afSavedResultBestLapTimes[2], g_aafSavedResultLapTimes[2][5];
const char *g_apszSeasonResultMsgLines[10];
char g_szFormatBuffer[0x20];
s32 g_adwWaterfallInstances[16], g_dwScreenClearFrames;
f32 g_fRaceCaptionTimer;
s32 g_dwRaceCaptionIndex, g_bDemoInterrupted, g_dwPrevViewportCount, g_dwRacePhase;
const char *g_apszPlaceNames[1];
s32 g_dwGameMode;
s32 g_dwRaceSessionType, g_dwMultiplayerMode;
u32 g_dwNumLocalPlayers = 1, g_dwSelectedTrackId, g_dwCurrentWeather;
u32 g_dwNightMode, g_dwSnowMode, g_dwRainMode, g_dwKeepScreenBlanked;
s32 g_dwNumVehicles, g_dwNumActiveVehicles, g_dwNumViewports;
s32 g_bTrackCollisionFrameToggle, gInstantReplayActive, g_dwReplayCameraSwitchTimer;
s32 g_dwTrackMirrored, g_dwCurrentSplitScreenIndex, g_dwVehicleViewModeIndex;
s32 g_dwSkyboxDisabled, g_dwHudDebugLevel, g_bShowHudHelpText, g_dwRearViewMirrorMode;
s32 g_bRaceScreenActive, g_dwScreenWidth = 320, g_dwScreenHeight = 240;
s32 g_dwCameraRayHitCount, g_dwCameraRayNearbyCount, g_dwCameraRayGroundSurface;
f32 g_fCameraRayGroundDistance, g_fCameraProjectionScale = 1.0f, g_fFrameDeltaTime = 1.0f / 30.0f;
u16 g_awCameraRayHitInstances[8], g_awCameraRayNearbyInstances[8];
s32 g_dwBestReplayLength, g_dwBestReplayPlayer;
u8 *g_apReplayGhostPtrs[4];
s32 g_adwReplayGhostSizes[4];
u8 g_abGhostRecording[0x100];
u32 g_dwGhostRecordingSize;
u8 g_abPlaybackRecording[0x100], g_aabInputRecordings[2][0xF000];
u8 g_abMusicModuleWorkBuffer[4], g_aDmaWorkBuffers[4];
s8 g_SilenceSampleData[4];
MixerChannel g_aSoundEffectChannels[SOUND_EFFECT_CHANNELS];
s32 g_dwSoundEffectEchoOffset;
VehicleModel *g_aVehicleModelsStorage;
VehicleModel g_aVehicleModels[1];
LastLapBuffer g_aLastLapBuffers[4];
MenuItem *g_apTrackMenuItems[10];
PlayerInputState g_aPlayerInputState[MAX_PLAYERS];
PlayerProfile *g_pActiveProfile;
OptionSettings g_OptionSettings;
u8 g_bSaveLoadEntryPoint;
Gfx *gDisplayListPtr;
Mtx g_mtxIdentity;
Mtx *g_pProjectionMtx;
f32 g_afMvpMatrix[4][4];
TrackHeader g_LoadedTrack;
Season g_SeasonTable[SEASONS];
RacerSlot g_aRacerSlots[20];
Vehicle g_aVehicles[MAX_VEHICLES];
ViewportRect g_aViewportRects[4];
CameraFrame *g_pActiveCamera;
Vehicle *g_pActiveCameraVehicle;

static PlayerProfile profiles[2];
static PlayerInputState inputs[MAX_VEHICLES];
static ScreenCallback screen;
static s32 fadingOut, fadeComplete, beeps, goes;

void SetCurrentScreenCallback(ScreenCallback callback) {
    screen = callback;
}
s32 IsFadingOut(void) {
    return fadingOut;
}
s32 IsFadeComplete(void) {
    return fadeComplete;
}
u32 DmaReadAssetSize(u32 romOffset) {
    return 0x40;
}
Model *DmaReadAndFixupTrackModel(void *dst, u32 romStart, u32 romEnd) {
    return dst;
}
s32 CastRayAgainstTrack(Vec3f *outPoint,
                        Vec3f *outNormal,
                        Vec3f *origin,
                        u16 *ground,
                        s32 *count,
                        u16 *nearby,
                        s32 *nearbyCount,
                        f32 *distance,
                        s32 *surface) {
    return 0;
}
void TransferFlags(PlayerInputState *state, u32 mask) {
    state->buttons &= ~mask;
}
void PlayBeepSound(void) {
    beeps++;
}
void PlayGoSound(void) {
    goes++;
}

static void Reset(void) {
    s32 i;

    for (i = 0; i < MAX_VEHICLES; i++) {
        g_aVehicles[i].input = &inputs[i];
        g_aVehicles[i].profile = &profiles[i & 1];
        g_aVehicles[i].index = i;
        g_aRacerSlots[i].vehicle = &g_aVehicles[i];
    }
    g_pActiveProfile = &profiles[0];
}

static void TestSeasons(void) {
    s32 r;

    Reset();
    g_SeasonTable[0].races[1][0] = TRACK_COASTLINE;
    g_SeasonTable[0].races[1][1] = 2;
    profiles[0].season = 0;
    profiles[0].round = 1;
    profiles[0].championshipFlags = 1;
    SetTrackFromSeason();
    assert(g_dwSelectedTrackId == TRACK_MIRROR_COASTLINE && g_dwCurrentWeather == 2);

    /* The last race of the last season, all won, in the mirrored championship. */
    for (r = 0; r < SEASONS; r++) {
        g_SeasonTable[r].raceCount = 4;
        g_SeasonTable[r].pointsNeeded = 20;
    }
    g_SeasonTable[SEASONS - 1].unlocks = 0x40;
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    g_dwSavedResultsValid = TRUE;
    profiles[0].season = SEASONS - 1;
    profiles[0].round = 3;
    profiles[0].seasonPoints[SEASONS - 1] = 36;
    profiles[0].championshipFlags = 1;
    profiles[0].unlocks = 0;
    profiles[0].unlockedTracks = 0;
    ProcessSeasonResults();
    assert(g_dwSeasonResultMsgLineCount == 6);
    assert(strcmp(g_apszSeasonResultMsgLines[0], "ALL SEASONS COMPLETED!") == 0);
    assert(strcmp(g_apszSeasonResultMsgLines[1], "") == 0);
    assert(strcmp(g_apszSeasonResultMsgLines[3], "Bonus: Mirror Mine Track Added!") == 0);
    assert(strcmp(g_apszSeasonResultMsgLines[5], "Bonus: Helmet Car Added!") == 0);
    assert(profiles[0].unlocks == (0x40 | 0x200) && profiles[0].season == 0 &&
           profiles[0].round == 0);
    assert(profiles[0].championshipFlags == 2);
    assert(profiles[0].unlockedTracks & (1 << TRACK_MIRROR_STRIP_MINE));

    /* Too few points to pass a season. */
    profiles[0].season = 2;
    profiles[0].round = 3;
    profiles[0].seasonPoints[2] = 5;
    ProcessSeasonResults();
    assert(g_dwSeasonResultMsgLineCount == 2 && profiles[0].season == 2 &&
           profiles[0].seasonPoints[2] == 0);

    /* Mid-season, the next race's track and weather unlock. */
    g_SeasonTable[2].races[1][0] = TRACK_JUNGLE;
    g_SeasonTable[2].races[1][1] = 3;
    profiles[0].round = 0;
    profiles[0].championshipFlags = 0;
    profiles[0].unlockedTracks = 0;
    profiles[0].unlockedWeathers = 0;
    ProcessSeasonResults();
    assert(g_dwSeasonResultMsgLineCount == 0 && profiles[0].round == 1);
    assert(profiles[0].unlockedTracks == 1 << TRACK_JUNGLE &&
           profiles[0].unlockedWeathers == 1 << 3);
}

static void TestResults(void) {
    Reset();
    g_dwTotalLaps = 3;
    g_aVehicles[0].racePosition = 1;
    g_aVehicles[0].totalTime = 99.0f;
    g_aVehicles[0].bestLap = 2;
    g_aVehicles[0].lap = 3;
    g_aVehicles[0].lapTimes[1] = 33.0f;
    SaveRaceResultsToSlots();
    assert(g_dwSavedResultsValid && g_adwSavedResultPositions[0] == 1 &&
           g_aafSavedResultLapTimes[0][1] == 33.0f);
    g_aVehicles[0].racePosition = 0;
    g_aVehicles[0].lapTimes[1] = 0.0f;
    profiles[0].season = 1;
    profiles[0].round = 2;
    profiles[0].seasonPoints[1] = 4;
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    RestoreRaceResultsFromSlots();
    assert(profiles[0].seasonPoints[1] == 10 && profiles[0].seasonPlaces[1][2] == 1);
    assert(profiles[0].seasonTimes[1][2] == 99.0f && g_aVehicles[0].lapTimes[1] == 33.0f);
    assert(g_aVehicles[0].racePosition == 1);
}

static void TestCredits(void) {
    CalculateScrollPositions();
    assert(g_aCreditScrollLines[0].y == 270 && g_aCreditScrollLines[1].y == 240 + 40 + 16 + 15);
    ScrollTextUp();
    assert(g_aCreditScrollLines[0].y == 269);
    StartCreditsRace();
    assert(g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType == RACE_SESSION_CREDIT_CAPTIONS);
    assert(screen == ProcessRacingScreen);
}

static void TestPhases(void) {
    s32 frames;

    Reset();
    g_dwGameMode = GAME_MODE_CHAMPIONSHIP;
    g_dwNumVehicles = 2;
    g_dwNumLocalPlayers = 1;
    g_dwRacePauseState = FALSE;
    g_dwRacePhaseIndex = 0;
    g_dwRacePhase = RACE_PHASE_HOLD;
    g_fRacePhaseTimer = 0.0f;
    StepRacePhases();
    assert(g_dwRacePhase == RACE_PHASE_WAIT && g_dwStartLightsAnimation == -1);
    assert(g_aRacerSlots[0].status & RACER_HELD);
    for (frames = 0; frames < 400 && g_dwRacePhase != RACE_PHASE_RACING; frames++) {
        StepRacePhases();
    }
    assert(g_dwRacePhase == RACE_PHASE_RACING && beeps == 3 && goes == 1);
    assert(!(g_aRacerSlots[0].status & RACER_HELD) && g_fStartLightsDrop > 14.0f);

    /* The racing phase lasts until every local player finished. */
    StepRacePhases();
    assert(g_dwRacePhase == RACE_PHASE_RACING);
    g_aRacerSlots[0].status |= RACER_FINISHED;
    StepRacePhases();
    assert(g_dwRacePhase == RACE_PHASE_FINISHED);
    g_dwSavedResultsValid = FALSE;
    StepRacePhases();
    assert(g_dwRacePhase == RACE_PHASE_AFTER && g_dwSavedResultsValid);
}

static void TestPause(void) {
    Reset();
    g_dwNumLocalPlayers = 1;
    g_dwPauseMenuSelection = PAUSE_CONTINUE;
    inputs[0].buttons = INPUT_UP;
    assert(ProcessPauseMenu(TRUE));
    assert(g_dwPauseMenuSelection == PAUSE_EXIT);
    inputs[0].buttons = INPUT_DOWN;
    ProcessPauseMenu(TRUE);
    assert(g_dwPauseMenuSelection == PAUSE_CONTINUE);
    /* A on Continue resumes and is left for the race. */
    inputs[0].buttons = INPUT_A;
    assert(!ProcessPauseMenu(TRUE) && inputs[0].buttons == INPUT_A);
    g_dwPauseMenuSelection = PAUSE_EXIT;
    g_dwRaceExitState = FALSE;
    inputs[0].buttons = INPUT_A;
    assert(ProcessPauseMenu(TRUE) && g_dwRaceExitState && inputs[0].buttons == 0);
    inputs[0].buttons = INPUT_START;
    g_dwGameMode = GAME_MODE_ARCADE;
    gInstantReplayActive = FALSE;
    assert(ProcessRaceButtons(FALSE) && g_dwPauseMenuSelection == PAUSE_CONTINUE);
}

static void TestTimeAttack(void) {
    Reset();
    g_dwSelectedTrackId = 1;
    g_dwGhostRecordingSize = 0x20;
    g_abGhostRecording[0] = 1;
    inputs[0].recordBuffers[0] = g_aabInputRecordings[0];
    inputs[0].recordPositions[0] = 0x10;
    g_aabInputRecordings[0][0] = 1;
    g_aabInputRecordings[0][9] = 0x5A;
    g_dwSaveAvailable = FALSE;
    KeepTimeAttackRecording(0);
    assert(g_dwGhostRecordingSize == 0x10 && g_abGhostRecording[9] == 0x5A && g_dwSaveAvailable);
    assert(inputs[0].recordBuffers[0] == NULL &&
           strcmp(g_aVehicles[0].message, "NEW RECORD!") == 0);
    /* A slower recording is not kept. */
    inputs[0].recordBuffers[0] = g_aabInputRecordings[0];
    inputs[0].recordPositions[0] = 0x30;
    g_dwSaveAvailable = FALSE;
    KeepTimeAttackRecording(0);
    assert(g_dwGhostRecordingSize == 0x10 && !g_dwSaveAvailable);
}

static void TestFinish(void) {
    Reset();
    g_dwRaceExitState = TRUE;
    g_bReplayAfterRace = FALSE;
    g_dwGameMode = GAME_MODE_AFTER_SEASON;
    profiles[0].season = 0;
    FinishRace();
    assert(g_dwGameMode == GAME_MODE_DEMO && g_dwRaceSessionType == RACE_SESSION_SCROLLING_CREDITS);
    assert(screen == ProcessRacingScreen && !g_dwRaceInitialized && g_wRaceScreenLoopExited);
    /* After the ending credits the season is saved. */
    FinishRace();
    assert(g_dwGameMode == GAME_MODE_CHAMPIONSHIP && screen == ProcessDataSaveLoadMenu);
    assert(g_bSaveLoadEntryPoint == SAVE_LOAD_FROM_SEASON);
    g_dwSavedResultsValid = TRUE;
    g_dwGameMode = GAME_MODE_TIME_ATTACK;
    FinishRace();
    assert(screen == ProcessRaceResultsScreen);
    g_dwGameMode = GAME_MODE_DEMO;
    g_dwRaceSessionType = RACE_SESSION_ATTRACT;
    g_bDemoInterrupted = FALSE;
    FinishRace();
    assert(screen == ProcessSplashScreens);

    /* Leaving for the replay chooses the shorter recording. */
    g_dwGameMode = GAME_MODE_ARCADE;
    g_dwNumLocalPlayers = 2;
    g_bReplayAfterRace = TRUE;
    gInstantReplayActive = FALSE;
    inputs[0].recordPositions[0] = 0x200;
    inputs[1].recordPositions[1] = 0x100;
    inputs[0].recordBuffers[1] = g_aabInputRecordings[1];
    FinishRace();
    assert(gInstantReplayActive && g_dwBestReplayPlayer == 1 &&
           g_apReplayGhostPtrs[0] == g_aabInputRecordings[1]);
    assert(inputs[0].recordBuffers[1] == NULL);
}

static void TestReplayEnd(void) {
    Reset();
    g_dwGameMode = GAME_MODE_ARCADE;
    g_dwNumLocalPlayers = 2;
    g_dwBestReplayPlayer = 1;
    g_aViewportRects[0].vehicleIndex = 1;
    inputs[1].playbackLimit = 0x40;
    inputs[1].playbackPosition = 0x40;
    inputs[0].playbackPosition = 0x40 + 0xF0;
    inputs[0].playbackLimit = 0x400;
    g_dwRaceExitState = FALSE;
    CheckInstantReplayEnd();
    assert(g_aViewportRects[1].vehicleIndex == 1 && g_aViewportRects[0].vehicleIndex == 0);
    assert(!g_dwRaceExitState);
    inputs[0].playbackLimit = inputs[0].playbackPosition;
    CheckInstantReplayEnd();
    assert(g_dwRaceExitState);
}

static void TestStartLights(void) {
    g_fFrameDeltaTime = 0.5f;
    g_dwStartLightsAnimation = -1;
    AnimateStartLights();
    assert(g_fFrameDeltaTime == 0.5f);
}

int main(void) {
    TestSeasons();
    TestResults();
    TestCredits();
    TestPhases();
    TestPause();
    TestTimeAttack();
    TestFinish();
    TestReplayEnd();
    TestStartLights();
    puts("race ok");
    return 0;
}
