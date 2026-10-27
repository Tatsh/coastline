#include "platform/win/input.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <new>

#include <hud.h>
#include <windows.h>

#include "game.h"
#include "instantreplay.h"
#include "laps.h"
#include "pause.h"
#include "platform/win/boot/app.h"
#include "platform/win/boot/rallyconfig.h"
#include "platform/win/boot/strings.h"
#include "platform/win/directplay.h"
#include "platform/win/frontend.h"
#include "platform/win/ghost.h"
#include "platform/win/hud.h"
#include "platform/win/settings.h"
#include "platform/win/snapshot.h"
#include "platform/win/sound.h"
#include "platform/win/timer.h"
#include "playerinput.h"
#include "vehicle.h"
#include "vehicleaccess.h"

MouseDevice *g_pMouseDevice;

namespace {

/**
 * DirectInput.
 *
 * @ghidraAddress BRD3D: 0x118abd70
 * @ghidraAddress BRGlide: 0x118eee88
 */
IDirectInputA *g_pDirectInput;

/**
 * The users of DirectInput. The last ShutdownDirectInput() releases it.
 *
 * @ghidraAddress BRD3D: 0x118abde8
 * @ghidraAddress BRGlide: 0x118eef00
 */
int g_dwDirectInputRefCount;

/**
 * The DirectInput device of the keyboard.
 *
 * @ghidraAddress BRD3D: 0x118abdd0
 * @ghidraAddress BRGlide: 0x118eeee8
 */
IDirectInputDeviceA *g_pKeyboardDevice;

/**
 * The users of the keyboard device.
 *
 * @ghidraAddress BRD3D: 0x118abdd8
 * @ghidraAddress BRGlide: 0x118eeef0
 */
int g_dwKeyboardDeviceRefCount;

/**
 * The DirectInput device of the game controller.
 *
 * @ghidraAddress BRD3D: 0x118abdd4
 * @ghidraAddress BRGlide: 0x118eeeec
 */
IDirectInputDevice2A *g_pJoystickDevice;

/**
 * The users of the game controller device.
 *
 * @ghidraAddress BRD3D: 0x118abe00
 * @ghidraAddress BRGlide: 0x118eef18
 */
int g_dwJoystickDeviceRefCount;

/**
 * The jolt effect of force feedback of the game controller.
 *
 * @ghidraAddress BRD3D: 0x118abdfc
 * @ghidraAddress BRGlide: 0x118eef14
 */
IDirectInputEffect *g_pJoltEffect;

/**
 * The condition effect of force feedback of the game controller.
 *
 * @ghidraAddress BRD3D: 0x118abdec
 * @ghidraAddress BRGlide: 0x118eef04
 */
IDirectInputEffect *g_pConditionEffect;

/**
 * The two states of the keyboard, the current one and the one before.
 *
 * @ghidraAddress BRD3D: 0x118ab8b8
 * @ghidraAddress BRGlide: 0x118ee9d0
 */
unsigned char g_aabKeyboardStateBuffers[2][KEYBOARD_KEY_COUNT];

/**
 * The entry of g_aabKeyboardStateBuffers that is current.
 *
 * @ghidraAddress BRD3D: 0x118abad8
 * @ghidraAddress BRGlide: 0x118eebf0
 */
int g_dwKeyboardStateCurIndex;

/**
 * The entry of g_aabKeyboardStateBuffers from before.
 *
 * @ghidraAddress BRD3D: 0x118ab8b4
 * @ghidraAddress BRGlide: 0x118ee9cc
 */
int g_dwKeyboardStatePrevIndex;

/**
 * The two states of the game controller, the current one and the one before.
 *
 * @ghidraAddress BRD3D: 0x118abae0
 * @ghidraAddress BRGlide: 0x118eebf8
 */
DIJOYSTATE2 g_aJoystickStateBuffers[2];

/**
 * The entry of g_aJoystickStateBuffers that is current.
 *
 * @ghidraAddress BRD3D: 0x118abab8
 * @ghidraAddress BRGlide: 0x118eebd0
 */
int g_dwJoystickStateCurIndex;

/**
 * The entry of g_aJoystickStateBuffers from before.
 *
 * @ghidraAddress BRD3D: 0x118abd7c
 * @ghidraAddress BRGlide: 0x118eee94
 */
int g_dwJoystickStatePrevIndex;

/**
 * The two states of the mouse, the current one and the one before.
 *
 * @ghidraAddress BRD3D: 0x118abd38
 * @ghidraAddress BRGlide: 0x118eee50
 */
MouseInputState g_aMouseStateBuffers[2];

/**
 * The entry of g_aMouseStateBuffers that is current.
 *
 * @ghidraAddress BRD3D: 0x118abd80
 * @ghidraAddress BRGlide: 0x118eee98
 */
int g_dwMouseStateCurIndex;

/**
 * The entry of g_aMouseStateBuffers from before.
 *
 * @ghidraAddress BRD3D: 0x118abad4
 * @ghidraAddress BRGlide: 0x118eebec
 */
int g_dwMouseStatePrevIndex;

/**
 * Whether each key was down when the keyboard was read before.
 *
 * @ghidraAddress BRD3D: 0x10aa2e88
 * @ghidraAddress BRGlide: 0x10ac61e8
 */
int g_adwKeyPrevDown[KEYBOARD_KEY_COUNT];

/**
 * Input poll count, saturated at 32767.
 * @ghidraAddress BRD3D: 0x118abddc
 */
int g_dwInputPollCounter;

/**
 * Initial timestamp for benchmark frame rate measurement, in milliseconds.
 * @ghidraAddress BRD3D: 0x118abd00
 */
DWORD g_dwBenchmarkStartTime;

/**
 * Initial interpolated frame count for benchmark measurement.
 * @ghidraAddress BRD3D: 0x118abd74
 */
int g_dwBenchmarkStartFrameCount;

/**
 * Whether each button of the mouse was down when the mouse was read before.
 *
 * @ghidraAddress BRD3D: 0x10aa3388
 * @ghidraAddress BRGlide: 0x10ac66e8
 */
int g_adwMouseButtonPrev[MOUSE_BUTTON_COUNT];

/**
 * The width of the screen the cursor of the mouse moves in. Its name is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x10aa33b8
 * @ghidraAddress BRGlide: 0x10ac6718
 */
int g_dwEntityHalfWidth;

/**
 * The height of the screen the cursor of the mouse moves in. Its name is not recovered yet.
 *
 * @ghidraAddress BRD3D: 0x10aa33b4
 * @ghidraAddress BRGlide: 0x10ac6714
 */
int g_dwEntityHalfHeight;

/**
 * The state of the menus the mouse and the arrows of the keyboard move.
 *
 * @ghidraAddress BRD3D: 0x10aa3398
 * @ghidraAddress BRGlide: 0x10ac66f8
 */
MouseMenuState g_MouseMenuState;

/**
 * The condition effect of force feedback, a spring on X and Y.
 *
 * @ghidraAddress BRD3D: 0x118ab880
 * @ghidraAddress BRGlide: 0x118ee998
 */
DIEFFECT g_DiConditionEffect;

/**
 * The springs of X and Y of g_DiConditionEffect.
 *
 * @ghidraAddress BRD3D: 0x118abd08
 * @ghidraAddress BRGlide: 0x118eee20
 */
DICONDITION g_aDiConditionParams[2];

/**
 * The direction of g_DiConditionEffect.
 *
 * @ghidraAddress BRD3D: 0x118abdf0
 * @ghidraAddress BRGlide: 0x118eef08
 */
LONG g_alConditionEffectDirection[2];

/**
 * The strength of the spring of X once the condition effect exists.
 *
 * @ghidraAddress BRD3D: 0x118abd78
 * @ghidraAddress BRGlide: 0x118eee90
 */
int g_dwForceFeedbackMagnitude;

/**
 * Whether the condition effect does not play. Set once the effect is created.
 *
 * @ghidraAddress BRD3D: 0x118abdf8
 * @ghidraAddress BRGlide: 0x118eef10
 */
int g_bForceFeedbackForceStopped;

/**
 * The jolt effect of force feedback, a square wave.
 *
 * @ghidraAddress BRD3D: 0x118abd88
 * @ghidraAddress BRGlide: 0x118eeea0
 */
DIEFFECT g_DiJoltEffect;

/**
 * The square wave of g_DiJoltEffect.
 *
 * @ghidraAddress BRD3D: 0x118abac0
 * @ghidraAddress BRGlide: 0x118eebd8
 */
DIPERIODIC g_DiJoltPeriodic;

/**
 * The duration of the jolt in microseconds, 125000.
 *
 * @ghidraAddress BRD3D: 0x100bd438
 * @ghidraAddress BRGlide: 0x100bcc40
 */
DWORD g_dwForceFeedbackDuration = 125000;

/**
 * Gain used in spring ramp calculations, in ten-thousandths.
 *
 * @ghidraAddress BRD3D: 0x100bd424
 * @ghidraAddress BRGlide: 0x100bcc2c
 */
int g_dwForceFeedbackGain = 10000;

/**
 * Lower bound of the spring ramp before gain scaling.
 *
 * @ghidraAddress BRD3D: 0x100bd428
 * @ghidraAddress BRGlide: 0x100bcc30
 */
int g_dwForceFeedbackMagnitudeMax = 2000;

/**
 * Upper bound of the spring ramp before gain scaling.
 *
 * @ghidraAddress BRD3D: 0x100bd42c
 * @ghidraAddress BRGlide: 0x100bcc34
 */
int g_dwForceFeedbackConstantForce = 10000;

/**
 * The time of the console the game emulates, in ticks of its CPU counter.
 *
 * @ghidraAddress BRD3D: 0x118abde0
 * @ghidraAddress BRGlide: 0x118eeef8
 */
u64 g_qwEmulatedOsTime;

} // namespace

