#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace coastline {

/** Five console tracks and their five mirrored variants precede additional tracks. */
inline constexpr int CONSOLE_TRACK_COUNT = 10;

/** Additional tracks after the console tracks, before menu actions. */
struct ExtraMenuTrack {
    int engine;             /*!< The track index accepted by the PC race engine. */
    std::string_view name;  /*!< The title displayed in track selection. */
    std::string_view file;  /*!< The asset basename shared by normal and mirrored variants. */
    bool mirrored;          /*!< Whether the race reflects the track horizontally. */
    bool selectable = true; /*!< Whether ordinary track selection can offer this entry. */
};

/** PC-only track entries; GAMEWIN remains indexed but unavailable for ordinary selection. */
inline constexpr std::array<ExtraMenuTrack, 5> EXTRA_MENU_TRACKS{{
    {5, "Race Track", "race", false},
    {11, "Mirror Race Track", "race", true},
    {14, "Bonus Track", "bonus", false},
    {13, "Mirror Bonus Track", "bonus", true},
    {12, "Gamewin", "gamewin", false, false},
}};

/** The number of track entries before menu actions, including unavailable entries. */
inline constexpr int MENU_TRACK_COUNT =
    CONSOLE_TRACK_COUNT + static_cast<int>(EXTRA_MENU_TRACKS.size());

/**
 * @param track The zero-based track-selection index.
 * @return The race-engine track, or -1 for a menu action or invalid index.
 */
constexpr int EngineTrackForMenu(int track) {
    if (track < 0 || track >= MENU_TRACK_COUNT) {
        return -1;
    }
    if (track < CONSOLE_TRACK_COUNT) {
        return track < 5 ? track : track + 1;
    }
    return EXTRA_MENU_TRACKS[static_cast<std::size_t>(track - CONSOLE_TRACK_COUNT)].engine;
}

/**
 * @param track The PC race-engine track index.
 * @return The menu track, or -1 for an invalid engine track.
 */
constexpr int MenuTrackForEngine(int track) {
    for (int menu = 0; menu < MENU_TRACK_COUNT; ++menu) {
        if (EngineTrackForMenu(menu) == track) {
            return menu;
        }
    }
    return -1;
}

} // namespace coastline
