// The race the menus chose and the results screen after it. The results screen of the console is
// its car select screen in review mode: the car of the race on its stage, the places and the times
// of the race, the races of the season, and the message of the season, a page each.
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

#include <SDL3/SDL.h>

#include "common/game.h"
#include "common/race.h"
#include "common/weather.h"
#include "frontend.h"
#include "menus.h"
#include "menutext.h"

namespace coastline {

namespace {

// The pages of the results.
enum ResultsPage {
    RESULTS_PAGE_PLAYER_1,
    RESULTS_PAGE_PLAYER_2,
    RESULTS_PAGE_SEASON,
    RESULTS_PAGE_MESSAGE,
    RESULTS_PAGE_END,
};

// The first step of choosing a car, the step of the stages of the results, and the last.
constexpr int CAR_SELECT_CAR = 0;
constexpr int CAR_SELECT_READY = 6;

// The distance the views of two players spread the cars apart by.
constexpr float SPREAD_TWO_VIEWS = 12.0f;

// The low-resolution screen the layouts of the console are written for.
constexpr int WIDTH = 320;
constexpr int HEIGHT = 240;

// The tracks of the console, the mirrored ones after the others, and the track of the race after a
// season.
constexpr int MENU_TRACKS = 10;
constexpr int MIRRORED_TRACKS = 5;
constexpr int AFTER_SEASON_MENU_TRACK = 10;

// The tracks of the race engine: the mirrored ones after the six of the PC release, and
// gamewin.trk.
constexpr int PC_MIRRORED_TRACKS = 6;
constexpr int PC_AFTER_SEASON_TRACK = 12;

// The seasons of the championship, and the unlocks of the strip mine and of the cars.
constexpr int SEASONS = 6;
constexpr int STRIP_MINE = 3;
constexpr int MIRROR_STRIP_MINE = 8;

// g_apszPlaceNames of the console.
constexpr std::array<const char *, 20> PLACE_NAMES{
    "1st",  "2nd",  "3rd",  "4th",  "5th",  "6th",  "7th",  "8th",  "9th",  "10th",
    "11th", "12th", "13th", "14th", "15th", "16th", "17th", "18th", "19th", "20th"};

// The track of the race engine of a track of the console.
int PcTrack(int track) {
    if (track == AFTER_SEASON_MENU_TRACK) {
        return PC_AFTER_SEASON_TRACK;
    }
    return track < MIRRORED_TRACKS ? track : track - MIRRORED_TRACKS + PC_MIRRORED_TRACKS;
}

// The weather of a race: SetupRaceVehicles() of the console races a time attack on the strip mine
// and its mirror in the fog, and on the other tracks in the sun.
int RaceWeather(GameMode mode, int track, int weather) {
    if (mode != GameMode::TimeAttack) {
        return weather;
    }
    return track == STRIP_MINE || track == MIRROR_STRIP_MINE ? WEATHER_FOG : WEATHER_SUNNY;
}

const char *PlaceName(int place) {
    return PLACE_NAMES[static_cast<std::size_t>(std::clamp(place, 0, 19))];
}

int PlacePoints(int place) {
    return g_abRacePositionPoints[std::clamp(place, 0, 19)];
}

// FormatLapTime() of the console.
std::string FormatLapTime(f32 seconds) {
    auto hundredths = static_cast<s32>(seconds * 100.0f);
    s32 whole = hundredths / 100;
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

RaceSetup Menus::ChosenRace() const {
    RaceSetup setup;
    setup.gameMode = m_afterSeasonRace ? GAME_MODE_AFTER_SEASON : static_cast<int>(m_gameMode);
    setup.players = m_numLocalPlayers;
    setup.track = PcTrack(m_selectedTrackId);
    setup.weather = RaceWeather(m_gameMode, m_selectedTrackId, m_currentWeather);
    // InitRace() of the console races three laps but in arcade.
    setup.laps = m_gameMode == GameMode::Arcade ? m_totalLaps : 3;
    for (int player = 0; player < m_numLocalPlayers; ++player) {
        const MenuProfile &profile = player == 0 ? m_profile : m_secondProfile;
        setup.cars[static_cast<std::size_t>(player)] = {
            m_carSelect[static_cast<std::size_t>(player)].carModel,
            profile.handling,
            profile.transmission,
            profile.tires,
            profile.suspension,
            std::nullopt,
            std::nullopt};
        // The custom decals race the model the decals loaded into, and ExtractVehicleBaseColor()
        // takes the colour of the car from its paint.
        auto index = static_cast<std::size_t>(player);
        const auto &custom = m_customDecals[index];
        if (m_carSelect[index].customDecals && custom && m_customDecalImages[index]) {
            RacePlayerSetup &car = setup.cars[index];
            car.images = m_customDecalImages[index];
            u16 paint = custom->PaletteColor(CAR_MATERIAL_PAINT, 0);
            car.color =
                std::array<u8, 3>{static_cast<u8>(((paint >> 13) & 7) | ((paint >> 8) & 0xF8)),
                                  static_cast<u8>(((paint >> 8) & 7) | ((paint >> 3) & 0xF8)),
                                  static_cast<u8>(((paint >> 3) & 7) | ((paint << 2) & 0xF8))};
        }
    }
    setup.season = m_profile.season;
    setup.round = m_profile.round;
    setup.championshipFlags = m_profile.championshipFlags;
    for (int track = 0; track < MENU_TRACKS; ++track) {
        auto pc = static_cast<std::size_t>(PcTrack(track));
        setup.lapRecords[pc] = m_profile.lapRecords[static_cast<std::size_t>(track)];
        setup.raceRecords[pc] = m_profile.raceRecords[static_cast<std::size_t>(track)];
    }
    return setup;
}

void Menus::FinishRace(RaceEnd end, const RaceResults &results) {
    bool afterSeason = m_afterSeasonRace;
    m_afterSeasonRace = false;
    if (end == RaceEnd::Results) {
        // The races and the laps of record the race set.
        m_results = results;
        for (int track = 0; track < MENU_TRACKS; ++track) {
            auto pc = static_cast<std::size_t>(PcTrack(track));
            m_profile.lapRecords[static_cast<std::size_t>(track)] = results.lapRecords[pc];
            m_profile.raceRecords[static_cast<std::size_t>(track)] = results.raceRecords[pc];
        }
        // RestoreRaceResultsFromSlots() of the console: the points, the place, and the time of
        // the race of the season.
        if (m_gameMode == GameMode::Championship) {
            int place = results.players[0].position;
            m_profile.seasonPoints[m_profile.season] =
                static_cast<u16>(m_profile.seasonPoints[m_profile.season] + PlacePoints(place));
            m_profile.seasonPlaces[m_profile.season][m_profile.round] = static_cast<u8>(place);
            m_profile.seasonTimes[m_profile.season][m_profile.round] = results.players[0].totalTime;
        }
        m_resultsInitialized = false;
        SetCurrentScreen(Screen::RaceResultsScreen);
    } else if (end == RaceEnd::SeasonMenu) {
        // After the race of a season, the championship goes on with the next season.
        // ponytail: the data save and load screen is not built yet, and the track menu follows.
        m_gameMode = GameMode::Championship;
        SetTrackFromSeason();
        m_continueFromResults = false;
        SetCurrentScreen(Screen::TrackMenu);
    } else {
        if (afterSeason) {
            SDL_Log("The ending credits are not built yet.");
        }
        SetCurrentScreen(Screen::MainMenu);
    }
}

void Menus::StartRaceResults(SceneRenderer &renderer) {
    m_screen->FadeIn();
    m_resultsSeason = m_profile.season;
    m_resultsRound = m_profile.round;
    m_resultsWeather = RaceWeather(m_gameMode, m_selectedTrackId, m_currentWeather);
    m_resultsChampionshipFlags = m_profile.championshipFlags;
    m_resultsTrack = m_selectedTrackId;
    m_seasonResultsShown = false;
    m_carSelectAngle = {0.0f, 1.0f};
    // The cars of the race in the slots of the players, on the step of the car.
    for (int i = 0; i < m_numLocalPlayers; ++i) {
        CarSelectPlayer &player = m_carSelect[static_cast<std::size_t>(i)];
        player.slot = i;
        player.state = CAR_SELECT_CAR;
        player.slide = 0.0f;
        player.cursor = 0;
        player.options = &m_handlingOptions;
        LoadCarModel(renderer, i, player.carModel);
    }
    m_resultsPage = RESULTS_PAGE_PLAYER_1;
    m_resultsInitialized = true;
}

// ProcessSeasonResults() of the console: the next race of the season, or after its last race the
// next season and its bonuses, or the season again without the points to pass it.
void Menus::ProcessSeasonResults() {
    m_seasonMessage.clear();
    if (m_gameMode != GameMode::Championship) {
        return;
    }
    auto bonus = [this](const char *line) {
        if (!m_seasonMessage.empty()) {
            m_seasonMessage.emplace_back();
        }
        m_seasonMessage.emplace_back(line);
    };
    MenuProfile &profile = m_profile;
    ++profile.round;
    if (profile.round == m_seasons[profile.season].raceCount) {
        profile.round = 0;
        if (profile.seasonPoints[profile.season] >= m_seasons[profile.season].pointsNeeded) {
            profile.unlocks = static_cast<u16>(profile.unlocks | m_seasons[profile.season].unlocks);
            ++profile.season;
            if (profile.season == SEASONS) {
                m_seasonMessage.emplace_back("ALL SEASONS COMPLETED!");
                // Winning every race of the last season adds the strip mine.
                bool perfect = true;
                for (int race = 0; race < m_seasons[SEASONS - 1].raceCount; ++race) {
                    perfect =
                        perfect &&
                        profile.seasonPlaces[SEASONS - 1][static_cast<std::size_t>(race)] == 0;
                }
                if (perfect && (profile.championshipFlags & 1)) {
                    if (!(profile.unlockedTracks & (1 << MIRROR_STRIP_MINE))) {
                        bonus("You Won all races this season!");
                        m_seasonMessage.emplace_back("Bonus: Mirror Mine Track Added!");
                        profile.unlockedTracks =
                            static_cast<u16>(profile.unlockedTracks | 1 << MIRROR_STRIP_MINE);
                    }
                } else if (perfect && !(profile.unlockedTracks & (1 << STRIP_MINE))) {
                    bonus("You Won all races this season!");
                    m_seasonMessage.emplace_back("Bonus: Mine Track Added!");
                    profile.unlockedTracks =
                        static_cast<u16>(profile.unlockedTracks | 1 << STRIP_MINE);
                }
                // Each championship of the years unlocks a car.
                constexpr std::array<std::pair<const char *, u16>, 5> YEAR_BONUSES{{
                    {"Bonus: Milk Truck Added!", 0x400},
                    {"Bonus: Helmet Car Added!", 0x200},
                    {"Bonus: Cupra Car Added!", 0x800},
                    {"Bonus: Beach Ball Car Added!", 0x1000},
                    {"Bonus: Mirrored Cars Added!", 0x8000},
                }};
                if (profile.championshipFlags >= 0 &&
                    profile.championshipFlags < static_cast<s32>(YEAR_BONUSES.size())) {
                    const auto &[line, unlock] =
                        YEAR_BONUSES[static_cast<std::size_t>(profile.championshipFlags)];
                    bonus(line);
                    profile.unlocks = static_cast<u16>(profile.unlocks | unlock);
                }
                profile.season = 0;
                ++profile.championshipFlags;
            } else {
                m_seasonMessage.emplace_back("SEASON COMPLETED!");
                m_seasonMessage.emplace_back();
                m_seasonMessage.emplace_back("PROGRESS TO THE NEXT SEASON");
            }
        } else {
            m_seasonMessage.emplace_back("Sorry, you don't have enough points to");
            m_seasonMessage.emplace_back("pass this season.  You'll have to try again.");
        }
        profile.seasonPoints[profile.season] = 0;
    }
    // The track and the weather of the next race become available.
    const MenuSeason &season = m_seasons[profile.season];
    int base = profile.championshipFlags & 1 ? MIRRORED_TRACKS : 0;
    profile.unlockedTracks =
        static_cast<u16>(profile.unlockedTracks | 1 << (season.races[profile.round][0] + base));
    profile.unlockedWeathers =
        static_cast<u16>(profile.unlockedWeathers | 1 << season.races[profile.round][1]);
}

void Menus::DrawRaceResultsTitle(SceneRenderer &renderer) {
    MenuScreen::DrawTexture(renderer, m_screen->TitleBar(), 22, 9, 280, 42);
    std::string title;
    if (m_resultsPage == RESULTS_PAGE_PLAYER_1 || m_resultsPage == RESULTS_PAGE_PLAYER_2) {
        // The track and the weather of the race, its name marked when it is mirrored.
        bool mirrored = m_resultsTrack >= MIRRORED_TRACKS;
        int track = mirrored ? m_resultsTrack - MIRRORED_TRACKS : m_resultsTrack;
        title = std::string("%ry") + (mirrored ? "M-" : "") +
                m_trackMenuItems[static_cast<std::size_t>(track)].name + "/" +
                m_weatherNames[static_cast<std::size_t>(m_resultsWeather)];
    } else {
        title = "%ry" + m_seasons[static_cast<std::size_t>(m_resultsSeason)].name;
    }
    TextStyle style;
    style.size = 30;
    style.align = TextStyle::Align::Center;
    style.colors = m_textCustomColors;
    m_screen->Text().Draw(
        renderer, title, WIDTH / 2, HEIGHT / 6 - 2, style, MenuScreen::FlatFor(renderer));
}

// DrawRaceResults() of the console: the place of a player and the times of the laps and the race.
void Menus::DrawPlayerRaceResults(SceneRenderer &renderer, int player) {
    const RacePlayerResult &result = m_results.players[static_cast<std::size_t>(player)];
    const MenuText &text = m_screen->Text();
    Matrix4 flat = MenuScreen::FlatFor(renderer);
    int totalLaps = m_results.totalLaps;
    char line[96];
    if (m_numLocalPlayers >= 2) {
        if (result.lap < totalLaps) {
            std::snprintf(line, sizeof(line), "%%ryP%d: Did not finish", player + 1);
        } else {
            std::snprintf(
                line, sizeof(line), "%%ryP%d: %s Place", player + 1, PlaceName(result.position));
        }
    } else if (m_gameMode == GameMode::Championship) {
        int place = m_profile.seasonPlaces[static_cast<std::size_t>(m_resultsSeason)]
                                          [static_cast<std::size_t>(m_resultsRound)];
        int points = PlacePoints(place);
        std::snprintf(line,
                      sizeof(line),
                      "%%ry%s Place - %d Point%s",
                      PlaceName(place),
                      points,
                      points == 1 ? "" : "s");
    } else if (result.lap < totalLaps) {
        std::snprintf(line, sizeof(line), "%%ryDid not finish");
    } else {
        std::snprintf(line, sizeof(line), "%%ry%s Place", PlaceName(result.position));
    }
    TextStyle style;
    style.size = 20;
    style.align = TextStyle::Align::Center;
    style.colors = m_textCustomColors;
    text.Draw(renderer, line, WIDTH / 2, HEIGHT * 19 / 64 - 4, style, flat);
    m_textCustomColors = std::array<int, 6>{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    style.colors = m_textCustomColors;
    style.size = 10;

    // A line for each lap, then the time of the race.
    auto track = static_cast<std::size_t>(m_resultsTrack);
    int y = 79;
    for (int lap = 0; lap <= totalLaps; ++lap, y += 10) {
        std::string label;
        std::string time;
        style.align = TextStyle::Align::Left;
        if (lap == totalLaps) {
            y += 2;
            if (result.totalTime == m_profile.raceRecords[track]) {
                text.Draw(renderer, "  %yyRECORD TIME!", WIDTH * 11 / 16, y, style, flat);
            }
            label = "Race time:";
            time = FormatLapTime(result.totalTime);
        } else {
            label = "Lap " + std::to_string(lap + 1) + " time:";
            time = lap < result.lap ?
                       FormatLapTime(result.lapTimes[static_cast<std::size_t>(lap)]) :
                       "--    ";
        }
        text.Draw(renderer, label, WIDTH * 5 / 16, y, style, flat);
        // RestoreRaceResultsFromSlots() does not restore the best lap time, and the time is the
        // one the instant replay arrived at. A replay exited before the lap ends shows BEST LAP
        // for a record, as on the console.
        if (lap < result.lap && lap == result.bestLap) {
            text.Draw(renderer,
                      result.bestLapTime == m_profile.lapRecords[track] ? "  %yyRECORD LAP!" :
                                                                          "  %yyBEST LAP!",
                      WIDTH * 11 / 16,
                      y,
                      style,
                      flat);
        }
        style.align = TextStyle::Align::Right;
        text.Draw(renderer, time, WIDTH * 11 / 16, y, style, flat);
    }
}

// DrawSeasonResults() of the console: the races of the season, their times, places, and points,
// and the total.
void Menus::DrawSeasonResults(SceneRenderer &renderer) {
    const MenuSeason &season = m_seasons[static_cast<std::size_t>(m_resultsSeason)];
    const MenuText &text = m_screen->Text();
    Matrix4 flat = MenuScreen::FlatFor(renderer);
    TextStyle style;
    style.size = 20;
    style.align = TextStyle::Align::Center;
    style.colors = m_textCustomColors;
    text.Draw(renderer, "%rySeason Results", WIDTH / 2, HEIGHT * 19 / 64 - 4, style, flat);
    m_textCustomColors = std::array<int, 6>{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    style.colors = m_textCustomColors;
    style.size = 10;

    // The columns move right when a name is of the longer, mirrored tracks.
    bool mirrored = (m_resultsChampionshipFlags & 1) != 0;
    int shift = 0;
    for (int race = 0; race < season.raceCount; ++race) {
        if ((season.races[static_cast<std::size_t>(race)][0] < MIRRORED_TRACKS) == mirrored) {
            shift = WIDTH / 16;
            break;
        }
    }
    const auto &places = m_profile.seasonPlaces[static_cast<std::size_t>(m_resultsSeason)];
    const auto &times = m_profile.seasonTimes[static_cast<std::size_t>(m_resultsSeason)];
    int total = 0;
    int y = 79;
    for (int race = 0; race < season.raceCount; ++race, y += 10) {
        const auto &entry = season.races[static_cast<std::size_t>(race)];
        int track = entry[0];
        if (mirrored) {
            track = track < MIRRORED_TRACKS ? track + MIRRORED_TRACKS : track - MIRRORED_TRACKS;
        }
        style.align = TextStyle::Align::Left;
        text.Draw(renderer,
                  m_trackMenuItems[static_cast<std::size_t>(track)].name + "/" +
                      m_weatherNames[static_cast<std::size_t>(entry[1])],
                  WIDTH * 3 / 16 - shift,
                  y,
                  style,
                  flat);
        style.align = TextStyle::Align::Center;
        bool run = race <= m_resultsRound;
        std::string time = run ? FormatLapTime(times[static_cast<std::size_t>(race)]) : "--";
        text.Draw(renderer, time, WIDTH * 17 / 32 + shift, y, style, flat);
        text.Draw(renderer,
                  run ? PlaceName(places[static_cast<std::size_t>(race)]) : "--",
                  WIDTH * 21 / 32 + shift,
                  y,
                  style,
                  flat);
        // A race not yet run shows the dashes of its time again.
        style.align = TextStyle::Align::Right;
        std::string points = time;
        if (run) {
            int scored = PlacePoints(places[static_cast<std::size_t>(race)]);
            points = std::to_string(scored) + " PTS";
            total += scored;
        }
        text.Draw(renderer, points, WIDTH * 13 / 16 + shift, y, style, flat);
    }
    y += 2;
    text.Draw(renderer,
              "TOTAL:    " + std::to_string(total) + " PTS ",
              WIDTH * 13 / 16 + shift,
              y,
              style,
              flat);
}

// DrawSeasonMessage() of the console.
void Menus::DrawSeasonMessage(SceneRenderer &renderer) {
    m_textCustomColors = std::array<int, 6>{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    TextStyle style;
    style.size = 12;
    style.align = TextStyle::Align::Center;
    style.colors = m_textCustomColors;
    int y = 98 - static_cast<int>(m_seasonMessage.size()) * 120 / 20;
    for (const std::string &line : m_seasonMessage) {
        m_screen->Text().Draw(renderer, line, WIDTH / 2, y, style, MenuScreen::FlatFor(renderer));
        y += 12;
    }
}

// HandleCarSelectInput() of the console in review mode: B exits, and A or Start turns the page,
// past the last page out of the screen.
void Menus::HandleRaceResultsInput(const MenuButtons &buttons) {
    if (buttons.back) {
        m_resultsDone = false;
        m_screen->FadeOut();
        m_screen->PlaySelectSound();
        return;
    }
    if (!buttons.choose) {
        return;
    }
    int previous = m_resultsPage;
    int page = previous + 1;
    if (page == RESULTS_PAGE_PLAYER_2 && m_numLocalPlayers != 2) {
        ++page;
    }
    if (page == RESULTS_PAGE_SEASON && m_gameMode != GameMode::Championship) {
        page = RESULTS_PAGE_END;
    }
    m_resultsPage = page;
    if (page == RESULTS_PAGE_MESSAGE) {
        ProcessSeasonResults();
        m_seasonResultsShown = true;
        if (m_seasonMessage.empty()) {
            ++m_resultsPage;
        }
    }
    if (m_resultsPage == RESULTS_PAGE_END) {
        // Every player is ready, and the last page stays in view as the screen fades.
        for (CarSelectPlayer &player : m_carSelect) {
            player.state = CAR_SELECT_READY;
        }
        m_resultsPage = previous;
        m_resultsDone = true;
        m_screen->FadeOut();
    }
    m_screen->PlaySelectSound();
}

// LeaveCarSelect() of the console in review mode.
void Menus::LeaveRaceResults() {
    m_resultsInitialized = false;
    MenuScreen::SilenceSounds();
    if (!m_resultsDone) {
        if (m_gameMode == GameMode::Championship && !m_seasonResultsShown) {
            ProcessSeasonResults();
        }
        SetCurrentScreen(Screen::MainMenu);
        return;
    }
    if (m_gameMode != GameMode::Championship) {
        // ponytail: the data save and load screen, where a time attack saves its ghost, is not
        // built yet.
        SetCurrentScreen(Screen::MainMenu);
        return;
    }
    // The next race of the season, or after its last race, the race after the season.
    m_numLocalPlayers = 1;
    m_continueFromResults = m_profile.round != 0;
    if (m_resultsSeason == m_profile.season) {
        // ponytail: the data save and load screen is not built yet, and the track menu follows.
        SetTrackFromSeason();
        SetCurrentScreen(Screen::TrackMenu);
    } else {
        m_afterSeasonRace = true;
        m_end = MenusEnd::Race;
    }
}

void Menus::ProcessRaceResultsScreen(SceneRenderer &renderer,
                                     float seconds,
                                     const MenuButtons &first) {
    if (!m_resultsInitialized) {
        StartRaceResults(renderer);
    }
    m_screen->UpdateFade(seconds);
    m_screen->DrawBackground(renderer, {0x00, 0x00, 0x00}, {0x00, 0x82, 0x8C});

    // The stages turn, the second the other way.
    m_carSelectAngle[0] += seconds;
    m_carSelectAngle[1] += TWO_PI - seconds;
    for (float &angle : m_carSelectAngle) {
        while (TWO_PI < angle) {
            angle -= TWO_PI;
        }
    }
    DrawPlayerStage(
        renderer, m_resultsPage == RESULTS_PAGE_PLAYER_2 ? 1 : 0, 59, HEIGHT, SPREAD_TWO_VIEWS);
    DrawRaceResultsTitle(renderer);
    if (m_resultsPage == RESULTS_PAGE_PLAYER_1 || m_resultsPage == RESULTS_PAGE_PLAYER_2) {
        DrawPlayerRaceResults(renderer, m_resultsPage == RESULTS_PAGE_PLAYER_2 ? 1 : 0);
    } else if (m_resultsPage == RESULTS_PAGE_MESSAGE) {
        DrawSeasonMessage(renderer);
    } else {
        DrawSeasonResults(renderer);
    }
    m_screen->DrawPauseFooter(renderer);
    m_screen->DrawFade(renderer);

    if (m_screen->FadingOut()) {
        if (m_screen->FadeComplete()) {
            LeaveRaceResults();
        }
        return;
    }
    HandleRaceResultsInput(first);
}

} // namespace coastline