unsigned char g_abKeyboardState[KEYBOARD_KEY_COUNT];
int g_adwKeyPressedEdge[KEYBOARD_KEY_COUNT];

static inline bool IsButtonPressedEdge(unsigned char previous, unsigned char current) {
    return !(previous & 0x80) && (current & 0x80);
}

static inline LONG GetJoystickBindingAxis(const DIJOYSTATE2 &state, unsigned nAxis) {
    switch (nAxis) {
    case 0:
        return state.lX;
    case 1:
        return state.lY;
    default:
        return state.lZ;
    }
}

static inline LONG GetMouseBindingAxis(const MouseInputState &state, unsigned nAxis) {
    switch (nAxis) {
    case 0:
        return state.lX;
    case 1:
        return state.lY;
    default:
        return state.lZ;
    }
}

unsigned char IsInputBindingHeld(int nBinding) {
    const InputBinding &binding = g_GlobalRallyConfig.pActiveInputBindings[nBinding];
    const unsigned nKind = binding.wControl & 0xff00;
    const unsigned nButton = binding.wControl & 0xff;
    unsigned char result = 0;

    switch (nKind) {
    case 0:
        result = g_aabKeyboardStateBuffers[g_dwKeyboardStateCurIndex][nButton] & 0x80;
        break;
    case 0x100:
        result = g_aJoystickStateBuffers[g_dwJoystickStateCurIndex].rgbButtons[nButton] & 0x80;
        break;
    case 0x300:
        result = g_aMouseStateBuffers[g_dwMouseStateCurIndex].abButtons[nButton] & 0x80;
        break;
    default:
        if (nKind >= 0x8000 && nKind <= 0x8b00) {
            const unsigned nAxis = (nKind - 0x8000) >> 9;
            const LONG current =
                nAxis < 3 ?
                    GetJoystickBindingAxis(g_aJoystickStateBuffers[g_dwJoystickStateCurIndex],
                                           nAxis) :
                    GetMouseBindingAxis(g_aMouseStateBuffers[g_dwMouseStateCurIndex], nAxis - 3);
            result = ((nKind & 0x100) ? current > 50 : current < -50) ? 0x80 : 0;
        }
        break;
    }
    for (unsigned short control : binding.awAlternateControls) {
        if ((control & 0xff00) == 0) {
            result |= g_aabKeyboardStateBuffers[g_dwKeyboardStateCurIndex][control & 0xff] & 0x80;
        }
    }
    return result;
}

unsigned char IsInputBindingPressed(int nBinding) {
    const InputBinding &binding = g_GlobalRallyConfig.pActiveInputBindings[nBinding];
    const unsigned nKind = binding.wControl & 0xff00;
    const unsigned nButton = binding.wControl & 0xff;
    unsigned char result = 0;

    switch (nKind) {
    case 0:
        result = IsButtonPressedEdge(g_aabKeyboardStateBuffers[g_dwKeyboardStatePrevIndex][nButton],
                                     g_aabKeyboardStateBuffers[g_dwKeyboardStateCurIndex][nButton]);
        break;
    case 0x100:
        result = IsButtonPressedEdge(
            g_aJoystickStateBuffers[g_dwJoystickStatePrevIndex].rgbButtons[nButton],
            g_aJoystickStateBuffers[g_dwJoystickStateCurIndex].rgbButtons[nButton]);
        break;
    case 0x300:
        result =
            IsButtonPressedEdge(g_aMouseStateBuffers[g_dwMouseStatePrevIndex].abButtons[nButton],
                                g_aMouseStateBuffers[g_dwMouseStateCurIndex].abButtons[nButton]);
        break;
    default:
        if (nKind >= 0x8000 && nKind <= 0x8b00) {
            const unsigned nAxis = (nKind - 0x8000) >> 9;
            const LONG previous =
                nAxis < 3 ?
                    GetJoystickBindingAxis(g_aJoystickStateBuffers[g_dwJoystickStatePrevIndex],
                                           nAxis) :
                    GetMouseBindingAxis(g_aMouseStateBuffers[g_dwMouseStatePrevIndex], nAxis - 3);
            const LONG current =
                nAxis < 3 ?
                    GetJoystickBindingAxis(g_aJoystickStateBuffers[g_dwJoystickStateCurIndex],
                                           nAxis) :
                    GetMouseBindingAxis(g_aMouseStateBuffers[g_dwMouseStateCurIndex], nAxis - 3);
            if (nKind & 0x100) {
                result = previous <= 50 && current > 50 ? 0x80 : 0;
            } else {
                result = previous >= -50 && current < -50 ? 0x80 : 0;
            }
        }
        break;
    }
    for (unsigned short control : binding.awAlternateControls) {
        if ((control & 0xff00) == 0) {
            result |= IsButtonPressedEdge(
                g_aabKeyboardStateBuffers[g_dwKeyboardStatePrevIndex][control & 0xff],
                g_aabKeyboardStateBuffers[g_dwKeyboardStateCurIndex][control & 0xff]);
        }
    }
    return result;
}

unsigned g_dwInputButtonMask;
int g_bRespawnRequested;
int g_bNetReadyToStartSent;
int g_bNetGamePaused;
int g_anMouseAxisRanges[8] = {803, 618, 475, 366, 281, 216, 166, 128};

static inline void ClampMouseAxis(LONG &axis, LONG &accumulated, int range) {
    axis = static_cast<int>(static_cast<unsigned>(accumulated) * 128) / range;
    if (axis < -128) {
        axis = -128;
        accumulated = -range;
    } else if (axis > 128) {
        axis = 128;
        accumulated = range;
    }
}

