#ifndef COASTLINE_TRACKMENUITEM_H
#define COASTLINE_TRACKMENUITEM_H

#include "types.h"

struct Model;

/** An item of a menu screen. */
typedef struct {
    const char *name;    /*!< The text of the item. */
    u32 flags;           /*!< The MENU_ITEM_* flags. */
    struct Model *model; /*!< The model of the item, or NULL to show its name. */
    u32 romStart;        /*!< The cartridge offset of the model of the item. */
    u32 romEnd;          /*!< The cartridge offset of the end of the model of the item. */
} MenuItem;

/** The classes of cars an arcade race keeps times for, by the tableIndex of their physics. */
#define ARCADE_CAR_CLASSES 4

/** The weathers an arcade race keeps times for. */
#define ARCADE_WEATHERS 3

/** The times of an arcade race on a track for a class of car and a weather. */
typedef struct {
    f32 gateBonuses[6]; /*!< The seconds each gate adds, gate 0 being the lap line. */
    f32 startTime;      /*!< The time the race starts with, in seconds. */
} ArcadeTimes;

/**
 * A track: its item of the track menu, where its geometry and its two sections of route are on
 * the cartridge, and its arcade times. The end of the geometry is 0 for a track that does not
 * exist.
 */
typedef struct {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    u8 unreconstructed0[0x2c]; /* +0x0 */
#else
    MenuItem item;      /*!< The item of the track menu. */
    u32 trackRom;       /*!< The cartridge offset of the compressed geometry. */
    u32 trackRomEnd;    /*!< The cartridge offset of the end of the geometry, 0 for no track. */
    u32 sectionARom;    /*!< The cartridge offset of the first section of route. */
    u32 sectionARomEnd; /*!< The cartridge offset of the end of the first section of route. */
    u32 sectionBRom;    /*!< The cartridge offset of the second section of route. */
    u32 sectionBRomEnd; /*!< The cartridge offset of the end of the second section of route. */
#endif
    ArcadeTimes arcadeTimes[ARCADE_CAR_CLASSES][ARCADE_WEATHERS]; /*!< The arcade times by car
                                                                     class and by weather less 1. */
} TrackMenuItem;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The items of the track menu: the tracks, then the loads and resets.
 *
 * @ghidraAddress USA: 0x80271d1c
 * @ghidraAddress EUR: 0x80273cfc
 * @ghidraAddress JPN: 0x802737bc
 * @ghidraAddress BRD3D: 0x100bd2a8
 */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
extern TrackMenuItem *g_apTrackMenuItems[];
#else
extern MenuItem *g_apTrackMenuItems[];
#endif

#ifdef __cplusplus
}
#endif

#endif
