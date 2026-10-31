#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "ghostnote.h"
#include "menus.h"
#include "menutext.h"
#include "volumelevels.h"

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

// The notes of the Controller Pak, as the console writes their names in the characters of the pak.
constexpr const char *SEASON_NOTE = "TOP GEAR SEASON";
constexpr const char *OPTIONS_NOTE = "TOP GEAR OPTIONS";
constexpr const char *GHOST_NOTE = "TOP GEAR GHOST";

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
    PAK_DATA_LOAD_SEASON,
    PAK_DATA_LOAD_GHOST,
    PAK_DATA_LOAD_CONFIGURATION,
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
    PAK_ERROR_GHOST_TOO_LARGE,
};

// The choices of the save and load menu.
enum SaveLoadChoice {
    SAVE_LOAD_LOAD_SEASON,
    SAVE_LOAD_SAVE_SEASON,
    SAVE_LOAD_LOAD_GHOST,
    SAVE_LOAD_SAVE_GHOST,
    SAVE_LOAD_LOAD_OPTIONS,
    SAVE_LOAD_SAVE_OPTIONS,
    SAVE_LOAD_CHOICES,
};
constexpr std::array<const char *, SAVE_LOAD_CHOICES> SAVE_LOAD_CHOICE_NAMES{"LOAD SEASON DATA",
                                                                             "SAVE SEASON DATA",
                                                                             "LOAD GHOST CAR DATA",
                                                                             "SAVE GHOST CAR DATA",
                                                                             "LOAD OPTIONS",
                                                                             "SAVE OPTIONS"};

