// The input of a race on SDL: the keyboard, the first gamepad, and the mouse, read into the states
// the bindings of the PC release address (the scan codes of DirectInput, the actions of the
// Controls section of BossRally.ini in place of the joystick, and the mouse), and the force
// feedback of the gamepad through the haptic or the rumble of SDL.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

#include <SDL3/SDL.h>

#include "common/fatalerror.h"
#include "common/frameclock.h"
#include "common/game.h"
#include "common/ghost.h"
#include "common/hud.h"
#include "common/inputpoll.h"
#include "common/instantreplay.h"
#include "common/laps.h"
#include "common/pause.h"
#include "common/racephases.h"
#include "common/rallyconfig.h"
#include "common/settings.h"
#include "common/snapshot.h"
#include "common/soundsystem.h"
#include "common/vehicle.h"
#include "common/vehicleaccess.h"
#include "controls.h"
#include "input.h"

namespace coastline {

namespace {

constexpr int KEYBOARD_KEY_COUNT = 256;
constexpr int MOUSE_BUTTON_COUNT = 4;
// The bit of a key or a button that is set while it is down.
constexpr unsigned char DOWN = 0x80;
// The greatest value DirectInput gives an axis of the joystick.
constexpr int JOYSTICK_AXIS_MAX = 128;
// The scan codes of DirectInput the poll reads by name.
constexpr int DIK_F5 = 0x3f;
constexpr int DIK_F11 = 0x57;
constexpr int DIK_F12 = 0x58;
// The range of each sensitivity of the mouse, by the steering mode of the first vehicle.
constexpr int MOUSE_AXIS_RANGES[8] = {803, 618, 475, 366, 281, 216, 166, 128};
// The force feedback of the PC release, in ten-thousandths, and its durations in microseconds.
constexpr int FORCE_FEEDBACK_FULL = 10000;
constexpr unsigned FORCE_FEEDBACK_LONG_DURATION = 250000;
constexpr unsigned FORCE_FEEDBACK_SHORT_DURATION = 125000;
constexpr Uint16 JOLT_PERIOD_MILLISECONDS = 250;
// The strengths of the springs of X and Y InitializeJoystickDevice() creates.
constexpr int SPRING_COEFFICIENT_X = 1000;
constexpr int SPRING_COEFFICIENT_Y = 8000;

struct ScanCode {
    unsigned char dik;
    SDL_Scancode sdl;
};

// The keys of the scan codes of DirectInput the bindings may address.
constexpr ScanCode SCAN_CODES[] = {
    {0x01, SDL_SCANCODE_ESCAPE},
    {0x02, SDL_SCANCODE_1},
    {0x03, SDL_SCANCODE_2},
    {0x04, SDL_SCANCODE_3},
    {0x05, SDL_SCANCODE_4},
    {0x06, SDL_SCANCODE_5},
    {0x07, SDL_SCANCODE_6},
    {0x08, SDL_SCANCODE_7},
    {0x09, SDL_SCANCODE_8},
    {0x0a, SDL_SCANCODE_9},
    {0x0b, SDL_SCANCODE_0},
    {0x0c, SDL_SCANCODE_MINUS},
    {0x0d, SDL_SCANCODE_EQUALS},
    {0x0e, SDL_SCANCODE_BACKSPACE},
    {0x0f, SDL_SCANCODE_TAB},
    {0x10, SDL_SCANCODE_Q},
    {0x11, SDL_SCANCODE_W},
    {0x12, SDL_SCANCODE_E},
    {0x13, SDL_SCANCODE_R},
    {0x14, SDL_SCANCODE_T},
    {0x15, SDL_SCANCODE_Y},
    {0x16, SDL_SCANCODE_U},
    {0x17, SDL_SCANCODE_I},
    {0x18, SDL_SCANCODE_O},
    {0x19, SDL_SCANCODE_P},
    {0x1a, SDL_SCANCODE_LEFTBRACKET},
    {0x1b, SDL_SCANCODE_RIGHTBRACKET},
    {0x1c, SDL_SCANCODE_RETURN},
    {0x1d, SDL_SCANCODE_LCTRL},
    {0x1e, SDL_SCANCODE_A},
    {0x1f, SDL_SCANCODE_S},
    {0x20, SDL_SCANCODE_D},
    {0x21, SDL_SCANCODE_F},
    {0x22, SDL_SCANCODE_G},
    {0x23, SDL_SCANCODE_H},
    {0x24, SDL_SCANCODE_J},
    {0x25, SDL_SCANCODE_K},
    {0x26, SDL_SCANCODE_L},
    {0x27, SDL_SCANCODE_SEMICOLON},
    {0x28, SDL_SCANCODE_APOSTROPHE},
    {0x29, SDL_SCANCODE_GRAVE},
    {0x2a, SDL_SCANCODE_LSHIFT},
    {0x2b, SDL_SCANCODE_BACKSLASH},
    {0x2c, SDL_SCANCODE_Z},
    {0x2d, SDL_SCANCODE_X},
    {0x2e, SDL_SCANCODE_C},
    {0x2f, SDL_SCANCODE_V},
    {0x30, SDL_SCANCODE_B},
    {0x31, SDL_SCANCODE_N},
    {0x32, SDL_SCANCODE_M},
    {0x33, SDL_SCANCODE_COMMA},
    {0x34, SDL_SCANCODE_PERIOD},
    {0x35, SDL_SCANCODE_SLASH},
    {0x36, SDL_SCANCODE_RSHIFT},
    {0x37, SDL_SCANCODE_KP_MULTIPLY},
    {0x38, SDL_SCANCODE_LALT},
    {0x39, SDL_SCANCODE_SPACE},
    {0x3a, SDL_SCANCODE_CAPSLOCK},
    {0x3b, SDL_SCANCODE_F1},
    {0x3c, SDL_SCANCODE_F2},
    {0x3d, SDL_SCANCODE_F3},
    {0x3e, SDL_SCANCODE_F4},
    {0x3f, SDL_SCANCODE_F5},
    {0x40, SDL_SCANCODE_F6},
    {0x41, SDL_SCANCODE_F7},
    {0x42, SDL_SCANCODE_F8},
    {0x43, SDL_SCANCODE_F9},
    {0x44, SDL_SCANCODE_F10},
    {0x45, SDL_SCANCODE_NUMLOCKCLEAR},
    {0x46, SDL_SCANCODE_SCROLLLOCK},
    {0x47, SDL_SCANCODE_KP_7},
    {0x48, SDL_SCANCODE_KP_8},
    {0x49, SDL_SCANCODE_KP_9},
    {0x4a, SDL_SCANCODE_KP_MINUS},
    {0x4b, SDL_SCANCODE_KP_4},
    {0x4c, SDL_SCANCODE_KP_5},
    {0x4d, SDL_SCANCODE_KP_6},
    {0x4e, SDL_SCANCODE_KP_PLUS},
    {0x4f, SDL_SCANCODE_KP_1},
    {0x50, SDL_SCANCODE_KP_2},
    {0x51, SDL_SCANCODE_KP_3},
    {0x52, SDL_SCANCODE_KP_0},
    {0x53, SDL_SCANCODE_KP_PERIOD},
    {0x56, SDL_SCANCODE_NONUSBACKSLASH},
    {0x57, SDL_SCANCODE_F11},
    {0x58, SDL_SCANCODE_F12},
    {0x9c, SDL_SCANCODE_KP_ENTER},
    {0x9d, SDL_SCANCODE_RCTRL},
    {0xb5, SDL_SCANCODE_KP_DIVIDE},
    {0xb7, SDL_SCANCODE_PRINTSCREEN},
    {0xb8, SDL_SCANCODE_RALT},
    {0xc5, SDL_SCANCODE_PAUSE},
    {0xc7, SDL_SCANCODE_HOME},
    {0xc8, SDL_SCANCODE_UP},
    {0xc9, SDL_SCANCODE_PAGEUP},
    {0xcb, SDL_SCANCODE_LEFT},
    {0xcd, SDL_SCANCODE_RIGHT},
    {0xcf, SDL_SCANCODE_END},
    {0xd0, SDL_SCANCODE_DOWN},
    {0xd1, SDL_SCANCODE_PAGEDOWN},
    {0xd2, SDL_SCANCODE_INSERT},
    {0xd3, SDL_SCANCODE_DELETE},
    {0xdb, SDL_SCANCODE_LGUI},
    {0xdc, SDL_SCANCODE_RGUI},
    {0xdd, SDL_SCANCODE_APPLICATION},
};

struct RaceControl {
    ControlAction action;
    int binding;
};

// The actions of the Controls section and the bindings of the PC release they drive.
constexpr RaceControl RACE_CONTROLS[] = {
    {CONTROL_STEER_LEFT, 0},
    {CONTROL_STEER_RIGHT, 1},
    {CONTROL_ACCELERATE, 2},
    {CONTROL_BRAKE, 3},
    {CONTROL_REVERSE, 4},
    {CONTROL_GEAR_UP, 5},
    {CONTROL_GEAR_DOWN, 6},
    {CONTROL_COCKPIT_CAMERA, 8},
    {CONTROL_BUMPER_CAMERA, 9},
    {CONTROL_CHASE_CAMERA, 10},
    {CONTROL_PAUSE_MENU_UP, 12},
    {CONTROL_PAUSE_MENU_DOWN, 13},
    {CONTROL_PAUSE_MENU_SELECT, 14},
    {CONTROL_PAUSE, 15},
    {CONTROL_START_OR_RESPAWN, 16},
};

// The kinds of a binding of the PC release that address a button or one of the three axes of a
// joystick, read from the Controls section in place of the joystick.
constexpr unsigned JOYSTICK_BUTTON_KIND = 0x100;
constexpr unsigned JOYSTICK_AXIS_KIND = 0x8000;
constexpr unsigned MOUSE_AXIS_KIND = 0x8600;

bool IsJoystickKind(unsigned kind) {
    return kind == JOYSTICK_BUTTON_KIND || (kind >= JOYSTICK_AXIS_KIND && kind < MOUSE_AXIS_KIND);
}

// A magnitude of the Controls section on the range of an axis of DirectInput, 0 to 128.
int ScaleControl(int magnitude) {
    return magnitude * JOYSTICK_AXIS_MAX / CONTROL_FULL;
}

struct JoystickState {
    std::array<int, CONTROL_ACTION_COUNT> controls;
};

struct MouseState {
    int x;
    int y;
    int z;
    int accumulatedX;
    int accumulatedY;
    int accumulatedZ;
    std::array<unsigned char, MOUSE_BUTTON_COUNT> buttons;
};

bool IsPressedEdge(unsigned char previous, unsigned char current) {
    return !(previous & DOWN) && (current & DOWN);
}

int ScaleForceFeedback(int value, int multiplier) {
    return static_cast<int>(static_cast<unsigned>(value) * static_cast<unsigned>(multiplier)) /
           FORCE_FEEDBACK_FULL;
}

// A spring of DirectInput, in ten-thousandths, as a coefficient of SDL.
Sint16 HapticCoefficient(int coefficient) {
    return static_cast<Sint16>(std::clamp(coefficient, -FORCE_FEEDBACK_FULL, FORCE_FEEDBACK_FULL) *
                               32767 / FORCE_FEEDBACK_FULL);
}

void ClampMouseAxis(int &axis, int &accumulated, int range) {
    axis = static_cast<int>(static_cast<unsigned>(accumulated) * 128) / range;
    if (axis < -128) {
        axis = -128;
        accumulated = -range;
    } else if (axis > 128) {
        axis = 128;
        accumulated = range;
    }
}

} // namespace

/** The devices and the state of the input of a race. */
class RaceInput {
public:
    static RaceInput &Get() {
        static RaceInput input;
        return input;
    }

