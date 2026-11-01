#include <array>

#include <SDL3/SDL.h>

#include "assetpack.h"
#include "common/audio.h"
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

// The channels of the module of the menus are 16, and those of the modules of the races are 6.
constexpr int MENU_AUDIO_CHANNELS = 8;

} // namespace

std::unique_ptr<Menus> Menus::Load(SceneRenderer &renderer,
                                   const std::filesystem::path &release,
                                   const std::filesystem::path &saves,
                                   const std::filesystem::path &cache,
                                   GameConfig &config,
                                   const std::filesystem::path &configPath,
                                   std::span<const std::filesystem::path> dataRoots,
                                   std::string_view assetPack) {
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
    const bool pcPreviews =
        (assetPack.empty() ? config.Get("Game", "AssetPack").value_or("default") : assetPack) ==
        BOSS_PACK;
    if (!menus->LoadPlayerCountItems(renderer, release) ||
        !menus->LoadTrackMenu(renderer, release, pcPreviews) ||
        !menus->LoadWeatherItems(renderer, release) || !menus->LoadCarSelect(renderer, release) ||
        !menus->LoadOptionsItems(renderer, release) ||
        !menus->LoadControllerScreen(renderer, release)) {
        return nullptr;
    }
    menus->m_cache = cache;
    // The Controller Pak of the first controller is a directory of the saves, with its note of the
    // settings beside BossRally.ini.
    menus->m_pak =
        ControllerPak(saves / "controller-pak", configPath.parent_path() / "controller-pak");
    menus->m_ghostFramesFile = saves / "ghost-frames.bin";
    menus->LoadPakData();
    menus->m_config = &config;
    menus->m_configPath = configPath;
    std::vector<std::array<int, 2>> displayResolutions;
    int displayCount = 0;
    SDL_DisplayID *displays = SDL_GetDisplays(&displayCount);
    for (int display = 0; displays && display < displayCount; ++display) {
        int modeCount = 0;
        SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(displays[display], &modeCount);
        for (int mode = 0; modes && mode < modeCount; ++mode) {
            displayResolutions.push_back({modes[mode]->w, modes[mode]->h});
        }
        SDL_free(modes);
        if (const auto *desktop = SDL_GetDesktopDisplayMode(displays[display])) {
            displayResolutions.push_back({desktop->w, desktop->h});
        }
    }
    SDL_free(displays);
    menus->m_gameSettings = std::make_unique<GameSettings>(config, dataRoots, displayResolutions);
    menus->m_gameSettingsStart = static_cast<int>(menus->m_optionsMenuItems.size());
    for (std::size_t index = 0; index < menus->m_gameSettings->Count(); ++index) {
        menus->m_optionsMenuItems.push_back({menus->m_gameSettings->Label(index),
                                             MENU_ITEM_ADJUSTABLE | MENU_ITEM_IMMEDIATE,
                                             nullptr,
                                             true});
    }
    menus->LoadExtraTrackRecords();
    menus->LoadCarChoices();
    menus->m_paintShop = PaintShop::Load(renderer,
                                         release,
                                         *menus->m_screen,
                                         menus->m_textCustomColors,
                                         menus->m_pak,
                                         cache / "paint-shop");
    // The other screens show without the paint shop, which returns to the main menu.
    if (!menus->m_paintShop) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "The paint shop did not load.");
    }
    return menus;
}

void Menus::ProcessMainMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first) {
    if (m_mainMenuInitState <= 0) {
        m_numLocalPlayers = 2;
        m_screen->Reset();
        // The module of a race plays until the main menu, which starts the music of the menus.
        if (g_nAudioChannelCount < MENU_AUDIO_CHANNELS && m_playMenuMusic) {
            m_playMenuMusic();
        }
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
            StartDataSaveLoadMenu(SaveLoadEntry::MainMenu);
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