// The steps of the save and load menu, without the steps of a missing pak, another pak, and a
// Rumble Pak.
enum SaveLoadStep {
    SAVE_LOAD_STEP_FADE_IN,
    SAVE_LOAD_STEP_CHOOSE,
    SAVE_LOAD_STEP_QUIET,
    SAVE_LOAD_STEP_WAIT_QUIET,
    SAVE_LOAD_STEP_FIND,
    SAVE_LOAD_STEP_LOAD,
    SAVE_LOAD_STEP_ALLOCATE,
    SAVE_LOAD_STEP_CONFIRM_OVERWRITE,
    SAVE_LOAD_STEP_SAVE,
    SAVE_LOAD_STEP_ERROR,
    SAVE_LOAD_STEP_DONE,
    SAVE_LOAD_STEP_FADE_OUT,
    SAVE_LOAD_STEP_LEAVE,
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

void Put16(std::vector<u8> &bytes, std::size_t at, u16 value) {
    bytes[at] = static_cast<u8>(value >> 8);
    bytes[at + 1] = static_cast<u8>(value);
}

// The fields of a profile ReadProfile() reads, over the bytes of a note. The bytes of the console
// the menus do not read are left as the note has them.
void WriteProfile(std::vector<u8> &bytes, const MenuProfile &profile) {
    Put32(bytes, 0x00, static_cast<u32>(profile.championshipFlags));
    bytes[0x04] = profile.season;
    bytes[0x05] = profile.round;
    for (std::size_t season = 0; season < profile.seasonPlaces.size(); ++season) {
        for (std::size_t race = 0; race < profile.seasonPlaces[season].size(); ++race) {
            bytes[0x06 + season * 4 + race] = profile.seasonPlaces[season][race];
            Put32(bytes,
                  0x2C + (season * 4 + race) * 4,
                  std::bit_cast<u32>(profile.seasonTimes[season][race]));
        }
        Put16(bytes, 0x1E + season * 2, profile.seasonPoints[season]);
    }
    for (std::size_t track = 0; track < profile.lapRecords.size(); ++track) {
        Put32(bytes, 0x8C + track * 4, std::bit_cast<u32>(profile.lapRecords[track]));
        Put32(bytes, 0xE8 + track * 4, std::bit_cast<u32>(profile.raceRecords[track]));
    }
    Put16(bytes, 0xCC, profile.unlocks);
    Put16(bytes, 0xCE, profile.unlockedTracks);
    Put16(bytes, 0xD0, profile.unlockedWeathers);
    GetEquipment(bytes, 0xD4, profile);
}

// The options as OptionSettings of the console at the start of the note of the options.
void ReadOptions(MenuOptions &options, const std::vector<u8> &bytes) {
    options.musicVolume = static_cast<s32>(Get32(bytes, 0));
    options.sfxVolume = static_cast<s32>(Get32(bytes, 4));
    options.speedInMph = Get32(bytes, 8) != 0;
}

} // namespace

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
    if (season && SeasonNoteIsValid(*season)) {
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
        m_pakNote = m_pak.ReadNote(dataType == PAK_DATA_LOAD_SEASON ? SEASON_NOTE :
                                   dataType == PAK_DATA_LOAD_GHOST  ? GHOST_NOTE :
                                                                      OPTIONS_NOTE);
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
        // A season or a ghost car loads without a message.
        if (dataType == PAK_DATA_LOAD_SEASON || dataType == PAK_DATA_LOAD_GHOST) {
            bool loaded = dataType == PAK_DATA_LOAD_SEASON ?
                              ReadSeasonNote(*m_pakNote, player, true) :
                              ReadGhostNote(*m_pakNote);
            if (loaded) {
                m_pakDataStep = PAK_DATA_STEP_FINISH;
            } else {
                m_pakError = PAK_ERROR_NOT_FOUND;
                m_pakDataStep = PAK_DATA_STEP_ERROR;
            }
            break;
        }
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
            m_optionsMenuState = -1;
            SetCurrentScreen(Screen::ControllerConfigScreen);
            break;
        case OPTIONS_ITEM_SAVE_CONFIGURATION:
            m_saveConfigurationPending = true;
            break;
        case OPTIONS_ITEM_LOAD_CONFIGURATION:
            m_loadConfigurationPending = true;
            break;
        case OPTIONS_ITEM_CREDITS:
            m_optionsMenuState = -1;
            m_end = MenusEnd::Credits;
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

void Menus::StartDataSaveLoadMenu(SaveLoadEntry entry) {
    m_saveLoadEntry = entry;
    SetCurrentScreen(Screen::DataSaveLoadMenu);
}

void Menus::DrawShortMessage(
    int boxX, int boxWidth, int textX, const char *line1, const char *line2) {
    DrawMessageBox(boxX, 0xD9, boxWidth, 0x4A);
    m_paintShop->DrawText(line1, textX, 0x7C);
    m_paintShop->DrawText(line2, textX, 0x8A);
}

void Menus::DrawSaveLoadChoices() {
    PaintShop &draw = *m_paintShop;
    // The current choice is brighter, and every choice dims while a message shows.
    auto colors = [&](bool current) {
        if (m_saveLoadInputEnabled) {
            if (current) {
                draw.SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00);
            } else {
                draw.SetTextCustomColors(0xFF, 0xF5, 0x00, 0xC8, 0x00, 0x00);
            }
        } else if (current) {
            draw.SetTextCustomColors(0x80, 0x80, 0x80, 0x84, 0x7C, 0x00);
        } else {
            draw.SetTextCustomColors(0x80, 0x78, 0x00, 0x64, 0x00, 0x00);
        }
    };
    constexpr int FOOTER_Y = 240 * 19 / 20 - 3;
    if (m_saveLoadEntry == SaveLoadEntry::MainMenu) {
        draw.m_text.size = 16;
        for (int choice = 0; choice != SAVE_LOAD_CHOICES; ++choice) {
            colors(choice == m_saveLoadChoice);
            draw.DrawText(
                SAVE_LOAD_CHOICE_NAMES[static_cast<std::size_t>(choice)], 0x9E, 0x57 + choice * 18);
        }
        draw.DrawTexturedSprite(*draw.m_buttonA, 0xCA, 0x1B0);
        draw.DrawTexturedSprite(*draw.m_buttonB, 0x142, 0x1B0);
        draw.m_text.align = TextStyle::Align::Left;
        draw.m_text.size = 11;
        draw.DrawText("%wwSelect", 0x74, FOOTER_Y);
        draw.DrawText("%wwGo Back", 0xB0, FOOTER_Y);
        return;
    }
    // After a race the menu only requests whether to save it.
    draw.m_text.size = 20;
    colors(true);
    if (m_saveLoadEntry == SaveLoadEntry::Season) {
        draw.DrawText("SAVE SEASON DATA?", 0x9E, 0x80);
        m_saveLoadChoice = SAVE_LOAD_SAVE_SEASON;
    } else {
        draw.DrawText("SAVE GHOST CAR DATA?", 0x9E, 0x80);
        m_saveLoadChoice = SAVE_LOAD_SAVE_GHOST;
    }
    draw.DrawTexturedSprite(*draw.m_buttonA, 0xEC, 0x1B0);
    draw.DrawTexturedSprite(*draw.m_buttonB, 0x154, 0x1B0);
    draw.m_text.align = TextStyle::Align::Left;
    draw.m_text.size = 11;
    draw.DrawText("%wwYES", 0x84, FOOTER_Y);
    draw.DrawText("%wwNO", 0xB9, FOOTER_Y);
}

