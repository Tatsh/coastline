#pragma once

/**
 * @file
 * The levels of the volumes of the options menu of the Nintendo 64 release, from 0 to 10, and of
 * the race engine of the PC release, from 0 to 9.
 */

#include <algorithm>

namespace coastline {

/** The loudest level of the options menu. */
inline constexpr int MAX_VOLUME_LEVEL = 10;

/** The loudest level of the race engine. */
inline constexpr int MAX_RACE_VOLUME_LEVEL = 9;

/**
 * @param level A level of the options menu.
 * @return The nearest level of the race engine. The loudest level of each is the same.
 */
constexpr int RaceVolumeLevel(int level) {
    return (level * MAX_RACE_VOLUME_LEVEL + MAX_VOLUME_LEVEL / 2) / MAX_VOLUME_LEVEL;
}

/**
 * The level of the options menu after the pause menu of a race changed the level of the race
 * engine.
 *
 * @param raceLevel The level of the race engine as the race ended.
 * @param level The level of the options menu the race started at.
 * @return @p level when RaceVolumeLevel() gives @p raceLevel for it, else the highest level of the
 * options menu RaceVolumeLevel() gives @p raceLevel for.
 */
constexpr int MenuVolumeLevel(int raceLevel, int level) {
    raceLevel = std::clamp(raceLevel, 0, MAX_RACE_VOLUME_LEVEL);
    if (RaceVolumeLevel(level) == raceLevel) {
        return level;
    }
    return (raceLevel * MAX_VOLUME_LEVEL + MAX_RACE_VOLUME_LEVEL / 2) / MAX_RACE_VOLUME_LEVEL;
}

} // namespace coastline
