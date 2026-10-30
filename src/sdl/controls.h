#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>

namespace coastline {

class GameConfig;

/** The actions of a gamepad the Controls section of BossRally.ini binds, in the order of keys. */
enum ControlAction : std::size_t {
    CONTROL_STEER_LEFT,        /*!< Steer left in a race. */
    CONTROL_STEER_RIGHT,       /*!< Steer right in a race. */
    CONTROL_ACCELERATE,        /*!< Accelerate in a race. */
    CONTROL_BRAKE,             /*!< Brake in a race. */
    CONTROL_REVERSE,           /*!< Reverse in a race. */
    CONTROL_GEAR_UP,           /*!< Change up a gear in a race. */
    CONTROL_GEAR_DOWN,         /*!< Change down a gear in a race. */
    CONTROL_COCKPIT_CAMERA,    /*!< Choose the cockpit camera in a race. */
    CONTROL_BUMPER_CAMERA,     /*!< Choose the bumper camera in a race. */
    CONTROL_CHASE_CAMERA,      /*!< Choose the chase camera in a race. */
    CONTROL_PAUSE_MENU_UP,     /*!< Move up the pause menu of a race. */
    CONTROL_PAUSE_MENU_DOWN,   /*!< Move down the pause menu of a race. */
    CONTROL_PAUSE_MENU_SELECT, /*!< Choose an item of the pause menu of a race. */
    CONTROL_PAUSE,             /*!< Pause a race. */
    CONTROL_START_OR_RESPAWN,  /*!< Start a race, and reset the car with a double press. */
    CONTROL_MENU_A,            /*!< The A button of the menus. */
    CONTROL_MENU_B,            /*!< The B button of the menus. */
    CONTROL_MENU_Z,            /*!< The Z trigger of the menus. */
    CONTROL_MENU_START,        /*!< The Start button of the menus. */
    CONTROL_MENU_L,            /*!< The left shoulder of the menus. */
    CONTROL_MENU_R,            /*!< The right shoulder of the menus. */
    CONTROL_MENU_C_UP,         /*!< C up of the menus. */
    CONTROL_MENU_C_DOWN,       /*!< C down of the menus. */
    CONTROL_MENU_C_LEFT,       /*!< C left of the menus. */
    CONTROL_MENU_C_RIGHT,      /*!< C right of the menus. */
    CONTROL_MENU_D_UP,         /*!< The directional pad up of the menus. */
    CONTROL_MENU_D_DOWN,       /*!< The directional pad down of the menus. */
    CONTROL_MENU_D_LEFT,       /*!< The directional pad left of the menus. */
    CONTROL_MENU_D_RIGHT,      /*!< The directional pad right of the menus. */
    CONTROL_MENU_STICK_LEFT,   /*!< The stick of the menus to the left. */
    CONTROL_MENU_STICK_RIGHT,  /*!< The stick of the menus to the right. */
    CONTROL_MENU_STICK_UP,     /*!< The stick of the menus up. */
    CONTROL_MENU_STICK_DOWN,   /*!< The stick of the menus down. */
    CONTROL_ACTION_COUNT,      /*!< The number of actions. */
};

/** The keys of the Controls section, by ControlAction. */
inline constexpr std::array<std::string_view, CONTROL_ACTION_COUNT> CONTROL_KEYS{
    "SteerLeft",      "SteerRight",    "Accelerate",      "Brake",        "Reverse",
    "GearUp",         "GearDown",      "CockpitCamera",   "BumperCamera", "ChaseCamera",
    "PauseMenuUp",    "PauseMenuDown", "PauseMenuSelect", "Pause",        "StartOrRespawn",
    "MenuA",          "MenuB",         "MenuZ",           "MenuStart",    "MenuL",
    "MenuR",          "MenuCUp",       "MenuCDown",       "MenuCLeft",    "MenuCRight",
    "MenuDUp",        "MenuDDown",     "MenuDLeft",       "MenuDRight",   "MenuStickLeft",
    "MenuStickRight", "MenuStickUp",   "MenuStickDown",
};

/** The key of the Controls section that chooses a preset. */
inline constexpr std::string_view CONTROL_PRESET_KEY = "Preset";

/** A set of bindings of every action, in the syntax of the Controls section. */
struct ControlPreset {
    std::string_view name;                                     /*!< The value of the Preset key. */
    std::array<std::string_view, CONTROL_ACTION_COUNT> values; /*!< The bindings by action. */
};

/**
 * The presets the Preset key chooses from, the first the default. Kart follows Mario Kart 8 on the
 * Pro Controller, with the east button for accelerate and A of the menus, and the south button for
 * brake and B of the menus.
 */
inline constexpr std::array<ControlPreset, 2> CONTROL_PRESETS{{
    {"Default",
     {"leftx-,dpleft",
      "leftx+,dpright",
      "a,righttrigger",
      "b,lefttrigger",
      "x",
      "rightshoulder",
      "leftshoulder",
      "righty-",
      "rightx-",
      "righty+,y",
      "lefty-,dpup",
      "lefty+,dpdown",
      "a",
      "start",
      "back",
      "a",
      "b,x",
      "lefttrigger",
      "start",
      "leftshoulder",
      "rightshoulder,righttrigger",
      "righty-",
      "righty+",
      "rightx-",
      "rightx+",
      "dpup",
      "dpdown",
      "dpleft",
      "dpright",
      "leftx-",
      "leftx+",
      "lefty-",
      "lefty+"}},
    {"Kart",
     {"leftx-,dpleft",
      "leftx+,dpright",
      "b",
      "a",
      "x",
      "rightshoulder,righttrigger",
      "leftshoulder,lefttrigger",
      "righty-",
      "rightx-",
      "righty+,y",
      "lefty-,dpup",
      "lefty+,dpdown",
      "b",
      "start",
      "back",
      "b",
      "a,x",
      "lefttrigger",
      "start",
      "leftshoulder",
      "rightshoulder,righttrigger",
      "righty-",
      "righty+",
      "rightx-",
      "rightx+",
      "dpup",
      "dpdown",
      "dpleft",
      "dpright",
      "leftx-",
      "leftx+",
      "lefty-",
      "lefty+"}},
}};

/** A button or the half of an axis of a gamepad that drives an action. */
struct ControlSource {
    bool axis; /*!< Whether the source is an axis rather than a button. */
    int index; /*!< The SDL_GamepadButton or the SDL_GamepadAxis. */
    int sign;  /*!< 1 for the positive half of an axis, -1 for the negative half. */
};

/** The magnitude of a button held down or of an axis at its end. */
inline constexpr int CONTROL_FULL = 32768;

/**
 * Read the Controls section of @p config: the preset of the Preset key, the default for an absent
 * or unknown preset, with each action key present replacing the binding of the preset. A name
 * that is not a button or an axis of a gamepad is logged and ignored.
 *
 * @param config The settings of BossRally.ini.
 */
void LoadControls(const GameConfig &config);

/**
 * @param action The action.
 * @return Whether an axis drives the action.
 */
bool IsControlAnalog(ControlAction action);

/**
 * @param gamepad The gamepad.
 * @param action The action.
 * @return The greatest magnitude of the sources of the action, 0 to CONTROL_FULL.
 */
int ReadControl(SDL_Gamepad *gamepad, ControlAction action);

} // namespace coastline
