#ifndef COASTLINE_RACE_H
#define COASTLINE_RACE_H

/**
 * @file
 * The screen of a race: its phases, its views, the pause menu, the results it keeps for the
 * results screen, and the credits the attract races show.
 */

#include <PR/ultratypes.h>

#include "laps.h"
#include "model.h"
#include "pause.h"
#include "render.h"
#include "vecmath.h"

#if TGR_PLATFORM == TGR_PLATFORM_WIN
#include "platform/win/airplanestate.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** The phases of a race, which g_aRacePhaseTable times. */
enum {
    RACE_PHASE_HOLD,      /*!< The start lights stay in their first pose. */
    RACE_PHASE_WAIT,      /*!< A timed wait before the countdown that changes nothing. */
    RACE_PHASE_COUNTDOWN, /*!< The start lights count down. */
    RACE_PHASE_GO,        /*!< The start lights rise away and the vehicles go. */
    RACE_PHASE_RACING,    /*!< The race itself, until its routine ends the phase. */
    RACE_PHASE_FINISHED, /*!< The race is over, and the results are saved for the results screen. */
    RACE_PHASE_AFTER,    /*!< A timed wait after the finish. */
    RACE_PHASE_LEAVE,    /*!< The screen, the music, and the sound fade out. */
};

/** The phases of g_aRacePhaseTable. */
#define RACE_PHASES 8

/** The steps of the countdown of the start lights. */
#define START_LIGHT_STEPS 5

/** A phase of a race and how long it lasts. */
typedef struct {
    s32 phase;   /*!< The phase, by the RACE_PHASE_ values. */
    f32 seconds; /*!< Its length in seconds, or 0 for a phase another condition ends. */
} RacePhaseStep;

/** The lines of a caption of the credits. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#define CAPTION_LINES 6
#else
#define CAPTION_LINES 5
#endif

/** A caption the credits show over a race. */
typedef struct {
    const char *title; /*!< The small title above the lines, or NULL to end the captions. */
    f32 duration;      /*!< The time it shows, in seconds. */
    const char *lines[CAPTION_LINES]; /*!< The lines, ending with NULL when there are fewer than
                                           #CAPTION_LINES. */
#if TGR_PLATFORM != TGR_PLATFORM_WIN
    u8 unreconstructed1C[0x20 - 0x1C]; /*!< Not yet reconstructed. */
#endif
} RaceCaption;

/** A line of the credits that scroll up at the end of the championship. */
typedef struct {
    s32 y;            /*!< The baseline on the screen, which moves up a pixel each frame. */
    s32 size;         /*!< The size of its text, in pixels. */
    s32 gap;          /*!< The space below it, in pixels. */
    const char *text; /*!< Its text, or NULL to end the credits. */
} CreditLine;

/** The players whose results are kept for the results screen. */
#define SAVED_RESULTS 2

/**
 * The cartridge offsets of the music of each track, the mirrored ones after the others, then that
 * of the race after a season.
 *
 * @ghidraAddress USA: 0x8026ff24
 * @ghidraAddress EUR: 0x80271ef4
 * @ghidraAddress JPN: 0x802719a4
 */
extern u32 g_adwTrackMusicRom[12];

extern u8 _startLightsSegmentRomStart[];
extern u8 _startLightsSegmentRomEnd[];

/**
 * The model of the start lights.
 *
 * @ghidraAddress USA: 0x8026ff54
 * @ghidraAddress EUR: 0x80271f24
 * @ghidraAddress JPN: 0x802719d4
 * @ghidraAddress BRD3D: 0x10680944
 * @ghidraAddress BRGlide: 0x105bcaec
 */
extern Model *g_pStartLightsModel;

/**
 * How the start lights animate: 0 not at all, -1 held in their first pose, and 1 running.
 *
 * @ghidraAddress USA: 0x8026ff58
 * @ghidraAddress EUR: 0x80271f28
 * @ghidraAddress JPN: 0x802719d8
 * @ghidraAddress BRD3D: 0x106909b0
 * @ghidraAddress BRGlide: 0x105ccb58
 */
extern s32 g_dwStartLightsAnimation;

/**
 * Whether the start lights are drawn.
 *
 * @ghidraAddress USA: 0x8026ff5c
 * @ghidraAddress EUR: 0x80271f2c
 * @ghidraAddress JPN: 0x802719dc
 * @ghidraAddress BRD3D: 0x106909d0
 */