void Menus::DrawSaveLoadError(int noteSize) {
    PaintShop &draw = *m_paintShop;
    bool load = m_saveLoadChoice == SAVE_LOAD_LOAD_SEASON ||
                m_saveLoadChoice == SAVE_LOAD_LOAD_GHOST ||
                m_saveLoadChoice == SAVE_LOAD_LOAD_OPTIONS;
    SetMessageText();
    switch (m_pakError) {
    case PAK_ERROR_FULL: {
        DrawMessageBox(0xA5, 0x9F, 0x136, 0xBD);
        const char *pages =
            noteSize == static_cast<int>(SEASON_NOTE_SIZE) ? "TWO PAGES AND ONE NOTE" :
            noteSize == static_cast<int>(GHOST_NOTE_SIZE)  ? "58 PAGES AND ONE NOTE" :
                                                             "ONE PAGE AND ONE NOTE";
        const char *data = noteSize == static_cast<int>(SEASON_NOTE_SIZE) ? "SEASON DATA." :
                           noteSize == static_cast<int>(GHOST_NOTE_SIZE)  ? "GHOST CAR DATA." :
                                                                            "OPTIONS.";
        draw.DrawText("INSUFFICIENT FREE PAGES", 0x5A, 0x5F);
        draw.DrawText("OR FREE NOTES IN THE", 0x5A, 0x5F + 0xD);
        draw.DrawText("SAVE FOLDER.", 0x5A, 0x5F + 0x1A);
        draw.DrawText(pages, 0x5A, 0x5F + 0x2D);
        draw.DrawText("ARE NEEDED TO SAVE THE", 0x5A, 0x5F + 0x3A);
        draw.DrawText(data, 0x5A, 0x5F + 0x47);
        break;
    }
    case PAK_ERROR_GHOST_TOO_LARGE:
        DrawMessageBox(0x75, 0xBD, 0x196, 0x82);
        draw.DrawText("UNABLE TO SAVE GHOST CAR DATA.", 0x42, 0x6E);
        draw.DrawText("GHOST CAR DATA IS LARGER THAN", 0x42, 0x7C);
        draw.DrawText("THE MAXIMUM ALLOCATED SPACE", 0x42, 0x8A);
        draw.DrawText("IN THE SAVE FOLDER.", 0x42, 0x98);
        break;
    case PAK_ERROR_NOT_FOUND:
        if (load) {
            DrawShortMessage(0xAD, 0x125, 0x5E, "SAVED DATA NOT FOUND", "IN THE SAVE FOLDER.");
        } else {
            DrawShortMessage(0xB6, 0x114, 0x63, "UNABLE TO SAVE DATA", "TO THE SAVE FOLDER.");
        }
        break;
    case PAK_ERROR_WRITE:
        DrawShortMessage(0xB6, 0x114, 0x63, "UNABLE TO SAVE DATA", "TO THE SAVE FOLDER.");
        break;
    default:
        DrawShortMessage(0xB1, 0x11E, 0x60, "SAVE FOLDER ERROR", "HAS BEEN DETECTED!");
        break;
    }
}

