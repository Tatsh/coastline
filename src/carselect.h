#ifndef COASTLINE_CARSELECT_H
#define COASTLINE_CARSELECT_H

/**
 * @file
 * The car selection screen, which also shows the results of a race and of a season, and the views
 * of the cars on it.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "model.h"
#include "racemenu.h"
#include "vehicle.h"

/** The steps of choosing a car, by the CAR_SELECT_ values. */
enum {
    CAR_SELECT_CAR,
    CAR_SELECT_HANDLING,
    CAR_SELECT_TRANSMISSION,
    CAR_SELECT_TIRES,
    CAR_SELECT_SUSPENSION,
    CAR_SELECT_DECALS,
    /** The player is ready to race. */
    CAR_SELECT_READY,
};

/** The pages of the results: the race of each player, the season, and its message. */
enum {
    RESULTS_PAGE_PLAYER_1,
    RESULTS_PAGE_PLAYER_2,
    RESULTS_PAGE_SEASON,
    RESULTS_PAGE_MESSAGE,
    /** Past the last page, which leaves the screen. */
    RESULTS_PAGE_END,
};

/** The decals a player may choose. */
enum {
    DECALS_DEFAULT,
    /** Decals loaded from the controller pak. */
    DECALS_LOAD,
    DECALS_CUSTOM,
};

/**
 * Whether the screen shows the results of a race instead of choosing a car.
 *
 * @ghidraAddress USA: 0x80272074
 */
extern s32 g_dwIsReviewMode;

/** Whether the screen has been set up since it was last left. @ghidraAddress USA: 0x80272238 */
extern s32 g_bCarSelectInitialized;

/** The time the screen started, in milliseconds. @ghidraAddress USA: 0x80316250 */
extern s32 g_dwCarSelectStartTime;

/** The slot of g_aVehicleModelSlots of the car each player sees. @ghidraAddress USA: 0x80316258 */
extern s32 g_adwCarSelectModelSlots[2];

/**
 * How far the cars of each player slide on their stage, from 1 or -1 to 0 at rest.
 *
 * @ghidraAddress USA: 0x80316260
 */
extern f32 g_afCarSelectSlide[2];

/** The turn of the stage of each player, in radians. @ghidraAddress USA: 0x80316268 */
extern f32 g_afCarSelectAngle[2];

/** The options each player chooses from. @ghidraAddress USA: 0x80316270 */
extern MenuItem **g_aapCarSelectOptions[2];

/** The option under the cursor of each player. @ghidraAddress USA: 0x80316278 */
extern s32 g_anCarSelectCursor[2];

/** The option each player's cursor left, which slides out. @ghidraAddress USA: 0x80316280 */
extern s32 g_anCarSelectPrevCursor[2];

/** The number of options each player chooses from. @ghidraAddress USA: 0x80316288 */
extern s32 g_anCarSelectOptionCount[2];

/** The name of the option each player chooses. @ghidraAddress USA: 0x80316290 */
extern const char *g_apszCarSelectCategory[2];

/** Whether every player chose, rather than one leaving. @ghidraAddress USA: 0x80316298 */
extern s32 g_bCarSelectDone;

/**
 * The sounds of moving the cursor and of choosing, and their lengths.
 *
 * @ghidraAddress USA: 0x8031629c
 */
extern s8 *g_pScrollSoundBuf;

/** @ghidraAddress USA: 0x803162a0 */
extern u32 g_dwScrollSoundSize;

/** @ghidraAddress USA: 0x803162a4 */
extern s8 *g_pSelectSoundBuf;

/** @ghidraAddress USA: 0x803162a8 */
extern u32 g_dwSelectSoundSize;

/**
 * The frames the title is still drawn once the screen has faded in. It is set to 2 each frame.
 *
 * @ghidraAddress USA: 0x803162ac
 */
extern s32 g_dwTitleRedrawFrames;

/** The step of each player, by the CAR_SELECT_ values. @ghidraAddress USA: 0x803162b0 */
extern s32 g_adwCarSelectState[2];

/**
 * The dialog of the controller pak that loads decals: whether it is open, for which player, its
 * result, and whether it is done.
 *
 * @ghidraAddress USA: 0x803162b8
 */
extern s32 g_dwPakDialogActive;

/** @ghidraAddress USA: 0x803162bc */
extern s32 g_dwPakDialogPlayerIdx;

/** @ghidraAddress USA: 0x803162c0 */
extern s32 g_dwPakDialogResult;

/** @ghidraAddress USA: 0x803162c4 */
extern u8 g_bPakDialogDone;

/**
 * The championship as the results show it: its season, round and flags, the track and the weather
 * of the race.
 *
 * @ghidraAddress USA: 0x803162fc
 */
extern s32 g_dwResultsSeason;

/** @ghidraAddress USA: 0x80316300 */
extern s32 g_dwResultsRound;

/** @ghidraAddress USA: 0x80316304 */
extern s32 g_dwResultsChampionshipFlags;

/** @ghidraAddress USA: 0x80316308 */
extern s32 g_dwResultsTrack;

/** @ghidraAddress USA: 0x8031630c */
extern s32 g_dwCarSelectWeather;

