#include "dinputcompat.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

#include <SDL3/SDL.h>

const DIDATAFORMAT c_dfDIKeyboard = {0};
const DIDATAFORMAT c_dfDIMouse = {1};
const DIDATAFORMAT c_dfDIJoystick2 = {2};
const GUID GUID_SysKeyboard = {
    0x6F1D2B61, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0, 0}};
const GUID GUID_SysMouse = {0x6F1D2B60, 0xD5A0, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0, 0}};
const GUID GUID_Square = {
    0x13541C22, 0x8E33, 0x11D0, {0x9A, 0xD0, 0x00, 0xA0, 0xC9, 0xA0, 0x6E, 0x35}};
const GUID GUID_Spring = {
    0x13541C27, 0x8E33, 0x11D0, {0x9A, 0xD0, 0x00, 0xA0, 0xC9, 0xA0, 0x6E, 0x35}};
const GUID IID_IDirectInputDevice2A = {
    0x5944E682, 0xC92E, 0x11CF, {0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0, 0}};
const GUID DIPROP_RANGE_GUID = {4, 0, 0, {0}};
const GUID DIPROP_DEADZONE_GUID = {5, 0, 0, {0}};
const GUID DIPROP_AUTOCENTER_GUID = {9, 0, 0, {0}};

namespace {

// The instance of the joystick, which is the first gamepad of SDL.
const GUID JOYSTICK_INSTANCE = {0x636F6173, 0x746C, 0x696E, {'g', 'a', 'm', 'e', 'p', 'a', 'd', 0}};

bool SameGuid(const GUID &a, const GUID &b) {
    return std::memcmp(&a, &b, sizeof(GUID)) == 0;
}

// The scan codes of DirectInput for the keys of SDL.
constexpr std::pair<SDL_Scancode, BYTE> KEY_CODES[] = {
    {SDL_SCANCODE_ESCAPE, 0x01},
    {SDL_SCANCODE_1, 0x02},
    {SDL_SCANCODE_2, 0x03},
    {SDL_SCANCODE_3, 0x04},
    {SDL_SCANCODE_4, 0x05},
    {SDL_SCANCODE_5, 0x06},
    {SDL_SCANCODE_6, 0x07},
    {SDL_SCANCODE_7, 0x08},
    {SDL_SCANCODE_8, 0x09},
    {SDL_SCANCODE_9, 0x0A},
    {SDL_SCANCODE_0, 0x0B},
    {SDL_SCANCODE_MINUS, 0x0C},
    {SDL_SCANCODE_EQUALS, 0x0D},
    {SDL_SCANCODE_BACKSPACE, 0x0E},
    {SDL_SCANCODE_TAB, 0x0F},
    {SDL_SCANCODE_Q, 0x10},
    {SDL_SCANCODE_W, 0x11},
    {SDL_SCANCODE_E, 0x12},
    {SDL_SCANCODE_R, 0x13},
    {SDL_SCANCODE_T, 0x14},
    {SDL_SCANCODE_Y, 0x15},
    {SDL_SCANCODE_U, 0x16},
    {SDL_SCANCODE_I, 0x17},
    {SDL_SCANCODE_O, 0x18},
    {SDL_SCANCODE_P, 0x19},
    {SDL_SCANCODE_LEFTBRACKET, 0x1A},
    {SDL_SCANCODE_RIGHTBRACKET, 0x1B},
    {SDL_SCANCODE_RETURN, 0x1C},
    {SDL_SCANCODE_LCTRL, 0x1D},
    {SDL_SCANCODE_A, 0x1E},
    {SDL_SCANCODE_S, 0x1F},
    {SDL_SCANCODE_D, 0x20},
    {SDL_SCANCODE_F, 0x21},
    {SDL_SCANCODE_G, 0x22},
    {SDL_SCANCODE_H, 0x23},
    {SDL_SCANCODE_J, 0x24},
    {SDL_SCANCODE_K, 0x25},
    {SDL_SCANCODE_L, 0x26},
    {SDL_SCANCODE_SEMICOLON, 0x27},
    {SDL_SCANCODE_APOSTROPHE, 0x28},
    {SDL_SCANCODE_GRAVE, 0x29},
    {SDL_SCANCODE_LSHIFT, 0x2A},
    {SDL_SCANCODE_BACKSLASH, 0x2B},
    {SDL_SCANCODE_Z, 0x2C},
    {SDL_SCANCODE_X, 0x2D},
    {SDL_SCANCODE_C, 0x2E},
    {SDL_SCANCODE_V, 0x2F},
    {SDL_SCANCODE_B, 0x30},
    {SDL_SCANCODE_N, 0x31},
    {SDL_SCANCODE_M, 0x32},
    {SDL_SCANCODE_COMMA, 0x33},
    {SDL_SCANCODE_PERIOD, 0x34},
    {SDL_SCANCODE_SLASH, 0x35},
    {SDL_SCANCODE_RSHIFT, 0x36},
    {SDL_SCANCODE_KP_MULTIPLY, 0x37},
    {SDL_SCANCODE_LALT, 0x38},
    {SDL_SCANCODE_SPACE, 0x39},
    {SDL_SCANCODE_CAPSLOCK, 0x3A},
    {SDL_SCANCODE_F1, 0x3B},
    {SDL_SCANCODE_F2, 0x3C},
    {SDL_SCANCODE_F3, 0x3D},
    {SDL_SCANCODE_F4, 0x3E},
    {SDL_SCANCODE_F5, 0x3F},
    {SDL_SCANCODE_F6, 0x40},
    {SDL_SCANCODE_F7, 0x41},
    {SDL_SCANCODE_F8, 0x42},
    {SDL_SCANCODE_F9, 0x43},
    {SDL_SCANCODE_F10, 0x44},
    {SDL_SCANCODE_NUMLOCKCLEAR, 0x45},
    {SDL_SCANCODE_SCROLLLOCK, 0x46},
    {SDL_SCANCODE_KP_7, 0x47},
    {SDL_SCANCODE_KP_8, 0x48},
    {SDL_SCANCODE_KP_9, 0x49},
    {SDL_SCANCODE_KP_MINUS, 0x4A},
    {SDL_SCANCODE_KP_4, 0x4B},
    {SDL_SCANCODE_KP_5, 0x4C},
    {SDL_SCANCODE_KP_6, 0x4D},
    {SDL_SCANCODE_KP_PLUS, 0x4E},
    {SDL_SCANCODE_KP_1, 0x4F},
    {SDL_SCANCODE_KP_2, 0x50},
    {SDL_SCANCODE_KP_3, 0x51},
    {SDL_SCANCODE_KP_0, 0x52},
    {SDL_SCANCODE_KP_PERIOD, 0x53},
    {SDL_SCANCODE_F11, 0x57},
    {SDL_SCANCODE_F12, 0x58},
    {SDL_SCANCODE_KP_ENTER, 0x9C},
    {SDL_SCANCODE_RCTRL, 0x9D},
    {SDL_SCANCODE_KP_DIVIDE, 0xB5},
    {SDL_SCANCODE_RALT, 0xB8},
    {SDL_SCANCODE_HOME, 0xC7},
    {SDL_SCANCODE_UP, 0xC8},
    {SDL_SCANCODE_PAGEUP, 0xC9},
    {SDL_SCANCODE_LEFT, 0xCB},
    {SDL_SCANCODE_RIGHT, 0xCD},
    {SDL_SCANCODE_END, 0xCF},
    {SDL_SCANCODE_DOWN, 0xD0},
    {SDL_SCANCODE_PAGEDOWN, 0xD1},
    {SDL_SCANCODE_INSERT, 0xD2},
    {SDL_SCANCODE_DELETE, 0xD3},
};

// The buttons of a joystick of the time for the buttons of an SDL gamepad: the four face buttons,
// the shoulders, select and start, the stick clicks, then the directional pad.
constexpr SDL_GamepadButton JOYSTICK_BUTTONS[] = {
    SDL_GAMEPAD_BUTTON_SOUTH,
    SDL_GAMEPAD_BUTTON_EAST,
    SDL_GAMEPAD_BUTTON_WEST,
    SDL_GAMEPAD_BUTTON_NORTH,
    SDL_GAMEPAD_BUTTON_LEFT_SHOULDER,
    SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
    SDL_GAMEPAD_BUTTON_BACK,
    SDL_GAMEPAD_BUTTON_START,
    SDL_GAMEPAD_BUTTON_LEFT_STICK,
    SDL_GAMEPAD_BUTTON_RIGHT_STICK,
    SDL_GAMEPAD_BUTTON_DPAD_UP,
    SDL_GAMEPAD_BUTTON_DPAD_DOWN,
    SDL_GAMEPAD_BUTTON_DPAD_LEFT,
    SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
};

SDL_Gamepad *FirstGamepad() {
    static SDL_Gamepad *gamepad = nullptr;
    if (gamepad != nullptr && SDL_GamepadConnected(gamepad)) {
        return gamepad;
    }
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    gamepad = count > 0 ? SDL_OpenGamepad(ids[0]) : nullptr;
    SDL_free(ids);
    return gamepad;
}

LONG ScaleAxis(Sint16 value, LONG min, LONG max) {
    double unit = (static_cast<double>(value) + 32768.0) / 65535.0;
    return static_cast<LONG>(min + unit * static_cast<double>(max - min));
}

// The SDL keys of the virtual keys the game asks GetAsyncKeyState() for.
SDL_Scancode VirtualKeyScancode(int key) {
    if (key >= 'A' && key <= 'Z') {
        return static_cast<SDL_Scancode>(SDL_SCANCODE_A + (key - 'A'));
    }
    if (key >= '1' && key <= '9') {
        return static_cast<SDL_Scancode>(SDL_SCANCODE_1 + (key - '1'));
    }
    if (key == '0') {
        return SDL_SCANCODE_0;
    }
    if (key == VK_ESCAPE) {
        return SDL_SCANCODE_ESCAPE;
    }
    return SDL_SCANCODE_UNKNOWN;
}

std::vector<int> g_scriptedKeys;

// Whether a key is down on the keyboard or held by SetScriptedKeys().
bool IsKeyDown(SDL_Scancode scancode) {
    return SDL_GetKeyboardState(nullptr)[scancode] ||
           std::ranges::find(g_scriptedKeys, static_cast<int>(scancode)) != g_scriptedKeys.end();
}

} // namespace

