#include <common/camera.h>
#include <common/effects.h>
#include <common/fade.h>
#include <common/hud.h>
#include <common/physics.h>
#include <common/race.h>
#include <common/racers.h>
#include <common/render.h>
#include <common/replay.h>
#include <common/rumble.h>
#include <common/shadowbuffers.h>
#include <common/soundsource.h>
#include <common/system.h>
#include <common/timing.h>
#include <common/track.h>
#include <common/trackrelocation.h>
#include <common/trackrender.h>
#include <common/tricache.h>
#include <common/util.h>
#include <common/vehiclerender.h>
#include <common/visgrid.h>
#include <common/weather.h>
#include <win32/fontdata.h>
#include <win32/framestatistics.h>

#include "shadowclip.h"

f32 (*g_apVehicleModelMatrix[MAX_VEHICLES])[4];
f32 (*g_apVehicleLightMatrix[MAX_VEHICLES])[4];

u16 g_awCameraRayNearbyInstances[RAY_INSTANCES];
u8 g_abRspDramStackBuffer[SP_DRAM_STACK_SIZE8 + 16];
alignas(8)
    DisplayListCommand g_abDisplayListMemory[2 * DISPLAY_LIST_COMMANDS + DISPLAY_LIST_RESERVED];
u8 g_aabInputRecordings[INPUT_RECORDERS][0xF000];
u8 g_abPlaybackRecording[RECORDING_HEADER_SIZE];
DWORD g_adwGameFpsFrameTimeSamples[GAME_FPS_FRAME_TIME_SAMPLE_COUNT];

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
u8 g_abTrackGeometryBuffer[TRACK_MAX_SIZE];
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
