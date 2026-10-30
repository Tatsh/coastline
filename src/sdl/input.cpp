#include "input.h"

#include <algorithm>
#include <cmath>

#include "common/inputpoll.h"
#include "controls.h"

namespace coastline {

namespace {

// A real stick reads about 80 at its gate. The dead zone hides the drift of worn pads.
constexpr int STICK_RANGE = 80;
constexpr int DEAD_ZONE = 7849;
constexpr int C_BUTTON_THRESHOLD = 16384;

std::vector<int> g_scriptedKeys;
ControllerState g_secondController;

struct KeyBinding {
    SDL_Scancode key;
    std::uint16_t button;
};

constexpr KeyBinding KEY_BUTTONS[] = {
    {SDL_SCANCODE_X, BUTTON_A},
    {SDL_SCANCODE_C, BUTTON_B},
    {SDL_SCANCODE_Z, BUTTON_Z},
    {SDL_SCANCODE_RETURN, BUTTON_START},
    {SDL_SCANCODE_A, BUTTON_L},
    {SDL_SCANCODE_S, BUTTON_R},
    {SDL_SCANCODE_I, BUTTON_C_UP},
    {SDL_SCANCODE_K, BUTTON_C_DOWN},
    {SDL_SCANCODE_J, BUTTON_C_LEFT},
    {SDL_SCANCODE_L, BUTTON_C_RIGHT},
    {SDL_SCANCODE_KP_8, BUTTON_D_UP},
    {SDL_SCANCODE_KP_2, BUTTON_D_DOWN},
    {SDL_SCANCODE_KP_4, BUTTON_D_LEFT},
    {SDL_SCANCODE_KP_6, BUTTON_D_RIGHT},
};

struct PadBinding {
    ControlAction action;
    std::uint16_t button;
};

constexpr PadBinding PAD_BUTTONS[] = {
    {CONTROL_MENU_A, BUTTON_A},
    {CONTROL_MENU_B, BUTTON_B},
    {CONTROL_MENU_Z, BUTTON_Z},
    {CONTROL_MENU_START, BUTTON_START},
    {CONTROL_MENU_L, BUTTON_L},
    {CONTROL_MENU_R, BUTTON_R},
    {CONTROL_MENU_C_UP, BUTTON_C_UP},
    {CONTROL_MENU_C_DOWN, BUTTON_C_DOWN},
    {CONTROL_MENU_C_LEFT, BUTTON_C_LEFT},
    {CONTROL_MENU_C_RIGHT, BUTTON_C_RIGHT},
    {CONTROL_MENU_D_UP, BUTTON_D_UP},
    {CONTROL_MENU_D_DOWN, BUTTON_D_DOWN},
    {CONTROL_MENU_D_LEFT, BUTTON_D_LEFT},
    {CONTROL_MENU_D_RIGHT, BUTTON_D_RIGHT},
};

// The magnitudes of two opposite directions as one SDL axis.
std::int8_t
ScaleStickDirections(SDL_Gamepad *gamepad, ControlAction negative, ControlAction positive) {
    int axis = ReadControl(gamepad, positive) - ReadControl(gamepad, negative);
    return ScaleStickAxis(static_cast<std::int16_t>(std::clamp(axis, -32768, 32767)));
}

void PollGamepad(SDL_Gamepad *gamepad, ControllerState &state) {
    for (const auto &binding : PAD_BUTTONS) {
        if (ReadControl(gamepad, binding.action) > C_BUTTON_THRESHOLD) {
            state.buttons |= binding.button;
        }
    }
    state.stickX = ScaleStickDirections(gamepad, CONTROL_MENU_STICK_LEFT, CONTROL_MENU_STICK_RIGHT);
    // SDL reports down as positive, and the console reports up as positive.
    state.stickY = static_cast<std::int8_t>(
        -ScaleStickDirections(gamepad, CONTROL_MENU_STICK_UP, CONTROL_MENU_STICK_DOWN));
}

void PollKeyboard(ControllerState &state) {
    for (const auto &binding : KEY_BUTTONS) {
        if (IsKeyDown(binding.key)) {
            state.buttons |= binding.button;
        }
    }
    int x = (IsKeyDown(SDL_SCANCODE_RIGHT) ? STICK_RANGE : 0) -
            (IsKeyDown(SDL_SCANCODE_LEFT) ? STICK_RANGE : 0);
    int y = (IsKeyDown(SDL_SCANCODE_UP) ? STICK_RANGE : 0) -
            (IsKeyDown(SDL_SCANCODE_DOWN) ? STICK_RANGE : 0);
    if (x != 0) {
        state.stickX = static_cast<std::int8_t>(x);
    }
    if (y != 0) {
        state.stickY = static_cast<std::int8_t>(y);
    }
}

} // namespace

std::int8_t ScaleStickAxis(std::int16_t axis) {
    int value = axis;
    int magnitude = std::abs(value);
    if (magnitude <= DEAD_ZONE) {
        return 0;
    }
    int scaled = (magnitude - DEAD_ZONE) * STICK_RANGE / (32768 - DEAD_ZONE);
    scaled = std::min(scaled, STICK_RANGE);
    return static_cast<std::int8_t>(value < 0 ? -scaled : scaled);
}

InputBindings::InputBindings() {
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    for (int i = 0; ids != nullptr && i < count; ++i) {
        SDL_Event event{};
        event.gdevice.type = SDL_EVENT_GAMEPAD_ADDED;
        event.gdevice.which = ids[i];
        HandleEvent(event);
    }
    SDL_free(ids);
}

InputBindings::~InputBindings() {
    for (auto *gamepad : m_gamepads) {
        if (gamepad != nullptr) {
            SDL_CloseGamepad(gamepad);
        }
    }
}

void InputBindings::HandleEvent(const SDL_Event &event) {
    if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
        for (auto *gamepad : m_gamepads) {
            if (gamepad != nullptr && SDL_GetGamepadID(gamepad) == event.gdevice.which) {
                return;
            }
        }
        auto free = std::ranges::find(m_gamepads, nullptr);
        if (free == m_gamepads.end()) {
            SDL_Log("Every controller port is in use, and the new gamepad is ignored.");
            return;
        }
        *free = SDL_OpenGamepad(event.gdevice.which);
        if (*free != nullptr) {
            SDL_Log("Gamepad %s is bound to controller %d.",
                    SDL_GetGamepadName(*free),
                    static_cast<int>(free - m_gamepads.begin()) + 1);
        }
    } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
        for (auto *&gamepad : m_gamepads) {
            if (gamepad != nullptr && SDL_GetGamepadID(gamepad) == event.gdevice.which) {
                SDL_CloseGamepad(gamepad);
                gamepad = nullptr;
            }
        }
    }
}

