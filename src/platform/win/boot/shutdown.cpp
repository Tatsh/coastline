#include <cstdlib>

#include <objbase.h>
#include <windows.h>

#include "platform/win/boot/app.h"
#include "platform/win/boot/strings.h"
#include "platform/win/d3d/renderer.h"
#include "platform/win/directplay.h"
#include "platform/win/frontend.h"
#include "platform/win/input.h"
#include "platform/win/paintshop.h"
#include "platform/win/podarchive.h"
#include "platform/win/render.h"
#include "platform/win/settings.h"
#include "platform/win/snapshot.h"
#include "platform/win/sound.h"

void ShutdownApplication(int nExitCode) {
    if (g_pActiveMenuScreen != nullptr && g_dwNullStubEnable) {
        g_pActiveMenuScreen->bRunning = false;
        g_pActiveMenuScreen->ShutdownFrontEnd(false);
    }
    FreeAllRacerColorPanels();
    InvalidateFrameSnapshotSlots();
    if (g_pfnRenderBackendShutdown != nullptr) {
        g_pfnRenderBackendShutdown();
    }
    ReleaseJoystickDevice();
    ReleaseKeyboardDevice();
    ShutdownDirectInput();
    ShutdownDirectSound();
    if (g_dwNetworkPlayMode) {
        ShutdownNetworkSubsystem(1);
    }
    ShutdownDirectPlay();
    DisconnectNetworkSession();
    if (g_dwPlayMusicMode) {
        ReleaseMusicDeviceReference();
    }
    NullStub();
    if (g_pfnShutdownTextureSubsystem != nullptr) {
        g_pfnShutdownTextureSubsystem();
    }
    if (g_pfnRenderDeviceShutdown != nullptr) {
        g_pfnRenderDeviceShutdown();
    }
    FreeAllPaintBitmaps();
    g_BossRallyPodArchive.ClosePodFile();
    ShutdownEarSound();
    FreeStringTable();
    CoUninitialize();
    exit(nExitCode);
}
