#ifndef COASTLINE_SEASONS_H
#define COASTLINE_SEASONS_H

/**
 * @file
 * The seasons of the championship.
 */

#include "profile.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A season of the championship. */
typedef struct {
#if TGR_PC
    u32 nameStringId; /*!< The string of the name of the season. */
#else
    const char *name;     /*!< The name of the season. */
    const char *subtitle; /*!< The heading of the records of the season. */
#endif
    s32 raceCount;             /*!< The number of races of the season. */
    s32 pointsNeeded;          /*!< The points a season needs to advance to the next. */
    u8 cars[2];                /*!< The two cars of the season, shown in its records. */
    u16 unlocks;               /*!< The cars passing the season unlocks, as bits of the unlocks. */
    u8 races[SEASON_RACES][2]; /*!< The track and the weather of each race. */
} Season;

/**
 * @ghidraAddress USA: 0x8028b944
 * @ghidraAddress EUR: 0x8028dbf4
 * @ghidraAddress JPN: 0x8028d624
 * @ghidraAddress BRD3D: 0x100b3810
 * @ghidraAddress BRGlide: 0x100b3018
 */
extern Season g_SeasonTable[SEASONS];

#ifdef __cplusplus
}
#endif

#endif /* COASTLINE_SEASONS_H */
