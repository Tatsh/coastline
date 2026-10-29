#include "win32/app.h"
#include "win32/buildinfo.h"
#include "win32/rallyconfig.h"
#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include "sdl/renderhost.h"
#elif TGR_BRD3D
#include "win32/d3d/renderer.h"
#endif
#include "win32/directplay.h"
#include "win32/gameinit.h"
#include "win32/input.h"
#include "win32/paintshop.h"
#include "win32/render.h"
#include "win32/renderinit.h"
#include "win32/season.h"
#include "win32/settings.h"
#include "win32/sound.h"

void ShutdownGameSession(void) {
    PrintWineDiagnostic("coastline: Shutting down race session.\n");
    if (g_dwSavedSfxVolume != g_GlobalRallyConfig.dwSfxVolume ||
        g_dwSavedMusicVolume != g_GlobalRallyConfig.dwMusicVolume) {
        g_GlobalRallyConfig.SaveConfigToFile(g_szConfigFilePath);
    }
    ResetAudioSubsystem();
    PrintWineDiagnostic("coastline: Race audio reset.\n");
    ReleaseJoystickDevice();
    if (g_dwNetworkPlayMode != 0) {
        if (g_dwNetworkPlayMode > 1) {
            ResetAllPlayerSlots();
        }
        ResetNetworkState();
    }
    SetGameStateFrontEnd();
    CloseHandle(g_hRenderMutex);
    g_hRenderMutex = nullptr;
    PrintWineDiagnostic("coastline: Restoring menu renderer.\n");
#if TGR_BRD3D
    g_pMainRenderWindow->BeginRenderScene();
#endif
    ResetRenderSubsystems(RENDER_BACKEND_MODE, 640, 480, 16, 0);
    PrintWineDiagnostic("coastline: Menu renderer restored; freeing paint textures.\n");
    FreeCarPaintTextures();
    PrintWineDiagnostic("coastline: Race session shutdown completed.\n");
}
