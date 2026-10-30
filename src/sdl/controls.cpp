#include "controls.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <ranges>
#include <string>

#include "config.h"

namespace coastline {

namespace {

std::array<std::vector<ControlSource>, CONTROL_ACTION_COUNT> g_controls;

bool EqualsIgnoringCase(std::string_view left, std::string_view right) {
    return std::ranges::equal(left, right, [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) ==
               std::tolower(static_cast<unsigned char>(b));
    });
}

std::string_view Trim(std::string_view text) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!text.empty() && isSpace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && isSpace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    return text;
}

// A sign after the name of an axis chooses its half, and an axis with no sign reads its positive
// half, the whole travel of a trigger.
std::optional<ControlSource> ParseSource(std::string_view token) {
    std::string name(token);
    int sign = 1;
    bool hasSign = !name.empty() && (name.back() == '+' || name.back() == '-');
    if (hasSign) {
        sign = name.back() == '-' ? -1 : 1;
        name.pop_back();
    }
    SDL_GamepadButton button = SDL_GetGamepadButtonFromString(name.c_str());
    if (!hasSign && button != SDL_GAMEPAD_BUTTON_INVALID) {
        return ControlSource{false, button, 1};
    }
    SDL_GamepadAxis axis = SDL_GetGamepadAxisFromString(name.c_str());
    if (axis != SDL_GAMEPAD_AXIS_INVALID) {
        return ControlSource{true, axis, sign};
    }
    return std::nullopt;
}

std::vector<ControlSource> ParseSources(std::string_view key, std::string_view value) {
    std::vector<ControlSource> sources;
    for (auto range : value | std::views::split(',')) {
        auto token = Trim(std::string_view(range.begin(), range.end()));
        if (token.empty()) {
            continue;
        }
        auto source = ParseSource(token);
        if (!source) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Controls %.*s: %.*s is not a button or an axis of a gamepad, and is "
                        "ignored.",
                        static_cast<int>(key.size()),
                        key.data(),
                        static_cast<int>(token.size()),
                        token.data());
            continue;
        }
        sources.push_back(*source);
    }
    return sources;
}

} // namespace

void LoadControls(const GameConfig &config) {
    const ControlPreset *preset = &CONTROL_PRESETS[0];
    auto name = config.Get("Controls", CONTROL_PRESET_KEY);
    if (name && !name->empty()) {
        auto found = std::ranges::find_if(CONTROL_PRESETS, [&](const ControlPreset &p) {
            return EqualsIgnoringCase(p.name, *name);
        });
        if (found == CONTROL_PRESETS.end()) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                        "Controls Preset: there is no preset %s, and the preset %.*s is used.",
                        name->c_str(),
                        static_cast<int>(preset->name.size()),
                        preset->name.data());
        } else {
            preset = &*found;
        }
    }
    for (std::size_t i = 0; i < CONTROL_ACTION_COUNT; ++i) {
        auto value = config.Get("Controls", CONTROL_KEYS[i]);
        g_controls[i] = ParseSources(CONTROL_KEYS[i], value ? *value : preset->values[i]);
    }
}

bool IsControlAnalog(ControlAction action) {
    return std::ranges::any_of(g_controls[action], &ControlSource::axis);
}

int ReadControl(SDL_Gamepad *gamepad, ControlAction action) {
    int magnitude = 0;
    for (const ControlSource &source : g_controls[action]) {
        int value = 0;
        if (source.axis) {
            // A stick at rest reads a little off centre, and its other half would otherwise cancel
            // a full press of the same action from a button.
            constexpr int DEAD_ZONE = 7849;
            const int raw =
                std::max(0,
                         SDL_GetGamepadAxis(gamepad, static_cast<SDL_GamepadAxis>(source.index)) *
                             source.sign);
            value = raw <= DEAD_ZONE ?
                        0 :
                        std::min(CONTROL_FULL,
                                 (raw - DEAD_ZONE) * CONTROL_FULL / (CONTROL_FULL - DEAD_ZONE));
        } else if (SDL_GetGamepadButton(gamepad, static_cast<SDL_GamepadButton>(source.index))) {
            value = CONTROL_FULL;
        }
        magnitude = std::max(magnitude, value);
    }
    return magnitude;
}

} // namespace coastline
