#include "menus.h"

#include <SDL3/SDL.h>

namespace coastline {

Menus::Menus(std::unique_ptr<MenuScreen> screen) : m_screen(std::move(screen)) {
}

Menus::~Menus() = default;

void Menus::Restart() {
    m_current = Screen::MainMenu;
}

void Menus::SetCurrentScreen(Screen screen) {
    // ponytail: a screen not built yet returns to the main menu on the item chosen.
    constexpr const char *NOT_BUILT[] = {
        nullptr, nullptr, nullptr, nullptr, nullptr, "data save and load", "options"};
    const char *name = NOT_BUILT[static_cast<int>(screen)];
    if (name != nullptr) {
        SDL_Log("The %s screen is not built yet.", name);
        screen = Screen::MainMenu;
    }
    m_current = screen;
}

MenusEnd Menus::Frame(SceneRenderer &renderer,
                      float seconds,
                      const MenuButtons &first,
                      bool secondController) {
    m_end = MenusEnd::Running;
    switch (m_current) {
    case Screen::MainMenu:
        ProcessMainMenu(renderer, seconds, first);
        break;
    case Screen::PlayerCountSelect:
        ProcessPlayerCountSelect(renderer, seconds, first, secondController);
        break;
    case Screen::TrackMenu:
        ProcessTrackMenu(renderer, seconds, first);
        break;
    case Screen::WeatherSelect:
        ProcessWeatherSelect(renderer, seconds, first);
        break;
    case Screen::CarSelect:
        ProcessCarSelectScreen(renderer, seconds, first);
        break;
    default:
        break;
    }
    return m_end;
}

} // namespace coastline
