#pragma once

/**
 * @file
 * The attract demo of the PC release, run by its race engine: a recording of a car on a track,
 * replayed for 39.6 seconds under the cameras of the race.
 */

namespace coastline {

/**
 * Set the default configuration and bindings of the race, and open BossRally.pod, the archive the
 * race loads models from, as the PC release does once at its start. The working directory is the
 * directory of the default assets.
 */
void OpenRaceData();

/**
 * Start the attract demo as the root menu of the PC release starts it, on the recording after
 * the one it played last. The working directory is the directory of the default assets.
 */
void StartAttractDemo();

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

} // namespace coastline