static inline void ReadMouseBindingState(void) {
    g_dwMouseStatePrevIndex = g_dwMouseStateCurIndex;
    g_dwMouseStateCurIndex = (g_dwMouseStateCurIndex - 1) & 1;
    MouseInputState &state = g_aMouseStateBuffers[g_dwMouseStateCurIndex];
    if (g_pMouseDevice == nullptr || g_pMouseDevice->GetDirectInputDevice() == nullptr) {
        memset(&state, 0, sizeof(state));
        return;
    }
    DIMOUSESTATE mouse;
    const HRESULT result =
        g_pMouseDevice->GetDirectInputDevice()->GetDeviceState(sizeof(mouse), &mouse);
    if (result != DI_OK) {
        if (result == DIERR_INPUTLOST) {
            g_pMouseDevice->GetDirectInputDevice()->Acquire();
        }
        memset(state.abButtons, 0, sizeof(state.abButtons));
        return;
    }
    const MouseInputState &previous = g_aMouseStateBuffers[g_dwMouseStatePrevIndex];
    state.lAccumulatedX = previous.lAccumulatedX + mouse.lX;
    // All three accumulators use the previous horizontal accumulator in the binary.
    state.lAccumulatedY = previous.lAccumulatedX + mouse.lY;
    state.lAccumulatedZ = previous.lAccumulatedX + mouse.lZ;
    int sensitivity = VEHICLE_GET(&(g_aVehicles[0]), SteeringMode);
    if (sensitivity < 0) {
        sensitivity = 0;
    } else if (sensitivity > 7) {
        sensitivity = 7;
    }
    const int range = g_anMouseAxisRanges[sensitivity];
    ClampMouseAxis(state.lX, state.lAccumulatedX, range);
    ClampMouseAxis(state.lY, state.lAccumulatedY, range);
    ClampMouseAxis(state.lZ, state.lAccumulatedZ, range);
    memcpy(state.abButtons, mouse.rgbButtons, sizeof(state.abButtons));
}

static inline int ReadBindingAxisMagnitude(int nBinding) {
    const unsigned kind = g_GlobalRallyConfig.pActiveInputBindings[nBinding].wControl & 0xff00;
    if (kind < 0x8000 || kind > 0x8b00) {
        return -1;
    }
    const unsigned axis = (kind - 0x8000) >> 9;
    const LONG value =
        axis < 3 ?
            GetJoystickBindingAxis(g_aJoystickStateBuffers[g_dwJoystickStateCurIndex], axis) :
            GetMouseBindingAxis(g_aMouseStateBuffers[g_dwMouseStateCurIndex], axis - 3);
    if (kind & 0x100) {
        return value > 0 ? value * 80 / 128 : -1;
    }
    return value < 0 ? -value * 80 / 128 : -1;
}

unsigned ReadInputDevicesButtonMask(int *pnSteering, int *pnAcceleration) {
    if (g_dwInputPollCounter < 0x7fff) {
        ++g_dwInputPollCounter;
    }
    g_dwKeyboardStatePrevIndex = g_dwKeyboardStateCurIndex;
    g_dwKeyboardStateCurIndex = (g_dwKeyboardStateCurIndex - 1) & 1;
    unsigned char *keys = g_aabKeyboardStateBuffers[g_dwKeyboardStateCurIndex];
    HRESULT result = g_pKeyboardDevice->GetDeviceState(KEYBOARD_KEY_COUNT, keys);
    if (FAILED(result)) {
        if (result != DIERR_INPUTLOST || FAILED(g_pKeyboardDevice->Acquire()) ||
            FAILED(g_pKeyboardDevice->GetDeviceState(KEYBOARD_KEY_COUNT, keys))) {
            g_dwInputButtonMask = 0;
            return 0;
        }
    }
    keys[0] = 0;
    if (g_GlobalRallyConfig.dwInputDeviceType == 1 || g_GlobalRallyConfig.dwInputDeviceType == 2) {
        g_dwJoystickStatePrevIndex = g_dwJoystickStateCurIndex;
        g_dwJoystickStateCurIndex = (g_dwJoystickStateCurIndex - 1) & 1;
        g_pJoystickDevice->Poll();
        DIJOYSTATE2 &joystick = g_aJoystickStateBuffers[g_dwJoystickStateCurIndex];
        result = g_pJoystickDevice->GetDeviceState(sizeof(joystick), &joystick);
        if (result != DI_OK) {
            if (result == DIERR_INPUTLOST) {
                TryAcquireJoystickDevice();
            }
            memset(joystick.rgbButtons, 0, sizeof(joystick.rgbButtons));
        }
    }
    ReadMouseBindingState();
    if (IsInputBindingPressed(15)) {
        if (!g_dwNetworkPlayMode || !g_bNetRaceStartReached || gInstantReplayActive ||
            VEHICLE_GET(&(g_aVehicles[0]), Lap) >= g_dwTotalLaps) {
            g_bNetGamePaused = 1;
            g_dwInputButtonMask = 0x4000;
            return g_dwInputButtonMask;
        }
        RequestNetworkPause();
    }
    int *toggles[6] = {&g_bHudShowLapCounter,
                       &g_bHudShowRaceTimer,
                       &g_bHudShowLapTimes,
                       &g_bHudShowSpeedometer,
                       &g_bHudShowPosition,
                       &g_bHudShowNetworkStandings};
    for (int i = 0; i < 6; ++i) {
        if (IsButtonPressedEdge(g_aabKeyboardStateBuffers[g_dwKeyboardStatePrevIndex][DIK_F5 + i],
                                keys[DIK_F5 + i])) {
            *toggles[i] = !*toggles[i];
        }
    }
    if (IsButtonPressedEdge(g_aabKeyboardStateBuffers[g_dwKeyboardStatePrevIndex][DIK_F11],
                            keys[DIK_F11])) {
        SelectPreviousMusicTrack();
    }
    if (IsButtonPressedEdge(g_aabKeyboardStateBuffers[g_dwKeyboardStatePrevIndex][DIK_F12],
                            keys[DIK_F12])) {
        SelectNextMusicTrack();
    }
    if ((GetAsyncKeyState('F') & 1) && g_dwInputPollCounter > 15) {
        g_bShowFpsCounter = !g_bShowFpsCounter;
    }
    if ((GetAsyncKeyState('P') & 1) && g_dwInputPollCounter > 15) {
        g_bHideNetStandingsHud = !g_bHideNetStandingsHud;
    }
    unsigned mask = 0;
    if (gInstantReplayActive) {
        if (gInstantReplayActive == 2) {
            for (int binding = 8; binding <= 10; ++binding) {
                if (IsInputBindingPressed(binding)) {
                    mask |= 1u << binding;
                }
            }
        }
        if (IsInputBindingPressed(11)) {
            mask |= 0x800;
        }
        if (IsInputBindingPressed(21)) {
            mask |= 0x100400;
        }
        for (int binding = 22; binding <= 25; ++binding) {
            if (IsInputBindingHeld(binding)) {
                mask |= 1u << (binding - 1);
            }
        }
        if (IsInputBindingPressed(26)) {
            mask |= 0x200000;
        }
        if (IsInputBindingPressed(27)) {
            mask |= 0x400000;
        }
        g_dwInputButtonMask = mask;
        return mask;
    }
    if (IsInputBindingPressed(16)) {
        if (!g_dwRacePauseState && g_dwGameMode != GAME_MODE_DEMO &&
            g_dwGameMode != GAME_MODE_AFTER_SEASON) {
            g_bRespawnRequested = 1;
        }
        if (g_dwNetworkPlayMode) {
            if (!g_bNetReadyToStartSent) {
                SetNetworkReadyToStartFlag();
                g_bNetReadyToStartSent = 1;
            }
        } else {
            g_bNetRaceStartReached = 1;
            if (g_dwGameMode == GAME_MODE_TIME_ATTACK) {
                StartGhostRecording();
            }
        }
    }
    if (IsInputBindingPressed(14)) {
        mask = 0x8000;
    }
    *pnSteering = 0;
    if (g_dwGameMode != GAME_MODE_DEMO && g_dwGameMode != GAME_MODE_AFTER_SEASON) {
        const int left = ReadBindingAxisMagnitude(0);
        if (left >= 0) {
            *pnSteering = -left;
        }
        const int right = ReadBindingAxisMagnitude(1);
        if (right >= 0) {
            *pnSteering = right;
        }
    }
    if (g_dwRacePauseState) {
        for (int binding : {12, 13, 0, 1, 2}) {
            if (IsInputBindingPressed(binding)) {
                mask |= 1u << binding;
            }
        }
    } else {
        *pnAcceleration = 0;
        if (IsInputBindingHeld(2)) {
            mask |= 4;
            if (!(g_GlobalRallyConfig.pActiveInputBindings[2].wControl & 0x8000)) {
                *pnAcceleration = 80;
            }
        }
        const int acceleration = ReadBindingAxisMagnitude(2);
        if (acceleration >= 0) {
            *pnAcceleration = acceleration;
        }
        if (IsInputBindingHeld(3)) {
            mask |= 8;
        }
        if (IsInputBindingHeld(4)) {
            mask |= 16;
            *pnAcceleration = -80;
        }
        if (g_dwGameMode != GAME_MODE_DEMO && g_dwGameMode != GAME_MODE_AFTER_SEASON) {
            for (int binding : {0, 1}) {
                if (IsInputBindingHeld(binding)) {
                    mask |= 1u << binding;
                }
            }
        }
        for (int binding : {5, 6, 7}) {
            if (IsInputBindingHeld(binding)) {
                mask |= 1u << binding;
            }
        }
        for (int binding : {8, 9, 10, 17, 18, 19, 20}) {
            if (IsInputBindingPressed(binding)) {
                mask |= 1u << (binding >= 17 ? binding - 1 : binding);
            }
        }
        if (g_dwRunBenchmark) {
            if (g_dwInputPollCounter == 1) {
                g_dwBenchmarkStartTime = GetHighResolutionTime();
                g_dwBenchmarkStartFrameCount = GetInterpolatedFrameCount();
            }
            if (g_dwInputPollCounter == 441) {
                const DWORD now = GetHighResolutionTime();
                const DWORD elapsed = now - g_dwBenchmarkStartTime;
                g_dwBenchmarkStartTime = now;
                const int frames = GetInterpolatedFrameCount() - g_dwBenchmarkStartFrameCount;
                char message[256];
                sprintf(message, "fps = %0.2f", frames * 1000.0 / static_cast<int>(elapsed));
                exit(1);
                ShowFatalErrorScreen(message);
            }
            GetAsyncKeyState(VK_ESCAPE);
            if (g_dwInputPollCounter == 1) {
                mask = 0x400;
                g_bShowFpsCounter = 1;
            } else {
                mask = g_dwInputPollCounter < 60 ? 0 : 4;
            }
        }
    }
    g_dwInputButtonMask = mask;
    return mask;
}