std::array<ControllerState, CONTROLLER_PORTS> InputBindings::Poll() const {
    std::array<ControllerState, CONTROLLER_PORTS> ports{};
    for (std::size_t i = 0; i < CONTROLLER_PORTS; ++i) {
        if (m_gamepads[i] != nullptr) {
            ports[i].connected = true;
            PollGamepad(m_gamepads[i], ports[i]);
        }
    }
    ports[0].connected = true;
    PollKeyboard(ports[0]);
    return ports;
}

void SetScriptedKeys(std::vector<int> scancodes) {
    g_scriptedKeys = std::move(scancodes);
}

bool IsKeyDown(SDL_Scancode scancode) {
    return SDL_GetKeyboardState(nullptr)[scancode] ||
           std::ranges::find(g_scriptedKeys, static_cast<int>(scancode)) != g_scriptedKeys.end();
}

void SetSecondControllerState(const ControllerState &state) {
    g_secondController = state;
}

} // namespace coastline

int ReadSecondControllerPad(unsigned short *buttons, signed char *stickX, signed char *stickY) {
    *buttons = coastline::g_secondController.buttons;
    *stickX = coastline::g_secondController.stickX;
    *stickY = coastline::g_secondController.stickY;
    return coastline::g_secondController.connected ? 1 : 0;
}
