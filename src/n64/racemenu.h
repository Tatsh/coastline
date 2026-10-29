#ifndef COASTLINE_RACEMENU_H
#define COASTLINE_RACEMENU_H

/**
 * @file
 * The menu screens: the generic screen that turns an item at a time, the track menu, the records of
 * a season, and the models the screens draw.
 */

#include <PR/gbi.h>
#include <PR/ultratypes.h>

#include "common/frontendscreens.h"
#include "common/model.h"
#include "common/seasons.h"
#include "common/texrect.h"
#include "common/trackmenuitem.h"
#include "common/vehicle.h"
#include "common/vehicleview.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The flag of an item whose up and down change it in place, as the results 3 and 4. */
#define MENU_ITEM_ADJUSTABLE 0x01

/** The flag of an item chosen at once, without fading the screen out. */
#define MENU_ITEM_IMMEDIATE 0x02

/** The flag of an item drawn as its model turning on the stage. */
#define MENU_ITEM_SPINNING 0x04

/** The flag of an item that cannot be chosen, whose screen explains why. */
#define MENU_ITEM_UNAVAILABLE 0x08

/** The flag of an item only the first player may choose. */
#define MENU_ITEM_FIRST_PLAYER 0x20

/** The flag of an item that cannot be chosen, without a reason shown. */
#define MENU_ITEM_LOCKED 0x40

/* The results of ProcessScreenState(). */
#define MENU_RESULT_CHOSEN 1    /*!< The player chose the item under the cursor. */
#define MENU_RESULT_CANCELLED 2 /*!< The player left the screen. */
#define MENU_RESULT_UP 3        /*!< The player moved up. */
#define MENU_RESULT_DOWN 4      /*!< The player moved down. */

/** A routine that tells whether a menu item may be chosen. */
typedef s32 (*MenuItemFilter)(s32 item);

/** A routine a menu screen calls after drawing, with the selected item. */
typedef void (*MenuPostDrawCallback)(s32 item);

/** A routine a menu screen calls in place of drawing its items. */
typedef void (*MenuPreDrawCallback)(void);

/**
 * @ghidraAddress USA: 0x80271fc8
 * @ghidraAddress EUR: 0x80273fa8
 * @ghidraAddress JPN: 0x80273a68
 */
extern MenuPostDrawCallback g_pfnScreenPostDrawCallback;

/**
 * @ghidraAddress USA: 0x80271fcc
 * @ghidraAddress EUR: 0x80273fac
 * @ghidraAddress JPN: 0x80273a6c
 */
extern MenuPreDrawCallback g_pfnScreenPreDrawCallback;

/**
 * The player whose press chose the last menu item.
 *
 * @ghidraAddress USA: 0x80271fa8
 * @ghidraAddress EUR: 0x80273f88
 * @ghidraAddress JPN: 0x80273a48
 */
extern s32 g_dwMenuChoosingPlayer;

/**
 * The player a screen belongs to, such as the player whose car the paint shop paints or whose
 * controller the controller screen sets up, or -1.
 *
 * @ghidraAddress USA: 0x80271fa0
 * @ghidraAddress EUR: 0x80273f80
 * @ghidraAddress JPN: 0x80273a40
 */
extern s32 g_dwScreenPlayerIdx;

/**
 * A bit for each player whose input the menu screens read, or -1 for all.
 *
 * @ghidraAddress USA: 0x80271fa4
 * @ghidraAddress EUR: 0x80273f84
 * @ghidraAddress JPN: 0x80273a44
 */
extern s32 g_dwMenuInputPlayerMask;

/**
 * The picture of the title of a menu screen, and its rectangle.
 *
 * @ghidraAddress USA: 0x80271fb0
 * @ghidraAddress EUR: 0x80273f90
 * @ghidraAddress JPN: 0x80273a50
 */
extern Texture *g_pMenuTitleImage;

/**
 * @ghidraAddress USA: 0x80271fb4
 * @ghidraAddress EUR: 0x80273f94
 * @ghidraAddress JPN: 0x80273a54
 */
