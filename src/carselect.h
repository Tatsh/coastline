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
    CAR_SELECT_CAR,          /*!< The choice of the car. */
    CAR_SELECT_HANDLING,     /*!< The choice of the handling. */
    CAR_SELECT_TRANSMISSION, /*!< The choice of the transmission. */
    CAR_SELECT_TIRES,        /*!< The choice of the tyres. */
    CAR_SELECT_SUSPENSION,   /*!< The choice of the suspension. */
    CAR_SELECT_DECALS,       /*!< The choice of the decals, by the DECALS_ values. */
    CAR_SELECT_READY,        /*!< The player is ready to race. */
};

/** The pages of the results: the race of each player, the season, and its message. */
enum {
    RESULTS_PAGE_PLAYER_1, /*!< The race of the first player. */
    RESULTS_PAGE_PLAYER_2, /*!< The race of the second player. */
    RESULTS_PAGE_SEASON,   /*!< The standings of the season, skipped outside a championship. */
    RESULTS_PAGE_MESSAGE,  /*!< The message of the season. */
    RESULTS_PAGE_END,      /*!< Past the last page, which exits the screen. */
};

/** The decals a player may choose. */
enum {
    DECALS_DEFAULT, /*!< The default decals of the car. */
    DECALS_LOAD,    /*!< Decals loaded from the Controller Pak. */
    DECALS_CUSTOM,  /*!< The decals painted in the paint shop. */
};

/**
 * Whether the screen shows the results of a race instead of choosing a car.
 *
 * @ghidraAddress USA: 0x80272074
 * @ghidraAddress JPN: 0x80273b00
 * @ghidraAddress EUR: 0x80274054
 */
extern s32 g_dwIsReviewMode;

/**
 * Whether the screen has been set up since it was last left.
 *
 * @ghidraAddress USA: 0x80272238
 * @ghidraAddress JPN: 0x80273cc8
 * @ghidraAddress EUR: 0x80274218
 */
extern s32 g_bCarSelectInitialized;

/**
 * The time the screen started, in milliseconds.
 *
 * @ghidraAddress USA: 0x80316250
 * @ghidraAddress JPN: 0x80316b68
 * @ghidraAddress EUR: 0x80315390
 */
extern s32 g_dwCarSelectStartTime;

/**
 * The slot of g_aVehicleModelSlots of the car each player sees.
 *
 * @ghidraAddress USA: 0x80316258
 * @ghidraAddress JPN: 0x80316b70
 * @ghidraAddress EUR: 0x80317f38
 */
extern s32 g_adwCarSelectModelSlots[2];

/**
 * How far the cars of each player slide on their stage, from 1 or -1 to 0 at rest.
 *
 * @ghidraAddress USA: 0x80316260
 * @ghidraAddress JPN: 0x80316b78
 * @ghidraAddress EUR: 0x80317f40
 */
extern f32 g_afCarSelectSlide[2];

/**
 * The turn of the stage of each player, in radians.
 *
 * @ghidraAddress USA: 0x80316268
 * @ghidraAddress JPN: 0x80316b80
 * @ghidraAddress EUR: 0x80317f48
 */
extern f32 g_afCarSelectAngle[2];

/**
 * The options each player chooses from.
 *
 * @ghidraAddress USA: 0x80316270
 * @ghidraAddress JPN: 0x80316b88
 * @ghidraAddress EUR: 0x803153b0
 */
extern MenuItem **g_aapCarSelectOptions[2];

/**
 * The option under the cursor of each player.
 *
 * @ghidraAddress USA: 0x80316278
 * @ghidraAddress JPN: 0x80316b60
 * @ghidraAddress EUR: 0x803153b8
 */
extern s32 g_anCarSelectCursor[2];

/**
 * The option each player's cursor left, which slides out.
 *
 * @ghidraAddress USA: 0x80316280
 * @ghidraAddress JPN: 0x80316b98
 * @ghidraAddress EUR: 0x803153c0
 */
extern s32 g_anCarSelectPrevCursor[2];

/**
 * The number of options each player chooses from.
 *
 * @ghidraAddress USA: 0x80316288
 * @ghidraAddress JPN: 0x80316ba0
 * @ghidraAddress EUR: 0x803153c8
 */
extern s32 g_anCarSelectOptionCount[2];

/**
 * The name of the option each player chooses.
 *
 * @ghidraAddress USA: 0x80316290
 * @ghidraAddress JPN: 0x80316ba8
 * @ghidraAddress EUR: 0x80317f70
 */
extern const char *g_apszCarSelectCategory[2];

/**
 * Whether every player chose, rather than one leaving.
 *
 * @ghidraAddress USA: 0x80316298
 * @ghidraAddress JPN: 0x80316bb0
 * @ghidraAddress EUR: 0x80317f78
 */
extern s32 g_bCarSelectDone;

/**
 * The sounds of moving the cursor and of choosing, and their lengths.
 *
 * @ghidraAddress USA: 0x8031629c
 * @ghidraAddress JPN: 0x80316bb4
 * @ghidraAddress EUR: 0x803153dc
 */
