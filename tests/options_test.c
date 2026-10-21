/* Test of src/options.c against the branches of the disassembly of 12840.s, with a fake Controller
 * Pak. */
#include <assert.h>
#include <stdio.h>

#include "options.c"

int strcmp(const char *a, const char *b);

/* The data of the ROM the routines read. */
OptionSettings g_OptionSettings = {10, 10, 1};
MenuItem g_aOptionsMenuItems[OPTIONS_ITEMS];
MenuItem *g_apOptionsMenuItems[OPTIONS_ITEMS + 1];
s32 g_adwBgmVolumeLevels[VOLUME_LEVELS] = {0, 4, 9, 14, 19, 24, 28, 33, 38, 43, 48};
s32 g_adwSfxVolumeLevels[VOLUME_LEVELS] = {0, 25, 51, 76, 102, 127, 153, 178, 204, 229, 255};
s32 g_dwOptionsMenuState;
u8 g_bSaveLoadEntryPoint, g_bSaveLoadMenuInitialized, g_bSaveLoadRumbleSwapPending;
u8 *g_pPakDataBuffer;
u8 g_abSaveLoadSeasonNoteName[16] = {1}, g_abSaveLoadGhostNoteName[16] = {2};
u8 g_abSaveLoadOptionsNoteName[16] = {3}, g_abSaveLoadNoteExtension[4];
const char *g_apszSaveLoadChoices[SAVE_LOAD_CHOICES] = {"a", "b", "c", "d", "e", "f"};
u8 g_bSaveLoadChoice, g_bSaveLoadStep;
u8 g_abPakDataSeasonNoteName[16] = {4}, g_abPakDataGhostNoteName[16] = {5};
u8 g_abPakDataConfigurationNoteName[16] = {6}, g_abPakDataNoteExtension[4];
s32 g_bLoadConfigurationPending, g_bSaveConfigurationPending;
s32 g_dwOptionsMenuItemCount, g_dwOptionsMenuSelection;
char g_szBgmVolumeTextBuf[0x18], g_szSfxVolumeTextBuf[0x18];
u8 g_abSavePakIdKnown[2], g_bSaveLoadDataWasLoaded, g_bSaveLoadInputEnabled;
s32 g_dwSaveLoadNoteIndex, g_dwPakDataNoteIndex;
u8 g_bSaveLoadFadeDelayFrames, g_bPakDataStep, g_bPakDataWaitFrames;

Sprite g_texButtonA, g_texButtonB, g_texBackground, g_texLogo;
OSPfs g_aPakPfs[MAXCONTROLLERS], g_aContPfs[MAXCONTROLLERS];
s32 gControllerPakState;
u8 g_aabSavePakIds[2][32];
OSMesgQueue g_SiMesgQueue;
u8 g_abRumblePakPresent[MAXCONTROLLERS];
s16 g_wSerialInterfaceIdle;
PlayerInputState g_aPlayerInputState[MAX_PLAYERS];
u32 g_dwNumLocalPlayers = 1;
s32 g_dwMenuChoosingPlayer;
Vehicle g_aVehicles[MAX_VEHICLES];
u8 g_bBgmVolume, g_bSfxMasterVolume;
u8 g_abGhostRecording[0x100];
u32 g_dwGhostRecordingSize, g_dwSelectedTrackId;
s32 g_dwScreenWidth = 320, g_dwScreenHeight = 240;
Gfx *gDisplayListPtr;
s32 g_dwMenuInputPlayerMask, g_dwScreenPlayerIdx;
MenuPostDrawCallback g_pfnScreenPostDrawCallback;
Texture *g_pMenuTitleImage;
Texture g_TitleBarTextureDesc;
s32 g_dwMenuTitleImageX, g_dwMenuTitleImageY, g_dwMenuTitleImageWidth, g_dwMenuTitleImageHeight;

static PlayerProfile profiles[2];
static Gfx gfx[64];
static u8 heap[0x4000];
static s32 menuResult;
static ScreenCallback screen;
static s32 fadedToBlack, fadedToWhite = 1, credits;
static const char *texts[64];
static s32 textCount;

