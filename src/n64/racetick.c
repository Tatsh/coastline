/* clang-format off */
#include "n64/racetick-aliases.h"
#include "n64/race-internal.h"
/* racetick.c -- the race: one frame of it, from the first-frame setup
 * (players, cars, ghosts, the track's special objects) through the start
 * lights, every view drawn, the pause menu, to the fade that hands over to
 * the next game mode.
 *
 * ProcessRacingScreen was compiled without IDO's global optimiser (it is over the
 * -Olimit threshold), so its locals live in the frame and its `register`
 * variables in s0-s6 and f20-f24: its declarations reproduce that frame slot
 * for slot, which the whole-image run needs (the frame is live across every
 * retrace).  Paths no box script reaches yet: two players, the instant
 * replay, the time-attack ghost, the airplane, the rear-view mirror and the
 * season end.
 */
/* n64-cflags: -O2 */

/* -- declarations -- */
typedef struct N64CarEnt {       /* a race entity (0x78 bytes): the cars, then the rest */
  char pad00[0x60];
  N64Car *car;                   /* 0x60  0 for an entity that is not a car */
  int x64;
  unsigned int flags;           /* 0x68  bit 0: held at the start, bit 1: finished */
  char pad6c[0x78 - 0x6C];
} N64CarEnt;
extern N64CarEnt g_aRacerSlots[];

typedef struct N64ViewRect {     /* a player's view (0x14 bytes) */
  int x; int y; int w; int h;
  int car;                      /* 0x10  the car it follows */
} N64ViewRect;
extern N64ViewRect g_aViewportRects[2];

typedef struct N64RaceLight {    /* one step of the start-light script */
  int state;
  float secs;
} N64RaceLight;
extern N64RaceLight g_aRacePhaseTable[];
extern float g_afStartLightThresholds[];      /* countdown beep times (the state-2 timer runs down) */

typedef struct N64Special {      /* one of the track's special objects (12 bytes) */
  int obj;                      /* 0x00  its track object */
  int arg;                      /* 0x04  a count, a path, or an angle's bits */
  unsigned char kind;           /* 0x08  0-2 spinner (z, x, y axis), 3 airplane, 4/5 its
                                 *       left/right path, 6 its trigger, 7 waterfall */
  char pad09[3];
} N64Special;

typedef struct N64TrackObj {     /* a track object (0x54 bytes) */
  N64Vec3 x; float wx;           /* 0x00  its matrix, rows */
  N64Vec3 y; float wy;           /* 0x10 */
  N64Vec3 z; float wz;           /* 0x20 */
  N64Vec3 pos; float wpos;       /* 0x30 */
  char pad40[0x4C - 0x40];
  unsigned short flags;         /* 0x4C */
  char pad4e[0x54 - 0x4E];
} N64TrackObj;

typedef struct N64TrackHdr {     /* the loaded track's header */
  char pad00[0x60];
  N64TrackObj *objs;             /* 0x60  its objects */
  char pad64[0x164 - 0x64];
  N64Special specials[16];       /* 0x164 */
  int nSpecials;                /* 0x224 */
} N64TrackHdr;
extern N64TrackHdr g_LoadedTrack;

typedef struct N64TimeLimit {    /* time allowed per difficulty and car class (0x1C bytes) */
  float secs;
  char pad04[0x18];
} N64TimeLimit;
typedef struct N64TrackTimes {
  char pad00[0x44];
  N64TimeLimit limit[4][3];      /* 0x44  [class][difficulty] */
} N64TrackTimes;
extern N64TrackTimes *g_apTrackMenuItems[];  /* per track (the same records as g_aTrackDescriptors) */

typedef struct N64Tip {          /* a demo caption (0x20 bytes) */
  char *title;
  float secs;
  char *lines[6];
} N64Tip;
extern N64Tip g_aCreditCaptions[];

typedef struct N64RaceSnd {      /* 0x18 bytes, six of them */
  void *p;
  int x4;
  long long x8;
  int x10;
  int x14;
} N64RaceSnd;
extern N64RaceSnd g_aSoundEffectChannels[6];
extern int g_dwSoundEffectEchoOffset;
extern char g_SilenceSampleData[];

typedef struct { char raw[0xdf88]; } N64CarModelBuf;
extern N64CarModelBuf g_aVehicleModels[];

extern int g_dwRaceInitialized;          /* the race has been set up */
extern int g_dwKeepScreenBlanked;          /* setting up */
extern int g_dwNumLocalPlayers;          /* human players */
extern int g_dwRacePauseState;          /* paused */
extern int g_dwPauseMenuSelection;          /* the pause menu's row */
extern int g_dwGameMode;          /* the game mode: 0 championship, 1 arcade, 2 time attack,
                                 * 4 attract demo, 5 season end */
extern int g_dwRaceSessionType;          /* which demo */
extern int g_bReplayAfterRace;
extern int gInstantReplayActive;          /* this race is the instant replay */
extern int g_dwGhostRecordingSize;          /* bytes in the saved time-attack ghost */
extern int g_dwSavedResultsValid;
extern int g_dwSaveAvailable;          /* a new time-attack record */
extern int g_dwWaterfallCount;          /* waterfalls */
extern int g_dwRaceExitState;          /* the race is fading out */
extern int g_dwTotalLaps;
extern int g_bTrackCollisionFrameToggle;          /* flips every frame */
extern int g_dwNumVehicles;          /* entries in g_aRacerSlots */
extern int g_dwNumActiveVehicles;          /* cars in the race */
extern int g_dwReplayCameraSwitchTimer;
extern int g_dwSelectedTrackId;          /* the track */
extern int g_dwCurrentWeather;
extern int g_dwBestReplayLength;          /* the best recording's length */
extern int g_dwBestReplayPlayer;          /* the player whose recording is the best */
extern unsigned char *g_apReplayGhostPtrs[2];    /* the recordings the replay plays */
extern int g_adwReplayGhostSizes[2];       /* and their lengths */
extern unsigned char g_abGhostRecording[];      /* the saved time-attack ghost: track, kind, ... */
extern int g_dwTrackMirrored;          /* the track is mirrored */
extern int g_pStartLightsModel;
extern char g_aDmaWorkBuffers[];
extern int g_dwStartLightsAnimation;
extern int g_bStartLightsVisible;
float g_fStartLightsDrop = 0.0f;
typedef struct N64Airplane {     /* the airplane some tracks fly over the course */
  int obj;                      /* 0x00  its track object, 0 for none */
  int trigger;                  /* 0x04  the trigger that sends it */
  int lap;                      /* 0x08  on this lap */
  int sent;                     /* 0x0C  it has been sent */
  int point;                    /* 0x10  the path point it is past */
  int count;                    /* 0x14  path points */
  float scale;                  /* 0x18 */
  int left;                     /* 0x1C  its path, left and right edges (N64Vec3 *) */
  int right;                    /* 0x20 */
  float dist;                   /* 0x24  distance along the current path segment */
  float segLen;                 /* 0x28  the segment's length */
  N64Vec3 prev;                  /* 0x2C  the previous, current and next segment directions */
  N64Vec3 dir;                   /* 0x38 */
  N64Vec3 next;                  /* 0x44 */
} N64Airplane;
N64Airplane g_dwAirplaneInstance = { 0 };
#define PLANE (&g_dwAirplaneInstance)
typedef struct N64Waterfall {    /* a waterfall: its track object */
  int obj;
} N64Waterfall;
extern N64Waterfall g_adwWaterfallInstances[];
extern int D_8031B2D8;
extern int D_8031B2EC;
extern int g_dwNumViewports;          /* views on screen */
extern int g_dwRacePhase;          /* the start-light state */
extern float g_fRacePhaseTimer;        /* its timer */
extern int g_dwStartLightsStep;          /* countdown beeps played */
extern int g_dwRacePhaseIndex;          /* the step of the light script */
extern int g_dwRaceFinishOrderCounter;
extern int g_dwNightMode;
extern int g_dwSnowMode;
extern int g_dwRainMode;
extern int g_dwRearViewMirrorMode;          /* the rear-view mirror size, 0 for none */
extern int g_dwSkyboxDisabled;
extern int g_bShowHudHelpText;          /* the HUD is on */
extern int g_dwHudDebugLevel;
extern int g_dwScreenWidth;          /* screen width */
extern int g_dwScreenHeight;          /* screen height */
extern float g_fCameraProjectionScale;        /* far plane */
extern float g_fFrameDeltaTime;        /* seconds this frame */
extern int g_dwCurrentSplitScreenIndex;          /* the view being drawn */
extern N64Car *g_pActiveCameraVehicle;       /* its car */
extern N64CarCam *g_pActiveCamera;    /* its camera */
extern int g_fCameraRayGroundDistance;
extern int g_dwCameraRayGroundSurface;
extern int g_dwCameraRayHitCount;
extern int g_dwCameraRayNearbyCount;
extern int g_awCameraRayHitInstances;
extern int g_awCameraRayNearbyInstances;
extern int g_bRaceScreenActive;
extern int g_dwVehicleViewModeIndex;
extern int g_dwScreenClearFrames;          /* frames to clear the whole screen */
extern int g_dwPrevViewportCount;          /* views last frame */
extern float g_fRaceCaptionTimer;        /* the demo caption's clock */
extern int g_dwRaceCaptionIndex;          /* the demo caption shown */
extern int g_bDemoInterrupted;          /* the demo was skipped */
extern float g_afMvpMatrix[4][4];
extern char g_szFormatBuffer[];
extern int g_OptionSettings;          /* music volume */
extern int g_OptionSettingsSfxLevel;          /* effects volume */
extern unsigned char g_bSaveLoadEntryPoint;
extern short g_wRaceScreenLoopExited;
extern char _ghostRecordingSegmentRomStart[];       /* the three demo recordings in ROM */
extern char _creditsRecordingSegmentRomStart[];
extern char _endingRecordingSegmentRomStart[];
extern char _startLightsSegmentRomStart[];
extern char _startLightsSegmentRomEnd[];

