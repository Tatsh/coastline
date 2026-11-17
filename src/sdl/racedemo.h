#pragma once

/**
 * @file
 * The attract demo of the PC release, run by its race engine: a recording of a car on a track,
 * replayed for 39.6 seconds under the cameras of the race. The demos of the imported cartridges
 * follow the two of the PC release.
 */

#include <array>
#include <cstdint>
#include <optional>
#include <string>

#include "racecredits.h"

namespace coastline {

/**
 * Set the default configuration, the bindings, and the strings of the race, and open its sound, as
 * the PC release does once at its start. The models of BossRally.pod load from its import.
 * The working directory is the directory of the default assets.
 *
 * @param playSounds Whether the race plays its sound effects.
 */
void OpenRaceData(bool playSounds);

/** Attract-demo rotation for one front end. One instance exists at a time. */
class AttractDemo {
public:
    /** Register the exclusive attract-demo controller with the first recording selected next. */
    AttractDemo();
    /** Unregister the controller after its active race has shut down. */
    ~AttractDemo();
    /** Prevent copying the registered demo controller. */
    AttractDemo(const AttractDemo &) = delete;
    /**
     * Prevent assignment of the registered demo controller.
     *
     * @return This controller; the operator is deleted and never called.
     */
    AttractDemo &operator=(const AttractDemo &) = delete;

    /**
     * Find the registered controller, for the race engine's callbacks.
     *
     * @return The active demo controller, or null outside its lifetime.
     */
    static AttractDemo *Active();

    /**
     * Give mutable access to the credits of the demo race.
     *
     * @return Credits displayed over the demo controller's race.
     */
    ConsoleCredits &Credits() {
        return m_credits;
    }
    /**
     * Give read-only access to the credits of the demo race.
     *
     * @return Credits displayed over the demo controller's race.
     */
    const ConsoleCredits &Credits() const {
        return m_credits;
    }

    /**
     * Select the next recording in rotation. The rotation lists the two PC demos, then the
     * cartridge demo of the release directory's region when its `attract.json` exists.
     *
     * @return The next recording path, valid until the next selection or destruction.
     */
    const char *NextRecording();

    /**
     * Start the attract demo as the root menu of the PC release starts it, on the recording after
     * the one it played last. The working directory is the directory of the default assets.
     */
    void Start();

    /**
     * Start the race of the credits, on the recording of the credits of the PC release, as
     * StartCreditsRace() of the console starts the captions from the options menu and
     * StartEndingCredits() starts the scrolling credits after the last season. The console scrolls
     * the credits of the options menu instead under the multiplayer mode of a cheat, absent from
     * the SDL game. Step() runs the race. The working directory is the directory of the default
     * assets.
     *
     * @param scrolling Whether the credits scroll rather than show as captions.
     */
    void StartCredits(bool scrolling);

    /**
     * Run one frame of the race of the demo, a 30th of a second.
     *
     * @param leave Whether a button that leaves the demo was pressed, as the pause binding of the
     * PC release leaves it.
     * @return Whether the demo continues, or false after its time expires or a button ends the
     * demo.
     */
    bool Step(bool leave);

    /**
     * Report whether the demo ended early, read from `g_bDemoInterrupted`.
     *
     * @return Whether a button ended the last demo before its time expired.
     */
    bool Interrupted() const;

    /**
     * Report the track of the demo race, read from `g_dwSelectedTrackId`.
     *
     * @return The track of the demo, an index of g_apszTrackFilenames, once its race has loaded.
     */
    int Track() const;

    /**
     * Report the car of the demo recording, read from `g_GhostRecordingInfo`.
     *
     * @return The car of the demo, an index of g_apszCarModelPrefixes, once its race has loaded.
     */
    int Car() const;

    /**
     * Report the body colour of the first vehicle of the demo race.
     *
     * @return The red, green, and blue of the colour of the car of the demo, once its race has
     * loaded.
     */
    std::array<std::uint8_t, 3> Color() const;

    /**
     * Report whether NextRecording() last selected a cartridge demo.
     *
     * @return Whether the demo is the one of a cartridge, recorded at its import. The console plays
     * it in fog with no help on the screen and ends it with its recording.
     */
    bool FromConsole() const;

private:
    std::size_t m_nextDemo = 0; // The rotation index of the recording selected next.
    std::string m_demoPath;     // The path NextRecording() returned last.
    // The index of the selected cartridge demo, absent for a PC demo.
    std::optional<std::size_t> m_consoleDemo;
    ConsoleCredits m_credits; // The captions or scrolling credits of the credits race.
};

} // namespace coastline
