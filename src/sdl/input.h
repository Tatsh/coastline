#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <SDL3/SDL.h>

namespace coastline {

/** The buttons of a Nintendo 64 controller, with the bits libultra reports. */
enum ControllerButton : std::uint16_t {
    BUTTON_C_RIGHT = 0x0001, /*!< C right. */
    BUTTON_C_LEFT = 0x0002,  /*!< C left. */
    BUTTON_C_DOWN = 0x0004,  /*!< C down. */
    BUTTON_C_UP = 0x0008,    /*!< C up. */
    BUTTON_R = 0x0010,       /*!< The right shoulder. */
    BUTTON_L = 0x0020,       /*!< The left shoulder. */
    BUTTON_D_RIGHT = 0x0100, /*!< The directional pad right. */
    BUTTON_D_LEFT = 0x0200,  /*!< The directional pad left. */
    BUTTON_D_DOWN = 0x0400,  /*!< The directional pad down. */
    BUTTON_D_UP = 0x0800,    /*!< The directional pad up. */
    BUTTON_START = 0x1000,   /*!< Start. */
    BUTTON_Z = 0x2000,       /*!< The Z trigger. */
    BUTTON_B = 0x4000,       /*!< B. */
    BUTTON_A = 0x8000,       /*!< A. */
};

/** The state of one controller port, as libultra reports a pad. */
struct ControllerState {
    std::uint16_t buttons = 0; /*!< The ControllerButton bits held. */
    std::int8_t stickX = 0;    /*!< The stick, right positive, about -80 to 80. */
    std::int8_t stickY = 0;    /*!< The stick, up positive, about -80 to 80. */
    bool connected = false;    /*!< Whether a device is bound to the port. */
};

/** The number of controller ports of the console. */
inline constexpr std::size_t CONTROLLER_PORTS = 4;

/**
 * Bind gamepads and the keyboard to the four ports without configuration. Each gamepad takes the
 * first free port in the order it connects, and the keyboard always drives the first port.
 */
class InputBindings {
public:
    InputBindings();
    ~InputBindings();
    InputBindings(const InputBindings &) = delete;
    InputBindings &operator=(const InputBindings &) = delete;

    /**
     * Open or close a gamepad for a connection event. Other events are ignored.
     *
     * @param event The SDL event.
     */
    void HandleEvent(const SDL_Event &event);

    /** @return The state of every port, read now. */
    std::array<ControllerState, CONTROLLER_PORTS> Poll() const;

private:
    std::array<SDL_Gamepad *, CONTROLLER_PORTS> m_gamepads{};
};

/**
 * Scale an SDL axis of -32768 to 32767 to the range of the controller stick, with a dead zone
 * around the centre.
 *
 * @param axis The SDL axis value.
 * @return The stick value.
 */
std::int8_t ScaleStickAxis(std::int16_t axis);

/**
 * Hold keys down as if the player held them, in addition to the keyboard, for tests.
 *
 * @param scancodes The SDL scancodes of the keys held, replacing those of the last call.
 */
void SetScriptedKeys(std::vector<int> scancodes);

/**
 * @param scancode The key.
 * @return Whether the key is down on the keyboard or held by SetScriptedKeys().
 */
bool IsKeyDown(SDL_Scancode scancode);

} // namespace coastline