extern s8 *g_pScrollSoundBuf;

/**
 * @ghidraAddress USA: 0x803162a0
 * @ghidraAddress EUR: 0x803153e0
 */
extern u32 g_dwScrollSoundSize;

/**
 * @ghidraAddress USA: 0x803162a4
 * @ghidraAddress EUR: 0x803153e4
 */
extern s8 *g_pSelectSoundBuf;

/**
 * @ghidraAddress USA: 0x803162a8
 * @ghidraAddress JPN: 0x80316bc0
 * @ghidraAddress EUR: 0x80317f88
 */
extern u32 g_dwSelectSoundSize;

/**
 * The frames the title is still drawn once the screen has faded in. It is set to 2 each frame.
 *
 * @ghidraAddress USA: 0x803162ac
 * @ghidraAddress JPN: 0x80316bc4
 * @ghidraAddress EUR: 0x803153ec
 */
extern s32 g_dwTitleRedrawFrames;

/**
 * The step of each player, by the CAR_SELECT_ values.
 *
 * @ghidraAddress USA: 0x803162b0
 * @ghidraAddress JPN: 0x80316bc8
 * @ghidraAddress EUR: 0x80317f90
 */
extern s32 g_adwCarSelectState[2];

/**
 * The dialog of the controller pak that loads decals: whether it is open, for which player, its
 * result, and whether it is done.
 *
 * @ghidraAddress USA: 0x803162b8
 * @ghidraAddress JPN: 0x80316bd0
 * @ghidraAddress EUR: 0x80317f98
 */
extern s32 g_dwPakDialogActive;

/**
 * @ghidraAddress USA: 0x803162bc
 * @ghidraAddress JPN: 0x80316bd4
 * @ghidraAddress EUR: 0x80317f9c
 */
extern s32 g_dwPakDialogPlayerIdx;

/**
 * @ghidraAddress USA: 0x803162c0
 * @ghidraAddress JPN: 0x80316bd8
 * @ghidraAddress EUR: 0x80317fa0
 */
extern s32 g_dwPakDialogResult;

/**
 * @ghidraAddress USA: 0x803162c4
 * @ghidraAddress JPN: 0x80316bdc
 * @ghidraAddress EUR: 0x80317fa4
 */
extern u8 g_bPakDialogDone;

/**
 * The championship as the results show it: its season, round and flags, the track and the weather
 * of the race.
 *
 * @ghidraAddress USA: 0x803162fc
 * @ghidraAddress JPN: 0x80316c14
 * @ghidraAddress EUR: 0x80317fdc
 */
extern s32 g_dwResultsSeason;

/**
 * @ghidraAddress USA: 0x80316300
 * @ghidraAddress JPN: 0x80316c18
 * @ghidraAddress EUR: 0x80315440
 */
extern s32 g_dwResultsRound;

/**
 * @ghidraAddress USA: 0x80316304
 * @ghidraAddress JPN: 0x80316c1c
 * @ghidraAddress EUR: 0x80317fe4
 */
extern s32 g_dwResultsChampionshipFlags;

/**
 * @ghidraAddress USA: 0x80316308
 * @ghidraAddress JPN: 0x8031689c
 * @ghidraAddress EUR: 0x80317c6c
 */
extern s32 g_dwResultsTrack;

/**
 * @ghidraAddress USA: 0x8031630c
 * @ghidraAddress JPN: 0x80316c24
 * @ghidraAddress EUR: 0x8031544c
 */
extern s32 g_dwCarSelectWeather;

/**
 * Whether the season results were processed on this screen.
 *
 * @ghidraAddress USA: 0x80316310
 * @ghidraAddress JPN: 0x80316c28
 * @ghidraAddress EUR: 0x80317ff0
 */
extern s32 g_bSeasonResultsShown;

/**
 * The page of the results, by the RESULTS_PAGE_ values.
 *
 * @ghidraAddress USA: 0x80316314
 * @ghidraAddress JPN: 0x80316c2c
 * @ghidraAddress EUR: 0x80317ff4
 */
extern s32 g_dwResultsPage;

/**
 * The text of a line of the results.
 *
 * @ghidraAddress USA: 0x80316318
 * @ghidraAddress JPN: 0x80316c30
 * @ghidraAddress EUR: 0x80317ff8
 */
extern char g_szResultsBuf[0x58];

/**
 * The options of the cars, each ending with NULL, and their numbers.
 *
 * @ghidraAddress USA: 0x802720b4
 * @ghidraAddress JPN: 0x80273b40
 * @ghidraAddress EUR: 0x80274094
 */
extern MenuItem *g_apHandlingOptions[];

/**
 * @ghidraAddress USA: 0x802720c4
 * @ghidraAddress JPN: 0x80273b50
 * @ghidraAddress EUR: 0x802740a4
 */
extern s32 g_dwHandlingOptionCount;

