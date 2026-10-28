#include "pcgamerun.h"

#include <cstdlib>
#include <cstring>
#include <memory>

#include "sdl/d3dcompat.h"
#include "sdl/gamefiles.h"
#include "sdl/renderhost.h"
#include "win32/boot/app.h"
#include "win32/boot/rallyconfig.h"
#include "win32/boot/strings.h"
#include "win32/render.h"
#include "win32/settings.h"
#include "win32/snapshot.h"

// The anchor of the offsets of Ptr32. It is part of the static data of the program, and every
// object a Ptr32 addresses is static data or comes from the pool of CHK_AllocateMemory().
extern "C" unsigned char g_abPtr32Anchor[16];
unsigned char g_abPtr32Anchor[16];

// The globals of the window of winmain-d3d.cpp. The window of SDL is always active.
HINSTANCE g_hAppInstance;
HWND g_hMainWindow;
WPARAM g_dwAppActive = 1;

void ShutdownApplication(int nExitCode) {
    FreeAllRacerColorPanels();
    InvalidateFrameSnapshotSlots();
    FreeStringTable();
    std::exit(nExitCode);
}

namespace coastline {

namespace {

std::unique_ptr<IDirect3DDevice2> g_device;
DirectDrawRenderer g_mainRenderWindow;

} // namespace

bool StartPcGame(TlRenderer &renderer,
                 const std::filesystem::path &data,
                 const std::filesystem::path &state) {
    SetGameDirectories(data, state);
    // The PC release opens its archive and other files by a path relative to the working
    // directory, its installation directory on Windows.
    std::error_code error;
    std::filesystem::current_path(data, error);
    if (error) {
        return false;
    }
    g_device = std::make_unique<IDirect3DDevice2>(renderer);
    g_pD3DDevice2 = g_device.get();
    g_pMainRenderWindow = &g_mainRenderWindow;
    // RallyMain() of BRD3D.dll without COM, the version of DirectX, the window of the PC release,
    // and DirectPlay.
    RefreshTotalPhysicalMemory();
    ReadInstallDirectoryFromRegistry();
    LoadStringTableFromDll();
    LoadBossRallyIniSettings("");
    // ponytail: music mode 2 needs the EAR library, which the SDL game does not have yet.
    if (g_dwPlayMusicMode == 2) {
        g_dwPlayMusicMode = 0;
    }
    std::strcpy(g_szConfigFilePath, g_szInstallDirectory);
    std::strcat(g_szConfigFilePath, "BossRally.cfg");
    g_GlobalRallyConfig.LoadRallyConfigFile(g_szConfigFilePath);
    InitializeTextureSubsystem();
    return true;
}

bool RunPcGameFrame(const std::filesystem::path *capture) {
    g_device->SetCapture(capture);
    g_device->Renderer().SetScreenSize(static_cast<std::uint32_t>(g_dwConfigFramebufferWidth),
                                       static_cast<std::uint32_t>(g_dwConfigFramebufferHeight));
    return DispatchGameStateFrame() != 0;
}

} // namespace coastline