// The bit of a key in the state of DirectInput that is set while the key is down.
#define KEY_DOWN_BIT 7
// The entries of the edges of the keys ClearKeyboardStateBuffers() clears.
#define KEY_EDGE_CLEAR_COUNT 64

int g_adwMouseButtonRaw[MOUSE_BUTTON_COUNT];
int g_adwMouseButtonPressed[MOUSE_BUTTON_COUNT];
int g_fCharInputSuppressed;
short g_dwMouseScrollDirection = 1;
unsigned short g_wMenuHoverIndex;
DWORD g_dwLastInputActivityTime;
MouseDevice *g_pMousePosition;

void PlayerInputState::ConsumeInputFlags(void) {
    ::ConsumeInputFlags(this);
}

void PlayerInputState::TransferFlags(u32 mask) {
    ::TransferFlags(this, mask);
}

void PlayerInputState::InitPlayerInputState(void) {
    ::InitPlayerInputState(this);
}

void UpdateMouseButtonEdges(void) {
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        g_adwMouseButtonPressed[i] = (g_adwMouseButtonPrev[i] == 0) & g_adwMouseButtonRaw[i];
        g_adwMouseButtonPrev[i] = g_adwMouseButtonRaw[i];
    }
}

bool WINAPI ResetMouseCursorToCenter(HWND) {
    g_dwEntityHalfWidth = g_dwConfigFramebufferWidth;
    g_pMouseDevice->SetCursorX(g_dwConfigFramebufferWidth / 2);
    g_dwEntityHalfHeight = g_dwConfigFramebufferHeight;
    g_pMouseDevice->SetCursorY(g_dwConfigFramebufferHeight / 2);
    memset(&g_MouseMenuState, 0, sizeof(g_MouseMenuState));
    return true;
}

// The input device type of the first game controller.
#define INPUT_DEVICE_FIRST_CONTROLLER 2
// The bit of a button in a state of DirectInput that is set while the button is down.
#define BUTTON_DOWN_MASK 0x80
// The distance an axis is pushed for GetPressedJoystickControl() to report it.
#define JOYSTICK_AXIS_THRESHOLD 50

InputControlName g_aJoystickButtonNamesA[JOYSTICK_NAME_COUNT_A];
InputControlName g_aJoystickButtonNamesB[JOYSTICK_NAME_COUNT_B];

// The bytes of each table of names that InitJoystickBindingNames() clears.
#define JOYSTICK_NAMES_CLEARED_A 0xdc
#define JOYSTICK_NAMES_CLEARED_B 0x158
// The buttons named in each table, and the first code of its axes.
#define JOYSTICK_BUTTON_COUNT_A 4
#define JOYSTICK_AXIS_CODE_A 0x86
#define JOYSTICK_BUTTON_COUNT_B 128
#define JOYSTICK_AXIS_CODE_B 0x80
// The string of the format of the name of a button, and the first string of an axis.
#define JOYSTICK_BUTTON_NAME_STRING 0xc3
#define JOYSTICK_AXIS_NAME_STRING 0xc4

static inline void
FillJoystickNames(InputControlName *pNames, int nCount, int nButtonCount, int nAxisCode) {
    for (int i = 0; i < nCount; i++) {
        if (i < nButtonCount) {
            pNames[i].nCode = i;
            sprintf(pNames[i].szName, GetStringResource(JOYSTICK_BUTTON_NAME_STRING), i);
        } else {
            pNames[i].nCode = nAxisCode + (i - nButtonCount);
            // The original passes the name to sprintf() as the format.
            sprintf(pNames[i].szName,
                    "%s",
                    GetStringResource(JOYSTICK_AXIS_NAME_STRING + (i - nButtonCount)));
        }
    }
}

void InitJoystickBindingNames(void) {
    memset(g_aJoystickButtonNamesA, 0, JOYSTICK_NAMES_CLEARED_A);
    memset(g_aJoystickButtonNamesB, 0, JOYSTICK_NAMES_CLEARED_B);
    FillJoystickNames(g_aJoystickButtonNamesA,
                      JOYSTICK_NAME_COUNT_A,
                      JOYSTICK_BUTTON_COUNT_A,
                      JOYSTICK_AXIS_CODE_A);
    FillJoystickNames(g_aJoystickButtonNamesB,
                      JOYSTICK_NAME_COUNT_B,
                      JOYSTICK_BUTTON_COUNT_B,
                      JOYSTICK_AXIS_CODE_B);
}

int GetPressedMouseButton(void) {
    DIMOUSESTATE state;
    HRESULT hResult;

    if (g_pMouseDevice == nullptr || g_pMouseDevice->GetDirectInputDevice() == nullptr) {
        return -1;
    }
    for (int &bDown : g_adwMouseButtonRaw) {
        bDown = 0;
    }
    hResult = g_pMouseDevice->GetDirectInputDevice()->GetDeviceState(sizeof(state), &state);
    // A lost device is acquired again and the state that was not read is used.
    if (hResult == DIERR_INPUTLOST) {
        g_pMouseDevice->GetDirectInputDevice()->Acquire();
    }
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        if (state.rgbButtons[i] & BUTTON_DOWN_MASK) {
            g_adwMouseButtonRaw[i] = 1;
        }
    }
    UpdateMouseButtonEdges();
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        if (g_adwMouseButtonPressed[i] != 0) {
            return i;
        }
    }
    return -1;
}

