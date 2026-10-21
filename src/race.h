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

/** Whether the race is paused. @ghidraAddress USA: 0x8026ff10 */
extern u32 g_dwRacePauseState;

/** The item of the pause menu, by the PAUSE_ values. @ghidraAddress USA: 0x8026ff14 */
extern s32 g_dwPauseMenuSelection;

/**
 * The cartridge offsets of the music of each track, the mirrored ones after the others, then that
 * of the race after a season.
 *
 * @ghidraAddress USA: 0x8026ff24
 */
extern u32 g_adwTrackMusicRom[12];

/** The model of the start lights. @ghidraAddress USA: 0x8026ff54 */
extern Model *g_pStartLightsModel;

/**
 * How the start lights animate: 0 not at all, -1 held in their first pose, and 1 running.
 *
 * @ghidraAddress USA: 0x8026ff58
 */
extern s32 g_dwStartLightsAnimation;

/** Whether the start lights are drawn. @ghidraAddress USA: 0x8026ff5c */
extern s32 g_bStartLightsVisible;

/**
 * How far the start lights rise out of view once the race starts.
 *
 * @ghidraAddress USA: 0x8026ff60
 */
extern f32 g_fStartLightsDrop;

/**
 * The placed model of the airplane, or 0 when the track has none or it landed, and the model a
 * vehicle sends it off from on the lap g_dwAirplaneTriggerLap.
 *
 * @ghidraAddress USA: 0x8026ff64
 */
extern s32 g_dwAirplaneInstance;

/** @ghidraAddress USA: 0x8026ff68 */
extern s32 g_dwAirplaneTrigger;

/** @ghidraAddress USA: 0x8026ff6c */
extern s32 g_dwAirplaneTriggerLap;

/** Whether the airplane flies. @ghidraAddress USA: 0x8026ff70 */
extern s32 g_bAirplaneFlying;

/**
 * The node of the path the airplane flies towards, and the nodes of the path.
 *
 * @ghidraAddress USA: 0x8026ff74
 */
extern s32 g_dwAirplaneNode;

/** @ghidraAddress USA: 0x8026ff78 */
extern s32 g_dwAirplaneNodeCount;

/** The scale of the model of the airplane. @ghidraAddress USA: 0x8026ff7c */
extern f32 g_fAirplaneScale;

/** The left and right edges of the path of the airplane. @ghidraAddress USA: 0x8026ff80 */
extern Vec3f *g_pAirplanePathLeft;

/** @ghidraAddress USA: 0x8026ff84 */
extern Vec3f *g_pAirplanePathRight;

/**
 * How far the airplane flew along the leg of the path to the current node, and its length.
 *
 * @ghidraAddress USA: 0x8026ff88
 */
extern f32 g_fAirplaneDistance;

/** @ghidraAddress USA: 0x8026ff8c */
extern f32 g_fAirplaneSegmentLength;

/**
 * The directions of the legs of the path before, along and after the one the airplane flies, which
 * it turns between.
 *
 * @ghidraAddress USA: 0x8026ff90
 */
extern Vec3f g_vAirplaneDirPrev;

/** @ghidraAddress USA: 0x8026ff9c */
extern Vec3f g_vAirplaneDir;

/** @ghidraAddress USA: 0x8026ffa8 */
extern Vec3f g_vAirplaneDirNext;

/** The captions of the credits, ending with a NULL title. @ghidraAddress USA: 0x8026ffb4 */
extern RaceCaption g_aCreditCaptions[];

/** The scrolling credits, ending with a NULL text. @ghidraAddress USA: 0x80270194 */
extern CreditLine g_aCreditScrollLines[];

/** Whether leaving the race starts its instant replay. @ghidraAddress USA: 0x8027078c */
extern s32 g_bReplayAfterRace;

/** Whether the results of the last race are kept. @ghidraAddress USA: 0x80270790 */
extern s32 g_dwSavedResultsValid;

/**
 * The times left in the countdown at which the start lights change, in seconds.
 *
 * @ghidraAddress USA: 0x80270794
 */
extern f32 g_afStartLightThresholds[START_LIGHT_STEPS];

/**
 * Whether a time attack left something to save, so that its results open the save menu.
 *
 * @ghidraAddress USA: 0x802707a8
 */
extern s32 g_dwSaveAvailable;

/** The points of each place of a race of the championship. @ghidraAddress USA: 0x802707ac */
extern u8 g_abRacePositionPoints[];

/** @ghidraAddress USA: 0x802707c0 */
extern s32 g_dwSeasonResultMsgLineCount;

/** The waterfalls of the track, as placed models. @ghidraAddress USA: 0x802707c4 */
extern s32 g_dwWaterfallCount;

/** Whether the race is set up. @ghidraAddress USA: 0x802707c8 */
extern s32 g_dwRaceInitialized;

/** Whether the screen is leaving the race. @ghidraAddress USA: 0x802707cc */
extern s32 g_dwRaceExitState;

/** The phases of a race in their order. @ghidraAddress USA: 0x802707d0 */
extern RacePhaseStep g_aRacePhaseTable[RACE_PHASES];

/** The vehicles that finished the race so far. @ghidraAddress USA: 0x8028b300 */
extern s32 g_dwRaceFinishOrderCounter;

/** The laps of the race. @ghidraAddress USA: 0x8028b304 */
extern s32 g_dwTotalLaps;