void Menus::DrawSaveLoadDone() {
    PaintShop &draw = *m_paintShop;
    draw.m_text = TextStyle();
    draw.m_text.size = 12;
    draw.m_text.align = TextStyle::Align::Center;
    int top = m_saveLoadEntry != SaveLoadEntry::MainMenu ? 0xCE : 0xD9;
    auto done = [&](int boxX, int boxWidth, const char *line1, const char *line2) {
        DrawMessageBox(boxX, top, boxWidth, 0x4A);
        draw.DrawText(line1, 0x9F, (top + 0x20) >> 1);
        draw.DrawText(line2, 0x9F, ((top + 0x20) >> 1) + 0xE);
    };
    switch (m_saveLoadChoice) {
    case SAVE_LOAD_LOAD_SEASON:
        done(0xE2, 0xBC, "%ywSEASON DATA", "%ywLOADED OK!");
        break;
    case SAVE_LOAD_SAVE_SEASON:
        done(0xE2, 0xBC, "%ywSEASON DATA", "%ywSAVED OK!");
        break;
    case SAVE_LOAD_LOAD_GHOST:
        done(0xE2, 0xBC, "%ywGHOST DATA", "%ywLOADED OK!");
        break;
    case SAVE_LOAD_SAVE_GHOST:
        done(0xD8, 0xD0, "%ywGHOST CAR DATA", "%ywSAVED OK!");
        break;
    case SAVE_LOAD_LOAD_OPTIONS:
        DrawMessageBox(0xC3, 0xE7, 0xFA, 0x2E);
        draw.DrawText("%ywOPTIONS LOADED OK!", 0x9F, 0x83);
        break;
    default:
        DrawMessageBox(0xC3, 0xE7, 0xFA, 0x2E);
        draw.DrawText("%ywOPTIONS SAVED OK!", 0x9F, 0x83);
        break;
    }
}

void Menus::DrawConfirmOverwrite() {
    PaintShop &draw = *m_paintShop;
    int top = m_saveLoadEntry != SaveLoadEntry::MainMenu ? 0xBF : 0xC9;
    // The buttons are placed by the width of the picture of A, not its height.
    int buttonY = top - draw.m_buttonA->width + 0x62;
    DrawMessageBox(0xD5, top, 0xD6, 0x6A);
    draw.m_text = TextStyle();
    draw.m_text.size = 13;
    draw.SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0x00);
    int textY = (top + 0x1E) >> 1;
    draw.DrawText("OK TO OVERWRITE", 0x72, textY);
    draw.DrawText("SAVED DATA?", 0x72, textY + 0xE);
    draw.m_text.size = 10;
    textY = (buttonY + 0x12) >> 1;
    draw.DrawText("%wwOK", (draw.m_buttonA->width + 0xED) >> 1, textY);
    // CANCEL is placed by the height of the picture of B, unlike the other labels.
    draw.DrawText("%wwCANCEL", (draw.m_buttonB->height + 0x13B) >> 1, textY);
    draw.DrawTexturedSprite(*draw.m_buttonA, 0xE7, buttonY);
    draw.DrawTexturedSprite(*draw.m_buttonB, 0x135, buttonY);
}

bool Menus::SeasonNoteIsValid(const std::vector<u8> &note) const {
    if (note.size() < PROFILE_SIZE) {
        return false;
    }
    // A note edited or damaged outside the game could name a season or a race past the
    // championship, or no car, and the car select would then find no car to start on.
    std::size_t season = note[0x04];
    int round = note[0x05];
    auto unlocks = static_cast<unsigned>(Get16(note, 0xCC));
    unsigned cars = (1u << m_cars.size()) - 1;
    const char *reason = nullptr;
    if (season >= m_seasons.size()) {
        reason = "a season past the last";
    } else if (round >= m_seasons[season].raceCount) {
        reason = "a race past the last of its season";
    } else if ((unlocks & cars) == 0) {
        reason = "no car";
    }
    if (reason != nullptr) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION, "The season note is ignored, as it has %s.", reason);
        return false;
    }
    return true;
}

bool Menus::ReadSeasonNote(const std::vector<u8> &note, int player, bool otherRetainsEquipment) {
    if (!SeasonNoteIsValid(note)) {
        return false;
    }
    // Both players take the season.
    MenuProfile &other = ProfileOf(player ^ 1);
    std::vector<u8> equipment(EQUIPMENT_SIZE);
    GetEquipment(equipment, 0, other);
    ReadProfile(m_profile, note);
    ReadProfile(m_secondProfile, note);
    if (otherRetainsEquipment) {
        SetEquipment(other, equipment, 0);
    }
    return true;
}