void coastline::SetScriptedKeys(std::vector<int> scancodes) {
    g_scriptedKeys = std::move(scancodes);
}

HRESULT IDirectInputEffect::SetParameters(const DIEFFECT *effect, DWORD flags) {
    if (effect->dwDuration != INFINITE_EFFECT) {
        m_durationMs = effect->dwDuration / 1000;
    }
    m_strength = static_cast<float>(effect->dwGain) / DI_FFNOMINALMAX;
    if (flags & DIEP_START) {
        Start(1, 0);
    }
    return DI_OK;
}

HRESULT IDirectInputEffect::Start(DWORD iterations, DWORD flags) {
    if (SDL_Gamepad *gamepad = FirstGamepad()) {
        auto level = static_cast<Uint16>(std::clamp(m_strength, 0.0f, 1.0f) * 0xFFFF);
        SDL_RumbleGamepad(gamepad, level, level, m_durationMs != 0 ? m_durationMs : 0xFFFF);
    }
    return DI_OK;
}

HRESULT IDirectInputEffect::Stop() {
    if (SDL_Gamepad *gamepad = FirstGamepad()) {
        SDL_RumbleGamepad(gamepad, 0, 0, 0);
    }
    return DI_OK;
}

ULONG IDirectInputEffect::Release() {
    delete this;
    return 0;
}

