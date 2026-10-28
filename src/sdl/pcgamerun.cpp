#include "pcgamerun.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <memory>
#include <mutex>
#include <vector>

#include "pause.h"
#include "race.h"
#include "sdl/d3dcompat.h"
#include "sdl/gamefiles.h"
#include "sdl/n64displaylist.h"
#include "sdl/renderhost.h"
#include "trackselect.h"
#include "win32/boot/app.h"
#include "win32/boot/rallyconfig.h"
#include "win32/boot/strings.h"
#include "win32/input.h"
#include "win32/render.h"
#include "win32/settings.h"
#include "win32/snapshot.h"
#include "win32/sound.h"

// The anchor of the offsets of Ptr32. It is part of the static data of the program, and every
// object a Ptr32 addresses is static data or comes from the pool of CHK_AllocateMemory().
extern "C" unsigned char g_abPtr32Anchor[16];
unsigned char g_abPtr32Anchor[16];

// The globals of the window of winmain-d3d.cpp. The window of SDL is always active.
HINSTANCE g_hAppInstance;
HWND g_hMainWindow;
WPARAM g_dwAppActive = 1;

namespace {

unsigned g_msvcRandomSeed = 1;

} // namespace

// The random numbers of the C library of Microsoft, which the PC release scales by 0x8000. They
// take the place of those of the C library of the system for the whole program.
extern "C" int rand(void) noexcept {
    g_msvcRandomSeed = g_msvcRandomSeed * 214013u + 2531011u;
    return static_cast<int>(g_msvcRandomSeed >> 16 & 0x7fff);
}

extern "C" void srand(unsigned seed) noexcept {
    g_msvcRandomSeed = seed;
}

void ShutdownApplication(int nExitCode) {
    FreeAllRacerColorPanels();
    InvalidateFrameSnapshotSlots();
    FreeStringTable();
    std::exit(nExitCode);
}

namespace coastline {

namespace {

struct PostedMessage {
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
};

std::unique_ptr<IDirect3DDevice2> g_device;
DirectDrawRenderer g_mainRenderWindow;
std::mutex g_messageMutex;
std::vector<PostedMessage> g_postedMessages;

// Handle the messages of the window procedure of winmain-d3d.cpp that the SDL game posts.
void DispatchPostedMessages() {
    std::vector<PostedMessage> messages;
    {
        std::lock_guard lock(g_messageMutex);
        messages.swap(g_postedMessages);
    }
    for (const PostedMessage &posted : messages) {
        // A track of the CD audio that ends plays again, unless the race is paused.
        if (posted.message == MM_MCINOTIFY && g_dwPlayMusicMode == 1 &&
            static_cast<MCIDEVICEID>(posted.lParam) == g_dwMciCdAudioDeviceId &&
            posted.wParam == MCI_NOTIFY_SUCCESSFUL && !g_dwRacePauseState) {
            PlayCdAudioFromCurrentTrack();
        }
    }
}

} // namespace

bool StartPcGame(TlRenderer &renderer,
                 const std::filesystem::path &data,
                 const std::filesystem::path &state,
                 bool pcMusic) {
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
    // Mode 1 plays the CD audio, and mode 0 leaves the music to the modules of the cartridge.
    // ponytail: mode 2, the music of the EAR library, waits for EAR on SDL.
    g_dwPlayMusicMode = pcMusic ? 1 : 0;
    std::strcpy(g_szConfigFilePath, g_szInstallDirectory);
    std::strcat(g_szConfigFilePath, "BossRally.cfg");
    g_GlobalRallyConfig.LoadRallyConfigFile(g_szConfigFilePath);
#ifdef ENABLE_PATCHES
    // The screen of the race has the aspect ratio of the window at the height of the display mode,
    // and the camera widens its view with it.
    if (renderer.FrameHeight() != 0) {
        g_GlobalRallyConfig.dwDisplayModeWidth = static_cast<int>(
            std::lround(static_cast<double>(g_GlobalRallyConfig.dwDisplayModeHeight) *
                        renderer.FrameWidth() / renderer.FrameHeight()));
    }
#endif
    InitializeTextureSubsystem();
    return true;
}

void StartPcGameForFrontEnd() {
    InitializeRenderSubsystems(RENDER_BACKEND_MODE);
    RenderN64DisplayLists();
    InitializeDirectInput();
    InitializeKeyboardDevice();
}

void SetPcGameCapture(const std::filesystem::path *capture) {
    g_device->SetCapture(capture);
}

bool RunPcGameFrame(const std::filesystem::path *capture) {
    g_device->SetCapture(capture);
    g_device->Renderer().SetScreenSize(static_cast<std::uint32_t>(g_dwConfigFramebufferWidth),
                                       static_cast<std::uint32_t>(g_dwConfigFramebufferHeight));
    DispatchPostedMessages();
    return DispatchGameStateFrame() != 0;
}

std::uint32_t PcGameMusicModule() {
    // The modules g_adwTrackMusicRom of the USA cartridge lists for each track, and the module of
    // its menus.
    constexpr std::uint32_t trackModules[] = {0x113660,
                                              0x12EAB0,
                                              0x149C80,
                                              0x164B60,
                                              0x17FD10,
                                              0x113660,
                                              0x12EAB0,
                                              0x149C80,
                                              0x164B60,
                                              0x17FD10,
                                              0x19BC10,
                                              0x12EAB0};
    constexpr std::uint32_t menuModule = 0x0EBC00;
    if (g_dwPlayMusicMode != 0) {
        return 0;
    }
    if (!g_dwRaceInitialized) {
        return menuModule;
    }
    return trackModules[std::min<std::size_t>(g_dwSelectedTrackId, std::size(trackModules) - 1)];
}

} // namespace coastline

BOOL PostMessageA(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    std::lock_guard lock(coastline::g_messageMutex);
    coastline::g_postedMessages.push_back({message, wParam, lParam});
    return TRUE;
}
