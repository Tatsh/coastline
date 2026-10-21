/* Host check of the control flow of src/controllerscreen.c. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "controllerscreen.h"
#include "heap.h"
#include "options.h"
#include "system.h"
#include "text.h"
#include "vehicle.h"

Texture g_texControllerLayout0, g_texControllerLayout1, g_texControllerLayout2,
    g_texControllerLayout3, g_texControllerLayout4, g_TitleBarTextureDesc;
MenuItem *g_apControllerScreenItems[6];
s32 g_dwScreenPlayerIdx, g_dwMenuTitleImageX, g_dwMenuTitleImageY, g_dwMenuTitleImageWidth,
    g_dwMenuTitleImageHeight;
Texture *g_pMenuTitleImage;
MenuPostDrawCallback g_pfnScreenPostDrawCallback;
Vehicle g_aVehicles[MAX_VEHICLES];
static PlayerProfile profile;
static s32 result, loads, texts, pictures;
static const char *lastTitle;
static s32 *selection;
static ScreenCallback lastCallback;

void ResetAndInitDisplay(MenuItem **items, s32 *itemCount) {
}
void LoadAndDecompressAsset(Texture *texture, AssetAllocator alloc) {
    loads++;
}
void *AllocHeapAligned(u32 size) {
    return NULL;
}
void SetCurrentScreenCallback(ScreenCallback callback) {
    lastCallback = callback;
}
void ProcessOptionsMenu(void) {
}
void SetTextSize(s32 size) {
}
void SetTextStyleNormal(void) {
}
void SetTextAlignLeft(void) {
}
void SetTextAlignRight(void) {
}
void SetTextAlignCenter(void) {
}
void DrawText(const char *text, s32 x, s32 y) {
    texts++;
}
void DrawTexturedRect(Texture *t,
                      s32 x,
                      s32 y,
                      s32 w,
                      s32 h,
                      s32 a,
                      s32 b,
                      s32 c,
                      s32 d,
                      s32 e,
                      s32 f,
                      s32 g,
                      s32 i,
                      s32 mode) {
    pictures++;
}
s32 ProcessScreenState(const char *title,
                       s32 n,
                       MenuItem **items,
                       s32 *sel,
                       MenuItemFilter f,
                       s32 tr,
                       s32 tg,
                       s32 tb,
                       s32 br,
                       s32 bg,
                       s32 bb) {
    lastTitle = title;
    selection = sel;
    assert(tr == 8 && br == 0xA0 && bg == 0x80 && bb == 0x20);
    return result;
}

int main(void) {
    profile.controllerLayout = 3;
    g_aVehicles[1].profile = &profile;
    g_dwScreenPlayerIdx = 1;

    result = 0;
    ProcessControllerConfigScreen();
    assert(loads == 5 && *selection == 3);
    assert(strcmp(lastTitle, "Controller 2") == 0);
    assert(g_pfnScreenPostDrawCallback == DrawControllerLayoutDiagram);
    /* A running screen does not start again. */
    ProcessControllerConfigScreen();
    assert(loads == 5);

    *selection = 2;
    result = MENU_RESULT_CHOSEN;
    ProcessControllerConfigScreen();
    assert(profile.controllerLayout == 2 && lastCallback == ProcessOptionsMenu);
    /* Leaving the screen returns it to the state before it starts. */
    result = 0;
    ProcessControllerConfigScreen();
    assert(loads == 10 && *selection == 2);

    DrawControllerLayoutDiagram(3);
    assert(pictures == 3 && texts == 8);
    puts("controllerscreen ok");
    return 0;
}
