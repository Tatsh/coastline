/* The host test uses native struct layouts; the MIPS source keeps the original 32-bit layout. */
#if defined(__mips__) || defined(__sgi)
/* clang-format off */
#if defined(__GNUC__) && !defined(__sgi)
#define TGR_MATCHING_UNUSED __attribute__((unused))
#else
#define TGR_MATCHING_UNUSED
#endif
#include "n64/carselect-aliases.h"
#include "n64/race-internal.h"
/* carselect.c -- the car-select screen
 */

/* -- declarations -- */
int IsVehicleModelValid(int param_1);
extern int D_8020C688;
extern int D_8020C68C;
extern int g_dwIsPaintMode;
extern char *g_pActiveProfile;
extern char D_8028AE24;
extern Gfx *gDisplayListPtr;
int sprintf(char *buf, char *fmt, ...);
/* -- end declarations -- */

/* WHAT IT DOES: Draw one of the car-select screen's stat bars at (x, y),
 * w by h, in fill mode: a dark frame, the empty bar inset by 3 pixels, and
 * the filled part as the given fraction of its width.
 * The box is inset by updating x, y, w and h in place, and the filled width
 * is w = w * frac before its pipe sync, as the ROM computes them. */
/* @implements 0x8020C460 tgr DrawProgressBar */
void DrawProgressBar(int x, int y, int w, int h, float frac)
{
  gDPPipeSync(gDisplayListPtr++);
  gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
  gDPSetRenderMode(gDisplayListPtr++, 0, 0);
  gDPSetCombine(gDisplayListPtr++, 0xffffff, 0xfffdf6fb);
  gDPSetFillColor(gDisplayListPtr++, 1);
  gDPFillRectangle(gDisplayListPtr++, x, y, x + w, y + h);
  x += 3;
  y += 3;
  w -= 6;
  h -= 6;
  gDPPipeSync(gDisplayListPtr++);
  gDPSetFillColor(gDisplayListPtr++, 0x1c1);
  gDPFillRectangle(gDisplayListPtr++, x, y, x + w, y + h);
  w = w * frac;
  gDPPipeSync(gDisplayListPtr++);
  gDPSetFillColor(gDisplayListPtr++, 0x781);
  gDPFillRectangle(gDisplayListPtr++, x, y, x + w, y + h);
}

/* WHAT IT DOES: Tell whether car n may be picked on the car-select screen:
 * cars 9 and up (Japan: 10 and up) only while the flag at 0x80272070 is clear, the car's bit
 * must be set in the season record's unlock mask, and its model must be
 * present. */
/* @implements 0x8020C66C tgr IsCarUnlocked */
int IsCarUnlocked(int n)
{
#if TGR_REGION_JPN
  return (g_dwIsPaintMode == 0 || n < 9 || n == 9) && (*(unsigned short *)(g_pActiveProfile + 0xcc) & (1 << n)) &&
         IsVehicleModelValid(n);
#else
  return (g_dwIsPaintMode == 0 || n < 9) && (*(unsigned short *)(g_pActiveProfile + 0xcc) & (1 << n)) &&
         IsVehicleModelValid(n);
#endif
}

/* -- declarations: RenderCameraView -- */
typedef struct N64Model N64Model;
typedef struct N64Stream { char pad00[0x28]; } N64Stream;
extern N64Car *g_pActiveCameraVehicle;               /* the car being drawn */
extern N64CarCam *g_pActiveCamera;            /* the camera being drawn from */
extern float g_fCameraFovRadians;                /* the lens scale */
extern float g_fCameraProjectionScale;
extern int g_dwIsReviewMode;
extern N64Stream g_aVehicleModelSlots[4];
extern float g_afMvpMatrix[4][4];
extern float g_afScratchMatrix[4][4];
extern char g_CameraViewAmbientLight[];               /* the car-select lights: the ambient, then four lights */
float cosf(float x);
float sinf(float x);
void Vec3fCross(N64Vec3 *pOut, N64Vec3 *pA, N64Vec3 *pB);
float Atan2f(float x, float y);
void SetupCameraMatrix(N64CarCam *cam, float fov, float far, float w, float h);
void SetViewport(int x, int y, int w, int h, int scissor);
void MarkVisibleCell(int x, int y);
void TransformWheelPositions(int n);
int PollVehicleModelDma(N64Stream *s);
int IsVehicleModelLoaded(N64Stream *s);
void SetVehicleModelSlot(N64Car *car, int kind);
void DrawCarStageDisc(N64Car *car);
void DrawCarStageChoiceMarker(N64Car *car);
void DrawCarStageShadow(N64Car *car);
void CheckVehicleVisibility(N64Car *car);
void RenderVehicleModel(N64Car *car, int a);
void *AllocLightBlock(void);
Mtx *AllocDisplayListBlock(void);
void guLookAtReflectF(float mf[4][4], void *l, float xEye, float yEye, float zEye, float xAt, float yAt,
                      float zAt, float xUp, float yUp, float zUp);
void guScaleF(float mf[4][4], float x, float y, float z);
void guRotateF(float mf[4][4], float a, float x, float y, float z);
void guMtxCatF(float m[4][4], float n[4][4], float r[4][4]);
void guTranslateF(float mf[4][4], float x, float y, float z);
void DrawModelList(N64Model *model, float m[4][4]);
/* -- end declarations -- */

/* WHAT IT DOES: Draw a car-select view: point the player's car and camera
 * at the view (turned by spin radians), set the viewport, and slide the
 * shown car sideways by slide squared times k.  The car kindA is drawn
 * there -- its own model once streamed in, or the model iconA under fixed
 * lights when given -- and while the view is sliding the car kindB (or
 * iconB) comes in from the other side, k further along.  The unused
 * array and the order of la among the locals are what the ROM frame holds
 * (0xE0, the two look-at pointers at 0xCC and 0xA4).  Japan draws the
 * incoming car whenever its model slot is free, without reloading it. */
