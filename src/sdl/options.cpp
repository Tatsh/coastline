#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "menus.h"
#include "menutext.h"

namespace coastline {

namespace {

// The items of the options menu, by the names of their models in the import. Load Configuration
// shares the model of the Load item of the track menu, and the import files the model under the
// track menu.
enum OptionsItem {
    OPTIONS_ITEM_BGM,
    OPTIONS_ITEM_SFX,
    OPTIONS_ITEM_UNITS,
    OPTIONS_ITEM_CONTROLLER,
    OPTIONS_ITEM_SAVE_CONFIGURATION,
    OPTIONS_ITEM_LOAD_CONFIGURATION,
    OPTIONS_ITEM_CREDITS,
};
constexpr std::array<const char *, 5> OPTIONS_STEMS{
    "bgm", "sfx", "units", "controller-configuration", "save-configuration"};
constexpr const char *LOAD_CONFIGURATION_MODEL = "tracks/load-and-gamewin/menu-model.gltf";
constexpr const char *CREDITS_STEM = "credits";

// g_adwBgmVolumeLevels and g_adwSfxVolumeLevels of the console, by the levels of the options.
constexpr std::array<u8, 11> BGM_VOLUME_LEVELS{
    0x00, 0x04, 0x09, 0x0E, 0x13, 0x18, 0x1C, 0x21, 0x26, 0x2B, 0x30};
constexpr std::array<u8, 11> SFX_VOLUME_LEVELS{
    0x00, 0x19, 0x33, 0x4C, 0x66, 0x7F, 0x99, 0xB2, 0xCC, 0xE5, 0xFF};
constexpr int MAX_VOLUME_LEVEL = 10;

// The levels of the race engine, from 0 to 9.
constexpr int MAX_RACE_VOLUME_LEVEL = 9;

// The notes of the Controller Pak, as the console writes their names in the characters of the pak.
constexpr const char *SEASON_NOTE = "TOP GEAR SEASON";
constexpr const char *OPTIONS_NOTE = "TOP GEAR OPTIONS";

// The sizes of the notes of a season and of the options. The options store the settings in the
// first half of their note and the equipment of the car in the second.
constexpr std::size_t SEASON_NOTE_SIZE = 0x200;
constexpr std::size_t OPTIONS_NOTE_SIZE = 0x100;
constexpr std::size_t OPTIONS_HALF_SIZE = 0x80;

// The bytes of the profile of a player in the note of a season, and of the equipment of a car in
// the note of the options: handling, gearbox, tyres, suspension, and button layout.
constexpr std::size_t PROFILE_SIZE = 0x128;
constexpr std::size_t EQUIPMENT_SIZE = 5 * 4;

// The data ProcessPakDataScreen() loads or saves.
enum PakData {
    PAK_DATA_LOAD_CONFIGURATION = 2,
    PAK_DATA_SAVE_CONFIGURATION,
};

// The steps of the screen of the Controller Pak data. A pak of files is never missing, never
// another pak, and never a Rumble Pak, and the screen never takes the steps for those.
enum PakDataStep {
    PAK_DATA_STEP_START,
    PAK_DATA_STEP_WAIT_QUIET,
    PAK_DATA_STEP_FIND,
    PAK_DATA_STEP_LOAD,
    PAK_DATA_STEP_ALLOCATE,
    PAK_DATA_STEP_SAVE,
    PAK_DATA_STEP_ERROR,
    PAK_DATA_STEP_DONE,
    PAK_DATA_STEP_FINISH,
};

// The errors of the directory of the Controller Pak. The console reports PFS_ERR_INVALID for a
// missing note and PFS_DATA_FULL for a full pak.
enum PakError {
    PAK_ERROR_NONE,
    PAK_ERROR_NOT_FOUND,
    PAK_ERROR_FULL,
    PAK_ERROR_WRITE,
};

// The notes are in the byte order of the console.
u32 Get32(const std::vector<u8> &bytes, std::size_t at) {
    return static_cast<u32>(bytes[at]) << 24 | static_cast<u32>(bytes[at + 1]) << 16 |
           static_cast<u32>(bytes[at + 2]) << 8 | bytes[at + 3];
}

u16 Get16(const std::vector<u8> &bytes, std::size_t at) {
    return static_cast<u16>(bytes[at] << 8 | bytes[at + 1]);
}

void Put32(std::vector<u8> &bytes, std::size_t at, u32 value) {
    bytes[at] = static_cast<u8>(value >> 24);
    bytes[at + 1] = static_cast<u8>(value >> 16);
    bytes[at + 2] = static_cast<u8>(value >> 8);
    bytes[at + 3] = static_cast<u8>(value);
}

// The equipment of the car of a profile, as GET_EQUIPMENT and SET_EQUIPMENT of the console copy it.
void GetEquipment(std::vector<u8> &bytes, std::size_t at, const MenuProfile &profile) {
    Put32(bytes, at, static_cast<u32>(profile.handling));
    Put32(bytes, at + 4, static_cast<u32>(profile.transmission));
    Put32(bytes, at + 8, static_cast<u32>(profile.tires));
    Put32(bytes, at + 12, static_cast<u32>(profile.suspension));
    Put32(bytes, at + 16, static_cast<u32>(profile.controllerLayout));
}

void SetEquipment(MenuProfile &profile, const std::vector<u8> &bytes, std::size_t at) {
    profile.handling = static_cast<s32>(Get32(bytes, at));
    profile.transmission = static_cast<s32>(Get32(bytes, at + 4));
    profile.tires = static_cast<s32>(Get32(bytes, at + 8));
    profile.suspension = static_cast<s32>(Get32(bytes, at + 12));
    profile.controllerLayout = static_cast<s32>(Get32(bytes, at + 16));
}

// The PlayerProfile of the console at the start of the note of a season.
void ReadProfile(MenuProfile &profile, const std::vector<u8> &bytes) {
    profile.championshipFlags = static_cast<s32>(Get32(bytes, 0x00));
    profile.season = bytes[0x04];
    profile.round = bytes[0x05];
    for (std::size_t season = 0; season < profile.seasonPlaces.size(); ++season) {
        for (std::size_t race = 0; race < profile.seasonPlaces[season].size(); ++race) {
            profile.seasonPlaces[season][race] = bytes[0x06 + season * 4 + race];
            profile.seasonTimes[season][race] =
                std::bit_cast<f32>(Get32(bytes, 0x2C + (season * 4 + race) * 4));
        }
        profile.seasonPoints[season] = Get16(bytes, 0x1E + season * 2);
    }
    for (std::size_t track = 0; track < profile.lapRecords.size(); ++track) {
        profile.lapRecords[track] = std::bit_cast<f32>(Get32(bytes, 0x8C + track * 4));
        profile.raceRecords[track] = std::bit_cast<f32>(Get32(bytes, 0xE8 + track * 4));
    }
    profile.unlocks = Get16(bytes, 0xCC);
    profile.unlockedTracks = Get16(bytes, 0xCE);
    profile.unlockedWeathers = Get16(bytes, 0xD0);
    SetEquipment(profile, bytes, 0xD4);
}

// The options as OptionSettings of the console at the start of the note of the options.
void ReadOptions(MenuOptions &options, const std::vector<u8> &bytes) {
    options.musicVolume = static_cast<s32>(Get32(bytes, 0));
    options.sfxVolume = static_cast<s32>(Get32(bytes, 4));
    options.speedInMph = Get32(bytes, 8) != 0;
}

} // namespace

int Menus::RaceVolumeLevel(int level) {
    // The race engine has a level fewer, and the loudest of each is the same.
    return (level * MAX_RACE_VOLUME_LEVEL + MAX_VOLUME_LEVEL / 2) / MAX_VOLUME_LEVEL;
}

bool Menus::LoadOptionsItems(SceneRenderer &renderer, const std::filesystem::path &release) {
    auto items = m_screen->LoadItems(renderer, release / "menu-items" / "options", OPTIONS_STEMS);
    const char *const credits[] = {CREDITS_STEM};
    auto last = m_screen->LoadItems(renderer, release / "menu-items" / "options", credits);
    const RenderModel *load = m_screen->LoadModel(renderer, release / LOAD_CONFIGURATION_MODEL);
    if (!items || !last || load == nullptr) {
        return false;
    }
    m_optionsMenuItems = std::move(*items);
    m_optionsMenuItems.push_back({"Load Configuration", MENU_ITEM_IMMEDIATE, load});
    m_optionsMenuItems.push_back(std::move(last->front()));
    return true;
}

void Menus::LoadPakData() {
    // The quiet loads of the season and of the configuration as the console boots.
    auto season = m_pak.ReadNote(SEASON_NOTE);
    if (season && season->size() >= PROFILE_SIZE) {
        ReadProfile(m_profile, *season);
        ReadProfile(m_secondProfile, *season);
    }
    auto options = m_pak.ReadNote(OPTIONS_NOTE);
    if (options && options->size() >= OPTIONS_HALF_SIZE + EQUIPMENT_SIZE) {
        ReadOptions(m_options, *options);
        SetEquipment(m_profile, *options, OPTIONS_HALF_SIZE);
    }
    ApplyVolumeSettings();
}

void Menus::ApplyVolumeSettings() const {
    // A note written elsewhere may have any level.
    g_bBgmVolume = BGM_VOLUME_LEVELS[static_cast<std::size_t>(
        std::clamp(m_options.musicVolume, 0, MAX_VOLUME_LEVEL))];
    g_bSfxMasterVolume = SFX_VOLUME_LEVELS[static_cast<std::size_t>(
        std::clamp(m_options.sfxVolume, 0, MAX_VOLUME_LEVEL))];
}

bool Menus::SaveOptionsNote(int player) const {
    std::vector<u8> note(OPTIONS_NOTE_SIZE, 0);
    Put32(note, 0, static_cast<u32>(m_options.musicVolume));
    Put32(note, 4, static_cast<u32>(m_options.sfxVolume));
    Put32(note, 8, m_options.speedInMph ? 1 : 0);
    GetEquipment(note, OPTIONS_HALF_SIZE, player == 0 ? m_profile : m_secondProfile);
    return m_pak.WriteNote(OPTIONS_NOTE, note);
}

void Menus::DrawMessageBox(int x, int y, int width, int height) {
    m_paintShop->DrawRectBorder(x, y, width, height, 3, 0, 0, 0x80, 0x80, 0x80);
}

void Menus::SetMessageText() {
    m_paintShop->m_text = TextStyle();
    m_paintShop->m_text.size = 12;
    m_paintShop->SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0x00);
}