int GetPressedJoystickControl(unsigned *pwControl) {
    DIJOYSTATE2 state;
    HRESULT hResult;

    if (g_pJoystickDevice == nullptr) {
        g_GlobalRallyConfig.dwInputDeviceType = INPUT_DEVICE_FIRST_CONTROLLER;
        g_GlobalRallyConfig.SelectActiveInputBindings();
        InitializeJoystickDevice();
        if (g_pJoystickDevice == nullptr) {
            return 1;
        }
    }
    g_pJoystickDevice->Poll();
    hResult = g_pJoystickDevice->GetDeviceState(sizeof(state), &state);
    if (hResult == DIERR_INPUTLOST) {
        TryAcquireJoystickDevice();
    }
    for (int i = 0; i < static_cast<int>(sizeof(state.rgbButtons)); i++) {
        if (state.rgbButtons[i] & BUTTON_DOWN_MASK) {
            *pwControl = JOYSTICK_CONTROL_BUTTON;
            return i;
        }
    }
    if (state.lX < -JOYSTICK_AXIS_THRESHOLD) {
        *pwControl = JOYSTICK_CONTROL_X_NEGATIVE;
    } else if (state.lX > JOYSTICK_AXIS_THRESHOLD) {
        *pwControl = JOYSTICK_CONTROL_X_POSITIVE;
    } else if (state.lY < -JOYSTICK_AXIS_THRESHOLD) {
        *pwControl = JOYSTICK_CONTROL_Y_NEGATIVE;
    } else if (state.lY > JOYSTICK_AXIS_THRESHOLD) {
        *pwControl = JOYSTICK_CONTROL_Y_POSITIVE;
    } else {
        return -1;
    }
    return 0;
}

void ResetMouseState(void) {
    memset(g_aMouseStateBuffers, 0, sizeof(g_aMouseStateBuffers));
    g_dwMouseStatePrevIndex = 0;
    g_dwMouseStateCurIndex = 1;
}

bool InitializeKeyboardDevice(void) {
    if (++g_dwKeyboardDeviceRefCount != 1) {
        return true;
    }
    g_dwKeyboardStateCurIndex = 1;
    memset(g_aabKeyboardStateBuffers, 0, sizeof(g_aabKeyboardStateBuffers));
    memset(g_aJoystickStateBuffers, 0, sizeof(g_aJoystickStateBuffers));
    g_dwKeyboardStatePrevIndex = 0;
    g_dwJoystickStatePrevIndex = 0;
    g_dwJoystickStateCurIndex = 1;
    ResetMouseState();
    if (FAILED(g_pDirectInput->CreateDevice(GUID_SysKeyboard, &g_pKeyboardDevice, nullptr)) ||
        FAILED(g_pKeyboardDevice->SetDataFormat(&c_dfDIKeyboard)) ||
        FAILED(g_pKeyboardDevice->SetCooperativeLevel(g_hMainWindow,
                                                      DISCL_NONEXCLUSIVE | DISCL_FOREGROUND))) {
        return false;
    }
    if (g_pKeyboardDevice != nullptr) {
        g_pKeyboardDevice->Acquire();
    }
    return true;
}

HRESULT ReadKeyboardState(unsigned char *pabState) {
    HRESULT hResult;

    if (g_pKeyboardDevice == nullptr) {
        return 1;
    }
    hResult = g_pKeyboardDevice->GetDeviceState(KEYBOARD_KEY_COUNT, pabState);
    if (hResult == DIERR_INPUTLOST) {
        hResult = g_pKeyboardDevice->Acquire();
        if (SUCCEEDED(hResult)) {
            hResult = g_pKeyboardDevice->GetDeviceState(KEYBOARD_KEY_COUNT, pabState);
        }
    }
    return hResult;
}

void ClearKeyboardStateBuffers(void) {
    memset(g_abKeyboardState, 0, sizeof(g_abKeyboardState));
    memset(g_adwKeyPressedEdge, 0, KEY_EDGE_CLEAR_COUNT * sizeof(g_adwKeyPressedEdge[0]));
    memset(g_adwKeyPrevDown, 0, KEY_EDGE_CLEAR_COUNT * sizeof(g_adwKeyPrevDown[0]));
}

void UpdateKeyboardEdgeState(void) {
    for (int i = 0; i < KEYBOARD_KEY_COUNT; i++) {
        int bDown = (g_abKeyboardState[i] >> KEY_DOWN_BIT) & 1;

        g_adwKeyPressedEdge[i] = (g_adwKeyPrevDown[i] == 0) & bDown;
        g_adwKeyPrevDown[i] = bDown;
    }
}

void PollKeyboardState(void) {
    if (SUCCEEDED(ReadKeyboardState(g_abKeyboardState))) {
        UpdateKeyboardEdgeState();
    }
}

int GetFirstPressedKeyIndex(void) {
    for (int i = 0; i < KEYBOARD_KEY_COUNT; i++) {
        if (g_adwKeyPressedEdge[i]) {
            return i;
        }
    }
    return -1;
}

bool IsInputOrEventActive(void) {
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        if (g_adwMouseButtonPressed[i]) {
            return true;
        }
    }
    if (g_dwInNetworkCarSetup) {
        return false;
    }
    return g_adwKeyPressedEdge[DIK_RETURN] || g_adwKeyPressedEdge[DIK_NUMPADENTER] ||
           g_adwKeyPressedEdge[DIK_LEFT] || g_adwKeyPressedEdge[DIK_RIGHT];
}

bool CheckForUserInput(void) {
    return GetFirstPressedKeyIndex() >= 0 || IsInputOrEventActive();
}

MouseDevice::MouseDevice() {
    pDevice = nullptr;
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        abHeld[i] = 0;
        abClicked[i] = 0;
    }
    bPressPending = 0;
}

// The time the hover of a menu waits before it moves again, in milliseconds.
#define MENU_SCROLL_REPEAT_MS 120

// The cursor of the mouse stays within the screen, the far edge included.
static inline int ClampCursor(int nPosition, int nLimit) {
    if (nPosition < 0) {
        return 0;
    }
    return nPosition >= nLimit ? nLimit : nPosition;
}