    unsigned ReadButtonMask(int *steering, int *acceleration);
    void ResetMouse();
    int InitializeJoystick();
    void ReleaseJoystick();
    void UpdateForce(int reduce, int enable, int force);
    void SetDirection(int direction);
    void SetDuration(unsigned microseconds);
    void StartJolt();

private:
    RaceInput();
    void ReadKeyboard();
    void ReadJoystick();
    void ReadMouse();
    unsigned char IsHeld(int binding) const;
    unsigned char IsPressed(int binding) const;
    int ControlOf(int binding, bool current) const;
    int AxisOf(unsigned kind, bool current) const;
    int ReadAxisMagnitude(int binding) const;
    bool IsForceFeedbackInUse() const;
    void SetSpringMagnitude(int magnitude);
    bool AsyncKeyPressed(SDL_Scancode scancode);

    std::array<std::array<unsigned char, KEYBOARD_KEY_COUNT>, 2> m_keyboard{};
    int m_keyboardCurrent = 1;
    int m_keyboardPrevious = 0;
    std::array<JoystickState, 2> m_joystick{};
    int m_joystickCurrent = 1;
    int m_joystickPrevious = 0;
    std::array<MouseState, 2> m_mouse{};
    int m_mouseCurrent = 1;
    int m_mousePrevious = 0;
    int m_pollCounter = 0;
    unsigned m_benchmarkStartTime = 0;
    int m_benchmarkStartFrameCount = 0;
    std::array<bool, SDL_SCANCODE_COUNT> m_asyncDown{};
#ifdef ENABLE_PATCHES
    unsigned m_lastRespawnPress = 0;
    // The reads of the input, from 1.
    unsigned m_steps = 0;
#endif