/* The fake Controller Pak: whether it is plugged in, what mounting returns, its note. */
static s32 pakPlugged = 1, pakInit, pakNoteExists, pakFree = 0x1000, motor = -1;
static u8 pakNote[0x4000];
static s32 pakWrites, compressResult;

void SetTextStyleNormal(void) {
}
void SetTextAlignCenter(void) {
}
void SetTextAlignLeft(void) {
}
void SetTextSize(s32 size) {
}
void SetTextCustomColors(s32 r, s32 g, s32 b, s32 r2, s32 g2, s32 b2) {
}
void DrawRectBorder(s32 x, s32 y, s32 w, s32 h, s32 b, u8 c0, u8 c1, u8 c2, u8 c3, u8 c4) {
}
void DrawText(const char *text, s32 x, s32 y) {
    if (textCount < 64) {
        texts[textCount++] = text;
    }
}
void DrawTexturedSprite(Sprite *sprite, s32 x, s32 y) {
}
void DrawSpriteTinted(Sprite *sprite, u8 red, u8 green, u8 blue) {
}
void LoadTextureFromRom(Sprite *sprite) {
}
void *AllocHeapAligned(u32 size) {
    return heap;
}
void ResetHeap(void) {
}
void FadeSfxTo(float target, float seconds) {
}
void FadeMusicTo(float target, float seconds) {
}
s32 HasSfxFadeFinished(void) {
    return TRUE;
}
s32 IsFadedToWhite(void) {
    return fadedToWhite;
}
s32 IsFadedToBlack(void) {
    return fadedToBlack;
}
void SetFadeTarget(float target, float seconds) {
}
void UpdateFrameTiming(void) {
}
void UpdateFadeTransitions(void) {
}
void StartHighResFrame(void) {
}
void SetupProjectionMatrix(f32 width, f32 height) {
}
void SetViewport(s32 x, s32 y, s32 width, s32 height, s32 setScissor) {
}
void ClearDepthBuffer(void) {
}
void ClearScreen(s32 red, s32 green, s32 blue) {
}
void FlushDisplayList(void) {
}
void ApplyScreenFade(void) {
}
void SetCurrentScreenCallback(ScreenCallback callback) {
    screen = callback;
}
void ResetAndInitDisplay(MenuItem **items, s32 *itemCount) {
    *itemCount = OPTIONS_ITEMS;
}
s32 ProcessScreenState(const char *title,
                       s32 itemCount,
                       MenuItem **items,
                       s32 *selection,
                       MenuItemFilter isSelectable,
                       s32 r,
                       s32 g,
                       s32 b,
                       s32 r2,
                       s32 g2,
                       s32 b2) {
    return menuResult;
}
void ProcessControllerConfigScreen(void) {
}
void ProcessMainMenu(void) {
}
void ProcessTrackMenu(void) {
}
void StartChampionshipRace(void) {
}
void StartTimeAttack(void) {
}
void StartCreditsRace(void) {
    credits++;
}
void TransferFlags(PlayerInputState *state, u32 mask) {
    state->buttons &= ~mask;
}
void UpdateStickRepeat(PlayerInputState *state) {
}
s32 CompressRleChannels(u8 *dst, s32 dstSize, u8 *src, s32 srcSize, s32 channels) {
    return compressResult;
}
s32 DecompressInterleavedRle(u8 *dst, s32 dstSize, u8 *src, s32 channels) {
    dst[0] = 3;
    dst[1] = 5;
    return 0x40;
}
void osViBlack(u8 active) {
}
void osSyncPrintf(const char *fmt, ...) {
}
s32 osPfsIsPlug(OSMesgQueue *queue, u8 *pattern) {
    *pattern = pakPlugged;
    return 0;
}
s32 osPfsInitPak(OSMesgQueue *queue, OSPfs *pfs, int channel) {
    pfs->id[0] = 0x42;
    return pakInit;
}
s32 osPfsRepairId(OSPfs *pfs) {
    return 0;
}
s32 osPfsChecker(OSPfs *pfs) {
    return 0;
}
s32 osPfsFindFile(OSPfs *pfs, u16 company, u32 game, u8 *name, u8 *ext, s32 *file) {
    assert(company == 0x3544 && game == 0x4E475245);
    *file = 7;
    return pakNoteExists ? 0 : PFS_ERR_INVALID;
}
s32 osPfsReadWriteFile(OSPfs *pfs, s32 file, u8 flag, int offset, int size, u8 *data) {
    assert(file == 7);
    if (flag == PFS_READ) {
        memcpy(data, pakNote + offset, size);
    } else {
        memcpy(pakNote + offset, data, size);
        pakWrites++;
    }
    return 0;
}
s32 osPfsFreeBlocks(OSPfs *pfs, s32 *free) {
    *free = pakFree;
    return 0;
}
s32 osPfsAllocateFile(OSPfs *pfs, u16 company, u32 game, u8 *name, u8 *ext, int size, s32 *file) {
    *file = 7;
    pakNoteExists = TRUE;
    return 0;
}
s32 osMotorInit(OSMesgQueue *queue, OSPfs *pfs, int channel) {
    return motor;
}
s32 osMotorStop(OSPfs *pfs) {
    return 0;
}