void osViBlack(int black);
#if TGR_REGION_JPN
void BlankDisplay(void);
#define RACE_BLANK() BlankDisplay()
#elif TGR_REGION_EUR
void BlankDisplay(void);
#define RACE_BLANK() BlankDisplay()
#else
#define RACE_BLANK() osViBlack(1)
#endif
void osSyncPrintf(char *fmt, ...);
void *memcpy(void *dst, void *src, unsigned int n);
int sprintf(char *buf, char *fmt, ...);
void guRotateF(float mf[4][4], float a, float x, float y, float z);
void guMtxCatF(float m[4][4], float n[4][4], float r[4][4]);
void ApplyWeather(int kind);
void LoadAndPlayTrackBgm(void);
void UpdateRaceAnimations(void);
void RenderStartLights(void);
void StartEndingCredits(void);
void CalculateScrollPositions(void);
void ScrollTextUp(void);
void DrawScrollingText(void);
void SaveRaceResultsToSlots(void);
void RestoreRaceResultsFromSlots(void);
void SetTrackFromSeason(void);
void ProcessSplashScreens(void);
void FormatLapTime(char *psz, float t);
void ProcessRaceResultsScreen(void);
void ProcessMainMenu(void);
void IncreaseBgmVolume(void);
void DecreaseBgmVolume(void);
void IncreaseSfxVolume(void);
void DecreaseSfxVolume(void);
void ProcessOptionsMenu(void);
void ProcessDataSaveLoadMenu(void);
void InitAllRumblePaks(void);
void StubEmpty(void);
void ClearDepthBuffer(void);
void ClearZBufferRect(int x, int y, int w, int h);
void ClearScreen(int r, int g, int b);
void DrawFilledRect(int x, int y, int w, int h, int r, int g, int b);
void SetupFog(void);
void SetupWeatherLights(void);
void StartLowResFrame(void);
#ifdef ENABLE_PATCHES
extern unsigned int osMemSize;
#endif
void SetScissorRect(int x, int y, int w, int h);
void SetViewport(int x, int y, int w, int h, int scissor);
void SetupViewportAndScissor(int x, int y, int w, int h, int scissor);
void DrawRdpOutlineRect(int x, int y, int w, int h);
void DrawRdpBorderRect(int x, int y, int w, int h);
void ClearFramebuffersHighRes(void);
void StreamAnimatedTrackTextures(void);
void FlushDisplayList(void);
void SetupFrustumCorners(N64CarCam *cam, float fov, float far, float w, float h);
void SetupCameraMatrix(float m[4][4], float fov, float far, float w, float h);
void SetupMenuProjection(float w, float h);
void ResetFrameClock(void);
void UpdateFrameTiming(void);
void SetNextScreenCallback(int hook);
void SetCurrentScreenCallback(int mode);
int DmaReadAssetSize(int rom);
void DecompressFromRom(unsigned char *dst, char *rom, int s);
void SetMaterialColor(int model, unsigned int r, unsigned int g, int b);
void PlayModelAnimationsOnce(int anim);
int DmaReadAndFixupTrackModel(char *dst, char *rom, char *romEnd);
void LoadTrackGeometryFromRom(int track);
int CastRayAgainstTrack(int a, int b, float *at, int *c, int *d, int *e, int *f, int *g, int *h);
void InstallVehicleModel(N64Car *car, int slot, int kind);
#if TGR_REGION_JPN
void LoadAndActivateTrackWithPaintColor(N64Car *car, int slot, int kind);
#endif
#if TGR_REGION_EUR || TGR_REGION_JPN
void RestoreReplayKeyframeState(void);
#endif
#if TGR_REGION_EUR
void DrawScreenFadeOverlay(float level);
#endif
#if TGR_REGION_EUR
extern int eur_D_8028DEC0;              /* demo frames played */
extern int eur_D_80272798;              /* the FINISH! cheat */
#define DEMO_FRAMES eur_D_8028DEC0
#define CHEAT_FINISH eur_D_80272798
#endif
#if TGR_REGION_JPN
extern int jpn_D_8028D8F0;              /* demo frames played */
extern int jpn_D_80272248;              /* the FINISH! cheat */
#define DEMO_FRAMES jpn_D_8028D8F0
#define CHEAT_FINISH jpn_D_80272248
#endif
void ResetHeadlightGlare(void);
void RenderHeadlightGlare(void);
void SetFadeTarget(float level, float seconds);
void FadeSfxTo(float level, float seconds);
void FadeMusicTo(float level, float seconds);
int IsFadingOut(void);
int IsFadeComplete(void);
void ApplyScreenFade(void);
void DrawScreenFadeOverlay(float level);
void UpdateFadeTransitions(void);
void Vec3fCross(N64Vec3 *pOut, N64Vec3 *pA, N64Vec3 *pB);
void Vec3fScaleInPlace(N64Vec3 *pV, float s);
void Vec3fNormalize(N64Vec3 *pV);
void Vec3fSubtract(N64Vec3 *pOut, N64Vec3 *pA, N64Vec3 *pB);
void Vec3fLerp(N64Vec3 *pOut, N64Vec3 *pA, N64Vec3 *pB, float t);
void Vec3fMidpoint(N64Vec3 *pOut, N64Vec3 *pA, N64Vec3 *pB);
float Vec3fDistance(N64Vec3 *pA, N64Vec3 *pB);
float Vec3fLength(N64Vec3 *pV);
void Mtx4x4ProjectPoint(float out[3], float v[3], float m[4][4]);
void SetRngSeed(int seed);
void ClearPerFrameVariables(void);
void ChooseVehicleCar(N64Car *car);
void UpdatePhysicsStep(int ent);
void DriveAiVehicle(int ctl);
void SetupVehicleForRaceStart(N64Car *car);
void InitRacerSlot(N64CarEnt *e);
void SortRacersByPosition(void);
void ManageAiRacerSlots(N64Car *car);
void SaveLastLapSnapshot(N64Car *car);
void RestoreRaceSnapshot(char *buf);
void SaveClockForPause(void);
void RestoreClockAfterPause(void);
void PlayBeepSound(void);
void PlayGoSound(void);
void ResetSoundSourceTracking(void);
void ResetAllSoundState(void);
void UpdateAmbientSoundChannel(void);
void UpdateAirplaneSound(int f8C, void *pPos, void *pListener);
void UpdateWaterfallSound(int f8C, void *pPos, void *pListener);
void StubSoundUpdate(int arg0);
void LoadAllSoundEffects(void);
void ClearCameraChanged(int car);
void UpdateVehicleSounds(N64Car *car);
void UpdateRacerSlotMotion(N64CarEnt *e);
void UpdateRacerSlotRaceState(N64CarEnt *e);
void BuildVisibleGrid(void);
void AddTimingMarker(int bar, int r, int g, int b, int a);
void DrawTimingBars(void);
void SetTextStyleNormal(void);
void SetTextAlignCenter(void);
void SetTextAlignLeft(void);
void SetTextAlignRight(void);
void SetTextCustomColors(int a, int b, int c, int d, int e, int f);
void SetTextSize(int font);
void DrawText(char *s, int x, int y);
void RenderSkybox(void);
void CheckVehicleVisibility(N64Car *car);
void RenderVehicleModel(N64Car *car, int a);
void RenderVehicleHeadlightBeams(N64Car *car);
void ResetShadowBuffers(void);
void RenderTrackGeometry(int pass);
void StubDashboard(void);
void DrawHudMessage(void);
void DrawSpeedAndHudOverlay(void);
void UpdatePrecipitationParticles(void);
void RenderPrecipitationEffect(void);
void InitParticleSystem(void);
void UpdateAllVehicleEffects(void);
void RenderTireSmoke(void);
void RenderExhaustAndTireSpray(void);
void TransferFlags(N64PadRec *pad, unsigned int bits);
void UpdateStickRepeat(N64PadRec *pad);
void ResetCollisionCaches(void);
/* -- end declarations -- */

#define PAD(i) ((N64PadRec *)g_aVehicles[i].pad)
#define OBJ(n) ((N64TrackObj *)((char *)g_LoadedTrack.objs + (n) * sizeof(N64TrackObj)))
#define LEFT ((N64Vec3 *)PLANE->left)
#define RIGHT ((N64Vec3 *)PLANE->right)

/* WHAT IT DOES: One frame of the race (the game mode function while a race
 * runs).  The first call sets the race up: how many cars and entities, the
 * players' recordings for the instant replay and time attack's ghost (or,
 * for the attract demo and replay, the recording to play back), each car's
 * setup, control and model, the track's special objects (spinners, the
 * airplane and its path, waterfalls) and the start-light script.  Every
 * frame then steps the start lights and the race state (countdown beeps,
 * the finish, time attack's new record and TIME UP), ticks and moves every
 * entity, flies the airplane, draws each view (and the rear-view mirror)
 * with the HUD or the replay captions, runs the pause menu, and once the
 * screen has faded out picks the next game mode: the replay, results, the
 * next race or the menus.  Europe and Japan add the intro demo (Mountain,
 * resynced every frame from the demo variables) and the FINISH! cheat;
 * Europe's demo cuts at 170 s and 42 s and dims over its last half-second.
 */