/** The time left in the phase of the race, in seconds. @ghidraAddress USA: 0x802f7ef0 */
extern f32 g_fRacePhaseTimer;

/** The step of the countdown of the start lights. @ghidraAddress USA: 0x802f7ef4 */
extern s32 g_dwStartLightsStep;

/** The index of the phase in g_aRacePhaseTable. @ghidraAddress USA: 0x802f7ef8 */
extern s32 g_dwRacePhaseIndex;

/**
 * Set once the loop of the race screen ends, and cleared when a race starts.
 *
 * @ghidraAddress USA: 0x802a4c04
 */
extern s16 g_wRaceScreenLoopExited;

/**
 * The results of the local players the results screen shows: the place, the time, the best lap and
 * its time, the laps and the time of each.
 *
 * @ghidraAddress USA: 0x80315d60
 */
extern s32 g_adwSavedResultPositions[SAVED_RESULTS];

/** @ghidraAddress USA: 0x80315d68 */
extern f32 g_afSavedResultTimes[SAVED_RESULTS];

/** @ghidraAddress USA: 0x80315d70 */
extern f32 g_afSavedResultBestLapTimes[SAVED_RESULTS];

/** @ghidraAddress USA: 0x80315d78 */
extern f32 g_aafSavedResultLapTimes[SAVED_RESULTS][5];

/** @ghidraAddress USA: 0x80315da0 */
extern s32 g_adwSavedResultBestLaps[SAVED_RESULTS];

/** @ghidraAddress USA: 0x80315da8 */
extern s32 g_adwSavedResultLaps[SAVED_RESULTS];

/** The lines of the message of the result of a season. @ghidraAddress USA: 0x80315db0 */
extern const char *g_apszSeasonResultMsgLines[];

/** The text of a line of the pause menu. @ghidraAddress USA: 0x80315dd8 */
extern char g_szFormatBuffer[0x20];

/** @ghidraAddress USA: 0x80315df8 */
extern s32 g_adwWaterfallInstances[16];

/**
 * The frames the screen is still cleared, after the views changed or the pause ended.
 *
 * @ghidraAddress USA: 0x80315e38
 */
extern s32 g_dwScreenClearFrames;

/** The time of the caption shown and its index. @ghidraAddress USA: 0x80315e68 */
extern f32 g_fRaceCaptionTimer;

/** @ghidraAddress USA: 0x80315e6c */
extern s32 g_dwRaceCaptionIndex;

/**
 * Whether a button ended the race the computer drives, which then returns to the main menu rather
 * than the splash screens.
 *
 * @ghidraAddress USA: 0x80315e70
 */
extern s32 g_bDemoInterrupted;

/** The views of the last frame. @ghidraAddress USA: 0x80315e74 */
extern s32 g_dwPrevViewportCount;

/** The phase of the race, by the RACE_PHASE_ values. @ghidraAddress USA: 0x80315e78 */
extern s32 g_dwRacePhase;

/** The names of the places in a race: "1st" and on. @ghidraAddress USA: 0x80271fe0 */
extern const char *g_apszPlaceNames[];

/**
 * Decompress the music of the selected track and start it.
 *
 * @ghidraAddress USA: 0x80200320
 */
void LoadAndPlayTrackBgm(void);

/**
 * Advance the animations of the start lights, holding them in place while they hold their first
 * pose.
 *
 * @ghidraAddress USA: 0x8020037c
 */
void AnimateStartLights(void);

/**
 * Move the animations of the race between frames, while the race is not paused.
 *
 * @ghidraAddress USA: 0x802003e4
 */
void UpdateRaceAnimations(void);

/**
 * Draw the start lights over the start of the track.
 *
 * @ghidraAddress USA: 0x8020046c
 */
void RenderStartLights(void);

/**
 * Place the lines of the scrolling credits below the screen.
 *
 * @ghidraAddress USA: 0x802006c8
 */
void CalculateScrollPositions(void);

/**
 * Move the scrolling credits up a pixel.
 *
 * @ghidraAddress USA: 0x8020072c
 */
void ScrollTextUp(void);

/**
 * Draw the lines of the scrolling credits that are on the screen.
 *
 * @ghidraAddress USA: 0x80200764
 */
void DrawScrollingText(void);

/**
 * Run one frame of the screen of a race: set the race up, step its phases, move and draw the
 * vehicles in each view, and handle the pause menu, the instant replay and leaving the race.
 *
 * @ghidraAddress USA: 0x8020082c
 */
void ProcessRacingScreen(void);

/**
 * Keep the results of the local players for the results screen.
 *
 * @ghidraAddress USA: 0x80205fdc
 */
void SaveRaceResultsToSlots(void);

/**
 * Give the kept results back to the vehicles, scoring them in the championship first.
 *
 * @ghidraAddress USA: 0x802060dc
 */
void RestoreRaceResultsFromSlots(void);

/**
 * Choose the track of the next race of the championship.
 *
 * @ghidraAddress USA: 0x80206304
 */
void SetTrackFromSeason(void);

/**
 * Score the season of the championship: advance to the next season when its points allow, unlock
 * its bonuses, and write the lines of the message of its result.
 *
 * @ghidraAddress USA: 0x802063a4
 */
void ProcessSeasonResults(void);

#endif