/* @implements 0x8020C6D0 tgr RenderCameraView */
void RenderCameraView(N64Model *iconA, N64Model *iconB, int player, int x, int y, int w, int h, float slide,
                   int noBegin, int kindA, int kindB, float k, float spin)
{
  N64Stream *sA;
  N64Stream *sB;
  float neg;
  float *p;
  void *la;
  float deg;

  g_pActiveCameraVehicle = &g_aVehicles[player];
  g_pActiveCameraVehicle->mtx0[0][0] = cosf(spin);
  g_pActiveCameraVehicle->mtx0[0][1] = sinf(spin);
  g_pActiveCameraVehicle->mtx0[0][2] = 0.0f;
  Vec3fCross((N64Vec3 *)g_pActiveCameraVehicle->mtx0[1], (N64Vec3 *)g_pActiveCameraVehicle->mtx0[2], (N64Vec3 *)g_pActiveCameraVehicle->mtx0[0]);
  g_pActiveCamera = g_pActiveCameraVehicle->cam = &g_pActiveCameraVehicle->cams[3];
  g_pActiveCameraVehicle->heading = Atan2f(g_pActiveCamera->mtx[0][0], g_pActiveCamera->mtx[0][1]);
  g_pActiveCameraVehicle->heading = Atan2f(g_pActiveCamera->mtx[0][0], g_pActiveCamera->mtx[0][1]);
  SetupCameraMatrix(g_pActiveCamera, g_fCameraFovRadians, g_fCameraProjectionScale * 0.2f, w, h);
  SetViewport(x, y, w, h, 1);
  MarkVisibleCell(0, 0);
  if (slide < 0.0) {
    slide = slide * slide * -k;
  } else {
    slide = slide * slide * k;
  }
  p = g_pActiveCameraVehicle->mtx0[3];
  p[0] = slide;
  p[1] = neg = -slide;
  p[2] = 0.254f;
  if (iconA == 0) {
    TransformWheelPositions(player);
  }
  sA = &g_aVehicleModelSlots[kindA];
  PollVehicleModelDma(sA);
  DrawCarStageDisc(g_pActiveCameraVehicle);
  if (noBegin == 0 && g_dwIsReviewMode == 0) {
    DrawCarStageChoiceMarker(g_pActiveCameraVehicle);
  }
  if (iconA) {
    la = AllocLightBlock();
    guLookAtReflectF(g_afScratchMatrix, la, 11.0f, 11.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    gSPLookAtX(gDisplayListPtr++, la);
    gSPLookAtY(gDisplayListPtr++, (char *)la + 16);
    gSPNumLights(gDisplayListPtr++, 4);
    gSPLight(gDisplayListPtr++, g_CameraViewAmbientLight + 8, 1);
    gSPLight(gDisplayListPtr++, g_CameraViewAmbientLight + 24, 2);
    gSPLight(gDisplayListPtr++, g_CameraViewAmbientLight + 40, 3);
    gSPLight(gDisplayListPtr++, g_CameraViewAmbientLight + 56, 4);
    gSPLight(gDisplayListPtr++, g_CameraViewAmbientLight, 5);
    AllocDisplayListBlock();
    guScaleF(g_afMvpMatrix, 1.0f / 768, 1.0f / 768, 1.0f / 768);
    guRotateF(g_afScratchMatrix, spin * 57.295776f, 0.0f, 0.0f, 1.0f);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    guTranslateF(g_afScratchMatrix, slide, neg, 0);
    guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
    DrawModelList(iconA, g_afMvpMatrix);
  } else if (IsVehicleModelLoaded(sA)) {
    SetVehicleModelSlot(g_pActiveCameraVehicle, kindA);
    DrawCarStageShadow(g_pActiveCameraVehicle);
    CheckVehicleVisibility(g_pActiveCameraVehicle);
    RenderVehicleModel(g_pActiveCameraVehicle, 0);
  }
  sB = &g_aVehicleModelSlots[kindB];
  PollVehicleModelDma(sB);
  if (slide != 0.0f) {
    if (iconB) {
      void *la;
      int u0[9] TGR_MATCHING_UNUSED;

      deg = spin * 57.295776f;
      la = AllocLightBlock();
      guLookAtReflectF(g_afScratchMatrix, la, 0.0f, -1.0f, 15.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
      gSPLookAtX(gDisplayListPtr++, la);
      gSPLookAtY(gDisplayListPtr++, (char *)la + 16);
      gSPNumLights(gDisplayListPtr++, 4);
      gSPLight(gDisplayListPtr++, g_CameraViewAmbientLight + 8, 1);
      gSPLight(gDisplayListPtr++, g_CameraViewAmbientLight + 24, 2);
      gSPLight(gDisplayListPtr++, g_CameraViewAmbientLight + 40, 3);
      gSPLight(gDisplayListPtr++, g_CameraViewAmbientLight + 56, 4);
      gSPLight(gDisplayListPtr++, g_CameraViewAmbientLight, 5);
      AllocDisplayListBlock();
      guScaleF(g_afMvpMatrix, 1.0f / 768, 1.0f / 768, 1.0f / 768);
      guRotateF(g_afScratchMatrix, deg, 0, 0, 1.0f);
      guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
      if (0.0f < slide) {
        guTranslateF(g_afScratchMatrix, slide - k, -(slide - k), 0);
      } else {
        guTranslateF(g_afScratchMatrix, slide + k, -(slide + k), 0);
      }
      guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
      DrawModelList(iconB, g_afMvpMatrix);
    } else {
#if TGR_REGION_JPN
      if (IsVehicleModelLoaded(sB)) {
#else
      if (IsVehicleModelLoaded(sB) || !PollVehicleModelDma(sB)) {
#endif
        SetVehicleModelSlot(g_pActiveCameraVehicle, kindB);
        if (0.0f < slide) {
          p = g_pActiveCameraVehicle->mtx0[3];
          p[0] = slide - k;
          p[1] = -(slide - k);
          p[2] = 0.254f;
        } else {
          p = g_pActiveCameraVehicle->mtx0[3];
          p[0] = slide + k;
          p[1] = -(slide + k);
          p[2] = 0.254f;
        }
        TransformWheelPositions(player);
        DrawCarStageShadow(g_pActiveCameraVehicle);
        CheckVehicleVisibility(g_pActiveCameraVehicle);
        RenderVehicleModel(g_pActiveCameraVehicle, 0);
        if (IsVehicleModelLoaded(sA)) {
          SetVehicleModelSlot(g_pActiveCameraVehicle, kindA);
        }
      }
    }
  }
}

/* WHAT IT DOES: Format a time in seconds as minutes'seconds"hundredths.
 * Same arithmetic as the PC twin FormatLapTime (br_timefmt.c). */
/* @implements 0x8020CF44 tgr FormatLapTime */
void FormatLapTime(char *psz, float t)
{
  int total = (int)(t * 100.0f);
  int whole = total / 100;
  int minutes;

  total -= whole * 100;
  minutes = whole / 60;
  whole -= minutes * 60;
  sprintf(psz, "%d'%02d\"%02d", minutes, whole, total);
}

/* -- declarations: ProcessCarSelectScreen -- */
typedef struct N64RomFile { int start; int end; void *data; } N64RomFile;
typedef struct { char raw[0xdf88]; } N64CarModelBuf;
extern N64CarModelBuf g_aVehicleModels[];  /* each car's model buffer */
extern N64Stream g_aVehicleModelSlots[4];  /* the two players' model streams, two apiece */
extern MenuItem *g_apHandlingOptions[];  /* the setup choices: handling, transmission, */
extern int g_dwHandlingOptionCount;          /* tyres, suspension, decals, and their counts */
extern MenuItem *g_apTransmissionOptions[];
extern int g_dwTransmissionOptionCount;
extern MenuItem *g_apTiresOptions[];
extern int g_dwTiresOptionCount;
extern MenuItem *g_apSuspensionOptions[];
extern int g_dwSuspensionOptionCount;
extern MenuItem *g_apDecalOptions[];
extern int g_dwDecalListCount;
extern N64RomFile g_BgTextureDesc;    /* the background */
extern N64RomFile g_TitleBarTextureDesc;    /* the title bar */
extern N64RomFile g_ArrowLeftTextureDesc;
extern N64RomFile g_ArrowRightTextureDesc;
extern char *g_apszEngineLayouts[];      /* engine layouts */
extern char *g_apszDriveTypes[];      /* transmission names */
extern char *g_apszPlaceNames[];      /* places: 1st, 2nd, ... */
extern unsigned char g_abRacePositionPoints[];  /* championship points per place */
extern N64Track *g_apTrackMenuItems[];   /* the tracks */
extern MenuItem *g_apWeatherMenuItems[];  /* the weathers */
extern int g_bCarSelectInitialized;          /* the screen has been set up */
extern int g_dwIsReviewMode;          /* the screen shows the race results */
extern int g_dwIsPaintMode;          /* the screen is the paint shop's car select */
extern N64CarModelRec g_aCarModels[];  /* car n's record (0x60 bytes) */
extern int g_dwCarCount;          /* cars */
extern int g_dwElapsedTimeMs;
extern float g_fFrameDeltaTime;        /* seconds this frame */
extern int g_dwScreenWidth;          /* screen width */
extern int g_dwScreenHeight;          /* screen height */
extern int g_dwWeatherActive;
extern int g_dwNightMode;
extern int g_dwSnowMode;
extern int g_dwRainMode;
extern int g_dwHudDebugLevel;
extern int g_dwVehicleViewModeIndex;
extern int g_dwCurrentWeather;
extern int g_dwTotalLaps;          /* laps */
extern int g_dwNumActiveVehicles;          /* cars in the race */
extern int g_dwSelectedTrackId;          /* the track */
extern int g_dwNumLocalPlayers;          /* human players */
extern int g_dwGameMode;          /* the game mode */
extern int g_dwSeasonResultMsgLineCount;          /* lines of season news */
extern char *g_apszSeasonResultMsgLines[];      /* the lines */
extern int g_dwSaveAvailable;          /* a new time-attack record */
extern int g_dwContinueFromResults;
extern unsigned char g_bSaveLoadEntryPoint;
extern int g_dwScreenPlayerIdx;          /* the paint shop's player */
extern N64Round g_SeasonTable[];
typedef struct N64RaceSnd {      /* a sound channel (0x18 bytes) */
  void *p;
  int x4;
  unsigned long long pitch;     /* 0x08  32.32 playback ratio */
  int x10;
  unsigned int level;           /* 0x14  left << 16 | right */
} N64RaceSnd;
extern N64RaceSnd g_aSoundEffectChannels[6];  /* six sound channels */
extern char g_SilenceSampleData[];
extern char _scrollSoundSegmentRomStart[];       /* the screen's two sound banks in the ROM */
extern char _selectSoundSegmentRomStart[];
typedef struct N64ViewRect { int x; int y; int w; int h; int x10; } N64ViewRect;
extern N64ViewRect g_aViewportRects[2];  /* the two views; x10 is the car each follows */
void InitVehicleColor(N64Car *car);
void UpdatePhysicsStep(int ent);
void DriveAiVehicle(int ctl);
void ChooseVehicleCar(N64Car *car);
int IsCarUnlocked(int n);
void InstallVehicleModel(N64Car *car, int slot, int kind);
void Vec3fScale(N64Vec3 *pOut, N64Vec3 *pV, float s);
void Vec3fNormalize(N64Vec3 *pV);
void Vec3fCross(N64Vec3 *pOut, N64Vec3 *pA, N64Vec3 *pB);
void *memcpy(void *dst, void *src, unsigned int n);
void ResetHeap(void);
void LoadMenuItemModels(MenuItem **items, int *count);
void LoadAndDecompressAsset(N64RomFile *f, void *(*alloc)(int size));
void *AllocHeapAligned(int size);
int DmaReadAssetSize(int rom);
unsigned int DecompressFromRom(void *dst, int rom, void *s);
void AllocateEditorBuffers(void);
void UpdateFrameTiming(void);
void UpdateFadeTransitions(void);
void StartHighResFrame(void);
void SetupFog(void);
void SetupWeatherLights(void);
void AddTimingMarker(int bar, int r, int g, int b, int a);
void ClearDepthBuffer(void);
void DrawTexturedRect(N64RomFile *f, int x, int y, int w, int h, int a, int b, int c, int d,
                   int r, int g, int bl, int e, int f2);
int IsFadeComplete(void);
int IsFadingOut(void);
int IsFadingIn(void);
void SetScissorRect(int x, int y, int w, int h);
void SetupMenuProjection(float w, float h);
void SetViewport(int x, int y, int w, int h, int scissor);
void SetTextStyleNormal(void);
void SetTextAlignCenter(void);
void SetTextAlignLeft(void);
void SetTextAlignRight(void);
void SetTextCustomColors(int a, int b, int c, int d, int e, int f);
void SetTextSize(int font);
void DrawText(const char *s, int x, int y);
void DrawProgressBar(int x, int y, int w, int h, float frac);
void DrawMenuFooterButtons(void);
void DrawPauseMenuButtons(void);
void ApplyScreenFade(void);
void DrawTimingBars(void);
int ProcessDecalSaveLoad(N64CarModelBuf *m, unsigned char port, char op, char fromMenu,
                       unsigned char *done);
void DrawPakErrorMessage(int status, char op, char fromMenu);
void ExtractVehicleBaseColor(N64Car *car, void *m);
void FlushDisplayList(void);
void UpdateStickRepeat(N64PadRec *pad);
void TransferFlags(N64PadRec *pad, unsigned int bits);
short FindFreeSoundChannel(void);
void InitSoundChannel(short v, unsigned char *start, int len, int loop);
void LoadVehicleModel(N64Stream *s, int slot, int car, int bufIdx);
void SetMaterialColor(int model, unsigned int r, unsigned int g, int b);
int PollVehicleModelDma(N64Stream *s);
void ProcessSeasonResults(void);
void FadeSfxTo(float level, float seconds);
void FadeMusicTo(float level, float seconds);
void SetFadeTarget(float level, float seconds);
void ClearFramebuffersForWideMode(void);
void ClearFramebuffersLowRes(void);
void SetTrackFromSeason(void);
void SetCurrentScreenCallback(int mode);
void ProcessTrackMenu(void);
void ProcessWeatherSelect(void);
void ProcessMainMenu(void);
void ProcessRacingScreen(void);
void ProcessPaintShopScreen(void);
void ProcessDataSaveLoadMenu(void);
#if TGR_REGION_JPN || TGR_REGION_EUR
int IsCurrentScreen(void (*fn)(void));
void ProcessRaceResultsScreen(void);
extern int g_dwKeepScreenBlanked;
#endif
#if TGR_REGION_JPN
void BlankDisplay(void);
#define CARSEL_BLANK BlankDisplay
#endif
#if TGR_REGION_EUR
void BlankDisplay(void);
#define CARSEL_BLANK BlankDisplay
#endif
#if TGR_REGION_JPN
extern unsigned char g_abPlayerColorsRgb[][3];     /* the players' own colours */
#endif
/* The setup lists' titles: Japan's are singular. */
#if TGR_REGION_JPN
#define TIRES_TITLE "Tire"
#define DECALS_TITLE "Decal"
#else
#define TIRES_TITLE "Tires"
#define DECALS_TITLE "Decals"
#endif
/* -- end declarations -- */

#define CAR(i) (&g_aVehicles[i])
/* the pad loop's car: Japan indexes the array outside the colour block */
#if TGR_REGION_JPN
#define PCAR CAR(p)
#else
#define PCAR car
#endif
#define STREAM(slot) (&g_aVehicleModelSlots[slot])

/* one player's setup list: the choices, their count, the title */
#define SETUP(p, list, count, title) \
  { D_80316270[p] = (list); D_80316288[p] = (count); D_80316290[p] = (title); }

/* WHAT IT DOES: One frame of the car-select screen, which is also the race
 * results (g_dwIsReviewMode) and the paint shop's car select (g_dwIsPaintMode).  The
 * first frame sets up fades, the season state, each player's car, the
 * turntable camera, the setup lists, artwork and sound banks.  Every frame
 * turns the turntables, draws each player's car and setup choice (or the
 * results pages), runs the Controller Pak decal load, then reads the pads:
 * pick a car, tint it, step the setup choices, and when all are ready fade
 * out to the race, paint shop, menus, season screens or pak save.  Europe
 * and Japan blank two frames after the results, tint in steps of 8 and
 * repaint once; only Japan has the separate decal choice.  The
 * branches run in the order of the file's literals in the ROM, so stored
 * title and message pointers are the ROM's; six scalars declared above buf
 * put buf/time at the ROM's 0xE8/0xC4.
 * The screen's state is function-static (the ROM stores each value back
 * after it is read), declared in address order with the unused slots kept.
 * DecompressFromRom returns its length, so v0 is live after the call and the
 * sound-bank clearing loop takes v1/a0 for its pointers and v0 for the
 * bound.  Japan's pad loop reaches the car through CAR(p) outside the colour
 * block (so the base register is known not to alias the stack), and an empty
 * block ahead of the computer cars' loop sets the base address's register. */
/* @implements 0x8020D004 tgr ProcessCarSelectScreen */
void ProcessCarSelectScreen(void)
{
#if TGR_REGION_JPN
  static int D_80316248[2];       /* each player's car is to be repainted */
#endif
  static int D_80316250 TGR_MATCHING_UNUSED;
  static int D_80316254 TGR_MATCHING_UNUSED;
  static int D_80316258[2];       /* each player's model slot (0-3; slot ^ 2 is the other) */
  static float D_80316260[2];     /* each player's car sliding in (-1..1, 0 at rest) */
  static float D_80316268[2];     /* each player's turntable angle */
  static MenuItem **D_80316270[2];  /* each player's current setup list */
  static int D_80316278[2];       /* the choice in it */
  static int D_80316280[2];       /* the previous choice */
  static int D_80316288[2];       /* choices in the list */
  static char *D_80316290[2];     /* the list's title */
  static int D_80316298;          /* going ahead (not back) */
  static unsigned char *D_8031629C;  /* the two sound banks for the screen */
  static int D_803162A0;
  static unsigned char *D_803162A4;
  static int D_803162A8;
  static int D_803162AC;          /* frames the title is still drawn */
  static int D_803162B0[2];       /* each player's step: 0 car, 1-4 setup, 5 decals, 6 ready */
  static int D_803162B8;          /* loading decals from the pak */
  static int D_803162BC;          /* for this player */
  static int D_803162C0;          /* the load's status */
  static unsigned char D_803162C4;  /* the load has finished */
  static int D_803162C8[13] TGR_MATCHING_UNUSED;
  static int D_803162FC;          /* the season's round, race, state, the track and */
  static int D_80316300;          /* the difficulty when the results were entered */
  static int D_80316304;
  static int D_80316308;
  static int D_8031630C;
  static int D_80316310;          /* the season results have been kept */
  static int D_80316314;          /* the results page */
  static char D_80316318[80];
  int p;
  int n;
  int i TGR_MATCHING_UNUSED;
  int k;
  int y;
  N64Car *car;
  char buf[256];
  char time[36];
#if TGR_REGION_JPN
  int total;
#else
  unsigned int round;
#endif
  unsigned int race;
  int tr;
  unsigned int pl;
  int x;
#if TGR_REGION_JPN
  unsigned int round;
#else
  int total;
#endif
  int a;
  int back;
  short v;
  N64PadRec *pad;
  float spin;
  float dt TGR_MATCHING_UNUSED;

  if (g_bCarSelectInitialized == 0) {
#if TGR_REGION_JPN || TGR_REGION_EUR
    if (IsCurrentScreen(ProcessRaceResultsScreen)) {
      g_dwKeepScreenBlanked = 1;
      CARSEL_BLANK();
      StartHighResFrame();
      FlushDisplayList();
      CARSEL_BLANK();
      StartHighResFrame();
      FlushDisplayList();
    }
#endif
    D_80316250 = g_dwElapsedTimeMs;
    SetFadeTarget(1.0f, 0.2f);
    FadeSfxTo(1.0f, 0.2f);
    FadeMusicTo(1.0f, 0.2f);
    D_803162FC = CAR(0)->season->round;
    D_80316300 = CAR(0)->season->race;
    D_80316304 = CAR(0)->season->state;
    D_80316308 = g_dwSelectedTrackId;
    D_8031630C = g_dwCurrentWeather;
    D_80316310 = 0;
    g_dwVehicleViewModeIndex = 0;
    D_80316268[0] = 0.0f;
    D_80316268[1] = 1.0f;
    for (p = 0; p < g_dwNumLocalPlayers; p++) {
      if (g_dwIsReviewMode == 0) {
        InitVehicleColor(CAR(p));
        CAR(p)->xed8 = (int)UpdatePhysicsStep;
      }
      D_80316258[p] = p;
      D_80316260[p] = 0;
      D_803162B0[p] = 0;
      CAR(p)->x2000 = 0;
      *(int *)((char *)CAR(p) + 0x2044) = 0;
      D_803162B8 = 0;
    }
    if (g_dwIsReviewMode == 0) {
#if TGR_REGION_JPN
      do {                      /* a compiled-out statement: its empty block */
      } while (0);              /* puts the car array's base in a3 */
#endif
      for (; p < 4; p++) {
        InitVehicleColor(CAR(p));
        CAR(p)->xed8 = (int)DriveAiVehicle;
      }
    }
    g_dwNumActiveVehicles = 2;
    if (g_dwIsReviewMode == 0) {
      for (p = 0; p < g_dwNumLocalPlayers; p++) {
        ChooseVehicleCar(CAR(p));
        CAR(p)->x2058 = CAR(p)->kind;
        n = CAR(p)->kind;
        while (IsCarUnlocked(n) == 0) {
          n = (n + 1) % g_dwCarCount;
        }
        CAR(p)->kind = n;
        CAR(p)->x2058 = n;
        InstallVehicleModel(CAR(p), p, n);
      }
    }
    /* the two players' turntable frames and the camera that looks at them */
    CAR(0)->mtx0[3][2] = 0.254f;
    CAR(1)->mtx0[3][2] = 0.254f;
    CAR(0)->mtx0[0][0] = 1.0f;
    CAR(0)->mtx0[1][1] = 1.0f;
    CAR(0)->mtx0[2][2] = 1.0f;
    CAR(1)->mtx0[0][0] = 1.0f;
    CAR(1)->mtx0[1][1] = 1.0f;
    CAR(1)->mtx0[2][2] = 1.0f;
    CAR(0)->mtx0[0][2] = 0.0f;
    CAR(0)->mtx0[0][1] = 0.0f;
    CAR(0)->mtx0[1][2] = 0.0f;
    CAR(0)->mtx0[1][0] = 0.0f;
    CAR(0)->mtx0[2][1] = 0.0f;
    CAR(0)->mtx0[2][0] = 0.0f;
    CAR(0)->mtx0[3][1] = 0.0f;
    CAR(0)->mtx0[3][0] = 0.0f;
    CAR(1)->mtx0[0][2] = 0.0f;
    CAR(1)->mtx0[0][1] = 0.0f;
    CAR(1)->mtx0[1][2] = 0.0f;
    CAR(1)->mtx0[1][0] = 0.0f;
    CAR(1)->mtx0[2][1] = 0.0f;
    CAR(1)->mtx0[2][0] = 0.0f;
    CAR(1)->mtx0[3][1] = 0.0f;
    CAR(1)->mtx0[3][0] = 0.0f;
    if (g_dwNumLocalPlayers == 2 && g_dwIsReviewMode == 0) {
      CAR(0)->cams[3].mtx[3][0] = 15.0f;
      CAR(0)->cams[3].mtx[3][1] = 15.0f;
      CAR(0)->cams[3].mtx[3][2] = 7.5f;
    } else {
      CAR(0)->cams[3].mtx[3][0] = 10.0f;
      CAR(0)->cams[3].mtx[3][1] = 10.0f;
      CAR(0)->cams[3].mtx[3][2] = 5.0f;
    }
    Vec3fScale((N64Vec3 *)CAR(0)->cams[3].mtx[0], (N64Vec3 *)CAR(0)->cams[3].mtx[3], -1.0f);
    Vec3fNormalize((N64Vec3 *)CAR(0)->cams[3].mtx[0]);
    CAR(0)->cams[3].mtx[2][0] = 0.0f;
    CAR(0)->cams[3].mtx[2][1] = 0.0f;
    CAR(0)->cams[3].mtx[2][2] = 1.0f;
    Vec3fCross((N64Vec3 *)CAR(0)->cams[3].mtx[1], (N64Vec3 *)CAR(0)->cams[3].mtx[2],
                (N64Vec3 *)CAR(0)->cams[3].mtx[0]);
    Vec3fCross((N64Vec3 *)CAR(0)->cams[3].mtx[2], (N64Vec3 *)CAR(0)->cams[3].mtx[0],
                (N64Vec3 *)CAR(0)->cams[3].mtx[1]);
    memcpy(&CAR(1)->cams[3], &CAR(0)->cams[3], 0x44);
    ResetHeap();
    LoadMenuItemModels(g_apHandlingOptions, &g_dwHandlingOptionCount);
    LoadMenuItemModels(g_apTransmissionOptions, &g_dwTransmissionOptionCount);
    LoadMenuItemModels(g_apTiresOptions, &g_dwTiresOptionCount);
    LoadMenuItemModels(g_apSuspensionOptions, &g_dwSuspensionOptionCount);
    LoadMenuItemModels(g_apDecalOptions, &g_dwDecalListCount);
    LoadAndDecompressAsset(&g_BgTextureDesc, AllocHeapAligned);
    LoadAndDecompressAsset(&g_ArrowLeftTextureDesc, AllocHeapAligned);
    LoadAndDecompressAsset(&g_ArrowRightTextureDesc, AllocHeapAligned);
    LoadAndDecompressAsset(&g_TitleBarTextureDesc, AllocHeapAligned);
    D_803162A0 = DmaReadAssetSize((int)_scrollSoundSegmentRomStart);
    D_8031629C = AllocHeapAligned(D_803162A0 + 0x100);
    DecompressFromRom(D_8031629C, (int)_scrollSoundSegmentRomStart, 0);
    D_803162A8 = DmaReadAssetSize((int)_selectSoundSegmentRomStart);
    D_803162A4 = AllocHeapAligned(D_803162A8 + 0x100);
    DecompressFromRom(D_803162A4, (int)_selectSoundSegmentRomStart, 0);
    for (p = 0; p != 0x100; p++) {
      D_8031629C[D_803162A0 + p] = 0;
      D_803162A4[D_803162A8 + p] = 0;
    }
    AllocateEditorBuffers();
    D_803162AC = 2;
    g_bCarSelectInitialized = 1;
    D_80316270[0] = D_80316270[1] = g_apHandlingOptions;
    D_80316278[0] = D_80316278[1] = 0;
#if TGR_REGION_JPN || TGR_REGION_EUR
    g_dwKeepScreenBlanked = 0;
#endif
#if TGR_REGION_JPN
    D_80316248[0] = D_80316248[1] = 1;
#endif
    D_80316314 = 0;
  }

  /* ---- every frame: the turntables ---- */
  UpdateFrameTiming();
  UpdateFadeTransitions();
  StartHighResFrame();
#if TGR_REGION_JPN
  for (p = 0; p < g_dwNumLocalPlayers; p++) {
    if (D_80316248[p] != 0) {
      D_80316248[p] = 0;
      if (D_803162B0[p] == 0) {
        ExtractVehicleBaseColor(CAR(p), &g_aVehicleModels[D_80316258[p]]);
        if (g_dwNumLocalPlayers == 2) {
          CAR(p)->colour[0] = g_abPlayerColorsRgb[p][0];
          CAR(p)->colour[1] = g_abPlayerColorsRgb[p][1];
          CAR(p)->colour[2] = g_abPlayerColorsRgb[p][2];
        }
        SetMaterialColor((int)&g_aVehicleModels[D_80316258[p]], CAR(p)->colour[0] >> 3, CAR(p)->colour[1] >> 3,
                          CAR(p)->colour[2] >> 3);
      } else if (D_803162B0[p] == 5) {
        ExtractVehicleBaseColor(CAR(p), &g_aVehicleModels[D_80316258[p]]);
      }
      SetMaterialColor((int)&g_aVehicleModels[D_80316258[p]], CAR(p)->colour[0] >> 3, CAR(p)->colour[1] >> 3,
                        CAR(p)->colour[2] >> 3);
    }
  }
#endif
  g_dwWeatherActive = 0;
  g_dwNightMode = 0;
  g_dwRainMode = 0;
  g_dwSnowMode = 0;
  SetupFog();
  SetupWeatherLights();
  AddTimingMarker(0, 0, 0, 200, 0xFF);
  g_aViewportRects[0].x10 = 0;
  g_aViewportRects[1].x10 = 1;
  D_803162AC = 2;
  ClearDepthBuffer();
  if (g_dwIsReviewMode != 0) {
    DrawTexturedRect(&g_BgTextureDesc, 0, 0, 320, 240, 0, 0, 0, 0xFF, 0, 0x82, 0x8C, 0xFF, 4);
  } else {
    DrawTexturedRect(&g_BgTextureDesc, 0, 0, 320, 240, 0, 0, 0, 0xFF, 0x50, 0, 0, 0xFF, 4);
  }
  if (D_803162AC == 0 && IsFadeComplete() != 0) {
    SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
  }
  spin = 6.2831855f;
  dt = g_fFrameDeltaTime;
  D_80316268[0] += g_fFrameDeltaTime;
  D_80316268[1] += 6.2831855f - g_fFrameDeltaTime;
  for (p = 0; p < g_dwNumLocalPlayers; p++) {
    while (spin < D_80316268[p]) {
      D_80316268[p] -= spin;
    }
  }
  for (p = 0; p < g_dwNumLocalPlayers; p++) {
    if (D_80316260[p] < 0.0f) {
#if TGR_REGION_JPN
      D_80316260[p] = D_80316260[p] + (g_fFrameDeltaTime + g_fFrameDeltaTime);
#else
      D_80316260[p] = D_80316260[p] + (dt + dt);
#endif
      if (0.0f < D_80316260[p]) {
        D_80316260[p] = 0.0f;
      }
#if TGR_REGION_JPN
      if (D_803162B0[p] == 5 && D_80316278[p] != 1) {
        D_80316248[p] = 1;
      }
#endif
    } else if (0.0f < D_80316260[p]) {
#if TGR_REGION_JPN
      D_80316260[p] = D_80316260[p] - (g_fFrameDeltaTime + g_fFrameDeltaTime);
#else
      D_80316260[p] = D_80316260[p] - (dt + dt);
#endif
      if (D_80316260[p] < 0.0f) {
        D_80316260[p] = 0.0f;
      }
#if TGR_REGION_JPN
      if (D_803162B0[p] == 5 && D_80316278[p] != 1) {
        D_80316248[p] = 1;
      }
#endif
    }
#if TGR_REGION_JPN
    if (D_80316248[p] == 0 && IsVehicleModelLoaded(STREAM(D_80316258[p])) == 0) {
      D_80316248[p] = -1;
    }
#endif
  }
  /* which model slot each player shows */
  for (p = 0; p < g_dwNumLocalPlayers; p++) {
    if (D_803162B0[p] == 6) {
      if (g_dwIsPaintMode == 0 && g_dwIsReviewMode == 0) {
        D_80316258[p] = (CAR(p)->xe58 << 1) ^ (p ^ 2);
      }
    } else if (D_803162B0[p] == 5) {
      if (D_80316278[p] == 0) {
        D_80316258[p] = p ^ 2;
      } else if (D_80316278[p] == 2) {
        D_80316258[p] = p;
      } else if (D_80316278[p] == 1) {
        if (D_80316280[p] == 0) {
          D_80316258[p] = p;
        } else {
          D_80316258[p] = p ^ 2;
        }
      }
    }
  }
  g_dwNightMode = g_dwRainMode = g_dwWeatherActive = g_dwSnowMode = 0;
#define ICON(p, idx) ((D_803162B0[p] == 0 || D_803162B0[p] == 6) ? 0 : D_80316270[p][D_80316278[idx]]->icon)
#define ICON2(p, idx) ((D_803162B0[p] == 0 || D_803162B0[p] == 6) ? 0 : D_80316270[p][D_80316280[idx]]->icon)
  if (g_dwIsReviewMode != 0) {
    RenderCameraView(ICON(0, 0), ICON2(0, 0), D_80316314 == 1, 0, 0x3B, g_dwScreenWidth, g_dwScreenHeight,
                  D_80316260[0], D_803162B0[0] == 6, D_80316258[D_80316314 == 1],
                  D_80316258[D_80316314 == 1] ^ 2, 12.0f, D_80316268[0]);
  } else {
    if (g_dwNumLocalPlayers == 1) {
      RenderCameraView(ICON(0, 0), ICON2(0, 0), 0, 0, 0x2D, g_dwScreenWidth, g_dwScreenHeight, D_80316260[0],
                    D_803162B0[0] == 6, D_80316258[0], D_80316258[0] ^ 2, 8.0f, D_80316268[0]);
    } else {
      RenderCameraView(ICON(0, 0), ICON2(0, 0), 0, 0, 0, g_dwScreenWidth, g_dwScreenHeight - 24, D_80316260[0],
                    D_803162B0[0] == 6, D_80316258[0], D_80316258[0] ^ 2, 12.0f, D_80316268[0]);
      RenderCameraView(ICON(1, 1), ICON2(1, 1), 1, 0, 0x47, g_dwScreenWidth, g_dwScreenHeight, D_80316260[1],
                    D_803162B0[1] == 6, D_80316258[1], D_80316258[1] ^ 2, 12.0f, D_80316268[1]);
    }
  }
#if TGR_REGION_JPN
  for (p = 0; p < g_dwNumLocalPlayers; p++) {
    if (D_80316248[p] == -1) {
      if (IsVehicleModelLoaded(STREAM(D_80316258[p]))) {
        D_80316248[p] = 1;
      } else {
        D_80316248[p] = 0;
      }
    }
  }
#endif
  SetupMenuProjection((float)g_dwScreenWidth, (float)g_dwScreenHeight);
  SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);

  /* ---- the title ---- */
  if (D_803162AC != 0 || IsFadeComplete() == 0) {
#if TGR_REGION_JPN
    DrawTexturedRect(&g_TitleBarTextureDesc, 0x19, 0xE, 0x118, 0x20, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 2);
#else
    DrawTexturedRect(&g_TitleBarTextureDesc, 0x16, 9, 0x118, 0x2A, 0, 0, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 2);
#endif
    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(30);
    if (g_dwIsReviewMode != 0) {
      if (D_80316314 == 0 || D_80316314 == 1) {
#if TGR_REGION_JPN
        sprintf(buf, "%%ry%s%s %%j%c", D_80316308 >= 5 ? "M-" : "", g_apTrackMenuItems[D_80316308 >= 5 ? D_80316308 - 5 : D_80316308]->item.label, D_8031630C + 0x29);
#else
        sprintf(buf, "%%ry%s%s/%s", D_80316308 >= 5 ? "M-" : "", g_apTrackMenuItems[D_80316308 >= 5 ? D_80316308 - 5 : D_80316308]->item.label, g_apWeatherMenuItems[D_8031630C]->label);
#endif

        DrawText(buf, g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);
      } else {
        sprintf(buf, "%%ry%s", *(char **)&g_SeasonTable[D_803162FC]);
        DrawText(buf, g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);
      }
    } else if (g_dwIsPaintMode != 0) {
      DrawText("%ryPaint car select", g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);
    } else {
      DrawText("%ryCar Select", g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);
    }
    if (IsFadeComplete() != 0) {
      D_803162AC--;
    }
  }

  /* ---- each player's panel, or the results ---- */
  if (g_dwIsReviewMode != 0) {
    y = 0x4F;
    race = D_80316300;
    round = D_803162FC;
    p = 0;
    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(20);
    if (D_80316314 == 0 || D_80316314 == 1) {
      /* a player's race: the place, then each lap and the race time */
      pl = D_80316314 == 0 ? 0 : 1;
      if (g_dwNumLocalPlayers >= 2) {
        car = CAR(pl);
        if (car->laps < g_dwTotalLaps) {
#if TGR_REGION_JPN
          sprintf(D_80316318, "%%ryP%d:GAME OVER", pl + 1);
#else
          sprintf(D_80316318, "%%ryP%d: Did not finish", pl + 1);
#endif
        } else {
          sprintf(D_80316318, "%%ryP%d: %s Place", pl + 1, g_apszPlaceNames[car->xfac]);
        }
      } else {
        if (g_dwGameMode == 0) {
          car = CAR(pl);
          k = CAR(pl)->season->place[round][race];
          sprintf(D_80316318, "%%ry%s Place - %d Point%s", g_apszPlaceNames[k], g_abRacePositionPoints[k],
                  g_abRacePositionPoints[k] == 1 ? "" : "s");
        } else {
          car = CAR(pl);
          if (car->laps < g_dwTotalLaps) {
#if TGR_REGION_JPN
            sprintf(D_80316318, "%%ryGAME OVER");
#else
            sprintf(D_80316318, "%%ryDid not finish");
#endif
          } else {
            sprintf(D_80316318, "%%ry%s Place", g_apszPlaceNames[car->xfac]);
          }
        }
      }
      DrawText(D_80316318, g_dwScreenWidth / 2, g_dwScreenHeight * 19 / 64 - 4);
      SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
      SetTextSize(10);
      for (; p <= g_dwTotalLaps; p++) {
        SetTextAlignLeft();
        if (p == g_dwTotalLaps) {
          y += 2;
          if (car->lapTime == CAR(0)->season->xe8[D_80316308]) {
            DrawText("  %yyRECORD TIME!", g_dwScreenWidth * 11 / 16, y);
          }
          sprintf(D_80316318, "Race time:");
          FormatLapTime(time, car->lapTime);
        } else {
          sprintf(D_80316318, "Lap %d time:", p + 1);
          if (p < car->laps) {
            FormatLapTime(time, CAR(pl)->lapTimes[p]);
          } else {
            sprintf(time, "--    ");
          }
        }
        DrawText(D_80316318, g_dwScreenWidth * 5 / 16, y);
        if (p < car->laps && p == car->xf9c) {
          if (car->xf98 == CAR(0)->season->x8c[D_80316308]) {
            DrawText("  %yyRECORD LAP!", g_dwScreenWidth * 11 / 16, y);
          } else {
            DrawText("  %yyBEST LAP!", g_dwScreenWidth * 11 / 16, y);
          }
        }
        SetTextAlignRight();
        DrawText(time, g_dwScreenWidth * 11 / 16, y);
        y += 10;
      }
    } else if (D_80316314 == 3) {
      /* the season news */
      SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
      SetTextSize(12);
      y = 0x62 - g_dwSeasonResultMsgLineCount * 120 / 20;
      SetTextAlignCenter();
      for (p = 0; p < g_dwSeasonResultMsgLineCount; p++) {
        DrawText(g_apszSeasonResultMsgLines[p], g_dwScreenWidth / 2, y);
        y += 12;
      }
    } else {
      /* the season so far: each race, its time, place and points */
      DrawText("%rySeason Results", g_dwScreenWidth / 2, g_dwScreenHeight * 19 / 64 - 4);
      SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
      SetTextSize(10);
      x = 0;
      for (p = 0; p < g_SeasonTable[round].x8; p++) {
        if (g_SeasonTable[round].races[p][0] < 5) {
          if (D_80316304 & 1) {
            x = g_dwScreenWidth / 16;
            break;
          }
        } else if ((D_80316304 & 1) == 0) {
          x = g_dwScreenWidth / 16;
          break;
        }
      }
      total = 0;
      for (p = 0; p < g_SeasonTable[round].x8; p++) {
        tr = g_SeasonTable[round].races[p][0];
        if (D_80316304 & 1) {
          if (tr < 5) {
            tr += 5;
          } else {
            tr -= 5;
          }
        }
        SetTextAlignLeft();
#if TGR_REGION_JPN
        sprintf(D_80316318, "%%j%c", g_SeasonTable[round].races[p][1] + 0x29);
        SetTextSize(13);
        DrawText(D_80316318, g_dwScreenWidth * 3 / 16 - x, y + 1);
        SetTextSize(10);
        DrawText((const char *)g_apTrackMenuItems[tr]->item.label,
                 g_dwScreenWidth * 3 / 16 - x + 12, y);
#else
        sprintf(D_80316318, "%s/%s", g_apTrackMenuItems[tr]->item.label,
                g_apWeatherMenuItems[g_SeasonTable[round].races[p][1]]->label);
        DrawText(D_80316318, g_dwScreenWidth * 3 / 16 - x, y);
#endif
        SetTextAlignCenter();
        if (p <= (int)race) {
          FormatLapTime(D_80316318, CAR(0)->season->times[round][p]);
        } else {
          sprintf(D_80316318, "--");
        }
        DrawText(D_80316318, g_dwScreenWidth * 17 / 32 + x, y);
        DrawText(p <= (int)race ? g_apszPlaceNames[CAR(0)->season->place[round][p]] : "--",
                    g_dwScreenWidth * 21 / 32 + x, y);
        SetTextAlignRight();
        if (p <= (int)race) {
          sprintf(D_80316318, "%d PTS",
                  g_abRacePositionPoints[CAR(0)->season->place[round][p]]);
          total += g_abRacePositionPoints[CAR(0)->season->place[round][p]];
        }
        DrawText(D_80316318, g_dwScreenWidth * 13 / 16 + x, y);
        y += 10;
      }
      y += 2;
      sprintf(D_80316318, "TOTAL:    %d PTS ", total);
      DrawText(D_80316318, g_dwScreenWidth * 13 / 16 + x, y);
    }
  } else {
    if (g_dwNumLocalPlayers == 2) {
      SetTextSize(17);
      if (D_803162B0[0] == 0) {
        sprintf(buf, "%%ry%%i%s", *(char **)&g_aCarModels[CAR(0)->x2058]);
      } else if (D_803162B0[0] == 6) {
        sprintf(buf, "%%ryREADY!");
      } else {
        sprintf(buf, "%%ry%s: %%i%s", D_80316290[0], D_80316270[0][D_80316278[0]]->label);
      }
      DrawText(buf, g_dwScreenWidth / 2, g_dwScreenHeight / 4);
      if (D_803162B0[1] == 0) {
        sprintf(buf, "%%ry%%i%s", *(char **)&g_aCarModels[CAR(1)->x2058]);
      } else if (D_803162B0[1] == 6) {
        sprintf(buf, "%%ryREADY!");
      } else {
        sprintf(buf, "%%ry%s: %%i%s", D_80316290[1], D_80316270[1][D_80316278[1]]->label);
      }
      DrawText(buf, g_dwScreenWidth / 2, g_dwScreenHeight * 3 / 5);
#if TGR_REGION_JPN
      SetTextSize(14);
      SetTextAlignLeft();
      if (D_803162B0[0] == 5 && D_80316278[0] != 1 && D_80316260[0] == 0) {
        DrawText("%ww%j\xD2\xD3" "2b3@sA5'", g_dwScreenWidth / 16, g_dwScreenHeight / 4 + 22);
        DrawText("%ww%j\xD2\xD3\x8D\x8F\xA1w", g_dwScreenWidth / 16, g_dwScreenHeight / 4 + 34);
        DrawText("%ww%j2Bc\xDB" "2@&", g_dwScreenWidth / 16, g_dwScreenHeight / 4 + 47);
      }
      if (D_803162B0[1] == 5 && D_80316278[1] != 1 && D_80316260[1] == 0) {
        DrawText("%ww%j\xD2\xD3" "2b3@sA5'", g_dwScreenWidth / 16, g_dwScreenHeight * 3 / 5 + 22);
        DrawText("%ww%j\xD2\xD3\x8D\x8F\xA1w", g_dwScreenWidth / 16, g_dwScreenHeight * 3 / 5 + 34);
        DrawText("%ww%j2Bc\xDB" "2@&", g_dwScreenWidth / 16, g_dwScreenHeight * 3 / 5 + 47);
      }
      SetTextSize(8);
#else
      SetTextSize(8);
      SetTextAlignLeft();
      if (D_803162B0[0] == 5 && D_80316278[0] != 1 && D_80316260[0] == 0) {
        DrawText("%wwIf you wish to use", g_dwScreenWidth * 11 / 16, g_dwScreenHeight / 4 + 19);
        DrawText("%wwthe Rumble Pak, you", g_dwScreenWidth * 11 / 16, g_dwScreenHeight / 4 + 28);
        DrawText("%wwshould ensure that it", g_dwScreenWidth * 11 / 16, g_dwScreenHeight / 4 + 37);
        DrawText("%wwis plugged in before", g_dwScreenWidth * 11 / 16, g_dwScreenHeight / 4 + 45);
        DrawText("%wwpressing A.", g_dwScreenWidth * 11 / 16, g_dwScreenHeight / 4 + 54);
      }
      if (D_803162B0[1] == 5 && D_80316278[1] != 1 && D_80316260[1] == 0) {
        DrawText("%wwIf you wish to use", g_dwScreenWidth * 11 / 16, g_dwScreenHeight * 3 / 5 + 19);
        DrawText("%wwthe Rumble Pak, you", g_dwScreenWidth * 11 / 16, g_dwScreenHeight * 3 / 5 + 28);
        DrawText("%wwshould ensure that it", g_dwScreenWidth * 11 / 16, g_dwScreenHeight * 3 / 5 + 37);
        DrawText("%wwis plugged in before", g_dwScreenWidth * 11 / 16, g_dwScreenHeight * 3 / 5 + 45);
        DrawText("%wwpressing A.", g_dwScreenWidth * 11 / 16, g_dwScreenHeight * 3 / 5 + 54);
      }
#endif
      SetTextAlignRight();
      if (D_803162B0[0] == 0) {
        car = CAR(0);
        SetTextAlignRight();
        DrawText("%wwAcceleration", g_dwScreenWidth / 4 - 5, g_dwScreenHeight / 4 + 2);
        DrawText("%wwMax Speed", g_dwScreenWidth / 4 - 5, g_dwScreenHeight / 4 - 6);
        SetTextAlignCenter();
        DrawText(g_apszEngineLayouts[*(int *)((char *)&g_aCarModels[car->x2058] + 0x40)],
                    g_dwScreenWidth * 3 / 4, g_dwScreenHeight / 4 + 2);
        sprintf(buf, "%%ww%d-Speed %s", *(int *)((char *)&g_aCarModels[car->x2058] + 0x54),
                g_apszDriveTypes[*(int *)((char *)&g_aCarModels[car->x2058] + 0x3C)]);
        DrawText(buf, g_dwScreenWidth * 3 / 4, g_dwScreenHeight / 4 - 6);
        DrawProgressBar(g_dwScreenWidth / 2, g_dwScreenHeight / 2 - 23, 80, 11,
                         *(float *)((char *)&g_aCarModels[car->x2058] + 0x44) / 500.0);
        DrawProgressBar(g_dwScreenWidth / 2, g_dwScreenHeight / 2 - 6, 80, 11,
                         *(float *)((char *)&g_aCarModels[car->x2058] + 0x48) / 10.0);
      }
      if (D_803162B0[1] == 0) {
        car = CAR(1);
        SetTextAlignRight();
        DrawText("%wwAcceleration", g_dwScreenWidth / 4 - 5, g_dwScreenHeight * 3 / 5 + 2);
        DrawText("%wwMax Speed", g_dwScreenWidth / 4 - 5, g_dwScreenHeight * 3 / 5 - 6);
        SetTextAlignCenter();
        DrawText(g_apszEngineLayouts[*(int *)((char *)&g_aCarModels[car->x2058] + 0x40)],
                    g_dwScreenWidth * 3 / 4, g_dwScreenHeight * 3 / 5 + 2);
        sprintf(buf, "%%ww%d-Speed %s", *(int *)((char *)&g_aCarModels[car->x2058] + 0x54),
                g_apszDriveTypes[*(int *)((char *)&g_aCarModels[car->x2058] + 0x3C)]);
        DrawText(buf, g_dwScreenWidth * 3 / 4, g_dwScreenHeight * 3 / 5 - 6);
        DrawProgressBar(g_dwScreenWidth / 2, g_dwScreenHeight * 6 / 5 - 23, 80, 11,
                         *(float *)((char *)&g_aCarModels[car->x2058] + 0x44) / 500.0);
        DrawProgressBar(g_dwScreenWidth / 2, g_dwScreenHeight * 6 / 5 - 6, 80, 11,
                         *(float *)((char *)&g_aCarModels[car->x2058] + 0x48) / 10.0);
      }
    } else {
      SetTextSize(20);
      if (g_dwIsPaintMode != 0) {
        sprintf(buf, "%%ry%%i%s", *(char **)&g_aCarModels[CAR(0)->x2058]);
        DrawText(buf, g_dwScreenWidth / 2, g_dwScreenHeight * 22 / 64);
      } else {
        if (D_803162B0[0] == 0) {
          sprintf(buf, "%%ry%%i%s", *(char **)&g_aCarModels[CAR(0)->x2058]);
        } else if (D_803162B0[0] == 6) {
          sprintf(buf, "%%ryLET'S GO!");
        } else {
          sprintf(buf, "%%ry%s: %%i%s", D_80316290[0], D_80316270[0][D_80316278[0]]->label);
        }
        DrawText(buf, g_dwScreenWidth / 2, g_dwScreenHeight * 20 / 64);
        SetTextSize(8);
        SetTextAlignCenter();
#if TGR_REGION_JPN
        if (D_803162B0[0] == 5 && D_80316278[0] != 1 && D_80316260[0] == 0) {
          SetTextSize(14);
          DrawText("%ww %j\xD2\xD3" "2b3@sA5'\xD2\xD3\x8D\x8F\xA1w2Bc\xDB" "2@&", g_dwScreenWidth / 2,
                      g_dwScreenHeight * 3 / 8 - 1);
          SetTextSize(8);
        }
#else
        if (D_803162B0[0] == 5 && D_80316278[0] != 1 && D_80316260[0] == 0) {
          DrawText("%wwIf you wish to use the Rumble Pak, you should", g_dwScreenWidth / 2,
                      g_dwScreenHeight * 3 / 8 - 6);
          DrawText("%wwensure that it is plugged in before pressing A.", g_dwScreenWidth / 2,
                      g_dwScreenHeight * 3 / 8 + 2);
        }
#endif
        if (D_803162B0[0] == 0) {
          car = CAR(0);
          SetTextAlignRight();
          SetTextSize(8);
          DrawText("%wwMax Speed", g_dwScreenWidth * 3 / 8, g_dwScreenHeight * 3 / 8 - 6);
          DrawText("%wwAcceleration", g_dwScreenWidth * 3 / 8, g_dwScreenHeight * 3 / 8 + 2);
          SetTextAlignCenter();
          DrawText(g_apszEngineLayouts[*(int *)((char *)&g_aCarModels[car->x2058] + 0x40)],
                      g_dwScreenWidth * 11 / 16, g_dwScreenHeight * 3 / 8 - 6);
          sprintf(buf, "%%ww%d-Speed %s", *(int *)((char *)&g_aCarModels[car->x2058] + 0x54),
                  g_apszDriveTypes[*(int *)((char *)&g_aCarModels[car->x2058] + 0x3C)]);
          DrawText(buf, g_dwScreenWidth * 11 / 16, g_dwScreenHeight * 3 / 8 + 2);
          DrawProgressBar(g_dwScreenWidth * 3 / 4 + 10, g_dwScreenHeight * 3 / 4 - 23, 80, 11,
                           *(float *)((char *)&g_aCarModels[car->x2058] + 0x44) / 500.0);
          DrawProgressBar(g_dwScreenWidth * 3 / 4 + 10, g_dwScreenHeight * 3 / 4 - 6, 80, 11,
                           *(float *)((char *)&g_aCarModels[car->x2058] + 0x48) / 10.0);
        }
      }
    }
  }
  if (D_803162AC != 0 || IsFadingOut() != 0 || IsFadingIn() != 0) {
    if (g_dwIsReviewMode != 0) {
      DrawPauseMenuButtons();
    } else {
      DrawMenuFooterButtons();
    }
  }
  ApplyScreenFade();
  if (1 < g_dwHudDebugLevel) {
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, 1);
    DrawTimingBars();
  }

  /* ---- the decal load from the Controller Pak ---- */
  if (D_803162B8 != 0) {
    if (D_803162C0 == 0) {
      if (D_803162C4 == 0) {
        D_803162C0 = ProcessDecalSaveLoad(&g_aVehicleModels[D_803162BC], D_803162BC, 9, 1, &D_803162C4);
      }
    } else if (D_803162C0 == 999) {
      D_803162C4 = 1;
    } else {
      DrawPakErrorMessage(D_803162C0, 9, 1);
      if (((N64PadRec *)CAR(D_803162BC)->pad)->pressed & 0x30) {
        TransferFlags((N64PadRec *)CAR(D_803162BC)->pad, 0x30);
        D_803162C4 = 1;
      }
    }
    if (D_803162C4 != 0) {
      if (D_803162C0 != 0) {
        D_80316288[D_803162BC] = g_dwDecalListCount - 1;
        D_803162C0 = 0;
      } else {
        D_80316288[D_803162BC] = g_dwDecalListCount;
        D_80316280[D_803162BC] = D_80316278[D_803162BC];
        D_80316278[D_803162BC] = (D_80316278[D_803162BC] + 1) % g_dwDecalListCount;
        D_80316260[D_803162BC] = 1.0f;
        ExtractVehicleBaseColor(CAR(D_803162BC), &g_aVehicleModels[D_803162BC]);
      }
    }
  }
  FlushDisplayList();

  /* ---- the pads ---- */
  if (IsFadingOut() != 0) {
    if (IsFadeComplete() != 0) {
      /* faded out: on to the next screen */
      g_bCarSelectInitialized = 0;
      ClearFramebuffersForWideMode();
      for (back = 0; back < 6; back++) {
        g_aSoundEffectChannels[back].level = 0;
        g_aSoundEffectChannels[back].x10 = 0;
        g_aSoundEffectChannels[back].pitch = 0;
        g_aSoundEffectChannels[back].p = g_SilenceSampleData;
      }
      if (D_80316298 != 0) {
        if (g_dwIsReviewMode != 0) {
          if (g_dwGameMode == 0) {
            g_dwNumLocalPlayers = 1;
            g_dwContinueFromResults = CAR(0)->season->race != 0;
            if (D_803162FC == CAR(0)->season->round) {
              SetTrackFromSeason();
              g_bSaveLoadEntryPoint = 1;
              SetCurrentScreenCallback((int)ProcessDataSaveLoadMenu);
            } else {
              ClearFramebuffersLowRes();
              g_dwGameMode = 5;
              SetCurrentScreenCallback((int)ProcessRacingScreen);
            }
          } else if (g_dwGameMode == 2 && g_dwSaveAvailable != 0) {
            g_bSaveLoadEntryPoint = 2;
            SetCurrentScreenCallback((int)ProcessDataSaveLoadMenu);
          } else {
            SetCurrentScreenCallback((int)ProcessMainMenu);
          }
        } else if (g_dwIsPaintMode != 0) {
          SetCurrentScreenCallback((int)ProcessPaintShopScreen);
        } else {
          ClearFramebuffersLowRes();
          SetCurrentScreenCallback((int)ProcessRacingScreen);
        }
      } else if (g_dwIsReviewMode != 0) {
        if (g_dwGameMode == 0 && D_80316310 == 0) {
          ProcessSeasonResults();
        }
        SetCurrentScreenCallback((int)ProcessMainMenu);
      } else if (g_dwIsPaintMode != 0) {
        SetCurrentScreenCallback((int)ProcessMainMenu);
      } else if (g_dwGameMode == 0 || g_dwGameMode == 2) {
        g_dwContinueFromResults = 1;
        SetCurrentScreenCallback((int)ProcessTrackMenu);
      } else {
        SetCurrentScreenCallback((int)ProcessWeatherSelect);
      }
    }
  } else {
    if (D_803162B8 != 0) {
      if (D_803162C4 != 0) {
        D_803162C4 = 0;
        D_803162B8 = 0;
      }
    } else {
      if (g_dwIsPaintMode != 0) {
        /* the paint shop's player drives the screen */
        ((N64PadRec *)CAR(0)->pad)->pressed = ((N64PadRec *)CAR(g_dwScreenPlayerIdx)->pad)->pressed;
        ((N64PadRec *)CAR(0)->pad)->axis[0] = ((N64PadRec *)CAR(g_dwScreenPlayerIdx)->pad)->axis[0];
        ((N64PadRec *)CAR(0)->pad)->axis[1] = ((N64PadRec *)CAR(g_dwScreenPlayerIdx)->pad)->axis[1];
      }
      for (p = 0; p < g_dwNumLocalPlayers; p++) {
        car = CAR(p);
        back = 1;
        if (g_dwIsReviewMode != 0) {
          ((N64PadRec *)PCAR->pad)->pressed &= ~0xF;
        } else {
          UpdateStickRepeat((N64PadRec *)PCAR->pad);
        }
        if (D_803162B0[p] == 6) {
          if (((N64PadRec *)CAR(p)->pad)->pressed & 0x20) {
            D_803162B0[p]--;
            TransferFlags((N64PadRec *)CAR(p)->pad, 0x20);
          }
        } else if (D_80316260[p] == 0) {
          /* the car (step 0): left and right pick the next selectable car */
          if (D_803162B0[p] == 0) {
            if (((N64PadRec *)PCAR->pad)->pressed & 4) {
              n = PCAR->x2058;
              do {
                PCAR->x2058 = (PCAR->x2058 + g_dwCarCount - 1) % g_dwCarCount;
              } while (IsCarUnlocked(PCAR->x2058) == 0);
              if (n != PCAR->x2058) {
                D_80316260[p] = 1.0f;
                goto newcar;
              }
            }
            if (((N64PadRec *)PCAR->pad)->pressed & 1) {
              n = PCAR->x2058;
              do {
                PCAR->x2058 = (PCAR->x2058 + 1) % g_dwCarCount;
              } while (IsCarUnlocked(PCAR->x2058) == 0);
              if (n != PCAR->x2058) {
                D_80316260[p] = -1.0f;
              newcar:
                v = FindFreeSoundChannel();
                if (v != -1) {
                  InitSoundChannel(v, D_8031629C, D_803162A0, 0);
                }
                D_80316258[p] ^= 2;
                LoadVehicleModel(STREAM(D_80316258[p]), D_80316258[p], PCAR->x2058, p);
#ifdef ENABLE_PATCHES
                while (PollVehicleModelDma(STREAM(D_80316258[p]))) {
                }
#endif
              }
            }
          }
#if TGR_REGION_JPN || TGR_REGION_EUR
#if TGR_REGION_JPN
          /* the decal choice (step 5): left and right */
          if (D_803162B0[p] == 5) {
            if (((N64PadRec *)CAR(p)->pad)->pressed & 4) {
              D_80316280[p] = D_80316278[p];
              D_80316278[p] = (D_80316278[p] + D_80316288[p] - 1) % D_80316288[p];
              D_80316260[p] = 1.0f;
              goto tick5;
            }
            if (((N64PadRec *)CAR(p)->pad)->pressed & 1) {
              D_80316280[p] = D_80316278[p];
              D_80316278[p] = (D_80316278[p] + 1) % D_80316288[p];
              D_80316260[p] = -1.0f;
            tick5:
              v = FindFreeSoundChannel();
              if (v != -1) {
                InitSoundChannel(v, D_8031629C, D_803162A0, 0);
              }
            }
          }
#endif
          if (D_80316260[p] != 0.f) {
            continue;
          }
          /* the colour, on the car and on the decals */
          if (D_803162B0[p] == 0 || D_803162B0[p] == 5) {
            if (((N64PadRec *)car->pad)->pressed & 0x400) {
              TransferFlags((N64PadRec *)car->pad, 0x400);
              if (car->season->unlocked & 0x8000) {
                car->x2068 ^= 1;
              }
            }
            a = ((N64PadRec *)car->pad)->pressed;
            if (a & 8) {
              if (a & 0x800) {
                if (CAR(p)->colour[0] < 0xF8) {
                  CAR(p)->colour[0] += 8;
                  a = ((N64PadRec *)car->pad)->pressed;
                } else {
                  CAR(p)->colour[0] = 0xFF;
                  a = ((N64PadRec *)car->pad)->pressed;
                }
              }
              if (a & 0x100) {
                if (CAR(p)->colour[1] < 0xF8) {
                  CAR(p)->colour[1] += 8;
                  a = ((N64PadRec *)car->pad)->pressed;
                } else {
                  CAR(p)->colour[1] = 0xFF;
                  a = ((N64PadRec *)car->pad)->pressed;
                }
              }
              if (a & 0x200) {
                if (CAR(p)->colour[2] < 0xF8) {
                  CAR(p)->colour[2] += 8;
                  a = ((N64PadRec *)car->pad)->pressed;
                } else {
                  CAR(p)->colour[2] = 0xFF;
                  a = ((N64PadRec *)car->pad)->pressed;
                }
              }
            }
            if (a & 2) {
              if (a & 0x800) {
                if (CAR(p)->colour[0] >= 8) {
                  CAR(p)->colour[0] -= 8;
                  a = ((N64PadRec *)car->pad)->pressed;
                } else {
                  CAR(p)->colour[0] = 0;
                  a = ((N64PadRec *)car->pad)->pressed;
                }
              }
              if (a & 0x100) {
                if (CAR(p)->colour[1] >= 8) {
                  CAR(p)->colour[1] -= 8;
                  a = ((N64PadRec *)car->pad)->pressed;
                } else {
                  CAR(p)->colour[1] = 0;
                  a = ((N64PadRec *)car->pad)->pressed;
                }
              }
              if (a & 0x200) {
                if (CAR(p)->colour[2] >= 8) {
                  CAR(p)->colour[2] -= 8;
                } else {
                  CAR(p)->colour[2] = 0;
                }
              }
            }
            SetMaterialColor((int)&g_aVehicleModels[D_80316258[p]], CAR(p)->colour[0] >> 3, CAR(p)->colour[1] >> 3,
                              CAR(p)->colour[2] >> 3);
          }
#if TGR_REGION_JPN
          /* the setup choice (steps 1-4): left and right */
          if (D_803162B0[p] != 0 && D_803162B0[p] != 5) {
#else
          /* the setup choice (steps 1-5): left and right */
          if (D_803162B0[p] != 0) {
#endif
            if (((N64PadRec *)PCAR->pad)->pressed & 4) {
              D_80316280[p] = D_80316278[p];
              D_80316278[p] = (D_80316278[p] + D_80316288[p] - 1) % D_80316288[p];
              D_80316260[p] = 1.0f;
              goto tick;
            }
            if (((N64PadRec *)PCAR->pad)->pressed & 1) {
              D_80316280[p] = D_80316278[p];
              D_80316278[p] = (D_80316278[p] + 1) % D_80316288[p];
              D_80316260[p] = -1.0f;
            tick:
              v = FindFreeSoundChannel();
              if (v != -1) {
                InitSoundChannel(v, D_8031629C, D_803162A0, 0);
              }
            }
          }
#else
          /* the colour, on the car and on the decals */
          if (D_803162B0[p] == 0 || D_803162B0[p] == 5) {
            if (((N64PadRec *)PCAR->pad)->pressed & 0x400) {
              TransferFlags((N64PadRec *)PCAR->pad, 0x400);
              if (PCAR->season->unlocked & 0x8000) {
                PCAR->x2068 ^= 1;
              }
            }
            a = ((N64PadRec *)PCAR->pad)->pressed;
            if (a & 8) {
              if (a & 0x800) {
                if (CAR(p)->colour[0] < 0xFC) {
                  CAR(p)->colour[0] += 4;
                  a = ((N64PadRec *)PCAR->pad)->pressed;
                } else {
                  CAR(p)->colour[0] = 0xFF;
                  a = ((N64PadRec *)PCAR->pad)->pressed;
                }
              }
              if (a & 0x100) {
                if (CAR(p)->colour[1] < 0xFC) {
                  CAR(p)->colour[1] += 4;
                  a = ((N64PadRec *)PCAR->pad)->pressed;
                } else {
                  CAR(p)->colour[1] = 0xFF;
                  a = ((N64PadRec *)PCAR->pad)->pressed;
                }
              }
              if (a & 0x200) {
                if (CAR(p)->colour[2] < 0xFC) {
                  CAR(p)->colour[2] += 4;
                } else {
                  CAR(p)->colour[2] = 0xFF;
                }
              }
              SetMaterialColor((int)PCAR->model, CAR(p)->colour[0] >> 3, CAR(p)->colour[1] >> 3,
                                CAR(p)->colour[2] >> 3);
              a = ((N64PadRec *)PCAR->pad)->pressed;
            }
            if (a & 2) {
              if (a & 0x800) {
                if (CAR(p)->colour[0] >= 4) {
                  CAR(p)->colour[0] -= 4;
                  a = ((N64PadRec *)PCAR->pad)->pressed;
                } else {
                  CAR(p)->colour[0] = 0;
                  a = ((N64PadRec *)PCAR->pad)->pressed;
                }
              }
              if (a & 0x100) {
                if (CAR(p)->colour[1] >= 4) {
                  CAR(p)->colour[1] -= 4;
                  a = ((N64PadRec *)PCAR->pad)->pressed;
                } else {
                  CAR(p)->colour[1] = 0;
                  a = ((N64PadRec *)PCAR->pad)->pressed;
                }
              }
              if (a & 0x200) {
                if (CAR(p)->colour[2] >= 4) {
                  CAR(p)->colour[2] -= 4;
                } else {
                  CAR(p)->colour[2] = 0;
                }
              }
              SetMaterialColor((int)PCAR->model, CAR(p)->colour[0] >> 3, CAR(p)->colour[1] >> 3,
                                CAR(p)->colour[2] >> 3);
            }
          }
          /* the setup choice (steps 1-5): left and right */
          if (D_803162B0[p] != 0) {
            if (((N64PadRec *)PCAR->pad)->pressed & 4) {
              D_80316280[p] = D_80316278[p];
              D_80316278[p] = (D_80316278[p] + D_80316288[p] - 1) % D_80316288[p];
              D_80316260[p] = 1.0f;
              goto tick;
            }
            if (((N64PadRec *)PCAR->pad)->pressed & 1) {
              D_80316280[p] = D_80316278[p];
              D_80316278[p] = (D_80316278[p] + 1) % D_80316288[p];
              D_80316260[p] = -1.0f;
            tick:
              v = FindFreeSoundChannel();
              if (v != -1) {
                InitSoundChannel(v, D_8031629C, D_803162A0, 0);
              }
            }
          }
#endif
          pad = (N64PadRec *)PCAR->pad;
          if (pad->pressed & 0x20) {
            /* back */
            TransferFlags(pad, 0x20);
            if (D_803162B0[p] == 0) {
              D_80316298 = 0;
              FadeSfxTo(0.0f, 0.2f);
              SetFadeTarget(0.0f, 0.2f);
            } else {
              D_803162B0[p]--;
              switch (D_803162B0[p]) {
              case 1:
                D_80316278[p] = PCAR->season->xd4;
#if TGR_REGION_JPN
                D_80316270[p] = g_apHandlingOptions;
                D_80316288[p] = g_dwHandlingOptionCount;
                D_80316290[p] = "Handling";
#else
                SETUP(p, g_apHandlingOptions, g_dwHandlingOptionCount, "Handling");
#endif
                break;
              case 2:
                D_80316278[p] = PCAR->season->xd8;
#if TGR_REGION_JPN
                D_80316270[p] = g_apTransmissionOptions;
                D_80316288[p] = g_dwTransmissionOptionCount;
                D_80316290[p] = "Transmission";
#else
                SETUP(p, g_apTransmissionOptions, g_dwTransmissionOptionCount, "Transmission");
#endif
                break;
              case 3:
                D_80316278[p] = PCAR->season->xdc;
#if TGR_REGION_JPN
                D_80316270[p] = g_apTiresOptions;
                D_80316288[p] = g_dwTiresOptionCount;
                D_80316290[p] = "TIRE";
#else
                SETUP(p, g_apTiresOptions, g_dwTiresOptionCount, TIRES_TITLE);
#endif
                break;
              case 4:
                D_80316278[p] = PCAR->season->xe0;
#if TGR_REGION_JPN
                D_80316270[p] = g_apSuspensionOptions;
                D_80316288[p] = g_dwSuspensionOptionCount;
                D_80316290[p] = "Suspension";
#else
                SETUP(p, g_apSuspensionOptions, g_dwSuspensionOptionCount, "Suspension");
#endif
                break;
              case 5:
                D_80316278[p] = 0;
                D_80316290[p] = DECALS_TITLE;
                break;
              }
              D_80316280[p] = D_80316278[p];
            }
            goto sound;
          } else if (pad->pressed & 0xC010) {
            /* on to the next step */
            TransferFlags(pad, 0xC010);
            switch (D_803162B0[p]) {
            case 0:
              PCAR->kind = PCAR->x2058;
              D_80316278[p] = PCAR->season->xd4;
#if TGR_REGION_JPN
              D_80316270[p] = g_apHandlingOptions;
              D_80316288[p] = g_dwHandlingOptionCount;
              D_80316290[p] = "Handling";
#else
              SETUP(p, g_apHandlingOptions, g_dwHandlingOptionCount, "Handling");
#endif
              break;
            case 1:
              PCAR->season->xd4 = D_80316278[p];
              D_80316278[p] = PCAR->season->xd8;
#if TGR_REGION_JPN
              D_80316270[p] = g_apTransmissionOptions;
              D_80316288[p] = g_dwTransmissionOptionCount;
              D_80316290[p] = "Transmission";
#else
              SETUP(p, g_apTransmissionOptions, g_dwTransmissionOptionCount, "Transmission");
#endif
              break;
            case 2:
              PCAR->season->xd8 = D_80316278[p];
              D_80316278[p] = PCAR->season->xdc;
#if TGR_REGION_JPN
              D_80316270[p] = g_apTiresOptions;
              D_80316288[p] = g_dwTiresOptionCount;
              D_80316290[p] = TIRES_TITLE;
#else
              SETUP(p, g_apTiresOptions, g_dwTiresOptionCount, TIRES_TITLE);
#endif
              break;
            case 3:
              PCAR->season->xdc = D_80316278[p];
              D_80316278[p] = PCAR->season->xe0;
#if TGR_REGION_JPN
              D_80316270[p] = g_apSuspensionOptions;
              D_80316288[p] = g_dwSuspensionOptionCount;
              D_80316290[p] = "Suspension";
#else
              SETUP(p, g_apSuspensionOptions, g_dwSuspensionOptionCount, "Suspension");
#endif
              break;
            case 4:
              PCAR->season->xe0 = D_80316278[p];
              D_80316278[p] = 0;
              D_80316270[p] = g_apDecalOptions;
#if TGR_REGION_JPN
              if (PCAR->kind < 9 || PCAR->kind == 9) {
#else
              if (PCAR->kind < 9) {
#endif
                D_80316288[p] = g_dwDecalListCount - 1;
              } else {
                D_80316288[p] = g_dwDecalListCount - 2;
              }
              D_80316290[p] = DECALS_TITLE;
              D_80316258[p] = p ^ 2;
              v = FindFreeSoundChannel();
              if (v != -1) {
                InitSoundChannel(v, D_803162A4, D_803162A8, 0);
              }
              back = 0;
              LoadVehicleModel(STREAM(D_80316258[p]), D_80316258[p], PCAR->kind, p);
              while (PollVehicleModelDma(STREAM(D_80316258[p])) != 0) {
              }
              LoadVehicleModel(STREAM(D_80316258[p] ^ 2), D_80316258[p] ^ 2, PCAR->kind, p);
              break;
            case 5:
              if (D_80316278[p] == 1) {
                D_803162B8 = 1;
                D_803162C4 = 0;
                D_803162C0 = 0;
                D_803162BC = p;
                goto sound;
              }
              PCAR->xe58 = D_80316278[p] == 2;
              break;
            }
            D_80316280[p] = D_80316278[p];
            if ((((N64PadRec *)PCAR->pad)->pressed & 0x4000) || g_dwIsPaintMode != 0 || g_dwIsReviewMode != 0) {
              /* START: straight to ready (or the next results page) */
              D_80316288[p] = g_dwDecalListCount - 1;
              D_80316278[p] = 0;
              D_80316280[p] = 0;
              if (g_dwIsReviewMode != 0) {
                y = D_80316314;
                D_80316314++;
                if (D_80316314 == 1 && g_dwNumLocalPlayers != 2) {
                  D_80316314++;
                }
                if (D_80316314 == 2 && g_dwGameMode != 0) {
                  D_80316314 = 4;
                }
                if (D_80316314 == 3) {
                  ProcessSeasonResults();
                  D_80316310 = 1;
                  if (g_dwSeasonResultMsgLineCount == 0) {
                    D_80316314++;
                  }
                }
                if (D_80316314 == 4) {
                  D_803162B0[0] = D_803162B0[1] = 6;
                  D_80316314 = y;
                }
              } else {
                D_803162B0[p] = 6;
              }
            } else {
              D_803162B0[p]++;
            }
            for (n = 0; n < g_dwNumLocalPlayers; n++) {
              if (D_803162B0[n] != 6) {
                goto sound;
              }
            }
            D_80316298 = 1;
            FadeSfxTo(0.0f, 0.2f);
            if (g_dwIsPaintMode == 0) {
              FadeMusicTo(0.0f, 0.2f);
            }
            SetFadeTarget(0.0f, 0.2f);
          sound:
            if (back) {
              v = FindFreeSoundChannel();
              if (v != -1) {
                InitSoundChannel(v, D_803162A4, D_803162A8, 0);
              }
            }
          }
        }
      }
    }
  }
}
/* clang-format on */

void ProcessRaceResultsScreen(void) {
    g_dwIsReviewMode = 1;
    ProcessCarSelectScreen();
    g_dwIsReviewMode = 0;
}
#else
#include "carselect.h"

#include <PR/gu.h>
#include <PR/os.h>
#include <PR/os_libc.h>

#include "common/audio.h"
#include "common/audiomix.h"
#include "common/camera.h"
#include "common/fade.h"
#include "common/game.h"
#include "common/heap.h"
#include "common/input.h"
#include "common/laptime.h"
#include "common/race.h"
#include "common/render.h"
#include "common/system.h"
#include "common/texrect.h"
#include "common/text.h"
#include "common/timing.h"
#include "common/trackselect.h"
#include "common/util.h"
#include "common/vehiclerender.h"
#include "common/visgrid.h"
#include "common/weather.h"
#include "controllerscreen.h"
#include "mainmenu.h"
#include "options.h"
#include "paintshop.h"
#include "playerselect.h"
#include "weatherselect.h"

/* The sounds of moving the cursor and of choosing, on the cartridge. */
#define ROM_SCROLL_SOUND ((u32)_scrollSoundSegmentRomStart)
#define ROM_SELECT_SOUND ((u32)_selectSoundSegmentRomStart)

/* The silence after each sound, in bytes. */
#define SOUND_TAIL_BYTES 0x100

/* The time a fade of the screen in or out takes, as its rate. */
#define SCREEN_FADE_RATE 0.2f

/* The labels of the tyres stepping back and forward, of the decals, and the results of a racer who
 * did not finish. */
#if TGR_REGION == TGR_REGION_JP
#define BACK_TIRES_LABEL "TIRE"
#define TIRES_LABEL "Tire"
#define DECALS_LABEL "Decal"
#define PLAYER_NOT_FINISHED_FORMAT "%%ryP%d:GAME OVER"
#define NOT_FINISHED_FORMAT "%%ryGAME OVER"
#else
#define BACK_TIRES_LABEL "Tires"
#define TIRES_LABEL "Tires"
#define DECALS_LABEL "Decals"
#define PLAYER_NOT_FINISHED_FORMAT "%%ryP%d: Did not finish"
#define NOT_FINISHED_FORMAT "%%ryDid not finish"
#endif /* TGR_REGION == TGR_REGION_JP */

/* The height of the cars over their stage. */
#define STAGE_HEIGHT 0.254f

/* The scale of a model on a stage, and the degrees of a radian. */
#define STAGE_MODEL_SCALE 0.001302083372f
#define RAD_TO_DEG 57.29577637f

/* The colours of a progress bar: its frame, its background and its filling. */
#define BAR_FRAME_COLOR 0x0001
#define BAR_BACK_COLOR 0x01C1
#define BAR_FILL_COLOR 0x0781

/* The size of the bars of the speed and acceleration of a car, and their full values. */
#define STAT_BAR_WIDTH 80
#define STAT_BAR_HEIGHT 11
#define FULL_ACCELERATION 500.0
#define FULL_SPEED 10.0

/* The cars the paint shop allows, and the first car whose decals have no custom choice. */
#define PAINT_SHOP_CARS 9

/* The cars the players' views spread apart by when they slide. */
#define SPREAD_ONE_VIEW 8.0f
#define SPREAD_TWO_VIEWS 12.0f

/* The steps of the colour of the paint, of 8 bits a channel. */
#define PAINT_STEP 4

/* The tracks whose names are mirrored ones, past the first five. */
#define MIRRORED_TRACKS 5

/* The unlock that lets a car be switched to its other look. */
#define UNLOCK_OTHER_LOOK 0x8000

/* The buttons that choose. */
#define CHOOSE_BUTTONS (INPUT_A | INPUT_START | INPUT_Z)

/* The car a player has selected, looked up again at each use as the original does. */
#define SELECTED_CAR(player) (g_aCarModels[g_aVehicles[player].selectedCar])

void DrawProgressBar(s32 x, s32 y, s32 width, s32 height, f32 fraction) {
    gDPPipeSync(gDisplayListPtr++);
    gDPSetCycleType(gDisplayListPtr++, G_CYC_FILL);
    gDPSetRenderMode(gDisplayListPtr++, 0, 0);
    gDPSetCombineLERP(gDisplayListPtr++,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      PRIMITIVE,
                      0,
                      0,
                      0,
                      PRIMITIVE);
    /* The colours fill only the second of the two pixels of each word. */
    gDPSetFillColor(gDisplayListPtr++, BAR_FRAME_COLOR);
    gDPFillRectangle(gDisplayListPtr++, x, y, x + width, y + height);
    x += 3;
    y += 3;
    width -= 6;
    height -= 6;
    gDPPipeSync(gDisplayListPtr++);
    gDPSetFillColor(gDisplayListPtr++, BAR_BACK_COLOR);
    gDPFillRectangle(gDisplayListPtr++, x, y, x + width, y + height);
    width = width * fraction;
    gDPPipeSync(gDisplayListPtr++);
    gDPSetFillColor(gDisplayListPtr++, BAR_FILL_COLOR);
    gDPFillRectangle(gDisplayListPtr++, x, y, x + width, y + height);
}

s32 IsCarUnlocked(s32 car) {
#if TGR_REGION == TGR_REGION_JP
    /* The Japanese paint shop also allows the tenth car, tested apart from the first nine. */
    return (!g_dwIsPaintMode || car < PAINT_SHOP_CARS || car == PAINT_SHOP_CARS) &&
#else
    return (!g_dwIsPaintMode || car < PAINT_SHOP_CARS) &&
#endif
           (g_aVehicles[0].profile->unlocks & (1 << car)) != 0 && IsVehicleModelValid(car) != 0;
}

/** Light a model on a stage, from an eye, with the four lights of the stage and its ambient light.
 */
#define LIGHT_STAGE_MODEL(eyeX, eyeY, eyeZ, upY, upZ)                                              \
    {                                                                                              \
        LookAt *lookAt = AllocLightBlock();                                                        \
                                                                                                   \
        guLookAtReflectF(                                                                          \
            g_afScratchMatrix, lookAt, eyeX, eyeY, eyeZ, 0.0f, 0.0f, 0.0f, 0.0f, upY, upZ);        \
        gSPLookAt(gDisplayListPtr++, lookAt);                                                      \
        gSPNumLights(gDisplayListPtr++, NUMLIGHTS_4);                                              \
        gSPLight(gDisplayListPtr++, &g_CameraViewLight1, 1);                                       \
        gSPLight(gDisplayListPtr++, &g_CameraViewLight2, 2);                                       \
        gSPLight(gDisplayListPtr++, &g_CameraViewLight3, 3);                                       \
        gSPLight(gDisplayListPtr++, &g_CameraViewLight4, 4);                                       \
        gSPLight(gDisplayListPtr++, &g_CameraViewAmbientLight, 5);                                 \
        /* The block is taken and left unused. */                                                  \
        AllocDisplayListBlock();                                                                   \
    }

/** Draw a model on a stage, turned and placed. */
#define DRAW_STAGE_MODEL(model, degrees, x, y)                                                     \
    {                                                                                              \
        guScaleF(g_afMvpMatrix, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE);          \
        guRotateF(g_afScratchMatrix, degrees, 0.0f, 0.0f, 1.0f);                                   \
        guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);                                \
        guTranslateF(g_afScratchMatrix, x, y, 0.0f);                                               \
        guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);                                \
        DrawModelList(model, g_afMvpMatrix);                                                       \
    }

/** Draw the vehicle of the camera with the car of its slot, and its shadow. */
#define DRAW_STAGE_VEHICLE()                                                                       \
    {                                                                                              \
        DrawCarStageShadow(g_pActiveCameraVehicle);                                                \
        CheckVehicleVisibility(g_pActiveCameraVehicle);                                            \
        RenderVehicleModel(g_pActiveCameraVehicle, 0);                                             \
    }

void RenderCameraView(Model *model,
                      Model *secondModel,
                      s32 vehicleIndex,
                      s32 x,
                      s32 y,
                      s32 width,
                      s32 height,
                      f32 slide,
                      s32 hideChoiceMarker,
                      s32 slot,
                      s32 secondSlot,
                      f32 spread,
                      f32 angle) {
    VehicleModelLoad *load, *secondLoad;
    CameraFrame *camera;
    f32 offset, otherOffset;

    g_pActiveCameraVehicle = &g_aVehicles[vehicleIndex];
    g_pActiveCameraVehicle->frame.forward.x = cosf(angle);
    g_pActiveCameraVehicle->frame.forward.y = sinf(angle);
    g_pActiveCameraVehicle->frame.forward.z = 0.0f;
    Vec3fCross(&g_pActiveCameraVehicle->frame.side,
               &g_pActiveCameraVehicle->frame.up,
               &g_pActiveCameraVehicle->frame.forward);
    camera = &g_pActiveCameraVehicle->cameras[VEHICLE_CAMERA_TRACKSIDE];
    g_pActiveCameraVehicle->activeCamera = camera;
    g_pActiveCamera = camera;
    /* The heading is taken twice. */
    g_pActiveCameraVehicle->cameraHeading = Atan2f(camera->forward.x, camera->forward.y);
    g_pActiveCameraVehicle->cameraHeading =
        Atan2f(g_pActiveCamera->forward.x, g_pActiveCamera->forward.y);
    SetupCameraMatrix(g_pActiveCamera,
                      g_fCameraFovRadians,
                      g_fCameraProjectionScale * 0.2f,
                      (f32)width,
                      (f32)height);
    SetViewport(x, y, width, height, TRUE);
    MarkVisibleCell(0, 0);

    /* The cars slide apart by the square of the slide. */
    if (slide < 0.0) {
        offset = slide * slide * -spread;
    } else {
        offset = slide * slide * spread;
    }
    g_pActiveCameraVehicle->frame.position.x = offset;
    otherOffset = -offset;
    g_pActiveCameraVehicle->frame.position.y = otherOffset;
    g_pActiveCameraVehicle->frame.position.z = STAGE_HEIGHT;
    if (model == NULL) {
        TransformWheelPositions(vehicleIndex);
    }
    load = &g_aVehicleModelSlots[slot];
    PollVehicleModelDma(load);
    DrawCarStageDisc(g_pActiveCameraVehicle);
    if (!hideChoiceMarker && !g_dwIsReviewMode) {
        DrawCarStageChoiceMarker(g_pActiveCameraVehicle);
    }
    if (model != NULL) {
        LIGHT_STAGE_MODEL(11.0f, 11.0f, 1.0f, 0.0f, 1.0f);
        DRAW_STAGE_MODEL(model, angle * RAD_TO_DEG, offset, otherOffset);
    } else if (IsVehicleModelLoaded(load)) {
        SetVehicleModelSlot(g_pActiveCameraVehicle, slot);
        DRAW_STAGE_VEHICLE();
    }

    /* The second car comes in from the far side as the first slides out. */
    secondLoad = &g_aVehicleModelSlots[secondSlot];
    PollVehicleModelDma(secondLoad);
    if (offset == 0.0f) {
        return;
    }
    if (secondModel != NULL) {
        angle *= RAD_TO_DEG;
        LIGHT_STAGE_MODEL(0.0f, -1.0f, 15.0f, 1.0f, 0.0f);
        guScaleF(g_afMvpMatrix, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE, STAGE_MODEL_SCALE);
        guRotateF(g_afScratchMatrix, angle, 0.0f, 0.0f, 1.0f);
        guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
        /* Each side of the stage has its own call. */
        if (0.0f < offset) {
            guTranslateF(g_afScratchMatrix, offset - spread, -(offset - spread), 0.0f);
        } else {
            guTranslateF(g_afScratchMatrix, offset + spread, -(offset + spread), 0.0f);
        }
        guMtxCatF(g_afMvpMatrix, g_afScratchMatrix, g_afMvpMatrix);
        DrawModelList(secondModel, g_afMvpMatrix);
        return;
    }
#if TGR_REGION == TGR_REGION_JP
    if (!IsVehicleModelLoaded(secondLoad)) {
        return;
    }
#else
    if (!IsVehicleModelLoaded(secondLoad) && PollVehicleModelDma(secondLoad)) {
        return;
    }
#endif
    SetVehicleModelSlot(g_pActiveCameraVehicle, secondSlot);
    /* Each side of the stage has its own stores. */
    if (0.0f < offset) {
        g_pActiveCameraVehicle->frame.position.x = offset - spread;
        g_pActiveCameraVehicle->frame.position.y = -(offset - spread);
        g_pActiveCameraVehicle->frame.position.z = STAGE_HEIGHT;
    } else {
        g_pActiveCameraVehicle->frame.position.x = offset + spread;
        g_pActiveCameraVehicle->frame.position.y = -(offset + spread);
        g_pActiveCameraVehicle->frame.position.z = STAGE_HEIGHT;
    }
    TransformWheelPositions(vehicleIndex);
    DRAW_STAGE_VEHICLE();
    if (IsVehicleModelLoaded(load)) {
        SetVehicleModelSlot(g_pActiveCameraVehicle, slot);
    }
}

/** Play a sound of the screen on a free channel. */
static inline void PlayCarSelectSound(s8 *sample, u32 length) {
    s16 channel = FindFreeSoundChannel();

    if (channel != -1) {
        InitSoundChannel(channel, sample, length, 0);
    }
}

/** Load a sound of the screen and silence its end. */
static inline s8 *LoadCarSelectSound(u32 rom, u32 *size) {
    s8 *buffer;

    *size = DmaReadAssetSize(rom);
    buffer = AllocHeapAligned(*size + SOUND_TAIL_BYTES);
    DecompressFromRom(buffer, rom, NULL);
    return buffer;
}

/** Set a vehicle on a stage, facing along x. */
static inline void PlaceOnStage(Vehicle *vehicle) {
    vehicle->frame.forward.x = 1.0f;
    vehicle->frame.forward.y = 0.0f;
    vehicle->frame.forward.z = 0.0f;
    vehicle->frame.side.x = 0.0f;
    vehicle->frame.side.y = 1.0f;
    vehicle->frame.side.z = 0.0f;
    vehicle->frame.up.x = 0.0f;
    vehicle->frame.up.y = 0.0f;
    vehicle->frame.up.z = 1.0f;
    vehicle->frame.position.x = 0.0f;
    vehicle->frame.position.y = 0.0f;
    vehicle->frame.position.z = STAGE_HEIGHT;
}

/** Set up the screen: the players' cars, the stages, the options and the sounds. */
static inline void StartCarSelect(void) {
    PlayerProfile *profile;
    CameraFrame *camera;
    Vehicle *vehicle;
    s32 i, car;

#if TGR_REGION != TGR_REGION_US
    /* After the results of a race the screen stays blank while the cars load. */
    if (IsCurrentScreen(ProcessRaceResultsScreen)) {
        g_dwKeepScreenBlanked = TRUE;
        BlankDisplay();
        StartHighResFrame();
        FlushDisplayList();
        BlankDisplay();
        StartHighResFrame();
        FlushDisplayList();
    }
#endif /* TGR_REGION != TGR_REGION_US */
    g_dwCarSelectStartTime = g_dwElapsedTimeMs;
    SetFadeTarget(1.0f, SCREEN_FADE_RATE);
    FadeSfxTo(1.0f, SCREEN_FADE_RATE);
    FadeMusicTo(1.0f, SCREEN_FADE_RATE);
    profile = g_aVehicles[0].profile;
    g_dwResultsSeason = profile->season;
    g_dwResultsRound = profile->round;
    g_dwCarSelectWeather = g_dwCurrentWeather;
    g_bSeasonResultsShown = FALSE;
    g_dwVehicleViewModeIndex = 0;
    g_afCarSelectAngle[0] = 0.0f;
    g_afCarSelectAngle[1] = 1.0f;
    g_dwResultsChampionshipFlags = profile->championshipFlags;
    g_dwResultsTrack = g_dwSelectedTrackId;
    for (i = 0, vehicle = g_aVehicles; i < (s32)g_dwNumLocalPlayers; i++, vehicle++) {
        if (!g_dwIsReviewMode) {
            InitVehicleColor(vehicle);
            vehicle->driver = UpdatePhysicsStep;
        }
        g_adwCarSelectModelSlots[i] = i;
        g_adwCarSelectState[i] = CAR_SELECT_CAR;
        vehicle->groundInstanceCount = 0;
        vehicle->nearbyInstanceCount = 0;
        g_afCarSelectSlide[i] = 0.0f;
        g_dwPakDialogActive = FALSE;
    }
    /* The vehicles of no player are the computer's. */
    if (!g_dwIsReviewMode && i < MAX_VEHICLES) {
        for (; i < MAX_VEHICLES; i++) {
            InitVehicleColor(&g_aVehicles[i]);
            g_aVehicles[i].driver = DriveAiVehicle;
        }
    }
    g_dwNumActiveVehicles = 2;
    if (!g_dwIsReviewMode) {
        for (i = 0, vehicle = g_aVehicles; i < (s32)g_dwNumLocalPlayers; i++, vehicle++) {
            ChooseVehicleCar(vehicle);
            car = vehicle->carModel;
            vehicle->selectedCar = car;
            while (!IsCarUnlocked(car)) {
                car = (car + 1) % g_dwCarCount;
            }
            vehicle->carModel = car;
            vehicle->selectedCar = car;
            InstallVehicleModel(vehicle, i, car);
        }
    }
    PlaceOnStage(&g_aVehicles[0]);
    PlaceOnStage(&g_aVehicles[1]);

    /* The trackside camera looks at the stage from above a corner, farther in two views. */
    camera = &g_aVehicles[0].cameras[VEHICLE_CAMERA_TRACKSIDE];
    if (g_dwNumLocalPlayers == 2 && !g_dwIsReviewMode) {
        camera->position.x = 15.0f;
        camera->position.y = 15.0f;
        camera->position.z = 7.5f;
    } else {
        camera->position.x = 10.0f;
        camera->position.y = 10.0f;
        camera->position.z = 5.0f;
    }
    Vec3fScale(&camera->forward, &camera->position, -1.0f);
    Vec3fNormalize(&camera->forward);
    camera->up.x = 0.0f;
    camera->up.y = 0.0f;
    camera->up.z = 1.0f;
    Vec3fCross(&camera->side, &camera->up, &camera->forward);
    Vec3fCross(&camera->up, &camera->forward, &camera->side);
    memcpy(&g_aVehicles[1].cameras[VEHICLE_CAMERA_TRACKSIDE], camera, sizeof(CameraFrame));

    ResetHeap();
    LoadMenuItemModels(g_apHandlingOptions, &g_dwHandlingOptionCount);
    LoadMenuItemModels(g_apTransmissionOptions, &g_dwTransmissionOptionCount);
    LoadMenuItemModels(g_apTiresOptions, &g_dwTiresOptionCount);
    LoadMenuItemModels(g_apSuspensionOptions, &g_dwSuspensionOptionCount);
    LoadMenuItemModels(g_apDecalOptions, &g_dwDecalListCount);
    LoadAndDecompressAsset(&g_BgTextureDesc, AllocHeapAligned);
    LoadAndDecompressAsset(&g_ArrowLeftTextureDesc, AllocHeapAligned);
    LoadAndDecompressAsset(&g_ArrowRightTextureDesc, AllocHeapAligned);
    LoadAndDecompressAsset(&g_TitleBarTextureDesc, AllocHeapAligned);
    g_pScrollSoundBuf = LoadCarSelectSound(ROM_SCROLL_SOUND, &g_dwScrollSoundSize);
    g_pSelectSoundBuf = LoadCarSelectSound(ROM_SELECT_SOUND, &g_dwSelectSoundSize);
    for (i = 0; i < SOUND_TAIL_BYTES; i++) {
        g_pScrollSoundBuf[g_dwScrollSoundSize + i] = 0;
        g_pSelectSoundBuf[g_dwSelectSoundSize + i] = 0;
    }
    AllocateEditorBuffers();
    g_dwTitleRedrawFrames = 2;
    g_bCarSelectInitialized = TRUE;
    g_aapCarSelectOptions[1] = g_apHandlingOptions;
    g_aapCarSelectOptions[0] = g_apHandlingOptions;
    g_anCarSelectCursor[1] = 0;
    g_anCarSelectCursor[0] = 0;
#if TGR_REGION != TGR_REGION_US
    g_dwKeepScreenBlanked = FALSE;
#endif
#if TGR_REGION == TGR_REGION_JP
    g_adwCarSelectPaintPending[1] = TRUE;
    g_adwCarSelectPaintPending[0] = TRUE;
#endif
    g_dwResultsPage = RESULTS_PAGE_PLAYER_1;
}

#if TGR_REGION == TGR_REGION_JP
/* Paint the model in the slot of each player whose paint is pending, with the colour of its car on
 * the car step, in the colour of the player when two play. The car step paints it twice. */
static inline void PaintPendingModels(void) {
    Vehicle *vehicle;
    s32 *slot;
    s32 player, state;

    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        if (!g_adwCarSelectPaintPending[player]) {
            continue;
        }
        g_adwCarSelectPaintPending[player] = FALSE;
        state = g_adwCarSelectState[player];
        vehicle = &g_aVehicles[player];
        slot = &g_adwCarSelectModelSlots[player];
        if (state == CAR_SELECT_CAR) {
            ExtractVehicleBaseColor(vehicle, &g_aVehicleModels[*slot]);
            if (g_dwNumLocalPlayers == 2) {
                vehicle->color[0] = g_abPlayerColorsRgb[player][0];
                vehicle->color[1] = g_abPlayerColorsRgb[player][1];
                vehicle->color[2] = g_abPlayerColorsRgb[player][2];
            }
            SetMaterialColor(&g_aVehicleModels[*slot],
                             vehicle->color[0] >> 3,
                             vehicle->color[1] >> 3,
                             vehicle->color[2] >> 3);
        } else if (state == CAR_SELECT_DECALS) {
            ExtractVehicleBaseColor(vehicle, &g_aVehicleModels[*slot]);
        }
        SetMaterialColor(&g_aVehicleModels[*slot],
                         vehicle->color[0] >> 3,
                         vehicle->color[1] >> 3,
                         vehicle->color[2] >> 3);
    }
}
#endif /* TGR_REGION == TGR_REGION_JP */

