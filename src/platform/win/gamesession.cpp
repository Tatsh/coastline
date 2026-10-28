#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/buildinfo.h"
#if TGR_RENDERER == TGR_RENDERER_D3D
#include "platform/win/d3d/renderer.h"
#endif
#include "platform/win/directplay.h"
#include "platform/win/gameinit.h"
#include "platform/win/input.h"
#include "platform/win/paintshop.h"
#include "platform/win/render.h"
#include "platform/win/renderinit.h"
#include "platform/win/season.h"
#include "platform/win/settings.h"
#include "platform/win/sound.h"

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
#if TGR_RENDERER == TGR_RENDERER_D3D
    g_pMainRenderWindow->BeginRenderScene();
#endif
    ResetRenderSubsystems(RENDER_BACKEND_MODE, 640, 480, 16, 0);
    PrintWineDiagnostic("coastline: Menu renderer restored; freeing paint textures.\n");
    FreeCarPaintTextures();
    PrintWineDiagnostic("coastline: Race session shutdown completed.\n");
}
