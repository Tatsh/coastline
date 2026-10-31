#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace coastline {

/** The frames a press of a button or a key lasts when its line gives no number. */
inline constexpr int SCRIPT_PRESS_FRAMES = 6;

/** A press of the buttons of a controller from a frame for a number of frames. */
struct ScriptPress {
    int frame;            /*!< The first frame the buttons are down, from 1. */
    std::uint16_t button; /*!< The ControllerButton bit pressed. */
    int frames;           /*!< The frames the button stays down. */
    std::size_t port;     /*!< The controller, from 0. */
};

/** The button of the mouse a move holds down. */
enum class ScriptMouseButton {
    None,  /*!< No button. */
    Left,  /*!< The left button. */
    Right, /*!< The right button. */
};

/** The mouse at a place of the 320 by 240 screen from a frame for a number of frames. */
struct ScriptMouse {
    int frame;                /*!< The first frame of the move, from 1. */
    int x;                    /*!< The place across the 320 by 240 screen. */
    int y;                    /*!< The place down the 320 by 240 screen. */
    ScriptMouseButton button; /*!< The button held down while the mouse is there. */
    int frames;               /*!< The frames the mouse stays there. */
};

/** A key of the keyboard down from a frame for a number of frames. */
struct ScriptKey {
    int frame;        /*!< The first frame the key is down, from 1. */
    std::string name; /*!< The SDL name of the key, such as Return or F9. */
    int frames;       /*!< The frames the key stays down. */
    int line;         /*!< The line of the script, from 1, for the message of an unknown key. */
};

/** A text typed at a frame, or a key of <backspace> or <enter>. */
struct ScriptTyping {
    int frame;        /*!< The frame of the typing, from 1. */
    std::string text; /*!< The characters typed, or <backspace> or <enter>. */
};

/** The input and the captures of a script of run-script. */
struct Script {
    bool autopilot = false;              /*!< Whether the computer drives the first player. */
    int screenshotEvery = 0;             /*!< Save every this many frames, or 0 for none. */
    std::vector<ScriptPress> presses;    /*!< The presses of controller buttons. */
    std::vector<ScriptMouse> mouseMoves; /*!< The moves of the mouse. */
    std::vector<ScriptKey> keys;         /*!< The presses of keys. */
    std::vector<ScriptTyping> typing;    /*!< The typings. */
    std::vector<int> screenshots;        /*!< The frames saved. */
    std::optional<int> quitFrame;        /*!< The frame the game exits after, the earliest. */
};

/** A line of a script that cannot be read. */
struct ScriptError {
    int line;            /*!< The line, from 1. */
    std::string message; /*!< The reason. */
};

/**
 * Read a script of run-script: one command a line, with # starting a comment and blank lines
 * ignored. A line that starts with a frame number acts at that frame, and the lines autopilot and
 * screenshot-every N take no frame and apply from the start.
 *
 * @param text The text of the script.
 * @return The script, or the first line that cannot be read.
 */
std::expected<Script, ScriptError> ParseScript(std::string_view text);

} // namespace coastline