extern s32 g_bStartLightsVisible;

/**
 * How far the start lights rise out of view once the race starts.
 *
 * @ghidraAddress USA: 0x8026ff60
 * @ghidraAddress EUR: 0x80271f30
 * @ghidraAddress JPN: 0x802719e0
 * @ghidraAddress BRD3D: 0x106805bc
 */
extern f32 g_fStartLightsDrop;

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/**
 * The placed model of the airplane, or 0 when the track has none or it landed, and the model a
 * vehicle sends it off from on the lap g_dwAirplaneTriggerLap.
 *
 * @ghidraAddress USA: 0x8026ff64
 * @ghidraAddress EUR: 0x80271f34
 * @ghidraAddress JPN: 0x802719e4
 * @ghidraAddress BRD3D: 0x10680618
 */
extern s32 g_dwAirplaneInstance;

/**
 * @ghidraAddress USA: 0x8026ff68
 * @ghidraAddress EUR: 0x80271f38
 * @ghidraAddress JPN: 0x802719e8
 * @ghidraAddress BRD3D: 0x1068061c
 */
extern s32 g_dwAirplaneTrigger;

/**
 * @ghidraAddress USA: 0x8026ff6c
 * @ghidraAddress EUR: 0x80271f3c
 * @ghidraAddress JPN: 0x802719ec
 * @ghidraAddress BRD3D: 0x10680620
 */
extern s32 g_dwAirplaneTriggerLap;

/**
 * Whether the airplane flies.
 *
 * @ghidraAddress USA: 0x8026ff70
 * @ghidraAddress EUR: 0x80271f40
 * @ghidraAddress JPN: 0x802719f0
 * @ghidraAddress BRD3D: 0x10680624
 */
extern s32 g_bAirplaneFlying;

/**
 * The node of the path the airplane flies towards, and the nodes of the path.
 *
 * @ghidraAddress USA: 0x8026ff74
 * @ghidraAddress EUR: 0x80271f44
 * @ghidraAddress JPN: 0x802719f4
 * @ghidraAddress BRD3D: 0x10680628
 */
extern s32 g_dwAirplaneNode;

/**
 * @ghidraAddress USA: 0x8026ff78
 * @ghidraAddress EUR: 0x80271f48
 * @ghidraAddress JPN: 0x802719f8
 * @ghidraAddress BRD3D: 0x1068062c
 */
extern s32 g_dwAirplaneNodeCount;

/**
 * The scale of the model of the airplane.
 *
 * @ghidraAddress USA: 0x8026ff7c
 * @ghidraAddress EUR: 0x80271f4c
 * @ghidraAddress JPN: 0x802719fc
 * @ghidraAddress BRD3D: 0x10680630
 */
extern f32 g_fAirplaneScale;

/**
 * The left and right edges of the path of the airplane.
 *
 * @ghidraAddress USA: 0x8026ff80
 * @ghidraAddress EUR: 0x80271f50
 * @ghidraAddress JPN: 0x80271a00
 * @ghidraAddress BRD3D: 0x10680634
 */
extern Vec3f *g_pAirplanePathLeft;

/**
 * @ghidraAddress USA: 0x8026ff84
 * @ghidraAddress EUR: 0x80271f54
 * @ghidraAddress JPN: 0x80271a04
 * @ghidraAddress BRD3D: 0x10680638
 */
extern Vec3f *g_pAirplanePathRight;

/**
 * How far the airplane flew along the leg of the path to the current node, and its length.
 *
 * @ghidraAddress USA: 0x8026ff88
 * @ghidraAddress EUR: 0x80271f58
 * @ghidraAddress JPN: 0x80271a08
 * @ghidraAddress BRD3D: 0x1068063c
 */
extern f32 g_fAirplaneDistance;

/**
 * @ghidraAddress USA: 0x8026ff8c
 * @ghidraAddress EUR: 0x80271f5c
 * @ghidraAddress JPN: 0x80271a0c
 * @ghidraAddress BRD3D: 0x10680640
 */
extern f32 g_fAirplaneSegmentLength;

/**
 * The directions of the legs of the path before, along and after the one the airplane flies, which
 * it turns between.
 *
 * @ghidraAddress USA: 0x8026ff90
 * @ghidraAddress EUR: 0x80271f60
 * @ghidraAddress JPN: 0x80271a10
 * @ghidraAddress BRD3D: 0x10680644
 */
