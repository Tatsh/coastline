#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "frontend.h"
#include "gpu.h"
#include "menus.h"
#include "menutext.h"

namespace coastline {

namespace {

// The items after the tracks on the track menu, by the names of their models in the import.
constexpr std::array<const char *, 3> RESET_STEMS{
    "reset-to-first-round", "reset-to-first-season", "reset-to-first-year"};

// How long the round under way blinks in the records, and how long of that it shows yellow.
constexpr float BLINK_PERIOD = 0.75f;
constexpr float BLINK_ON = 0.4f;

// The laps of an arcade race.
constexpr int MIN_LAPS = 1;
constexpr int MAX_LAPS = 5;

// The tracks whose names are mirrored ones, past the first five.
constexpr int MIRRORED_TRACKS = 5;

// The size of the ghost recording of the time attack once it is reset.
constexpr int EMPTY_GHOST_RECORDING = 8;

// The low-resolution screen the layouts of the console are written for.
constexpr int WIDTH = 320;
constexpr int HEIGHT = 240;

// FormatLapTime() of the console: minutes, seconds, and hundredths.
std::string FormatLapTime(float seconds) {
    auto hundredths = static_cast<int>(seconds * 100.0f);
    int whole = hundredths / 100;
    char text[32];
    std::snprintf(text,
                  sizeof(text),
                  "%d'%02d\"%02d",
                  whole / 60,
                  whole - whole / 60 * 60,
                  hundredths - whole * 100);
    return text;
}

} // namespace

bool Menus::LoadTrackMenu(SceneRenderer &renderer, const std::filesystem::path &release) {
    m_release = release;
    try {
        std::ifstream file(release / "data" / "menus.json");
        auto menu = nlohmann::json::parse(file);
        m_numTracks = menu.at("numTracks").get<int>();
        for (const auto &track : menu.at("tracks")) {
            MenuItem item;
            item.name = track.at("name").get<std::string>();
            item.flags = track.at("flags").get<u32>();
            item.model =
                m_screen->LoadModel(renderer, release / track.at("model").get<std::string>());
            if (!item.model) {
                return false;
            }
            m_trackMenuItems.push_back(std::move(item));
            m_trackExists.push_back(track.at("exists").get<bool>());
        }
        m_weatherNames = menu.at("weathers").get<std::vector<std::string>>();
        for (const auto &record : menu.at("cars")) {
            MenuCar car;
            car.name = record.at("name").get<std::string>();
            car.directory = record.at("directory").get<std::string>();
            car.exists = record.at("exists").get<bool>();
            car.driveType = record.at("driveType").get<int>();
            car.engineLayout = record.at("engineLayout").get<int>();
            car.acceleration = record.at("acceleration").get<float>();
            car.maxSpeed = record.at("maxSpeed").get<float>();
            car.gearCount = record.at("gearCount").get<int>();
            car.shadowLength = record.at("shadowLength").get<float>();
            car.shadowHalfWidth = record.at("shadowHalfWidth").get<float>();
            m_cars.push_back(std::move(car));
        }
        for (const auto &record : menu.at("seasons")) {
            MenuSeason season;
            season.name = record.at("name").get<std::string>();
            season.subtitle = record.at("subtitle").get<std::string>();
            season.raceCount = record.at("raceCount").get<int>();
            season.pointsNeeded = record.at("pointsNeeded").get<int>();
            season.cars = record.at("cars").get<std::array<int, 2>>();
            season.races = record.at("races").get<std::array<std::array<int, 2>, 4>>();
            m_seasons.push_back(std::move(season));
        }
    } catch (const nlohmann::json::exception &error) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s does not have the track menu: %s",
                     release.string().c_str(),
                     error.what());
        return false;
    }
    auto resets = m_screen->LoadItems(renderer, release / "menu-items", RESET_STEMS);
    if (!resets) {
        return false;
    }
    m_trackMenuItems.insert(m_trackMenuItems.end(), resets->begin(), resets->end());
    return !m_seasons.empty() && m_numTracks + 4 == static_cast<int>(m_trackMenuItems.size());
}