void MouseDevice::PollMouseDevice(HWND) {
    DIMOUSESTATE state;
    DWORD dwNow;
    HRESULT hResult;
    bool bKeyLeft;
    bool bAnyDown;

    if (g_fCharInputSuppressed) {
        return;
    }
    dwNow = GetHighResolutionTime();
    g_MouseMenuState.dwScrollRepeatTimerMs += dwNow - g_MouseMenuState.dwPollLastTimeMs;
    g_MouseMenuState.dwPollLastTimeMs = dwNow;
    if (static_cast<int>(g_MouseMenuState.dwScrollRepeatTimerMs) > MENU_SCROLL_REPEAT_MS) {
        g_MouseMenuState.bScrollRepeatReady = true;
    }
    if (pDevice == nullptr) {
        return;
    }
    pDevice->Acquire();
    hResult = pDevice->GetDeviceState(sizeof(state), &state);
    // The device is acquired twice before each read that follows a loss.
    while (hResult == DIERR_INPUTLOST && SUCCEEDED(pDevice->Acquire())) {
        pDevice->Acquire();
        hResult = pDevice->GetDeviceState(sizeof(state), &state);
    }
    nCursorX += state.lX;
    nCursorY += state.lY;
    nWheel += state.lZ;
    nCursorX = ClampCursor(nCursorX, g_dwEntityHalfWidth);
    nCursorY = ClampCursor(nCursorY, g_dwEntityHalfHeight);
    bAnyDown = false;
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        abButtons[i] = state.rgbButtons[i] & BUTTON_DOWN_MASK;
        bAnyDown = bAnyDown || abButtons[i] != 0;
    }
    if (bAnyDown) {
        bPressPending = 1;
    }

    if (g_adwKeyPressedEdge[DIK_F11]) {
        SelectPreviousMusicTrack();
    }
    if (g_adwKeyPressedEdge[DIK_F12]) {
        SelectNextMusicTrack();
    }
    if (g_MouseMenuState.bScrollRepeatReady) {
        if (g_abKeyboardState[DIK_UP] & BUTTON_DOWN_MASK) {
            g_MouseMenuState.bScrollDecPending = true;
            g_dwMouseScrollDirection = -1;
            g_MouseMenuState.dwScrollRepeatTimerMs = 0;
        }
        if (g_abKeyboardState[DIK_DOWN] & BUTTON_DOWN_MASK) {
            g_MouseMenuState.bScrollIncPending = true;
            g_dwMouseScrollDirection = 1;
            g_MouseMenuState.dwScrollRepeatTimerMs = 0;
        }
    }
    if (g_adwKeyPressedEdge[DIK_LEFT]) {
        bKeyLeft = true;
        g_dwMouseScrollDirection = -1;
        g_MouseMenuState.dwScrollRepeatTimerMs = 0;
    } else {
        bKeyLeft = g_MouseMenuState.bKeyLeftAsButton0;
    }
    if (g_adwKeyPressedEdge[DIK_RIGHT]) {
        g_MouseMenuState.bKeyRightAsButton1 = true;
        g_dwMouseScrollDirection = 1;
        g_MouseMenuState.dwScrollRepeatTimerMs = 0;
    }
    if (bKeyLeft) {
        abButtons[0] = 1;
        bPressPending = 0;
        g_MouseMenuState.bScrollRepeatReady = false;
    }
    if (g_MouseMenuState.bKeyRightAsButton1) {
        abButtons[1] = 1;
        bPressPending = 0;
        g_MouseMenuState.bScrollRepeatReady = false;
    }
    if (g_MouseMenuState.bScrollRepeatReady) {
        if (g_MouseMenuState.bScrollDecPending) {
            g_wMenuHoverIndex--;
            g_MouseMenuState.bScrollRepeatReady = false;
        }
        if (g_MouseMenuState.bScrollIncPending) {
            g_wMenuHoverIndex++;
            g_MouseMenuState.bScrollRepeatReady = false;
        }
    }
    g_MouseMenuState.bKeyRightAsButton1 = false;
    g_MouseMenuState.bKeyLeftAsButton0 = false;
    g_MouseMenuState.bScrollIncPending = false;
    g_MouseMenuState.bScrollDecPending = false;

    if (nCursorX != nPrevCursorX || nCursorY != nPrevCursorY ||
        memcmp(abButtons, abPrevButtons, sizeof(abButtons)) != 0) {
        g_dwLastInputActivityTime = GetHighResolutionTime();
    }
    nPrevCursorX = nCursorX;
    nPrevCursorY = nCursorY;
    memcpy(abPrevButtons, abButtons, sizeof(abButtons));

    for (int &bDown : g_adwMouseButtonRaw) {
        bDown = 0;
    }
    // A button counts as pressed for the game when it is let go.
    for (int i = 0; i < MOUSE_BUTTON_COUNT; i++) {
        if (abButtons[i] && !abHeld[i]) {
            abHeld[i] = 1;
        } else if (!abButtons[i] && abHeld[i]) {
            abHeld[i] = 0;
            abClicked[i] = 1;
            bPressPending = 0;
            g_adwMouseButtonRaw[i] = 1;
        } else {
            abClicked[i] = 0;
        }
    }
    g_pMousePosition = this;
    UpdateMouseButtonEdges();
}

void UpdateMouseInput(void) {
    g_pMouseDevice->PollMouseDevice(g_hMainWindow);
}

int ShowInfoMessageBox(HWND hWnd, HRESULT, const char *pszText) {
    return MessageBoxA(hWnd, pszText, GetStringResource(0xaa), MB_OK);
}