extern s32 g_dwMenuTitleImageX;

/**
 * @ghidraAddress USA: 0x80271fb8
 * @ghidraAddress EUR: 0x80273f98
 * @ghidraAddress JPN: 0x80273a58
 */
extern s32 g_dwMenuTitleImageY;

/**
 * @ghidraAddress USA: 0x80271fbc
 * @ghidraAddress EUR: 0x80273f9c
 * @ghidraAddress JPN: 0x80273a5c
 */
extern s32 g_dwMenuTitleImageWidth;

/**
 * @ghidraAddress USA: 0x80271fc0
 * @ghidraAddress EUR: 0x80273fa0
 * @ghidraAddress JPN: 0x80273a60
 */
extern s32 g_dwMenuTitleImageHeight;

/**
 * The time without input after which a screen chooses for itself, or 0 for never, and the time
 * without input so far.
 *
 * @ghidraAddress USA: 0x80271fc4
 * @ghidraAddress EUR: 0x80273fa4
 * @ghidraAddress JPN: 0x80273a64
 */
extern f32 g_fScreenIdleTimeout;

/**
 * @ghidraAddress USA: 0x80316240
 * @ghidraAddress EUR: 0x80317f20
 * @ghidraAddress JPN: 0x80316b50
 */
extern f32 g_fScreenIdleTime;

/**
 * Whether ProcessScreenState() has set up its screen.
 *
 * @ghidraAddress USA: 0x80271fd4
 * @ghidraAddress EUR: 0x80273fb4
 * @ghidraAddress JPN: 0x80273a74
 */
extern s32 g_bScreenStateInitFlag;

/**
 * The item under the cursor of the screen.
 *
 * @ghidraAddress USA: 0x80316244
 * @ghidraAddress EUR: 0x80317f24
 * @ghidraAddress JPN: 0x80316b54
 */
extern s32 g_nScreenCursor;

/**
 * The item the cursor left.
 *
 * @ghidraAddress USA: 0x80316220
 * @ghidraAddress EUR: 0x80317f00
 * @ghidraAddress JPN: 0x80316b30
 */
extern s32 g_nPrevScreenSelection;

/**
 * The item under the cursor when the screen last ran.
 *
 * @ghidraAddress USA: 0x80271fac
 * @ghidraAddress EUR: 0x80273f8c
 * @ghidraAddress JPN: 0x80273a4c
 */
extern s32 g_nScreenLastActiveIndex;

/**
 * An item to move the cursor to, or -1.
 *
 * @ghidraAddress USA: 0x80271fd0
 * @ghidraAddress EUR: 0x80273fb0
 * @ghidraAddress JPN: 0x80273a70
 */
extern s32 g_nScreenPendingSelection;

/**
 * How far the items have still to slide, from 1 or -1 when the cursor moves to 0, and the angle of
 * the model turning under the cursor.
 *
 * @ghidraAddress USA: 0x80316224
 * @ghidraAddress EUR: 0x80317f04
 * @ghidraAddress JPN: 0x80316b34
 */
extern f32 g_fScreenSlideOffset;

/**
 * @ghidraAddress USA: 0x80316228
 * @ghidraAddress EUR: 0x80317f08
 * @ghidraAddress JPN: 0x80316b38
 */
extern f32 g_fScreenRotationAngle;

/**
 * Whether the screen fades out because an item was chosen.
 *
 * @ghidraAddress USA: 0x8031622c
 * @ghidraAddress EUR: 0x80317f0c
 * @ghidraAddress JPN: 0x80316b3c
 */
extern s32 g_bScreenItemChosen;

/**
 * The sounds of moving the cursor and of choosing, and their lengths.
 *
 * @ghidraAddress USA: 0x80316230
 * @ghidraAddress EUR: 0x80317f10
 * @ghidraAddress JPN: 0x80316b40
 */
extern s8 *g_pScreenScrollSoundBuf;