    SDL_Gamepad *m_gamepad = nullptr;
    SDL_Haptic *m_haptic = nullptr;
    int m_joystickUsers = 0;
    bool m_forceFeedbackAvailable = false;
    int m_springEffect = -1;
    int m_joltEffect = -1;
    SDL_HapticEffect m_spring{};
    SDL_HapticEffect m_jolt{};
    int m_forceMagnitude = 0;
    bool m_forceStopped = false;
    int m_forceGain = FORCE_FEEDBACK_FULL;
    int m_forceMagnitudeMax = 2000;
    int m_forceConstant = FORCE_FEEDBACK_FULL;
    unsigned m_forceDuration = FORCE_FEEDBACK_SHORT_DURATION;
    std::array<int, 2> m_forceDirection{-1, 0};
};

RaceInput::RaceInput() {
    ResetMouse();
}

void RaceInput::ResetMouse() {
    m_mouse = {};
    m_mousePrevious = 0;
    m_mouseCurrent = 1;
}

void RaceInput::ReadKeyboard() {
    m_keyboardPrevious = m_keyboardCurrent;
    m_keyboardCurrent = (m_keyboardCurrent - 1) & 1;
    auto &keys = m_keyboard[static_cast<std::size_t>(m_keyboardCurrent)];
    keys.fill(0);
    for (const auto &code : SCAN_CODES) {
        if (IsKeyDown(code.sdl)) {
            keys[code.dik] = DOWN;
        }
    }
    keys[0] = 0;
}

void RaceInput::ReadJoystick() {
    m_joystickPrevious = m_joystickCurrent;
    m_joystickCurrent = (m_joystickCurrent - 1) & 1;
    auto &state = m_joystick[static_cast<std::size_t>(m_joystickCurrent)];
    state.controls.fill(0);
    if (m_gamepad == nullptr || !SDL_GamepadConnected(m_gamepad)) {
        return;
    }
    for (const RaceControl &control : RACE_CONTROLS) {
        state.controls[control.action] = ReadControl(m_gamepad, control.action);
    }
}

// The magnitude of the action of the Controls section that drives a binding, 0 to 128.
int RaceInput::ControlOf(int binding, bool current) const {
    const auto *control = std::ranges::find(RACE_CONTROLS, binding, &RaceControl::binding);
    if (control == std::end(RACE_CONTROLS)) {
        return 0;
    }
    const JoystickState &state =
        m_joystick[static_cast<std::size_t>(current ? m_joystickCurrent : m_joystickPrevious)];
    return ScaleControl(state.controls[control->action]);
}

void RaceInput::ReadMouse() {
    m_mousePrevious = m_mouseCurrent;
    m_mouseCurrent = (m_mouseCurrent - 1) & 1;
    MouseState &state = m_mouse[static_cast<std::size_t>(m_mouseCurrent)];
    const MouseState &previous = m_mouse[static_cast<std::size_t>(m_mousePrevious)];
    float dx = 0;
    float dy = 0;
    SDL_GetRelativeMouseState(&dx, &dy);
    SDL_MouseButtonFlags buttons = SDL_GetMouseState(nullptr, nullptr);
    // ponytail: SDL reports the wheel only as events, so the third axis of the mouse does not
    // move; read SDL_EVENT_MOUSE_WHEEL into it when a binding needs the wheel.
    int dz = 0;
    state.accumulatedX = previous.accumulatedX + static_cast<int>(dx);
    // The three accumulators add to the horizontal accumulator of the state before, as in the
    // original.
    state.accumulatedY = previous.accumulatedX + static_cast<int>(dy);
    state.accumulatedZ = previous.accumulatedX + dz;
    int sensitivity = std::clamp(VEHICLE_GET(&(g_aVehicles[0]), SteeringMode), 0, 7);
    const int range = MOUSE_AXIS_RANGES[sensitivity];
    ClampMouseAxis(state.x, state.accumulatedX, range);
    ClampMouseAxis(state.y, state.accumulatedY, range);
    ClampMouseAxis(state.z, state.accumulatedZ, range);
    // DirectInput numbers the left, the right, the middle, and the first extra button.
    constexpr SDL_MouseButtonFlags MOUSE_BUTTONS[MOUSE_BUTTON_COUNT] = {
        SDL_BUTTON_LMASK, SDL_BUTTON_RMASK, SDL_BUTTON_MMASK, SDL_BUTTON_X1MASK};
    for (int i = 0; i < MOUSE_BUTTON_COUNT; ++i) {
        state.buttons[static_cast<std::size_t>(i)] = (buttons & MOUSE_BUTTONS[i]) ? DOWN : 0;
    }
}

// The axes of the joystick are read through ControlOf().
int RaceInput::AxisOf(unsigned kind, bool current) const {
    const unsigned axis = (kind - 0x8000) >> 9;
    const MouseState &state =
        m_mouse[static_cast<std::size_t>(current ? m_mouseCurrent : m_mousePrevious)];
    return axis == 3 ? state.x : axis == 4 ? state.y : state.z;
}

unsigned char RaceInput::IsHeld(int binding) const {
    const InputBinding &entry = g_GlobalRallyConfig.pActiveInputBindings[binding];
    const unsigned kind = entry.wControl & 0xff00;
    const unsigned button = entry.wControl & 0xff;
    const auto &keys = m_keyboard[static_cast<std::size_t>(m_keyboardCurrent)];
    unsigned char result = 0;

    if (IsJoystickKind(kind)) {
        result = ControlOf(binding, true) > 50 ? DOWN : 0;
    } else if (kind == 0) {
        result = keys[button] & DOWN;
    } else if (kind == 0x300) {
        result = m_mouse[static_cast<std::size_t>(m_mouseCurrent)].buttons[button & 3] & DOWN;
    } else if (kind >= MOUSE_AXIS_KIND && kind <= 0x8b00) {
        const int current = AxisOf(kind, true);
        result = ((kind & 0x100) ? current > 50 : current < -50) ? DOWN : 0;
    }
    for (unsigned short control : entry.awAlternateControls) {
        if ((control & 0xff00) == 0) {
            result |= keys[control & 0xff] & DOWN;
        }
    }
    return result;
}

unsigned char RaceInput::IsPressed(int binding) const {
    const InputBinding &entry = g_GlobalRallyConfig.pActiveInputBindings[binding];
    const unsigned kind = entry.wControl & 0xff00;
    const unsigned button = entry.wControl & 0xff;
    const auto &keys = m_keyboard[static_cast<std::size_t>(m_keyboardCurrent)];
    const auto &previousKeys = m_keyboard[static_cast<std::size_t>(m_keyboardPrevious)];
    unsigned char result = 0;

    if (IsJoystickKind(kind)) {
        result = ControlOf(binding, false) <= 50 && ControlOf(binding, true) > 50 ? DOWN : 0;
    } else if (kind == 0) {
        result = IsPressedEdge(previousKeys[button], keys[button]);
    } else if (kind == 0x300) {
        result =
            IsPressedEdge(m_mouse[static_cast<std::size_t>(m_mousePrevious)].buttons[button & 3],
                          m_mouse[static_cast<std::size_t>(m_mouseCurrent)].buttons[button & 3]);
    } else if (kind >= MOUSE_AXIS_KIND && kind <= 0x8b00) {
        const int previous = AxisOf(kind, false);
        const int current = AxisOf(kind, true);
        if (kind & 0x100) {
            result = previous <= 50 && current > 50 ? DOWN : 0;
        } else {
            result = previous >= -50 && current < -50 ? DOWN : 0;
        }
    }
    for (unsigned short control : entry.awAlternateControls) {
        if ((control & 0xff00) == 0) {
            result |= IsPressedEdge(previousKeys[control & 0xff], keys[control & 0xff]);
        }
    }
    return result;
}

int RaceInput::ReadAxisMagnitude(int binding) const {
    const unsigned kind = g_GlobalRallyConfig.pActiveInputBindings[binding].wControl & 0xff00;
    if (kind < 0x8000 || kind > 0x8b00) {
        return -1;
    }
    if (IsJoystickKind(kind)) {
        // A key of the alternates drives an analogue action fully, as a button of the gamepad does.
        const auto &keys = m_keyboard[static_cast<std::size_t>(m_keyboardCurrent)];
        int value = ControlOf(binding, true);
        for (unsigned short control :
             g_GlobalRallyConfig.pActiveInputBindings[binding].awAlternateControls) {
            if ((control & 0xff00) == 0 && (keys[control & 0xff] & DOWN)) {
                value = JOYSTICK_AXIS_MAX;
            }
        }
        return value > 0 ? value * 80 / 128 : -1;
    }
    const int value = AxisOf(kind, true);
    if (kind & 0x100) {
        return value > 0 ? value * 80 / 128 : -1;
    }
    return value < 0 ? -value * 80 / 128 : -1;
}

// GetAsyncKeyState() reports a key pressed since the call before in its lowest bit.
bool RaceInput::AsyncKeyPressed(SDL_Scancode scancode) {
    bool down = IsKeyDown(scancode);
    bool pressed = down && !m_asyncDown[scancode];
    m_asyncDown[scancode] = down;
    return pressed;
}

unsigned RaceInput::ReadButtonMask(int *pnSteering, int *pnAcceleration) {
    if (m_pollCounter < 0x7fff) {
        ++m_pollCounter;
    }
#ifdef ENABLE_PATCHES
    ++m_steps;
#endif
    ReadKeyboard();
    const auto &keys = m_keyboard[static_cast<std::size_t>(m_keyboardCurrent)];
    const auto &previousKeys = m_keyboard[static_cast<std::size_t>(m_keyboardPrevious)];
    if (g_GlobalRallyConfig.dwInputDeviceType == 1 || g_GlobalRallyConfig.dwInputDeviceType == 2) {
        ReadJoystick();
    }
    ReadMouse();
    // The SDL game has no network play, and a pause binding always pauses.
    if (IsPressed(15)) {
        g_bNetGamePaused = 1;
        g_dwInputButtonMask = 0x4000;
        return g_dwInputButtonMask;
    }
#ifdef ENABLE_PATCHES
    // Until the start binding begins a race, every other binding but the pause is ignored.
    if (!g_bRaceStartReached && !g_dwNetworkPlayMode && !gInstantReplayActive) {
        if (IsPressed(16)) {
            g_bRaceStartReached = 1;
            if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
                StartGhostRecording();
            }
        }
        *pnSteering = 0;
        *pnAcceleration = 0;
        g_dwInputButtonMask = 0;
        return 0;
    }
#endif /* ENABLE_PATCHES */
    // F10 shows the standings of network play, and P hides them, in the PC release.
    int *toggles[5] = {&g_bHudShowLapCounter,
                       &g_bHudShowRaceTimer,
                       &g_bHudShowLapTimes,
                       &g_bHudShowSpeedometer,
                       &g_bHudShowPosition};
    for (int i = 0; i < 5; ++i) {
        if (IsPressedEdge(previousKeys[DIK_F5 + i], keys[DIK_F5 + i])) {
            *toggles[i] = !*toggles[i];
        }
    }
    if (IsPressedEdge(previousKeys[DIK_F11], keys[DIK_F11])) {
        SelectPreviousMusicTrack();
    }
    if (IsPressedEdge(previousKeys[DIK_F12], keys[DIK_F12])) {
        SelectNextMusicTrack();
    }
    if (AsyncKeyPressed(SDL_SCANCODE_F) && m_pollCounter > 15) {
        g_bShowFpsCounter = !g_bShowFpsCounter;
    }
    unsigned mask = 0;
    if (gInstantReplayActive) {
        if (gInstantReplayActive == 2) {
            for (int binding = 8; binding <= 10; ++binding) {
                if (IsPressed(binding)) {
                    mask |= 1u << binding;
                }
            }
        }
        if (IsPressed(11)) {
            mask |= 0x800;
        }
        if (IsPressed(21)) {
            mask |= 0x100400;
        }
        for (int binding = 22; binding <= 25; ++binding) {
            if (IsHeld(binding)) {
                mask |= 1u << (binding - 1);
            }
        }
        if (IsPressed(26)) {
            mask |= 0x200000;
        }
        if (IsPressed(27)) {
            mask |= 0x400000;
        }
        g_dwInputButtonMask = mask;
        return mask;
    }
    if (IsPressed(16)) {
#ifdef ENABLE_PATCHES
        // The car is reset by a second press within half a second of the first.
        // The SDL game reads the input once each step of the race, a 30th of a second, and counts
        // the half second in steps for the same presses to respawn in every run of fixed steps.
        constexpr unsigned RESPAWN_DOUBLE_PRESS_STEPS = 15;
        const unsigned now = m_steps;
        const bool doublePress =
            m_lastRespawnPress != 0 && now - m_lastRespawnPress <= RESPAWN_DOUBLE_PRESS_STEPS;
        m_lastRespawnPress = doublePress ? 0 : now;
        if (doublePress && !g_dwRacePauseState && g_dwGameMode != GAME_MODE_DEMO &&
            g_dwGameMode != GAME_MODE_AFTER_SEASON) {
            g_bRespawnRequested = 1;
        }
#else
        if (!g_dwRacePauseState && g_dwGameMode != GAME_MODE_DEMO &&
            g_dwGameMode != GAME_MODE_AFTER_SEASON) {
            g_bRespawnRequested = 1;
        }
#endif /* ENABLE_PATCHES */
        g_bRaceStartReached = 1;
        if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
            StartGhostRecording();
        }
    }
    if (IsPressed(14)) {
        mask = 0x8000;
    }
    *pnSteering = 0;
    if (g_dwGameMode != GAME_MODE_DEMO && g_dwGameMode != GAME_MODE_AFTER_SEASON) {
        const int left = ReadAxisMagnitude(0);
        if (left >= 0) {
            *pnSteering = -left;
        }
        const int right = ReadAxisMagnitude(1);
        if (right >= 0) {
            *pnSteering = right;
        }
    }
    if (g_dwRacePauseState) {
        for (int binding : {12, 13, 0, 1, 2}) {
            if (IsPressed(binding)) {
                mask |= 1u << binding;
            }
        }
    } else {
        *pnAcceleration = 0;
        if (IsHeld(2)) {
            mask |= 4;
            if (!(g_GlobalRallyConfig.pActiveInputBindings[2].wControl & 0x8000)) {
                *pnAcceleration = 80;
            }
        }
        const int acceleration = ReadAxisMagnitude(2);
        if (acceleration >= 0) {
            *pnAcceleration = acceleration;
        }
        if (IsHeld(3)) {
            mask |= 8;
        }
        if (IsHeld(4)) {
            mask |= 16;
            *pnAcceleration = -80;
        }
        if (g_dwGameMode != GAME_MODE_DEMO && g_dwGameMode != GAME_MODE_AFTER_SEASON) {
            for (int binding : {0, 1}) {
                if (IsHeld(binding)) {
                    mask |= 1u << binding;
                }
            }
        }
        for (int binding : {5, 6, 7}) {
            if (IsHeld(binding)) {
                mask |= 1u << binding;
            }
        }
        for (int binding : {8, 9, 10, 17, 18, 19, 20}) {
            if (IsPressed(binding)) {
                mask |= 1u << (binding >= 17 ? binding - 1 : binding);
            }
        }
        if (g_dwRunBenchmark) {
            if (m_pollCounter == 1) {
                m_benchmarkStartTime = GetHighResolutionTime();
                m_benchmarkStartFrameCount = GetInterpolatedFrameCount();
            }
            if (m_pollCounter == 441) {
                const unsigned now = GetHighResolutionTime();
                const unsigned elapsed = now - m_benchmarkStartTime;
                m_benchmarkStartTime = now;
                const int frames = GetInterpolatedFrameCount() - m_benchmarkStartFrameCount;
                char message[256];
                std::snprintf(message,
                              sizeof(message),
                              "fps = %0.2f",
                              frames * 1000.0 / static_cast<int>(elapsed));
                // The original exits before it shows the rate.
                SDL_Log("%s", message);
                std::exit(1);
            }
            if (m_pollCounter == 1) {
                mask = 0x400;
                g_bShowFpsCounter = 1;
            } else {
                mask = m_pollCounter < 60 ? 0 : 4;
            }
        }
    }
    g_dwInputButtonMask = mask;
    return mask;
}