bool Menus::ReadGhostNote(const std::vector<u8> &note) {
    auto recording = UnpackGhostNote(note);
    // A recording written elsewhere may have any track and car.
    if (!recording || (*recording)[0] >= m_numTracks ||
        static_cast<std::size_t>((*recording)[1]) >= m_cars.size()) {
        return false;
    }
    // The race engine plays the ghost car from the frames it recorded with the recording. A ghost
    // recorded on the console has none, and its car does not race.
    m_ghostFrames = LoadGhostFrames(m_ghostFramesFile, *recording).value_or(std::vector<u8>());
    if (m_ghostFrames.empty()) {
        SDL_Log("The ghost car has no frames of the race engine and does not race.");
    }
    m_ghostRecordingSize = static_cast<int>(recording->size());
    recording->resize(GHOST_RECORDING_CAPACITY);
    m_ghostRecording = std::move(*recording);
    // The recording starts with its track and its car, and the load unlocks both.
    m_profile.unlockedTracks =
        static_cast<u16>(m_profile.unlockedTracks | 1 << m_ghostRecording[0]);
    m_profile.unlocks = static_cast<u16>(m_profile.unlocks | 1 << m_ghostRecording[1]);
    return true;
}

bool Menus::LoadSaveLoadNote(int player) {
    const std::vector<u8> &note = *m_pakNote;
    if (m_saveLoadChoice == SAVE_LOAD_LOAD_SEASON) {
        // The other player retains the equipment of the car when the main menu opened the menu.
        if (!ReadSeasonNote(note, player, m_saveLoadEntry == SaveLoadEntry::MainMenu)) {
            return false;
        }
        // A loaded season starts its race at once, without a message.
        m_saveLoadStep = SAVE_LOAD_STEP_FADE_OUT;
        m_saveLoadDataWasLoaded = true;
    } else if (m_saveLoadChoice == SAVE_LOAD_LOAD_GHOST) {
        if (!ReadGhostNote(note)) {
            return false;
        }
        m_selectedTrackId = m_ghostRecording[0];
        m_saveLoadStep = SAVE_LOAD_STEP_FADE_OUT;
        m_saveLoadDataWasLoaded = true;
    } else {
        if (note.size() < OPTIONS_HALF_SIZE + EQUIPMENT_SIZE) {
            return false;
        }
        ReadOptions(m_options, note);
        SetEquipment(ProfileOf(player), note, OPTIONS_HALF_SIZE);
        ApplyVolumeSettings();
        m_saveLoadStep = SAVE_LOAD_STEP_DONE;
    }
    return true;
}

int Menus::SaveSaveLoadNote(int player) {
    if (m_saveLoadChoice == SAVE_LOAD_SAVE_SEASON) {
        // The bytes of the note the menus do not read are written back as they are.
        std::vector<u8> note = m_pakNote.value_or(std::vector<u8>());
        note.resize(SEASON_NOTE_SIZE);
        WriteProfile(note, m_profile);
        return m_pak.WriteNote(SEASON_NOTE, note) ? PAK_ERROR_NONE : PAK_ERROR_WRITE;
    }
    if (m_saveLoadChoice == SAVE_LOAD_SAVE_GHOST) {
        auto recording = std::span<const u8>(m_ghostRecording)
                             .first(static_cast<std::size_t>(m_ghostRecordingSize));
        auto note = PackGhostNote(recording);
        if (!note) {
            return PAK_ERROR_GHOST_TOO_LARGE;
        }
        if (!m_pak.WriteNote(GHOST_NOTE, *note)) {
            return PAK_ERROR_WRITE;
        }
        // The note is written whether or not the frames are, and a ghost without its frames does
        // not race.
        if (!SaveGhostFrames(m_ghostFramesFile, recording, m_ghostFrames)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "The frames of the ghost car were not written to %s.",
                         m_ghostFramesFile.string().c_str());
        }
        return PAK_ERROR_NONE;
    }
    return SaveOptionsNote(player) ? PAK_ERROR_NONE : PAK_ERROR_WRITE;
}

void Menus::LeaveDataSaveLoadMenu() {
    m_saveLoadInitialized = false;
    m_pakError = PAK_ERROR_NONE;
    if (m_saveLoadChoice == SAVE_LOAD_LOAD_SEASON && m_saveLoadDataWasLoaded) {
        m_saveLoadDataWasLoaded = false;
        StartChampionshipRace();
    } else if (m_saveLoadChoice == SAVE_LOAD_LOAD_GHOST && m_saveLoadDataWasLoaded) {
        m_saveLoadDataWasLoaded = false;
        StartTimeAttack();
    } else {
        m_saveLoadChoice = SAVE_LOAD_LOAD_SEASON;
        SetCurrentScreen(m_saveLoadEntry == SaveLoadEntry::Season ? Screen::TrackMenu :
                                                                    Screen::MainMenu);
    }
}

