#ifndef COASTLINE_RACE_H
#define COASTLINE_RACE_H

/**
 * @file
 * The screen of a race: its phases, its views, the pause menu, the results it keeps for the
 * results screen, and the credits the attract races show.
 */

#include <PR/ultratypes.h>

#include "model.h"
#include "vecmath.h"

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

/** The items of the pause menu. */
enum {
    PAUSE_CONTINUE, /*!< Return to the race. */
    PAUSE_RESTART,  /*!< Restart the race. */
    PAUSE_MUSIC,    /*!< The volume of the music, changed with left and right. */
    PAUSE_SOUND,    /*!< The volume of the sound effects, changed with left and right. */
    PAUSE_EXIT,     /*!< Exit to the main menu. */
    PAUSE_ITEMS,    /*!< The number of items. */
};

/** A phase of a race and how long it lasts. */
typedef struct {
    s32 phase;   /*!< The phase, by the RACE_PHASE_ values. */
    f32 seconds; /*!< Its length in seconds, or 0 for a phase another condition ends. */
} RacePhaseStep;

/** The lines of a caption of the credits. */
#define CAPTION_LINES 5

/** A caption the credits show over a race. */
typedef struct {
    const char *title; /*!< The small title above the lines, or NULL to end the captions. */
    f32 duration;      /*!< The time it shows, in seconds. */
    const char *lines[CAPTION_LINES];  /*!< The lines, ending with NULL when there are fewer than
                                            CAPTION_LINES. */
    u8 unreconstructed1C[0x20 - 0x1C]; /*!< Not yet reconstructed. */
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
 * Whether the race is paused.
 *
 * @ghidraAddress USA: 0x8026ff10
 * @ghidraAddress JPN: 0x80271990
 * @ghidraAddress EUR: 0x80271ee0
 */
extern u32 g_dwRacePauseState;

/**
 * The item of the pause menu, by the PAUSE_ values.
 *
 * @ghidraAddress USA: 0x8026ff14
 * @ghidraAddress JPN: 0x80271994
 * @ghidraAddress EUR: 0x80271ee4
 */
extern s32 g_dwPauseMenuSelection;

/**
 * The cartridge offsets of the music of each track, the mirrored ones after the others, then that
 * of the race after a season.
 *
 * @ghidraAddress USA: 0x8026ff24
 * @ghidraAddress JPN: 0x802719a4
 * @ghidraAddress EUR: 0x80271ef4
 */
extern u32 g_adwTrackMusicRom[12];

extern u8 _startLightsSegmentRomStart[];
extern u8 _startLightsSegmentRomEnd[];

/**
 * The model of the start lights.
 *
 * @ghidraAddress USA: 0x8026ff54
 * @ghidraAddress JPN: 0x802719d4
 * @ghidraAddress EUR: 0x80271f24
 */
extern Model *g_pStartLightsModel;

/**
 * How the start lights animate: 0 not at all, -1 held in their first pose, and 1 running.
 *
 * @ghidraAddress USA: 0x8026ff58
 * @ghidraAddress JPN: 0x802719d8
 * @ghidraAddress EUR: 0x80271f28
 */
extern s32 g_dwStartLightsAnimation;

/**
 * Whether the start lights are drawn.
 *
 * @ghidraAddress USA: 0x8026ff5c
 * @ghidraAddress JPN: 0x802719dc
 * @ghidraAddress EUR: 0x80271f2c
 */
extern s32 g_bStartLightsVisible;

/**
 * How far the start lights rise out of view once the race starts.
 *
 * @ghidraAddress USA: 0x8026ff60
 * @ghidraAddress JPN: 0x802719e0
 * @ghidraAddress EUR: 0x80271f30
 */
extern f32 g_fStartLightsDrop;

/**
 * The placed model of the airplane, or 0 when the track has none or it landed, and the model a
 * vehicle sends it off from on the lap g_dwAirplaneTriggerLap.
 *
 * @ghidraAddress USA: 0x8026ff64
 * @ghidraAddress JPN: 0x802719e4
 * @ghidraAddress EUR: 0x80271f34
 */
extern s32 g_dwAirplaneInstance;

/**
 * @ghidraAddress USA: 0x8026ff68
 * @ghidraAddress JPN: 0x802719e8
 * @ghidraAddress EUR: 0x80271f38
 */
extern s32 g_dwAirplaneTrigger;

/**
 * @ghidraAddress USA: 0x8026ff6c
 * @ghidraAddress JPN: 0x802719ec
 * @ghidraAddress EUR: 0x80271f3c
 */
extern s32 g_dwAirplaneTriggerLap;

/**
 * Whether the airplane flies.
 *
 * @ghidraAddress USA: 0x8026ff70
 * @ghidraAddress JPN: 0x802719f0
 * @ghidraAddress EUR: 0x80271f40
 */
extern s32 g_bAirplaneFlying;

/**
 * The node of the path the airplane flies towards, and the nodes of the path.
 *
 * @ghidraAddress USA: 0x8026ff74
 * @ghidraAddress JPN: 0x802719f4
 * @ghidraAddress EUR: 0x80271f44
 */
extern s32 g_dwAirplaneNode;

/**
 * @ghidraAddress USA: 0x8026ff78
 * @ghidraAddress JPN: 0x802719f8
 * @ghidraAddress EUR: 0x80271f48
 */
extern s32 g_dwAirplaneNodeCount;

/**
 * The scale of the model of the airplane.
 *
 * @ghidraAddress USA: 0x8026ff7c
 * @ghidraAddress JPN: 0x802719fc
 * @ghidraAddress EUR: 0x80271f4c
 */
extern f32 g_fAirplaneScale;

/**
 * The left and right edges of the path of the airplane.
 *
 * @ghidraAddress USA: 0x8026ff80
 * @ghidraAddress JPN: 0x80271a00
 * @ghidraAddress EUR: 0x80271f50
 */
extern Vec3f *g_pAirplanePathLeft;

/**
 * @ghidraAddress USA: 0x8026ff84
 * @ghidraAddress JPN: 0x80271a04
 * @ghidraAddress EUR: 0x80271f54
 */
extern Vec3f *g_pAirplanePathRight;

/**
 * How far the airplane flew along the leg of the path to the current node, and its length.
 *
 * @ghidraAddress USA: 0x8026ff88
 * @ghidraAddress JPN: 0x80271a08
 * @ghidraAddress EUR: 0x80271f58
 */
extern f32 g_fAirplaneDistance;

/**
 * @ghidraAddress USA: 0x8026ff8c
 * @ghidraAddress JPN: 0x80271a0c
 * @ghidraAddress EUR: 0x80271f5c
 */
extern f32 g_fAirplaneSegmentLength;

/**
 * The directions of the legs of the path before, along and after the one the airplane flies, which
 * it turns between.
 *
 * @ghidraAddress USA: 0x8026ff90
 * @ghidraAddress JPN: 0x80271a10
 * @ghidraAddress EUR: 0x80271f60
 */
extern Vec3f g_vAirplaneDirPrev;

/**
 * @ghidraAddress USA: 0x8026ff9c
 * @ghidraAddress JPN: 0x80271a1c
 * @ghidraAddress EUR: 0x80271f6c
 */
extern Vec3f g_vAirplaneDir;

/**
 * @ghidraAddress USA: 0x8026ffa8
 * @ghidraAddress JPN: 0x80271a28
 * @ghidraAddress EUR: 0x80271f78
 */
extern Vec3f g_vAirplaneDirNext;

/**
 * The captions of the credits, ending with a NULL title.
 *
 * @ghidraAddress USA: 0x8026ffb4
 * @ghidraAddress JPN: 0x80271a34
 * @ghidraAddress EUR: 0x80271f84
 */
extern RaceCaption g_aCreditCaptions[];

/**
 * The scrolling credits, ending with a NULL text.
 *
 * @ghidraAddress USA: 0x80270194
 * @ghidraAddress JPN: 0x80271c14
 * @ghidraAddress EUR: 0x80272164
 */
extern CreditLine g_aCreditScrollLines[];

/**
 * Whether leaving the race starts its instant replay.
 *
 * @ghidraAddress USA: 0x8027078c
 * @ghidraAddress JPN: 0x8027220c
 * @ghidraAddress EUR: 0x8027275c
 */
extern s32 g_bReplayAfterRace;

/**
 * Whether the results of the last race are kept.
 *
 * @ghidraAddress USA: 0x80270790
 * @ghidraAddress JPN: 0x80272210
 * @ghidraAddress EUR: 0x80272760
 */
extern s32 g_dwSavedResultsValid;

/**
 * The times left in the countdown at which the start lights change, in seconds.
 *
 * @ghidraAddress USA: 0x80270794
 * @ghidraAddress JPN: 0x80272214
 * @ghidraAddress EUR: 0x80272764
 */
extern f32 g_afStartLightThresholds[START_LIGHT_STEPS];

/**
 * Whether a time attack left something to save, so that its results open the save menu.
 *
 * @ghidraAddress USA: 0x802707a8
 * @ghidraAddress JPN: 0x80272228
 * @ghidraAddress EUR: 0x80272778
 */
extern s32 g_dwSaveAvailable;

/**
 * The points of each place of a race of the championship.
 *
 * @ghidraAddress USA: 0x802707ac
 * @ghidraAddress JPN: 0x8027222c
 * @ghidraAddress EUR: 0x8027277c
 */
extern u8 g_abRacePositionPoints[];

/**
 * @ghidraAddress USA: 0x802707c0
 * @ghidraAddress JPN: 0x80272240
 * @ghidraAddress EUR: 0x80272790
 */
extern s32 g_dwSeasonResultMsgLineCount;

/**
 * The waterfalls of the track, as placed models.
 *
 * @ghidraAddress USA: 0x802707c4
 * @ghidraAddress JPN: 0x80272244
 * @ghidraAddress EUR: 0x80272794
 */
extern s32 g_dwWaterfallCount;

/**
 * Whether the race is set up.
 *
 * @ghidraAddress USA: 0x802707c8
 * @ghidraAddress JPN: 0x8027224c
 * @ghidraAddress EUR: 0x8027279c
 */
extern s32 g_dwRaceInitialized;

/**
 * Whether the screen is leaving the race.
 *
 * @ghidraAddress USA: 0x802707cc
 * @ghidraAddress JPN: 0x80272250
 * @ghidraAddress EUR: 0x802727a0
 */
extern s32 g_dwRaceExitState;

/** The phases of a race in their order. @ghidraAddress USA: 0x802707d0 */
extern RacePhaseStep g_aRacePhaseTable[RACE_PHASES];

/**
 * The vehicles that finished the race so far.
 *
 * @ghidraAddress USA: 0x8028b300
 * @ghidraAddress JPN: 0x8028cfe0
 * @ghidraAddress EUR: 0x8028d5b0
 */
extern s32 g_dwRaceFinishOrderCounter;

/**
 * The laps of the race.
 *
 * @ghidraAddress USA: 0x8028b304
 * @ghidraAddress JPN: 0x8028cfe4
 * @ghidraAddress EUR: 0x8028d5b4
 */
extern s32 g_dwTotalLaps;

/**
 * The time left in the phase of the race, in seconds.
 *
 * @ghidraAddress USA: 0x802f7ef0
 * @ghidraAddress JPN: 0x802f8624
 * @ghidraAddress EUR: 0x802f9be4
 */
extern f32 g_fRacePhaseTimer;

/**
 * The step of the countdown of the start lights.
 *
 * @ghidraAddress USA: 0x802f7ef4
 * @ghidraAddress JPN: 0x802f8628
 * @ghidraAddress EUR: 0x802f9be8
 */
extern s32 g_dwStartLightsStep;

/**
 * The index of the phase in g_aRacePhaseTable.
 *
 * @ghidraAddress USA: 0x802f7ef8
 * @ghidraAddress EUR: 0x802f9bec
 */
extern s32 g_dwRacePhaseIndex;

/**
 * Set once the loop of the race screen ends, and cleared when a race starts.
 *
 * @ghidraAddress USA: 0x802a4c04
 * @ghidraAddress JPN: 0x802a6e2c
 * @ghidraAddress EUR: 0x802a738c
 */
extern s16 g_wRaceScreenLoopExited;

/**
 * The results of the local players the results screen shows: the place, the time, the best lap and
 * its time, the laps and the time of each.
 *
 * @ghidraAddress USA: 0x80315d60
 * @ghidraAddress JPN: 0x80316490
 * @ghidraAddress EUR: 0x80317a50
 */
extern s32 g_adwSavedResultPositions[SAVED_RESULTS];

/**
 * @ghidraAddress USA: 0x80315d68
 * @ghidraAddress JPN: 0x80316498
 * @ghidraAddress EUR: 0x80317a58
 */
extern f32 g_afSavedResultTimes[SAVED_RESULTS];

/**
 * @ghidraAddress USA: 0x80315d70
 * @ghidraAddress JPN: 0x803164a0
 * @ghidraAddress EUR: 0x80317a60
 */
extern f32 g_afSavedResultBestLapTimes[SAVED_RESULTS];

/**
 * @ghidraAddress USA: 0x80315d78
 * @ghidraAddress JPN: 0x803164a8
 * @ghidraAddress EUR: 0x80317a68
 */
extern f32 g_aafSavedResultLapTimes[SAVED_RESULTS][5];

/**
 * @ghidraAddress USA: 0x80315da0
 * @ghidraAddress JPN: 0x803164d0
 * @ghidraAddress EUR: 0x80317a90
 */
extern s32 g_adwSavedResultBestLaps[SAVED_RESULTS];

/**
 * @ghidraAddress USA: 0x80315da8
 * @ghidraAddress JPN: 0x803164d8
 * @ghidraAddress EUR: 0x80317a98
 */
extern s32 g_adwSavedResultLaps[SAVED_RESULTS];

/**
 * The lines of the message of the result of a season.
 *
 * @ghidraAddress USA: 0x80315db0
 * @ghidraAddress JPN: 0x803164e0
 * @ghidraAddress EUR: 0x80317aa0
 */
extern const char *g_apszSeasonResultMsgLines[];

/**
 * The text of a line of the pause menu.
 *
 * @ghidraAddress USA: 0x80315dd8
 * @ghidraAddress JPN: 0x80316508
 * @ghidraAddress EUR: 0x80317ac8
 */
extern char g_szFormatBuffer[0x20];

/**
 * @ghidraAddress USA: 0x80315df8
 * @ghidraAddress EUR: 0x80314f38
 */
extern s32 g_adwWaterfallInstances[16];

/**
 * The frames the screen is still cleared, after the views changed or the pause ended.
 *
 * @ghidraAddress USA: 0x80315e38
 * @ghidraAddress EUR: 0x80314f78
 */
extern s32 g_dwScreenClearFrames;

/**
 * The time of the caption shown and its index.
 *
 * @ghidraAddress USA: 0x80315e68
 * @ghidraAddress JPN: 0x80316598
 * @ghidraAddress EUR: 0x80317b58
 */
extern f32 g_fRaceCaptionTimer;

/**
 * @ghidraAddress USA: 0x80315e6c
 * @ghidraAddress JPN: 0x8031659c
 * @ghidraAddress EUR: 0x80317b5c
 */
extern s32 g_dwRaceCaptionIndex;

/**
 * Whether a button ended the race the computer drives, which then returns to the main menu rather
 * than the splash screens.
 *
 * @ghidraAddress USA: 0x80315e70
 * @ghidraAddress EUR: 0x80314fb0
 */
extern s32 g_bDemoInterrupted;

/**
 * The views of the last frame.
 *
 * @ghidraAddress USA: 0x80315e74
 * @ghidraAddress EUR: 0x80314fb4
 */
extern s32 g_dwPrevViewportCount;

/**
 * The phase of the race, by the RACE_PHASE_ values.
 *
 * @ghidraAddress USA: 0x80315e78
 * @ghidraAddress EUR: 0x80314fb8
 */
extern s32 g_dwRacePhase;

/**
 * The names of the places in a race: "1st" and on.
 *
 * @ghidraAddress USA: 0x80271fe0
 * @ghidraAddress JPN: 0x80273a80
 * @ghidraAddress EUR: 0x80273fc0
 */
extern const char *g_apszPlaceNames[];

/**
 * Decompress the music of the selected track and start it.
 *
 * @ghidraAddress USA: 0x80200320
 * @ghidraAddress JPN: 0x80200310
 * @ghidraAddress EUR: 0x80200350
 */
void LoadAndPlayTrackBgm(void);

/**
 * Advance the animations of the start lights, holding them in place while they hold their first
 * pose.
 *
 * @ghidraAddress USA: 0x8020037c
 * @ghidraAddress JPN: 0x8020036c
 * @ghidraAddress EUR: 0x802003ac
 */
void AnimateStartLights(void);

/**
 * Move the animations of the race between frames, while the race is not paused.
 *
 * @ghidraAddress USA: 0x802003e4
 * @ghidraAddress JPN: 0x802003d4
 * @ghidraAddress EUR: 0x80200414
 */
void UpdateRaceAnimations(void);

/**
 * Draw the start lights over the start of the track.
 *
 * @ghidraAddress USA: 0x8020046c
 * @ghidraAddress JPN: 0x8020045c
 * @ghidraAddress EUR: 0x8020049c
 */
void RenderStartLights(void);

/**
 * Place the lines of the scrolling credits below the screen.
 *
 * @ghidraAddress USA: 0x802006c8
 * @ghidraAddress JPN: 0x802006b8
 * @ghidraAddress EUR: 0x802006f8
 */
void CalculateScrollPositions(void);

/**
 * Move the scrolling credits up a pixel.
 *
 * @ghidraAddress USA: 0x8020072c
 * @ghidraAddress JPN: 0x8020071c
 * @ghidraAddress EUR: 0x8020075c
 */
void ScrollTextUp(void);

/**
 * Draw the lines of the scrolling credits that are on the screen.
 *
 * @ghidraAddress USA: 0x80200764
 * @ghidraAddress JPN: 0x80200754
 * @ghidraAddress EUR: 0x80200794
 */
void DrawScrollingText(void);

/**
 * Run one frame of the screen of a race: set the race up, step its phases, move and draw the
 * vehicles in each view, and handle the pause menu, the instant replay and leaving the race.
 *
 * @ghidraAddress USA: 0x8020082c
 * @ghidraAddress EUR: 0x8020085c
 * @ghidraAddress JPN: 0x8020081c
 */
void ProcessRacingScreen(void);

/**
 * Keep the results of the local players for the results screen.
 *
 * @ghidraAddress USA: 0x80205fdc
 * @ghidraAddress JPN: 0x80206270
 * @ghidraAddress EUR: 0x8020619c
 */
void SaveRaceResultsToSlots(void);

/**
 * Give the kept results back to the vehicles, scoring them in the championship first.
 *
 * @ghidraAddress USA: 0x802060dc
 * @ghidraAddress JPN: 0x80206370
 * @ghidraAddress EUR: 0x8020629c
 */
void RestoreRaceResultsFromSlots(void);

/**
 * Choose the track of the next race of the championship.
 *
 * @ghidraAddress USA: 0x80206304
 * @ghidraAddress JPN: 0x80206598
 * @ghidraAddress EUR: 0x802064c4
 */
void SetTrackFromSeason(void);

/**
 * Score the season of the championship: advance to the next season when its points allow, unlock
 * its bonuses, and write the lines of the message of its result.
 *
 * @ghidraAddress USA: 0x802063a4
 * @ghidraAddress JPN: 0x80206638
 * @ghidraAddress EUR: 0x80206564
 */
void ProcessSeasonResults(void);

#endif
