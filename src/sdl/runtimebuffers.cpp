// The variables of the race engine that the Windows libraries define in their platform units, and
// the console in its data. Each has the type, the size, and the first value of the Windows
// definition.
#include <PR/os.h>

#include "common/assetnames.h"
#include "common/camera.h"
#include "common/effects.h"
#include "common/fade.h"
#include "common/fontdata.h"
#include "common/gameinit.h"
#include "common/ghost.h"
#include "common/hud.h"
#include "common/hudassets.h"
#include "common/inputpoll.h"
#include "common/physics.h"
#include "common/race.h"
#include "common/racephases.h"
#include "common/racers.h"
#include "common/rallyconfig.h"
#include "common/render.h"
#include "common/renderbackend.h"
#include "common/replay.h"
#include "common/rumble.h"
#include "common/season.h"
#include "common/seasons.h"
#include "common/session.h"
#include "common/settings.h"
#include "common/shadowbuffers.h"
#include "common/shadowclip.h"
#include "common/snapshot.h"
#include "common/soundsource.h"
#include "common/soundsystem.h"
#include "common/system.h"
#include "common/timing.h"
#include "common/track.h"
#include "common/trackrelocation.h"
#include "common/trackrender.h"
#include "common/trackskidmarks.h"
#include "common/tricache.h"
#include "common/util.h"
#include "common/vehiclecount.h"
#include "common/vehicledeform.h"
#include "common/vehiclerender.h"
#include "common/visgrid.h"
#include "common/weather.h"

f32 (*g_apVehicleModelMatrix[MAX_VEHICLES])[4];
f32 (*g_apVehicleLightMatrix[MAX_VEHICLES])[4];

u16 g_awCameraRayNearbyInstances[RAY_INSTANCES];
u8 g_abRspDramStackBuffer[SP_DRAM_STACK_SIZE8 + 16];
alignas(8)
    DisplayListCommand g_abDisplayListMemory[2 * DISPLAY_LIST_COMMANDS + DISPLAY_LIST_RESERVED];
u8 g_aabInputRecordings[INPUT_RECORDERS][0xF000];
u8 g_abPlaybackRecording[RECORDING_HEADER_SIZE];

SoundSourceTracking g_ClosestSoundSource;
OSMesgQueue g_DmaMessageQueue;
TrackHeader g_LoadedTrack;
OSMesgQueue g_RdpDoneMsgQueue;
OSMesgQueue g_RspDoneMsgQueue;
OSMesgQueue g_SiMesgQueue;
OSMesgQueue g_ViRetraceMesgQueue;
CollisionTriangleNode g_aCollisionTriangleNodes[COLLISION_TRIANGLE_NODES];
DisplayListBlock g_aDisplayListBlocks[2][FRAME_DISPLAY_LIST_BLOCKS + 1];
OSTask g_aGfxTask[2];
Hilite g_aHiliteBlocks[2][FRAME_HILITE_BLOCKS + 1];
OSIoMesg g_aIoMessagePool[32];
LookAt g_aLightBlocks[2][FRAME_LIGHT_BLOCKS + 1];
Particle g_aParticlePool[PARTICLES];
ShadowClipVertex g_aShadowClipVertexPool[SHADOW_CLIP_VERTEX_COUNT];
DisplayListCommand g_aShadowDisplayLists[2][SHADOW_COMMANDS];
ConvertedModelVertex g_aShadowVertices[2][SHADOW_VERTICES];
s32 g_aTimingMarkerCounts[2][TIMING_BARS];
TimingMarker g_aTimingMarkers[2][TIMING_BARS][TIMING_MARKERS];
ConvertedModelVertex g_aTrackQuadVertices[2][TRACK_QUAD_VERTICES];
TrackTriangle g_aTriCacheTriangles[TRI_CACHE_SLOTS][TRI_CACHE_TRIANGLES];
Lights1 g_aVehicleLights[MAX_VEHICLES];
VisibleCell g_aVisibleCellTuples[VISIBLE_CELLS + 1];
f32 g_aafSavedResultLapTimes[SAVED_RESULTS][12];
u8 g_abPausedClockState[PAUSED_CLOCK_STATE_SIZE];
u8 g_abPrecipitationTextures[WEATHER_VIEWS][RAIN_STREAK_TEXTURE_SIDE * RAIN_STREAK_TEXTURE_SIDE *
                                            RAIN_STREAK_TEXTURE_BYTES_PER_TEXEL];
