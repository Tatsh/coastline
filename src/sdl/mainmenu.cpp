#include <array>

#include "menus.h"

namespace coastline {

namespace {

// The items of the main menu in their order, by the names of their models in the import.
enum MainMenuItem {
    MAIN_MENU_CHAMPIONSHIP,
    MAIN_MENU_ARCADE,
    MAIN_MENU_TIME_ATTACK,
    MAIN_MENU_PRACTICE,
    MAIN_MENU_PAINT_SHOP,
    MAIN_MENU_LOAD_SAVE,
    MAIN_MENU_OPTIONS,
};
constexpr std::array<const char *, 7> MAIN_MENU_STEMS{
    "championship", "arcade", "time-attack", "practice", "paint-shop", "load-save", "options"};

// The modes that return to their item of the main menu.
constexpr int ENTRY_FROM_CHAMPIONSHIP = 1;
constexpr int ENTRY_FROM_TIME_ATTACK = 2;

// How long the main menu waits for a press before it exits to the splash screens.
constexpr float MAIN_MENU_IDLE_TIMEOUT = 20.0f;

} // namespace

std::unique_ptr<Menus> Menus::Load(SceneRenderer &renderer, const std::filesystem::path &release) {
    auto screen = MenuScreen::Load(renderer, release);
    if (!screen) {
        return nullptr;
    }
    auto main = screen->LoadItems(renderer, release / "menu-items" / "main", MAIN_MENU_STEMS);
    if (!main) {
        return nullptr;
    }
    std::unique_ptr<Menus> menus(new Menus(std::move(screen)));
    menus->m_mainMenuItems = std::move(*main);
    if (!menus->LoadPlayerCountItems(renderer, release) ||
        !menus->LoadTrackMenu(renderer, release) || !menus->LoadWeatherItems(renderer, release)) {
        return nullptr;
    }
    return menus;
}

void Menus::ProcessMainMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first) {
    if (m_mainMenuInitState <= 0) {
        m_numLocalPlayers = 2;
        m_screen->Reset();
        m_screen->titleImage = MenuTitleImage{&m_screen->MainMenuTitle(), 25, 14, 280, 32};
        m_screen->idleTimeout = MAIN_MENU_IDLE_TIMEOUT;

        // Return to the item of the mode that led back here.
        if (m_mainMenuEntrySource == ENTRY_FROM_CHAMPIONSHIP) {
            m_mainMenuSelection = MAIN_MENU_CHAMPIONSHIP;
        } else if (m_mainMenuEntrySource == ENTRY_FROM_TIME_ATTACK) {
            m_mainMenuSelection = MAIN_MENU_TIME_ATTACK;
        }
        m_mainMenuEntrySource = 0;
        if (m_mainMenuInitState == 0) {
            m_mainMenuSelection = MAIN_MENU_CHAMPIONSHIP;
        }
        m_mainMenuInitState = 1;
    }

    MenuResult result = m_screen->Process(renderer,
                                          seconds,
                                          {&first, 1},
                                          {.title = "TOP GEAR RALLY",
                                           .items = m_mainMenuItems,
                                           .selection = &m_mainMenuSelection,
                                           .isSelectable = {},
                                           .top = {0x00, 0x00, 0x00},
                                           .bottom = {0x40, 0x40, 0x40}});
    if (result == MenuResult::Chosen) {
        m_paintMode = false;
        switch (m_mainMenuSelection) {
        case MAIN_MENU_CHAMPIONSHIP:
            StartChampionshipRace();
            break;
        case MAIN_MENU_ARCADE:
            m_numLocalPlayers = 2;
            m_gameMode = GameMode::Arcade;
            SetCurrentScreen(Screen::PlayerCountSelect);
            break;
        case MAIN_MENU_PRACTICE:
            m_numLocalPlayers = 2;
            m_gameMode = GameMode::Practice;
            SetCurrentScreen(Screen::PlayerCountSelect);
            break;
        case MAIN_MENU_PAINT_SHOP:
            m_paintMode = true;
            m_numLocalPlayers = 1;
            SetCurrentScreen(Screen::CarSelect);
            break;
        case MAIN_MENU_LOAD_SAVE:
            SetCurrentScreen(Screen::DataSaveLoadMenu);
            break;
        case MAIN_MENU_OPTIONS:
            SetCurrentScreen(Screen::OptionsMenu);
            break;
        default:
            StartTimeAttack();
            break;
        }
        m_mainMenuInitState = -1;
    } else if (result == MenuResult::Cancelled) {
        m_end = MenusEnd::Splash;
        m_mainMenuInitState = 0;
    }
}

} // namespace coastline