bool MouseDevice::CreateMouseDevice(HWND hWnd) {
    HRESULT hResult;

    hResult = g_pDirectInput->CreateDevice(GUID_SysMouse, &pDevice, nullptr);
    if (FAILED(hResult)) {
        ShowInfoMessageBox(hWnd, hResult, GetStringResource(0xac));
        return false;
    }
    hResult = pDevice->SetDataFormat(&c_dfDIMouse);
    if (FAILED(hResult)) {
        ShowInfoMessageBox(hWnd, hResult, GetStringResource(0xad));
        return false;
    }
    hResult = pDevice->SetCooperativeLevel(hWnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
    if (FAILED(hResult)) {
        ShowInfoMessageBox(hWnd, hResult, GetStringResource(0xae));
        return false;
    }
    return true;
}

void MouseDevice::SyncMouseAcquireState(HWND hWnd) {
    IDirectInputDeviceA *device = pDevice;
    if (device && g_pActiveMenuScreen->IsBuilt()) {
        if (g_fHostWindowActive) {
            device->Unacquire();
        } else {
            device->Acquire();
        }
    }
}

void MouseDevice::ReleaseMouseDevice() {
    if (pDevice != nullptr) {
        pDevice->Unacquire();
        pDevice->Release();
        pDevice = nullptr;
    }
}

bool InitializeDirectInput(void) {
    if (++g_dwDirectInputRefCount != 1) {
        return true;
    }
    if (FAILED(DirectInputCreateA(g_hAppInstance, DIRECTINPUT_VERSION, &g_pDirectInput, nullptr))) {
        MessageBoxA(
            g_hMainWindow, GetStringResource(0x127), GetStringResource(0x126), MB_ICONERROR);
        return false;
    }
    g_pMouseDevice = new (std::nothrow) MouseDevice();
    // The device is created through the pointer whether or not the allocation succeeded.
    g_pMouseDevice->CreateMouseDevice(g_hMainWindow);
    return true;
}

bool TryAcquireJoystickDevice(void) {
    if (g_pJoystickDevice == nullptr) {
        return false;
    }
    return SUCCEEDED(g_pJoystickDevice->Acquire());
}

HRESULT SetJoystickPropertyDword(IDirectInputDevice2A *pDevice,
                                 REFGUID guidProperty,
                                 DWORD dwObject,
                                 DWORD dwHow,
                                 DWORD dwData) {
    DIPROPDWORD property;

    property.diph.dwSize = sizeof(property);
    property.diph.dwHeaderSize = sizeof(property.diph);
    property.diph.dwObj = dwObject;
    property.diph.dwHow = dwHow;
    property.dwData = dwData;
    return pDevice->SetProperty(guidProperty, &property.diph);
}

HRESULT SetJoystickAxisRange(IDirectInputDevice2A *pDevice,
                             REFGUID guidProperty,
                             DWORD dwObject,
                             DWORD dwHow,
                             LONG lMin,
                             LONG lMax) {
    DIPROPRANGE property;

    property.diph.dwSize = sizeof(property);
    property.diph.dwHeaderSize = sizeof(property.diph);
    property.diph.dwObj = dwObject;
    property.diph.dwHow = dwHow;
    property.lMin = lMin;
    property.lMax = lMax;
    return pDevice->SetProperty(guidProperty, &property.diph);
}

int g_bForceFeedbackAvailable;

void SetForceFeedbackMagnitude(LONG lMagnitude) {
    g_aDiConditionParams[0].lPositiveCoefficient = lMagnitude;
    g_aDiConditionParams[0].lNegativeCoefficient = lMagnitude;
    if (g_pConditionEffect != nullptr) {
        g_pConditionEffect->SetParameters(&g_DiConditionEffect, DIEP_TYPESPECIFICPARAMS);
    }
}

LONG g_adwForceFeedbackDirection[2] = {-1, 0};

// The durations of the effects of force feedback, in microseconds.
#define FORCE_FEEDBACK_LONG_DURATION 250000
#define FORCE_FEEDBACK_SHORT_DURATION 125000

static inline bool IsForceFeedbackInUse(void) {
    return (g_GlobalRallyConfig.dwInputDeviceType == 1 ||
            g_GlobalRallyConfig.dwInputDeviceType == 2) &&
           g_GlobalRallyConfig.bForceFeedbackEnabled && g_bForceFeedbackAvailable &&
           !gInstantReplayActive;
}

static inline int ScaleForceFeedback(int value, int multiplier) {
    return static_cast<int>(static_cast<unsigned>(value) * static_cast<unsigned>(multiplier)) /
           10000;
}

void UpdateForceFeedbackForce(int bReduceForce, int bEnableForce, int nForce) {
    if (!IsForceFeedbackInUse()) {
        return;
    }
    g_dwForceFeedbackConstantForce =
        ScaleForceFeedback(static_cast<int>((static_cast<unsigned>(nForce) + 8) *
                                            static_cast<unsigned>(g_dwForceFeedbackGain)),
                           1000);
    if (!bEnableForce) {
        if (!g_bForceFeedbackForceStopped) {
            if (g_pConditionEffect != nullptr) {
                g_pConditionEffect->Stop();
            }
            g_bForceFeedbackForceStopped = true;
        }
        return;
    }
    if (g_bForceFeedbackForceStopped) {
        if (g_pConditionEffect != nullptr) {
            g_pConditionEffect->Start(1, 0);
        }
        SetForceFeedbackLongDuration();
        g_bForceFeedbackForceStopped = false;
    }
    const int previous = g_dwForceFeedbackMagnitude;
    if (bReduceForce) {
        g_dwForceFeedbackMagnitude += ScaleForceFeedback(g_dwForceFeedbackGain, -1000);
        const int minimum =
            ScaleForceFeedback(g_dwForceFeedbackMagnitudeMax, g_dwForceFeedbackGain);
        if (g_dwForceFeedbackMagnitude < minimum) {
            g_dwForceFeedbackMagnitude = minimum;
        }
    } else {
        g_dwForceFeedbackMagnitude += ScaleForceFeedback(g_dwForceFeedbackGain, 1000);
        const int maximum =
            ScaleForceFeedback(g_dwForceFeedbackConstantForce, g_dwForceFeedbackGain);
        if (g_dwForceFeedbackMagnitude > maximum) {
            g_dwForceFeedbackMagnitude = maximum;
        }
    }
    if (g_dwForceFeedbackMagnitude != previous) {
        SetForceFeedbackMagnitude(g_dwForceFeedbackMagnitude);
    }
}

void SetForceFeedbackDirection(LONG lDirection) {
    if (IsForceFeedbackInUse()) {
        g_adwForceFeedbackDirection[0] = lDirection;
    }
}

void SetForceFeedbackLongDuration(void) {
    if (IsForceFeedbackInUse()) {
        g_dwForceFeedbackDuration = FORCE_FEEDBACK_LONG_DURATION;
    }
}

void SetForceFeedbackShortDuration(void) {
    if (IsForceFeedbackInUse()) {
        g_dwForceFeedbackDuration = FORCE_FEEDBACK_SHORT_DURATION;
    }
}

// The ticks of the CPU counter of the console in a frame of 1/30 of a second.
#define OS_TICKS_PER_FRAME (46875000 / 30)

u64 AdvanceEmulatedOsTime(void) {
    g_qwEmulatedOsTime += OS_TICKS_PER_FRAME;
    return g_qwEmulatedOsTime;
}

void StartForceFeedbackJolt(void) {
    if (!IsForceFeedbackInUse()) {
        return;
    }
    g_DiJoltEffect.dwDuration = g_dwForceFeedbackDuration;
    if (g_pJoltEffect != nullptr) {
        // The result of the start is discarded.
        g_pJoltEffect->SetParameters(&g_DiJoltEffect, DIEP_DURATION | DIEP_DIRECTION | DIEP_START);
    }
}

// The saturation and the gain of the effects of force feedback.
#define FORCE_FEEDBACK_FULL 10000
// The period of the square wave of the jolt in microseconds.
#define JOLT_PERIOD 250000

// The effects play on X and Y by offsets, in Cartesian directions, and with no trigger.
static inline void SetUpForceFeedbackEffect(DIEFFECT *pEffect,
                                            DWORD dwDuration,
                                            DWORD *pdwAxes,
                                            LONG *plDirection,
                                            DWORD cbParams,
                                            void *pParams) {
    pEffect->dwSize = sizeof(*pEffect);
    pEffect->dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    pEffect->dwDuration = dwDuration;
    pEffect->dwSamplePeriod = 0;
    pEffect->dwGain = FORCE_FEEDBACK_FULL;
    pEffect->dwTriggerButton = DIEB_NOTRIGGER;
    pEffect->dwTriggerRepeatInterval = 0;
    pEffect->cAxes = 2;
    pEffect->rgdwAxes = pdwAxes;
    pEffect->rglDirection = plDirection;
    pEffect->lpEnvelope = nullptr;
    pEffect->cbTypeSpecificParams = cbParams;
    pEffect->lpvTypeSpecificParams = pParams;
}

static inline void SetSpring(DICONDITION *pCondition, LONG lCoefficient) {
    pCondition->lOffset = 0;
    pCondition->lPositiveCoefficient = lCoefficient;
    pCondition->lNegativeCoefficient = lCoefficient;
    pCondition->dwPositiveSaturation = FORCE_FEEDBACK_FULL;
    pCondition->dwNegativeSaturation = FORCE_FEEDBACK_FULL;
    pCondition->lDeadBand = 0;
}

HRESULT CreateForceFeedbackEffects(int nCoefficientX, int nCoefficientY) {
    // The effects retain a pointer to the axes after the routine returns.
    DWORD adwAxes[2] = {DIJOFS_X, DIJOFS_Y};

    SetSpring(&g_aDiConditionParams[0], nCoefficientX);
    SetSpring(&g_aDiConditionParams[1], nCoefficientY);
    SetUpForceFeedbackEffect(&g_DiConditionEffect,
                             INFINITE,
                             adwAxes,
                             g_alConditionEffectDirection,
                             sizeof(g_aDiConditionParams),
                             g_aDiConditionParams);
    if (g_pJoystickDevice->CreateEffect(
            GUID_Spring, &g_DiConditionEffect, &g_pConditionEffect, nullptr) == DI_OK) {
        g_dwForceFeedbackMagnitude = nCoefficientX;
        g_bForceFeedbackForceStopped = true;
    }
    g_DiJoltPeriodic.dwMagnitude = FORCE_FEEDBACK_FULL;
    g_DiJoltPeriodic.lOffset = 0;
    g_DiJoltPeriodic.dwPhase = 0;
    g_DiJoltPeriodic.dwPeriod = JOLT_PERIOD;
    SetUpForceFeedbackEffect(&g_DiJoltEffect,
                             g_dwForceFeedbackDuration,
                             adwAxes,
                             g_adwForceFeedbackDirection,
                             sizeof(g_DiJoltPeriodic),
                             &g_DiJoltPeriodic);
    return g_pJoystickDevice->CreateEffect(GUID_Square, &g_DiJoltEffect, &g_pJoltEffect, nullptr);
}

// The range of an axis of the game controller.
#define JOYSTICK_AXIS_MIN -128
#define JOYSTICK_AXIS_MAX 128

BOOL CALLBACK EnumJoysticksCallback_CreateDevice(const DIDEVICEINSTANCEA *pInstance,
                                                 void *pContext) {
    IDirectInputDeviceA *pDevice;
    GUID guidInstance = pInstance->guidInstance;
    HRESULT hResult;
    const char *pszError;

    if (FAILED(g_pDirectInput->CreateDevice(guidInstance, &pDevice, nullptr))) {
        OutputDebugStringA("Error: Failed to create device.\n");
        return DIENUM_STOP;
    }
    hResult = pDevice->QueryInterface(IID_IDirectInputDevice2A,
                                      reinterpret_cast<void **>(&g_pJoystickDevice));
    pDevice->Release();
    if (FAILED(hResult)) {
        OutputDebugStringA("Error: Failed to obtain interface.\n");
        return DIENUM_STOP;
    }
    if (FAILED(g_pJoystickDevice->SetCooperativeLevel(
            g_hMainWindow, static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(pContext))))) {
        pszError = "Error: Failed to set cooperative level.\n";
    } else if (FAILED(g_pJoystickDevice->SetDataFormat(&c_dfDIJoystick2))) {
        pszError = "Error: Failed to set game device data format.\n";
    } else {
        return DIENUM_STOP;
    }
    OutputDebugStringA(pszError);
    g_pJoystickDevice->Release();
    g_pJoystickDevice = nullptr;
    return DIENUM_STOP;
}

