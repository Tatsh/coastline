#include "screen.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
ScreenCallback g_pfnCurrentScreen;
ScreenCallback g_pfnNextScreen;
ScreenCallback g_pfnSecondaryScreen;

void NullStub(void) {
}
#endif

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