bool Menus::ProcessPakDataScreen(SceneRenderer &renderer, int dataType) {
    // The screen draws with the paint shop, and finishes at once without it.
    if (!m_paintShop) {
        return true;
    }
    const MenuButtons &input = m_pakInput;
    bool dismissed = input.choose || input.back;
    if (!m_pakDataInitialized) {
        m_pakDataInitialized = true;
        m_pakError = PAK_ERROR_NONE;
        m_pakDataStep = PAK_DATA_STEP_START;
        m_pakDataWaitFrames = 0;
    }
    int player = m_screen->ChoosingPlayer();
    m_paintShop->BeginDraw(renderer);
    switch (m_pakDataStep) {
    case PAK_DATA_STEP_START:
        m_screen->FadeSounds(0.0f);
        m_pakDataStep = PAK_DATA_STEP_WAIT_QUIET;
        break;

    case PAK_DATA_STEP_WAIT_QUIET:
        if (m_screen->SoundsFaded() && m_pakDataWaitFrames++ == 3) {
            m_pakDataStep = PAK_DATA_STEP_FIND;
        }
        break;

    case PAK_DATA_STEP_FIND:
        m_pakNote = m_pak.ReadNote(OPTIONS_NOTE);
        if (m_pakNote) {
            m_pakDataStep =
                dataType == PAK_DATA_SAVE_CONFIGURATION ? PAK_DATA_STEP_SAVE : PAK_DATA_STEP_LOAD;
        } else if (dataType == PAK_DATA_SAVE_CONFIGURATION) {
            m_pakDataStep = PAK_DATA_STEP_ALLOCATE;
        } else {
            m_pakError = PAK_ERROR_NOT_FOUND;
            m_pakDataStep = PAK_DATA_STEP_ERROR;
        }
        break;

    case PAK_DATA_STEP_LOAD:
        if (m_pakNote->size() < OPTIONS_HALF_SIZE + EQUIPMENT_SIZE) {
            m_pakError = PAK_ERROR_NOT_FOUND;
            m_pakDataStep = PAK_DATA_STEP_ERROR;
            break;
        }
        ReadOptions(m_options, *m_pakNote);
        SetEquipment(ProfileOf(player), *m_pakNote, OPTIONS_HALF_SIZE);
        m_pakDataStep = PAK_DATA_STEP_DONE;
        break;

    case PAK_DATA_STEP_ALLOCATE:
        if (m_pak.FreeBytes() < OPTIONS_NOTE_SIZE) {
            m_pakError = PAK_ERROR_FULL;
            m_pakDataStep = PAK_DATA_STEP_ERROR;
        } else {
            m_pakDataStep = PAK_DATA_STEP_SAVE;
        }
        break;

    case PAK_DATA_STEP_SAVE:
        if (SaveOptionsNote(player)) {
            m_pakDataStep = PAK_DATA_STEP_DONE;
        } else {
            m_pakError = PAK_ERROR_WRITE;
            m_pakDataStep = PAK_DATA_STEP_ERROR;
        }
        break;

    case PAK_DATA_STEP_ERROR:
        SetMessageText();
        switch (m_pakError) {
        case PAK_ERROR_FULL:
            DrawMessageBox(0xA5, 0xA5, 0x136, 0xBD);
            m_paintShop->DrawText("INSUFFICIENT FREE PAGES", 0x5A, 0x62);
            m_paintShop->DrawText("OR FREE NOTES IN THE", 0x5A, 0x62 + 0xD);
            m_paintShop->DrawText("SAVE FOLDER.", 0x5A, 0x62 + 0x1A);
            m_paintShop->DrawText("ONE PAGE AND ONE NOTE", 0x5A, 0x62 + 0x2D);
            m_paintShop->DrawText("ARE NEEDED TO SAVE THE", 0x5A, 0x62 + 0x3A);
            m_paintShop->DrawText("OPTIONS.", 0x5A, 0x62 + 0x47);
            break;
        case PAK_ERROR_NOT_FOUND:
            if (dataType != PAK_DATA_SAVE_CONFIGURATION) {
                DrawMessageBox(0xAF, 0xD9, 0x122, 0x4A);
                m_paintShop->DrawText("SAVED DATA NOT FOUND", 0x5F, 0x7C);
                m_paintShop->DrawText("IN THE SAVE FOLDER.", 0x5F, 0x8A);
                break;
            }
            [[fallthrough]];
        default:
            DrawMessageBox(0xAF, 0xD9, 0x122, 0x4A);
            m_paintShop->DrawText("SAVE FOLDER ERROR", 0x5F, 0x7C);
            m_paintShop->DrawText("HAS BEEN DETECTED!", 0x5F, 0x8A);
            break;
        }
        if (dismissed) {
            m_pakError = PAK_ERROR_NONE;
            m_pakDataStep = PAK_DATA_STEP_FINISH;
        }
        break;

    case PAK_DATA_STEP_DONE:
        SetMessageText();
        m_paintShop->m_text.align = TextStyle::Align::Center;
        DrawMessageBox(0xD8, 0xD9, 0xD0, 0x4A);
        m_paintShop->DrawText("%ywCONFIGURATION", 0x9F, 0x7C);
        m_paintShop->DrawText(
            dataType == PAK_DATA_LOAD_CONFIGURATION ? "%ywLOADED OK!" : "%ywSAVED OK!", 0x9F, 0x8A);
        if (dismissed) {
            if (dataType == PAK_DATA_LOAD_CONFIGURATION) {
                ApplyVolumeSettings();
            }
            m_pakDataStep = PAK_DATA_STEP_FINISH;
        }
        break;

    default:
        m_pakError = PAK_ERROR_NONE;
        m_pakDataInitialized = false;
        m_screen->FadeSounds(1.0f);
        m_paintShop->FlushFills();
        return true;
    }
    m_paintShop->FlushFills();
    return false;
}

