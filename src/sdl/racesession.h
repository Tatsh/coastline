#pragma once

/**
 * @file
 * A race of the menus of the Nintendo 64 release, run by the race engine of the PC release: the
 * choices of the menus set as the front end of the PC release sets them, and the results of the
 * race read back for the results screen of the console.
 */

#include <array>
#include <filesystem>
#include <vector>

#include "common/types.h"
#include "raceview.h"

namespace coastline {

/** The tracks of the records of a profile of the PC release. */
constexpr int PC_RECORD_TRACKS = 14;

/** The most laps of a race the results record. */
constexpr int RESULT_LAPS = 12;

/** The car and the setup of a player. */
struct RacePlayerSetup {
    int car = 0;          /*!< The car, an index of g_apszCarModelPrefixes. */
    int handling = 0;     /*!< The handling, the steering mode from 0. */
    int transmission = 1; /*!< The transmission, nonzero for the automatic. */
    int tires = 2;        /*!< The tyres. */
    int suspension = 1;   /*!< The suspension, from 0 for the softest springs. */
};

/** What a race is, in the terms of the race engine. */
struct RaceSetup {
    int gameMode = 0;                      /*!< The mode, a GAME_MODE_ value. */
    int players = 1;                       /*!< The number of local players. */
    int track = 2;                         /*!< The track, an index of g_apszTrackFilenames. */
    int weather = 0;                       /*!< The weather, a WEATHER_ value. */
    int laps = 3;                          /*!< The laps of an arcade race. */
    std::array<RacePlayerSetup, 2> cars{}; /*!< The car of each player. */
    int season = 0;                        /*!< The season of the championship. */
    int round = 0;                         /*!< The race of the season. */
    s32 championshipFlags = 0; /*!< The progress of the championship across its years. */
    /** The best lap on each track of the engine, or 0 for none. */
    std::array<f32, PC_RECORD_TRACKS> lapRecords{};
    /** The best race on each track of the engine, or 0 for none. */
    std::array<f32, PC_RECORD_TRACKS> raceRecords{};
    /** Whether the race waits for the start binding before the countdown, as the PC release does.
     */
    bool waitForStart = false;
    /** Whether the computer drives the car of the first player, for unattended checks. */
    bool autopilot = false;
};

/** How a player finished a race. */
struct RacePlayerResult {
    int lap = 0;                             /*!< The laps completed. */
    int position = 0;                        /*!< The place, from 0 for the winner. */
    int bestLap = 0;                         /*!< The best lap, from 0. */
    f32 totalTime = 0.0f;                    /*!< The time of the race, in seconds. */
    f32 bestLapTime = 0.0f;                  /*!< The time of the best lap, in seconds. */
    std::array<f32, RESULT_LAPS> lapTimes{}; /*!< The time of each lap, in seconds. */
};

/** The results of a race, as the race engine records them. */
struct RaceResults {
    int totalLaps = 0;                               /*!< The laps of the race. */
    std::array<RacePlayerResult, 2> players{};       /*!< The result of each player. */
    std::array<f32, PC_RECORD_TRACKS> lapRecords{};  /*!< The best laps after the race. */
    std::array<f32, PC_RECORD_TRACKS> raceRecords{}; /*!< The best races after the race. */
};

/** How a race ends. */
enum class RaceEnd {
    Running,    /*!< The race goes on. */
    Results,    /*!< The race finished with results, for the results screen. */
    Menu,       /*!< The race was abandoned or had no results, for the main menu. */
    SeasonMenu, /*!< The race after a season ended, for the next season of the championship. */
};

/**
 * Start a race as the front end of the PC release starts one. The race music starts with it. The
 * working directory is the directory of the default assets, and OpenRaceData() was called.
 *
 * @param setup The race.
 * @param music The directory of the music files.
 * @param cdAudio Whether the music is the CD audio of the PC release rather than the modules of
 * the Nintendo 64 release.
 */
void StartRace(const RaceSetup &setup, const std::filesystem::path &music, bool cdAudio);

/**
 * Run one frame of the race, a 30th of a second. The race music stops when the race ends.
 *
 * @return How the race ends, or RaceEnd::Running while it goes on.
 */
RaceEnd StepRace();

/** End the race that StartRace() started without running it, and stop its music. */
void AbandonRace();

/** @return The results of the race that ended last with RaceEnd::Results. */
const RaceResults &LastRaceResults();

/**
 * @return The track of the race that loaded last, an index of g_apszTrackFilenames: the track of
 * the race after a season in its mode.
 */
int RaceTrack();

/**
 * @return The car of each vehicle of the race that loaded last, in the colours of its images as
 * the North American release draws the cars of a race.
 */
std::vector<RaceCar> RaceCars();

} // namespace coastline