s32 g_adwFadeRectLeftPerBuffer[2];
s32 g_adwFadeRectRightPerBuffer[2];
unsigned g_adwLargeFontGlyphTexturesA[27];
unsigned g_adwLargeFontGlyphTexturesB[26];
s32 g_adwSavedResultBestLaps[SAVED_RESULTS];
s32 g_adwSavedResultLaps[SAVED_RESULTS];
s32 g_adwSavedResultPositions[SAVED_RESULTS];
unsigned g_adwSmallFontGlyphTexturesA[27];
unsigned g_adwSmallFontGlyphTexturesB[26];
u32 g_adwTriCacheLruStamp[TRI_CACHE_SLOTS];
s32 g_adwVehicleInViewRange[MAX_VEHICLES];
s32 g_adwVisibleGridColMaxRow[VISIBLE_GRID_SIZE];
s32 g_adwVisibleGridColMinRow[VISIBLE_GRID_SIZE];
s32 g_adwVisibleGridRowMaxCol[VISIBLE_GRID_SIZE];
s32 g_adwVisibleGridRowMinCol[VISIBLE_GRID_SIZE];
s32 g_adwWaterfallInstances[16];
f32 g_afBillboardMatrix[4][4];
f32 g_afCameraRollMatrix[4][4];
f32 g_afMvpMatrix[4][4];
f32 g_afNearbyVehicleDot[MAX_VEHICLES];
f32 g_afSavedResultBestLapTimes[SAVED_RESULTS];
f32 g_afSavedResultTimes[SAVED_RESULTS];
s32 g_anNearbyVehicleIdx[MAX_VEHICLES];
s32 g_anVehicleLod[MAX_VEHICLES];
Vec3f g_avLightningBolt[LIGHTNING_BOLT_POINTS];
Vec3f g_avRainPrevCameraPos[WEATHER_VIEWS];
Vec3f g_avRainVelocity[WEATHER_VIEWS];
s16 g_awRainParticlePos[WEATHER_VIEWS][RAIN_PARTICLE_SLOTS][3];
s16 g_awRumbleActive[RUMBLE_CHANNELS];
s16 g_awRumbleCounter[RUMBLE_CHANNELS];
s16 g_awRumbleFramesLeft[RUMBLE_CHANNELS];
s16 g_awRumbleMotorOn[RUMBLE_CHANNELS];
s16 g_awRumbleOffFrames[RUMBLE_CHANNELS];
s16 g_awRumbleOnFrames[RUMBLE_CHANNELS];
u16 g_awTriCacheSectorId[TRI_CACHE_SLOTS];
u16 g_awTriCacheTriCount[TRI_CACHE_SLOTS];
f32 g_mtxVehicleModelScaled[4][4];
char g_szHudSpeedTextBuf[16];
Vec3f g_vShadowForward;
Vec3f g_vTrackLightDirection;
Vec3f g_vTrackRoutePoint;
Vec3f g_vVehicleVisibilityDelta;
Vec3f g_vWindVelocity;
Vec3f g_vecVehicleHiliteDir;
Vec3f g_vecVehicleLightDir;

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
s16 g_wVehicleDeformMaxDent = 0x100;
u32 g_dwCurrentWeather;
s32 g_dwNumActiveVehicles = 2;
s32 g_dwSavedMusicVolume;
s32 g_dwSavedSfxVolume;
s32 g_dwUnk_InitGameAndShowMenus_106c664c;
s32 g_dwUnk_InitGameAndShowMenus_10b4df28;
int g_bRaceStartReached;
int g_dwNetworkPlayMode;
const char g_szEmptyUiString[1] = {0};
int g_dwNullStubEnable = 1;
RallyConfig g_GlobalRallyConfig;

// The ghost recordings and the seasons.
unsigned g_dwGhostRecordingSize;
GhostRecordingInfo g_GhostRecordingInfo;
u8 g_abSaveFileReadBuffer[SEASON_SAVE_PAYLOAD_SIZE];
int g_dwSaveSlotPlayerIndex;
Season g_SeasonTable[SEASONS] = {
    {0xdf, 2, 10, {1, 8}, 0x0081, {{2, 0}, {4, 0}, {0, 0}, {0, 0}}},
    {0xe0, 3, 15, {0, 7}, 0x0050, {{0, 0}, {2, 0}, {4, 2}, {0, 0}}},
    {0xe1, 4, 20, {4, 6}, 0x000c, {{4, 1}, {0, 1}, {2, 2}, {1, 0}}},
    {0xe2, 4, 20, {3, 2}, 0x0020, {{4, 3}, {1, 1}, {2, 4}, {0, 2}}},
    {0xe3, 4, 20, {2, 5}, 0x0020, {{2, 3}, {0, 4}, {10, 3}, {1, 2}}},
    {0xe4, 4, 20, {5, 5}, 0x0020, {{6, 1}, {2, 2}, {1, 4}, {10, 2}}},
};

// The input of the players.
int g_bNetGamePaused;
int g_bNetReadyToStartSent;
unsigned g_dwInputButtonMask;

// The sound and the music.
const int g_adwBgmVolumeLevels[VOLUME_LEVEL_COUNT] = {
    0x00, 0x1c, 0x38, 0x55, 0x71, 0x8d, 0xaa, 0xc6, 0xe2, 0xff};
const int g_adwSfxVolumeLevels[VOLUME_LEVEL_COUNT] = {
    0x00, 0x55, 0x78, 0x93, 0xaa, 0xbe, 0xd0, 0xe1, 0xf0, 0xff};
SoundChannelState g_aSoundChannelRequested[SOUND_CHANNEL_COUNT];
int g_bAltLoopingSoundSet;

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
decltype(g_pfnAliasTextureEntrySurface) g_pfnAliasTextureEntrySurface;
decltype(g_pfnCreateTextureFromN64Image) g_pfnCreateTextureFromN64Image;
decltype(g_pfnDecodeN64TextureToPixelBuffer) g_pfnDecodeN64TextureToPixelBuffer;
decltype(g_pfnInitializeTextureSubsystem) g_pfnInitializeTextureSubsystem;
decltype(g_pfnRenderBackendInit) g_pfnRenderBackendInit;
decltype(g_pfnRenderBackendShutdown) g_pfnRenderBackendShutdown;
decltype(g_pfnRenderBackendSubmitDisplayList) g_pfnRenderBackendSubmitDisplayList;
decltype(g_pfnRenderBackendUpdateFog) g_pfnRenderBackendUpdateFog;
decltype(g_pfnShutdownTextureSubsystem) g_pfnShutdownTextureSubsystem;