/** The models of the option a player chose and of the one the cursor left, or NULL for cars. */
static inline Model *OptionModel(s32 player, s32 option) {
    if (g_adwCarSelectState[player] == CAR_SELECT_CAR ||
        g_adwCarSelectState[player] == CAR_SELECT_READY) {
        return NULL;
    }
    return g_aapCarSelectOptions[player][option]->model;
}

/** Draw the stage of a player in a rectangle of the screen. */
static inline void
DrawPlayerStage(s32 player, s32 vehicleIndex, s32 slot, s32 y, s32 height, f32 spread) {
    Model *first, *second;

    first = OptionModel(player, g_anCarSelectCursor[player]);
    second = OptionModel(player, g_anCarSelectPrevCursor[player]);
    RenderCameraView(first,
                     second,
                     vehicleIndex,
                     0,
                     y,
                     g_dwScreenWidth,
                     height,
                     g_afCarSelectSlide[player],
                     g_adwCarSelectState[player] == CAR_SELECT_READY,
                     slot,
                     slot ^ 2,
                     spread,
                     g_afCarSelectAngle[player]);
}

/** Draw the title of the screen. */
static inline void DrawCarSelectTitle(char *text) {
    const char *prefix;
    s32 track;

    DrawTexturedRect(&g_TitleBarTextureDesc,
                     22,
                     9,
                     280,
                     42,
                     0,
                     0,
                     0,
                     0xFF,
                     0xFF,
                     0xFF,
                     0xFF,
                     0xFF,
                     TEXRECT_BLEND_TEXTURE_2);
    SetTextStyleNormal();
    SetTextAlignCenter();
    SetTextSize(30);
    if (g_dwIsReviewMode) {
        if (g_dwResultsPage == RESULTS_PAGE_PLAYER_1 || g_dwResultsPage == RESULTS_PAGE_PLAYER_2) {
            /* The track and the weather of the race, its name marked when it is mirrored. */
            prefix = g_dwResultsTrack < MIRRORED_TRACKS ? "" : "M-";
            track = g_dwResultsTrack < MIRRORED_TRACKS ? g_dwResultsTrack :
                                                         g_dwResultsTrack - MIRRORED_TRACKS;
#if TGR_REGION == TGR_REGION_JP
            sprintf(text,
                    "%%ry%s%s %%j%c",
                    prefix,
                    g_apTrackMenuItems[track]->name,
                    g_dwCarSelectWeather + FIRST_WEATHER_GLYPH);
#else
            sprintf(text,
                    "%%ry%s%s/%s",
                    prefix,
                    g_apTrackMenuItems[track]->name,
                    g_apWeatherMenuItems[g_dwCarSelectWeather]->name);
#endif /* TGR_REGION == TGR_REGION_JP */
        } else {
            sprintf(text, "%%ry%s", g_SeasonTable[g_dwResultsSeason].name);
        }
        DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);
    } else if (g_dwIsPaintMode) {
        DrawText("%ryPaint car select", g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);
    } else {
        DrawText("%ryCar Select", g_dwScreenWidth / 2, g_dwScreenHeight / 6 - 2);
    }
}