extern Vec3f g_vAirplaneDirPrev;

/**
 * @ghidraAddress USA: 0x8026ff9c
 * @ghidraAddress EUR: 0x80271f6c
 * @ghidraAddress JPN: 0x80271a1c
 * @ghidraAddress BRD3D: 0x10680650
 */
extern Vec3f g_vAirplaneDir;

/**
 * @ghidraAddress USA: 0x8026ffa8
 * @ghidraAddress EUR: 0x80271f78
 * @ghidraAddress JPN: 0x80271a28
 * @ghidraAddress BRD3D: 0x1068065c
 */
extern Vec3f g_vAirplaneDirNext;
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */

/**
 * The captions of the credits, ending with a NULL title.
 *
 * @ghidraAddress USA: 0x8026ffb4
 * @ghidraAddress EUR: 0x80271f84
 * @ghidraAddress JPN: 0x80271a34
 * @ghidraAddress BRD3D: 0x100aa018
 */
extern RaceCaption g_aCreditCaptions[];

/**
 * The scrolling credits, ending with a NULL text.
 *
 * @ghidraAddress USA: 0x80270194
 * @ghidraAddress EUR: 0x80272164
 * @ghidraAddress JPN: 0x80271c14
 * @ghidraAddress BRD3D: 0x100a66f0
 * @ghidraAddress BRGlide: 0x100a5eb0
 */
extern CreditLine g_aCreditScrollLines[];

/**
 * Whether leaving the race starts its instant replay.
 *
 * @ghidraAddress USA: 0x8027078c
 * @ghidraAddress EUR: 0x8027275c
 * @ghidraAddress JPN: 0x8027220c
 * @ghidraAddress BRD3D: 0x106909e4
 */
extern s32 g_bReplayAfterRace;

/**
 * Whether the results of the last race are kept.
 *
 * @ghidraAddress USA: 0x80270790
 * @ghidraAddress EUR: 0x80272760
 * @ghidraAddress JPN: 0x80272210
 * @ghidraAddress BRD3D: 0x106909b8
 */
extern s32 g_dwSavedResultsValid;

/**
 * The times left in the countdown at which the start lights change, in seconds.
 *
 * @ghidraAddress USA: 0x80270794
 * @ghidraAddress EUR: 0x80272764
 * @ghidraAddress JPN: 0x80272214
 * @ghidraAddress BRD3D: 0x100aa1f8
 */
extern f32 g_afStartLightThresholds[START_LIGHT_STEPS];

/**
 * Whether a time attack left something to save, so that its results open the save menu.
 *
 * @ghidraAddress USA: 0x802707a8
 * @ghidraAddress EUR: 0x80272778
 * @ghidraAddress JPN: 0x80272228
 * @ghidraAddress BRD3D: 0x10680610
 */
extern s32 g_dwSaveAvailable;

/**
 * The points of each place of a race of the championship.
 *
 * @ghidraAddress USA: 0x802707ac
 * @ghidraAddress EUR: 0x8027277c
 * @ghidraAddress JPN: 0x8027222c
 * @ghidraAddress BRD3D: 0x100aa210
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
extern u8 g_abRacePositionPoints[20];
#else
extern u8 g_abRacePositionPoints[];
#endif

/**
 * @ghidraAddress USA: 0x802707c0
 * @ghidraAddress EUR: 0x80272790
 * @ghidraAddress JPN: 0x80272240
 */
extern s32 g_dwSeasonResultMsgLineCount;

/**
 * The waterfalls of the track, as placed models.
 *
 * @ghidraAddress USA: 0x802707c4
 * @ghidraAddress EUR: 0x80272794
 * @ghidraAddress JPN: 0x80272244
 * @ghidraAddress BRD3D: 0x10680940
 */
extern s32 g_dwWaterfallCount;

/**
 * Whether the race is set up.
 *
 * @ghidraAddress USA: 0x802707c8
 * @ghidraAddress EUR: 0x8027279c
 * @ghidraAddress JPN: 0x8027224c
 * @ghidraAddress BRD3D: 0x106909ec
 */
extern s32 g_dwRaceInitialized;

