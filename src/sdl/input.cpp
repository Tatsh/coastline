#include "input.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <string>

#include "common/inputpoll.h"
#include "config.h"
#include "controls.h"

namespace coastline {

namespace {

// A real stick reads about 80 at its gate.
constexpr int STICK_RANGE = 80;
constexpr int C_BUTTON_THRESHOLD = 16384;

// The units of the wheel of a DirectInput mouse in each notch, WHEEL_DELTA of Win32.
constexpr float WHEEL_UNITS_PER_NOTCH = 120;
constexpr const char *ACTIVE_INPUT_PROPERTY = "coastline.input-bindings";

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

// A stick at rest leaves the stick of another device of the same player.
void PollGamepad(SDL_Gamepad *gamepad, ControllerState &state) {
    for (const auto &binding : PAD_BUTTONS) {
        if (ReadControl(gamepad, binding.action) > C_BUTTON_THRESHOLD) {
            state.buttons |= binding.button;
        }
    }
    const std::int8_t x =
        ScaleStickDirections(gamepad, CONTROL_MENU_STICK_LEFT, CONTROL_MENU_STICK_RIGHT);
    // SDL reports down as positive, and the console reports up as positive.
    const auto y = static_cast<std::int8_t>(
        -ScaleStickDirections(gamepad, CONTROL_MENU_STICK_UP, CONTROL_MENU_STICK_DOWN));
    if (x != 0) {
        state.stickX = x;
    }
    if (y != 0) {
        state.stickY = y;
    }
}

void PollKeyboard(ControllerState &state) {
    // The letters typed into a text are not the buttons they are bound to as well.
    // The game has one window, and its text input stays on while the window is not focused.
    int count = 0;
    SDL_Window **windows = SDL_GetWindows(&count);
    bool typing = windows != nullptr && count > 0 && SDL_TextInputActive(windows[0]);
    SDL_free(windows);
    for (const auto &binding : KEY_BUTTONS) {
        if (!typing && IsKeyDown(binding.key)) {
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

std::string LowercaseName(SDL_Gamepad *gamepad) {
    const char *name = SDL_GetGamepadName(gamepad);
    std::string lowercase(name != nullptr ? name : "");
    std::ranges::transform(lowercase, lowercase.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return lowercase;
}

std::string DescribeDevices(const PlayerDevices &devices) {
    std::string text;
    for (SDL_Gamepad *gamepad : devices.gamepads) {
        const char *name = SDL_GetGamepadName(gamepad);
        text += text.empty() ? "" : ", ";
        text += name != nullptr ? name : "a gamepad";
    }
    if (devices.keyboard) {
        text += text.empty() ? "keyboard" : ", keyboard";
    }
    return text.empty() ? "none" : text;
}

void LogPlayerDevices() {
    SDL_Log("Using player 1 devices %s; player 2 devices %s.",
            DescribeDevices(AssignedDevices(0, 2)).c_str(),
            DescribeDevices(AssignedDevices(1, 2)).c_str());
}

} // namespace

std::int8_t ScaleStickAxis(std::int16_t axis) {
    return static_cast<std::int8_t>(
        std::clamp(axis * STICK_RANGE / 32767, -STICK_RANGE, STICK_RANGE));
}

PlayerDevices InputBindings::AssignedDevices(std::size_t player, std::size_t players) const {
    std::array<PlayerDevices, 2> devices;
    std::vector<bool> taken(m_gamepads.size());
    std::vector<std::size_t> automatic;
    for (std::size_t p = 0; p < devices.size(); ++p) {
        const std::string_view device = m_controls.PlayerDevice(p);
        if (device == "auto") {
            automatic.push_back(p);
            continue;
        }
        if (device == "none") {
            continue;
        }
        if (device == "keyboard") {
            devices[p].keyboard = true;
            continue;
        }
        // gamepad1 is the first gamepad connected, and another value is a part of a name.
        std::size_t number = 0;
        const bool numbered =
            device.starts_with("gamepad") && device.size() > 7 &&
            std::ranges::all_of(device.substr(7), [](char c) { return c >= '0' && c <= '9'; });
        if (numbered) {
            number = std::stoul(std::string(device.substr(7)));
        }
        for (std::size_t i = 0; i < m_gamepads.size(); ++i) {
            const bool match = numbered ?
                                   number == i + 1 :
                                   LowercaseName(m_gamepads[i]).find(device) != std::string::npos;
            if (match && !taken[i]) {
                devices[p].gamepads.push_back(m_gamepads[i]);
                taken[i] = true;
                break;
            }
        }
    }
    const bool keyboardTaken = devices[0].keyboard || devices[1].keyboard;
    std::size_t next = 0;
    for (std::size_t i = 0; i < m_gamepads.size() && !automatic.empty(); ++i) {
        if (!taken[i]) {
            const std::size_t p = next < automatic.size() ? automatic[next++] : automatic.front();
            devices[p].gamepads.push_back(m_gamepads[i]);
        }
    }
    if (!keyboardTaken && !automatic.empty()) {
        auto free = std::ranges::find_if(
            automatic, [&](std::size_t p) { return devices[p].gamepads.empty(); });
        devices[free == automatic.end() ? automatic.front() : *free].keyboard = true;
    }
    if (players < 2) {
        devices[0].keyboard = devices[0].keyboard || devices[1].keyboard;
        devices[0].gamepads.insert(
            devices[0].gamepads.end(), devices[1].gamepads.begin(), devices[1].gamepads.end());
        devices[1] = {};
    }
    return devices[player];
}

InputBindings::InputBindings() : InputBindings(GameConfig{}) {
}

InputBindings::InputBindings(const GameConfig &config) : m_controls(config) {
    assert(Active() == nullptr);
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    for (int i = 0; ids != nullptr && i < count; ++i) {
        OpenGamepad(ids[i]);
    }
    SDL_free(ids);
    SDL_SetPointerProperty(SDL_GetGlobalProperties(), ACTIVE_INPUT_PROPERTY, this);
    LogPlayerDevices();
}

InputBindings::~InputBindings() {
    SDL_ClearProperty(SDL_GetGlobalProperties(), ACTIVE_INPUT_PROPERTY);
    for (auto *gamepad : m_gamepads) {
        SDL_CloseGamepad(gamepad);
    }
}

InputBindings *InputBindings::Active() {
    return static_cast<InputBindings *>(
        SDL_GetPointerProperty(SDL_GetGlobalProperties(), ACTIVE_INPUT_PROPERTY, nullptr));
}

bool InputBindings::OpenGamepad(SDL_JoystickID id) {
    if (std::ranges::any_of(
            m_gamepads, [&](SDL_Gamepad *gamepad) { return SDL_GetGamepadID(gamepad) == id; })) {
        return false;
    }
    SDL_Gamepad *gamepad = SDL_OpenGamepad(id);
    if (gamepad == nullptr) {
        SDL_LogWarn(SDL_LOG_CATEGORY_INPUT, "Cannot open gamepad: %s", SDL_GetError());
        return false;
    }
    m_gamepads.push_back(gamepad);
    SDL_Log(
        "Using gamepad %d (%s).", static_cast<int>(m_gamepads.size()), SDL_GetGamepadName(gamepad));
    return true;
}

PlayerDevices AssignedDevices(std::size_t player, std::size_t players) {
    const auto *input = InputBindings::Active();
    return input ? input->AssignedDevices(player, players) : PlayerDevices{};
}

void InputBindings::HandleEvent(const SDL_Event &event) {
    if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
        if (OpenGamepad(event.gdevice.which)) {
            LogPlayerDevices();
        }
    } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
        auto removed = std::ranges::find_if(m_gamepads, [&](SDL_Gamepad *gamepad) {
            return SDL_GetGamepadID(gamepad) == event.gdevice.which;
        });
        if (removed != m_gamepads.end()) {
            SDL_CloseGamepad(*removed);
            m_gamepads.erase(removed);
            LogPlayerDevices();
        }
    } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        // DirectInput reports the wheel as the hand turns it, whatever the scrolling of the
        // system.
        float notches =
            event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -event.wheel.y : event.wheel.y;
        m_mouseWheel += notches * WHEEL_UNITS_PER_NOTCH;
    }
}

std::array<ControllerState, CONTROLLER_PORTS> InputBindings::Poll(std::size_t players,
                                                                  bool race) const {
    std::array<ControllerState, CONTROLLER_PORTS> ports{};
    for (std::size_t player = 0; player < std::min<std::size_t>(players, 2); ++player) {
        const PlayerDevices devices = AssignedDevices(player, players);
        if (race && player == 1) {
            ports[player] = ReadRacePad(devices);
            continue;
        }
        for (SDL_Gamepad *gamepad : devices.gamepads) {
            PollGamepad(gamepad, ports[player]);
        }
        if (devices.keyboard) {
            PollKeyboard(ports[player]);
        }
    }
    ports[0].connected = true;
    const PlayerDevices second = AssignedDevices(1, 2);
    ports[1].connected = second.keyboard || !second.gamepads.empty();
    return ports;
}

void InputBindings::SetScriptedKeys(std::vector<int> scancodes) {
    m_scriptedKeys = std::move(scancodes);
}

bool InputBindings::IsKeyDown(SDL_Scancode scancode) {
    bool down =
        SDL_GetKeyboardState(nullptr)[scancode] ||
        std::ranges::find(m_scriptedKeys, static_cast<int>(scancode)) != m_scriptedKeys.end();
    auto ignored = std::ranges::find(m_ignoredKeys, static_cast<int>(scancode));
    if (ignored == m_ignoredKeys.end()) {
        return down;
    }
    if (!down) {
        m_ignoredKeys.erase(ignored);
    }
    return false;
}

void InputBindings::IgnoreKeyUntilReleased(SDL_Scancode scancode) {
    if (std::ranges::find(m_ignoredKeys, static_cast<int>(scancode)) == m_ignoredKeys.end()) {
        m_ignoredKeys.push_back(static_cast<int>(scancode));
    }
}

void InputBindings::SetSecondControllerState(const ControllerState &state) {
    m_secondController = state;
}

ControllerState InputBindings::SecondControllerState() const {
    return m_secondController;
}

int InputBindings::TakeMouseWheel() {
    int units = static_cast<int>(m_mouseWheel);
    m_mouseWheel -= static_cast<float>(units);
    return units;
}

void InputBindings::ClearMouseWheel() {
    m_mouseWheel = 0;
}

void SetScriptedKeys(std::vector<int> scancodes) {
    if (auto *input = InputBindings::Active()) {
        input->SetScriptedKeys(std::move(scancodes));
    }
}

bool IsKeyDown(SDL_Scancode scancode) {
    auto *input = InputBindings::Active();
    return input ? input->IsKeyDown(scancode) : SDL_GetKeyboardState(nullptr)[scancode];
}

void IgnoreKeyUntilReleased(SDL_Scancode scancode) {
    if (auto *input = InputBindings::Active()) {
        input->IgnoreKeyUntilReleased(scancode);
    }
}

void SetSecondControllerState(const ControllerState &state) {
    if (auto *input = InputBindings::Active()) {
        input->SetSecondControllerState(state);
    }
}

int TakeMouseWheel() {
    auto *input = InputBindings::Active();
    return input ? input->TakeMouseWheel() : 0;
}

void ClearMouseWheel() {
    if (auto *input = InputBindings::Active()) {
        input->ClearMouseWheel();
    }
}

} // namespace coastline

int ReadSecondControllerPad(unsigned short *buttons, signed char *stickX, signed char *stickY) {
    const auto *input = coastline::InputBindings::Active();
    const auto state = input ? input->SecondControllerState() : coastline::ControllerState{};
    *buttons = state.buttons;
    *stickX = state.stickX;
    *stickY = state.stickY;
    return state.connected ? 1 : 0;
}
