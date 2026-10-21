/* Host check of the control flow of src/pakmenu.c with the Pak and drawing stubbed. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "fade.h"
#include "game.h"
#include "heap.h"
#include "input.h"
#include "pakmenu.h"
#include "render.h"
#include "rumble.h"
#include "system.h"
#include "text.h"

Sprite g_texBackground, g_texLogo, g_texButtonA, g_texButtonB, g_texButtonBSmall;
const char g_szDecalCharMap[66] =
    "\0               0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#'*+,-./:=?@";
u8 g_bPakMenuInitialized, g_bPakDirLoaded, g_bPakSelectedNote, g_bPakFirstValidNote,
    g_bPakLastValidNote, g_bPakMenuState, g_aabSavePakIds[2][32];
u16 g_wPakEmptySlotMask, g_wPakScanEmptyMask;
s32 g_dwPakNoteCount, g_dwPakFreeBlocks, g_nPakFilesUsed, gControllerPakState;
OSPfsState g_aPakDirEntries[PAK_NOTES], g_aPakFileStates[PAK_NOTES];
OSPfs g_aPakPfs[MAXCONTROLLERS], g_aContPfs[MAXCONTROLLERS];
OSMesgQueue g_SiMesgQueue;
s16 g_wSerialInterfaceIdle;
PlayerInputState g_aPlayerInputState[MAX_PLAYERS];
Gfx *gDisplayListPtr;
s32 g_dwScreenWidth = 320, g_dwScreenHeight = 240;
static Gfx list[64];
static OSPfsState pak[PAK_NOTES];
static u16 pakFiles = (1 << 2) | (1 << 5);
static s32 faded = 1, deleted;
static ScreenCallback lastCallback;

s32 osPfsIsPlug(OSMesgQueue *q, u8 *pattern) {
    *pattern = 1;
    return 0;
}
s32 osPfsInitPak(OSMesgQueue *q, OSPfs *pfs, int channel) {
    return 0;
}
s32 osPfsRepairId(OSPfs *pfs) {
    return 0;
}
s32 osPfsChecker(OSPfs *pfs) {
    return 0;
}
s32 osMotorInit(OSMesgQueue *q, OSPfs *pfs, int channel) {
    return 1;
}
s32 osPfsNumFiles(OSPfs *pfs, s32 *max, s32 *used) {
    *max = 16;
    *used = 2;
    return 0;
}
s32 osPfsFileState(OSPfs *pfs, s32 n, OSPfsState *s) {
    if (!(pakFiles & (1 << n))) {
        return 5;
    }
    *s = pak[n];
    return 0;
}
s32 osPfsFreeBlocks(OSPfs *pfs, s32 *bytes) {
    *bytes = 100 << 8;
    return 0;
}
s32 osPfsDeleteFile(OSPfs *pfs, u16 c, u32 g, u8 *n, u8 *e) {
    deleted++;
    return 0;
}
void bcopy(const void *s, void *d, size_t n) {
    memmove(d, s, n);
}
void osViBlack(u8 b) {
}
void osSyncPrintf(const char *f, ...) {
}
void SetCurrentScreenCallback(ScreenCallback c) {
    lastCallback = c;
}
void MainMenuEntry(void) {
}
void ResetHeap(void) {
}
void LoadTextureFromRom(Sprite *s) {
}
void SetFadeTarget(float t, float s) {
}
s32 IsFadedToWhite(void) {
    return faded;
}
s32 IsFadedToBlack(void) {
    return 0;
}
void ApplyScreenFade(void) {
}
void UpdateFadeTransitions(void) {
}
void UpdateFrameTiming(void) {
}
void SetDisplayModeWide(void) {
}
void SetupProjectionMatrix(f32 w, f32 h) {
}
void SetViewport(s32 x, s32 y, s32 w, s32 h, s32 s) {
}
void SetupFillMode(void) {
}
void ClearScreen(s32 r, s32 g, s32 b) {
}
void FlushDisplayList(void) {
    gDisplayListPtr = list;
}
void DrawTiledSprite(Sprite *s, s32 a, s32 b, s32 c) {
}
void DrawSpriteRegion(Sprite *s, s32 x, s32 y, s32 w, s32 h, s32 r, s32 g, s32 b) {
}
void DrawTexturedSprite(Sprite *s, s32 x, s32 y) {
}
void DrawRectBorder(s32 x, s32 y, s32 w, s32 h, s32 b, s32 c0, s32 c1, s32 c2, s32 c3, s32 c4) {
}
void SetTextAlignCenter(void) {
}
void SetTextAlignLeft(void) {
}
void SetTextAlignRight(void) {
}
void SetTextStyleNormal(void) {
}
void SetTextSize(s32 s) {
}
void SetTextCustomColors(s32 a, s32 b, s32 c, s32 d, s32 e, s32 f) {
}
void DrawText(const char *t, s32 x, s32 y) {
}
void UpdateStickRepeat(PlayerInputState *s) {
}
void TransferFlags(PlayerInputState *s, u32 m) {
    s->buttons &= ~m;
}

int main(void) {
    PlayerInputState *in = &g_aPlayerInputState[0];

    assert(DecalCharToAscii(0x10) == '0' && DecalCharToAscii(0x1A) == 'A' &&
           DecalCharToAscii(80) == ' ');
    gDisplayListPtr = list;
    pak[2].file_size = 3 << 8;
    pak[2].game_name[0] = 0x1A;
    pak[5].file_size = 4 << 8;

    ProcessPakMenuScreen();
    assert(g_bPakMenuState == PAK_MENU_BROWSE && g_bPakDirLoaded);
    assert(g_bPakFirstValidNote == 2 && g_bPakLastValidNote == 5 && g_bPakSelectedNote == 2);

    in->buttons = INPUT_DOWN;
    ProcessPakMenuScreen();
    assert(g_bPakSelectedNote == 5);

    in->buttons = INPUT_A;
    ProcessPakMenuScreen();
    assert(g_bPakMenuState == PAK_MENU_CONFIRM_DELETE);
    in->buttons = INPUT_A;
    ProcessPakMenuScreen();
    assert(g_bPakMenuState == PAK_MENU_DELETE);
    in->buttons = 0;
    ProcessPakMenuScreen();
    assert(deleted == 1 && g_bPakMenuState == PAK_MENU_BROWSE);
    assert(g_bPakLastValidNote == 2 && g_bPakSelectedNote == 2 && g_dwPakNoteCount == 1);
    assert(g_dwPakFreeBlocks == (104 << 8) && g_wSerialInterfaceIdle == 1);

    /* A note of another game fails the start-up check. */
    pak[2].game_code = 0x4E475245;
    pak[2].company_code = 0x3544;
    pak[5].game_code = 1;
    assert(InitPakAndEnumerateFiles() == FALSE);
    pak[5].game_code = 0x4E475245;
    pak[5].company_code = 0x3544;
    g_wPakScanEmptyMask = 0;
    assert(InitPakAndEnumerateFiles() == TRUE);
    puts("pakmenu ok");
    return 0;
}