/* @implements 0x8020082C tgr ProcessRacingScreen */
void ProcessRacingScreen(void)
{
  /* declared in the order of the original's frame (first = highest slot);
   * register variables take a slot too, and get s0, s1, ... in this order */
  register int j;
  register int i;
  N64Car *car0;
  float v[3];
  register float vx;
  register float vy;
  register float vz;
  N64Car *p0;
  N64Car *p1;
  N64Car *p2;
  N64Car *p3;
  N64Car *p4;
  N64Car *p5;
  int paused;
  N64Car *car;
  short s;
  int running;
  int k50;
  int k54;
  int k58;
  N64Vec3 a;
  N64Vec3 b;
  N64Vec3 c;
  register float *d1;
  register float *s1;
  register float *d2;
  register float *s2;
  float f;
  int mx;
  int my;
  int mw;
  int mh;
  N64CarCam *saved;
  register int x;
  int y;
  int y0;
  int kb4;
  int kb8;
  int kbc;

  if (g_dwRaceInitialized == 0) {
    g_dwKeepScreenBlanked = 1;
    RACE_BLANK();
    StartLowResFrame();
#ifdef ENABLE_PATCHES
    if (osMemSize > 0x400000) ClearScreen(0, 0, 0);
#endif
    FlushDisplayList();
    RACE_BLANK();
    StartLowResFrame();
#ifdef ENABLE_PATCHES
    if (osMemSize > 0x400000) ClearScreen(0, 0, 0);
#endif
    FlushDisplayList();
    SetRngSeed(123);
    g_bReplayAfterRace = 0;
    if (g_dwGameMode != 1) {
      g_dwTotalLaps = 3;
    }
    g_bTrackCollisionFrameToggle = 0;
    switch (g_dwGameMode) {
    case 0:
      g_dwNumVehicles = 20;
      g_dwNumActiveVehicles = 3;
    players:
      if (gInstantReplayActive != 0) {
        RestoreRaceSnapshot(g_aVehicleModels[3 - g_dwBestReplayPlayer].raw);
        for (i = 0; i < 6; i++) {
          g_aSoundEffectChannels[i].x14 = 0;
          g_aSoundEffectChannels[i].x10 = 0;
        }
        for (i = 0; i < g_dwNumLocalPlayers; i++) {
          PAD(i)->ghost = g_apReplayGhostPtrs[i];
          PAD(i)->ghostLen = g_adwReplayGhostSizes[i];
          PAD(i)->ghostPos = 0;
        }
      } else {
        for (i = 0; i < g_dwNumLocalPlayers; i++) {
          PAD(i)->ghost = 0;
          PAD(i)->rec[0] = 0;
          PAD(i)->rec[1] = 0;
        }
      }
      g_bReplayAfterRace = !gInstantReplayActive;
      break;
    case 1:
      g_dwNumVehicles = 2;
      g_dwNumActiveVehicles = 2;
      goto players;
    case 5:
      g_dwNumLocalPlayers = 1;
      g_dwNumVehicles = 1;
      g_dwNumActiveVehicles = 1;
      g_bReplayAfterRace = 0;
      gInstantReplayActive = 0;
      g_aVehicles[0].xe58 = 1;
      g_aVehicles[0].cam = &g_aVehicles[0].cams[1];
      g_dwReplayCameraSwitchTimer = 0;
      break;
    case 4:
      g_dwCurrentWeather = 1;
      g_dwNumLocalPlayers = 1;
      g_dwNumVehicles = 1;
      g_dwNumActiveVehicles = 1;
      g_bReplayAfterRace = 0;
      g_aVehicles[0].xe58 = 0;
      PAD(0)->ghost = (unsigned char *)0x80197000;
      switch (g_dwRaceSessionType) {
      case 0:
        PAD(0)->ghostLen = DmaReadAssetSize((int)_ghostRecordingSegmentRomStart);
        DecompressFromRom(PAD(0)->ghost, _ghostRecordingSegmentRomStart, 0);
        break;
      case 1:
        PAD(0)->ghostLen = DmaReadAssetSize((int)_creditsRecordingSegmentRomStart);
        DecompressFromRom(PAD(0)->ghost, _creditsRecordingSegmentRomStart, 0);
        break;
      case 2:
        CalculateScrollPositions();
        PAD(0)->ghostLen = DmaReadAssetSize((int)_endingRecordingSegmentRomStart);
        DecompressFromRom(PAD(0)->ghost, _endingRecordingSegmentRomStart, 0);
        break;
      }
      PAD(0)->ghostPos = 8;
      PAD(0)->rec[0] = 0;
      PAD(0)->rec[1] = 0;
      car0 = &g_aVehicles[0];
      car0->colour[0] = 0xFF;
      car0->colour[1] = 0xFF;
      car0->colour[2] = 0xFF;
      g_dwSelectedTrackId = PAD(0)->ghost[0];
      g_aVehicles[0].kind = g_aVehicles[0].x2058 = PAD(0)->ghost[1];
      g_aVehicles[0].xe68 = PAD(0)->ghost[2];
      g_aVehicles[0].xe6c = PAD(0)->ghost[3];
      g_aVehicles[0].xe60 = PAD(0)->ghost[4];
      g_aVehicles[0].xe64 = PAD(0)->ghost[5];
      PAD(0)->x25 = PAD(0)->ghost[6];
      break;
    case 2:
      if (g_dwSelectedTrackId == 3 || g_dwSelectedTrackId == 8) {
        g_dwCurrentWeather = 1;
      } else {
        g_dwCurrentWeather = 0;
      }
      g_dwNumActiveVehicles = g_dwNumVehicles = g_dwNumLocalPlayers + 1;
      g_bReplayAfterRace = 0;
      g_dwSaveAvailable = 0;
      PAD(g_dwNumLocalPlayers)->ghost = (unsigned char *)0x80197000;
      osSyncPrintf("savedRecordingLength = %d\n", g_dwGhostRecordingSize);
      memcpy(PAD(g_dwNumLocalPlayers)->ghost, g_abGhostRecording, g_dwGhostRecordingSize);
      g_aVehicles[g_dwNumLocalPlayers].kind = g_aVehicles[g_dwNumLocalPlayers].x2058 = PAD(g_dwNumLocalPlayers)->ghost[1];
      if (PAD(g_dwNumLocalPlayers)->ghost[0] != g_dwSelectedTrackId) {
        osSyncPrintf("WRONG TRACK (%d!=%d)\n", PAD(g_dwNumLocalPlayers)->ghost[0], g_dwSelectedTrackId);
        PAD(g_dwNumLocalPlayers)->ghost = 0;
      } else {
        osSyncPrintf("TRACK=%d\n", PAD(g_dwNumLocalPlayers)->ghost[0]);
        PAD(g_dwNumLocalPlayers)->ghostPos = 8;
        osSyncPrintf("PLAYLIMIT=%d\n", g_dwGhostRecordingSize);
        PAD(g_dwNumLocalPlayers)->ghostLen = g_dwGhostRecordingSize;
      }
      for (i = 0; i < g_dwNumLocalPlayers; i++) {
        PAD(i)->rec[i] = (unsigned char *)(0x801A6000 + i * 0xF000);
        PAD(i)->rec[i][0] = g_dwSelectedTrackId;
        PAD(i)->rec[i][1] = g_aVehicles[i].kind;
        PAD(i)->rec[i][2] = g_aVehicles[i].season->xd4;
        PAD(i)->rec[i][3] = g_aVehicles[i].season->xd8;
        PAD(i)->rec[i][4] = g_aVehicles[i].season->xdc;
        PAD(i)->rec[i][5] = g_aVehicles[i].season->xe0;
        PAD(i)->rec[i][6] = g_aVehicles[i].season->xe4;
        PAD(i)->recLen[i] = 8;
        PAD(i)->recKeep[i] = 0xDE5C;
      }
      break;
    default:
      g_dwNumVehicles = g_dwNumLocalPlayers;
      g_dwNumActiveVehicles = g_dwNumLocalPlayers;
      g_bReplayAfterRace = 0;
      break;
    }
    if (g_dwGameMode == 5) {
      g_dwTrackMirrored = 0;
    } else {
      g_dwTrackMirrored = !!(((MenuItem *)g_apTrackMenuItems[g_dwSelectedTrackId])->flags & 0x10);
    }
    if (g_dwGameMode != 4 && gInstantReplayActive == 0) {
      InitAllRumblePaks();
      LoadAndPlayTrackBgm();
    }
    LoadAllSoundEffects();
    LoadTrackGeometryFromRom(g_dwGameMode == 5 ? 10 : g_dwSelectedTrackId);
    if (gInstantReplayActive == 0) {
      PLANE->sent = 0;
      PLANE->trigger = 0;
      PLANE->obj = 0;
      PLANE->right = 0;
      PLANE->left = 0;
      PLANE->dist = 0.0f;
      PLANE->segLen = 0.0f;
      PLANE->point = 0;
      PLANE->count = 0;
      osSyncPrintf("specials: %d\n", g_LoadedTrack.nSpecials);
      g_dwWaterfallCount = 0;
      for (i = 0; i < g_LoadedTrack.nSpecials; i++) {
        switch (g_LoadedTrack.specials[i].kind) {
        case 4:
          PLANE->count = g_LoadedTrack.specials[i].arg;
          osSyncPrintf("airplanePathLeft = %08x\n",
                       PLANE->left = g_LoadedTrack.specials[i].obj);
          break;
        case 5:
          PLANE->right = g_LoadedTrack.specials[i].obj;
          osSyncPrintf("airplanePathRight = %08x\n", PLANE->right);
          break;
        case 3:
          PLANE->obj = g_LoadedTrack.specials[i].obj;
          osSyncPrintf("airplane = %d\n", PLANE->obj);
          break;
        case 6:
          PLANE->trigger = g_LoadedTrack.specials[i].obj;
          osSyncPrintf("airplanetrigger = %d\n", PLANE->trigger);
          break;
        case 7:
          g_adwWaterfallInstances[g_dwWaterfallCount].obj = g_LoadedTrack.specials[i].obj;
          g_dwWaterfallCount++;
          osSyncPrintf("waterfall = %d\n", g_LoadedTrack.specials[i].obj);
          break;
        }
      }
      if (PLANE->trigger == 0 || PLANE->left == 0 || PLANE->right == 0) {
        PLANE->obj = 0;
      }
      if (PLANE->obj != 0) {
        vx = 1.0f;
        vy = 0.0f;
        vz = 0.0f;
        (&v)[0][0] = vx;
        (&v)[0][1] = vy;
        (&v)[0][2] = vz;
        Mtx4x4ProjectPoint(v, v, (float (*)[4])OBJ(PLANE->obj));
        PLANE->scale = Vec3fLength((N64Vec3 *)v);
      }
      PLANE->lap = g_dwTotalLaps - 1;
    }
    if (gInstantReplayActive != 0) {
      g_dwNumViewports = 1;
    } else {
      g_dwNumViewports = g_dwNumLocalPlayers;
    }
    if (gInstantReplayActive != 0) {
      g_aViewportRects[0].car = g_dwBestReplayPlayer;
    } else {
      g_aViewportRects[0].car = 0;
    }
    g_aViewportRects[1].car = !g_aViewportRects[0].car;
    if (g_dwGameMode == 4) {
      g_aVehicles[0].cam = &g_aVehicles[0].cams[1];
#if TGR_REGION_JPN || TGR_REGION_EUR
      if (g_dwRaceSessionType == 0) {
        g_dwReplayCameraSwitchTimer = 0;
      } else {
        g_dwReplayCameraSwitchTimer = 180;
      }
#else
      g_dwReplayCameraSwitchTimer = 180;
#endif
    } else {
      for (i = 0; i < g_dwNumLocalPlayers; i++) {
        g_aVehicles[i].xe6c = g_aVehicles[i].season->xd8;
        g_aVehicles[i].xe64 = g_aVehicles[i].season->xe0;
        g_aVehicles[i].xe60 = g_aVehicles[i].season->xdc;
        g_aVehicles[i].xe68 = g_aVehicles[i].season->xd4;
        PAD(i)->x25 = g_aVehicles[i].season->xe4;
      }
      if (g_dwGameMode == 2 && PAD(1)->ghost != 0) {
        g_aVehicles[1].xe68 = PAD(1)->ghost[2];
        g_aVehicles[1].xe6c = PAD(1)->ghost[3];
        g_aVehicles[1].xe60 = PAD(1)->ghost[4];
        g_aVehicles[1].xe64 = PAD(1)->ghost[5];
        PAD(1)->x25 = PAD(1)->ghost[6];
      } else {
        for (; i < g_dwNumActiveVehicles; i++) {
          g_aVehicles[i].xe6c = 1;
          g_aVehicles[i].xe64 = 1;
          g_aVehicles[i].xe60 = 2;
          g_aVehicles[i].xe68 = 0;
          PAD(i)->x25 = 0;
        }
      }
    }
    if (gInstantReplayActive == 0) {
      g_dwSavedResultsValid = 0;
      for (i = 0; i < g_dwNumActiveVehicles; i++) {
        g_aVehicles[i].x2064 = 1.0f;
        if (i < g_dwNumLocalPlayers) {
          p0 = &g_aVehicles[i]; p0->xed8 = (int)UpdatePhysicsStep;
          p1 = &g_aVehicles[i]; p1->colour[3] = 0;
        } else if (g_dwGameMode == 2) {
          p2 = &g_aVehicles[i]; p2->xed8 = (int)UpdatePhysicsStep;
          p3 = &g_aVehicles[i]; p3->colour[3] = 2;
          g_aVehicles[i].x2064 = 0.375f;
        } else {
          ChooseVehicleCar(&g_aVehicles[i]);
          p4 = &g_aVehicles[i]; p4->xed8 = (int)DriveAiVehicle;
          p5 = &g_aVehicles[i]; p5->colour[3] = 0;
        }
#if TGR_REGION_JPN
        if (g_aVehicles[i].xe58 == 0) {
          if (i < g_dwNumLocalPlayers) {
            LoadAndActivateTrackWithPaintColor(&g_aVehicles[i], i, g_aVehicles[i].kind);
          } else if (i == 1 && (g_dwGameMode == 1 || g_dwGameMode == 2)) {
            LoadAndActivateTrackWithPaintColor(&g_aVehicles[i], i, g_aVehicles[i].kind);
          } else {
            InstallVehicleModel(&g_aVehicles[i], i, g_aVehicles[i].kind);
          }
        } else {
          osSyncPrintf("not loading %d\n", i);
        }
#else
        if (g_aVehicles[i].xe58 == 0) {
          InstallVehicleModel(&g_aVehicles[i], i, g_aVehicles[i].kind);
        }
#endif
        SetupVehicleForRaceStart(&g_aVehicles[i]);
        g_aVehicles[i].xf4c = 0;
        g_aVehicles[i].msgA = 0;
        g_aVehicles[i].msgB = 0;
      }
    } else {
      for (i = g_dwNumLocalPlayers; i < g_dwNumActiveVehicles; i++) {
        SetMaterialColor((int)g_aVehicles[i].model, g_aVehicles[i].colour[0] >> 3,
                          g_aVehicles[i].colour[1] >> 3, g_aVehicles[i].colour[2] >> 3);
      }
    }
    if (gInstantReplayActive != 0) {
      g_dwRacePhaseIndex = 4;
    } else if (g_dwGameMode == 5) {
      g_dwRacePhaseIndex = 4;
#if TGR_REGION_JPN || TGR_REGION_EUR
    } else if (g_dwGameMode == 4 && g_dwRaceSessionType == 0) {
      g_dwRacePhaseIndex = 4;
#endif
    } else {
      g_dwRacePhaseIndex = 0;
    }
    g_dwRacePhase = g_aRacePhaseTable[g_dwRacePhaseIndex].state;
    g_fRacePhaseTimer = g_aRacePhaseTable[g_dwRacePhaseIndex].secs;
    if (gInstantReplayActive == 0) {
      g_dwRaceFinishOrderCounter = 0;
      for (i = 0; i < g_dwNumVehicles; i++) {
        InitRacerSlot(&g_aRacerSlots[i]);
      }
    }
    ApplyWeather(g_dwCurrentWeather);
    if (g_dwNightMode == 0 && (g_dwSelectedTrackId == 0 || g_dwSelectedTrackId == 5)) {
      PLANE->obj = 0;
    }
    SetFadeTarget(1.0f, 0.2f);
    FadeSfxTo(1.0f, 0.2f);
    FadeMusicTo(1.0f, 0.2f);
    g_dwScreenClearFrames = 2;
    g_dwRaceInitialized = 1;
    g_pStartLightsModel = (int)g_aDmaWorkBuffers;
    DmaReadAndFixupTrackModel(g_aDmaWorkBuffers, _startLightsSegmentRomStart, _startLightsSegmentRomEnd);
    PlayModelAnimationsOnce(g_pStartLightsModel);
    InitParticleSystem();
    ResetCollisionCaches();
    g_dwRacePauseState = 0;
    g_dwRaceExitState = 0;
    g_dwPrevViewportCount = -1;
    g_fRaceCaptionTimer = 0.0f;
    g_dwRaceCaptionIndex = 0;
    g_bRaceScreenActive = 1;
    if (gInstantReplayActive == 0) {
      ResetFrameClock();
      ResetAllSoundState();
    }
#if TGR_REGION_JPN || TGR_REGION_EUR
    g_bDemoInterrupted = 0;
    g_dwKeepScreenBlanked = 0;
    DEMO_FRAMES = -1;
#else
    g_dwKeepScreenBlanked = 0;
#endif
  }

  /* ---- every frame ---- */
  g_wRaceScreenLoopExited = 0;
#if TGR_REGION_JPN || TGR_REGION_EUR
  RestoreReplayKeyframeState();
#endif
  if (g_dwRacePhase == 4) {
    for (i = 0; i < g_dwNumLocalPlayers; i++) {
      SaveLastLapSnapshot(&g_aVehicles[i]);
    }
  }
#if TGR_REGION_EUR
  /* Europe flips it every frame; the attract-race test is left empty */
  g_bTrackCollisionFrameToggle ^= 1;
  if (g_dwGameMode != 4) {
  }
#else
#if TGR_REGION_JPN
  if (g_dwGameMode != 4 || g_dwRaceSessionType == 0) {
#else
  if (g_dwGameMode != 4) {
#endif
    g_bTrackCollisionFrameToggle ^= 1;
  }
#endif
  /* the replay ends when the recordings run out */
  if (gInstantReplayActive != 0 && g_dwRacePhase == 4) {
    if (g_dwGameMode == 1) {
      if (g_aViewportRects[0].car == g_dwBestReplayPlayer) {
        if (PAD(g_dwBestReplayPlayer)->ghostPos == PAD(g_dwBestReplayPlayer)->ghostLen) {
          if (g_dwNumLocalPlayers == 1) {
          replay_over:
            if (g_dwRaceExitState == 0) {
              g_dwRaceExitState = 1;
              g_bReplayAfterRace = 0;
              FadeSfxTo(0.0f, 0.2f);
              SetFadeTarget(0.0f, 0.2f);
            }
          } else if (PAD(!g_dwBestReplayPlayer)->ghostPos == PAD(g_dwBestReplayPlayer)->ghostLen + 240) {
            g_aViewportRects[1].car = g_dwBestReplayPlayer;
            g_aViewportRects[0].car = !g_aViewportRects[1].car;
            goto other_view;
          }
        }
      } else {
      other_view:
        if (PAD(!g_dwBestReplayPlayer)->ghostPos == PAD(!g_dwBestReplayPlayer)->ghostLen) {
          goto replay_over;
        }
      }
    } else if (PAD(0)->ghostPos == PAD(0)->ghostLen) {
      if (g_dwRaceExitState == 0) {
        g_dwRaceExitState = 1;
        g_bReplayAfterRace = 0;
        FadeSfxTo(0.0f, 0.2f);
        SetFadeTarget(0.0f, 0.2f);
      }
    }
  }
  UpdateFrameTiming();
  g_dwStartLightsAnimation = 0;
  paused = g_dwRacePauseState;
  g_bStartLightsVisible = 0;

  /* the start lights and the race state */
  if (g_dwRacePhase < 3) {
    g_bStartLightsVisible = 1;
    for (i = 0; i < g_dwNumVehicles; i++) {
      g_aRacerSlots[i].flags |= 1;
      car = g_aRacerSlots[i].car;
      if (car != 0 && car->slot < g_dwNumLocalPlayers) {
        if (g_dwGameMode == 1) {
          s = g_dwCurrentWeather - 1;
          if (s > 2 || s < 0) {
            s = 0;
          }
          car->xfa4 = g_apTrackMenuItems[g_dwSelectedTrackId]->limit[car->xe34][s].secs;
        }
        car->msgATime = 1.0f;
        switch (g_dwRacePhase) {
        case 0:
          g_dwStartLightsAnimation = -1;
          g_dwStartLightsStep = 0;
          break;
        case 1:
          break;
        case 2:
          g_dwStartLightsAnimation = 1;
          if (g_afStartLightThresholds[g_dwStartLightsStep] > g_fRacePhaseTimer) {
            g_dwStartLightsStep++;
            if (g_dwStartLightsStep == 4) {
              PlayGoSound();
            } else {
              PlayBeepSound();
            }
          }
          break;
        }
      }
    }
    g_fStartLightsDrop = 0.0f;
  timer:
    if (g_dwRacePauseState == 0 && (g_fRacePhaseTimer -= g_fFrameDeltaTime) < 0.0f) {
    advance:
      g_dwRacePhaseIndex++;
      g_dwRacePhase = g_aRacePhaseTable[g_dwRacePhaseIndex].state;
      g_fRacePhaseTimer = g_aRacePhaseTable[g_dwRacePhaseIndex].secs;
    }
  } else if (g_dwRacePhase == 3) {
    g_dwStartLightsAnimation = 1;
    g_bStartLightsVisible = 1;
    for (i = 0; i < g_dwNumVehicles; i++) {
      g_aRacerSlots[i].flags &= ~1;
    }
    g_fStartLightsDrop = (g_aRacePhaseTable[g_dwRacePhaseIndex].secs - g_fRacePhaseTimer) / g_aRacePhaseTable[g_dwRacePhaseIndex].secs;
    g_fStartLightsDrop *= 15.0f * g_fStartLightsDrop;
    goto timer;
  } else if (g_dwRacePhase == 4) {
    running = 1;
#if TGR_REGION_EUR
    if (((g_dwGameMode == 4
          && ((g_dwRaceSessionType == 2 && g_aVehicles[0].lapTime > 170.0f)
              || (g_dwRaceSessionType == 1 && g_aVehicles[0].lapTime > 42.0f)
              || (g_dwRaceSessionType == 0 && PAD(0)->ghostPos == PAD(0)->ghostLen)))
#elif TGR_REGION_JPN
    if (((g_dwGameMode == 4
          && ((g_dwRaceSessionType == 2 && g_aVehicles[0].lapTime > 140.0f)
              || (g_dwRaceSessionType == 1 && g_aVehicles[0].lapTime > 39.6f)
              || (g_dwRaceSessionType == 0 && PAD(0)->ghostPos == PAD(0)->ghostLen)))
#else
    if (((g_dwGameMode == 4
          && ((g_dwRaceSessionType == 2 && g_aVehicles[0].lapTime > 140.0f)
              || (g_dwRaceSessionType != 2 && g_aVehicles[0].lapTime > 39.6f)))
#endif
         || (g_dwGameMode == 5 && g_aVehicles[0].lapTime > 16.0f))
        && IsFadingOut() == 0) {
      SetFadeTarget(0.0f, 0.2f);
      FadeSfxTo(0.0f, 0.2f);
      g_dwRaceExitState = 1;
      gInstantReplayActive = 0;
      g_bReplayAfterRace = 0;
      running = 0;
    }
    for (i = 0; i < g_dwNumLocalPlayers; i++) {
      if ((g_aRacerSlots[i].flags & 2) != 0) {
        if (g_dwGameMode == 2 && PAD(i)->rec[i] != 0) {
          osSyncPrintf("# Format SB BitSize 24 Rows %d Columns 1\n", PAD(i)->recLen[i] >> 1);
          for (k50 = 0; k50 < PAD(i)->recLen[i]; k50 += 2) {
            osSyncPrintf("%.02x %.02x\n", PAD(i)->rec[i][k50], PAD(i)->rec[i][k50 + 1]);
          }
          if (i == 0
              && (PAD(i)->recLen[i] < g_dwGhostRecordingSize || g_abGhostRecording[0] != g_dwSelectedTrackId
                  || g_dwGhostRecordingSize < 9)) {
            osSyncPrintf("%d: %d<%d=%d || %d!=%d=%d || %d<=%d=%d\n", i, PAD(i)->recLen[i],
                         g_dwGhostRecordingSize, PAD(i)->recLen[i] < g_dwGhostRecordingSize, g_abGhostRecording[0], g_dwSelectedTrackId,
                         g_abGhostRecording[0] != g_dwSelectedTrackId, g_dwGhostRecordingSize, 8, g_dwGhostRecordingSize < 9);
            g_dwGhostRecordingSize = PAD(i)->recLen[i];
            memcpy(g_abGhostRecording, PAD(i)->rec[i], g_dwGhostRecordingSize);
            g_aVehicles[i].msgA = "NEW RECORD!";
            g_aVehicles[i].msgATime = 1.0f;
            FormatLapTime(g_aVehicles[i].xfc0, g_aVehicles[i].lapTime);
            g_aVehicles[i].msgB = g_aVehicles[i].xfc0;
            g_aVehicles[i].msgBTime = 1.0f;
            g_dwSaveAvailable = 1;
          }
          PAD(i)->rec[i] = 0;
        } else if (g_dwGameMode == 1) {
          for (k54 = 0; k54 < g_dwNumLocalPlayers; k54++) {
            PAD(i)->recKeep[k54] = PAD(i)->recLen[k54];
          }
        }
      } else {
        running = 0;
      }
    }
    if (running == 0) {
      if (g_dwGameMode == 1 && g_dwNumLocalPlayers == 1 && g_aVehicles[0].xfa4 == 0.0) {
        g_bReplayAfterRace = 0;
        g_aVehicles[0].msgA = g_aVehicles[1].msgA = "%ryTIME UP!";
        g_aVehicles[1].msgATime = 2.0f;
        g_aVehicles[0].msgATime = g_aVehicles[1].msgATime;
      } else {
        goto draw;
      }
    }
    goto advance;
  } else if (g_dwRacePhase == 5) {
    if ((g_dwGameMode == 2 || g_dwGameMode == 1 || g_dwGameMode == 0) && gInstantReplayActive == 0) {
      SaveRaceResultsToSlots();
    }
    goto advance;
  } else if (g_dwRacePhase == 6) {
    goto timer;
  } else if (g_dwRacePhase == 7 && g_dwRaceExitState == 0) {
    FadeSfxTo(0.0f, 0.2f);
    if (g_bReplayAfterRace == 0) {
      FadeMusicTo(0.0f, 0.2f);
    }
    SetFadeTarget(0.0f, 0.2f);
    g_dwRaceExitState = 1;
  }

draw:
  AddTimingMarker(0, 0, 0, 200, 0xFF);
  ClearPerFrameVariables();
  for (i = 0; i < g_dwNumActiveVehicles; i++) {
    ClearCameraChanged((int)&g_aVehicles[i]);
  }
  for (i = 0; i < g_dwNumVehicles; i++) {
    UpdateRacerSlotMotion(&g_aRacerSlots[i]);
  }
  for (i = 0; i < g_dwNumVehicles; i++) {
    UpdateRacerSlotRaceState(&g_aRacerSlots[i]);
  }
  if (g_dwGameMode == 0) {
    for (i = 0; i < g_dwNumLocalPlayers; i++) {
      ManageAiRacerSlots(&g_aVehicles[i]);
    }
  }
  SortRacersByPosition();
  if (g_dwRacePauseState == 0) {
    AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
    UpdateAllVehicleEffects();
    AddTimingMarker(0, 0, 0xFF, 0xFF, 0xFF);
    UpdatePrecipitationParticles();
    /* the airplane goes when a player passes its trigger on the right lap */
    if (PLANE->obj != 0 && PLANE->sent == 0
        && ((g_dwRainMode == 0 && g_dwSnowMode == 0 && g_dwNightMode == 0)
            || (g_dwSelectedTrackId != 2 && g_dwSelectedTrackId != 7))) {
      for (i = 0; i < g_dwNumLocalPlayers; i++) {
        for (k58 = 0; k58 < g_aVehicles[i].x2000; k58++) {
          if (g_aVehicles[i].laps == PLANE->lap && g_aVehicles[i].x1fc0[k58] == PLANE->trigger) {
            PLANE->sent = 1;
            break;
          }
        }
      }
    }
    /* the track's moving objects */
    for (i = 0; i < g_LoadedTrack.nSpecials; i++) {
      switch (g_LoadedTrack.specials[i].kind) {
      case 0:
        guRotateF(g_afMvpMatrix, *(float *)&g_LoadedTrack.specials[i].arg, 0.0f, 0.0f, 1.0f);
        goto spin;
      case 1:
        guRotateF(g_afMvpMatrix, *(float *)&g_LoadedTrack.specials[i].arg, 1.0f, 0.0f, 0.0f);
        goto spin;
      case 2:
        guRotateF(g_afMvpMatrix, *(float *)&g_LoadedTrack.specials[i].arg, 0.0f, 1.0f, 0.0f);
      spin:
        guMtxCatF(g_afMvpMatrix, (float (*)[4])OBJ((g_LoadedTrack.specials + i)->obj),
                  (float (*)[4])OBJ((g_LoadedTrack.specials + i)->obj));
        OBJ((g_LoadedTrack.specials + i)->obj)->flags &= 0xDFFF;
        break;
      case 3:
        if (PLANE->obj == 0 || PLANE->sent == 0) {
          break;
        }
        switch (g_dwSelectedTrackId) {
        case 1:
        case 6:
          PLANE->dist += g_fFrameDeltaTime * 18.0f;
          break;
        default:
          PLANE->dist += g_fFrameDeltaTime * 50.0f;
          break;
        }
        while (PLANE->dist > PLANE->segLen) {
          PLANE->dist -= PLANE->segLen;
          PLANE->point++;
          if (PLANE->point >= PLANE->count) {
            PLANE->obj = 0;
            break;
          }
          Vec3fMidpoint(&a, &LEFT[PLANE->point - 1], &RIGHT[PLANE->point - 1]);
          Vec3fMidpoint(&b, &LEFT[PLANE->point], &RIGHT[PLANE->point]);
          PLANE->segLen = Vec3fDistance(&a, &b);
          Vec3fSubtract(&PLANE->dir, &b, &a);
          Vec3fNormalize(&PLANE->dir);
          if (PLANE->point < 2) {
            d1 = (float *)&PLANE->prev;
            s1 = (float *)&PLANE->dir;
            d1[0] = s1[0];
            d1[1] = s1[1];
            d1[2] = s1[2];
          } else {
            Vec3fMidpoint(&c, &LEFT[PLANE->point - 2], &RIGHT[PLANE->point - 2]);
            Vec3fSubtract(&PLANE->prev, &a, &c);
            Vec3fNormalize(&PLANE->prev);
          }
          if (PLANE->point + 1 == PLANE->count) {
            d2 = (float *)&PLANE->next;
            s2 = (float *)&PLANE->dir;
            d2[0] = s2[0];
            d2[1] = s2[1];
            d2[2] = s2[2];
          } else {
            Vec3fMidpoint(&c, &LEFT[PLANE->point + 1], &RIGHT[PLANE->point + 1]);
            Vec3fSubtract(&PLANE->next, &c, &b);
            Vec3fNormalize(&PLANE->next);
          }
        }
        if (PLANE->obj != 0) {
          f = PLANE->dist / PLANE->segLen;
          Vec3fLerp(&a, &LEFT[PLANE->point], &LEFT[PLANE->point - 1], f);
          Vec3fLerp(&b, &RIGHT[PLANE->point], &RIGHT[PLANE->point - 1], f);
          Vec3fMidpoint(&OBJ(PLANE->obj)->pos, &a, &b);
          if (f > 0.5f) {
            Vec3fLerp(&OBJ(PLANE->obj)->x, &PLANE->next, &PLANE->dir, f - 0.5f);
          } else {
            Vec3fLerp(&OBJ(PLANE->obj)->x, &PLANE->dir, &PLANE->prev, f + 0.5f);
          }
          Vec3fNormalize(&OBJ(PLANE->obj)->x);
          Vec3fSubtract(&OBJ(PLANE->obj)->z, &a, &b);
          Vec3fCross(&OBJ(PLANE->obj)->y, &OBJ(PLANE->obj)->x, &OBJ(PLANE->obj)->z);
          Vec3fNormalize(&OBJ(PLANE->obj)->y);
          Vec3fCross(&OBJ(PLANE->obj)->z, &OBJ(PLANE->obj)->y, &OBJ(PLANE->obj)->x);
          Vec3fNormalize(&OBJ(PLANE->obj)->z);
          Vec3fScaleInPlace(&OBJ(PLANE->obj)->x, PLANE->scale);
          Vec3fScaleInPlace(&OBJ(PLANE->obj)->z, -PLANE->scale);
          Vec3fScaleInPlace(&OBJ(PLANE->obj)->y, PLANE->scale);
        }
        break;
      }
    }
  }
  for (i = 0; i < g_dwNumActiveVehicles; i++) {
    UpdateVehicleSounds(&g_aVehicles[i]);
  }
  ResetSoundSourceTracking();
  for (j = 0; j < g_dwNumViewports; j++) {
    g_pActiveCameraVehicle = &g_aVehicles[g_aViewportRects[j].car];
    g_pActiveCamera = g_pActiveCameraVehicle->cam;
    if (PLANE->obj != 0 && PLANE->sent != 0) {
      UpdateAirplaneSound(PLANE->obj, &OBJ(PLANE->obj)->pos, g_pActiveCamera);
    }
    for (i = 0; g_pActiveCamera != 0 && i < g_dwWaterfallCount; i++) {
      UpdateWaterfallSound(PLANE->obj, &OBJ((g_adwWaterfallInstances + i)->obj)->pos, g_pActiveCamera);
    }
    StubSoundUpdate((int)g_pActiveCamera);
  }
  UpdateAmbientSoundChannel();
#if TGR_REGION_JPN
  if (g_dwGameMode != 4 || g_dwRaceSessionType != 0 || PAD(0)->ghostPos >= 0x7EA) {
    UpdateFadeTransitions();
  }
#else
  UpdateFadeTransitions();
#endif
  StartLowResFrame();
  AddTimingMarker(0, 0, 0x82, 0, 0xFF);
  ResetShadowBuffers();
  ClearDepthBuffer();
  if (g_dwScreenClearFrames != 0 || g_dwSkyboxDisabled != 0) {
    ClearScreen(0, 0, 0);
    if (g_dwScreenClearFrames != 0) {
      g_dwScreenClearFrames--;
    }
  } else if (g_aViewportRects[0].w < 304) {
    DrawFilledRect(g_aViewportRects[0].x + g_aViewportRects[0].w, 8, 304 - g_aViewportRects[0].w, 224, 0, 0, 0);
  }
  if (g_dwNumViewports != g_dwPrevViewportCount) {
    g_dwScreenClearFrames = 2;
    g_dwPrevViewportCount = g_dwNumViewports;
  }

  /* each view */
  for (j = 0; j < g_dwNumViewports; j++) {
    AddTimingMarker(0, 0, 0, 0, 0xFF);
    g_pActiveCameraVehicle = &g_aVehicles[g_aViewportRects[j].car];
    g_pActiveCamera = g_pActiveCameraVehicle->cam;
    g_dwCurrentSplitScreenIndex = j;
    CastRayAgainstTrack(0, 0, g_pActiveCamera->mtx[3], &g_awCameraRayHitInstances, &g_dwCameraRayHitCount, &g_awCameraRayNearbyInstances, &g_dwCameraRayNearbyCount,
                  &g_fCameraRayGroundDistance, &g_dwCameraRayGroundSurface);
    SetViewport(g_aViewportRects[j].x, g_aViewportRects[j].y, g_aViewportRects[j].w, g_aViewportRects[j].h, 1);
    if (g_dwNumViewports >= 2) {
      SetupFrustumCorners(g_pActiveCamera, g_pActiveCamera->fov * 0.7f, g_fCameraProjectionScale * 0.65f,
                    (float)g_aViewportRects[j].w, (float)g_aViewportRects[j].h);
      SetupCameraMatrix(g_pActiveCamera->mtx, g_pActiveCamera->fov * 0.7f, g_fCameraProjectionScale * 0.65f,
                  (float)g_aViewportRects[j].w, (float)g_aViewportRects[j].h);
    } else {
      SetupFrustumCorners(g_pActiveCamera, g_pActiveCamera->fov, g_fCameraProjectionScale, (float)g_aViewportRects[j].w,
                    (float)g_aViewportRects[j].h);
      SetupCameraMatrix(g_pActiveCamera->mtx, g_pActiveCamera->fov, g_fCameraProjectionScale, (float)g_aViewportRects[j].w,
                  (float)g_aViewportRects[j].h);
    }
    SetupFog();
    SetupWeatherLights();
    AddTimingMarker(0, 0, 0x82, 0, 0xFF);
    RenderSkybox();
    ResetHeadlightGlare();
    BuildVisibleGrid();
    if (g_dwNightMode != 0) {
      g_dwVehicleViewModeIndex = 2;
    } else {
      g_dwVehicleViewModeIndex = 1;
    }
    for (i = 0; i < g_dwNumActiveVehicles; i++) {
      CheckVehicleVisibility(&g_aVehicles[i]);
    }
    RenderTrackGeometry(0);
    if (g_dwSnowMode == 0 || g_dwSelectedTrackId == 2 || g_dwSelectedTrackId == 7) {
      SetupFog();
      SetupWeatherLights();
      AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
      for (i = 0; i < g_dwNumVehicles; i++) {
        if (g_aRacerSlots[i].car != 0 && g_aRacerSlots[i].car->colour[3] != 2) {
          RenderVehicleModel(g_aRacerSlots[i].car, 0);
        }
      }
      AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
      RenderTireSmoke();
    }
    AddTimingMarker(0, 0x40, 0x40, 0x40, 0xFF);
    RenderStartLights();
    RenderTrackGeometry(1);
    if (g_dwSnowMode != 0 && g_dwSelectedTrackId != 2 && g_dwSelectedTrackId != 7) {
      SetupFog();
      SetupWeatherLights();
      AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
      for (i = 0; i < g_dwNumVehicles; i++) {
        if (g_aRacerSlots[i].car != 0 && g_aRacerSlots[i].car->colour[3] != 2) {
          RenderVehicleModel(g_aRacerSlots[i].car, 0);
        }
      }
      AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
      RenderTireSmoke();
    }
    AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
    for (i = 0; i < g_dwNumVehicles; i++) {
      if (g_aRacerSlots[i].car != 0) {
        RenderVehicleHeadlightBeams(g_aRacerSlots[i].car);
      }
    }
    AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
    for (i = 0; i < g_dwNumVehicles; i++) {
      if (g_aRacerSlots[i].car != 0 && g_aRacerSlots[i].car->colour[3] == 2) {
        RenderVehicleModel(g_aRacerSlots[i].car, 0);
      }
    }
    AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
    RenderExhaustAndTireSpray();
    AddTimingMarker(0, 0, 0xFF, 0xFF, 0xFF);
    RenderPrecipitationEffect();
    AddTimingMarker(0, 0, 0x82, 0, 0xFF);
    RenderHeadlightGlare();
    SetupMenuProjection((float)g_aViewportRects[j].w, (float)g_aViewportRects[j].h);
    SetupViewportAndScissor(g_aViewportRects[j].x, g_aViewportRects[j].y, g_aViewportRects[j].w, g_aViewportRects[j].h, 1);
    /* the rear-view mirror, in the bumper view of a single player */
    if (&g_pActiveCameraVehicle->cams[2] == g_pActiveCamera && g_dwNumViewports == 1 && g_dwRearViewMirrorMode != 0) {
      if (g_dwRearViewMirrorMode == 1) {
        mw = g_aViewportRects[j].w / 4;
      } else if (g_dwRearViewMirrorMode == 2) {
        mw = g_aViewportRects[j].w * 5 / 16;
      } else {
        mw = g_aViewportRects[j].w * 3 / 8 + 2;
      }
      mh = mw >> 2;
      mx = g_aViewportRects[j].x + ((g_aViewportRects[j].w - mw) >> 1);
      my = g_aViewportRects[j].y + g_aViewportRects[j].h / 16;
      saved = g_pActiveCamera;
      g_pActiveCamera = g_pActiveCameraVehicle->cam = &g_pActiveCameraVehicle->cam4;
      SetViewport(mx, my, -mw, mh, 1);
      ClearZBufferRect(mx, my, mw, mh);
      SetupFrustumCorners(g_pActiveCamera, g_pActiveCamera->fov, g_fCameraProjectionScale * 0.2f, (float)mw, (float)mh);
      SetupCameraMatrix(g_pActiveCamera->mtx, g_pActiveCamera->fov, g_fCameraProjectionScale * 0.3f, (float)mw, (float)mh);
      SetupFog();
      SetupWeatherLights();
      AddTimingMarker(0, 0, 0x82, 0, 0xFF);
      RenderSkybox();
      ResetHeadlightGlare();
      BuildVisibleGrid();
      AddTimingMarker(0, 0, 0x82, 0, 0xFF);
      for (i = 0; i < g_dwNumActiveVehicles; i++) {
        CheckVehicleVisibility(&g_aVehicles[i]);
      }
      RenderTrackGeometry(0);
      if (g_dwSnowMode == 0 || g_dwSelectedTrackId == 2 || g_dwSelectedTrackId == 7) {
        SetupFog();
        SetupWeatherLights();
        AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
        for (i = 0; i < g_dwNumVehicles; i++) {
          if (g_aRacerSlots[i].car != 0 && g_aRacerSlots[i].car->colour[3] != 2) {
            RenderVehicleModel(g_aRacerSlots[i].car, 0);
          }
        }
        AddTimingMarker(0, 0x80, 0x80, 0, 0xFF);
        RenderTireSmoke();
      }
      RenderTrackGeometry(1);
      if (g_dwSnowMode != 0 && g_dwSelectedTrackId != 2 && g_dwSelectedTrackId != 7) {
        SetupFog();
        SetupWeatherLights();
        AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
        for (i = 0; i < g_dwNumVehicles; i++) {
          if (g_aRacerSlots[i].car != 0 && g_aRacerSlots[i].car->colour[3] != 2) {
            RenderVehicleModel(g_aRacerSlots[i].car, 0);
          }
        }
      }
      AddTimingMarker(0, 0x80, 0x80, 0x80, 0xFF);
      for (i = 0; i < g_dwNumVehicles; i++) {
        if (g_aRacerSlots[i].car != 0 && g_aRacerSlots[i].car->colour[3] == 2) {
          RenderVehicleModel(g_aRacerSlots[i].car, 0);
        }
      }
      RenderHeadlightGlare();
      AddTimingMarker(0, 0, 0x82, 0, 0xFF);
      g_pActiveCameraVehicle->cam = saved;
      g_pActiveCamera = g_pActiveCameraVehicle->cam;
      SetViewport(g_aViewportRects[j].x, g_aViewportRects[j].y, g_aViewportRects[j].w, g_aViewportRects[j].h, 1);
      DrawRdpBorderRect(mx, my, mw, mh);
    }
    if (g_dwGameMode == 5) {
      SetTextStyleNormal();
      SetTextAlignCenter();
      SetTextSize(40);
      if (g_aVehicles[0].season->round != 0) {
        DrawText("%rySEASON WINNER!", 160, 210);
      } else {
        DrawText("%ryCHAMPION!", 160, 210);
      }
    } else if (g_bShowHudHelpText != 0 && g_dwGameMode != 4) {
      if (gInstantReplayActive != 0) {
        SetTextStyleNormal();
        SetTextAlignLeft();
        SetTextSize(15);
#if !TGR_REGION_JPN
        DrawText("%wwINSTANT REPLAY!", 28, 32);
#endif
#if TGR_REGION_JPN
        DrawText("%wwA : RESTART", 28, g_dwScreenHeight - 24);
        SetTextAlignRight();
        DrawText("%wwSTART : EXIT", g_dwScreenWidth - 28, g_dwScreenHeight - 24);
#else
        DrawText("%wwA to restart", 28, g_dwScreenHeight - 24);
        SetTextAlignRight();
        DrawText("%wwSTART to exit", g_dwScreenWidth - 28, g_dwScreenHeight - 24);
#endif
      } else {
        if (&g_pActiveCameraVehicle->cams[3] != g_pActiveCamera) {
          AddTimingMarker(0, 0xFF, 0, 0, 0xFF);
          DrawSpeedAndHudOverlay();
          AddTimingMarker(0, 0, 0x82, 0, 0xFF);
          if (&g_pActiveCameraVehicle->cams[2] != g_pActiveCamera) {
            StubDashboard();
          }
        } else if (g_pActiveCameraVehicle->link->flags & 2) {
          DrawHudMessage();
        }
      }
    }
    DrawRdpOutlineRect(g_aViewportRects[j].x, g_aViewportRects[j].y, g_aViewportRects[j].w, g_aViewportRects[j].h);
  }
  StubEmpty();
  SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
#if TGR_REGION_JPN || TGR_REGION_EUR
  /* the FINISH! cheat: both cars have done every lap */
  if (CHEAT_FINISH != 0) {
    g_aVehicles[0].xfac = 0;
    g_aVehicles[0].laps = g_dwTotalLaps;
    g_aRacerSlots[0].flags &= 1;
    g_aRacerSlots[0].flags |= 2;
    g_aVehicles[1].xfac = 1;
    g_aVehicles[1].laps = g_dwTotalLaps;
    g_aRacerSlots[1].flags &= 1;
    g_aRacerSlots[1].flags |= 2;
    g_bReplayAfterRace = 0;
    if (g_dwRacePhase < 4) {
      g_dwRacePhase = 4;
    }
    CHEAT_FINISH = 0;
  }
#endif

  /* input: skip the demo, pause, or the pause menu */
  if (g_dwRacePauseState != 0) {
    for (i = 0; i < g_dwNumLocalPlayers; i++) {
      UpdateStickRepeat(PAD(i));
      if (PAD(i)->pressed & 0x20) {
        TransferFlags(PAD(i), 0x20);
        FadeSfxTo(1.0f, 0.2f);
        FadeMusicTo(1.0f, 0.2f);
        g_dwScreenClearFrames = 2;
        paused = 0;
      }
      if (PAD(i)->pressed & 0xC010) {
        if (g_dwPauseMenuSelection != 0 || (PAD(i)->pressed & 0x10) == 0) {
          TransferFlags(PAD(i), 0xC010);
        }
        switch (g_dwPauseMenuSelection) {
        case 0:
          FadeSfxTo(1.0f, 0.2f);
          FadeMusicTo(1.0f, 0.2f);
          g_dwScreenClearFrames = 2;
          paused = 0;
          break;
        case 1:
          SetFadeTarget(0.0f, 0.2f);
          g_dwRaceExitState = 0;
          g_bReplayAfterRace = 0;
          g_dwRaceInitialized = 0;
          break;
        case 4:
          SetFadeTarget(0.0f, 0.2f);
          FadeSfxTo(0.0f, 0.2f);
          FadeMusicTo(0.0f, 0.2f);
          g_bReplayAfterRace = 0;
          g_dwRaceExitState = 1;
          break;
        }
      }
      if (PAD(i)->pressed & 8) {
        TransferFlags(PAD(i), 5);
        PAD(i)->pressed &= ~5;
        TransferFlags(PAD(i), 8);
        g_dwPauseMenuSelection = (g_dwPauseMenuSelection + 4) % 5;
      }
      if (PAD(i)->pressed & 2) {
        TransferFlags(PAD(i), 5);
        PAD(i)->pressed &= ~5;
        TransferFlags(PAD(i), 2);
        g_dwPauseMenuSelection = (g_dwPauseMenuSelection + 1) % 5;
      }
      if (PAD(i)->pressed & 4) {
        if (g_dwPauseMenuSelection != 0) {
          TransferFlags(PAD(i), 4);
        }
        switch (g_dwPauseMenuSelection) {
        case 2:
          DecreaseBgmVolume();
          break;
        case 3:
          DecreaseSfxVolume();
          break;
        }
      }
      if (PAD(i)->pressed & 1) {
        if (g_dwPauseMenuSelection != 0) {
          TransferFlags(PAD(i), 1);
        }
        switch (g_dwPauseMenuSelection) {
        case 2:
          IncreaseBgmVolume();
          break;
        case 3:
          IncreaseSfxVolume();
          break;
        }
      }
    }
    {
      x = g_aViewportRects[0].x + (g_aViewportRects[0].w >> 1);
      DrawScreenFadeOverlay(0.33f);
      SetTextStyleNormal();
      SetTextAlignCenter();
      SetTextSize(40);
      DrawText("%ryPAUSED", x, g_dwScreenHeight * 5 / 16);
      y = g_dwScreenHeight * 5 / 11;
      SetTextSize(20);
      DrawText(g_dwPauseMenuSelection == 0 ? "%y1Continue" : "%ryContinue", x, y);
      y += 20;
      DrawText(g_dwPauseMenuSelection == 1 ? "%y1Restart Race" : "%ryRestart Race", x, y);
      y += 20;
      sprintf(g_szFormatBuffer, "%sBGM Volume: %d", g_dwPauseMenuSelection == 2 ? "%y1" : "%ry", g_OptionSettings);
      DrawText(g_szFormatBuffer, x, y);
      y += 20;
      sprintf(g_szFormatBuffer, "%sSFX Volume: %d", g_dwPauseMenuSelection == 3 ? "%y1" : "%ry", g_OptionSettingsSfxLevel);
      DrawText(g_szFormatBuffer, x, y);
      y += 20;
      DrawText(g_dwPauseMenuSelection == 4 ? "%y1Exit to Main Menu" : "%ryExit to Main Menu", x, y);
      y += 20;
    }
  } else {
    if (g_dwGameMode == 4 || g_dwGameMode == 5) {
      if (IsFadingOut() == 0) {
        for (i = 0; i < 2; i++) {
          if (g_aPlayerInputState[i].pressed & 0xC010) {
            TransferFlags(&g_aPlayerInputState[i], 0xC010);
            g_bDemoInterrupted = 1;
            g_dwRaceExitState = 1;
            g_bReplayAfterRace = 0;
            FadeSfxTo(0.0f, 0.2f);
            SetFadeTarget(0.0f, 0.2f);
          }
        }
      }
    } else {
      for (i = 0; i < g_dwNumLocalPlayers; i++) {
        if (PAD(i)->pressed & 0x4000) {
          if (gInstantReplayActive != 0) {
            SetFadeTarget(0.0f, 0.2f);
            g_bReplayAfterRace = 0;
            g_dwRaceExitState = 1;
          } else {
            paused = 1;
            g_dwPauseMenuSelection = 0;
          }
          FadeSfxTo(0.0f, 0.2f);
          FadeMusicTo(0.0f, 0.2f);
          TransferFlags(PAD(0), 0x4000);
          TransferFlags(PAD(1), 0x4000);
          break;
        }
        if (PAD(i)->pressed & 0x10) {
          if (gInstantReplayActive != 0) {
            SetFadeTarget(0.0f, 0.2f);
            g_bReplayAfterRace = 1;
            g_dwRaceExitState = 1;
          }
          TransferFlags(PAD(0), 0x10);
          TransferFlags(PAD(1), 0x10);
          break;
        }
      }
    }
  }

  /* the attract demo's captions */
  if (g_dwGameMode == 4 && g_dwRaceSessionType == 2) {
    ScrollTextUp();
    DrawScrollingText();
  } else if (g_dwGameMode == 4 && g_dwRaceSessionType == 1 && g_aCreditCaptions[g_dwRaceCaptionIndex].title != 0) {
    if (g_fRaceCaptionTimer > 0.5f) {
      SetTextStyleNormal();
      SetTextAlignCenter();
      SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
      for (i = 0; i < 5; i++) {
        if (g_aCreditCaptions[g_dwRaceCaptionIndex].lines[i] == 0) {
          break;
        }
      }
      y0 = 132 - i * 40 / 4;
      if (g_aCreditCaptions[g_dwRaceCaptionIndex].title[0] == 0) {
        y0 -= 5;
      }
      while (--i >= 0) {
        SetTextSize(20);
        DrawText(g_aCreditCaptions[g_dwRaceCaptionIndex].lines[i], 160, i * 40 / 2 + y0);
      }
      SetTextSize(15);
      DrawText(g_aCreditCaptions[g_dwRaceCaptionIndex].title, 160, y0 - 20);
    }
    g_fRaceCaptionTimer += g_fFrameDeltaTime;
    if (g_aCreditCaptions[g_dwRaceCaptionIndex].secs < g_fRaceCaptionTimer) {
      g_fRaceCaptionTimer = 0.0f;
      g_dwRaceCaptionIndex++;
    }
  }
  ApplyScreenFade();
  if (1 < g_dwHudDebugLevel) {
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    DrawTimingBars();
  }
  AddTimingMarker(0, 0xFF, 0xE0, 0, 0xFF);
  StreamAnimatedTrackTextures();
  AddTimingMarker(0, 0, 0x82, 0, 0xFF);
  SetNextScreenCallback((int)UpdateRaceAnimations);
#if TGR_REGION_JPN || TGR_REGION_EUR
  /* the intro demo's last half-second goes to black */
  if (g_dwGameMode == 4 && g_dwRaceSessionType == 0 && PAD(0)->ghostLen - 30 <= PAD(0)->ghostPos) {
#if TGR_REGION_EUR
    DrawScreenFadeOverlay((PAD(0)->ghostLen - PAD(0)->ghostPos) / 30.0f);
#else
    ClearScreen(0, 0, 0);
#endif
  }
#endif
  FlushDisplayList();

  /* faded out: on to the next thing */
  if (IsFadingOut() != 0 && IsFadeComplete() != 0) {
    for (i = 0; i < 4; i++) {
      for (kb4 = 0; kb4 < g_dwNumLocalPlayers; kb4++) {
        PAD(i)->recKeep[kb4] = PAD(i)->recLen[kb4];
      }
    }
    g_dwSoundEffectEchoOffset = 0;
    for (kb8 = 0; kb8 < 6; kb8++) {
      g_aSoundEffectChannels[kb8].x14 = 0;
      g_aSoundEffectChannels[kb8].x10 = 0;
      g_aSoundEffectChannels[kb8].x8 = 0;
      g_aSoundEffectChannels[kb8].p = g_SilenceSampleData;
    }
    g_bRaceScreenActive = 0;
    if (g_dwRaceExitState != 0) {
      if (g_bReplayAfterRace != 0) {
        /* the instant replay plays the best lap's recordings */
        if (gInstantReplayActive == 0) {
          g_dwBestReplayLength = PAD(0)->recLen[0];
          g_dwBestReplayPlayer = 0;
          for (i = 1; i < g_dwNumLocalPlayers; i++) {
            if (g_dwBestReplayLength > PAD(i)->recLen[i]) {
              g_dwBestReplayLength = PAD(i)->recLen[i];
              g_dwBestReplayPlayer = i;
            }
          }
          for (i = 0; i < g_dwNumLocalPlayers; i++) {
            g_apReplayGhostPtrs[i] = PAD(i)->rec[g_dwBestReplayPlayer];
            g_adwReplayGhostSizes[i] = PAD(i)->recLen[g_dwBestReplayPlayer];
          }
          for (i = 0; i < g_dwNumLocalPlayers; i++) {
            for (kbc = 0; kbc < g_dwNumLocalPlayers; kbc++) {
              osSyncPrintf("veh[%d]->recIndex[%d] = %d\n", i, kbc, PAD(i)->recLen);
              PAD(i)->rec[kbc] = 0;
            }
          }
          osSyncPrintf("bestNumber = %d, bestIndex = %d\n", g_dwBestReplayPlayer, g_dwBestReplayLength);
        }
        gInstantReplayActive = 1;
      } else {
        ClearFramebuffersHighRes();
        g_dwTrackMirrored = 0;
        if (g_dwGameMode == 5) {
          if (g_aVehicles[0].season->round != 0) {
          next_race:
            g_dwGameMode = 0;
            SetTrackFromSeason();
            g_bSaveLoadEntryPoint = 1;
            SetCurrentScreenCallback((int)ProcessDataSaveLoadMenu);
          } else {
            StartEndingCredits();
          }
        } else if (g_dwGameMode == 4 && g_dwRaceSessionType == 2) {
          goto next_race;
        } else if (g_dwSavedResultsValid != 0 && (g_dwGameMode == 2 || g_dwGameMode == 1 || g_dwGameMode == 0)) {
          RestoreRaceResultsFromSlots();
          SetCurrentScreenCallback((int)ProcessRaceResultsScreen);
        } else {
          if (g_dwGameMode == 4 && g_dwRaceSessionType == 1) {
            SetCurrentScreenCallback((int)ProcessOptionsMenu);
          } else if (g_dwGameMode == 4 && g_dwRaceSessionType == 0 && g_bDemoInterrupted == 0) {
            SetCurrentScreenCallback((int)ProcessSplashScreens);
          } else {
            SetCurrentScreenCallback((int)ProcessMainMenu);
          }
        }
        gInstantReplayActive = 0;
      }
      g_dwRaceInitialized = 0;
    }
    g_wRaceScreenLoopExited = 1;
    for (i = 0; i < 4; i++) {
      PAD(i)->ghost = 0;
    }
  }
  if (g_dwRacePauseState != paused) {
    if (paused != 0) {
      SaveClockForPause();
    } else {
      RestoreClockAfterPause();
    }
    g_dwRacePauseState = paused;
  }
}
/* IDO needs local definitions to match, while the original data assembly supplies their address. */
#pragma weak g_dwAirplaneInstance
#pragma weak g_fStartLightsDrop
/* clang-format on */
