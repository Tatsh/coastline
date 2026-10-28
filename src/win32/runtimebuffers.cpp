#include <win32/boot/app.h>
#include <win32/directplay.h>
#if TGR_PLATFORM != TGR_PLATFORM_SDL
#include <win32/frontend.h>
#else
#include <win32/frontendstate.h>
#endif
#include <win32/ghost.h>
#include <win32/netstream.h>
#include <win32/render.h>
#include <win32/settings.h>
#include <win32/snapshot.h>

unsigned char g_abAlphaRampPixelStrip[64];
unsigned char g_abAlphaRampRgba[16][4];
unsigned char g_abDaySkyPalette[32];
unsigned char g_abDaySkyTexture[2048];
KeyframeState g_LastSentKeyframe;
int g_abFrameSnapshotInUse[FRAME_SNAPSHOT_COUNT];
unsigned char g_abNightSkyPalette[32];
unsigned char g_abNightSkyTexture[2048];
int g_adwKeyBindingDuplicateFlags[KEY_CONFIG_ENTRY_COUNT];
float g_afKeyframeAccumMax[7];
int g_anSkidMarkHead[SKID_MARK_TRACK_COUNT];
int g_anSkidMarkTail[SKID_MARK_TRACK_COUNT];

char g_szInstallDirectory[MAX_PATH + 1];
char g_szConfigFilePath[MAX_PATH + sizeof("BossRally.cfg")];
char g_szBossRallyIniPath[MAX_PATH + sizeof("BossRally.ini")];
char g_szModemPhoneNumber[MAX_PATH];
char g_szModemDeviceName[MAX_PATH];
char g_szNetworkChatText[201];
char g_szSessionName[80];
