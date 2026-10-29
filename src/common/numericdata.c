#include <common/audio.h>
#include <common/camera.h>
#include <common/fade.h>
#include <common/game.h>
#include <common/hud.h>
#include <common/input.h>
#include <common/laps.h>
#include <common/model.h>
#include <common/race.h>
#include <common/raceprogress.h>
#include <common/render.h>
#include <common/rumble.h>
#include <common/text.h>
#include <common/track.h>
#include <common/trackselect.h>
#include <common/vehiclecount.h>
#include <common/vehiclerender.h>
#include <common/weather.h>

#include "common/fontdata.h"
#include "common/frameclock.h"

int g_adwHudFont0GlyphOffsets[28] = {0,   26,  52,  78,  104, 130, 156, 182, 208, 234,
                                     260, 274, 290, 315, 332, 381, 407, 437, 462, 488,
                                     505, 522, 539, 565, 596, 626, 648, 674};

int g_adwHudFont1GlyphOffsets[27] = {0,   29,  56,  82,  109, 134, 160, 185, 213,
                                     229, 248, 276, 298, 332, 360, 386, 413, 439,
                                     466, 492, 517, 545, 575, 610, 641, 671, 697};

int g_adwHudFont2GlyphOffsets[28] = {0,   14,  28,  42,  56,  70,  84,  98,  112, 126,
                                     140, 148, 156, 172, 181, 207, 222, 238, 252, 266,
                                     276, 285, 296, 310, 327, 344, 355, 371};

int g_adwHudFont3GlyphOffsets[27] = {0,   16,  31,  46,  61,  75,  89,  103, 119,
                                     128, 139, 154, 166, 184, 199, 214, 229, 243,
                                     258, 272, 286, 301, 317, 337, 353, 370, 384};

u32 g_adwModelShadeColors[4] = {1077952512u, 2155905024u, 3233857536u, 4294967040u};

f32 g_afStartLightThresholds[START_LIGHT_STEPS] = {2.15f, 1.5f, 0.85f, 0.2f, 0.0f};

s32 g_anRaceProgressStringIds[RACER_SLOTS] = {269, 270, 271, 272, 273, 274, 275, 276, 277, 278,
                                              279, 280, 281, 282, 283, 284, 285, 286, 287, 288};

u16 g_awVehicleShadePalette[16] = {1u,
                                   4229u,
                                   8457u,
                                   12685u,
                                   16913u,
                                   21141u,
                                   25369u,
                                   29597u,
                                   35939u,
                                   40167u,
                                   44395u,
                                   48623u,
                                   52851u,
                                   57079u,
                                   61307u,
                                   65535u};

s32 g_bBilinearFiltering = 1;

s32 g_bGbiClipping = 1;

u8 g_bMusicVolume = 255u;

u8 g_bSfxFadeVolume = 255u;

s32 g_bShowHudHelpText = 1;

s32 g_bSmoothShading = 1;

s32 g_bTrackTextureLod = 1;

s32 g_bVehicleEnvMapLinearTexGen = 1;

int g_dwGameFpsFrameCountdown = -1;

int g_dwGameFpsSampleFrames = 5;

s32 g_dwGameMode = 1;

s32 g_dwLightningFlashCounter = -1;

s32 g_dwNumTracks = 14;

s32 g_dwNumVehicles = 1;

s32 g_dwNumViewports = 1;

s32 g_dwRearViewMirrorMode = 3;

u32 g_dwSelectedTrackId = 2u;

s32 g_dwStreamDmaPrevSegment = -1;

s32 g_dwStreamedSnowMode = -1;

s32 g_dwTextCustomColorB = 255;

s32 g_dwTextCustomColorG = 255;

s32 g_dwTextCustomColorR = 255;

s32 g_dwTotalLaps = 1;

f32 g_fCameraFovRadians = 0.5235988f;

f32 g_fCameraProjectionScale = 4e+02f;

f32 g_fWindStrength = 0.5f;

f32 g_mtxIdentity[4][4] = {{1.0f, 0.0f, 0.0f, 0.0f},
                           {0.0f, 1.0f, 0.0f, 0.0f},
                           {0.0f, 0.0f, 1.0f, 0.0f},
                           {0.0f, 0.0f, 0.0f, 1.0f}};

s16 g_wSerialInterfaceIdle = 1;
