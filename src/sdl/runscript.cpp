#include "runscript.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <utility>

#include "controller.h"

namespace coastline {

namespace {

constexpr std::string_view SPACE = " \t\r";

std::optional<int> ParseNumber(std::string_view token) {
    int value = 0;
    auto [end, error] = std::from_chars(token.data(), token.data() + token.size(), value);
    if (error != std::errc() || end != token.data() + token.size()) {
        return std::nullopt;
    }
    return value;
}

// The words of a line, each with the rest of the line from it.
struct Word {
    std::string_view text;
    std::string_view rest;
};

std::vector<Word> SplitWords(std::string_view line) {
    std::vector<Word> words;
    std::size_t start = line.find_first_not_of(SPACE);
    while (start != std::string_view::npos) {
        std::size_t end = line.find_first_of(SPACE, start);
        std::string_view rest = line.substr(start);
        rest = rest.substr(0, rest.find_last_not_of(SPACE) + 1);
        words.push_back(
            {line.substr(start, end == std::string_view::npos ? end : end - start), rest});
        start = end == std::string_view::npos ? end : line.find_first_not_of(SPACE, end);
    }
    return words;
}

std::optional<std::uint16_t> ParseButton(std::string_view name) {
    static constexpr std::array<std::pair<std::string_view, std::uint16_t>, 14> BUTTONS{
        {{"a", BUTTON_A},
         {"b", BUTTON_B},
         {"z", BUTTON_Z},
         {"start", BUTTON_START},
         {"l", BUTTON_L},
         {"r", BUTTON_R},
         {"up", BUTTON_D_UP},
         {"down", BUTTON_D_DOWN},
         {"left", BUTTON_D_LEFT},
         {"right", BUTTON_D_RIGHT},
         {"cu", BUTTON_C_UP},
         {"cd", BUTTON_C_DOWN},
         {"cl", BUTTON_C_LEFT},
         {"cr", BUTTON_C_RIGHT}}};
    auto found =
        std::ranges::find(BUTTONS, name, &std::pair<std::string_view, std::uint16_t>::first);
    if (found == BUTTONS.end()) {
        return std::nullopt;
    }
    return found->second;
}

// The command of one line, its words after the frame, into @p script, or the reason it cannot be
// read.
std::optional<std::string>
ParseCommand(const std::vector<Word> &words, std::optional<int> frame, int line, Script &script) {
    std::string_view command = words[0].text;
    std::size_t arguments = words.size() - 1;
    auto number = [&](std::size_t index) { return ParseNumber(words[index].text); };
    auto badNumber = [&](std::size_t index) {
        return std::string(words[index].text) + " is not a number of frames above 0.";
    };
    auto count = [&](std::size_t index, int fallback) -> std::optional<int> {
        if (index >= words.size()) {
            return fallback;
        }
        auto value = number(index);
        if (!value || *value <= 0) {
            return std::nullopt;
        }
        return value;
    };
    auto usage = [&](std::string_view form) {
        return std::string(command) + " takes " + std::string(form) + ".";
    };
    if (command == "autopilot" || command == "screenshot-every") {
        if (frame) {
            return std::string(command) + " applies from the start and takes no frame.";
        }
        if (command == "autopilot") {
            if (arguments != 0) {
                return usage("no arguments");
            }
            script.autopilot = true;
            return std::nullopt;
        }
        if (arguments != 1) {
            return usage("a number of frames");
        }
        auto every = count(1, 0);
        if (!every) {
            return badNumber(1);
        }
        script.screenshotEvery = *every;
        return std::nullopt;
    }
    if (command != "press" && command != "key" && command != "type" && command != "mouse" &&
        command != "screenshot" && command != "quit") {
        return std::string(command) + " is not a command.";
    }
    if (!frame) {
        return std::string(command) + " needs a frame at the start of the line.";
    }
    if (command == "press") {
        if (arguments < 1 || arguments > 3) {
            return usage("BUTTON [FRAMES [PORT]]");
        }
        auto button = ParseButton(words[1].text);
        if (!button) {
            return std::string(words[1].text) +
                   " is not one of a, b, z, start, l, r, up, down, left, right, cu, cd, cl, or cr.";
        }
        auto frames = count(2, SCRIPT_PRESS_FRAMES);
        if (!frames) {
            return badNumber(2);
        }
        auto port = count(3, 1);
        if (!port || *port > static_cast<int>(CONTROLLER_PORTS)) {
            return std::string(words[3].text) + " is not a controller from 1 to 4.";
        }
        script.presses.push_back({*frame, *button, *frames, static_cast<std::size_t>(*port - 1)});
    } else if (command == "key") {
        if (arguments < 1 || arguments > 2) {
            return usage("KEY [FRAMES]");
        }
        auto frames = count(2, SCRIPT_PRESS_FRAMES);
        if (!frames) {
            return badNumber(2);
        }
        script.keys.push_back({*frame, std::string(words[1].text), *frames, line});
    } else if (command == "type") {
        if (arguments == 0) {
            return usage("TEXT");
        }
        script.typing.push_back({*frame, std::string(words[1].rest)});
    } else if (command == "mouse") {
        if (arguments < 2 || arguments > 4) {
            return usage("X Y [BUTTON [FRAMES]]");
        }
        auto x = number(1);
        auto y = number(2);
        if (!x || !y) {
            return std::string(words[!x ? 1 : 2].text) + " is not a place of the screen.";
        }
        ScriptMouseButton button = ScriptMouseButton::None;
        if (arguments >= 3) {
            std::string_view name = words[3].text;
            if (name == "left") {
                button = ScriptMouseButton::Left;
            } else if (name == "right") {
                button = ScriptMouseButton::Right;
            } else if (name != "none") {
                return std::string(name) + " is not one of left, right, or none.";
            }
        }
        auto frames = count(4, 1);
        if (!frames) {
            return badNumber(4);
        }
        script.mouseMoves.push_back({*frame, *x, *y, button, *frames});
    } else {
        if (arguments != 0) {
            return usage("no arguments");
        }
        if (command == "screenshot") {
            script.screenshots.push_back(*frame);
        } else {
            script.quitFrame = std::min(script.quitFrame.value_or(*frame), *frame);
        }
    }
    return std::nullopt;
}

} // namespace

std::expected<Script, ScriptError> ParseScript(std::string_view text) {
    Script script;
    int line = 0;
    std::size_t start = 0;
    while (start < text.size()) {
        ++line;
        std::size_t end = text.find('\n', start);
        std::string_view content =
            text.substr(start, end == std::string_view::npos ? end : end - start);
        start = end == std::string_view::npos ? text.size() : end + 1;
        content = content.substr(0, content.find('#'));
        std::vector<Word> words = SplitWords(content);
        if (words.empty()) {
            continue;
        }
        std::optional<int> frame;
        if (words[0].text.find_first_not_of("+-0123456789") == std::string_view::npos) {
            frame = ParseNumber(words[0].text);
            if (!frame || *frame <= 0) {
                return std::unexpected(ScriptError{
                    line, std::string(words[0].text) + " is not a frame number from 1."});
            }
            words.erase(words.begin());
            if (words.empty()) {
                return std::unexpected(ScriptError{line, "The frame has no command."});
            }
        }
        if (auto failure = ParseCommand(words, frame, line, script)) {
            return std::unexpected(ScriptError{line, *failure});
        }
    }
    return script;
}

} // namespace coastline
