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
    AttractDemo();
    ~AttractDemo();
    AttractDemo(const AttractDemo &) = delete;
    AttractDemo &operator=(const AttractDemo &) = delete;

    /** @return The active demo controller, or null outside its lifetime. */
    static AttractDemo *Active();

    /** @return Credits displayed over the demo controller's race. */
    ConsoleCredits &Credits() {
        return m_credits;
    }
    /** @return Credits displayed over the demo controller's race. */
    const ConsoleCredits &Credits() const {
        return m_credits;
    }

    /** @return The next recording path, valid until the next selection or destruction. */
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

    /** @return Whether a button ended the last demo before its time expired. */
    bool Interrupted() const;

    /** @return The track of the demo, an index of g_apszTrackFilenames, once its race has loaded.
     */
    int Track() const;

    /** @return The car of the demo, an index of g_apszCarModelPrefixes, once its race has loaded.
     */
    int Car() const;

    /** @return The red, green, and blue of the colour of the car of the demo, once its race has
     * loaded.
     */
    std::array<std::uint8_t, 3> Color() const;

    /**
     * @return Whether the demo is the one of a cartridge, recorded at its import. The console plays
     * it in fog with no help on the screen and ends it with its recording.
     */
    bool FromConsole() const;

private:
    std::size_t m_nextDemo = 0;
    std::string m_demoPath;
    std::optional<std::size_t> m_consoleDemo;
    ConsoleCredits m_credits;
};

} // namespace coastline
