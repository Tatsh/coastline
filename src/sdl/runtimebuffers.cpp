// The variables of the race engine that the Windows libraries define in their platform units, and
// the console in its data. Each has the type, the size, and the first value of the Windows
// definition.
#include <PR/os.h>

#include "common/frontendstate.h"
#include "common/hudassets.h"
#include "common/inputpoll.h"
#include "common/racephases.h"
#include "common/render.h"
#include "common/renderbackend.h"
#include "common/session.h"
#include "common/settings.h"
#include "common/snapshot.h"
#include "common/system.h"
#include "common/trackskidmarks.h"
#include "common/vehicledeform.h"
#include "enginetextures.h"

unsigned char g_abDaySkyTexture[2048];
int g_abFrameSnapshotInUse[FRAME_SNAPSHOT_COUNT];
unsigned char g_abNightSkyTexture[2048];
int g_anSkidMarkHead[SKID_MARK_TRACK_COUNT];
int g_anSkidMarkTail[SKID_MARK_TRACK_COUNT];
// The path of BossRally.cfg. A path of the host may be longer than a path of Windows.
char g_szConfigFilePath[4096];

// The threads of the console that wait for the RSP, the RDP, and the retrace.
u32 g_dwDummyOsMesg;
s32 g_dwVblankCounter;
OSMesgQueue g_RdpEventMsgQueue;
OSMesgQueue g_RspEventMsgQueue;
OSMesgQueue g_ViRetracePollMesgQueue;

// The front end, the game session, and the network session.
int g_dwChosenCar;
int g_dwInNetworkCarSetup;
char g_szSeasonSaveFileName[FRONT_END_SAVE_PATH_SIZE];
char g_szTimeAttackSaveFileName[FRONT_END_SAVE_PATH_SIZE];
int g_dwFrontEndAtRootMenu;
int g_dwSelectedTransmission;
int g_dwTireMenuIndex = 1;
int g_dwSuspensionMenuIndex = 1;
int g_dwCarMenuIndex = 1;
int g_dwTotalLapsSetting = 3;
int g_dwAccumUnlockedCarMask;
int g_dwAccumUnlockedTrackMask;
int g_dwHandlingTypeMenuIndex = 4;
s16 g_wVehicleDeformMaxDent = 0x100;
int g_bRaceStartReached;
int g_dwRunBenchmark;
int g_dwNetworkPlayMode;
const char g_szEmptyUiString[1] = {0};
int g_dwNullStubEnable = 1;

// The input of the players.
int g_bNetGamePaused;
int g_bNetReadyToStartSent;
int g_bRespawnRequested;
unsigned g_dwInputButtonMask;

// The renderer. The SDL game sets no routine of a renderer yet.
int g_adwPrecipitationTextureIds[PRECIPITATION_TEXTURE_COUNT];
int g_bDisableCarShadow;
int g_bForceTextureClone;
int g_dwAlphaRampStripRowCount;
unsigned g_dwFallbackEnvMapTextureId;
unsigned g_dwHudOverlayHalfTextureId;
unsigned g_dwHudOverlayTextureId;
unsigned g_dwParticleTextureId;
unsigned g_dwTireTrailTextureId;
unsigned g_dwVehicleEnvMapTextureId;
// The renderer of the SDL game draws the race from the converted models and textures. The hooks
// of the renderers of the Windows libraries keep the 8-bit colour-indexed images of the engine,
// the dashboards of the cars, as texels for the SDL game, and otherwise do nothing.
constexpr int IMAGE_SIZE_8 = 1;
constexpr int IMAGE_FORMAT_CI = 2;
decltype(g_pfnAliasTextureEntrySurface) g_pfnAliasTextureEntrySurface = [](int, int) {};
decltype(g_pfnCreateTextureFromN64Image) g_pfnCreateTextureFromN64Image =
    [](const unsigned char *image,
       const unsigned char *palette,
       int width,
       int height,
       int lineBytes,
       int size,
       int format,
       int,
       int,
       int,
       int,
       int,
       int,
       int,
       int) -> unsigned {
    if (size != IMAGE_SIZE_8 || format != IMAGE_FORMAT_CI) {
        return 0;
    }
    return coastline::CreateEngineTexture(image, palette, width, height, lineBytes);
};
decltype(g_pfnDecodeN64TextureToPixelBuffer) g_pfnDecodeN64TextureToPixelBuffer =
    [](void *destination,
       const unsigned char *image,
       const unsigned char *palette,
       int width,
       int height,
       int lineBytes,
       int size,
       int format) {
        if (size != IMAGE_SIZE_8 || format != IMAGE_FORMAT_CI) {
            return 0;
        }
        return coastline::DecodeEngineTexture(
            destination, image, palette, width, height, lineBytes);
    };
decltype(g_pfnInitializeTextureSubsystem) g_pfnInitializeTextureSubsystem = [] {};
decltype(g_pfnRenderBackendInit) g_pfnRenderBackendInit = [] {};
decltype(g_pfnRenderBackendShutdown) g_pfnRenderBackendShutdown = [] {};
decltype(g_pfnRenderBackendSubmitDisplayList) g_pfnRenderBackendSubmitDisplayList =
    [](GbiCommand *) {};
decltype(g_pfnRenderBackendUpdateFog) g_pfnRenderBackendUpdateFog = [] {};
decltype(g_pfnShutdownTextureSubsystem) g_pfnShutdownTextureSubsystem = [] {
    coastline::ClearEngineTextures();
};