HRESULT IDirectInputDeviceA::GetDeviceState(DWORD size, LPVOID data) {
    std::memset(data, 0, size);
    if (m_kind == 0) {
        auto *state = static_cast<BYTE *>(data);
        for (const auto &[scancode, code] : KEY_CODES) {
            if (code < size && IsKeyDown(scancode)) {
                state[code] = 0x80;
            }
        }
    } else if (m_kind == 1) {
        auto *state = static_cast<DIMOUSESTATE *>(data);
        float x = 0;
        float y = 0;
        SDL_MouseButtonFlags buttons = SDL_GetRelativeMouseState(&x, &y);
        state->lX = static_cast<LONG>(x);
        state->lY = static_cast<LONG>(y);
        for (int i = 0; i < 4; ++i) {
            state->rgbButtons[i] = (buttons & SDL_BUTTON_MASK(i + 1)) ? 0x80 : 0;
        }
    } else if (SDL_Gamepad *gamepad = FirstGamepad()) {
        auto *state = static_cast<DIJOYSTATE2 *>(data);
        state->lX = ScaleAxis(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX), m_min, m_max);
        state->lY = ScaleAxis(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY), m_min, m_max);
        // The triggers share the z axis, the brake below the centre and the throttle above.
        int z = SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) -
                SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
        state->lZ = ScaleAxis(static_cast<Sint16>(std::clamp(z, -32768, 32767)), m_min, m_max);
        for (std::size_t i = 0; i < std::size(JOYSTICK_BUTTONS); ++i) {
            state->rgbButtons[i] = SDL_GetGamepadButton(gamepad, JOYSTICK_BUTTONS[i]) ? 0x80 : 0;
        }
        for (DWORD &pov : state->rgdwPOV) {
            pov = 0xFFFFFFFF;
        }
    }
    return DI_OK;
}

