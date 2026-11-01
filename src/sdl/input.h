#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <SDL3/SDL.h>

#include "controller.h"
#include "controls.h"
#include "raceinput.h"

namespace coastline {

/** The devices that drive one player. */
struct PlayerDevices {
    bool keyboard = false; /*!< Whether the keyboard drives the player. */
    /** Borrowed gamepads, valid until disconnection or InputBindings destruction. */
    std::vector<SDL_Gamepad *> gamepads;
};

/**
 * Open every gamepad in the order it connects, and drive the first two ports with the devices the
 * Player1 and Player2 keys of the Controls section choose. One instance exists at a time.
 */
class InputBindings {
public:
    /** Open connected gamepads and register default controls for the active input context. */
    InputBindings();
    /**
     * Open connected gamepads and register configured controls for the active input context.
     *
     * @param config The initial keyboard, gamepad, and player-assignment settings.
     */
    explicit InputBindings(const GameConfig &config);
    /** Close gamepads and unregister this input context. */
    ~InputBindings();
    /** Copying is disabled because SDL events address one active input context. */
    InputBindings(const InputBindings &) = delete;
    /** Assignment is disabled because the active input context has a stable address. */
    InputBindings &operator=(const InputBindings &) = delete;

    /** @return The active input owner registered with SDL, or null outside its lifetime. */
    static InputBindings *Active();

    /** @return The input owner's configured bindings. */
    ControlBindings &Controls() {
        return m_controls;
    }
    /** @return The input owner's configured bindings. */
    const ControlBindings &Controls() const {
        return m_controls;
    }

    /** @copydoc coastline::AssignedDevices */
    PlayerDevices AssignedDevices(std::size_t player, std::size_t players) const;
    /** @copydoc coastline::SetScriptedKeys */
    void SetScriptedKeys(std::vector<int> scancodes);
    /** @copydoc coastline::IsKeyDown */
    bool IsKeyDown(SDL_Scancode scancode);
    /** @copydoc coastline::IgnoreKeyUntilReleased */
    void IgnoreKeyUntilReleased(SDL_Scancode scancode);
    /** @copydoc coastline::SetSecondControllerState */
    void SetSecondControllerState(const ControllerState &state);
    /** @return The latest sample of the second controller. */
    ControllerState SecondControllerState() const;
    /** @copydoc coastline::TakeMouseWheel */
    int TakeMouseWheel();
    /** Discard accumulated mouse-wheel movement. */
    void ClearMouseWheel();

    /**
     * Open or close a gamepad for a connection event, and log the devices of the players. A wheel
     * event of the mouse adds to the wheel TakeMouseWheel() reads. Other events are ignored.
     *
     * @param event The SDL event.
     */
    void HandleEvent(const SDL_Event &event);

    /**
     * @param players The number of players the screen reads, 1 or 2. In single-player mode,
     * devices assigned to either player drive the first port.
     * @param race Whether a race reads the second port, through the race actions of the Controls
     * section for a gamepad and the keyboard bindings of the PC release for the keyboard.
     * @return The state of every port, read now. The second port is connected whenever a device
     * would drive the second of two players.
     */
    std::array<ControllerState, CONTROLLER_PORTS> Poll(std::size_t players, bool race) const;

private:
    bool OpenGamepad(SDL_JoystickID id);
    ControlBindings m_controls;
    std::vector<int> m_scriptedKeys;
    std::vector<int> m_ignoredKeys;
    ControllerState m_secondController;
    std::vector<SDL_Gamepad *> m_gamepads;
    float m_mouseWheel = 0.0f;
    RaceInputContext m_raceInput;
};

/**
 * Resolve the Player1 and Player2 keys of the Controls section against the open gamepads. A key
 * that is `auto` takes the gamepads no other key chooses, one each in the order of the players
 * and the rest by the first player that is `auto`, and the keyboard goes to the first player that
 * is `auto` with no gamepad, else to the first player that is `auto`.
 *
 * @param player The player, 0 for the first.
 * @param players The number of players, 1 or 2. One player takes the devices of both.
 * @return The devices of the player.
 */
PlayerDevices AssignedDevices(std::size_t player, std::size_t players);

/**
 * Read the second player of a race as the console reads a pad: the race actions of the Controls
 * section for each gamepad, and the keyboard bindings of the PC release for the keyboard.
 *
 * @param devices The devices of the player.
 * @return The state of the pad, connected.
 */
ControllerState ReadRacePad(const PlayerDevices &devices);

/**
 * Scale an SDL axis of -32768 to 32767 to the range of the controller stick. The axis has passed
 * through the dead zone of ReadControl().
 *
 * @param axis The SDL axis value.
 * @return The stick value from -80 to 80, rounded towards zero.
 */
std::int8_t ScaleStickAxis(std::int16_t axis);

/**
 * Add simulated key presses alongside physical keyboard input, for tests.
 *
 * @param scancodes The SDL scancodes to report as down, replacing the previous list.
 */
void SetScriptedKeys(std::vector<int> scancodes);

/**
 * @param scancode The key.
 * @return Whether the key is down on the keyboard or reported by SetScriptedKeys(), and not
 * ignored by IgnoreKeyUntilReleased(). Without active bindings, only physical input is read.
 */
bool IsKeyDown(SDL_Scancode scancode);

/**
 * Report a key as up to IsKeyDown() until it is released, for a key a shortcut of the window
 * consumed, such as Enter of Alt and Enter.
 *
 * @param scancode The key.
 */
void IgnoreKeyUntilReleased(SDL_Scancode scancode);

/**
 * Set the state of the second controller the race reads for its second player, until the next
 * call.
 *
 * @param state The state of the second port.
 */
void SetSecondControllerState(const ControllerState &state);

/**
 * @return The wheel of the mouse scrolled since the last call, in the units of the third axis of a
 * DirectInput mouse (120 a notch, positive away from the user). A fraction of a unit is retained
 * for the next call.
 */
int TakeMouseWheel();

/** Discard the wheel of the mouse scrolled since TakeMouseWheel() last read it. */
void ClearMouseWheel();

} // namespace coastline