/**
 * Whether the screen is leaving the race.
 *
 * @ghidraAddress USA: 0x802707cc
 * @ghidraAddress EUR: 0x802727a0
 * @ghidraAddress JPN: 0x80272250
 * @ghidraAddress BRD3D: 0x106909f0
 */
extern s32 g_dwRaceExitState;

/**
 * The phases of a race in their order.
 *
 * @ghidraAddress USA: 0x802707d0
 * @ghidraAddress EUR: 0x802727a4
 * @ghidraAddress JPN: 0x80272254
 * @ghidraAddress BRD3D: 0x100aa228
 */
extern RacePhaseStep g_aRacePhaseTable[RACE_PHASES];

/**
 * The vehicles that finished the race so far.
 *
 * @ghidraAddress USA: 0x8028b300
 * @ghidraAddress EUR: 0x8028d5b0
 * @ghidraAddress JPN: 0x8028cfe0
 * @ghidraAddress BRD3D: 0x118ab470
 */
extern s32 g_dwRaceFinishOrderCounter;

/**
 * The time left in the phase of the race, in seconds.
 *
 * @ghidraAddress USA: 0x802f7ef0
 * @ghidraAddress EUR: 0x802f9be4
 * @ghidraAddress JPN: 0x802f8624
 * @ghidraAddress BRD3D: 0x106806d8
 */
extern f32 g_fRacePhaseTimer;

#if TGR_PLATFORM != TGR_PLATFORM_WIN
/**
 * The step of the countdown of the start lights.
 *
 * @ghidraAddress USA: 0x802f7ef4
 * @ghidraAddress EUR: 0x802f9be8
 * @ghidraAddress JPN: 0x802f8628
 * @ghidraAddress BRD3D: 0x106909cc
 */
extern s32 g_dwStartLightsStep;
#endif /* TGR_PLATFORM != TGR_PLATFORM_WIN */

/**
 * The index of the phase in g_aRacePhaseTable.
 *
 * @ghidraAddress USA: 0x802f7ef8
 * @ghidraAddress EUR: 0x802f9bec
 * @ghidraAddress JPN: 0x802f862c
 * @ghidraAddress BRD3D: 0x106805a8
 */
extern s32 g_dwRacePhaseIndex;

/**
 * Set once the loop of the race screen ends, and cleared when a race starts.
 *
 * @ghidraAddress USA: 0x802a4c04
 * @ghidraAddress EUR: 0x802a738c
 * @ghidraAddress JPN: 0x802a6e2c
 * @ghidraAddress BRD3D: 0x11782bc0
 */
extern s16 g_wRaceScreenLoopExited;

/**
 * The results of the local players the results screen shows: the place, the time, the best lap and
 * its time, the laps and the time of each.
 *
 * @ghidraAddress USA: 0x80315d60
 * @ghidraAddress EUR: 0x80317a50
 * @ghidraAddress JPN: 0x80316490
 * @ghidraAddress BRD3D: 0x106805c8
 */
extern s32 g_adwSavedResultPositions[SAVED_RESULTS];

/**
 * @ghidraAddress USA: 0x80315d68
 * @ghidraAddress EUR: 0x80317a58
 * @ghidraAddress JPN: 0x80316498
 * @ghidraAddress BRD3D: 0x10680728
 */
extern f32 g_afSavedResultTimes[SAVED_RESULTS];

/**
 * @ghidraAddress USA: 0x80315d70
 * @ghidraAddress EUR: 0x80317a60
 * @ghidraAddress JPN: 0x803164a0
 * @ghidraAddress BRD3D: 0x106909c0
 */
extern f32 g_afSavedResultBestLapTimes[SAVED_RESULTS];

/**
 * @ghidraAddress USA: 0x80315d78
 * @ghidraAddress EUR: 0x80317a68
 * @ghidraAddress JPN: 0x803164a8
 * @ghidraAddress BRD3D: 0x10690950
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
extern f32 g_aafSavedResultLapTimes[SAVED_RESULTS][12];
#else
extern f32 g_aafSavedResultLapTimes[SAVED_RESULTS][5];
#endif

/**
 * @ghidraAddress USA: 0x80315da0
 * @ghidraAddress EUR: 0x80317a90
 * @ghidraAddress JPN: 0x803164d0
 * @ghidraAddress BRD3D: 0x10680748
 */
extern s32 g_adwSavedResultBestLaps[SAVED_RESULTS];