/**
 * @ghidraAddress USA: 0x802720f0
 * @ghidraAddress JPN: 0x80273b7c
 * @ghidraAddress EUR: 0x802740d0
 */
extern MenuItem *g_apTransmissionOptions[];

/**
 * @ghidraAddress USA: 0x802720fc
 * @ghidraAddress JPN: 0x80273b88
 * @ghidraAddress EUR: 0x802740dc
 */
extern s32 g_dwTransmissionOptionCount;

/**
 * @ghidraAddress USA: 0x8027213c
 * @ghidraAddress JPN: 0x80273bc8
 * @ghidraAddress EUR: 0x8027411c
 */
extern MenuItem *g_apTiresOptions[];

/**
 * @ghidraAddress USA: 0x8027214c
 * @ghidraAddress JPN: 0x80273bd8
 * @ghidraAddress EUR: 0x8027412c
 */
extern s32 g_dwTiresOptionCount;

/**
 * @ghidraAddress USA: 0x8027218c
 * @ghidraAddress JPN: 0x80273c18
 * @ghidraAddress EUR: 0x8027416c
 */
extern MenuItem *g_apSuspensionOptions[];

/**
 * @ghidraAddress USA: 0x8027219c
 * @ghidraAddress JPN: 0x80273c28
 * @ghidraAddress EUR: 0x8027417c
 */
extern s32 g_dwSuspensionOptionCount;

/**
 * The decals, by the DECALS_ values.
 *
 * @ghidraAddress USA: 0x802721dc
 * @ghidraAddress EUR: 0x802741bc
 */
extern MenuItem *g_apDecalOptions[];

/**
 * @ghidraAddress USA: 0x802721ec
 * @ghidraAddress JPN: 0x80273c78
 * @ghidraAddress EUR: 0x802741cc
 */
extern s32 g_dwDecalListCount;

/**
 * The names of the layouts of the engines and of the driven wheels.
 *
 * @ghidraAddress USA: 0x80272030
 * @ghidraAddress JPN: 0x80273ad0
 * @ghidraAddress EUR: 0x80274010
 */
extern const char *g_apszEngineLayouts[];

/**
 * @ghidraAddress USA: 0x8027203c
 * @ghidraAddress JPN: 0x80273adc
 * @ghidraAddress EUR: 0x8027401c
 */
extern const char *g_apszDriveTypes[];

/**
 * The lights of the models on the stages.
 *
 * @ghidraAddress USA: 0x802721f0
 * @ghidraAddress JPN: 0x80273c80
 * @ghidraAddress EUR: 0x802741d0
 */
extern Ambient g_CameraViewAmbientLight;

/**
 * @ghidraAddress USA: 0x802721f8
 * @ghidraAddress JPN: 0x80273c88
 * @ghidraAddress EUR: 0x802741d8
 */
extern Light g_CameraViewLight1;

/**
 * @ghidraAddress USA: 0x80272208
 * @ghidraAddress JPN: 0x80273c98
 * @ghidraAddress EUR: 0x802741e8
 */
extern Light g_CameraViewLight2;

/**
 * @ghidraAddress USA: 0x80272218
 * @ghidraAddress JPN: 0x80273ca8
 * @ghidraAddress EUR: 0x802741f8
 */
extern Light g_CameraViewLight3;

/**
 * @ghidraAddress USA: 0x80272228
 * @ghidraAddress JPN: 0x80273cb8
 * @ghidraAddress EUR: 0x80274208
 */
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
 * @ghidraAddress JPN: 0x8020be40
 * @ghidraAddress EUR: 0x8020c240
 */
void DrawProgressBar(s32 x, s32 y, s32 width, s32 height, f32 fraction);

/**
 * Whether a car may be chosen: unlocked, on the cartridge, and in the paint shop one of the first
 * nine.
 *
 * @param car The index of the car in g_aCarModels.
 * @return Whether it may be chosen.
 * @ghidraAddress USA: 0x8020c66c
 * @ghidraAddress JPN: 0x8020c04c (stub)
 * @ghidraAddress EUR: 0x8020c44c
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
 * @ghidraAddress JPN: 0x8020c0bc (stub)
 * @ghidraAddress EUR: 0x8020c4b0
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
 * @ghidraAddress JPN: 0x8020c91c
 * @ghidraAddress EUR: 0x8020cd24
 */
void FormatLapTime(char *out, f32 seconds);

/**
 * Run one frame of the car selection screen, or of the results when g_dwIsReviewMode is set.
 *
 * @ghidraAddress USA: 0x8020d004
 * @ghidraAddress JPN: 0x8020c9dc (stub)
 * @ghidraAddress EUR: 0x8020cde4 (stub)
 */
void ProcessCarSelectScreen(void);

/**
 * Run one frame of the results of a race, on the car selection screen.
 *
 * @ghidraAddress USA: 0x80210f4c
 * @ghidraAddress JPN: 0x80210ca0
 * @ghidraAddress EUR: 0x80210da8
 */
void ProcessRaceResultsScreen(void);

#endif