// A property that is not set releases the game controller.
static inline int ReleaseJoystickAfterFailure(const char *pszError) {
    OutputDebugStringA(pszError);
    g_pJoystickDevice->Unacquire();
    g_pJoystickDevice->Release();
    g_pJoystickDevice = nullptr;
    return 0;
}

int InitializeJoystickDevice(void) {
    DIPROPDWORD autoCenter;

    if (g_GlobalRallyConfig.dwInputDeviceType == 0 || ++g_dwJoystickDeviceRefCount != 1) {
        return g_GlobalRallyConfig.dwInputDeviceType;
    }
    if (g_GlobalRallyConfig.bForceFeedbackEnabled &&
        g_pDirectInput->EnumDevices(DIDEVTYPE_JOYSTICK,
                                    EnumJoysticksCallback_CreateDevice,
                                    reinterpret_cast<void *>(DISCL_EXCLUSIVE | DISCL_FOREGROUND),
                                    DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK) == DI_OK &&
        g_pJoystickDevice != nullptr) {
        g_bForceFeedbackAvailable = true;
        autoCenter.diph.dwSize = sizeof(autoCenter);
        autoCenter.diph.dwHeaderSize = sizeof(autoCenter.diph);
        autoCenter.diph.dwObj = 0;
        autoCenter.diph.dwHow = DIPH_DEVICE;
        autoCenter.dwData = DIPROPAUTOCENTER_OFF;
        if (FAILED(g_pJoystickDevice->SetProperty(DIPROP_AUTOCENTER, &autoCenter.diph))) {
            OutputDebugStringA("Error: Failed to change device property.\n");
        }
        TryAcquireJoystickDevice();
        CreateForceFeedbackEffects(1000, 8000);
    } else {
        g_pDirectInput->EnumDevices(DIDEVTYPE_JOYSTICK,
                                    EnumJoysticksCallback_CreateDevice,
                                    reinterpret_cast<void *>(DISCL_NONEXCLUSIVE | DISCL_FOREGROUND),
                                    DIEDFL_ATTACHEDONLY);
        TryAcquireJoystickDevice();
        g_bForceFeedbackAvailable = false;
    }
    if (g_pJoystickDevice == nullptr) {
        return 0;
    }
    if (FAILED(SetJoystickAxisRange(g_pJoystickDevice,
                                    DIPROP_RANGE,
                                    DIJOFS_X,
                                    DIPH_BYOFFSET,
                                    JOYSTICK_AXIS_MIN,
                                    JOYSTICK_AXIS_MAX))) {
        return ReleaseJoystickAfterFailure(
            "Error: IDirectInputDevice::SetProperty(DIPH_RANGE) FAILED\n");
    }
    if (FAILED(SetJoystickPropertyDword(
            g_pJoystickDevice, DIPROP_DEADZONE, DIJOFS_X, DIPH_BYOFFSET, 0))) {
        return ReleaseJoystickAfterFailure(
            "Error: IDirectInputDevice::SetProperty(DIPH_WORD) FAILED\n");
    }
    if (FAILED(SetJoystickAxisRange(g_pJoystickDevice,
                                    DIPROP_RANGE,
                                    DIJOFS_Y,
                                    DIPH_BYOFFSET,
                                    JOYSTICK_AXIS_MIN,
                                    JOYSTICK_AXIS_MAX))) {
        return ReleaseJoystickAfterFailure(
            "Error: IDirectInputDevice::SetProperty(DIPH_RANGE) FAILED\n");
    }
    if (FAILED(SetJoystickPropertyDword(
            g_pJoystickDevice, DIPROP_DEADZONE, DIJOFS_Y, DIPH_BYOFFSET, 0))) {
        return ReleaseJoystickAfterFailure(
            "Error: IDirectInputDevice::SetProperty(DIPH_WORD) FAILED\n");
    }
    return g_GlobalRallyConfig.dwInputDeviceType;
}

void ProbeJoystickDevice(void) {
    int dwInputDeviceType = g_GlobalRallyConfig.dwInputDeviceType;
    int bForceFeedbackEnabled = g_GlobalRallyConfig.bForceFeedbackEnabled;

    g_GlobalRallyConfig.dwInputDeviceType = INPUT_DEVICE_FIRST_CONTROLLER;
    g_GlobalRallyConfig.SelectActiveInputBindings();
    g_GlobalRallyConfig.bForceFeedbackEnabled = true;
    InitializeJoystickDevice();
    ReleaseJoystickDevice();
    g_GlobalRallyConfig.dwInputDeviceType = dwInputDeviceType;
    g_GlobalRallyConfig.SelectActiveInputBindings();
    g_GlobalRallyConfig.bForceFeedbackEnabled = bForceFeedbackEnabled;
}

void ReleaseKeyboardDevice(void) {
    if (--g_dwKeyboardDeviceRefCount < 0) {
        g_dwKeyboardDeviceRefCount = 0;
        return;
    }
    if (g_dwKeyboardDeviceRefCount == 0 && g_pKeyboardDevice != nullptr) {
        g_pKeyboardDevice->Unacquire();
        g_pKeyboardDevice->Release();
        g_pKeyboardDevice = nullptr;
    }
}

void ReleaseJoystickDevice(void) {
    if (--g_dwJoystickDeviceRefCount < 0) {
        g_dwJoystickDeviceRefCount = 0;
        return;
    }
    if (g_dwJoystickDeviceRefCount != 0) {
        return;
    }
    if (g_pJoltEffect != nullptr) {
        g_pJoltEffect->Release();
        g_pJoltEffect = nullptr;
    }
    if (g_pConditionEffect != nullptr) {
        g_pConditionEffect->Release();
        g_pConditionEffect = nullptr;
    }
    if (g_pJoystickDevice != nullptr) {
        g_pJoystickDevice->Unacquire();
        g_pJoystickDevice->Release();
        g_pJoystickDevice = nullptr;
    }
}

bool ShutdownDirectInput(void) {
    if (--g_dwDirectInputRefCount == 0 && g_pDirectInput != nullptr) {
        if (g_pMouseDevice != nullptr) {
            g_pMouseDevice->ReleaseMouseDevice();
            ::operator delete(g_pMouseDevice);
        }
        g_pMouseDevice = nullptr;
        g_pDirectInput->Release();
        g_pDirectInput = nullptr;
    }
    return true;
}

void UpdateAppActiveStateFromActivate(WPARAM wParam) {
    g_dwAppActive = wParam;
    g_dwActivateState = LOWORD(wParam);
    g_dwActivateMinimized = HIWORD(wParam);
    // The original calls NullStub() when the window goes inactive or is minimised.
    if (g_dwActivateState == WA_INACTIVE || g_dwActivateMinimized) {
        NullStub();
    }
}
