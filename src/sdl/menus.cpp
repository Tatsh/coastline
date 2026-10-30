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
    constexpr const char *NOT_BUILT[] = {nullptr,
                                         nullptr,
                                         nullptr,
                                         nullptr,
                                         nullptr,
                                         "data save and load",
                                         "options",
                                         nullptr,
                                         nullptr};
    const char *name = NOT_BUILT[static_cast<int>(screen)];
    if (name != nullptr) {
        SDL_Log("The %s screen is not built yet.", name);
        screen = Screen::MainMenu;
    }
    m_current = screen;
}

bool Menus::DrawsPointer() const {
    return m_current == Screen::PaintShopScreen;
}

bool Menus::TakesText() const {
    return m_current == Screen::PaintShopScreen && m_paintShop->TakesText();
}

MenusEnd Menus::Frame(SceneRenderer &renderer,
                      float seconds,
                      const std::array<MenuButtons, 2> &players,
                      bool secondController,
                      const PaintShopInput &pointer) {
    // Escape on the keyboard backs out of a screen as B does, and the paint shop takes it itself.
    bool escape = pointer.escape && !m_escapeDown;
    m_escapeDown = pointer.escape;
    std::array<MenuButtons, 2> buttons = players;
    if (escape && m_current != Screen::PaintShopScreen) {
        buttons[0].back = true;
    }
    // The screens but the car select read the first controller.
    const MenuButtons &first = buttons[0];
    m_end = MenusEnd::Running;
    m_pointer = pointer;
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
        ProcessCarSelectScreen(renderer, seconds, buttons);
        break;
    case Screen::RaceResultsScreen:
        ProcessRaceResultsScreen(renderer, seconds, first);
        break;
    case Screen::PaintShopScreen:
        // The paint shop returns to the main menu on the item of the paint shop.
        if (m_paintShop->Frame(renderer, seconds, pointer)) {
            SetCurrentScreen(Screen::MainMenu);
        }
        break;
    default:
        break;
    }
    return m_end;
}

} // namespace coastline