/**
 * @ghidraAddress USA: 0x80315da8
 * @ghidraAddress EUR: 0x80317a98
 * @ghidraAddress JPN: 0x803164d8
 * @ghidraAddress BRD3D: 0x106805b0
 */
extern s32 g_adwSavedResultLaps[SAVED_RESULTS];

/**
 * The lines of the message of the result of a season.
 *
 * @ghidraAddress USA: 0x80315db0
 * @ghidraAddress EUR: 0x80317aa0
 * @ghidraAddress JPN: 0x803164e0
 * @ghidraAddress BRD3D: 0x10680940
 */
extern const char *g_apszSeasonResultMsgLines[];

/**
 * The text of a line of the pause menu.
 *
 * @ghidraAddress USA: 0x80315dd8
 * @ghidraAddress EUR: 0x80317ac8
 * @ghidraAddress JPN: 0x80316508
 */
extern char g_szFormatBuffer[0x20];

/**
 * @ghidraAddress USA: 0x80315df8
 * @ghidraAddress EUR: 0x80317ae8
 * @ghidraAddress JPN: 0x80316528
 * @ghidraAddress BRD3D: 0x106805d0
 */
extern s32 g_adwWaterfallInstances[16];

/**
 * The frames the screen is still cleared, after the views changed or the pause ended.
 *
 * @ghidraAddress USA: 0x80315e38
 * @ghidraAddress EUR: 0x80317b28
 * @ghidraAddress JPN: 0x80316568
 */
extern s32 g_dwScreenClearFrames;

/**
 * The time of the caption shown and its index.
 *
 * @ghidraAddress USA: 0x80315e68
 * @ghidraAddress EUR: 0x80317b58
 * @ghidraAddress JPN: 0x80316598
 * @ghidraAddress BRD3D: 0x106806dc
 */
extern f32 g_fRaceCaptionTimer;

/**
 * @ghidraAddress USA: 0x80315e6c
 * @ghidraAddress EUR: 0x80317b5c
 * @ghidraAddress JPN: 0x8031659c
 * @ghidraAddress BRD3D: 0x106805c0
 */
extern s32 g_dwRaceCaptionIndex;

/**
 * Whether a button ended the race the computer drives, which then returns to the main menu rather
 * than the splash screens.
 *
 * @ghidraAddress USA: 0x80315e70
 * @ghidraAddress EUR: 0x80317b60
 * @ghidraAddress JPN: 0x803165a0
 * @ghidraAddress BRD3D: 0x10680938
 */
extern s32 g_bDemoInterrupted;

/**
 * The views of the last frame.
 *
 * @ghidraAddress USA: 0x80315e74
 * @ghidraAddress EUR: 0x80317b64
 * @ghidraAddress JPN: 0x803165a4
 */
extern s32 g_dwPrevViewportCount;

/**
 * The phase of the race, by the RACE_PHASE_ values.
 *
 * @ghidraAddress USA: 0x80315e78
 * @ghidraAddress EUR: 0x802f9be0
 * @ghidraAddress JPN: 0x802f8620
 * @ghidraAddress BRD3D: 0x10680750
 */
extern s32 g_dwRacePhase;

/**
 * The names of the places in a race: "1st" and on.
 *
 * @ghidraAddress USA: 0x80271fe0
 * @ghidraAddress EUR: 0x80273fc0
 * @ghidraAddress JPN: 0x80273a80
 */
extern const char *g_apszPlaceNames[];

/**
 * Decompress the music of the selected track and start it.
 *
 * @ghidraAddress USA: 0x80200320
 * @ghidraAddress EUR: 0x80200350
 * @ghidraAddress JPN: 0x80200310
 */
void LoadAndPlayTrackBgm(void);

/**
 * Advance the animations of the start lights, holding them in place while they hold their first
 * pose.
 *
 * @ghidraAddress USA: 0x8020037c
 * @ghidraAddress EUR: 0x802003ac
 * @ghidraAddress JPN: 0x8020036c
 * @ghidraAddress BRD3D: 0x1002c2d0
 * @ghidraAddress BRGlide: 0x10019840
 */
void AnimateStartLights(void);

/**
 * Move the animations of the race between frames, while the race is not paused.
 *
 * @ghidraAddress USA: 0x802003e4
 * @ghidraAddress EUR: 0x80200414
 * @ghidraAddress JPN: 0x802003d4
 * @ghidraAddress BRD3D: 0x1002c320
 * @ghidraAddress BRGlide: 0x10019890
 */