static s32 Drew(const char *text) {
    s32 i;

    for (i = 0; i < textCount; i++) {
        if (strcmp(texts[i], text) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

static void Frame(void) {
    textCount = 0;
    gDisplayListPtr = gfx;
    ProcessDataSaveLoadMenu();
}

static void TestVolumes(void) {
    g_OptionSettings.musicVolume = 10;
    IncreaseBgmVolume();
    assert(g_OptionSettings.musicVolume == 10 && g_bBgmVolume == 48);
    DecreaseBgmVolume();
    assert(g_OptionSettings.musicVolume == 9 && g_bBgmVolume == 43);
    g_OptionSettings.sfxVolume = 0;
    DecreaseSfxVolume();
    assert(g_OptionSettings.sfxVolume == 0 && g_bSfxMasterVolume == 0);
    IncreaseSfxVolume();
    assert(g_bSfxMasterVolume == 25);
    ApplyVolumeSettings();
    assert(g_bBgmVolume == 43 && g_bSfxMasterVolume == 25);
}

static void TestOptionsMenu(void) {
    g_dwOptionsMenuState = 0;
    g_OptionSettings.speedInMph = 1;
    g_dwOptionsMenuSelection = 4;
    menuResult = 0;
    ProcessOptionsMenu();
    assert(g_dwOptionsMenuState == 1 && g_dwOptionsMenuSelection == 0);
    assert(g_pfnScreenPostDrawCallback == OptionsMenuPostDraw && g_dwMenuTitleImageWidth == 280);
    assert(strcmp(g_aOptionsMenuItems[OPTIONS_ITEM_BGM].name, "BGM Volume: 9") == 0);
    assert(strcmp(g_aOptionsMenuItems[OPTIONS_ITEM_UNITS].name, "Units: mph") == 0);
    g_dwOptionsMenuSelection = OPTIONS_ITEM_UNITS;
    menuResult = MENU_RESULT_DOWN;
    ProcessOptionsMenu();
    assert(g_OptionSettings.speedInMph == 0);
    menuResult = MENU_RESULT_CHOSEN;
    g_dwOptionsMenuSelection = OPTIONS_ITEM_LOAD_CONFIGURATION;
    ProcessOptionsMenu();
    assert(g_bLoadConfigurationPending && !g_bSaveConfigurationPending);
    g_bLoadConfigurationPending = FALSE;
    g_dwOptionsMenuSelection = OPTIONS_ITEM_CREDITS;
    ProcessOptionsMenu();
    assert(credits == 1 && g_dwOptionsMenuState == -1);
    /* Coming back keeps the selection. */
    menuResult = 0;
    ProcessOptionsMenu();
    assert(g_dwOptionsMenuSelection == OPTIONS_ITEM_CREDITS && g_dwOptionsMenuState == 1);
    menuResult = MENU_RESULT_CANCELLED;
    ProcessOptionsMenu();
    assert(screen == ProcessMainMenu && g_dwOptionsMenuState == 0);
}

/* Save the configuration to a Pak that lacks the note, then load it back quietly. */
static void TestPakDataScreen(void) {
    s32 frames;

    g_pPakDataBuffer = heap;
    g_aVehicles[0].profile = &profiles[0];
    g_aVehicles[1].profile = &profiles[1];
    profiles[0].handling = 1;
    profiles[0].transmission = 2;
    profiles[0].tires = 3;
    profiles[0].suspension = 4;
    profiles[0].controllerLayout = 5;
    g_OptionSettings.musicVolume = 7;
    g_OptionSettings.sfxVolume = 6;
    g_OptionSettings.speedInMph = 1;
    pakNoteExists = FALSE;
    g_bSaveLoadMenuInitialized = FALSE;
    for (frames = 0; frames < 20 && !ProcessPakDataScreen(PAK_DATA_SAVE_CONFIGURATION, FALSE);
         frames++) {
        if (g_bPakDataStep == PAK_DATA_STEP_DONE) {
            g_aPlayerInputState[0].buttons = INPUT_A;
        }
    }
    assert(frames < 20 && pakWrites == 2 && g_wSerialInterfaceIdle && !g_bSaveLoadMenuInitialized);
    assert(((s32 *)pakNote)[0] == 7 && ((s32 *)pakNote)[2] == 1);
    assert(((s32 *)(pakNote + 0x80))[0] == 1 && ((s32 *)(pakNote + 0x80))[4] == 5);

    g_OptionSettings.musicVolume = 0;
    profiles[0].tires = 0;
    for (frames = 0; frames < 20 && !ProcessPakDataScreen(PAK_DATA_LOAD_CONFIGURATION, TRUE);
         frames++) {
    }
    assert(frames < 20 && g_OptionSettings.musicVolume == 7 && profiles[0].tires == 3);
    /* Quietly, the volumes wait for whoever applies them. */
    assert(g_bBgmVolume != 33);

    /* A missing season is no error when quiet. */
    pakNoteExists = FALSE;
    for (frames = 0; frames < 20 && !ProcessPakDataScreen(PAK_DATA_LOAD_SEASON, TRUE); frames++) {
    }
    assert(frames < 20 && gControllerPakState == 0);
}

/* The main menu saves the options, then a ghost that is too large, then the Pak is missing. */
static void TestSaveLoadMenu(void) {
    s32 frames;

    g_bSaveLoadEntryPoint = SAVE_LOAD_FROM_MAIN_MENU;
    g_bSaveLoadMenuInitialized = FALSE;
    pakWrites = 0;
    pakNoteExists = TRUE;
    fadedToWhite = FALSE;
    Frame();
    assert(g_bSaveLoadChoice == SAVE_LOAD_LOAD_SEASON && g_bSaveLoadStep == SAVE_LOAD_STEP_FADE_IN);
    fadedToWhite = TRUE;
    Frame();
    assert(g_bSaveLoadStep == SAVE_LOAD_STEP_CHOOSE);
    /* Up from the first choice wraps to the last. */
    g_aPlayerInputState[0].buttons = INPUT_UP;
    Frame();
    assert(g_bSaveLoadChoice == SAVE_LOAD_SAVE_OPTIONS);
    g_aPlayerInputState[0].buttons = INPUT_A;
    for (frames = 0; frames < 20 && g_bSaveLoadStep != SAVE_LOAD_STEP_DONE; frames++) {
        Frame();
    }
    /* An existing options note is overwritten without asking. */
    assert(g_bSaveLoadStep == SAVE_LOAD_STEP_DONE && pakWrites == 2);
    Frame();
    assert(Drew("%ywOPTIONS SAVED OK!"));
    g_aPlayerInputState[0].buttons = INPUT_B;
    Frame();
    assert(g_bSaveLoadStep == SAVE_LOAD_STEP_CHOOSE);

    g_aPlayerInputState[0].buttons = INPUT_DOWN;
    Frame();
    assert(g_bSaveLoadChoice == SAVE_LOAD_LOAD_SEASON);
    g_bSaveLoadChoice = SAVE_LOAD_SAVE_GHOST;
    g_aPlayerInputState[0].buttons = INPUT_A;
    compressResult = -1;
    for (frames = 0; frames < 20 && g_bSaveLoadStep != SAVE_LOAD_STEP_CONFIRM_OVERWRITE; frames++) {
        Frame();
    }
    g_aPlayerInputState[0].buttons = INPUT_A;
    Frame();
    Frame();
    assert(gControllerPakState == PAK_STATE_GHOST_TOO_LARGE &&
           g_bSaveLoadStep == SAVE_LOAD_STEP_ERROR);
    Frame();
    assert(Drew("UNABLE TO SAVE GHOST CAR DATA."));
    g_aPlayerInputState[0].buttons = INPUT_Z;
    Frame();
    assert(gControllerPakState == 0 && g_bSaveLoadStep == SAVE_LOAD_STEP_CHOOSE);

    pakPlugged = 0;
    g_aPlayerInputState[0].buttons = INPUT_A;
    for (frames = 0; frames < 20 && g_bSaveLoadStep != SAVE_LOAD_STEP_ERROR; frames++) {
        Frame();
    }
    assert(gControllerPakState == PFS_ERR_NOPACK);
    Frame();
    assert(Drew("CONTROLLER PAK NOT FOUND.") && Drew("DATA CANNOT BE SAVED."));
    pakPlugged = 1;

    /* Leaving returns to the main menu and leaves the serial interface reserved. */
    g_bSaveLoadStep = SAVE_LOAD_STEP_FADE_OUT;
    Frame();
    fadedToBlack = TRUE;
    Frame();
    assert(screen == ProcessMainMenu && !g_bSaveLoadMenuInitialized && !g_wSerialInterfaceIdle);
    fadedToBlack = FALSE;

    /* Loading a ghost car unlocks its track and car, and starts a time attack. */
    g_bSaveLoadMenuInitialized = FALSE;
    Frame();
    Frame();
    g_bSaveLoadChoice = SAVE_LOAD_LOAD_GHOST;
    g_aPlayerInputState[0].buttons = INPUT_A;
    profiles[0].unlocks = 0;
    profiles[0].unlockedTracks = 0;
    for (frames = 0; frames < 20 && g_bSaveLoadStep != SAVE_LOAD_STEP_LEAVE; frames++) {
        Frame();
    }
    assert(g_dwSelectedTrackId == 3 && profiles[0].unlockedTracks == 1 << 3);
    assert(profiles[0].unlocks == 1 << 5 && g_bSaveLoadDataWasLoaded);
    fadedToBlack = TRUE;
    Frame();
    assert(screen == StartTimeAttack);
    fadedToBlack = FALSE;
}

static void TestRumblePaks(void) {
    motor = 0;
    InitAllRumblePaks();
    assert(g_abRumblePakPresent[0] && g_wSerialInterfaceIdle);
    pakInit = PFS_ERR_ID_FATAL;
    assert(!IsRumblePakMissing());
    pakInit = 0;
    assert(IsRumblePakMissing());
    motor = -1;
    g_aPlayerInputState[1].buttons = INPUT_B;
    g_dwMenuChoosingPlayer = 0;
    assert(!DrawRumblePakPrompt(FALSE));
    assert(DrawRumblePakPrompt(TRUE) && g_aPlayerInputState[1].buttons == 0);
}

int main(void) {
    TestVolumes();
    TestOptionsMenu();
    TestPakDataScreen();
    TestSaveLoadMenu();
    TestRumblePaks();
    puts("options ok");
    return 0;
}