int RaceInput::InitializeJoystick() {
    if (g_GlobalRallyConfig.dwInputDeviceType == 0 || ++m_joystickUsers != 1) {
        return g_GlobalRallyConfig.dwInputDeviceType;
    }
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    if (ids != nullptr && count > 0) {
        m_gamepad = SDL_OpenGamepad(ids[0]);
    }
    SDL_free(ids);
    if (m_gamepad == nullptr) {
        return 0;
    }
    SDL_Log("Gamepad %s drives the race.", SDL_GetGamepadName(m_gamepad));
    // Each action of the Controls section replaces the joystick or the key of its binding with the
    // joystick, an axis for an action an axis drives, as the race engine treats an axis as
    // analogue. The keys of the defaults remain in the alternates.
    InputBinding *bindings =
        g_GlobalRallyConfig.GetDeviceBindings(g_GlobalRallyConfig.dwInputDeviceType);
    for (const RaceControl &control : RACE_CONTROLS) {
        bindings[control.binding].wControl = static_cast<unsigned short>(
            IsControlAnalog(control.action) ? JOYSTICK_AXIS_KIND : JOYSTICK_BUTTON_KIND);
    }
    m_forceFeedbackAvailable = false;
    if (!g_GlobalRallyConfig.bForceFeedbackEnabled) {
        return g_GlobalRallyConfig.dwInputDeviceType;
    }
    m_haptic = SDL_OpenHapticFromJoystick(SDL_GetGamepadJoystick(m_gamepad));
    Uint32 features = m_haptic != nullptr ? SDL_GetHapticFeatures(m_haptic) : 0;
    if (features & SDL_HAPTIC_AUTOCENTER) {
        SDL_SetHapticAutocenter(m_haptic, 0);
    }
    if (features & SDL_HAPTIC_SPRING) {
        m_spring.type = SDL_HAPTIC_SPRING;
        m_spring.condition.direction.type = SDL_HAPTIC_CARTESIAN;
        m_spring.condition.length = SDL_HAPTIC_INFINITY;
        const int coefficients[2] = {SPRING_COEFFICIENT_X, SPRING_COEFFICIENT_Y};
        for (int axis = 0; axis < 2; ++axis) {
            m_spring.condition.right_sat[axis] = 0xffff;
            m_spring.condition.left_sat[axis] = 0xffff;
            m_spring.condition.right_coeff[axis] = HapticCoefficient(coefficients[axis]);
            m_spring.condition.left_coeff[axis] = HapticCoefficient(coefficients[axis]);
        }
        m_springEffect = SDL_CreateHapticEffect(m_haptic, &m_spring);
        if (m_springEffect >= 0) {
            m_forceMagnitude = SPRING_COEFFICIENT_X;
            m_forceStopped = true;
        }
    }
    if (features & SDL_HAPTIC_SQUARE) {
        m_jolt.type = SDL_HAPTIC_SQUARE;
        m_jolt.periodic.direction.type = SDL_HAPTIC_CARTESIAN;
        m_jolt.periodic.period = JOLT_PERIOD_MILLISECONDS;
        m_jolt.periodic.magnitude = 32767;
        m_jolt.periodic.length = m_forceDuration / 1000;
        m_joltEffect = SDL_CreateHapticEffect(m_haptic, &m_jolt);
    }
    // A gamepad without the effects of DirectInput shakes its rumble for the jolt.
    m_forceFeedbackAvailable = m_springEffect >= 0 || m_joltEffect >= 0 ||
                               SDL_GetBooleanProperty(SDL_GetGamepadProperties(m_gamepad),
                                                      SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN,
                                                      false);
    return g_GlobalRallyConfig.dwInputDeviceType;
}

