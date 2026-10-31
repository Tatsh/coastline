#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace coastline {

inline constexpr int CONSOLE_TRACK_COUNT = 10;

/** Additional tracks after the console tracks, before menu actions. */
struct ExtraMenuTrack {
    int engine;
    std::string_view name;
    std::string_view file;
    bool mirrored;
};

inline constexpr std::array<ExtraMenuTrack, 5> EXTRA_MENU_TRACKS{{
    {5, "Race Track", "race", false},
    {11, "Mirror Race Track", "race", true},
    {14, "Bonus Track", "bonus", false},
    {13, "Mirror Bonus Track", "bonus", true},
    {12, "Gamewin", "gamewin", false},
}};

inline constexpr int MENU_TRACK_COUNT = CONSOLE_TRACK_COUNT + EXTRA_MENU_TRACKS.size();

/** @return The race-engine track, or -1 for a menu action or invalid index. */
constexpr int EngineTrackForMenu(int track) {
    if (track < 0 || track >= MENU_TRACK_COUNT) {
        return -1;
    }
    if (track < CONSOLE_TRACK_COUNT) {
        return track < 5 ? track : track + 1;
    }
    return EXTRA_MENU_TRACKS[static_cast<std::size_t>(track - CONSOLE_TRACK_COUNT)].engine;
}

/** @return The menu track, or -1 for an invalid engine track. */
constexpr int MenuTrackForEngine(int track) {
    for (int menu = 0; menu < MENU_TRACK_COUNT; ++menu) {
        if (EngineTrackForMenu(menu) == track) {
            return menu;
        }
    }
    return -1;
}

} // namespace coastline