/** Draw the place and the times of the race of a player. */
static inline void DrawRaceResults(s32 player, char *time) {
    Vehicle *vehicle = &g_aVehicles[player];
    PlayerProfile *profile;
    s32 place, points, lap, y;

    if ((s32)g_dwNumLocalPlayers >= 2) {
        if (vehicle->lap < g_dwTotalLaps) {
            sprintf(g_szResultsBuf, PLAYER_NOT_FINISHED_FORMAT, player + 1);
        } else {
            sprintf(g_szResultsBuf,
                    "%%ryP%d: %s Place",
                    player + 1,
                    g_apszPlaceNames[vehicle->racePosition]);
        }
    } else if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
        profile = vehicle->profile;
        place = profile->seasonPlaces[g_dwResultsSeason][g_dwResultsRound];
        points = g_abRacePositionPoints[place];
        sprintf(g_szResultsBuf,
                "%%ry%s Place - %d Point%s",
                g_apszPlaceNames[place],
                points,
                points == 1 ? "" : "s");
    } else if (vehicle->lap < g_dwTotalLaps) {
        sprintf(g_szResultsBuf, NOT_FINISHED_FORMAT);
    } else {
        sprintf(g_szResultsBuf, "%%ry%s Place", g_apszPlaceNames[vehicle->racePosition]);
    }
    DrawText(g_szResultsBuf, g_dwScreenWidth / 2, g_dwScreenHeight * 19 / 64 - 4);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    SetTextSize(10);

    /* A line for each lap, then the time of the race. */
    for (lap = 0, y = 79; lap <= g_dwTotalLaps; lap++, y += 10) {
        SetTextAlignLeft();
        if (lap == g_dwTotalLaps) {
            y += 2;
            if (vehicle->totalTime == g_aVehicles[0].profile->raceRecords[g_dwResultsTrack]) {
                DrawText("  %yyRECORD TIME!", g_dwScreenWidth * 11 / 16, y);
            }
            sprintf(g_szResultsBuf, "Race time:");
            FormatLapTime(time, vehicle->totalTime);
        } else {
            sprintf(g_szResultsBuf, "Lap %d time:", lap + 1);
            if (lap < vehicle->lap) {
                FormatLapTime(time, g_aVehicles[player].lapTimes[lap]);
            } else {
                sprintf(time, "--    ");
            }
        }
        DrawText(g_szResultsBuf, g_dwScreenWidth * 5 / 16, y);
        if (lap < vehicle->lap && lap == vehicle->bestLap) {
            if (vehicle->bestLapTime == g_aVehicles[0].profile->lapRecords[g_dwResultsTrack]) {
                DrawText("  %yyRECORD LAP!", g_dwScreenWidth * 11 / 16, y);
            } else {
                DrawText("  %yyBEST LAP!", g_dwScreenWidth * 11 / 16, y);
            }
        }
        SetTextAlignRight();
        DrawText(time, g_dwScreenWidth * 11 / 16, y);
    }
}