void UpdateRaceAnimations(void);

/**
 * Draw the start lights over the start of the track.
 *
 * @ghidraAddress USA: 0x8020046c
 * @ghidraAddress EUR: 0x8020049c
 * @ghidraAddress JPN: 0x8020045c
 * @ghidraAddress BRD3D: 0x100147b0
 * @ghidraAddress BRGlide: 0x10011d20
 */
void RenderStartLights(void);

/**
 * Place the lines of the scrolling credits below the screen.
 *
 * @ghidraAddress USA: 0x802006c8
 * @ghidraAddress EUR: 0x802006f8
 * @ghidraAddress JPN: 0x802006b8
 * @ghidraAddress BRD3D: 0x1002c3c0
 * @ghidraAddress BRGlide: 0x10019930
 */
void CalculateScrollPositions(void);

/**
 * Move the scrolling credits up a pixel.
 *
 * @ghidraAddress USA: 0x8020072c
 * @ghidraAddress EUR: 0x8020075c
 * @ghidraAddress JPN: 0x8020071c
 * @ghidraAddress BRD3D: 0x1002c410
 * @ghidraAddress BRGlide: 0x10019980
 */
void ScrollTextUp(void);

/**
 * Draw the lines of the scrolling credits that are on the screen.
 *
 * @ghidraAddress USA: 0x80200764
 * @ghidraAddress EUR: 0x80200794
 * @ghidraAddress JPN: 0x80200754
 * @ghidraAddress BRD3D: 0x10015bd0
 * @ghidraAddress BRGlide: 0x10013140
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
void DrawScrollingText(const ViewportRect *viewport);
#else
void DrawScrollingText(void);
#endif

/**
 * Process the racing screen. The N64 version steps the race and renders its views; the Windows
 * version renders a captured frame, its HUD, and its pause or credits overlays.
 *
 * @ghidraAddress USA: 0x8020082c
 * @ghidraAddress EUR: 0x8020085c
 * @ghidraAddress JPN: 0x8020081c
 * @ghidraAddress BRD3D: 0x10014a30
 * @ghidraAddress BRGlide: 0x10011fa0
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
#ifdef __cplusplus
extern "C"
#endif
    void ProcessRacingScreen(int snapshotSlot);
#else
void ProcessRacingScreen(void);
#endif

/**
 * Keep the results of the local players for the results screen.
 *
 * @ghidraAddress USA: 0x80205fdc
 * @ghidraAddress EUR: 0x8020619c
 * @ghidraAddress JPN: 0x80206270
 * @ghidraAddress BRD3D: 0x1002f2a0
 * @ghidraAddress BRGlide: 0x1001c810
 */
void SaveRaceResultsToSlots(void);

/**
 * Give the kept results back to the vehicles, scoring them in the championship first.
 *
 * @ghidraAddress USA: 0x802060dc
 * @ghidraAddress EUR: 0x8020629c
 * @ghidraAddress JPN: 0x80206370
 * @ghidraAddress BRD3D: 0x1002f320
 * @ghidraAddress BRGlide: 0x1001c890
 */
void RestoreRaceResultsFromSlots(void);

/**
 * Choose the track of the next race of the championship.
 *
 * @ghidraAddress USA: 0x80206304
 * @ghidraAddress EUR: 0x802064c4
 * @ghidraAddress JPN: 0x80206598
 * @ghidraAddress BRD3D: 0x1002f460
 * @ghidraAddress BRGlide: 0x1001c9d0
 */
#ifdef __cplusplus
extern "C"
#endif
    void SetTrackFromSeason(void);

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/**
 * @ghidraAddress BRD3D: 0x1002c500
 */
#ifdef __cplusplus
extern "C"
#endif
    void RunRaceGameScreen(void);
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */

/**
 * Score championship progress and unlock bonuses when the season's points qualify.
 *
 * @ghidraAddress USA: 0x802063a4
 * @ghidraAddress EUR: 0x80206564
 * @ghidraAddress JPN: 0x80206638
 * @ghidraAddress BRD3D: 0x1002f4c0
 * @ghidraAddress BRGlide: 0x1001ca30
 */
void ProcessSeasonResults(void);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACE_H */