void RaceInput::ReleaseJoystick() {
    if (--m_joystickUsers < 0) {
        m_joystickUsers = 0;
        return;
    }
    if (m_joystickUsers != 0) {
        return;
    }
    if (m_haptic != nullptr) {
        SDL_CloseHaptic(m_haptic);
        m_haptic = nullptr;
    }
    m_springEffect = -1;
    m_joltEffect = -1;
    if (m_gamepad != nullptr) {
        SDL_CloseGamepad(m_gamepad);
        m_gamepad = nullptr;
    }
}

bool RaceInput::IsForceFeedbackInUse() const {
    return (g_GlobalRallyConfig.dwInputDeviceType == 1 ||
            g_GlobalRallyConfig.dwInputDeviceType == 2) &&
           g_GlobalRallyConfig.bForceFeedbackEnabled && m_forceFeedbackAvailable &&
           !gInstantReplayActive;
}

void RaceInput::SetSpringMagnitude(int magnitude) {
    m_spring.condition.right_coeff[0] = HapticCoefficient(magnitude);
    m_spring.condition.left_coeff[0] = HapticCoefficient(magnitude);
    if (m_springEffect >= 0) {
        SDL_UpdateHapticEffect(m_haptic, m_springEffect, &m_spring);
    }
}

void RaceInput::UpdateForce(int reduce, int enable, int force) {
    if (!IsForceFeedbackInUse()) {
        return;
    }
    m_forceConstant = ScaleForceFeedback(
        static_cast<int>((static_cast<unsigned>(force) + 8) * static_cast<unsigned>(m_forceGain)),
        1000);
    if (!enable) {
        if (!m_forceStopped) {
            if (m_springEffect >= 0) {
                SDL_StopHapticEffect(m_haptic, m_springEffect);
            }
            m_forceStopped = true;
        }
        return;
    }
    if (m_forceStopped) {
        if (m_springEffect >= 0) {
            SDL_RunHapticEffect(m_haptic, m_springEffect, 1);
        }
        SetDuration(FORCE_FEEDBACK_LONG_DURATION);
        m_forceStopped = false;
    }
    const int previous = m_forceMagnitude;
    if (reduce) {
        m_forceMagnitude += ScaleForceFeedback(m_forceGain, -1000);
        const int minimum = ScaleForceFeedback(m_forceMagnitudeMax, m_forceGain);
        if (m_forceMagnitude < minimum) {
            m_forceMagnitude = minimum;
        }
    } else {
        m_forceMagnitude += ScaleForceFeedback(m_forceGain, 1000);
        const int maximum = ScaleForceFeedback(m_forceConstant, m_forceGain);
        if (m_forceMagnitude > maximum) {
            m_forceMagnitude = maximum;
        }
    }
    if (m_forceMagnitude != previous) {
        SetSpringMagnitude(m_forceMagnitude);
    }
}

