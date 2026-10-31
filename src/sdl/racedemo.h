#pragma once

/**
 * @file
 * The attract demo of the PC release, run by its race engine: a recording of a car on a track,
 * replayed for 39.6 seconds under the cameras of the race. The demos of the imported cartridges
 * follow the two of the PC release.
 */

#include <array>
#include <cstdint>
#include <string_view>

namespace coastline {

/**
 * Set the default configuration, the bindings, and the strings of the race, and open its sound, as
 * the PC release does once at its start. The models of BossRally.pod load from its import.
 * The working directory is the directory of the default assets.
 *
 * @param playSounds Whether the race plays its sound effects.
 */
void OpenRaceData(bool playSounds);

/**
 * Start the attract demo as the root menu of the PC release starts it, on the recording after
 * the one it played last. The working directory is the directory of the default assets.
 */
void StartAttractDemo();

/**
 * Start the race of the credits, on the recording of the credits of the PC release, as
 * StartCreditsRace() of the console starts the captions from the options menu and
 * StartEndingCredits() starts the scrolling credits after the last season. The console scrolls the
 * credits of the options menu instead under the multiplayer mode of a cheat, absent from the SDL
 * game. StepAttractDemo() runs the race. The working directory is the directory of the default
 * assets.
 *
 * @param scrolling Whether the credits scroll rather than show as captions.
 */
void StartCreditsRace(bool scrolling);

/**
 * Run one frame of the race of the demo, a 30th of a second.
 *
 * @param leave Whether a button that leaves the demo was pressed, as the pause binding of the PC
 * release leaves it.
 * @return Whether the demo goes on, or false once it has ended, by its time or by a press of the
 * button that leaves it.
 */
bool StepAttractDemo(bool leave);

/** @return Whether the demo that ended last was left by a button rather than by its time. */
bool AttractDemoInterrupted();

/** @return The track of the demo, an index of g_apszTrackFilenames, once its race has loaded. */
int AttractDemoTrack();

/** @return The car of the demo, an index of g_apszCarModelPrefixes, once its race has loaded. */
int AttractDemoCar();

/** @return The red, green, and blue of the colour of the car of the demo, once its race has loaded.
 */
std::array<std::uint8_t, 3> AttractDemoColor();

/**
 * @return Whether the demo is the one of a cartridge, recorded at its import. The console plays it
 * in fog with no help on the screen and ends it with its recording.
 */
bool AttractDemoFromConsole();

} // namespace coastline