/** Whether the season results were processed on this screen. @ghidraAddress USA: 0x80316310 */
extern s32 g_bSeasonResultsShown;

/** The page of the results, by the RESULTS_PAGE_ values. @ghidraAddress USA: 0x80316314 */
extern s32 g_dwResultsPage;

/** The text of a line of the results. @ghidraAddress USA: 0x80316318 */
extern char g_szResultsBuf[0x58];

/**
 * The options of the cars, each ending with NULL, and their numbers.
 *
 * @ghidraAddress USA: 0x802720b4
 */
extern MenuItem *g_apHandlingOptions[];

/** @ghidraAddress USA: 0x802720c4 */
extern s32 g_dwHandlingOptionCount;

/** @ghidraAddress USA: 0x802720f0 */
extern MenuItem *g_apTransmissionOptions[];

/** @ghidraAddress USA: 0x802720fc */
extern s32 g_dwTransmissionOptionCount;

/** @ghidraAddress USA: 0x8027213c */
extern MenuItem *g_apTiresOptions[];

/** @ghidraAddress USA: 0x8027214c */
extern s32 g_dwTiresOptionCount;

/** @ghidraAddress USA: 0x8027218c */
extern MenuItem *g_apSuspensionOptions[];

/** @ghidraAddress USA: 0x8027219c */
extern s32 g_dwSuspensionOptionCount;

/** The decals, by the DECALS_ values. @ghidraAddress USA: 0x802721dc */
extern MenuItem *g_apDecalOptions[];

/** @ghidraAddress USA: 0x802721ec */
extern s32 g_dwDecalListCount;

/**
 * The names of the layouts of the engines and of the driven wheels.
 *
 * @ghidraAddress USA: 0x80272030
 */
extern const char *g_apszEngineLayouts[];

/** @ghidraAddress USA: 0x8027203c */
extern const char *g_apszDriveTypes[];

/** The lights of the models on the stages. @ghidraAddress USA: 0x802721f0 */
extern Ambient g_CameraViewAmbientLight;

/** @ghidraAddress USA: 0x802721f8 */
extern Light g_CameraViewLight1;

/** @ghidraAddress USA: 0x80272208 */
extern Light g_CameraViewLight2;

/** @ghidraAddress USA: 0x80272218 */
extern Light g_CameraViewLight3;

/** @ghidraAddress USA: 0x80272228 */
extern Light g_CameraViewLight4;

/**
 * Draw a bar: a black frame, a dark green background, and its filling of a fraction in bright
 * green.
 *
 * @param x The left edge, in pixels.
 * @param y The top edge, in pixels.
 * @param width The width, in pixels.
 * @param height The height, in pixels.
 * @param fraction The fraction filled, from 0 to 1.
 * @ghidraAddress USA: 0x8020c460
 */
void DrawProgressBar(s32 x, s32 y, s32 width, s32 height, f32 fraction);

/**
 * Whether a car may be chosen: unlocked, on the cartridge, and in the paint shop one of the first
 * nine.
 *
 * @param car The index of the car in g_aCarModels.
 * @return Whether it may be chosen.
 * @ghidraAddress USA: 0x8020c66c
 */
s32 IsCarUnlocked(s32 car);

/**
 * Draw a vehicle from its trackside camera in a rectangle of the screen, and a second one beside
 * it, or two models in their place.
 *
 * @param model A model to draw in place of the first car, or NULL.
 * @param secondModel A model to draw in place of the second car, or NULL.
 * @param vehicleIndex The vehicle whose camera looks.
 * @param x The left edge of the rectangle, in pixels.
 * @param y The top edge of the rectangle, in pixels.
 * @param width The width of the rectangle, in pixels.
 * @param height The height of the rectangle, in pixels.
 * @param slide How far the cars slide apart, as a fraction that the square of moves them by.
 * @param hideTachometer Whether the tachometer is left out.
 * @param slot The slot of g_aVehicleModelSlots of the first car.
 * @param secondSlot The slot of g_aVehicleModelSlots of the second car.
 * @param spread The distance of the slide at its full extent.
 * @param angle The direction the camera faces, in radians.
 * @ghidraAddress USA: 0x8020c6d0
 */
void RenderCameraView(Model *model,
                      Model *secondModel,
                      s32 vehicleIndex,
                      s32 x,
                      s32 y,
                      s32 width,
                      s32 height,
                      f32 slide,
                      s32 hideTachometer,
                      s32 slot,
                      s32 secondSlot,
                      f32 spread,
                      f32 angle);

/**
 * Write a time as minutes, seconds and hundredths.
 *
 * @param out The buffer to write to.
 * @param seconds The time, in seconds.
 * @ghidraAddress USA: 0x8020cf44
 */
void FormatLapTime(char *out, f32 seconds);

/**
 * Run one frame of the car selection screen, or of the results when g_dwIsReviewMode is set.
 *
 * @ghidraAddress USA: 0x8020d004
 */
void ProcessCarSelectScreen(void);

/**
 * Run one frame of the results of a race, on the car selection screen.
 *
 * @ghidraAddress USA: 0x80210f4c
 */
void ProcessRaceResultsScreen(void);

#endif