void RaceInput::SetDirection(int direction) {
    if (IsForceFeedbackInUse()) {
        m_forceDirection[0] = direction;
    }
}

void RaceInput::SetDuration(unsigned microseconds) {
    if (IsForceFeedbackInUse()) {
        m_forceDuration = microseconds;
    }
}

void RaceInput::StartJolt() {
    if (!IsForceFeedbackInUse()) {
        return;
    }
    const Uint32 milliseconds = m_forceDuration / 1000;
    if (m_joltEffect >= 0) {
        m_jolt.periodic.length = milliseconds;
        m_jolt.periodic.direction.dir[0] = m_forceDirection[0];
        m_jolt.periodic.direction.dir[1] = m_forceDirection[1];
        // The jolt starts again with its new duration and direction, and a failure is ignored.
        SDL_UpdateHapticEffect(m_haptic, m_joltEffect, &m_jolt);
        SDL_RunHapticEffect(m_haptic, m_joltEffect, 1);
    } else if (m_gamepad != nullptr) {
        // The motor of the side of the impact shakes harder.
        const Uint16 low = m_forceDirection[0] < 0 ? 0xffff : 0x8000;
        const Uint16 high = m_forceDirection[0] < 0 ? 0x8000 : 0xffff;
        SDL_RumbleGamepad(m_gamepad, low, high, milliseconds);
    }
}

} // namespace coastline