void Menus::SetTrackFromSeason() {
    const MenuSeason &season = m_seasons[m_profile.season];
    m_selectedTrackId = season.races[m_profile.round][0];
    if (m_profile.championshipFlags & 1) {
        m_selectedTrackId = m_selectedTrackId < MIRRORED_TRACKS ?
                                m_selectedTrackId + MIRRORED_TRACKS :
                                m_selectedTrackId - MIRRORED_TRACKS;
    }
    m_currentWeather = season.races[m_profile.round][1];
}

void Menus::StartChampionshipRace() {
    m_gameMode = GameMode::Championship;
    SetTrackFromSeason();
    m_numLocalPlayers = 1;
    SetCurrentScreen(Screen::TrackMenu);
    m_mainMenuEntrySource = 1;
}

void Menus::StartTimeAttack() {
    m_gameMode = GameMode::TimeAttack;
    m_numLocalPlayers = 1;
    m_currentWeather = 1;
    SetCurrentScreen(Screen::TrackMenu);
    m_mainMenuEntrySource = 2;
}

// The two cars of the season of the profile, as the console loads them into the two slots.
void Menus::LoadSeasonCars(SceneRenderer &renderer) {
    const MenuSeason &season = m_seasons[m_profile.season];
    for (std::size_t slot = 0; slot < season.cars.size(); ++slot) {
        LoadCarModel(renderer, static_cast<int>(slot), season.cars[slot]);
    }
}

bool Menus::IsTrackUnlocked(int track) const {
    if (m_gameMode == GameMode::Championship) {
        if (m_seasonFirstRace) {
            return track == m_selectedTrackId;
        }
        return track == m_selectedTrackId || track >= m_numTracks;
    }
    if (m_gameMode == GameMode::TimeAttack && (track == m_numTracks || track == m_numTracks + 2)) {
        return true;
    }
    return (m_profile.unlockedTracks & (1 << track)) && track < m_numTracks &&
           m_trackExists[static_cast<std::size_t>(track)];
}

void Menus::ValidateChampionshipProgress() {
    if (m_profile.championshipFlags < 0) {
        m_profile.championshipFlags = 0;
        m_profile.season = 0;
        m_profile.round = 0;
        m_profile.seasonPoints[m_profile.season] = 0;
    }
    if (m_profile.season >= m_seasons.size()) {
        m_profile.season = 0;
        m_profile.round = 0;
        m_profile.seasonPoints[m_profile.season] = 0;
    }
    if (m_profile.round >= m_seasons[m_profile.season].raceCount) {
        m_profile.round = 0;
        m_profile.seasonPoints[m_profile.season] = 0;
    }
}

void Menus::TrackMenuPostDraw(SceneRenderer &renderer, int item) {
    Matrix4 flat = MenuScreen::FlatFor(renderer);
    const MenuText &text = m_screen->Text();
    TextStyle style;
    style.size = 10;
    style.align = TextStyle::Align::Center;
    style.colors = m_textCustomColors;
    if (m_gameMode == GameMode::Arcade) {
        text.Draw(renderer, "%wwPush up or down to adjust number of laps", 160, 90, style, flat);
    }
    if (m_gameMode == GameMode::Championship && item < m_numTracks) {
        int needed =
            m_seasons[m_profile.season].pointsNeeded - m_profile.seasonPoints[m_profile.season];
        std::string points;
        if (needed <= 0) {
            points = "%wwYou have enough points to advance to next season";
        } else {
            text.Draw(renderer,
                      "%wwFirst=9, Second=6, Third=5, Fourth=3, Fifth=2, Sixth=1",
                      160,
                      100,
                      style,
                      flat);
            points = "%ww" + std::to_string(needed) + " point" + (needed == 1 ? "" : "s") +
                     " needed to advance to next season";
        }
        text.Draw(renderer, points, 160, 90, style, flat);
    }
    if (m_trackMenuPakScreen) {
        // ponytail: the screen of the controller pak is not built yet, and it returns at once as
        // if the load found no data.
        SDL_Log("The controller pak screen is not built yet.");
        m_trackMenuPakScreen = false;
    }
}