/**
 * @ghidraAddress USA: 0x80316234
 * @ghidraAddress EUR: 0x80317f14
 * @ghidraAddress JPN: 0x80316b44
 */
extern u32 g_dwScreenScrollSoundSize;

/**
 * @ghidraAddress USA: 0x80316238
 * @ghidraAddress EUR: 0x80317f18
 * @ghidraAddress JPN: 0x80316b48
 */
extern s8 *g_pScreenSelectSoundBuf;

/**
 * @ghidraAddress USA: 0x8031623c
 * @ghidraAddress EUR: 0x80317f1c
 * @ghidraAddress JPN: 0x80316b4c
 */
extern u32 g_dwScreenSelectSoundSize;

/**
 * The arrows of the buttons at the bottom of a screen.
 *
 * @ghidraAddress USA: 0x80271d70
 * @ghidraAddress EUR: 0x80273d50
 * @ghidraAddress JPN: 0x80273810
 */
extern Texture g_ArrowLeftTextureDesc;

/**
 * @ghidraAddress USA: 0x80271d84
 * @ghidraAddress EUR: 0x80273d64
 * @ghidraAddress JPN: 0x80273824
 */
extern Texture g_ArrowRightTextureDesc;

/**
 * The gradient behind a screen.
 *
 * @ghidraAddress USA: 0x80272048
 * @ghidraAddress EUR: 0x80274028
 * @ghidraAddress JPN: 0x80273ae8
 */
extern Texture g_BgTextureDesc;

/**
 * Whether the vertices of the beam of the spotlight over the stage are set; nothing sets it.
 *
 * @ghidraAddress USA: 0x80271dac
 * @ghidraAddress EUR: 0x80273d8c
 * @ghidraAddress JPN: 0x8027384c
 */
extern s32 g_bMenuSpotlightBuilt;

/**
 * The vertices of the beam of the spotlight, its source first.
 *
 * @ghidraAddress USA: 0x80315ff0
 * @ghidraAddress EUR: 0x80317cd0
 * @ghidraAddress JPN: 0x80316900
 */
extern Vtx g_aMenuSpotlightVertices[31];

/**
 * The display list of the beam.
 *
 * @ghidraAddress USA: 0x80271db0
 * @ghidraAddress EUR: 0x80273d90
 * @ghidraAddress JPN: 0x80273850
 */
extern Gfx g_aMenuSpotlightDisplayList[];

/**
 * The display list of the marker drawn under the stage while there is another item to move to, and
 * its matrix.
 *
 * @ghidraAddress USA: 0x80271f28
 * @ghidraAddress EUR: 0x80273f08
 * @ghidraAddress JPN: 0x802739c8
 */
extern Gfx g_aMenuChoiceMarkerDisplayList[];

/**
 * @ghidraAddress USA: 0x803161e0
 * @ghidraAddress EUR: 0x80317ec0
 * @ghidraAddress JPN: 0x80316af0
 */
extern Mtx g_MenuChoiceMarkerMtx;

/**
 * The number of items of the track menu.
 *
 * @ghidraAddress USA: 0x80315f88
 * @ghidraAddress EUR: 0x80317c68
 * @ghidraAddress JPN: 0x80316898
 */
extern s32 g_dwTrackMenuItemCount;

/**
 * The item of the track menu under the cursor.
 *
 * @ghidraAddress USA: 0x80315f8c
 * @ghidraAddress EUR: 0x80317c6c
 * @ghidraAddress JPN: 0x8031689c
 */
extern s32 g_dwTrackMenuSelection;

/**
 * Whether the track menu is of the championship; never read.
 *
 * @ghidraAddress USA: 0x80315f90
 * @ghidraAddress EUR: 0x80317c70
 * @ghidraAddress JPN: 0x803168a0
 */
extern s32 g_bTrackMenuChampionship;

