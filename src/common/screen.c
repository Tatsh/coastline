#include "common/screen.h"

#if TGR_BOSS_RALLY
/** @ghidraAddress BRD3D: 0x106c299c */
s32 g_dwScreenHeight;
/** @ghidraAddress BRD3D: 0x106c0684 */
s32 g_dwScreenWidth;
#endif /* TGR_BOSS_RALLY */

#if TGR_BOSS_RALLY
ScreenCallback g_pfnCurrentScreen;
ScreenCallback g_pfnNextScreen;
ScreenCallback g_pfnSecondaryScreen;

void NullStub(void) {
}

/* @ghidraAddress BRD3D: 0x10034c2d */
/* @ghidraAddress BRGlide: 0x1002e2de */
[[maybe_unused]] static void StubUnusedCallback(void) {
}
#endif /* TGR_BOSS_RALLY */

void StubRenderCallback(void) {
}

void SetNextScreenCallback(ScreenCallback callback) {
    g_pfnNextScreen = callback;
}

void SetSecondaryScreenCallback(ScreenCallback callback) {
    g_pfnSecondaryScreen = callback;
}

s32 IsCurrentScreen(ScreenCallback callback) {
    return callback == g_pfnCurrentScreen;
}

void SetCurrentScreenCallback(ScreenCallback callback) {
    g_pfnCurrentScreen = callback;
}

void CallCurrentGameMode(void) {
    g_pfnCurrentScreen();
}

void GameModeMainLoop(void) {
#if TGR_PLATFORM == TGR_PLATFORM_N64
    for (;;) {
        CallCurrentGameMode();
    }
#endif
}

void StubUpdateCallback(void) {
}