void Menus::ProcessDataSaveLoadMenu(SceneRenderer &renderer,
                                    float seconds,
                                    const MenuButtons &first) {
    // The menu draws with the paint shop, and returns to the main menu without it.
    if (!m_paintShop) {
        SetCurrentScreen(Screen::MainMenu);
        return;
    }
    PaintShop &draw = *m_paintShop;
    if (!m_saveLoadInitialized) {
        m_saveLoadInitialized = true;
        m_saveLoadDataWasLoaded = false;
        m_saveLoadInputEnabled = true;
        m_pakError = PAK_ERROR_NONE;
        m_saveLoadFadeDelayFrames = 0;
        draw.LoadTextureFromRom(*draw.m_background);
        draw.LoadTextureFromRom(*draw.m_logo);
        if (m_saveLoadEntry == SaveLoadEntry::MainMenu) {
            m_saveLoadChoice = SAVE_LOAD_LOAD_SEASON;
        } else if (m_saveLoadEntry == SaveLoadEntry::Season) {
            m_saveLoadChoice = SAVE_LOAD_SAVE_SEASON;
        } else {
            m_saveLoadChoice = SAVE_LOAD_SAVE_GHOST;
        }
        m_saveLoadStep = SAVE_LOAD_STEP_FADE_IN;
        // Only the screen fades in. The music and the sound effects stay silent after the results
        // until the menu exits or returns to its choices.
        m_screen->FadeIn(false, false);
    }

    m_screen->UpdateFade(seconds);
    draw.BeginDraw(renderer);
    draw.DrawSpriteTinted(*draw.m_background, 0x60, 0, 0x80);
    draw.DrawTexturedSprite(*draw.m_logo, draw.m_logo->originX, draw.m_logo->originY + 8);

    int noteSize = static_cast<int>(OPTIONS_NOTE_SIZE);
    const char *noteName = OPTIONS_NOTE;
    if (m_saveLoadChoice == SAVE_LOAD_LOAD_SEASON || m_saveLoadChoice == SAVE_LOAD_SAVE_SEASON) {
        noteName = SEASON_NOTE;
        noteSize = static_cast<int>(SEASON_NOTE_SIZE);
    } else if (m_saveLoadChoice == SAVE_LOAD_LOAD_GHOST ||
               m_saveLoadChoice == SAVE_LOAD_SAVE_GHOST) {
        noteName = GHOST_NOTE;
        noteSize = static_cast<int>(GHOST_NOTE_SIZE);
    }

    draw.m_text = TextStyle();
    draw.m_text.size = 30;
    draw.m_text.align = TextStyle::Align::Center;
    draw.DrawText(
        m_saveLoadEntry == SaveLoadEntry::MainMenu ? "%ryDATA LOAD/SAVE" : "%ryDATA SAVE", 160, 41);
    DrawSaveLoadChoices();

    int player = m_screen->ChoosingPlayer();
    bool dismissed = first.choose || first.back;
    bool load = m_saveLoadChoice == SAVE_LOAD_LOAD_SEASON ||
                m_saveLoadChoice == SAVE_LOAD_LOAD_GHOST ||
                m_saveLoadChoice == SAVE_LOAD_LOAD_OPTIONS;
    // A message dismissed or cancelled from the main menu returns to the choices, and after a race
    // it exits.
    auto backToChoices = [&]() {
        if (m_saveLoadEntry == SaveLoadEntry::MainMenu) {
            m_screen->FadeSounds(1.0f);
            m_saveLoadStep = SAVE_LOAD_STEP_CHOOSE;
        } else {
            m_saveLoadStep = SAVE_LOAD_STEP_FADE_OUT;
        }
    };
    switch (m_saveLoadStep) {
    case SAVE_LOAD_STEP_FADE_IN:
        if (m_screen->FadeComplete()) {
            m_saveLoadStep = SAVE_LOAD_STEP_CHOOSE;
        }
        break;

    case SAVE_LOAD_STEP_CHOOSE:
        m_saveLoadInputEnabled = true;
        if (m_saveLoadEntry == SaveLoadEntry::MainMenu) {
            if (first.down) {
                m_saveLoadChoice = (m_saveLoadChoice + 1) % SAVE_LOAD_CHOICES;
            } else if (first.up) {
                m_saveLoadChoice = (m_saveLoadChoice + SAVE_LOAD_CHOICES - 1) % SAVE_LOAD_CHOICES;
            }
        }
        if (first.choose) {
            m_saveLoadStep = SAVE_LOAD_STEP_QUIET;
        } else if (first.back) {
            m_saveLoadStep = SAVE_LOAD_STEP_FADE_OUT;
        }
        break;

    case SAVE_LOAD_STEP_QUIET:
        m_screen->FadeSounds(0.0f);
        m_saveLoadStep = SAVE_LOAD_STEP_WAIT_QUIET;
        break;

    case SAVE_LOAD_STEP_WAIT_QUIET:
        if (m_screen->SoundsFaded() && m_saveLoadFadeDelayFrames++ == 4) {
            m_saveLoadStep = SAVE_LOAD_STEP_FIND;
            m_saveLoadFadeDelayFrames = 0;
        }
        break;

    case SAVE_LOAD_STEP_FIND:
        m_pakNote = m_pak.ReadNote(noteName);
        if (m_pakNote) {
            if (load) {
                m_saveLoadStep = SAVE_LOAD_STEP_LOAD;
            } else if (m_saveLoadChoice == SAVE_LOAD_SAVE_OPTIONS) {
                // The options are overwritten without requesting confirmation.
                m_saveLoadStep = SAVE_LOAD_STEP_SAVE;
            } else {
                m_saveLoadStep = SAVE_LOAD_STEP_CONFIRM_OVERWRITE;
            }
        } else if (load) {
            m_pakError = PAK_ERROR_NOT_FOUND;
            m_saveLoadStep = SAVE_LOAD_STEP_ERROR;
        } else {
            m_saveLoadStep = SAVE_LOAD_STEP_ALLOCATE;
        }
        break;

    case SAVE_LOAD_STEP_LOAD:
        if (!LoadSaveLoadNote(player)) {
            m_pakError = PAK_ERROR_NOT_FOUND;
            m_saveLoadStep = SAVE_LOAD_STEP_ERROR;
        }
        break;

    case SAVE_LOAD_STEP_ALLOCATE:
        if (m_pak.FreeBytes() < static_cast<std::size_t>(noteSize)) {
            m_pakError = PAK_ERROR_FULL;
            m_saveLoadStep = SAVE_LOAD_STEP_ERROR;
        } else {
            m_saveLoadStep = SAVE_LOAD_STEP_SAVE;
        }
        break;

    case SAVE_LOAD_STEP_CONFIRM_OVERWRITE:
        m_saveLoadInputEnabled = false;
        DrawConfirmOverwrite();
        if (first.choose) {
            m_saveLoadStep = SAVE_LOAD_STEP_SAVE;
        } else if (first.back) {
            backToChoices();
        }
        break;

    case SAVE_LOAD_STEP_SAVE:
        m_pakError = SaveSaveLoadNote(player);
        m_saveLoadStep = m_pakError == PAK_ERROR_NONE ? SAVE_LOAD_STEP_DONE : SAVE_LOAD_STEP_ERROR;
        break;

    case SAVE_LOAD_STEP_ERROR:
        m_saveLoadInputEnabled = false;
        DrawSaveLoadError(noteSize);
        if (dismissed) {
            m_pakError = PAK_ERROR_NONE;
            m_saveLoadStep = SAVE_LOAD_STEP_CHOOSE;
        }
        break;

    case SAVE_LOAD_STEP_DONE:
        m_saveLoadInputEnabled = false;
        DrawSaveLoadDone();
        if (dismissed) {
            backToChoices();
        }
        break;

    case SAVE_LOAD_STEP_FADE_OUT:
        m_screen->FadeOut();
        m_screen->FadeSounds(1.0f);
        m_saveLoadStep = SAVE_LOAD_STEP_LEAVE;
        break;

    default:
        break;
    }
    draw.FlushFills();
    m_screen->DrawFade(renderer);
    if (m_screen->FadingOut() && m_screen->FadeComplete()) {
        LeaveDataSaveLoadMenu();
    }
}

} // namespace coastline