HRESULT IDirectInputDeviceA::SetDataFormat(const DIDATAFORMAT *format) {
    m_kind = format->nKind;
    return DI_OK;
}

HRESULT IDirectInputDeviceA::SetProperty(REFGUID property, const DIPROPHEADER *header) {
    if (SameGuid(property, DIPROP_RANGE)) {
        const auto *range = reinterpret_cast<const DIPROPRANGE *>(header);
        m_min = range->lMin;
        m_max = range->lMax;
    }
    return DI_OK;
}

HRESULT IDirectInputDeviceA::QueryInterface(REFGUID iid, void **object) {
    *object = static_cast<IDirectInputDevice2A *>(this);
    return DI_OK;
}

ULONG IDirectInputDeviceA::Release() {
    delete this;
    return 0;
}

HRESULT IDirectInputDevice2A::CreateEffect(REFGUID type,
                                           const DIEFFECT *parameters,
                                           IDirectInputEffect **effect,
                                           void *outer) {
    *effect = new IDirectInputEffect();
    (*effect)->SetParameters(parameters, 0);
    return DI_OK;
}

HRESULT IDirectInputA::CreateDevice(REFGUID guid, IDirectInputDeviceA **device, void *outer) {
    // Every device is an IDirectInputDevice2A, so the second interface is the same object.
    *device = new IDirectInputDevice2A();
    if (SameGuid(guid, GUID_SysMouse)) {
        (*device)->SetDataFormat(&c_dfDIMouse);
    } else if (SameGuid(guid, JOYSTICK_INSTANCE)) {
        (*device)->SetDataFormat(&c_dfDIJoystick2);
    }
    return DI_OK;
}

HRESULT IDirectInputA::EnumDevices(DWORD type,
                                   LPDIENUMDEVICESCALLBACKA callback,
                                   LPVOID context,
                                   DWORD flags) {
    SDL_Gamepad *gamepad = type == DIDEVTYPE_JOYSTICK ? FirstGamepad() : nullptr;
    if (gamepad == nullptr) {
        return DI_OK;
    }
    DIDEVICEINSTANCEA instance{};
    instance.dwSize = sizeof(instance);
    instance.guidInstance = JOYSTICK_INSTANCE;
    instance.guidProduct = JOYSTICK_INSTANCE;
    instance.dwDevType = DIDEVTYPE_JOYSTICK;
    SDL_strlcpy(
        instance.tszInstanceName, SDL_GetGamepadName(gamepad), sizeof(instance.tszInstanceName));
    SDL_strlcpy(
        instance.tszProductName, SDL_GetGamepadName(gamepad), sizeof(instance.tszProductName));
    callback(&instance, context);
    return DI_OK;
}

ULONG IDirectInputA::Release() {
    delete this;
    return 0;
}

HRESULT DirectInputCreateA(HINSTANCE instance, DWORD version, IDirectInputA **input, void *outer) {
    *input = new IDirectInputA();
    return DI_OK;
}

short GetAsyncKeyState(int key) {
    static std::array<bool, SDL_SCANCODE_COUNT> previous{};
    SDL_Scancode scancode = VirtualKeyScancode(key);
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        return 0;
    }
    bool down = IsKeyDown(scancode);
    bool pressed = down && !previous[scancode];
    previous[scancode] = down;
    return static_cast<short>((down ? 0x8000 : 0) | (pressed ? 1 : 0));
}
