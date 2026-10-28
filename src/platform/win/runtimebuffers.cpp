#include <platform/win/frontend.h>
#include <platform/win/netstream.h>
#include <platform/win/render.h>
#include <platform/win/snapshot.h>

unsigned char g_abAlphaRampPixelStrip[64];
unsigned char g_abAlphaRampRgba[16][4];
unsigned char g_abDaySkyPalette[32];
unsigned char g_abDaySkyTexture[2048];
int g_abFrameSnapshotInUse[FRAME_SNAPSHOT_COUNT];
unsigned char g_abNightSkyPalette[32];
unsigned char g_abNightSkyTexture[2048];
int g_adwKeyBindingDuplicateFlags[KEY_CONFIG_ENTRY_COUNT];
float g_afKeyframeAccumMax[7];
int g_anSkidMarkHead[SKID_MARK_TRACK_COUNT];
int g_anSkidMarkTail[SKID_MARK_TRACK_COUNT];
