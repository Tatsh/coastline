#ifndef COASTLINE_TRACKMENUITEM_H
#define COASTLINE_TRACKMENUITEM_H

#include "types.h"

struct Model;

/** The flag of an item whose model is drawn mirrored. */
#define MENU_ITEM_MIRRORED 0x10

/** An item of a menu screen. */
typedef struct {
    const char *name;    /*!< The text of the item. */
    u32 flags;           /*!< The MENU_ITEM_* flags. */
    struct Model *model; /*!< The model of the item, or NULL to show its name. */
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    const u32 *romStart;
    const u32 *romEnd;
#else
    u32 romStart; /*!< The cartridge offset of the model of the item. */
    u32 romEnd;   /*!< The cartridge offset of the end of the model of the item. */
#endif
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

#if TGR_PLATFORM == TGR_PLATFORM_WIN
/** The adjacent palette and image filenames for a sky texture. */
typedef struct {
    char palette[32];
    char image[32];
} SkyTextureFiles;
#endif

/**
 * A track: its item of the track menu, where its geometry and its two sections of route are on
 * the cartridge, and its arcade times. The end of the geometry is 0 for a track that does not
 * exist.
 */
typedef struct {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
    MenuItem item;
    const u32 *trackRom;
    const u32 *trackRomEnd;
    const SkyTextureFiles *daySkyFiles;
    const u32 *daySkyRomEnd;
    const SkyTextureFiles *nightSkyFiles;
    const u32 *nightSkyRomEnd;
#else
    MenuItem item;      /*!< The item of the track menu. */
    u32 trackRom;       /*!< The cartridge offset of the compressed geometry. */
    u32 trackRomEnd;    /*!< The cartridge offset of the end of the geometry, 0 for no track. */
    u32 sectionARom;    /*!< The cartridge offset of the first section of route. */
    u32 sectionARomEnd; /*!< The cartridge offset of the end of the first section of route. */
    u32 sectionBRom;    /*!< The cartridge offset of the second section of route. */
    u32 sectionBRomEnd; /*!< The cartridge offset of the end of the second section of route. */
#endif /* TGR_PLATFORM == TGR_PLATFORM_WIN */
    ArcadeTimes arcadeTimes[ARCADE_CAR_CLASSES][ARCADE_WEATHERS]; /*!< The arcade times by car
                                                                     class and by weather less 1. */
} TrackMenuItem;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The track descriptors.
 *
 * @ghidraAddress USA: 0x80270854
 * @ghidraAddress EUR: 0x80272834
 * @ghidraAddress JPN: 0x802722f4
 * @ghidraAddress BRD3D: 0x100bbae8
 * @ghidraAddress BRGlide: 0x100bb2f0
 */
extern TrackMenuItem g_aTrackDescriptors[];

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

#endif /* COASTLINE_TRACKMENUITEM_H */