/** Draw the races of the season, their times, places and points, and the total. */
static inline void DrawSeasonResults(void) {
    Season *season = &g_SeasonTable[g_dwResultsSeason];
    const char *place;
    s32 race, track, shift, total, y;
    u8 position;

    DrawText("%rySeason Results", g_dwScreenWidth / 2, g_dwScreenHeight * 19 / 64 - 4);
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    SetTextSize(10);

    /* The columns move right when a name is of the longer, mirrored tracks. */
    shift = 0;
    for (race = 0; race < season->raceCount; race++) {
        if ((season->races[race][0] < MIRRORED_TRACKS) ==
            ((g_dwResultsChampionshipFlags & 1) != 0)) {
            shift = g_dwScreenWidth / 16;
            break;
        }
    }

    total = 0;
    for (race = 0, y = 79; race < season->raceCount; race++, y += 10) {
        track = season->races[race][0];
        if (g_dwResultsChampionshipFlags & 1) {
            track = track < MIRRORED_TRACKS ? track + MIRRORED_TRACKS : track - MIRRORED_TRACKS;
        }
        SetTextAlignLeft();
#if TGR_REGION == TGR_REGION_JP
        /* The glyph of the weather leads the name of the track. */
        sprintf(g_szResultsBuf, "%%j%c", season->races[race][1] + FIRST_WEATHER_GLYPH);
        SetTextSize(13);
        DrawText(g_szResultsBuf, g_dwScreenWidth * 3 / 16 - shift, y + 1);
        SetTextSize(10);
        DrawText(g_apTrackMenuItems[track]->name, g_dwScreenWidth * 3 / 16 - shift + 12, y);
#else
        sprintf(g_szResultsBuf,
                "%s/%s",
                g_apTrackMenuItems[track]->name,
                g_apWeatherMenuItems[season->races[race][1]]->name);
        DrawText(g_szResultsBuf, g_dwScreenWidth * 3 / 16 - shift, y);
#endif /* TGR_REGION == TGR_REGION_JP */
        SetTextAlignCenter();
        if (race <= g_dwResultsRound) {
            FormatLapTime(g_szResultsBuf,
                          g_aVehicles[0].profile->seasonTimes[g_dwResultsSeason][race]);
        } else {
            sprintf(g_szResultsBuf, "--");
        }
        DrawText(g_szResultsBuf, g_dwScreenWidth * 17 / 32 + shift, y);
        if (race <= g_dwResultsRound) {
            place = g_apszPlaceNames[g_aVehicles[0].profile->seasonPlaces[g_dwResultsSeason][race]];
        } else {
            place = "--";
        }
        DrawText(place, g_dwScreenWidth * 21 / 32 + shift, y);
        SetTextAlignRight();
        /* A race not yet run shows the dashes of its time again. */
        if (race <= g_dwResultsRound) {
            position = g_aVehicles[0].profile->seasonPlaces[g_dwResultsSeason][race];
            sprintf(g_szResultsBuf, "%d PTS", g_abRacePositionPoints[position]);
            total += g_abRacePositionPoints[g_aVehicles[0]
                                                .profile->seasonPlaces[g_dwResultsSeason][race]];
        }
        DrawText(g_szResultsBuf, g_dwScreenWidth * 13 / 16 + shift, y);
    }
    y += 2;
    sprintf(g_szResultsBuf, "TOTAL:    %d PTS ", total);
    DrawText(g_szResultsBuf, g_dwScreenWidth * 13 / 16 + shift, y);
}

