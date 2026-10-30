#pragma once

/**
 * @file
 * The attract demo of the PC release, run by its race engine: a recording of a car on a track,
 * replayed for 39.6 seconds under the cameras of the race.
 */

namespace coastline {

/**
 * Open BossRally.pod, the archive the race loads models from, as OpenBossRallyPodAndInit() of the
 * PC release opens it once at its start. The working directory is the directory of the default
 * assets.
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
 * @return Whether the demo goes on, or false once it has ended, by its time or by a press of the
 * button that leaves it.
 */
bool StepAttractDemo();

} // namespace coastline