void Menus::OptionsMenuPostDraw(SceneRenderer &renderer, int item) {
    if (item < OPTIONS_ITEM_UNITS) {
        TextStyle style;
        style.size = 10;
        style.align = TextStyle::Align::Center;
        style.colors = m_textCustomColors;
        m_screen->Text().Draw(renderer,
                              "%wwPush up or down to adjust",
                              160,
                              90,
                              style,
                              MenuScreen::FlatFor(renderer));
    }
    if (m_loadConfigurationPending) {
        if (ProcessPakDataScreen(renderer, PAK_DATA_LOAD_CONFIGURATION)) {
            m_loadConfigurationPending = false;
        }
    } else if (m_saveConfigurationPending) {
        if (ProcessPakDataScreen(renderer, PAK_DATA_SAVE_CONFIGURATION)) {
            m_saveConfigurationPending = false;
        }
    }
}

void Menus::ProcessOptionsMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first) {
    if (m_optionsMenuState <= 0) {
        m_screen->Reset();
        m_screen->postDraw = [this](SceneRenderer &frame, int item) {
            OptionsMenuPostDraw(frame, item);
        };
        m_screen->titleImage = MenuTitleImage{&m_screen->TitleBar(), 22, 9, 280, 42};
        if (m_optionsMenuState == 0) {
            m_optionsChanged = false;
            m_optionsMenuSelection = OPTIONS_ITEM_BGM;
        }
        m_optionsMenuState = 1;
    }

    m_optionsMenuItems[OPTIONS_ITEM_BGM].name =
        "BGM Volume: " + std::to_string(m_options.musicVolume);
    m_optionsMenuItems[OPTIONS_ITEM_SFX].name =
        "SFX Volume: " + std::to_string(m_options.sfxVolume);
    m_optionsMenuItems[OPTIONS_ITEM_UNITS].name =
        m_options.speedInMph ? "Units: mph" : "Units: kph";

    // The menu takes no input while the screen of the Controller Pak runs and reads it instead.
    m_pakInput = first;
    const MenuButtons none;
    bool pakScreen = m_loadConfigurationPending || m_saveConfigurationPending;
    MenuResult result = m_screen->Process(renderer,
                                          seconds,
                                          {pakScreen ? &none : &first, 1},
                                          {.title = "OPTIONS",
                                           .items = m_optionsMenuItems,
                                           .selection = &m_optionsMenuSelection,
                                           .isSelectable = {},
                                           .top = {0x00, 0x00, 0x00},
                                           .bottom = {0x80, 0x40, 0x20}});
    // IncreaseBgmVolume() and the others of the console, by a level within the levels.
    auto step = [this](int &level, int by) {
        level = std::clamp(level + by, 0, MAX_VOLUME_LEVEL);
        ApplyVolumeSettings();
        m_optionsChanged = true;
    };
    switch (result) {
    case MenuResult::Down:
    case MenuResult::Up:
        if (m_optionsMenuSelection == OPTIONS_ITEM_BGM) {
            step(m_options.musicVolume, result == MenuResult::Up ? 1 : -1);
        } else if (m_optionsMenuSelection == OPTIONS_ITEM_SFX) {
            step(m_options.sfxVolume, result == MenuResult::Up ? 1 : -1);
        } else if (m_optionsMenuSelection == OPTIONS_ITEM_UNITS) {
            m_options.speedInMph = !m_options.speedInMph;
            m_optionsChanged = true;
        }
        break;
    case MenuResult::Chosen:
        switch (m_optionsMenuSelection) {
        case OPTIONS_ITEM_UNITS:
            m_options.speedInMph = !m_options.speedInMph;
            m_optionsChanged = true;
            break;
        case OPTIONS_ITEM_CONTROLLER:
            // ponytail: the controller configuration screen is not built yet, and the options
            // show again.
            SDL_Log("The controller configuration screen is not built yet.");
            m_optionsMenuState = -1;
            break;
        case OPTIONS_ITEM_SAVE_CONFIGURATION:
            m_saveConfigurationPending = true;
            break;
        case OPTIONS_ITEM_LOAD_CONFIGURATION:
            m_loadConfigurationPending = true;
            break;
        case OPTIONS_ITEM_CREDITS:
            // ponytail: the credits race is not built yet, and the options show again.
            SDL_Log("The credits race is not built yet.");
            m_optionsMenuState = -1;
            break;
        default:
            break;
        }
        break;
    case MenuResult::Cancelled:
        SetCurrentScreen(Screen::MainMenu);
        m_optionsMenuState = 0;
        // Changed options are saved as the menu closes, as the European and Japanese releases save
        // them to the cartridge, here to the note of the options.
        if (m_optionsChanged && !SaveOptionsNote(0)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "The options were not saved.");
        }
        break;
    default:
        break;
    }
}

} // namespace coastline