/** Draw the lines of the message of the result of the season. */
static inline void DrawSeasonMessage(void) {
    s32 line, y;

    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);
    SetTextSize(12);
    y = 98 - g_dwSeasonResultMsgLineCount * 120 / 20;
    SetTextAlignCenter();
    for (line = 0; line < g_dwSeasonResultMsgLineCount; line++, y += 12) {
        DrawText(g_apszSeasonResultMsgLines[line], g_dwScreenWidth / 2, y);
    }
}

/** Write what a player is choosing: the name of the car, ready, or the option and its name. */
static inline void FormatPlayerChoice(
    char *text, s32 player, const char *ready, const char *carFormat, const char *optionFormat) {
    s32 state = g_adwCarSelectState[player];

    if (state == CAR_SELECT_CAR) {
        sprintf(text, carFormat, g_aCarModels[g_aVehicles[player].selectedCar].name);
    } else if (state == CAR_SELECT_READY) {
        sprintf(text, ready);
    } else {
        sprintf(text,
                optionFormat,
                g_apszCarSelectCategory[player],
                g_aapCarSelectOptions[player][g_anCarSelectCursor[player]]->name);
    }
}

/** Draw the advice about the rumble pak a player sees while choosing decals other than loading. */
static inline void DrawRumbleNote(s32 player, s32 y) {
    s32 x;

    if (g_adwCarSelectState[player] != CAR_SELECT_DECALS ||
        g_anCarSelectCursor[player] == DECALS_LOAD || g_afCarSelectSlide[player] != 0.0f) {
        return;
    }
#if TGR_REGION == TGR_REGION_JP
    x = g_dwScreenWidth / 16;
    DrawText("%ww%j振動させたいときは、", x, y + 22);
    DrawText("%ww%j振動パックを", x, y + 34);
    DrawText("%ww%jさして下さい。", x, y + 47);
#else
    x = g_dwScreenWidth * 11 / 16;
    DrawText("%wwIf you wish to use", x, y + 19);
    DrawText("%wwthe Rumble Pak, you", x, y + 28);
    DrawText("%wwshould ensure that it", x, y + 37);
    DrawText("%wwis plugged in before", x, y + 45);
    DrawText("%wwpressing A.", x, y + 54);
#endif /* TGR_REGION == TGR_REGION_JP */
}