/**
 * Whether the track menu has set up, 1, has still to, 0, or has left for the next screen, -1.
 *
 * @ghidraAddress USA: 0x80271d60
 * @ghidraAddress EUR: 0x80273d40
 * @ghidraAddress JPN: 0x80273800
 */
extern s32 g_dwTrackMenuInitState;

/**
 * Whether the track menu shows the screen of the controller pak.
 *
 * @ghidraAddress USA: 0x80271d58
 * @ghidraAddress EUR: 0x80273d38
 * @ghidraAddress JPN: 0x802737f8
 */
extern s32 g_bTrackMenuPakScreen;

/**
 * Whether the next race of the season is its first after a load.
 *
 * @ghidraAddress USA: 0x80315ee0
 * @ghidraAddress EUR: 0x80317bc0
 * @ghidraAddress JPN: 0x803167f0
 */
extern s32 g_bSeasonFirstRace;

/**
 * Whether the track menu follows the results of a race.
 *
 * @ghidraAddress USA: 0x80270850
 * @ghidraAddress EUR: 0x80272830
 * @ghidraAddress JPN: 0x802722f0
 */
extern s32 g_dwContinueFromResults;

/**
 * The time of the blink of the round under way in the records.
 *
 * @ghidraAddress USA: 0x80271d5c
 * @ghidraAddress EUR: 0x80273d3c
 * @ghidraAddress JPN: 0x802737fc
 */
extern f32 g_fSeasonRecordsBlinkTime;

/**
 * The text of the track menu and of its records.
 *
 * @ghidraAddress USA: 0x80315f98
 * @ghidraAddress EUR: 0x80317c78
 * @ghidraAddress JPN: 0x803168a8
 */
extern char g_szTrackMenuTextBuf[];

/**
 * @ghidraAddress USA: 0x80315ee8
 * @ghidraAddress EUR: 0x80317bc8
 * @ghidraAddress JPN: 0x803167f8
 */
extern char g_szTrackSelectPointsTextBuf[];

/**
 * @ghidraAddress USA: 0x80315f38
 * @ghidraAddress EUR: 0x80317c18
 * @ghidraAddress JPN: 0x80316848
 */
extern char g_szTrackSelectRecordTextBuf[];

/**
 * The loads of the models of the cars the screens show: a slot for each player, and another for
 * each that its next car loads into.
 *
 * @ghidraAddress USA: 0x8031b370
 * @ghidraAddress EUR: 0x8031d870
 * @ghidraAddress JPN: 0x8031c4a0
 */
extern VehicleModelLoad g_aVehicleModelSlots[VEHICLE_MODEL_SLOTS];

/**
 * Whether a track may be chosen: in the championship only the track of the next race, in time
 * attack the loads and resets too, and otherwise an unlocked track.
 *
 * @param track The index of the track, or of an item past the tracks.
 * @return Whether it may be chosen.
 * @ghidraAddress USA: 0x80208900
 * @ghidraAddress EUR: 0x802086e0
 * @ghidraAddress JPN: 0x80208300
 */
s32 IsTrackUnlocked(s32 track);

/**
 * Reset the progress of the championship of the first player when its season, its round or its
 * flags are out of range.
 *
 * @ghidraAddress USA: 0x802089d0
 * @ghidraAddress EUR: 0x802087b0
 * @ghidraAddress JPN: 0x802083d0
 */
void ValidateChampionshipProgress(void);

/**
 * Draw the hints of the track menu, and run the screen of the controller pak while it is shown,
 * starting the championship or time attack once it is done.
 *
 * @param item The item under the cursor.
 * @ghidraAddress USA: 0x80208a58
 * @ghidraAddress EUR: 0x80208838
 * @ghidraAddress JPN: 0x80208458
 */
void TrackMenuPostDraw(s32 item);

/**
 * Draw the records of the season in place of the items of the track menu, and the cars of the
 * season.
 *
 * @ghidraAddress USA: 0x80208cf0
 * @ghidraAddress EUR: 0x80208ad0
 * @ghidraAddress JPN: 0x802086b4
 */
