#pragma once

#include <cstddef>
#include <cstdint>

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

} // namespace coastline