void Menus::SeasonRecordsPreDraw(SceneRenderer &renderer) {
    m_seasonRecordsBlinkTime += m_frameSeconds;
    while (BLINK_PERIOD < m_seasonRecordsBlinkTime) {
        m_seasonRecordsBlinkTime -= BLINK_PERIOD;
    }
    int round = m_profile.round;
    int flags = m_profile.championshipFlags;
    const MenuSeason &season = m_seasons[m_profile.season];
    Matrix4 flat = MenuScreen::FlatFor(renderer);
    const MenuText &text = m_screen->Text();
    TextStyle style;
    style.size = 20;
    style.align = TextStyle::Align::Center;
    style.colors = m_textCustomColors;
    text.Draw(renderer, "%ry" + season.subtitle + ":", WIDTH / 2, HEIGHT * 20 / 64, style, flat);

    // The columns move right when a name is of the longer, mirrored tracks.
    int shift = 0;
    for (int i = 0; i < season.raceCount; ++i) {
        if (season.races[static_cast<std::size_t>(i)][0] < MIRRORED_TRACKS) {
            if (flags & 1) {
                shift = WIDTH * 3 / 64;
                break;
            }
        } else if (!(flags & 1)) {
            shift = WIDTH * 3 / 64;
            break;
        }
    }

    style.size = 8;
    style.align = TextStyle::Align::Right;
    text.Draw(renderer, "Race Record", WIDTH * 23 / 32 + shift, 86, style, flat);
    text.Draw(renderer, "Lap Record", WIDTH * 28 / 32 + shift + 2, 86, style, flat);
    style.size = 10;
    int y = 95;
    for (int race = 0; race < season.raceCount; ++race, y += 10) {
        int track = season.races[static_cast<std::size_t>(race)][0];
        if (flags & 1) {
            track = track < MIRRORED_TRACKS ? track + MIRRORED_TRACKS : track - MIRRORED_TRACKS;
        }
        style.align = TextStyle::Align::Left;
        if (race == round && m_seasonRecordsBlinkTime < BLINK_ON) {
            m_textCustomColors = std::array<int, 6>{0xFF, 0xF0, 0x50, 0xFF, 0xF0, 0x50};
        } else {
            m_textCustomColors = std::array<int, 6>{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        }
        style.colors = m_textCustomColors;
        text.Draw(
            renderer, "Race " + std::to_string(race + 1), WIDTH * 4 / 32 - shift, y, style, flat);
        text.Draw(renderer,
                  m_trackMenuItems[static_cast<std::size_t>(track)].name + "/" +
                      m_weatherNames[static_cast<std::size_t>(
                          season.races[static_cast<std::size_t>(race)][1])],
                  WIDTH * 8 / 32 - shift + 4,
                  y,
                  style,
                  flat);
        style.align = TextStyle::Align::Right;
        float raceRecord = m_profile.raceRecords[static_cast<std::size_t>(track)];
        text.Draw(renderer,
                  raceRecord == 0.0f ? "--    " : FormatLapTime(raceRecord),
                  WIDTH * 23 / 32 + shift - 1,
                  y,
                  style,
                  flat);
        float lapRecord = m_profile.lapRecords[static_cast<std::size_t>(track)];
        text.Draw(renderer,
                  lapRecord == 0.0f ? "--    " : FormatLapTime(lapRecord),
                  WIDTH * 28 / 32 + shift,
                  y,
                  style,
                  flat);
    }

    m_textCustomColors = std::array<int, 6>{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    style.colors = m_textCustomColors;
    style.align = TextStyle::Align::Center;
    text.Draw(renderer, "Cars For", WIDTH >> 1, 148, style, flat);
    text.Draw(renderer, "This Season:", WIDTH >> 1, 158, style, flat);

    // The two cars of the season side by side, each on its stage from the trackside camera of its
    // view, without the choice marker.
    GpuWindow &window = renderer.Window();
    float aspect = static_cast<float>(window.FrameWidth()) /
                   static_cast<float>(std::max<Uint32>(window.FrameHeight(), 1));
    struct View {
        int x;
        float angle;
    };
    constexpr std::array<View, 2> VIEWS{{{0, HALF_PI}, {WIDTH * 3 / 8, 0.0f}}};
    constexpr std::array<float, 3> EYE{10.0f, 10.0f, 5.0f};
    for (std::size_t slot = 0; slot < VIEWS.size(); ++slot) {
        int x = VIEWS[slot].x;
        int top = HEIGHT * 7 / 16;
        int width = WIDTH * 5 / 8;
        int height = HEIGHT * 5 / 8;
        Matrix4 camera = StageCamera(aspect, EYE, x, top, width, height);
        DrawCarStage(renderer, camera, true);
        DrawStageCar(renderer,
                     camera,
                     EYE,
                     static_cast<int>(slot),
                     0.0f,
                     0.0f,
                     VIEWS[slot].angle,
                     FrameRect(window.FrameWidth(), window.FrameHeight(), x, top, width, height));
    }
}

void Menus::ProcessTrackMenu(SceneRenderer &renderer, float seconds, const MenuButtons &first) {
    m_frameSeconds = seconds;
    if (m_trackMenuInitState <= 0) {
        m_screen->Reset();
        for (int i = 0; i < m_numTracks; ++i) {
            MenuItem &item = m_trackMenuItems[static_cast<std::size_t>(i)];
            if (m_gameMode == GameMode::Arcade) {
                item.flags |= MENU_ITEM_ADJUSTABLE;
            } else {
                item.flags &= ~MENU_ITEM_ADJUSTABLE;
            }
        }
        ValidateChampionshipProgress();
        LoadSeasonCars(renderer);
        m_trackMenuSelection = m_selectedTrackId;
        while (!IsTrackUnlocked(m_trackMenuSelection)) {
            m_trackMenuSelection = (m_trackMenuSelection + 1) % m_numTracks;
        }
        m_screen->SetLastActiveIndex(m_trackMenuSelection);
        m_screen->postDraw = [this](SceneRenderer &frame, int item) {
            TrackMenuPostDraw(frame, item);
        };
        m_trackMenuChampionship = m_gameMode == GameMode::Championship;
        m_seasonFirstRace = m_trackMenuChampionship && !m_continueFromResults;
        m_continueFromResults = false;
        auto rename = [this](int offset, const char *name) {
            m_trackMenuItems[static_cast<std::size_t>(m_numTracks + offset)].name = name;
        };
        if (m_trackMenuChampionship) {
            rename(0, "Load Season Data");
            rename(1, "Reset To First Round");
            rename(2, "Reset To First Season");
            rename(3, "Reset To First Year");
        } else {
            rename(0, "Load Ghost Car Data");
            rename(2, "Reset Ghost Car");
        }
        m_trackMenuInitState = 1;
    }

    // The item of the track of the next race, or of the arcade, has its race or its laps in its
    // name for the frame.
    int named = -1;
    std::string name;
    MenuResult result;
    if (m_gameMode == GameMode::Championship) {
        named = m_selectedTrackId;
        MenuItem &item = m_trackMenuItems[static_cast<std::size_t>(named)];
        name = item.name;
        if (m_seasonFirstRace) {
            m_screen->preDraw = [this](SceneRenderer &frame) { SeasonRecordsPreDraw(frame); };
            item.flags |= MENU_ITEM_IMMEDIATE;
        } else {
            m_screen->preDraw = nullptr;
            item.name = "Race " + std::to_string(m_profile.round + 1) + ": " + name + "/" +
                        m_weatherNames[static_cast<std::size_t>(m_currentWeather)];
        }
        result = m_screen->Process(renderer,
                                   seconds,
                                   {&first, 1},
                                   {.title = m_seasons[m_profile.season].name,
                                    .items = m_trackMenuItems,
                                    .selection = &m_trackMenuSelection,
                                    .isSelectable = [this](int i) { return IsTrackUnlocked(i); },
                                    .top = {0x00, 0x00, 0x00},
                                    .bottom = {0x00, 0x78, 0x82}});
        if (m_seasonFirstRace) {
            item.flags &= ~MENU_ITEM_IMMEDIATE;
        }
    } else {
        if (m_gameMode == GameMode::Arcade) {
            named = m_screen->LastActiveIndex();
            MenuItem &item = m_trackMenuItems[static_cast<std::size_t>(named)];
            name = item.name;
            item.name = name + ": " + std::to_string(m_totalLaps) + " laps";
        }
        result = m_screen->Process(renderer,
                                   seconds,
                                   {&first, 1},
                                   {.title = "TRACK SELECT",
                                    .items = m_trackMenuItems,
                                    .selection = &m_trackMenuSelection,
                                    .isSelectable = [this](int i) { return IsTrackUnlocked(i); },
                                    .top = {0x00, 0x00, 0x00},
                                    .bottom = {0x80, 0x00, 0x80}});
    }
    if (named >= 0) {
        m_trackMenuItems[static_cast<std::size_t>(named)].name = name;
    }

    // Each reset of the championship includes the ones before it.
    auto resetRound = [&]() {
        m_profile.round = 0;
        m_profile.seasonPoints[m_profile.season] = 0;
        StartChampionshipRace();
        m_trackMenuSelection = m_selectedTrackId;
        m_screen->SetPendingSelection(m_selectedTrackId);
        LoadSeasonCars(renderer);
        m_seasonFirstRace = true;
    };
    if (result == MenuResult::Chosen) {
        int selection = m_trackMenuSelection;
        if (selection == m_numTracks) {
            m_trackMenuPakScreen = true;
        } else if (selection == m_numTracks + 1) {
            if (m_gameMode == GameMode::Championship) {
                resetRound();
            }
        } else if (selection == m_numTracks + 2) {
            if (m_gameMode == GameMode::Championship) {
                m_profile.season = 0;
                resetRound();
            } else if (m_gameMode == GameMode::TimeAttack) {
                m_ghostRecordingSize = EMPTY_GHOST_RECORDING;
                StartTimeAttack();
                m_trackMenuSelection = m_selectedTrackId;
                m_screen->SetPendingSelection(m_selectedTrackId);
            }
        } else if (selection == m_numTracks + 3) {
            if (m_gameMode == GameMode::Championship) {
                m_profile.championshipFlags = 0;
                m_profile.season = 0;
                resetRound();
            }
        } else if (m_seasonFirstRace) {
            // The records shown, the next race is chosen.
            m_seasonFirstRace = false;
        } else {
            m_selectedTrackId = selection;
            if (m_gameMode == GameMode::TimeAttack || m_gameMode == GameMode::Championship) {
                SetCurrentScreen(Screen::CarSelect);
            } else {
                SetCurrentScreen(Screen::WeatherSelect);
            }
            m_trackMenuInitState = -1;
        }
    } else if (result == MenuResult::Cancelled) {
        if (m_gameMode == GameMode::TimeAttack || m_gameMode == GameMode::Championship) {
            SetCurrentScreen(Screen::MainMenu);
        } else {
            SetCurrentScreen(Screen::PlayerCountSelect);
        }
        m_trackMenuInitState = 0;
    } else if (result == MenuResult::Up) {
        if (m_gameMode == GameMode::Arcade && m_totalLaps < MAX_LAPS) {
            ++m_totalLaps;
        }
    } else if (result == MenuResult::Down) {
        if (m_gameMode == GameMode::Arcade && m_totalLaps > MIN_LAPS) {
            --m_totalLaps;
        }
    }
}

} // namespace coastline