void SeasonRecordsPreDrawCallback(void);

/**
 * Draw the beam of the spotlight over the stage of a screen.
 *
 * @ghidraAddress USA: 0x8020a820
 * @ghidraAddress EUR: 0x8020a600
 * @ghidraAddress JPN: 0x8020a200
 */
void DrawMenuSpotlight(void);

/**
 * Draw the marker under the stage of a screen.
 *
 * @ghidraAddress USA: 0x8020a9d0
 * @ghidraAddress EUR: 0x8020a7b0
 * @ghidraAddress JPN: 0x8020a3b0
 */
void DrawMenuChoiceMarker(void);

/**
 * Draw the buttons of the bottom of a screen: select and go back.
 *
 * @ghidraAddress USA: 0x8020aae0
 * @ghidraAddress EUR: 0x8020a8c0
 * @ghidraAddress JPN: 0x8020a4c0
 */
void DrawMenuFooterButtons(void);

/**
 * Draw the buttons of the bottom of the pause menu: continue and exit.
 *
 * @ghidraAddress USA: 0x8020ac18
 * @ghidraAddress EUR: 0x8020a9f8
 * @ghidraAddress JPN: 0x8020a5f8
 */
void DrawPauseMenuButtons(void);

/**
 * Move the cursor of the screen to an item on its next frame.
 *
 * @param item The item.
 * @ghidraAddress USA: 0x8020ad50
 * @ghidraAddress EUR: 0x8020ab30
 * @ghidraAddress JPN: 0x8020a730
 */
void SetScreenPendingSelection(s32 item);

/**
 * Run one frame of a menu screen over a background gradient.
 *
 * @param title The title of the screen.
 * @param itemCount The number of items.
 * @param items The items of the menu.
 * @param selection The index of the item under the cursor, updated as the cursor moves.
 * @param isSelectable Tells whether an item may be chosen, or NULL to allow every item.
 * @param topRed The red component of the top of the gradient.
 * @param topGreen The green component of the top of the gradient.
 * @param topBlue The blue component of the top of the gradient.
 * @param bottomRed The red component of the bottom of the gradient.
 * @param bottomGreen The green component of the bottom of the gradient.
 * @param bottomBlue The blue component of the bottom of the gradient.
 * @return #MENU_RESULT_CHOSEN once an item is chosen, #MENU_RESULT_CANCELLED once the screen is
 *         left, #MENU_RESULT_UP or #MENU_RESULT_DOWN when an adjustable item is changed, and 0
 *         while the screen runs.
 * @ghidraAddress USA: 0x8020ad5c
 * @ghidraAddress EUR: 0x8020ab3c
 * @ghidraAddress JPN: 0x8020a73c
 */
s32 ProcessScreenState(const char *title,
                       s32 itemCount,
                       MenuItem **items,
                       s32 *selection,
                       MenuItemFilter isSelectable,
                       s32 topRed,
                       s32 topGreen,
                       s32 topBlue,
                       s32 bottomRed,
                       s32 bottomGreen,
                       s32 bottomBlue);

/**
 * Blank the screen, count the items of a menu, and load their models, once for each cartridge
 * range.
 *
 * @param items The items of the menu, ended by a null pointer.
 * @param itemCount Receives the number of items.
 * @ghidraAddress USA: 0x8020c27c
 * @ghidraAddress EUR: 0x8020c05c
 * @ghidraAddress JPN: 0x8020bc5c
 */
void LoadMenuItemModels(MenuItem **items, s32 *itemCount);

/**
 * Reset the heap and the callbacks of the menu screen, and load the models of its items.
 *
 * @param items The items of the menu, ended by a null pointer.
 * @param itemCount Receives the number of items.
 * @ghidraAddress USA: 0x8020c408
 * @ghidraAddress EUR: 0x8020c1e8
 * @ghidraAddress JPN: 0x8020bde8
 */
void ResetAndInitDisplay(MenuItem **items, s32 *itemCount);

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_RACEMENU_H */