/** Draw the engine, gearbox, acceleration and top speed of the car of a player. */
static inline void DrawCarStats(char *text, s32 player, s32 y, s32 barY) {
    SetTextAlignRight();
    DrawText("%wwAcceleration", g_dwScreenWidth / 4 - 5, y + 2);
    DrawText("%wwMax Speed", g_dwScreenWidth / 4 - 5, y - 6);
    SetTextAlignCenter();
    DrawText(
        g_apszEngineLayouts[SELECTED_CAR(player).engineLayout], g_dwScreenWidth * 3 / 4, y + 2);
    sprintf(text,
            "%%ww%d-Speed %s",
            SELECTED_CAR(player).gearCount,
            g_apszDriveTypes[SELECTED_CAR(player).driveType]);
    DrawText(text, g_dwScreenWidth * 3 / 4, y - 6);
    DrawProgressBar(g_dwScreenWidth / 2,
                    barY - 23,
                    STAT_BAR_WIDTH,
                    STAT_BAR_HEIGHT,
                    (f32)(SELECTED_CAR(player).acceleration / FULL_ACCELERATION));
    DrawProgressBar(g_dwScreenWidth / 2,
                    barY - 6,
                    STAT_BAR_WIDTH,
                    STAT_BAR_HEIGHT,
                    (f32)(SELECTED_CAR(player).maxSpeed / FULL_SPEED));
}

/** Draw what the two players are choosing, their advice, and the stats of their cars. */
static inline void DrawTwoPlayerChoices(char *text) {
    SetTextSize(17);
    FormatPlayerChoice(text, 0, "%%ryREADY!", "%%ry%%i%s", "%%ry%s: %%i%s");
    DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight / 4);
    FormatPlayerChoice(text, 1, "%%ryREADY!", "%%ry%%i%s", "%%ry%s: %%i%s");
    DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight * 3 / 5);
#if TGR_REGION == TGR_REGION_JP
    SetTextSize(14);
#else
    SetTextSize(8);
#endif
    SetTextAlignLeft();
    DrawRumbleNote(0, g_dwScreenHeight / 4);
    DrawRumbleNote(1, g_dwScreenHeight * 3 / 5);
#if TGR_REGION == TGR_REGION_JP
    SetTextSize(8);
#endif
    SetTextAlignRight();
    if (g_adwCarSelectState[0] == CAR_SELECT_CAR) {
        DrawCarStats(text, 0, g_dwScreenHeight / 4, g_dwScreenHeight / 2);
    }
    if (g_adwCarSelectState[1] == CAR_SELECT_CAR) {
        DrawCarStats(text, 1, g_dwScreenHeight * 3 / 5, g_dwScreenHeight * 3 * 2 / 5);
    }
}

/** Draw what the player is choosing, the advice, and the stats of the car. */
static inline void DrawOnePlayerChoices(char *text) {
    s32 player = 0;

    SetTextSize(20);
    if (g_dwIsPaintMode) {
        sprintf(text, "%%ry%%i%s", g_aCarModels[g_aVehicles[0].selectedCar].name);
        DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight * 22 / 64);
        return;
    }
    FormatPlayerChoice(text, 0, "%%ryLET'S GO!", "%%ry%%i%s", "%%ry%s: %%i%s");
    DrawText(text, g_dwScreenWidth / 2, g_dwScreenHeight * 20 / 64);
    SetTextSize(8);
    SetTextAlignCenter();
    if (g_adwCarSelectState[0] == CAR_SELECT_DECALS && g_anCarSelectCursor[0] != DECALS_LOAD &&
        g_afCarSelectSlide[0] == 0.0f) {
#if TGR_REGION == TGR_REGION_JP
        SetTextSize(14);
        DrawText("%ww %j振動させたいときは、振動パックをさして下さい。",
                 g_dwScreenWidth / 2,
                 g_dwScreenHeight * 3 / 8 - 1);
        SetTextSize(8);
#else
        DrawText("%wwIf you wish to use the Rumble Pak, you should",
                 g_dwScreenWidth / 2,
                 g_dwScreenHeight * 3 / 8 - 6);
        DrawText("%wwensure that it is plugged in before pressing A.",
                 g_dwScreenWidth / 2,
                 g_dwScreenHeight * 3 / 8 + 2);
#endif /* TGR_REGION == TGR_REGION_JP */
    }
    if (g_adwCarSelectState[0] != CAR_SELECT_CAR) {
        return;
    }
    SetTextAlignRight();
    SetTextSize(8);
    DrawText("%wwMax Speed", g_dwScreenWidth * 3 / 8, g_dwScreenHeight * 3 / 8 - 6);
    DrawText("%wwAcceleration", g_dwScreenWidth * 3 / 8, g_dwScreenHeight * 3 / 8 + 2);
    SetTextAlignCenter();
    DrawText(g_apszEngineLayouts[SELECTED_CAR(player).engineLayout],
             g_dwScreenWidth * 11 / 16,
             g_dwScreenHeight * 3 / 8 - 6);
    sprintf(text,
            "%%ww%d-Speed %s",
            SELECTED_CAR(player).gearCount,
            g_apszDriveTypes[SELECTED_CAR(player).driveType]);
    DrawText(text, g_dwScreenWidth * 11 / 16, g_dwScreenHeight * 3 / 8 + 2);
    /* The bars are placed in pixels of the 640 by 480 frame buffer, half the scale of the text. */
    DrawProgressBar(g_dwScreenWidth * 3 / 4 + 10,
                    g_dwScreenHeight * 3 / 4 - 23,
                    STAT_BAR_WIDTH,
                    STAT_BAR_HEIGHT,
                    (f32)(SELECTED_CAR(player).acceleration / FULL_ACCELERATION));
    DrawProgressBar(g_dwScreenWidth * 3 / 4 + 10,
                    g_dwScreenHeight * 3 / 4 - 6,
                    STAT_BAR_WIDTH,
                    STAT_BAR_HEIGHT,
                    (f32)(SELECTED_CAR(player).maxSpeed / FULL_SPEED));
}

/** Run the dialog of the controller pak that loads the decals of a player. */
static inline void RunPakDialog(void) {
    s32 result = g_dwPakDialogResult;
    s32 player;
    s32 previous;

    /* The original stores the result back on every path but that of a cancel, changed or not. */
    if (result == 0) {
        if (!g_bPakDialogDone) {
            result = ProcessDecalSaveLoad(&g_aVehicleModels[g_dwPakDialogPlayerIdx],
                                          (u8)g_dwPakDialogPlayerIdx,
                                          9,
                                          1,
                                          &g_bPakDialogDone);
        }
        g_dwPakDialogResult = result;
    } else if (result == PAK_DIALOG_CANCELLED) {
        g_bPakDialogDone = TRUE;
    } else {
        g_dwPakDialogResult = result;
        DrawPakErrorMessage(result, 9, 1);
        if (g_aVehicles[g_dwPakDialogPlayerIdx].input->buttons & (INPUT_A | INPUT_B)) {
            TransferFlags(g_aVehicles[g_dwPakDialogPlayerIdx].input, INPUT_A | INPUT_B);
            g_bPakDialogDone = TRUE;
        }
    }
    player = g_dwPakDialogPlayerIdx;
    if (!g_bPakDialogDone) {
        return;
    }
    result = g_dwPakDialogResult;
    if (result != 0) {
        /* The custom decals are out of reach without loaded ones. */
        g_anCarSelectOptionCount[player] = g_dwDecalListCount - 1;
        g_dwPakDialogResult = 0;
    } else {
        /* Loaded decals add the custom choice, and the cursor moves to it. The original clears
         * the result by storing the zero it read. */
        g_dwPakDialogResult = result;
        g_anCarSelectOptionCount[player] = g_dwDecalListCount;
        previous = g_anCarSelectCursor[player];
        g_anCarSelectPrevCursor[player] = previous;
        g_anCarSelectCursor[player] = (previous + 1) % g_dwDecalListCount;
        g_afCarSelectSlide[player] = 1.0f;
        g_dwPakDialogPlayerIdx = player;
        ExtractVehicleBaseColor(&g_aVehicles[player], &g_aVehicleModels[player]);
    }
}

/** Leave the screen once it has faded out, to the next screen or back. */
static inline void LeaveCarSelect(void) {
    s32 i;

    g_bCarSelectInitialized = FALSE;
    ClearFramebuffersForWideMode();
    for (i = 0; i < SOUND_EFFECT_CHANNELS; i++) {
        g_aSoundEffectChannels[i].volume = 0;
        g_aSoundEffectChannels[i].mixVolume = 0;
        g_aSoundEffectChannels[i].step = 0;
        g_aSoundEffectChannels[i].sample = g_SilenceSampleData;
    }
    if (g_bCarSelectDone) {
        if (g_dwIsReviewMode) {
            if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP) {
                /* The next race of the season, or after its last race, the end of the season. */
                g_dwNumLocalPlayers = 1;
                g_dwContinueFromResults = g_aVehicles[0].profile->round != 0;
                if (g_dwResultsSeason == g_aVehicles[0].profile->season) {
                    SetTrackFromSeason();
                    g_bSaveLoadEntryPoint = SAVE_LOAD_FROM_SEASON;
                    SetCurrentScreenCallback(ProcessDataSaveLoadMenu);
                } else {
                    ClearFramebuffersLowRes();
                    g_dwGameMode = GAME_MODE_AFTER_SEASON;
                    SetCurrentScreenCallback(ProcessRacingScreen);
                }
            } else if (g_dwGameMode == GAME_MODE_TIME_ATTACK && g_dwSaveAvailable) {
                g_bSaveLoadEntryPoint = SAVE_LOAD_FROM_TIME_ATTACK;
                SetCurrentScreenCallback(ProcessDataSaveLoadMenu);
            } else {
                SetCurrentScreenCallback(ProcessMainMenu);
            }
        } else if (g_dwIsPaintMode) {
            SetCurrentScreenCallback(ProcessPaintShopScreen);
        } else {
            ClearFramebuffersLowRes();
            SetCurrentScreenCallback(ProcessRacingScreen);
        }
    } else if (g_dwIsReviewMode) {
        if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP && !g_bSeasonResultsShown) {
            ProcessSeasonResults();
        }
        SetCurrentScreenCallback(ProcessMainMenu);
    } else if (g_dwIsPaintMode) {
        SetCurrentScreenCallback(ProcessMainMenu);
    } else if (g_dwGameMode == GAME_MODE_CHAMPIONSHIP || g_dwGameMode == GAME_MODE_TIME_ATTACK) {
        g_dwContinueFromResults = TRUE;
        SetCurrentScreenCallback(ProcessTrackMenu);
    } else {
        SetCurrentScreenCallback(ProcessWeatherSelect);
    }
}

/** Raise or lower a part of the colour of the paint by a step, within a byte. */
static inline void StepPaintPart(u8 *part, s32 up) {
    if (up) {
        *part = *part < 0x100 - PAINT_STEP ? *part + PAINT_STEP : 0xFF;
    } else {
        *part = *part < PAINT_STEP ? 0 : *part - PAINT_STEP;
    }
}

/** Change the colour of the paint of a vehicle with up or down and a C button for each part. */
static inline void AdjustPaint(Vehicle *vehicle, u32 direction) {
    s32 up = direction == INPUT_UP;

    if (vehicle->input->buttons & INPUT_C_LEFT) {
        StepPaintPart(&vehicle->color[0], up);
    }
    if (vehicle->input->buttons & INPUT_C_UP) {
        StepPaintPart(&vehicle->color[1], up);
    }
    if (vehicle->input->buttons & INPUT_C_RIGHT) {
        StepPaintPart(&vehicle->color[2], up);
    }
#if TGR_REGION == TGR_REGION_US
    SetMaterialColor(
        vehicle->model, vehicle->color[0] >> 3, vehicle->color[1] >> 3, vehicle->color[2] >> 3);
#endif
}

/** Make the options of a step those a player chooses from, with its name. */
static inline void
SetCarSelectOptions(s32 player, MenuItem **options, s32 count, const char *name, s32 cursor) {
    g_anCarSelectCursor[player] = cursor;
    g_aapCarSelectOptions[player] = options;
    g_anCarSelectOptionCount[player] = count;
    g_apszCarSelectCategory[player] = name;
}

/** Move the cursor of a player through the options with left and right. */
static inline void StepCarSelectOption(s32 player, u32 buttons) {
    s32 previous;

    if (buttons & INPUT_LEFT) {
        previous = g_anCarSelectCursor[player];
        g_anCarSelectPrevCursor[player] = previous;
        g_anCarSelectCursor[player] =
            (previous + g_anCarSelectOptionCount[player] - 1) % g_anCarSelectOptionCount[player];
        g_afCarSelectSlide[player] = 1.0f;
        PlayCarSelectSound(g_pScrollSoundBuf, g_dwScrollSoundSize);
    } else if (buttons & INPUT_RIGHT) {
        previous = g_anCarSelectCursor[player];
        g_anCarSelectPrevCursor[player] = previous;
        g_anCarSelectCursor[player] = (previous + 1) % g_anCarSelectOptionCount[player];
        g_afCarSelectSlide[player] = -1.0f;
        PlayCarSelectSound(g_pScrollSoundBuf, g_dwScrollSoundSize);
    }
}

