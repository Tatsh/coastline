#include <cstdlib>

#include <objbase.h>
#include <windows.h>

#include "win32/app.h"
#include "win32/d3d/renderer.h"
#include "win32/directplay.h"
#include "win32/frontend.h"
#include "win32/input.h"
#include "win32/paintshop.h"
#include "win32/podarchive.h"
#include "win32/render.h"
#include "win32/settings.h"
#include "win32/snapshot.h"
#include "win32/sound.h"
#include "win32/strings.h"

void ShutdownApplication(int nExitCode) {
    if (g_pActiveMenuScreen != nullptr && g_dwNullStubEnable) {
        g_pActiveMenuScreen->SetRunning(false);
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
