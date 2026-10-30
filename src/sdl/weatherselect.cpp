#include <array>

#include "menus.h"

namespace coastline {

namespace {

// The items of the screen in the order of the weathers, by the names of their models in the
// import.
constexpr std::array<const char *, 5> WEATHER_STEMS{"sunny", "fog", "rain", "snow", "night"};

// The weather a race of two players, or on Strip Mine, cannot be run in.
constexpr int WEATHER_SUNNY = 0;
constexpr int STRIP_MINE = 3;
constexpr int MIRROR_STRIP_MINE = 8;

} // namespace

bool Menus::LoadWeatherItems(SceneRenderer &renderer, const std::filesystem::path &release) {
    auto items = m_screen->LoadItems(renderer, release / "menu-items" / "weather", WEATHER_STEMS);
    if (!items) {
        return false;
    }
    m_weatherItems = std::move(*items);
    return true;
}

bool Menus::IsWeatherSelectable(int weather) const {
    return weather != WEATHER_SUNNY ||
           (m_selectedTrackId != STRIP_MINE && m_selectedTrackId != MIRROR_STRIP_MINE &&
            m_numLocalPlayers == 1);
}

void Menus::ProcessWeatherSelect(SceneRenderer &renderer, float seconds, const MenuButtons &first) {
    if (m_weatherSelectState <= 0) {
        m_screen->Reset();
        if (m_weatherSelectState == 0) {
            m_weatherMenuSelection = m_currentWeather;
        }
        // Move off a weather that may not be chosen. The console steps through as many entries
        // as there are tracks, though only the first step can occur.
        while (!IsWeatherSelectable(m_weatherMenuSelection)) {
            m_weatherMenuSelection = (m_weatherMenuSelection + 1) % m_numTracks;
        }
        m_weatherSelectState = 1;
    }

    MenuResult result =
        m_screen->Process(renderer,
                          seconds,
                          {&first, 1},
                          {.title = "WEATHER SELECT",
                           .items = m_weatherItems,
                           .selection = &m_weatherMenuSelection,
                           .isSelectable = [this](int i) { return IsWeatherSelectable(i); },
                           .top = {0x00, 0x00, 0x00},
                           .bottom = {0x20, 0x00, 0xA0}});
    if (result == MenuResult::Chosen) {
        m_currentWeather = m_weatherMenuSelection;
        SetCurrentScreen(Screen::CarSelect);
        m_weatherSelectState = -1;
    } else if (result == MenuResult::Cancelled) {
        SetCurrentScreen(Screen::TrackMenu);
        m_weatherSelectState = -1;
    }
}

} // namespace coastline