/** Handle the buttons of a player on the screen. */
static inline void HandleCarSelectInput(s32 player) {
    Vehicle *vehicle = &g_aVehicles[player];
    s32 previous, state, page, i;
    s32 playSound = TRUE;
    s32 *slot = &g_adwCarSelectModelSlots[player];

    if (g_dwIsReviewMode) {
        vehicle->input->buttons &= ~(INPUT_UP | INPUT_DOWN | INPUT_LEFT | INPUT_RIGHT);
    } else {
        UpdateStickRepeat(vehicle->input);
    }
    state = g_adwCarSelectState[player];
    if (state == CAR_SELECT_READY) {
        if (vehicle->input->buttons & INPUT_B) {
            g_adwCarSelectState[player] = state - 1;
            TransferFlags(vehicle->input, INPUT_B);
        }
        return;
    }
    if (g_afCarSelectSlide[player] != 0.0f) {
        return;
    }

    /* Left and right turn to another car, which loads into the other slot. */
    if (state == CAR_SELECT_CAR) {
        previous = vehicle->selectedCar;
        if (vehicle->input->buttons & INPUT_LEFT) {
            do {
                vehicle->selectedCar = (vehicle->selectedCar + g_dwCarCount - 1) % g_dwCarCount;
            } while (!IsCarUnlocked(vehicle->selectedCar));
        }
        if (previous != vehicle->selectedCar) {
            g_afCarSelectSlide[player] = 1.0f;
        } else if (vehicle->input->buttons & INPUT_RIGHT) {
            do {
                vehicle->selectedCar = (vehicle->selectedCar + 1) % g_dwCarCount;
            } while (!IsCarUnlocked(vehicle->selectedCar));
            if (previous != vehicle->selectedCar) {
                g_afCarSelectSlide[player] = -1.0f;
            }
        }
        if (previous != vehicle->selectedCar) {
            PlayCarSelectSound(g_pScrollSoundBuf, g_dwScrollSoundSize);
            *slot ^= 2;
            LoadVehicleModel(&g_aVehicleModelSlots[*slot], *slot, vehicle->selectedCar, player);
        }
    }

#if TGR_REGION == TGR_REGION_JP
    /* Left and right move through the decals before the paint, and a slide waits a frame. */
    if (g_adwCarSelectState[player] == CAR_SELECT_DECALS) {
        StepCarSelectOption(player, vehicle->input->buttons);
    }
    if (g_afCarSelectSlide[player] != 0.0f) {
        return;
    }
#endif

    /* While choosing the car or its decals, C-down switches the look, and up and down paint it. */
    state = g_adwCarSelectState[player];
    if (state == CAR_SELECT_CAR || state == CAR_SELECT_DECALS) {
        if (vehicle->input->buttons & INPUT_C_DOWN) {
            TransferFlags(vehicle->input, INPUT_C_DOWN);
            if (vehicle->profile->unlocks & UNLOCK_OTHER_LOOK) {
                vehicle->otherLook ^= 1;
            }
        }
        if (vehicle->input->buttons & INPUT_UP) {
            AdjustPaint(vehicle, INPUT_UP);
        }
        if (vehicle->input->buttons & INPUT_DOWN) {
            AdjustPaint(vehicle, INPUT_DOWN);
        }
#if TGR_REGION != TGR_REGION_US
        /* The later releases paint the model of the slot of the player every frame. */
        SetMaterialColor(&g_aVehicleModels[*slot],
                         vehicle->color[0] >> 3,
                         vehicle->color[1] >> 3,
                         vehicle->color[2] >> 3);
#endif
    }

    /* Left and right move through the options of the other steps. */
#if TGR_REGION == TGR_REGION_JP
    if (g_adwCarSelectState[player] != CAR_SELECT_CAR &&
        g_adwCarSelectState[player] != CAR_SELECT_DECALS) {
#else
    if (g_adwCarSelectState[player] != CAR_SELECT_CAR) {
#endif
        StepCarSelectOption(player, vehicle->input->buttons);
    }

    if (vehicle->input->buttons & INPUT_B) {
        /* B leaves the screen from the car, and steps back from the options. */
        TransferFlags(vehicle->input, INPUT_B);
        state = g_adwCarSelectState[player];
        if (state == CAR_SELECT_CAR) {
            g_bCarSelectDone = FALSE;
            FadeSfxTo(0.0f, SCREEN_FADE_RATE);
            SetFadeTarget(0.0f, SCREEN_FADE_RATE);
        } else {
            g_adwCarSelectState[player] = --state;
            switch (state) {
            case CAR_SELECT_HANDLING:
                SetCarSelectOptions(player,
                                    g_apHandlingOptions,
                                    g_dwHandlingOptionCount,
                                    "Handling",
                                    vehicle->profile->handling);
                break;
            case CAR_SELECT_TRANSMISSION:
                SetCarSelectOptions(player,
                                    g_apTransmissionOptions,
                                    g_dwTransmissionOptionCount,
                                    "Transmission",
                                    vehicle->profile->transmission);
                break;
            case CAR_SELECT_TIRES:
                SetCarSelectOptions(player,
                                    g_apTiresOptions,
                                    g_dwTiresOptionCount,
                                    BACK_TIRES_LABEL,
                                    vehicle->profile->tires);
                break;
            case CAR_SELECT_SUSPENSION:
                SetCarSelectOptions(player,
                                    g_apSuspensionOptions,
                                    g_dwSuspensionOptionCount,
                                    "Suspension",
                                    vehicle->profile->suspension);
                break;
            case CAR_SELECT_DECALS:
                /* The options stay those of the suspension. */
                g_anCarSelectCursor[player] = 0;
                g_apszCarSelectCategory[player] = DECALS_LABEL;
                break;
            }
            g_anCarSelectPrevCursor[player] = g_anCarSelectCursor[player];
        }
    } else if (vehicle->input->buttons & CHOOSE_BUTTONS) {
        TransferFlags(vehicle->input, CHOOSE_BUTTONS);
        switch (g_adwCarSelectState[player]) {
        case CAR_SELECT_CAR:
            vehicle->carModel = vehicle->selectedCar;
            SetCarSelectOptions(player,
                                g_apHandlingOptions,
                                g_dwHandlingOptionCount,
                                "Handling",
                                vehicle->profile->handling);
            break;
        case CAR_SELECT_HANDLING:
            vehicle->profile->handling = g_anCarSelectCursor[player];
            SetCarSelectOptions(player,
                                g_apTransmissionOptions,
                                g_dwTransmissionOptionCount,
                                "Transmission",
                                vehicle->profile->transmission);
            break;
        case CAR_SELECT_TRANSMISSION:
            vehicle->profile->transmission = g_anCarSelectCursor[player];
            SetCarSelectOptions(player,
                                g_apTiresOptions,
                                g_dwTiresOptionCount,
                                TIRES_LABEL,
                                vehicle->profile->tires);
            break;
        case CAR_SELECT_TIRES:
            vehicle->profile->tires = g_anCarSelectCursor[player];
            SetCarSelectOptions(player,
                                g_apSuspensionOptions,
                                g_dwSuspensionOptionCount,
                                "Suspension",
                                vehicle->profile->suspension);
            break;
        case CAR_SELECT_SUSPENSION:
            /* The decals show the car, loaded at once into both slots. */
            vehicle->profile->suspension = g_anCarSelectCursor[player];
            g_anCarSelectCursor[player] = 0;
            g_aapCarSelectOptions[player] = g_apDecalOptions;
            g_anCarSelectOptionCount[player] = vehicle->carModel < PAINT_SHOP_CARS ?
                                                   g_dwDecalListCount - 1 :
                                                   g_dwDecalListCount - 2;
            g_apszCarSelectCategory[player] = DECALS_LABEL;
            *slot = player ^ 2;
            PlayCarSelectSound(g_pSelectSoundBuf, g_dwSelectSoundSize);
            playSound = FALSE;
            LoadVehicleModel(&g_aVehicleModelSlots[*slot], *slot, vehicle->carModel, player);
            while (PollVehicleModelDma(&g_aVehicleModelSlots[*slot])) {
            }
            LoadVehicleModel(
                &g_aVehicleModelSlots[*slot ^ 2], *slot ^ 2, vehicle->carModel, player);
            break;
        case CAR_SELECT_DECALS:
            if (g_anCarSelectCursor[player] == DECALS_LOAD) {
                g_dwPakDialogActive = TRUE;
                g_bPakDialogDone = FALSE;
                g_dwPakDialogResult = 0;
                g_dwPakDialogPlayerIdx = player;
                goto sound;
            }
            vehicle->customDecals = g_anCarSelectCursor[player] == DECALS_CUSTOM;
            break;
        }
        g_anCarSelectPrevCursor[player] = g_anCarSelectCursor[player];

        /* Start, the paint shop and the results skip the rest; the results turn a page. */
        if ((vehicle->input->buttons & INPUT_START) || g_dwIsPaintMode || g_dwIsReviewMode) {
            g_anCarSelectOptionCount[player] = g_dwDecalListCount - 1;
            g_anCarSelectCursor[player] = 0;
            g_anCarSelectPrevCursor[player] = 0;
            if (g_dwIsReviewMode) {
                previous = g_dwResultsPage;
                page = previous + 1;
                if (page == RESULTS_PAGE_PLAYER_2 && g_dwNumLocalPlayers != 2) {
                    page++;
                }
                if (page == RESULTS_PAGE_SEASON && g_dwGameMode != GAME_MODE_CHAMPIONSHIP) {
                    page = RESULTS_PAGE_END;
                }
                g_dwResultsPage = page;
                if (page == RESULTS_PAGE_MESSAGE) {
                    ProcessSeasonResults();
                    g_bSeasonResultsShown = TRUE;
                    if (g_dwSeasonResultMsgLineCount == 0) {
                        g_dwResultsPage++;
                    }
                }
                if (g_dwResultsPage == RESULTS_PAGE_END) {
                    /* Both are ready, and the last page stays in view as the screen fades. */
                    g_adwCarSelectState[1] = CAR_SELECT_READY;
                    g_adwCarSelectState[0] = CAR_SELECT_READY;
                    g_dwResultsPage = previous;
                }
            } else {
                g_adwCarSelectState[player] = CAR_SELECT_READY;
            }
        } else {
            g_adwCarSelectState[player]++;
        }

        /* Once every player is ready, the screen fades out. */
        for (i = 0; i < (s32)g_dwNumLocalPlayers; i++) {
            if (g_adwCarSelectState[i] != CAR_SELECT_READY) {
                goto sound;
            }
        }
        g_bCarSelectDone = TRUE;
        FadeSfxTo(0.0f, SCREEN_FADE_RATE);
        if (!g_dwIsPaintMode) {
            FadeMusicTo(0.0f, SCREEN_FADE_RATE);
        }
        SetFadeTarget(0.0f, SCREEN_FADE_RATE);
    } else {
        return;
    }
sound:
    if (playSound) {
        PlayCarSelectSound(g_pSelectSoundBuf, g_dwSelectSoundSize);
    }
}

void ProcessCarSelectScreen(void) {
    char text[96], time[32];
    s32 player, i;

    if (!g_bCarSelectInitialized) {
        StartCarSelect();
    }
    UpdateFrameTiming();
    UpdateFadeTransitions();
    StartHighResFrame();
#if TGR_REGION == TGR_REGION_JP
    PaintPendingModels();
#endif
    g_dwWeatherActive = FALSE;
    g_dwNightMode = FALSE;
    g_dwRainMode = FALSE;
    g_dwSnowMode = FALSE;
    SetupFog();
    SetupWeatherLights();
    AddTimingMarker(0, 0, 0, 0xC8, 0xFF);
    g_aViewportRects[0].vehicleIndex = 0;
    g_aViewportRects[1].vehicleIndex = 1;
    /* Set each frame, so that the title keeps being drawn. */
    g_dwTitleRedrawFrames = 2;
    ClearDepthBuffer();
    if (g_dwIsReviewMode) {
        DrawTexturedRect(&g_BgTextureDesc,
                         0,
                         0,
                         320,
                         240,
                         0,
                         0,
                         0,
                         0xFF,
                         0,
                         0x82,
                         0x8C,
                         0xFF,
                         TEXRECT_BLEND_SHADED_OPAQUE);
    } else {
        DrawTexturedRect(&g_BgTextureDesc,
                         0,
                         0,
                         320,
                         240,
                         0,
                         0,
                         0,
                         0xFF,
                         0x50,
                         0,
                         0,
                         0xFF,
                         TEXRECT_BLEND_SHADED_OPAQUE);
    }
    if (g_dwTitleRedrawFrames == 0 && IsFadeComplete()) {
        SetScissorRect(0, 0, g_dwScreenWidth, g_dwScreenHeight);
    }

    /* The stages turn, the second the other way, and the cars slide to rest. */
    g_afCarSelectAngle[0] += g_fFrameDeltaTime;
    g_afCarSelectAngle[1] += 6.283185482f - g_fFrameDeltaTime;
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        while (6.283185482f < g_afCarSelectAngle[player]) {
            g_afCarSelectAngle[player] -= 6.283185482f;
        }
    }
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        if (g_afCarSelectSlide[player] < 0.0f) {
            g_afCarSelectSlide[player] += g_fFrameDeltaTime + g_fFrameDeltaTime;
            if (0.0f < g_afCarSelectSlide[player]) {
                g_afCarSelectSlide[player] = 0.0f;
            }
#if TGR_REGION == TGR_REGION_JP
            /* A sliding decal choice other than a load repaints the model. */
            if (g_adwCarSelectState[player] == CAR_SELECT_DECALS &&
                g_anCarSelectCursor[player] != DECALS_LOAD) {
                g_adwCarSelectPaintPending[player] = TRUE;
            }
#endif
        } else if (0.0f < g_afCarSelectSlide[player]) {
            g_afCarSelectSlide[player] -= g_fFrameDeltaTime + g_fFrameDeltaTime;
            if (g_afCarSelectSlide[player] < 0.0f) {
                g_afCarSelectSlide[player] = 0.0f;
            }
#if TGR_REGION == TGR_REGION_JP
            if (g_adwCarSelectState[player] == CAR_SELECT_DECALS &&
                g_anCarSelectCursor[player] != DECALS_LOAD) {
                g_adwCarSelectPaintPending[player] = TRUE;
            }
#endif
        }
#if TGR_REGION == TGR_REGION_JP
        /* A model still loading is painted once it has loaded. */
        if (!g_adwCarSelectPaintPending[player] &&
            !IsVehicleModelLoaded(&g_aVehicleModelSlots[g_adwCarSelectModelSlots[player]])) {
            g_adwCarSelectPaintPending[player] = -1;
        }
#endif
    }

    /* The slots of the cars shown when ready, and with the decals. */
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        if (g_adwCarSelectState[player] == CAR_SELECT_READY) {
            if (!g_dwIsPaintMode && !g_dwIsReviewMode) {
                g_adwCarSelectModelSlots[player] =
                    (g_aVehicles[player].customDecals << 1) ^ (player ^ 2);
            }
        } else if (g_adwCarSelectState[player] == CAR_SELECT_DECALS) {
            if (g_anCarSelectCursor[player] == DECALS_DEFAULT) {
                g_adwCarSelectModelSlots[player] = player ^ 2;
            } else if (g_anCarSelectCursor[player] == DECALS_CUSTOM) {
                g_adwCarSelectModelSlots[player] = player;
            } else if (g_anCarSelectCursor[player] == DECALS_LOAD) {
                g_adwCarSelectModelSlots[player] =
                    g_anCarSelectPrevCursor[player] != 0 ? player ^ 2 : player;
            }
        }
    }
    g_dwSnowMode = FALSE;
    g_dwWeatherActive = FALSE;
    g_dwRainMode = FALSE;
    g_dwNightMode = FALSE;

    if (g_dwIsReviewMode) {
        i = g_dwResultsPage == RESULTS_PAGE_PLAYER_2;
        DrawPlayerStage(0, i, g_adwCarSelectModelSlots[i], 59, g_dwScreenHeight, SPREAD_TWO_VIEWS);
    } else if (g_dwNumLocalPlayers == 1) {
        DrawPlayerStage(0, 0, g_adwCarSelectModelSlots[0], 45, g_dwScreenHeight, SPREAD_ONE_VIEW);
    } else {
        DrawPlayerStage(
            0, 0, g_adwCarSelectModelSlots[0], 0, g_dwScreenHeight - 24, SPREAD_TWO_VIEWS);
        DrawPlayerStage(1, 1, g_adwCarSelectModelSlots[1], 71, g_dwScreenHeight, SPREAD_TWO_VIEWS);
    }
#if TGR_REGION == TGR_REGION_JP
    /* A model that was loading is painted next frame once it has loaded. */
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        if (g_adwCarSelectPaintPending[player] == -1) {
            if (IsVehicleModelLoaded(&g_aVehicleModelSlots[g_adwCarSelectModelSlots[player]])) {
                g_adwCarSelectPaintPending[player] = TRUE;
            } else {
                g_adwCarSelectPaintPending[player] = FALSE;
            }
        }
    }
#endif /* TGR_REGION == TGR_REGION_JP */
    SetupMenuProjection((f32)g_dwScreenWidth, (f32)g_dwScreenHeight);
    SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
    if (g_dwTitleRedrawFrames != 0 || !IsFadeComplete()) {
        DrawCarSelectTitle(text);
        if (IsFadeComplete()) {
            g_dwTitleRedrawFrames--;
        }
    }

    if (g_dwIsReviewMode) {
        SetTextStyleNormal();
        SetTextAlignCenter();
        SetTextSize(20);
        if (g_dwResultsPage == RESULTS_PAGE_PLAYER_1 || g_dwResultsPage == RESULTS_PAGE_PLAYER_2) {
            DrawRaceResults(g_dwResultsPage != RESULTS_PAGE_PLAYER_1, time);
        } else if (g_dwResultsPage == RESULTS_PAGE_MESSAGE) {
            DrawSeasonMessage();
        } else {
            DrawSeasonResults();
        }
    } else if (g_dwNumLocalPlayers == 2) {
        DrawTwoPlayerChoices(text);
    } else {
        DrawOnePlayerChoices(text);
    }

    if (g_dwTitleRedrawFrames != 0 || IsFadingOut() || IsFadingIn()) {
        if (g_dwIsReviewMode) {
            DrawPauseMenuButtons();
        } else {
            DrawMenuFooterButtons();
        }
    }
    ApplyScreenFade();
    if (g_dwHudDebugLevel >= 2) {
        SetViewport(0, 0, g_dwScreenWidth, g_dwScreenHeight, TRUE);
        DrawTimingBars();
    }
    if (g_dwPakDialogActive) {
        RunPakDialog();
    }
    FlushDisplayList();

    if (IsFadingOut()) {
        if (IsFadeComplete()) {
            LeaveCarSelect();
        }
        return;
    }
    if (g_dwPakDialogActive) {
        if (g_bPakDialogDone) {
            g_bPakDialogDone = FALSE;
            g_dwPakDialogActive = FALSE;
        }
        return;
    }
    /* In the paint shop, the player of the screen steers the first vehicle. */
    if (g_dwIsPaintMode) {
        g_aVehicles[0].input->buttons = g_aVehicles[g_dwScreenPlayerIdx].input->buttons;
        g_aVehicles[0].input->stickX = g_aVehicles[g_dwScreenPlayerIdx].input->stickX;
        g_aVehicles[0].input->stickY = g_aVehicles[g_dwScreenPlayerIdx].input->stickY;
    }
    for (player = 0; player < (s32)g_dwNumLocalPlayers; player++) {
        HandleCarSelectInput(player);
    }
}

void ProcessRaceResultsScreen(void) {
    g_dwIsReviewMode = TRUE;
    ProcessCarSelectScreen();
    g_dwIsReviewMode = FALSE;
}
#endif
