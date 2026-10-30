#include "racesession.h"

#include <algorithm>

#include <SDL3/SDL.h>

#include "common/fade.h"
#include "common/game.h"
#include "common/ghost.h"
#include "common/instantreplay.h"
#include "common/laps.h"
#include "common/profile.h"
#include "common/race.h"
#include "common/racephases.h"
#include "common/rallyconfig.h"
#include "common/render.h"
#include "common/screen.h"
#include "common/season.h"
#include "common/settings.h"
#include "common/soundsystem.h"
#include "common/system.h"
#include "common/trackselect.h"
#include "common/vehicle.h"
#include "common/vehiclecount.h"
#include "common/weather.h"
#include "racemusic.h"

namespace coastline {

namespace {

RaceResults g_results;
bool g_musicStarted = false;

void StopMusicOfRace() {
    if (g_musicStarted) {
        StopRaceMusic();
        g_musicStarted = false;
    }
}

// SaveRaceResultsToSlots() and RestoreRaceResultsFromSlots() store the results of the race in the
// vehicles of the players, and the races and the laps of record in the profile of the first.
void ReadResults() {
    g_results = {};
    g_results.totalLaps = g_dwTotalLaps;
    for (int i = 0; i < std::min(static_cast<int>(g_dwNumLocalPlayers), 2); ++i) {
        const Vehicle &vehicle = g_aVehicles[i];
        RacePlayerResult &result = g_results.players[static_cast<std::size_t>(i)];
        result.lap = vehicle.GetLap();
        result.position = vehicle.GetRacePosition();
        result.bestLap = vehicle.GetBestLap();
        result.totalTime = vehicle.GetTotalTime();
        result.bestLapTime = vehicle.GetBestLapTime();
        std::copy_n(vehicle.GetLapTimes(), VEHICLE_LAP_TIMES, result.lapTimes.begin());
    }
    const PlayerProfile *profile = g_aVehicles[0].GetProfile();
    std::copy_n(profile->lapRecords, PC_RECORD_TRACKS, g_results.lapRecords.begin());
    std::copy_n(profile->raceRecords, PC_RECORD_TRACKS, g_results.raceRecords.begin());
}

bool g_waitForStart = false;
bool g_autopilot = false;
bool g_soundHeld = false;

// A race that has loaded is silent until the screen has wiped in from black, and its music starts
// from the beginning of its track then.
void HoldSoundUntilShown() {
    if (!g_soundHeld) {
        return;
    }
    if (g_fFadeLevel < 1.0f) {
        g_bSfxVolume = 0;
        return;
    }
    g_soundHeld = false;
    g_bSfxVolume =
        static_cast<unsigned char>(g_adwSfxVolumeLevels[g_GlobalRallyConfig.dwSfxVolume]);
    if (g_musicStarted) {
        RestartRaceMusic();
    }
}

} // namespace

void StartRace(const RaceSetup &setup, const std::filesystem::path &music, bool cdAudio) {
    // The choices of the front end of the PC release that InitGameAndShowMenus() reads.
    g_dwGameMode = setup.gameMode;
    g_dwRaceSessionType = RACE_SESSION_ATTRACT;
    g_dwRaceInitialized = 0;
    g_dwChosenCar = setup.cars[0].car;
    g_dwChosenWeather = setup.weather;
    g_dwHandlingType = setup.cars[0].handling;
    g_dwTransmissionType = setup.cars[0].transmission;
    g_dwTireType = setup.cars[0].tires;
    g_dwSuspensionType = setup.cars[0].suspension;
    g_dwSelectedTrackId = static_cast<u32>(setup.track);
    g_dwTotalLaps = setup.laps;
    // LoadConfigData() copies the season of the front end into the profile of each player. The
    // computer's cars of the championship drive by its season and its race.
    g_SeasonProgress.season = static_cast<u8>(setup.season);
    g_SeasonProgress.round = static_cast<u8>(setup.round);
    g_SeasonProgress.championshipFlags = setup.championshipFlags;
    std::copy(setup.lapRecords.begin(), setup.lapRecords.end(), g_SeasonProgress.lapRecords);
    std::copy(setup.raceRecords.begin(), setup.raceRecords.end(), g_SeasonProgress.raceRecords);
    // The options of the PC release choose the keyboard or the joystick, and the menus of the
    // console have no such choice. A connected gamepad is the joystick of those options.
    constexpr int KEYBOARD_DEVICE = 0;
    constexpr int JOYSTICK_DEVICE = 2;
    g_GlobalRallyConfig.dwInputDeviceType = SDL_HasGamepad() ? JOYSTICK_DEVICE : KEYBOARD_DEVICE;
    g_GlobalRallyConfig.SelectActiveInputBindings();
    InitGameAndShowMenus();
    // The PC release races one player, and a championship of the PC release takes its track and
    // its weather from the seasons of the PC release. The menus of the console chose both.
    g_dwNumLocalPlayers = static_cast<u32>(setup.players);
    g_dwSelectedTrackId = static_cast<u32>(setup.track);
    g_dwCurrentWeather = static_cast<u32>(setup.weather);
    if (setup.players > 1) {
        const RacePlayerSetup &second = setup.cars[1];
        g_aVehicles[1].SetVehicleCarId(second.car);
        PlayerProfile *profile = g_aVehicles[1].GetProfile();
        profile->handling = second.handling;
        profile->transmission = second.transmission;
        profile->tires = second.tires;
        profile->suspension = second.suspension;
    }
    g_musicStarted =
        StartRaceMusic(music, cdAudio ? RaceMusicSource::CdAudio : RaceMusicSource::Modules);
    g_waitForStart = setup.waitForStart;
    g_bRaceStartsItself = !setup.waitForStart;
    g_autopilot = setup.autopilot;
    // The first frame loads the race.
    StepRace();
    // A player with custom decals races in the colour of their paint.
    for (int i = 0; i < setup.players; ++i) {
        const auto &color = setup.cars[static_cast<std::size_t>(i)].color;
        if (color) {
            g_aVehicles[i].SetVehiclePaintColorRgb((*color)[0], (*color)[1], (*color)[2]);
        }
    }
}

RaceEnd StepRace() {
    // The front end of the PC release processes the season and returns to its menus. The menus of
    // the console process the season on their results screen, from the results of the race.
    if (IsCurrentScreen(ExitRaceAndProcessSeasonResults)) {
        ReadResults();
        ShutdownGameSession();
        SetCurrentScreenCallback(ExitRaceSessionToMenu);
        StopMusicOfRace();
        return RaceEnd::Results;
    }
    if (IsCurrentScreen(ShutdownGameSessionToSeasonMenu) ||
        IsCurrentScreen(ExitRaceSessionToMenu)) {
        bool season = IsCurrentScreen(ShutdownGameSessionToSeasonMenu) != 0;
        CallCurrentGameMode();
        SetCurrentScreenCallback(ExitRaceSessionToMenu);
        StopMusicOfRace();
        return season ? RaceEnd::SeasonMenu : RaceEnd::Menu;
    }
    // ponytail: the credits after the last season are a race of the recording of the PC release,
    // and the game returns to the main menu in their place.
    if (g_dwGameMode == GAME_MODE_DEMO) {
        SDL_Log("The ending credits are not built yet.");
        ShutdownGameSession();
        SetCurrentScreenCallback(ExitRaceSessionToMenu);
        StopMusicOfRace();
        return RaceEnd::Menu;
    }
    bool loading = !g_dwRaceInitialized;
    CallCurrentGameMode();
    // The PC release lays out the views with each frame it draws, and the race lays them out at
    // its load before it knows how many players race. The SDL game lays them out again once the
    // race has loaded.
    if (loading && g_dwRaceInitialized) {
        StartLowResFrame(g_aViewportRects);
    }
    if (loading && g_dwRaceInitialized && !gInstantReplayActive) {
        // The console counts down as soon as the race has loaded, where the PC release waits for
        // the start binding.
        if (!g_waitForStart && !g_bRaceStartReached) {
            g_bRaceStartReached = 1;
            if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
                StartGhostRecording();
            }
        }
        if (g_autopilot) {
            g_aVehicles[0].SetDriver(AiVehicleControllerCallback);
        }
        g_soundHeld = true;
        PauseMusicPlayback();
    }
    // The PC release advances the fades with each frame it draws, and the SDL game with each frame
    // of the race.
    UpdateFadeTransitions();
    HoldSoundUntilShown();
    return RaceEnd::Running;
}

void AbandonRace() {
    g_dwRaceInitialized = 0;
    ShutdownGameSession();
    SetCurrentScreenCallback(ExitRaceSessionToMenu);
    StopMusicOfRace();
}

const RaceResults &LastRaceResults() {
    return g_results;
}

int RaceTrack() {
    // AFTER_SEASON_TRACK of InitRace(), gamewin.trk.
    constexpr int AFTER_SEASON_TRACK = 12;
    return g_dwGameMode == GAME_MODE_AFTER_SEASON ? AFTER_SEASON_TRACK :
                                                    static_cast<int>(g_dwSelectedTrackId);
}

std::vector<RaceCar> RaceCars() {
    std::vector<RaceCar> cars;
    for (int i = 0; i < g_dwNumActiveVehicles; ++i) {
        const auto &color = g_aVehicles[i].GetColor();
        cars.push_back(
            {g_aVehicles[i].GetCarModel(), {color[0], color[1], color[2]}, std::nullopt});
    }
    return cars;
}

} // namespace coastline