unsigned ReadInputDevicesButtonMask(int *pnSteering, int *pnAcceleration) {
    return coastline::RaceInput::Get().ReadButtonMask(pnSteering, pnAcceleration);
}

void ResetMouseState(void) {
    coastline::RaceInput::Get().ResetMouse();
}

int InitializeJoystickDevice(void) {
    return coastline::RaceInput::Get().InitializeJoystick();
}

void ReleaseJoystickDevice(void) {
    coastline::RaceInput::Get().ReleaseJoystick();
}

void UpdateForceFeedbackForce(int bReduceForce, int bEnableForce, int nForce) {
    coastline::RaceInput::Get().UpdateForce(bReduceForce, bEnableForce, nForce);
}

void SetForceFeedbackDirection(int nDirection) {
    coastline::RaceInput::Get().SetDirection(nDirection);
}

void SetForceFeedbackLongDuration(void) {
    coastline::RaceInput::Get().SetDuration(coastline::FORCE_FEEDBACK_LONG_DURATION);
}

void SetForceFeedbackShortDuration(void) {
    coastline::RaceInput::Get().SetDuration(coastline::FORCE_FEEDBACK_SHORT_DURATION);
}

void StartForceFeedbackJolt(void) {
    coastline::RaceInput::Get().StartJolt();
}
