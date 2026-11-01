#include "menus.h"

#include <algorithm>

#include <SDL3/SDL.h>

namespace coastline {

Menus::Menus(std::unique_ptr<MenuScreen> screen) : m_screen(std::move(screen)) {
}

Menus::~Menus() = default;

void Menus::Restart() {
    m_current = Screen::MainMenu;
    m_exitEscapeSeconds = 0.0f;
    // The attract demo races at the levels of the race engine.
    ApplyVolumeSettings();
}

void Menus::FinishCredits() {
    // The credits race at the levels of the race engine.
    ApplyVolumeSettings();
    SetCurrentScreen(Screen::OptionsMenu);
}

void Menus::SetCurrentScreen(Screen screen) {
    m_current = screen;
    m_exitEscapeSeconds = 0.0f;
}

bool Menus::DrawsPointer() const {
    return m_current == Screen::PaintShopScreen;
}

bool Menus::TakesText() const {
    return m_current == Screen::PaintShopScreen && m_paintShop->TakesText();
}

int Menus::ControlledPlayers() const {
    return m_current == Screen::CarSelect ? m_numLocalPlayers : 1;
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
    if (m_current == Screen::MainMenu) {
        m_exitEscapeSeconds = std::max(0.0f, m_exitEscapeSeconds - seconds);
        if (escape) {
            if (m_exitEscapeSeconds > 0.0f) {
                return MenusEnd::Quit;
            }
            m_exitEscapeSeconds = 0.5f;
        }
        if (pointer.escape) {
            buttons[0].back = false;
        }
    } else {
        m_exitEscapeSeconds = 0.0f;
        if (escape && m_current != Screen::PaintShopScreen) {
            buttons[0].back = true;
        }
    }
    // The screens but the car select read the first controller.
    const MenuButtons &first = buttons[0];
    m_end = MenusEnd::Running;
    m_pointer = pointer;
    m_retiredCarModels.clear();
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
    case Screen::DataSaveLoadMenu:
        ProcessDataSaveLoadMenu(renderer, seconds, first);
        break;
    case Screen::OptionsMenu:
        ProcessOptionsMenu(renderer, seconds, first);
        break;
    case Screen::ControllerConfigScreen:
        ProcessControllerConfigScreen(renderer, seconds, first);
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
