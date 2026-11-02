#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <argparse/argparse.hpp>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#if !defined(_WIN32)
#include <sys/resource.h>
#endif

#include "assetimport.h"
#include "assetpack.h"
#include "common/framestatistics.h"
#include "common/game.h"
#include "common/hud.h"
#include "common/input.h"
#include "common/inputpoll.h"
#include "common/instantreplay.h"
#include "common/pause.h"
#include "common/rallyconfig.h"
#include "common/settings.h"
#include "common/soundbuffer.h"
#include "common/strings.h"
#include "config.h"
#include "controllerpak.h"
#include "controls.h"
#include "ear/sdlmixer.h"
#include "enginetextures.h"
#include "savefile.h"
#if COASTLINE_SETUP_WIZARD
#include "firstrun/wizardrun.h"
#endif
#include "discordpresence.h"
#include "frontend.h"
#include "gamemode.h"
#include "gamesettings.h"
#include "gpu.h"
#include "iconfile.h"
#include "importer.h"
#include "importversion.h"
#include "input.h"
#include "intro.h"
#include "language.h"
#include "log.h"
#include "menus.h"
#include "menutext.h"
#include "music.h"
#include "paths.h"
#include "performance.h"
#include "racecredits.h"
#include "racedemo.h"
#include "racemusic.h"
#include "racesession.h"
#include "raceview.h"
#include "runscript.h"
#include "runtimebuffers.h"
#include "scenerenderer.h"
#include "soundeffects.h"
#include "stringresources.h"
#include "trackload.h"

namespace {

constexpr const char *APPLICATION = "coastline";
#if defined(SDL_PLATFORM_MACOS) || defined(SDL_PLATFORM_WINDOWS)
constexpr const char *STORAGE_APPLICATION = "Coastline";
#else
constexpr const char *STORAGE_APPLICATION = APPLICATION;
#endif

const char *RegionName(const std::string &region) {
    if (region == "usa") {
        return "USA";
    }
    if (region == "jpn") {
        return "Japan";
    }
    if (region == "eur") {
        return "Europe";
    }
    return region.c_str();
}

struct SdlSession {
    bool ready = false;
    explicit SdlSession(SDL_InitFlags flags) : ready(SDL_Init(flags)) {
    }
    ~SdlSession() {
        coastline::ShutdownRaceMusic();
        SDL_Quit();
    }
    SdlSession(const SdlSession &) = delete;
    SdlSession &operator=(const SdlSession &) = delete;
};

auto RequiredArgumentValue(const char *option) {
    return [option](const std::string &value) {
        if (value.empty()) {
            throw std::invalid_argument(std::string(option) + " requires a value.");
        }
        return value;
    };
}

auto RequiredChoiceValue(const char *option, std::vector<std::string> choices) {
    return [option, choices = std::move(choices)](const std::string &value) {
        RequiredArgumentValue(option)(value);
        if (std::ranges::contains(choices, value)) {
            return value;
        }
        const std::string *match = nullptr;
        bool ambiguous = false;
        for (const auto &choice : choices) {
            if (choice.starts_with(value)) {
                if (match != nullptr) {
                    ambiguous = true;
                    break;
                }
                match = &choice;
            }
        }
        if (match == nullptr || ambiguous) {
            std::string message = "Invalid value '" + value + "' for " + option + ". Expected ";
            for (std::size_t index = 0; index < choices.size(); ++index) {
                if (index != 0) {
                    message += ", ";
                }
                message += choices[index];
            }
            throw std::invalid_argument(message + '.');
        }
        return *match;
    };
}

coastline::DisplayOptions DisplayOptionsFrom(const coastline::GameConfig &savedConfig,
                                             const argparse::ArgumentParser &parser,
                                             bool fixedFrame,
                                             bool setup = false) {
    auto config = savedConfig;
    if (const auto preset = parser.present("--preset")) {
        coastline::ApplyGraphicsPreset(config, *preset);
    }
    coastline::DisplayOptions options;
    options.width = static_cast<int>(config.GetInteger("Video", "Width", options.width));
    options.height = static_cast<int>(config.GetInteger("Video", "Height", options.height));
    options.fullscreen = config.GetBoolean("Video", "Fullscreen", options.fullscreen);
    options.exclusive = config.Get("Video", "FullscreenMode") == "exclusive";
    options.refreshRate = static_cast<int>(config.GetInteger("Video", "RefreshRate", 0));
    options.wideMenus = config.GetBoolean("Video", "WideMenus", false);
    options.autoWideMenus = config.Get("Video", "WideMenus") == "auto";
    options.fieldOfViewScale = config.GetFloat("Video", "FieldOfView", 1.0f);
    options.drawDistanceScale = config.GetFloat("Video", "DrawDistance", 1.0f);
    options.n64Effects = config.Get("Game", "Effects") == "N64";
    const auto reflections = config.Get("Game", "WindowReflections").value_or("auto");
    options.n64WindowReflections =
        reflections == "N64" || (reflections != "PC" && options.n64Effects);
    // VSync takes a present mode by name, or 1 and 0 for vertical sync and immediate. fifo is
    // vertical sync that waits for the vertical blank under Wayland too.
    auto present = config.Get("Video", "VSync");
    if (present == "mailbox") {
        options.presentMode = SDL_GPU_PRESENTMODE_MAILBOX;
    } else if (present == "fifo") {
        options.fifo = true;
    } else if (present == "immediate" ||
               (present != "vsync" && !config.GetBoolean("Video", "VSync", true))) {
        options.presentMode = SDL_GPU_PRESENTMODE_IMMEDIATE;
    }
    options.frameWidth = static_cast<int>(config.GetInteger("Video", "InternalWidth", 0));
    options.frameHeight = static_cast<int>(config.GetInteger("Video", "InternalHeight", 0));
    options.renderScale = std::clamp(config.GetFloat("Video", "RenderScale", 1.0f), 0.25f, 4.0f);
    // AspectRatio keeps a shape for the frame, auto for the shape of the window. --aspect-ratio
    // replaces it for this run.
    auto aspectText = parser.present("--aspect-ratio");
    if (!aspectText) {
        aspectText = config.Get("Video", "AspectRatio");
    }
    if (aspectText) {
        auto aspect = coastline::ParseAspectRatio(*aspectText);
        if (aspect) {
            options.aspectRatio = *aspect;
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                        "Invalid AspectRatio %s; using window aspect ratio.",
                        aspectText->c_str());
        }
    }
    if (config.Get("Video", "ScaleFilter") == "nearest") {
        options.scaleFilter = coastline::ScaleFilter::Nearest;
    } else if (config.Get("Video", "ScaleFilter") == "edge-adaptive") {
        options.scaleFilter = coastline::ScaleFilter::EdgeAdaptive;
    }
    const auto screenShader = config.Get("Video", "ScreenShader");
    if (screenShader == "consumer-crt") {
        options.screenShader = coastline::ScreenShader::ConsumerCrt;
    } else if (screenShader == "pvm") {
        options.screenShader = coastline::ScreenShader::Pvm;
    } else if (screenShader == "aperture-grille") {
        options.screenShader = coastline::ScreenShader::ApertureGrille;
    } else if (screenShader == "pc-crt") {
        options.screenShader = coastline::ScreenShader::PcCrt;
    }
    auto post = config.Get("Video", "AntialiasPost");
    if (post == "fxaa") {
        options.postAntialias = coastline::PostAntialias::Fxaa;
    } else if (post && *post != "none") {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                    "Invalid AntialiasPost %s; disabling post anti-aliasing.",
                    post->c_str());
    }
    options.gpuDebug = parser.get<bool>("--debug-gpu");
#ifndef NDEBUG
    options.minimized = parser.get<bool>("--minimized");
#endif
    // A capture is drawn at Width by Height, for the same frames on every machine.
    options.fixedFrame = fixedFrame;
    // Vulkan, Metal, and Direct3D 12 all filter at least 16 samples.
    options.anisotropy = static_cast<int>(
        std::clamp(config.GetInteger("Video", "AnisotropicFiltering", 0), 0L, 16L));
    options.alphaToCoverage = config.GetBoolean("Video", "AlphaToCoverage", true);
    auto filter = config.Get("Video", "TextureFilter");
    if (filter == "n64") {
        options.textureFilter = coastline::TextureFilter::N64;
    } else if (filter == "nearest") {
        options.textureFilter = coastline::TextureFilter::Nearest;
    } else if (filter && *filter != "modern") {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                    "Invalid TextureFilter %s; using modern filtering.",
                    filter->c_str());
    }
    options.antialias =
        static_cast<int>(config.GetInteger("Video", "Antialias", options.antialias));
    // Explicit display arguments override the preset after saved settings are read.
    if (auto preset = parser.present("--preset")) {
        if (*preset == "ultra") {
            options.frameWidth = 0;
            options.frameHeight = 0;
            options.renderScale = 1.0f;
            options.scaleFilter = coastline::ScaleFilter::Nearest;
            options.postAntialias = coastline::PostAntialias::None;
            options.screenShader = coastline::ScreenShader::None;
            options.drawDistanceScale = 2.0f;
            options.lowestFrame = false;
            options.antialias = 8;
            options.alphaToCoverage = true;
            options.anisotropy = 16;
            options.textureFilter = coastline::TextureFilter::Modern;
            options.fullscreen = true;
            options.exclusive = false;
        } else if (*preset == "worst") {
            options.frameWidth = 0;
            options.frameHeight = 0;
            options.renderScale = 1.0f;
            options.scaleFilter = coastline::ScaleFilter::Nearest;
            options.drawDistanceScale = 1.0f;
            options.antialias = 1;
            options.alphaToCoverage = false;
            options.postAntialias = coastline::PostAntialias::None;
            options.screenShader = coastline::ScreenShader::None;
            options.anisotropy = 0;
            options.textureFilter = coastline::TextureFilter::Nearest;
            options.lowestFrame = true;
        } else if (*preset != "n64" && *preset != "br" && *preset != "low-power") {
            SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Ignoring invalid GPU preset %s.", preset->c_str());
        }
    }
    if (auto antialias = parser.present<int>("--antialias")) {
        options.antialias = *antialias;
    }
    if (auto driver = config.Get("Video", "GpuDriver"); driver && !driver->empty()) {
        options.driver = *driver;
    }
    options.gpuPreference =
        parser.present("--gpu").value_or(config.Get("Video", "GpuPreference").value_or("auto"));
    if (options.gpuPreference != "auto" && options.gpuPreference != "high-performance" &&
        options.gpuPreference != "low-power") {
        SDL_LogWarn(SDL_LOG_CATEGORY_GPU, "Invalid GPU preference; using automatic selection.");
        options.gpuPreference = "auto";
    }
    options.unlockedFramerate = config.GetBoolean("Video", "UnlockedFramerate", false);
    if (auto width = parser.present<int>("--width")) {
        options.width = *width;
    }
    if (auto height = parser.present<int>("--height")) {
        options.height = *height;
    }
    if (parser.get<bool>("--fullscreen")) {
        options.fullscreen = true;
    }
    if (parser.get<bool>("--windowed")) {
        options.fullscreen = false;
    }
    if (parser.get<bool>("--no-vsync")) {
        options.presentMode = SDL_GPU_PRESENTMODE_IMMEDIATE;
    }
    if (setup) {
        coastline::DisplayOptions setupOptions;
        setupOptions.width = options.width;
        setupOptions.height = options.height;
        setupOptions.fullscreen = options.fullscreen;
        setupOptions.exclusive = options.exclusive;
        setupOptions.refreshRate = options.refreshRate;
        setupOptions.minimized = options.minimized;
        setupOptions.driver = options.driver;
        setupOptions.gpuPreference = options.gpuPreference;
        setupOptions.gpuDebug = options.gpuDebug;
        return setupOptions;
    }
    return options;
}

coastline::GameConfig CommandLineOverrides(const argparse::ArgumentParser &parser) {
    coastline::GameConfig overrides;
    if (const auto preset = parser.present("--preset")) {
        coastline::ApplyGraphicsPreset(overrides, *preset);
    }
    for (const auto &[option, section, key] : {std::tuple{"--aspect-ratio", "Video", "AspectRatio"},
                                               std::tuple{"--gpu", "Video", "GpuPreference"},
                                               std::tuple{"--asset-pack", "Game", "AssetPack"},
                                               std::tuple{"--region", "Game", "Region"},
                                               std::tuple{"--language", "Game", "Language"}}) {
        if (const auto value = parser.present(option)) {
            overrides.Set(section, key, *value);
        }
    }
    if (const auto samples = parser.present<int>("--antialias")) {
        overrides.Set("Video", "Antialias", std::to_string(*samples));
    }
    return overrides;
}

// The shortcuts of the window on every screen: Alt and Enter or F11 for the full screen, F for
// the frame rate, V for vertical sync, and Ctrl+U for an unlocked frame rate.
enum class DisplayKey { None, Fullscreen, FrameRate, VSync, Unlocked };

// The keys of the shortcuts held the frame before, from the keyboard and the scripted keys.
struct DisplayKeys {
    bool f11 = false;
    bool f = false;
    bool v = false;
    bool ctrlU = false;

    // The shortcut pressed since the frame before, F only when @p frameRate allows it, and V only
    // when @p vsync allows it.
    DisplayKey Read(bool frameRate, bool vsync) {
        bool alt =
            coastline::IsKeyDown(SDL_SCANCODE_LALT) || coastline::IsKeyDown(SDL_SCANCODE_RALT);
        // Enter of the shortcut is ignored until it is released, and does not also press Start or
        // choose.
        bool enter = false;
        for (SDL_Scancode key : {SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER}) {
            if (alt && coastline::IsKeyDown(key)) {
                coastline::IgnoreKeyUntilReleased(key);
                enter = true;
            }
        }
        bool f11Now = coastline::IsKeyDown(SDL_SCANCODE_F11);
        bool fNow = coastline::IsKeyDown(SDL_SCANCODE_F);
        bool fullscreen = enter || (f11Now && !f11);
        bool vNow = coastline::IsKeyDown(SDL_SCANCODE_V);
        const bool ctrlUNow = (coastline::IsKeyDown(SDL_SCANCODE_LCTRL) ||
                               coastline::IsKeyDown(SDL_SCANCODE_RCTRL)) &&
                              coastline::IsKeyDown(SDL_SCANCODE_U);
        const bool toggleUnlocked = ctrlUNow && !ctrlU;
        ctrlU = ctrlUNow;
        bool toggleRate = frameRate && fNow && !f;
        bool toggleVSync = vsync && vNow && !v;
        f11 = f11Now;
        f = fNow;
        v = vNow;
        if (fullscreen) {
            return DisplayKey::Fullscreen;
        }
        if (toggleUnlocked) {
            return DisplayKey::Unlocked;
        }
        if (toggleVSync) {
            return DisplayKey::VSync;
        }
        return toggleRate ? DisplayKey::FrameRate : DisplayKey::None;
    }
};

class FrameRateOverlay {
public:
    void Record();
    void Draw(coastline::SceneRenderer &renderer, const coastline::MenuText &text);

private:
    std::optional<unsigned> m_last;
    float m_rate = 30.0f;
};

// RecordFrameTime() of BRD3D measures milliseconds between frames for DrawFpsCounterOverlay().
void FrameRateOverlay::Record() {
    if (!m_last) {
        m_last = GetHighResolutionTime();
    }
    unsigned now = GetHighResolutionTime();
    unsigned delta = now - *m_last;
    m_last = now;
    int entry = g_dwFpsFrameCountdown;
    if (entry < 0) {
        // The first frame fills every sample with its time.
        std::fill_n(g_adwFpsFrameTimeSamples, g_dwFpsSampleFrames, delta);
        entry = g_dwFpsSampleFrames;
    }
    if (++entry >= g_dwFpsSampleFrames) {
        entry = 0;
    }
    g_dwFpsFrameCountdown = entry;
    g_adwFpsFrameTimeSamples[entry] = delta;
}

// Draw text on the game canvas, or in display-scaled output units when outputSize is supplied.
void DrawOverlayLine(coastline::SceneRenderer &renderer,
                     const coastline::MenuText &text,
                     const char *line,
                     int y,
                     int x = 320,
                     coastline::TextStyle::Align align = coastline::TextStyle::Align::Center,
                     bool white = false,
                     std::optional<std::array<float, 2>> outputSize = std::nullopt,
                     int fontSize = 15) {
    const coastline::GpuWindow &window = renderer.Window();
    coastline::TextStyle style;
    style.size = fontSize;
    style.align = align;
    style.doubled = false;
    if (white) {
        style.colors = std::array<int, 6>{255, 255, 255, 255, 255, 255};
    }
    auto projection =
        coastline::FlatProjection(static_cast<float>(window.FrameWidth()) /
                                  static_cast<float>(std::max(window.FrameHeight(), 1u)));
    if (outputSize) {
        projection = {2.0f / (*outputSize)[0],
                      0,
                      0,
                      0,
                      0,
                      -2.0f / (*outputSize)[1],
                      0,
                      0,
                      0,
                      0,
                      1,
                      0,
                      -1,
                      1,
                      0,
                      1};
    }
    text.Draw(renderer, line, x, y, style, projection);
}

// Draw the sampled frame rate with the PC font gradient on the 640 by 480 game canvas.
void FrameRateOverlay::Draw(coastline::SceneRenderer &renderer, const coastline::MenuText &text) {
    if (g_dwFpsFrameCountdown == 0) {
        unsigned total = 0;
        for (int index = 0; index < g_dwFpsSampleFrames; ++index) {
            total += g_adwFpsFrameTimeSamples[index];
        }
        m_rate = static_cast<float>(g_dwFpsSampleFrames) * 1000.0f /
                 static_cast<float>(std::max(total, 1u));
    }
    char line[32];
    std::snprintf(line, sizeof(line), "%6.2f FPS", static_cast<double>(m_rate));
    DrawOverlayLine(renderer, text, line, 470);
}

// A new file in a directory of screenshots, named by the local time to the millisecond, with the
// directory made when it does not exist.
std::optional<std::filesystem::path> ScreenshotPath(const std::filesystem::path &directory) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Cannot create %s: %s",
                    directory.string().c_str(),
                    error.message().c_str());
        return std::nullopt;
    }
    SDL_Time now = 0;
    SDL_DateTime time{};
    if (!SDL_GetCurrentTime(&now) || !SDL_TimeToDateTime(now, &time, true)) {
        return std::nullopt;
    }
    char name[64];
    std::snprintf(name,
                  sizeof(name),
                  "coastline-%04d%02d%02d-%02d%02d%02d-%03d.png",
                  time.year,
                  time.month,
                  time.day,
                  time.hour,
                  time.minute,
                  time.second,
                  time.nanosecond / 1000000);
    return directory / name;
}

// The keys of the default keyboard bindings of the PC release a press pushes down in a race.
constexpr std::array<std::pair<std::uint16_t, int>, 12> RACE_KEYS{{
    {coastline::BUTTON_A, SDL_SCANCODE_UP},
    {coastline::BUTTON_D_UP, SDL_SCANCODE_UP},
    {coastline::BUTTON_B, SDL_SCANCODE_DOWN},
    {coastline::BUTTON_D_DOWN, SDL_SCANCODE_DOWN},
    {coastline::BUTTON_D_LEFT, SDL_SCANCODE_LEFT},
    {coastline::BUTTON_D_RIGHT, SDL_SCANCODE_RIGHT},
    {coastline::BUTTON_START, SDL_SCANCODE_ESCAPE},
    {coastline::BUTTON_Z, SDL_SCANCODE_SPACE},
    {coastline::BUTTON_C_UP, SDL_SCANCODE_DELETE},
    {coastline::BUTTON_C_LEFT, SDL_SCANCODE_END},
    {coastline::BUTTON_C_DOWN, SDL_SCANCODE_PAGEDOWN},
    {coastline::BUTTON_C_RIGHT, SDL_SCANCODE_RETURN},
}};

// The script of run-script, with the scancode of each of its keys in order, or nothing with the
// line that cannot be read logged.
struct LoadedScript {
    coastline::Script script;
    std::vector<SDL_Scancode> keys;
};

std::optional<LoadedScript> LoadScript(const std::filesystem::path &path) {
    const std::string source = path == "-" ? "<stdin>" : path.string();
    std::ifstream file;
    std::istream *input = &std::cin;
    if (path != "-") {
        file.open(path, std::ios::binary);
        input = &file;
    }
    if (!*input) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot read %s.", source.c_str());
        return std::nullopt;
    }
    std::string text;
    std::array<char, 8192> buffer;
    while (input->read(buffer.data(), buffer.size()) || input->gcount() != 0) {
        text.append(buffer.data(), static_cast<std::size_t>(input->gcount()));
    }
    if (!input->eof() || input->bad()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot read %s.", source.c_str());
        return std::nullopt;
    }
    auto script = coastline::ParseScript(text);
    if (!script) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s:%d: %s",
                     source.c_str(),
                     script.error().line,
                     script.error().message.c_str());
        return std::nullopt;
    }
    LoadedScript loaded{std::move(*script), {}};
    for (const coastline::ScriptKey &key : loaded.script.keys) {
        SDL_Scancode scancode = SDL_GetScancodeFromName(key.name.c_str());
        static constexpr std::array<std::pair<std::string_view, SDL_Scancode>, 4> MODIFIERS{
            {{"Ctrl", SDL_SCANCODE_LCTRL},
             {"Alt", SDL_SCANCODE_LALT},
             {"Shift", SDL_SCANCODE_LSHIFT},
             {"Gui", SDL_SCANCODE_LGUI}}};
        if (const auto modifier =
                std::ranges::find(MODIFIERS, key.name, &decltype(MODIFIERS)::value_type::first);
            modifier != MODIFIERS.end()) {
            scancode = modifier->second;
        }
        if (scancode == SDL_SCANCODE_UNKNOWN) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "%s:%d: Invalid SDL key %s.",
                         source.c_str(),
                         key.line,
                         key.name.c_str());
            return std::nullopt;
        }
        loaded.keys.push_back(scancode);
    }
    return loaded;
}

// Push the events of the keyboard of the typings of a frame, as SDL sends them while text input
// is on.
void PushScriptedTyping(const std::vector<coastline::ScriptTyping> &typing,
                        int frame,
                        SDL_Window *window) {
    static constexpr std::array<std::pair<std::string_view, SDL_Keycode>, 2> KEYS{
        {{"<backspace>", SDLK_BACKSPACE}, {"<enter>", SDLK_RETURN}}};
    if (!SDL_TextInputActive(window)) {
        return;
    }
    for (const coastline::ScriptTyping &typed : typing) {
        if (typed.frame != frame) {
            continue;
        }
        SDL_Event event{};
        event.common.timestamp = SDL_GetTicksNS();
        auto key =
            std::ranges::find(KEYS, typed.text, &std::pair<std::string_view, SDL_Keycode>::first);
        if (key == KEYS.end()) {
            event.type = SDL_EVENT_TEXT_INPUT;
            event.text.windowID = SDL_GetWindowID(window);
            event.text.text = typed.text.c_str();
        } else {
            event.type = SDL_EVENT_KEY_DOWN;
            event.key.windowID = SDL_GetWindowID(window);
            event.key.key = key->second;
            event.key.down = true;
        }
        SDL_PushEvent(&event);
    }
}

// The mouse of a frame, where it is on the 640 by 480 screen of the menus and its buttons, and
// where it was the frame before.
struct MouseState {
    std::array<int, 2> at{};
    std::uint32_t buttons = 0;
    std::optional<std::array<int, 2>> last;
};

// The place of the mouse in the window on the 640 by 480 screen, centred at 4:3 in the window as
// the flat draws of the menus are.
std::array<int, 2> MouseOnScreen(const coastline::GpuWindow &window, float x, float y) {
    int width = 1;
    int height = 1;
    SDL_GetWindowSize(window.Window(), &width, &height);
    auto windowWidth = static_cast<float>(std::max(width, 1));
    auto windowHeight = static_cast<float>(std::max(height, 1));
    // The frame is fitted to the window as Present() fits it, and the screen to the frame.
    float frameAspect = static_cast<float>(window.FrameWidth()) /
                        static_cast<float>(std::max(window.FrameHeight(), 1u));
    float fit = std::min(windowWidth / frameAspect, windowHeight);
    float frameX = (x - (windowWidth - fit * frameAspect) / 2.0f) / (fit * frameAspect);
    float frameY = (y - (windowHeight - fit) / 2.0f) / fit;
    auto [across, down] = coastline::ScreenExtent(frameAspect);
    float clipX = frameX * 2.0f - 1.0f;
    float clipY = frameY * 2.0f - 1.0f;
    return {static_cast<int>((clipX / across + 1.0f) * 320.0f),
            static_cast<int>((clipY / down + 1.0f) * 240.0f)};
}

// The input of the paint shop from the first controller and the mouse: the buttons of the mouse
// are A and B, and the mouse moves the cursor when it moves.
coastline::PaintShopInput PaintShopInputOf(const coastline::ControllerState &port,
                                           MouseState &mouse) {
    static constexpr std::array<std::pair<std::uint16_t, std::uint32_t>, 14> BUTTONS{
        {{coastline::BUTTON_D_UP, INPUT_UP},
         {coastline::BUTTON_D_DOWN, INPUT_DOWN},
         {coastline::BUTTON_D_LEFT, INPUT_LEFT},
         {coastline::BUTTON_D_RIGHT, INPUT_RIGHT},
         {coastline::BUTTON_A, INPUT_A},
         {coastline::BUTTON_B, INPUT_B},
         {coastline::BUTTON_L, INPUT_L},
         {coastline::BUTTON_R, INPUT_R},
         {coastline::BUTTON_Z, INPUT_Z},
         {coastline::BUTTON_START, INPUT_START},
         {coastline::BUTTON_C_UP, INPUT_C_UP},
         {coastline::BUTTON_C_RIGHT, INPUT_C_RIGHT},
         {coastline::BUTTON_C_DOWN, INPUT_C_DOWN},
         {coastline::BUTTON_C_LEFT, INPUT_C_LEFT}}};
    // TranslateControllerInput() of the console scales the stick by 70.
    constexpr float STICK_RANGE = 70.0f;
    coastline::PaintShopInput input;
    for (const auto &[button, bit] : BUTTONS) {
        if ((port.buttons & button) != 0) {
            input.buttons |= bit;
        }
    }
    if ((mouse.buttons & SDL_BUTTON_LMASK) != 0) {
        input.buttons |= INPUT_A;
    }
    if ((mouse.buttons & SDL_BUTTON_RMASK) != 0) {
        input.buttons |= INPUT_B;
    }
    input.stickX = static_cast<float>(port.stickX) / STICK_RANGE;
    input.stickY = static_cast<float>(port.stickY) / STICK_RANGE;
    input.enter =
        coastline::IsKeyDown(SDL_SCANCODE_RETURN) || coastline::IsKeyDown(SDL_SCANCODE_KP_ENTER);
    input.escape = coastline::IsKeyDown(SDL_SCANCODE_ESCAPE);
    if (!mouse.last || *mouse.last != mouse.at) {
        input.pointer = mouse.at;
    }
    mouse.last = mouse.at;
    return input;
}

// Raise the soft limit of open files to the hard limit. The Vulkan driver of NVIDIA keeps a
// descriptor for each allocation of GPU memory, and the textures of a race need more than the
// usual soft limit of 1024.
void RaiseOpenFileLimit() {
#if !defined(_WIN32)
    rlimit limit{};
    if (getrlimit(RLIMIT_NOFILE, &limit) == 0 && limit.rlim_cur < limit.rlim_max) {
        limit.rlim_cur = limit.rlim_max;
        setrlimit(RLIMIT_NOFILE, &limit);
    }
#endif
}

// The icon of the window from the PNG images of icon in the default pack: the smallest at the scale
// of the display, and each larger size as its form for a higher scale.
void SetWindowIcon(SDL_Window *window, const std::filesystem::path &pack) {
    std::vector<std::pair<coastline::IconSize, std::filesystem::path>> images;
    std::error_code error;
    for (const auto &file : std::filesystem::directory_iterator(pack / "icon", error)) {
        if (auto size = coastline::ParseIconName(file.path().filename().string())) {
            images.emplace_back(*size, file.path());
        }
    }
    if (images.empty()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                    "Missing icon images in %s; import the disc again.",
                    (pack / "icon").string().c_str());
        return;
    }
    // Each size takes the image of the most bits per pixel.
    std::ranges::sort(images, {}, [](const auto &image) {
        return std::tuple(image.first.width, image.first.height, -image.first.bitCount);
    });
    struct SurfaceDeleter {
        void operator()(SDL_Surface *surface) const {
            SDL_DestroySurface(surface);
        }
    };
    std::unique_ptr<SDL_Surface, SurfaceDeleter> icon;
    coastline::IconSize previous;
    for (const auto &[size, path] : images) {
        if (size.width == previous.width && size.height == previous.height) {
            continue;
        }
        previous = size;
        std::unique_ptr<SDL_Surface, SurfaceDeleter> image(SDL_LoadPNG(path.string().c_str()));
        if (!image) {
            SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                        "Cannot read %s: %s",
                        path.string().c_str(),
                        SDL_GetError());
        } else if (!icon) {
            icon = std::move(image);
        } else {
            SDL_AddSurfaceAlternateImage(icon.get(), image.get());
        }
    }
    if (!icon || !SDL_SetWindowIcon(window, icon.get())) {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO, "Cannot set window icon: %s", SDL_GetError());
        return;
    }
    int count = 0;
    SDL_free(SDL_GetSurfaceImages(icon.get(), &count));
    SDL_Log("Using %dx%d icon, %d %s, from %s.",
            icon->w,
            icon->h,
            count,
            count == 1 ? "size" : "sizes",
            (pack / "icon").string().c_str());
}

int ImportFromCommandLine(const argparse::ArgumentParser &import,
                          const std::filesystem::path &data) {
    std::vector<std::filesystem::path> roms;
    for (const auto &rom : import.get<std::vector<std::string>>("--rom")) {
        roms.emplace_back(rom);
    }
    std::vector<std::filesystem::path> sources;
    auto failure = coastline::ImportAssetImages(
        data,
        import.present("--cd").value_or(""),
        roms,
        [](const std::string &step, double fraction) {
            SDL_Log("[%3d%%] %s", static_cast<int>(fraction * 100), step.c_str());
            return true;
        },
        import.is_used("--depfile") ? &sources : nullptr);
    if (!failure && import.is_used("--depfile")) {
        failure = coastline::WriteImportDependencies(import.get<std::string>("--depfile"),
                                                     import.get<std::string>("--depfile-target"),
                                                     sources);
    }
    if (failure) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", failure->c_str());
    }
    return failure ? EXIT_FAILURE : EXIT_SUCCESS;
}

// The notes of the subcommand import-pak, or of export-pak when @p exporting.
int PakFromCommandLine(const argparse::ArgumentParser &command,
                       bool exporting,
                       const coastline::GamePaths &paths) {
    coastline::ControllerPak pak(paths.data / "controller-pak", paths.config / "controller-pak");
    std::optional<std::string> failure;
    if (exporting) {
        auto name = command.get<std::string>("--release");
        auto release = name == "eur" ? coastline::PakRelease::Eur :
                       name == "jpn" ? coastline::PakRelease::Jpn :
                                       coastline::PakRelease::Usa;
        failure = coastline::ExportPakImage(pak, command.get<std::string>("file"), release);
    } else {
        failure = coastline::ImportPakImage(pak, command.get<std::string>("file"));
    }
    if (failure) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", failure->c_str());
    }
    return failure ? EXIT_FAILURE : EXIT_SUCCESS;
}

} // namespace

int main(int argc, char *argv[]) {
    RaiseOpenFileLimit();
    argparse::ArgumentParser parser(APPLICATION, COASTLINE_VERSION);
    parser.add_description("Top Gear Rally and Boss Rally in one stand-alone game.");
    if (coastline::PortablePathsEnabled()) {
        parser.add_argument("--portable", "-P")
            .help("Store assets, settings, cache, and state beside the application.")
            .flag();
    }
#if COASTLINE_SETUP_WIZARD
    parser.add_argument("--reset")
        .help("Reset imported assets and asset caches; preserve saves, decals, settings, and mods.")
        .flag();
    parser.add_argument("--factory-reset")
        .help("Delete all configuration, data, saved games, decals, mods, state, and cache.")
        .flag();
    parser.add_argument("--yes-really", "-y")
        .help("Confirm either reset without prompting.")
        .flag();
#endif
    parser.add_argument("--data-dir")
        .action(RequiredArgumentValue("--data-dir"))
        .help("read the game data from this directory");
    parser.add_argument("--config")
        .action(RequiredArgumentValue("--config"))
        .metavar("INI")
        .help("Read and save settings in the specified INI file.");
    parser.add_argument("--asset-pack")
        .action(RequiredArgumentValue("--asset-pack"))
        .help("asset pack for this run, not saved: default, boss for the images of the PC "
              "release in place of the branded images of the cartridges, or the name of a mod "
              "pack under assets of the data directory, read over the default pack");
    parser.add_argument("--language")
        .action(RequiredArgumentValue("--language"))
        .help("language of the text for this run, not saved: en-us for the text of the USA "
              "cartridge, en-gb for the font and the text of the European cartridge, or ja for "
              "the font and the Japanese text of the Japanese cartridge, the strings of "
              "strings/LANGUAGE.json");
    parser.add_argument("--region")
        .action(RequiredChoiceValue("--region", {"usa", "eur", "jpn"}))
        .metavar("{usa,eur,jpn}")
        .help("Select cartridge region for models, images, music, and intro, independent of "
              "language. Override the saved title-screen choice for this run.");
    parser.add_argument("--width").help("width of the window in pixels").scan<'i', int>();
    parser.add_argument("--height").help("height of the window in pixels").scan<'i', int>();
    parser.add_argument("--fullscreen").help("cover the display").flag();
    parser.add_argument("--windowed").help("draw in a window").flag();
    parser.add_argument("--antialias")
        .help("samples of each pixel for anti-aliasing: 1 for none, or 2, 4, or 8")
        .scan<'i', int>();
    parser.add_argument("--no-vsync").help("present frames without waiting").flag();
    parser.add_argument("--aspect-ratio")
        .action(RequiredArgumentValue("--aspect-ratio"))
        .help("shape of the frame for this run, in a window and in the full screen, with bars "
              "around it: auto for the shape of the window, a ratio such as 4:3, or a number such "
              "as 1.6; replaces AspectRatio of the settings");
    parser.add_argument("--preset")
        .action(RequiredChoiceValue("--preset", {"n64", "br", "ultra", "worst", "low-power"}))
        .metavar("{n64,br,ultra,worst,low-power}")
        .help("Graphics preset for this run, not saved. n64 selects console graphics and Consumer "
              "CRT; br selects PC graphics and PC CRT. ultra selects maximum quality and full "
              "screen; worst disables filters at minimum quality; low-power requests a lower-power "
              "GPU with moderate settings. Unique prefixes are accepted. Explicit display "
              "arguments still apply.");
    parser.add_argument("--gpu")
        .action(RequiredChoiceValue("--gpu", {"auto", "high-performance", "low-power"}))
        .metavar("{auto,high-performance,low-power}")
        .help("Select a GPU preference for this run; auto uses SDL's default selection.");
#ifndef NDEBUG
    parser.add_argument("--minimized")
        .help("start with the window minimized, for runs that save their frames unattended")
        .flag();
#endif
    parser.add_argument("--debug-gpu")
        .help("check the use of the GPU, with the validation layers of Vulkan when they are "
              "installed, and log what they find; slower")
        .flag();
    parser.add_argument("--debug")
        .help("log the debug messages of every category of SDL and of the game")
        .flag();

    argparse::ArgumentParser import("import", "", argparse::default_arguments::help);
    import.add_description("Import a Boss Rally disc source and every supplied Top Gear Rally ROM "
                           "image as one batch, replacing generated packs and preserving mod "
                           "packs. Both --cd and at least one --rom are required.");
    import.add_argument("--cd")
        .action(RequiredArgumentValue("--cd"))
        .metavar("DISC")
        .help("copy Boss Rally data from a disc image, PDI, unencrypted ISZ, or mounted disc "
              "directory; "
              "BIN, CUE, CloneCD, CDI, MDS, NRG, and BlindWrite images can also provide CD "
              "music");
    import.add_argument("--rom")
        .action(RequiredArgumentValue("--rom"))
        .metavar("ROM")
        .help("copy a Top Gear Rally cartridge image, checked by SHA-1; repeat for each release")
        .append();
    import.add_argument("--depfile")
        .action(RequiredArgumentValue("--depfile"))
        .metavar("FILE")
        .help("Write GCC-style source dependencies after a successful import; requires "
              "--depfile-target.");
    import.add_argument("--depfile-target")
        .action(RequiredArgumentValue("--depfile-target"))
        .metavar("TARGET")
        .help("Build output for --depfile; requires --depfile.");
    parser.add_subparser(import);

    argparse::ArgumentParser importPak("import-pak", "", argparse::default_arguments::help);
    importPak.add_description("Copy the notes of any release of Top Gear Rally from a Controller "
                              "Pak image, and exit.");
    importPak.add_argument("file")
        .action(RequiredArgumentValue("file"))
        .help("Controller Pak image (.mpk)");
    parser.add_subparser(importPak);

    argparse::ArgumentParser exportPak("export-pak", "", argparse::default_arguments::help);
    exportPak.add_description("Write the saved notes to a Controller Pak image, and exit. The "
                              "first pak of an image of four paks of mupen64plus is replaced, and "
                              "the other paks are retained.");
    exportPak.add_argument("--release")
        .action(RequiredChoiceValue("--release", {"usa", "eur", "jpn"}))
        .default_value(std::string("usa"))
        .nargs(1)
        .metavar("{usa,eur,jpn}")
        .help("release of the cartridge that reads the notes: usa, eur, or jpn");
    exportPak.add_argument("file")
        .action(RequiredArgumentValue("file"))
        .help("Controller Pak image (.mpk)");
    parser.add_subparser(exportPak);

    argparse::ArgumentParser runScript("run-script", "", argparse::default_arguments::help);
    runScript.add_description(
        "Start the game driven by the input of a script, and save its frames to screenshots in "
        "the data directory. Each line of the script is a command, with # starting a comment, and "
        "a line that starts with a frame number acts at that frame. The commands are autopilot, "
        "screenshot-every N, FRAME press BUTTON [FRAMES [PORT]], FRAME key KEY [FRAMES], FRAME "
        "type TEXT, FRAME mouse X Y [BUTTON [FRAMES]], FRAME screenshot, and FRAME quit.");
    runScript.add_argument("script")
        .nargs(argparse::nargs_pattern::optional)
        .default_value(std::string("-"))
        .action(RequiredArgumentValue("script"))
        .help("Read a script file, or standard input when omitted or '-'. Wait for EOF before "
              "starting.");
    parser.add_subparser(runScript);

    argparse::ArgumentParser migratePortable(
        "migrate-portable", "", argparse::default_arguments::help);
    migratePortable.add_description(
        "Move portable assets, settings, saves, and state to normal platform directories. "
        "Existing destination entries are never overwritten. Cache is regenerated.");
    migratePortable.add_argument("--copy")
        .help("Copy data and preserve the portable installation.")
        .flag();
    migratePortable.add_argument("directory")
        .nargs(argparse::nargs_pattern::optional)
        .default_value(std::string())
        .action(RequiredArgumentValue("directory"))
        .help("CoastlineData or its parent directory; omit to search beside the app and in the "
              "launch directory.");
    parser.add_subparser(migratePortable);

    argparse::ArgumentParser makePortable("make-portable", "", argparse::default_arguments::help);
#if defined(_WIN32) || defined(__APPLE__)
    constexpr const char *PORTABLE_EXPORT_DIRECTORY = "CoastlinePortable";
    makePortable.add_description(
        "Export installed program files and normal platform data as a complete portable "
        "distribution. Source program files are preserved.");
#else
    constexpr const char *PORTABLE_EXPORT_DIRECTORY = "CoastlineData";
    makePortable.add_description(
        "Export normal platform assets, settings, saves, and state as a portable data directory. "
        "The executable is not copied. Existing destination entries are never overwritten.");
#endif
    makePortable.add_argument("--copy")
        .help("Copy data and preserve the non-portable installation.")
        .flag();
    makePortable.add_argument("directory")
        .nargs(argparse::nargs_pattern::optional)
        .default_value(std::string())
        .action(RequiredArgumentValue("directory"))
        .help(std::string("Destination directory; defaults to ") + PORTABLE_EXPORT_DIRECTORY +
              " in the launch directory.");
    parser.add_subparser(makePortable);

    try {
        parser.parse_args(argc, argv);
        for (const char *option : {"--width", "--height"}) {
            if (const auto value = parser.present<int>(option);
                value && (*value < 1 || *value > 16384)) {
                throw std::invalid_argument(std::string(option) + " must be from 1 to 16384.");
            }
        }
        if (const auto samples = parser.present<int>("--antialias");
            samples && *samples != 1 && *samples != 2 && *samples != 4 && *samples != 8) {
            throw std::invalid_argument("--antialias must be 1, 2, 4, or 8.");
        }
        if (const auto aspect = parser.present("--aspect-ratio");
            aspect && !coastline::ParseAspectRatio(*aspect)) {
            throw std::invalid_argument("--aspect-ratio requires auto or a positive finite ratio.");
        }
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        for (const argparse::ArgumentParser *command :
             {&import, &importPak, &exportPak, &runScript, &migratePortable, &makePortable}) {
            if (parser.is_subcommand_used(*command)) {
                std::cerr << *command;
                return EXIT_FAILURE;
            }
        }
        std::cerr << parser;
        return EXIT_FAILURE;
    }
    if (parser.is_subcommand_used(import)) {
        if (!import.is_used("--cd") || !import.is_used("--rom")) {
            std::cerr << "import requires --cd and at least one --rom.\n" << import;
            return EXIT_FAILURE;
        }
        if (import.is_used("--depfile") != import.is_used("--depfile-target")) {
            std::cerr << "import requires --depfile and --depfile-target together.\n" << import;
            return EXIT_FAILURE;
        }
    }

    if (parser.is_subcommand_used(import) && parser.is_used("--data-dir")
#if COASTLINE_SETUP_WIZARD
        && !parser.get<bool>("--reset") && !parser.get<bool>("--factory-reset")
#endif
    ) {
        std::error_code error;
        auto data = std::filesystem::absolute(parser.get<std::string>("--data-dir"), error);
        if (!error) {
            std::filesystem::create_directories(data, error);
        }
        if (error) {
            std::cerr << "The import data directory cannot be created.\n";
            return EXIT_FAILURE;
        }
        SDL_SetMainReady();
        return ImportFromCommandLine(import, data);
    }
    auto paths = coastline::ResolveGamePaths(STORAGE_APPLICATION);
    if (!paths) {
        paths = coastline::GamePaths{};
    }
    SDL_SetMainReady();
    const char *base = SDL_GetBasePath();
    const std::string_view baseText = base ? base : "";
    const std::filesystem::path executable(std::u8string(baseText.begin(), baseText.end()));
    std::filesystem::path appImage;
#ifdef __linux__
    if (const char *image = std::getenv("APPIMAGE")) {
        const std::string_view text(image);
        appImage = std::filesystem::path(std::u8string(text.begin(), text.end()));
    }
#endif
    const auto portableRoot = coastline::PortableRoot(executable, appImage);
    if (parser.is_subcommand_used(makePortable)) {
        if (parser.is_used("--config") || parser.is_used("--data-dir")) {
            std::cerr << "Portable export requires default platform paths.\n";
            return EXIT_FAILURE;
        }
        const auto directory = makePortable.get<std::string>("directory");
        std::error_code error;
        const auto destination = std::filesystem::absolute(
            directory.empty() ? std::filesystem::path(PORTABLE_EXPORT_DIRECTORY) :
                                std::filesystem::path(directory),
            error);
        if (error) {
            std::cerr << "Cannot resolve portable export directory.\n";
            return EXIT_FAILURE;
        }
        const bool copy = makePortable.get<bool>("--copy");
        const auto failure =
#if defined(_WIN32) || defined(__APPLE__)
            coastline::MakePortableDistribution(*paths, executable, destination, copy);
#else
            coastline::MakePortablePaths(*paths, destination, copy);
#endif
        if (failure) {
            std::cerr << *failure << '\n';
            return EXIT_FAILURE;
        }
        std::cout << (copy ? "Copied" : "Moved") << " platform data to " << destination.string()
                  << ".\n";
        return EXIT_SUCCESS;
    }
    if (parser.is_subcommand_used(migratePortable)) {
        if (!coastline::PortablePathsEnabled() || parser.is_used("--config") ||
            parser.is_used("--data-dir")) {
            std::cerr << "Migration requires portable support and default platform paths.\n";
            return EXIT_FAILURE;
        }
        const auto directory = migratePortable.get<std::string>("directory");
        std::error_code error;
        const auto launch = std::filesystem::current_path(error);
        auto root = directory.empty() ? portableRoot : std::filesystem::absolute(directory, error);
        if (error) {
            std::cerr << "Cannot resolve portable migration directory.\n";
            return EXIT_FAILURE;
        }
        if (std::filesystem::is_directory(root / "CoastlineData", error)) {
            root /= "CoastlineData";
        }
        auto working = directory.empty() ? launch : std::filesystem::path{};
        if (!working.empty() && std::filesystem::is_directory(working / "CoastlineData", error)) {
            working /= "CoastlineData";
        }
        const auto source = coastline::ResolvePortablePaths(*paths, root, working);
        if (!source) {
            std::cerr << source.error() << '\n';
            return EXIT_FAILURE;
        }
        const bool copy = migratePortable.get<bool>("--copy");
        if (const auto failure = coastline::MigratePortablePaths(*source, *paths, copy)) {
            std::cerr << *failure << '\n';
            return EXIT_FAILURE;
        }
        std::cout << (copy ? "Copied" : "Moved") << " portable data to platform directories.\n"
                  << "Use COASTLINE_PORTABLE=0 to select platform storage on the next launch.\n";
        return EXIT_SUCCESS;
    }
    bool forcePortable = coastline::PortablePathsEnabled() && parser.get<bool>("--portable");
    bool disablePortable = false;
    if (const char *value = std::getenv("COASTLINE_PORTABLE"); value && *value && !forcePortable) {
        if (std::string_view(value) == "1" || SDL_strcasecmp(value, "true") == 0 ||
            SDL_strcasecmp(value, "yes") == 0 || SDL_strcasecmp(value, "on") == 0) {
            forcePortable = true;
        } else if (std::string_view(value) == "0" || SDL_strcasecmp(value, "false") == 0 ||
                   SDL_strcasecmp(value, "no") == 0 || SDL_strcasecmp(value, "off") == 0) {
            disablePortable = true;
        } else {
            std::cerr << "Invalid COASTLINE_PORTABLE value. Use 1 or 0.\n";
            return EXIT_FAILURE;
        }
    }
    if (forcePortable && !coastline::PortablePathsEnabled()) {
        std::cerr << "Portable mode is unavailable in this build.\n";
        return EXIT_FAILURE;
    }
    if (forcePortable) {
        std::error_code error;
        if (!portableRoot.is_absolute()) {
            std::cerr << "Cannot resolve portable directory.\n";
            return EXIT_FAILURE;
        }
        std::filesystem::create_directories(portableRoot / "assets", error);
        if (error) {
            std::cerr << "Cannot create portable assets directory. " << error.message() << '\n';
            return EXIT_FAILURE;
        }
    }
    std::error_code workingError;
    const auto workingDirectory = std::filesystem::current_path(workingError);
    auto portable = disablePortable ?
                        std::expected<coastline::GamePaths, std::string>(*paths) :
                        coastline::ResolvePortablePaths(*paths,
                                                        portableRoot,
                                                        forcePortable || workingError ?
                                                            std::filesystem::path{} :
                                                            workingDirectory);
    if (!portable) {
        std::cerr << portable.error() << '\n';
        return EXIT_FAILURE;
    }
    paths = *portable;
    if (forcePortable) {
        paths->config = portableRoot;
        paths->portableConfig = true;
        const auto ini = portableRoot / "BossRally.ini";
        auto settings = coastline::SaveFile::Open(ini);
        std::error_code error;
        const bool exists = std::filesystem::exists(ini, error);
        if (!settings || error || (exists && !std::filesystem::is_regular_file(ini, error)) ||
            (!exists && !settings->Write({}))) {
            std::cerr << "Cannot initialize portable settings.\n";
            return EXIT_FAILURE;
        }
    }
    if (paths->config.empty() || paths->data.empty() || paths->state.empty() ||
        paths->cache.empty()) {
        std::cerr << "The home directory is not known, and the game cannot find its files.\n";
        return EXIT_FAILURE;
    }
    // The game moves to the data directory, and a relative directory would then name another.
    if (auto dataDirectory = parser.present("--data-dir")) {
        std::error_code error;
        paths->data = std::filesystem::absolute(*dataDirectory, error).make_preferred();
        paths->portableData = false;
        if (error) {
            std::cerr << "The data directory cannot be resolved.\n";
            return EXIT_FAILURE;
        }
    }
    auto configPath = paths->config / "BossRally.ini";
    if (auto ini = parser.present("--config")) {
        std::error_code error;
        configPath = std::filesystem::absolute(*ini, error).make_preferred();
        if (error) {
            std::cerr << "The configuration file path cannot be resolved.\n";
            return EXIT_FAILURE;
        }
    }
#if COASTLINE_SETUP_WIZARD
    const bool factoryReset = parser.get<bool>("--factory-reset");
    if (parser.get<bool>("--reset") || factoryReset) {
        if (!parser.get<bool>("--yes-really")) {
            if (factoryReset) {
                std::cout << "Factory reset deletes all Coastline configuration, imported assets, "
                             "mods, saved games, decals, state, and cache in the following "
                             "locations:\n";
                for (const auto *directory :
                     {&paths->config, &paths->data, &paths->state, &paths->cache}) {
                    std::cout << "  " << directory->string() << '\n';
                }
                if (parser.is_used("--config")) {
                    std::cout << "  " << configPath.string() << " (INI file)\n";
                }
            } else {
                std::cout << "Reset removes generated asset packs and asset caches from the "
                             "following directories:\n"
                          << "  " << (paths->data / "assets").string() << '\n'
                          << "  " << paths->cache.string() << '\n'
                          << "Saves, decals, settings, and custom asset packs are preserved.\n";
            }
            std::string response;
            do {
                std::cout << (factoryReset ? "Delete all game data and start Coastline? [y/n] " :
                                             "Reset imported assets and start Coastline? [y/n] ")
                          << std::flush;
                if (!(std::cin >> response) || response == "n" || response == "N") {
                    std::cout << "Reset cancelled.\n";
                    return EXIT_SUCCESS;
                }
            } while (response != "y" && response != "Y");
        }
        if (auto failure = coastline::ResetGamePaths(*paths, factoryReset)) {
            std::cerr << *failure << '\n';
            return EXIT_FAILURE;
        }
        if (factoryReset && parser.is_used("--config")) {
            std::error_code error;
            std::filesystem::remove(configPath, error);
            if (error) {
                std::cerr << "Cannot delete " << configPath.string() << ". " << error.message()
                          << '\n';
                return EXIT_FAILURE;
            }
        }
    }
#endif
    if (!coastline::CreateGamePaths(*paths)) {
        std::cerr << "The directories of the game cannot be created.\n";
        return EXIT_FAILURE;
    }
    {
        std::error_code error;
        std::filesystem::create_directories(configPath.parent_path(), error);
        if (error) {
            std::cerr << "The configuration directory cannot be created.\n";
            return EXIT_FAILURE;
        }
    }
    SDL_SetMainReady();
    SDL_SetAppMetadata("Coastline", COASTLINE_VERSION, "sh.tat.coastline");
    // SDL logs only the errors of its categories other than the application, so the warnings of
    // the video, the GPU, and the audio reach the log too. SDL_LOGGING chooses its own levels.
    // --debug replaces both.
    if (parser.get<bool>("--debug")) {
        SDL_SetLogPriorities(SDL_LOG_PRIORITY_DEBUG);
    } else if (SDL_GetHint(SDL_HINT_LOGGING) == nullptr) {
        SDL_SetLogPriorities(SDL_LOG_PRIORITY_WARN);
        SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
    }
    auto gameLog = std::make_unique<coastline::GameLog>(paths->state / "game.log");
    if (!gameLog->IsOpen()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot open log file.");
    }
    std::string revision = COASTLINE_GIT_SHA;
    if (revision.empty()) {
        revision = "v" COASTLINE_VERSION;
    } else if (!revision.starts_with('v')) {
        revision.resize(std::min<std::size_t>(7, revision.size()));
    }
    SDL_Log("Coastline %s (https://github.com/Tatsh/coastline), built %s.",
            revision.c_str(),
            COASTLINE_BUILD_TIMESTAMP);
    SDL_Log("Using configuration directory %s.", paths->config.string().c_str());
    SDL_Log("Reading settings from %s.", configPath.string().c_str());
    SDL_Log("Using data directory %s.", paths->data.string().c_str());
    SDL_Log("Using state directory %s.", paths->state.string().c_str());
    SDL_Log("Using cache directory %s.", paths->cache.string().c_str());

    if (parser.is_subcommand_used(import)) {
        return ImportFromCommandLine(import, paths->data);
    }
    if (parser.is_subcommand_used(importPak) || parser.is_subcommand_used(exportPak)) {
        bool exporting = parser.is_subcommand_used(exportPak);
        return PakFromCommandLine(exporting ? exportPak : importPak, exporting, *paths);
    }
    LoadedScript script;
    if (parser.is_subcommand_used(runScript)) {
        auto loaded = LoadScript(runScript.get<std::string>("script"));
        if (!loaded) {
            return EXIT_FAILURE;
        }
        script = std::move(*loaded);
    }
    // Without usable data, the game starts the setup wizard. A script starts the wizard only when
    // scripted key presses, text input, or mouse movements can operate its controls.
    const auto systemData = (parser.is_used("--data-dir") || paths->portableData) ?
                                std::vector<std::filesystem::path>{} :
                                coastline::SystemDataDirectories(executable);
    const auto foundData = coastline::FindAssetData(paths->data, systemData);
    auto assetData = foundData.value_or(paths->data);
    const bool showWizard = !foundData;
    std::string reimportReason;
    if (showWizard) {
        auto directories = systemData;
        directories.insert(directories.begin(), paths->data);
        for (const auto &directory : directories) {
            std::error_code error;
            if (std::filesystem::exists(coastline::DefaultAssets(directory), error)) {
                reimportReason = "Your imported game assets are incomplete or incompatible with "
                                 "this version of Coastline. Import your game files again to "
                                 "continue.";
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Re-import required.");
                break;
            }
        }
    }
#if COASTLINE_SETUP_WIZARD
    if (showWizard && parser.is_subcommand_used(runScript) && script.script.keys.empty() &&
        script.script.typing.empty() && script.script.mouseMoves.empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Missing game data. Use coastline import --cd DISC "
                     "--rom ROM.");
        return EXIT_FAILURE;
    }
#else
    if (showWizard) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Usable installed assets unavailable. Install the Coastline data package "
                     "or select imported data with --data-dir.");
        return EXIT_FAILURE;
    }
#endif
    // A run that saves its frames draws them at Width by Height and steps the splash screens by a
    // 60th of a second a frame, for the same frames on every machine.
    const bool savesFrames =
        !script.script.screenshots.empty() || script.script.screenshotEvery > 0;
    std::filesystem::path scriptFrames = paths->data / "screenshots";
    if (savesFrames) {
        std::error_code error;
        std::filesystem::create_directories(scriptFrames, error);
    }
    auto config = coastline::GameConfig::Load(configPath);
    // Every setting is written to the file on the first run: the keys of the PC release first,
    // then the keys of this game.
    for (const auto &[section, key, value] : {std::tuple{"Video", "Driver", "SDL"},
                                              {"Video", "Width", "1280"},
                                              {"Video", "Height", "720"},
                                              {"Video", "Fullscreen", "0"},
                                              {"Video", "FullscreenMode", "borderless"},
                                              {"Video", "RefreshRate", "0"},
                                              {"Video", "VSync", "1"},
                                              {"Video", "FrameRateLimit", "0"},
                                              {"Video", "InternalWidth", "0"},
                                              {"Video", "InternalHeight", "0"},
                                              {"Video", "RenderScale", "1"},
                                              {"Video", "AspectRatio", "auto"},
                                              {"Video", "GraphicsPreset", "custom"},
                                              {"Video", "ScaleFilter", "linear"},
                                              {"Video", "ScreenShader", "none"},
                                              {"Video", "FieldOfView", "1"},
                                              {"Video", "DrawDistance", "1"},
                                              {"Video", "TextureFilter", "modern"},
                                              {"Video", "AnisotropicFiltering", "0"},
                                              {"Video", "Antialias", "1"},
                                              {"Video", "AlphaToCoverage", "1"},
                                              {"Video", "AntialiasPost", "none"},
                                              {"Video", "GpuDriver", ""},
                                              {"Video", "GpuPreference", "auto"},
                                              {"Video", "UnlockedFramerate", "0"},
                                              {"Video", "WideMenus", "0"},
                                              {"Game", "PlayMusic", "1"},
                                              {"Game", "PlaySFX", "1"},
                                              {"Game", "WaitForStart", "0"},
                                              {"Game", "PauseOnFocusLoss", "1"},
                                              {"Game", "GameMode", "1"},
                                              {"Game", "Music", "N64"},
                                              {"Game", "WindowReflections", "auto"},
                                              {"Game", "AssetPack", "default"},
                                              {"Game", "Language", "en-us"},
                                              {"Game", "TitleScreen", "regional"},
                                              {"Controls", "Preset", "Default"},
                                              {"Controls", "Player1", "auto"},
                                              {"Controls", "Player2", "auto"}}) {
        if (!config.Get(section, key)) {
            config.Set(section, key, value);
        }
    }
    int status = EXIT_FAILURE;
    // The frame the game starts at, after the frames of the setup wizard.
    int firstFrame = 1;
    bool setupQuit = false;
    bool setupFailed = false;
    // The setup wizard imports the data in the window of the game, and the game then reads it.
    auto setUp = [&]([[maybe_unused]] coastline::GpuWindow &window) {
#if COASTLINE_SETUP_WIZARD
        if (showWizard) {
            coastline::SetupRun run = coastline::RunSetupWizard(
                window,
                *paths,
                config,
                script.script,
                script.keys,
                scriptFrames,
                savesFrames,
                reimportReason,
                disablePortable || parser.is_used("--config") || parser.is_used("--data-dir") ?
                    std::filesystem::path{} :
                    portableRoot);
            if (run.portable) {
                configPath = paths->config / "BossRally.ini";
                assetData = paths->data;
                scriptFrames = paths->data / "screenshots";
                gameLog.reset();
                gameLog = std::make_unique<coastline::GameLog>(paths->state / "game.log");
            }
            firstFrame = run.frames + 1;
            setupQuit = !run.finished;
            setupFailed = run.failed;
        }
#endif
        return !setupQuit;
    };
    {
#ifdef SDL_PLATFORM_MACOS
        SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES, "0");
        SDL_SetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_MENU_VISIBILITY, "0");
#endif
        SdlSession sdl(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
        if (sdl.ready) {
            SDL_HideCursor();
        }
        coastline::GameModeSession gameMode(sdl.ready &&
                                            config.GetBoolean("Game", "GameMode", true));
        if (sdl.ready && !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO, "Cannot start audio: %s", SDL_GetError());
        }
        if (!sdl.ready) {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "Cannot start SDL: %s", SDL_GetError());
        } else if (auto window = coastline::GpuWindow::Create(
                       "Coastline", DisplayOptionsFrom(config, parser, savesFrames, showWizard));
                   window && setUp(*window)) {
            bool reinitialize = false;
            do {
                reinitialize = false;
                const coastline::DisplayOptions displayOptions =
                    DisplayOptionsFrom(config, parser, savesFrames);
                if (window->GpuPreference() != displayOptions.gpuPreference) {
                    window.reset();
                    window = coastline::GpuWindow::Create("Coastline", displayOptions);
                    if (!window) {
                        status = EXIT_FAILURE;
                        break;
                    }
                }
                if (!window->ApplyRendererOptions(displayOptions)) {
                    status = EXIT_FAILURE;
                    break;
                }
                // The assets of the pack of the command line or of the configuration, else of the
                // default pack.
                std::string pack =
                    parser.present("--asset-pack")
                        .value_or(config.Get("Game", "AssetPack")
                                      .value_or(std::string(coastline::DEFAULT_PACK)));
                if (!parser.is_used("--asset-pack")) {
                    if (parser.present("--preset") == "n64") {
                        pack = "default";
                    } else if (parser.present("--preset") == "br") {
                        pack = "boss";
                    }
                }
                const auto baseRegion = coastline::CartridgeRegion(
                    coastline::ReleaseDirectory(coastline::DefaultAssets(assetData)));
                if (!config.Get("Game", "Region")) {
                    config.Set("Game", "Region", baseRegion);
                }
                auto region = parser.present("--region")
                                  .value_or(config.Get("Game", "Region").value_or(baseRegion));
                if (region != baseRegion &&
                    ((region != "usa" && region != "eur" && region != "jpn") ||
                     coastline::CartridgeRegion(coastline::ReleaseDirectory(
                         coastline::PackDirectory(assetData, region, paths->data))) != region)) {
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                "Region %s is unavailable; using %s.",
                                RegionName(region),
                                RegionName(baseRegion));
                    region = baseRegion;
                }
                const std::string regionPack = region == baseRegion ? "" : region;
                auto packAssets =
                    coastline::PackAssets(assetData, paths->cache, pack, paths->data, regionPack);
                if (!packAssets) {
                    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                "Cannot read asset pack %s; using default.",
                                pack.c_str());
                    pack = coastline::DEFAULT_PACK;
                    packAssets =
                        coastline::PackAssets(assetData, paths->cache, pack, {}, regionPack);
                    if (!packAssets) {
                        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                     "Cannot prepare imported assets. Import them again with "
                                     "coastline import --cd DISC --rom ROM.");
                        return EXIT_FAILURE;
                    }
                }
                const std::filesystem::path assets = *packAssets;
                SDL_Log("Using asset pack %s (%s).", pack.c_str(), assets.string().c_str());
                SDL_Log(
                    "Using region %s with base %s.", RegionName(region), RegionName(baseRegion));
                coastline::GameText gameText(
                    assets,
                    parser.present("--language")
                        .value_or(config.Get("Game", "Language")
                                      .value_or(std::string(coastline::DEFAULT_LANGUAGE))));
                const std::filesystem::path release = coastline::ReleaseDirectory(assets);
                SDL_Log("Using language %s (%s).",
                        coastline::SelectedLanguage().c_str(),
                        release.lexically_relative(assets).string().c_str());
                const nlohmann::json packSettings =
                    coastline::PackSettings(assetData, pack, paths->data);
                // XmMusicGain of the configuration replaces the gain of the pack when the user
                // writes it.
                const float musicGain = config.GetFloat(
                    "Game",
                    "XmMusicGain",
                    packSettings.value("xmMusicGain", coastline::DEFAULT_XM_MUSIC_GAIN));
                SetWindowIcon(window->Window(), assets);
                coastline::N64MixerContext mixer;
                const auto audioPreset = parser.present("--preset");
                const bool n64SoundEffects =
                    audioPreset == "n64" ||
                    (audioPreset != "br" && config.Get("Game", "SoundEffects") == "N64");
                coastline::SoundEffects soundEffects(assets,
                                                     n64SoundEffects ?
                                                         coastline::SoundEffects::Source::N64 :
                                                         coastline::SoundEffects::Source::Pc,
                                                     config.GetBoolean("Game", "PlaySFX", true));
                coastline::EngineRuntime engineRuntime;
                coastline::InputBindings input(config);
                coastline::EngineModels engineModels;
                coastline::EngineTextures engineTextures;
                coastline::AttractDemo attractDemo;
                coastline::N64Audio audio;
                coastline::RaceMusicContext raceMusic;
                auto useCdMusic = [&]() {
                    return parser.present("--preset") == "br" ||
                           (parser.present("--preset") != "n64" &&
                            config.Get("Game", "Music") == "CD");
                };
                coastline::FrontEndMusic menuMusic(audio,
                                                   assets / "music",
                                                   useCdMusic() ?
                                                       coastline::RaceMusicSource::CdAudio :
                                                       coastline::RaceMusicSource::Modules);
                const bool musicEnabled = config.GetBoolean("Game", "PlayMusic", true);
                g_dwPlayMusicMode = musicEnabled ? 2 : 0;
                const bool audioOpen =
                    (musicEnabled || config.GetBoolean("Game", "PlaySFX", true)) &&
                    audio.OpenDevice();
                const bool playMusic = musicEnabled && audioOpen;
                const bool pauseOnFocusLoss = config.GetBoolean("Game", "PauseOnFocusLoss", true) &&
                                              !parser.is_subcommand_used(runScript);
                audio.SetMusicGain(musicGain);
                coastline::ShareRaceMusicPlayer(audioOpen ? &audio : nullptr);
                auto playMenuMusic = [&]() {
                    if (playMusic) {
                        menuMusic.SetVolume(g_bBgmVolume);
                        menuMusic.Play();
                    }
                };
                const coastline::Script &commands = script.script;
                MouseState mouse;
                std::unique_ptr<coastline::SceneRenderer> sceneRenderer;
                std::unique_ptr<coastline::SceneRenderer> outputRenderer;
                std::unique_ptr<coastline::Intro> intro;
                std::unique_ptr<coastline::Menus> mainMenu;
                std::unique_ptr<coastline::MenuText> frameRateText;
                // The statistics of the third state of F, sampled only while they show.
                std::unique_ptr<coastline::PerformanceStats> performanceStats;
                // The screen of the front end that shows.
                enum class FrontEnd { Intro, AttractDemo, MainMenu, Race, Credits };
                FrontEnd frontEnd = FrontEnd::Intro;
                coastline::DiscordPresence discord(COASTLINE_DISCORD_APPLICATION_ID);
                auto presenceScreen = frontEnd;
                discord.Update("Watching the intro");
                // The races run a 30th of a second a step, from the assets of the pack.
                constexpr float RACE_STEP = 1.0f / 30.0f;
                std::unique_ptr<coastline::RaceView> raceView;
                coastline::RaceSession raceSession;
                bool raceDataOpen = false;
                // The race engine reads its files from the assets of the pack.
                auto openRaceData = [&]() {
                    if (!raceDataOpen) {
                        std::error_code error;
                        std::filesystem::current_path(assets, error);
                        coastline::OpenRaceData(config.GetBoolean("Game", "PlaySFX", true));
                        raceDataOpen = true;
                    }
                };
                float raceTime = 0.0f;
                bool leaveDemo = false;
                // The first step of the race of the credits loads the race and with it the track
                // and the car of the recording. The ending credits scroll, and go on to the next
                // season.
                bool endingCredits = false;
                auto startCredits = [&](bool ending) {
                    endingCredits = ending;
                    openRaceData();
                    attractDemo.StartCredits(ending);
                    attractDemo.Credits().Start(release);
                    attractDemo.Step(false);
                    const coastline::RaceCar car{attractDemo.Car(),
                                                 attractDemo.Color(),
                                                 mainMenu ? mainMenu->CarImages(attractDemo.Car()) :
                                                            std::nullopt};
                    raceView = coastline::RaceView::Load(
                        *sceneRenderer, assets, attractDemo.Track(), {&car, 1});
                    raceTime = 0.0f;
                    leaveDemo = false;
                    return raceView != nullptr;
                };
                sceneRenderer = coastline::SceneRenderer::Create(*window);
                if (sceneRenderer) {
                    const std::filesystem::path frontEndRelease =
                        coastline::ReleaseDirectory(assets);
                    const auto titleChoice =
                        parser.is_used("--region") ?
                            "regional" :
                            config.Get("Game", "TitleScreen").value_or("regional");
                    const auto currentTitleRegion = coastline::CartridgeRegion(frontEndRelease);
                    const auto titleRegion = coastline::TitleScreenRegion(
                        titleChoice,
                        currentTitleRegion,
                        coastline::AvailableCartridgeRegions(std::array{assetData, paths->data}));
                    if (titleRegion && *titleRegion != currentTitleRegion) {
                        auto titleAssets =
                            coastline::PackAssets(assetData,
                                                  paths->cache,
                                                  pack,
                                                  paths->data,
                                                  *titleRegion == baseRegion ? "" : *titleRegion);
                        if (titleAssets) {
                            intro =
                                coastline::Intro::Load(*sceneRenderer,
                                                       coastline::ReleaseDirectory(*titleAssets),
                                                       *titleRegion);
                        }
                    } else if (titleRegion) {
                        intro =
                            coastline::Intro::Load(*sceneRenderer, frontEndRelease, *titleRegion);
                    }
                    if (!intro) {
                        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                                    "Title screen %s is unavailable; using region %s.",
                                    titleChoice.c_str(),
                                    RegionName(currentTitleRegion));
                        intro = coastline::Intro::Load(*sceneRenderer, frontEndRelease);
                    } else {
                        SDL_Log("Using title screen from region %s.", RegionName(*titleRegion));
                    }
                    mainMenu = coastline::Menus::Load(*sceneRenderer,
                                                      frontEndRelease,
                                                      paths->data,
                                                      paths->cache,
                                                      config,
                                                      configPath,
                                                      std::array{assetData, paths->data},
                                                      pack,
                                                      parser.present("--region").value_or(""));
                    if (mainMenu) {
                        auto overrides = CommandLineOverrides(parser);
                        if (overrides.Get("Game", "AssetPack")) {
                            overrides.Set("Game", "AssetPack", pack);
                        }
                        if (overrides.Get("Game", "Region")) {
                            overrides.Set("Game", "Region", region);
                            overrides.Set("Game",
                                          "TitleScreen",
                                          region == "jpn" ? "japanese" : "international");
                        }
                        mainMenu->SetOptionOverrides(std::move(overrides));
                        mainMenu->SetMenuMusic(playMenuMusic);
                    }
                    frameRateText = coastline::MenuText::Load(
                        *sceneRenderer,
                        coastline::FontDirectory(assets, coastline::SelectedLanguage()));
                }
                playMenuMusic();
                // The vertical sync V returns to, fifo when the settings choose it.
                const bool fifo = displayOptions.fifo;
                Uint64 lastTicks = SDL_GetTicksNS();
                // FrameRateLimit spaces the frames, at no limit when 0. --preset worst draws 30
                // frames a second, the steps of the race, for a slow computer.
                const auto graphicsPreset = parser.present("--preset");
                const long frameLimit =
                    graphicsPreset == "worst" || graphicsPreset == "n64"    ? 30 :
                    graphicsPreset == "br" || graphicsPreset == "low-power" ? 60 :
                    graphicsPreset == "ultra"                               ? 0 :
                                                config.GetInteger("Video", "FrameRateLimit", 0);
                const Uint64 framePeriod = frameLimit > 0 ? SDL_NS_PER_SECOND / frameLimit : 0;
                Uint64 nextFrame = lastTicks;
                Uint64 rateStart = lastTicks;
                int rateFrames = 0;
                DisplayKeys displayKeys;
                FrameRateOverlay frameRateOverlay;
                std::uint16_t lastButtons = 0;
                std::uint16_t lastSecondButtons = 0;
                bool running = true;
                bool printScreenWasDown = false;
                bool escapeWasDown = false;
                bool freeCameraToggleWasDown = false;
                bool undoPaintWasDown = false;
                float exitEscapeSeconds = 0.0f;
                coastline::MenusEnd settingsTransition = coastline::MenusEnd::Running;
                Uint64 settingsShownAt = 0;
                int settingsFrames = 0;
                for (int frame = firstFrame; running; ++frame) {
                    const auto frameStarted = SDL_GetTicksNS();
                    const auto waitStarted = window->PresentationWaitNanoseconds();
                    // The mixers take the volumes the screens and the race wrote in the last frame.
                    coastline::PublishN64Volumes();
                    const bool raceScreen = frontEnd == FrontEnd::Race ||
                                            frontEnd == FrontEnd::AttractDemo ||
                                            frontEnd == FrontEnd::Credits;
                    const bool sfxEnabled = config.GetBoolean("Game", "PlaySFX", true) &&
                                            g_bSfxMasterVolume != 0 &&
                                            (!raceScreen || g_GlobalRallyConfig.dwSfxVolume != 0);
                    audio.SetProcessingEnabled(musicEnabled, sfxEnabled);
                    EarSetSdlMixerEnabled(sfxEnabled);
                    PushScriptedTyping(commands.typing, frame, window->Window());
                    // The text typed on a keyboard while a screen takes it, with Backspace and
                    // Enter as control characters.
                    std::string typed;
                    bool printScreen = false;
                    bool undoPaint = false;
                    bool focusLost = false;
                    std::array<float, 2> cameraMouse{};
                    SDL_Event event;
                    if (frontEnd != presenceScreen) {
                        switch (frontEnd) {
                        case FrontEnd::Intro:
                            discord.Update("Watching the intro");
                            break;
                        case FrontEnd::AttractDemo:
                            discord.Update("Watching an attract demo");
                            break;
                        case FrontEnd::MainMenu:
                            discord.Update("In the menus");
                            break;
                        case FrontEnd::Race:
                            discord.Update("Racing");
                            break;
                        case FrontEnd::Credits:
                            discord.Update("Watching the credits");
                            break;
                        }
                        presenceScreen = frontEnd;
                    }
                    while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_EVENT_MOUSE_MOTION) {
                            cameraMouse[0] += event.motion.xrel;
                            cameraMouse[1] += event.motion.yrel;
                        }
                        if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                            event.key.key == SDLK_Z && (event.key.mod & SDL_KMOD_CTRL) != 0) {
                            undoPaint = true;
                        }
                        if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
                            focusLost = true;
                        }
                        // F9 takes the frame as well, as a desktop such as Plasma keeps Print
                        // Screen.
                        if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                            (event.key.scancode == SDL_SCANCODE_PRINTSCREEN ||
                             event.key.scancode == SDL_SCANCODE_F9)) {
                            printScreen = true;
                        }
                        if (event.type == SDL_EVENT_QUIT) {
                            running = false;
                        } else if (event.type == SDL_EVENT_TEXT_INPUT) {
                            typed += event.text.text;
                        } else if (event.type == SDL_EVENT_KEY_DOWN &&
                                   SDL_TextInputActive(window->Window())) {
                            if (event.key.key == SDLK_BACKSPACE) {
                                typed += '\b';
                            } else if (event.key.key == SDLK_RETURN ||
                                       event.key.key == SDLK_KP_ENTER) {
                                typed += '\r';
                            }
                        }
                        input.HandleEvent(event);
                    }
                    // A race pauses when the window loses the focus, as the pause binding pauses
                    // it, but not an instant replay or a run of a script.
                    if (focusLost && pauseOnFocusLoss && frontEnd == FrontEnd::Race &&
                        !g_dwRacePauseState && !gInstantReplayActive) {
                        g_bNetGamePaused = 1;
                    }
                    // The shortcuts of the window are read before the controllers, which then do
                    // not see the Enter of Alt and Enter. A race, the attract demo, and the credits
                    // read F themselves, and F and V type while a screen takes text.
                    bool raceShows = frontEnd == FrontEnd::Race ||
                                     frontEnd == FrontEnd::AttractDemo ||
                                     frontEnd == FrontEnd::Credits;
                    bool typing = SDL_TextInputActive(window->Window());
                    switch (displayKeys.Read(!raceShows && !typing, !typing)) {
                    case DisplayKey::Unlocked:
                        window->SetFramerateUnlocked(!window->FramerateUnlocked());
                        config.Set(
                            "Video", "UnlockedFramerate", window->FramerateUnlocked() ? "1" : "0");
                        config.Set("Video", "GraphicsPreset", "custom");
                        nextFrame = SDL_GetTicksNS();
                        break;
                    case DisplayKey::Fullscreen: {
                        bool fullscreen = !window->Fullscreen();
                        if (window->SetFullscreen(fullscreen)) {
                            config.Set("Video", "Fullscreen", fullscreen ? "1" : "0");
                        }
                        break;
                    }
                    case DisplayKey::VSync: {
                        // V goes from immediate to vertical sync, and from vertical sync or the
                        // mailbox to immediate.
                        bool vsync = window->PresentMode() == SDL_GPU_PRESENTMODE_IMMEDIATE;
                        window->SetPresentMode(vsync ? SDL_GPU_PRESENTMODE_VSYNC :
                                                       SDL_GPU_PRESENTMODE_IMMEDIATE);
                        config.Set("Video",
                                   "VSync",
                                   !vsync ? "immediate" :
                                   fifo   ? "fifo" :
                                            "vsync");
                        config.Set("Video", "GraphicsPreset", "custom");
                        break;
                    }
                    case DisplayKey::FrameRate:
                        g_bShowFpsCounter = (g_bShowFpsCounter + 1) % coastline::FRAME_RATE_STATES;
                        break;
                    case DisplayKey::None:
                        break;
                    }
                    const bool racing = frontEnd == FrontEnd::Race;
                    auto ports = input.Poll(racing ? g_dwNumLocalPlayers :
                                            mainMenu && frontEnd == FrontEnd::MainMenu ?
                                                     mainMenu->ControlledPlayers() :
                                                     1,
                                            racing);
                    std::uint16_t scripted = 0;
                    for (const coastline::ScriptPress &press : commands.presses) {
                        if (press.port != 0) {
                            ports[press.port].connected = true;
                        }
                        if (frame >= press.frame && frame - press.frame < press.frames) {
                            ports[press.port].buttons |= press.button;
                            if (press.port == 0) {
                                scripted |= press.button;
                            }
                        }
                    }
                    coastline::SetSecondControllerState(ports[1]);
                    // The scripted moves of the mouse take the place of the real mouse.
                    if (commands.mouseMoves.empty()) {
                        float x = 0.0f;
                        float y = 0.0f;
                        mouse.buttons = SDL_GetMouseState(&x, &y);
                        mouse.at = MouseOnScreen(*window, x, y);
                    } else {
                        mouse.buttons = 0;
                        for (const coastline::ScriptMouse &move : commands.mouseMoves) {
                            if (frame >= move.frame && frame - move.frame < move.frames) {
                                mouse.at = {move.x * 2, move.y * 2};
                                mouse.buttons = move.button == coastline::ScriptMouseButton::Left ?
                                                    SDL_BUTTON_LMASK :
                                                move.button == coastline::ScriptMouseButton::Right ?
                                                    SDL_BUTTON_RMASK :
                                                    0;
                            }
                        }
                    }
                    // The race reads the keyboard through the bindings of the PC release, and the
                    // presses push down the keys of its default bindings.
                    std::vector<int> scriptedKeys;
                    for (std::size_t i = 0; i < commands.keys.size(); ++i) {
                        const coastline::ScriptKey &key = commands.keys[i];
                        if (frame >= key.frame && frame - key.frame < key.frames) {
                            scriptedKeys.push_back(script.keys[i]);
                        }
                    }
                    if (frontEnd == FrontEnd::Race) {
                        for (const auto &[button, key] : RACE_KEYS) {
                            if (scripted & button) {
                                scriptedKeys.push_back(key);
                            }
                        }
                    }
                    coastline::SetScriptedKeys(std::move(scriptedKeys));
                    const bool undoPaintDown = coastline::IsKeyDown(SDL_SCANCODE_Z) &&
                                               (coastline::IsKeyDown(SDL_SCANCODE_LCTRL) ||
                                                coastline::IsKeyDown(SDL_SCANCODE_RCTRL));
                    undoPaint |= undoPaintDown && !undoPaintWasDown;
                    undoPaintWasDown = undoPaintDown;
                    if (undoPaint && frontEnd != FrontEnd::Race) {
                        coastline::IgnoreKeyUntilReleased(SDL_SCANCODE_Z);
                        ports[0].buttons &= ~coastline::BUTTON_Z;
                    }
                    bool printScreenDown = coastline::IsKeyDown(SDL_SCANCODE_PRINTSCREEN) ||
                                           coastline::IsKeyDown(SDL_SCANCODE_F9);
                    printScreen = printScreen || (printScreenDown && !printScreenWasDown);
                    printScreenWasDown = printScreenDown;
                    // The frames of the script are named by their number.
                    std::optional<std::filesystem::path> framePath;
                    if (std::ranges::contains(commands.screenshots, frame) ||
                        (commands.screenshotEvery > 0 && frame % commands.screenshotEvery == 0)) {
                        char name[32];
                        std::snprintf(name, sizeof(name), "frame-%06d.png", frame);
                        framePath = scriptFrames / name;
                    }
                    // Print Screen saves the frame to the screenshots of the data directory, named
                    // by the time it was taken.
                    if (printScreen && !framePath) {
                        framePath = ScreenshotPath(paths->data / "screenshots");
                    }
                    window->UpdateFrameSize();
                    if (intro && settingsTransition != coastline::MenusEnd::Running) {
                        const bool reboot = settingsTransition == coastline::MenusEnd::Reinitialize;
                        sceneRenderer->Begin({0.0f, 0.0f, 0.0f, 1.0f});
                        if (frameRateText) {
                            DrawOverlayLine(*sceneRenderer,
                                            *frameRateText,
                                            reboot ? "Reinitializing game..." :
                                                     "Applying settings...",
                                            220,
                                            320,
                                            coastline::TextStyle::Align::Center,
                                            true);
                        }
                        sceneRenderer->End(framePath ? &*framePath : nullptr);
                        if (settingsFrames++ == 0) {
                            settingsShownAt = SDL_GetTicksNS();
                            SDL_Log(reboot ? "Reinitializing game after Options." :
                                             "Applying settings after Options.");
                            if (!reboot) {
                                if (!window->ApplyLiveOptions(
                                        DisplayOptionsFrom(config, parser, savesFrames))) {
                                    settingsTransition = coastline::MenusEnd::Reinitialize;
                                    settingsFrames = 0;
                                } else if (playMusic) {
                                    menuMusic.SetSource(useCdMusic() ?
                                                            coastline::RaceMusicSource::CdAudio :
                                                            coastline::RaceMusicSource::Modules);
                                }
                            }
                        } else if (savesFrames ? settingsFrames >= 15 :
                                                 SDL_GetTicksNS() - settingsShownAt >=
                                                     SDL_NS_PER_SECOND / 4) {
                            if (reboot) {
                                firstFrame = frame + 1;
                                reinitialize = true;
                                break;
                            }
                            settingsTransition = coastline::MenusEnd::Running;
                            lastTicks = SDL_GetTicksNS();
                        }
                    } else if (intro) {
                        Uint64 now = SDL_GetTicksNS();
                        float seconds =
                            savesFrames ?
                                1.0f / 60.0f :
                                std::min(static_cast<float>(now - lastTicks) / 1e9f, 0.1f);
                        lastTicks = now;
                        const auto devices = coastline::AssignedDevices(0, 1);
                        bool cameraToggle = coastline::IsKeyDown(SDL_SCANCODE_F8);
                        for (auto *gamepad : devices.gamepads) {
                            cameraToggle |= SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_BACK);
                        }
                        if (frontEnd == FrontEnd::Race && raceView) {
                            if (cameraToggle && !freeCameraToggleWasDown) {
                                raceView->ToggleFreeCamera();
                                SDL_SetWindowRelativeMouseMode(window->Window(),
                                                               raceView->FreeCameraEnabled());
                            }
                            const bool captured = raceView->FreeCameraEnabled();
                            coastline::SetRaceControlsCaptured(captured);
                            if (captured) {
                                coastline::SetSecondControllerState({});
                                auto keyAxis = [](SDL_Scancode positive, SDL_Scancode negative) {
                                    return static_cast<float>(coastline::IsKeyDown(positive)) -
                                           static_cast<float>(coastline::IsKeyDown(negative));
                                };
                                std::array<float, 3> movement{
                                    keyAxis(SDL_SCANCODE_W, SDL_SCANCODE_S),
                                    keyAxis(SDL_SCANCODE_D, SDL_SCANCODE_A),
                                    keyAxis(SDL_SCANCODE_E, SDL_SCANCODE_Q)};
                                std::array<float, 2> rotation{-cameraMouse[0] * 0.003f,
                                                              -cameraMouse[1] * 0.003f};
                                bool fast = coastline::IsKeyDown(SDL_SCANCODE_LSHIFT) ||
                                            coastline::IsKeyDown(SDL_SCANCODE_RSHIFT);
                                for (auto *gamepad : devices.gamepads) {
                                    auto axis = [gamepad](SDL_GamepadAxis which) {
                                        float value =
                                            static_cast<float>(SDL_GetGamepadAxis(gamepad, which)) /
                                            32768.0f;
                                        return value > -0.15f && value < 0.15f ? 0.0f : value;
                                    };
                                    movement[0] -= axis(SDL_GAMEPAD_AXIS_LEFTY);
                                    movement[1] += axis(SDL_GAMEPAD_AXIS_LEFTX);
                                    movement[2] +=
                                        static_cast<float>(SDL_GetGamepadButton(
                                            gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER)) -
                                        static_cast<float>(SDL_GetGamepadButton(
                                            gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
                                    rotation[0] -= axis(SDL_GAMEPAD_AXIS_RIGHTX) * seconds * 2.0f;
                                    rotation[1] -= axis(SDL_GAMEPAD_AXIS_RIGHTY) * seconds * 2.0f;
                                    fast |= SDL_GetGamepadButton(gamepad,
                                                                 SDL_GAMEPAD_BUTTON_LEFT_STICK);
                                }
                                for (auto &value : movement) {
                                    value =
                                        std::clamp(value, -1.0f, 1.0f) * (fast ? 800.0f : 200.0f);
                                }
                                raceView->MoveFreeCamera(seconds, movement, rotation);
                            }
                        } else {
                            coastline::SetRaceControlsCaptured(false);
                            SDL_SetWindowRelativeMouseMode(window->Window(), false);
                        }
                        freeCameraToggleWasDown = cameraToggle;
                        const bool escape = coastline::IsKeyDown(SDL_SCANCODE_ESCAPE);
                        const bool canExit =
                            frontEnd == FrontEnd::Intro || frontEnd == FrontEnd::AttractDemo ||
                            (frontEnd == FrontEnd::MainMenu && mainMenu && mainMenu->IsMainMenu());
                        exitEscapeSeconds =
                            canExit ? std::max(0.0f, exitEscapeSeconds - seconds) : 0.0f;
                        if (canExit && escape && !escapeWasDown) {
                            if (exitEscapeSeconds > 0.0f) {
                                break;
                            }
                            exitEscapeSeconds = 0.5f;
                        }
                        escapeWasDown = escape;
                        // A press of A, Start, or Z skips or chooses, B goes back, and every C
                        // button together shows the date of the build on the splash screens.
                        constexpr std::uint16_t CHOOSE =
                            coastline::BUTTON_A | coastline::BUTTON_START | coastline::BUTTON_Z;
                        constexpr std::uint16_t C_BUTTONS =
                            coastline::BUTTON_C_UP | coastline::BUTTON_C_DOWN |
                            coastline::BUTTON_C_LEFT | coastline::BUTTON_C_RIGHT;
                        // The stick of a controller pushes the directions of its pad.
                        constexpr int STICK_PUSH = 40;
                        auto heldOf = [](const coastline::ControllerState &port) {
                            std::uint16_t buttons = port.buttons;
                            if (port.stickX < -STICK_PUSH) {
                                buttons |= coastline::BUTTON_D_LEFT;
                            }
                            if (port.stickX > STICK_PUSH) {
                                buttons |= coastline::BUTTON_D_RIGHT;
                            }
                            if (port.stickY > STICK_PUSH) {
                                buttons |= coastline::BUTTON_D_UP;
                            }
                            if (port.stickY < -STICK_PUSH) {
                                buttons |= coastline::BUTTON_D_DOWN;
                            }
                            return buttons;
                        };
                        std::uint16_t held = heldOf(ports[0]);
                        std::uint16_t pressed = held & ~lastButtons;
                        lastButtons = held;
                        std::uint16_t secondHeld = heldOf(ports[1]);
                        std::uint16_t secondPressed = secondHeld & ~lastSecondButtons;
                        lastSecondButtons = secondHeld;
                        if (frontEnd == FrontEnd::Intro) {
                            // Shift and I on the keyboard hold every C button at once.
                            const bool *keys = SDL_GetKeyboardState(nullptr);
                            bool shiftI =
                                keys[SDL_SCANCODE_I] && (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
                            coastline::IntroButtons buttons{
                                (pressed & CHOOSE) != 0, (held & C_BUTTONS) == C_BUTTONS || shiftI};
                            coastline::IntroEnd end = intro->Update(seconds, buttons);
                            if (end == coastline::IntroEnd::MainMenu && mainMenu) {
                                mainMenu->Restart();
                                frontEnd = FrontEnd::MainMenu;
                            } else if (end != coastline::IntroEnd::Running) {
                                openRaceData();
                                // The first step loads the race and with it the track and the car.
                                attractDemo.Start();
                                attractDemo.Step(false);
                                const coastline::RaceCar demoCar{
                                    attractDemo.Car(),
                                    attractDemo.Color(),
                                    mainMenu ? mainMenu->CarImages(attractDemo.Car()) :
                                               std::nullopt};
                                raceView = coastline::RaceView::Load(
                                    *sceneRenderer, assets, attractDemo.Track(), {&demoCar, 1});
                                raceTime = 0.0f;
                                leaveDemo = false;
                                if (raceView) {
                                    frontEnd = FrontEnd::AttractDemo;
                                } else {
                                    intro->Restart();
                                }
                            }
                        } else if (frontEnd == FrontEnd::AttractDemo) {
                            // A press of A, Start, or Z leaves the demo for the main menu.
                            leaveDemo = leaveDemo || (pressed & CHOOSE) != 0;
                            raceTime += seconds;
                            bool demoRunning = true;
                            while (demoRunning && raceTime >= RACE_STEP) {
                                raceTime -= RACE_STEP;
                                demoRunning = attractDemo.Step(leaveDemo);
                                leaveDemo = false;
                            }
                            if (!demoRunning) {
                                raceView.reset();
                                if (attractDemo.Interrupted() && mainMenu) {
                                    mainMenu->Restart();
                                    frontEnd = FrontEnd::MainMenu;
                                } else {
                                    intro->Restart();
                                    frontEnd = FrontEnd::Intro;
                                }
                            }
                        } else if (frontEnd == FrontEnd::Race) {
                            raceTime += seconds;
                            coastline::RaceEnd end = coastline::RaceEnd::Running;
                            while (end == coastline::RaceEnd::Running && raceTime >= RACE_STEP) {
                                raceTime -= RACE_STEP;
                                end = raceSession.Step();
                            }
                            bool cameraChanged = false;
                            const auto cameras = raceSession.CameraChoices();
                            for (std::size_t player = 0; player < cameras.size(); ++player) {
                                const auto section = "Player" + std::to_string(player + 1);
                                if (cameras[player] >= 0 &&
                                    config.GetInteger(section, "Camera", -1) != cameras[player]) {
                                    config.Set(section, "Camera", std::to_string(cameras[player]));
                                    cameraChanged = true;
                                }
                            }
                            if (cameraChanged && !config.Save(configPath)) {
                                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                                             "Cannot save camera settings.");
                            }
                            if (end != coastline::RaceEnd::Running) {
                                // The module of the track plays on after the race until the main
                                // menu starts the music of the menus. The credits play under the
                                // music of the menus.
                                raceView.reset();
                                frontEnd = FrontEnd::MainMenu;
                                if (end != coastline::RaceEnd::Credits) {
                                    mainMenu->FinishRace(end,
                                                         raceSession.Results(),
                                                         raceSession.SfxVolumeAdjusted());
                                } else {
                                    playMenuMusic();
                                    if (startCredits(true)) {
                                        frontEnd = FrontEnd::Credits;
                                    } else {
                                        mainMenu->FinishRace(end, {});
                                    }
                                }
                            }
                        } else if (frontEnd == FrontEnd::Credits) {
                            // A press of A, Start, or Z ends the credits as it ends the attract
                            // demo.
                            leaveDemo = leaveDemo || (pressed & CHOOSE) != 0;
                            raceTime += seconds;
                            bool creditsRunning = true;
                            while (creditsRunning && raceTime >= RACE_STEP) {
                                raceTime -= RACE_STEP;
                                creditsRunning = attractDemo.Step(leaveDemo);
                                attractDemo.Credits().Step(RACE_STEP);
                                leaveDemo = false;
                            }
                            if (!creditsRunning) {
                                raceView.reset();
                                if (endingCredits) {
                                    mainMenu->FinishRace(coastline::RaceEnd::Credits, {});
                                } else {
                                    mainMenu->FinishCredits();
                                }
                                frontEnd = FrontEnd::MainMenu;
                            }
                        }
                        bool startRace = false;
                        bool startCreditsRace = false;
                        if (frontEnd == FrontEnd::Intro) {
                            sceneRenderer->Begin({0.0f, 0.0f, 0.0f, 1.0f});
                            intro->Draw(*sceneRenderer);
                        } else if (frontEnd == FrontEnd::AttractDemo ||
                                   frontEnd == FrontEnd::Race || frontEnd == FrontEnd::Credits) {
                            sceneRenderer->Begin(raceView->ClearColor());
                            // The frame is raceTime into the step after the last snapshot.
                            raceView->Draw(*sceneRenderer, raceTime / RACE_STEP);
                        } else {
                            sceneRenderer->Begin({0.0f, 0.0f, 0.0f, 1.0f});
                            auto menuButtons = [&](std::uint16_t down,
                                                   std::uint16_t now,
                                                   const coastline::ControllerState &port) {
                                return coastline::MenuButtons{
                                    .left = (down & coastline::BUTTON_D_LEFT) != 0,
                                    .right = (down & coastline::BUTTON_D_RIGHT) != 0,
                                    .up = (down & coastline::BUTTON_D_UP) != 0,
                                    .down = (down & coastline::BUTTON_D_DOWN) != 0,
                                    .choose = (down & CHOOSE) != 0,
                                    .start = (down & coastline::BUTTON_START) != 0,
                                    .back = (down & coastline::BUTTON_B) != 0,
                                    .held = now != 0,
                                    .cDown = (down & coastline::BUTTON_C_DOWN) != 0,
                                    .heldButtons = port.buttons,
                                    .stickY = port.stickY};
                            };
                            coastline::PaintShopInput pointer = PaintShopInputOf(ports[0], mouse);
                            pointer.text = std::move(typed);
                            pointer.undo = undoPaint;
                            coastline::MenusEnd end =
                                mainMenu->Frame(*sceneRenderer,
                                                seconds,
                                                {menuButtons(pressed, held, ports[0]),
                                                 menuButtons(secondPressed, secondHeld, ports[1])},
                                                ports[1].connected,
                                                pointer);
                            // The keyboard of the computer types while a screen takes text.
                            if (mainMenu->TakesText() != SDL_TextInputActive(window->Window())) {
                                if (mainMenu->TakesText()) {
                                    SDL_StartTextInput(window->Window());
                                } else {
                                    SDL_StopTextInput(window->Window());
                                }
                            }
                            if (end == coastline::MenusEnd::Quit) {
                                running = false;
                            } else if (end == coastline::MenusEnd::Splash) {
                                intro->Restart();
                                frontEnd = FrontEnd::Intro;
                            }
                            startRace = end == coastline::MenusEnd::Race;
                            startCreditsRace = end == coastline::MenusEnd::Credits;
                            if (end == coastline::MenusEnd::ApplySettings ||
                                end == coastline::MenusEnd::Reinitialize) {
                                settingsTransition = end;
                                settingsFrames = 0;
                            }
                        }
                        if (g_bShowFpsCounter == 1 && frameRateText &&
                            (frontEnd == FrontEnd::Intro || frontEnd == FrontEnd::MainMenu)) {
                            frameRateOverlay.Draw(*sceneRenderer, *frameRateText);
                        }
                        if (g_bShowFpsCounter != coastline::FRAME_RATE_STATISTICS) {
                            performanceStats.reset();
                        } else if (frameRateText) {
                            if (!performanceStats) {
                                performanceStats = std::make_unique<coastline::PerformanceStats>(
                                    window->DeviceName(), window->NvmlAllowed());
                            }
                        }
                        coastline::GpuWindow::OutputOverlay outputOverlay;
                        if (performanceStats && frameRateText) {
                            if (!outputRenderer) {
                                outputRenderer = coastline::SceneRenderer::Create(*window, true);
                            }
                            if (outputRenderer) {
                                outputOverlay = [&](SDL_GPUCommandBuffer *gpuCommands,
                                                    SDL_GPUTexture *target,
                                                    Uint32 width,
                                                    Uint32 height) {
                                    outputRenderer->Begin({0, 0, 0, 0});
                                    float scale = SDL_GetWindowDisplayScale(window->Window());
                                    if (!std::isfinite(scale) || scale <= 0.0f) {
                                        scale = 1.0f;
                                    }
                                    const std::array<float, 2> size{
                                        static_cast<float>(width) / scale,
                                        static_cast<float>(height) / scale};
                                    // Reserve lap-counter space above the first statistics
                                    // baseline.
                                    int y = std::max(
                                        80, static_cast<int>(std::ceil(size[1] / 12.0f)) + 20);
                                    for (const auto &line :
                                         performanceStats->Lines(window->GpuMemoryBytes())) {
                                        DrawOverlayLine(*outputRenderer,
                                                        *frameRateText,
                                                        line.c_str(),
                                                        y,
                                                        16,
                                                        coastline::TextStyle::Align::Left,
                                                        true,
                                                        size,
                                                        20);
                                        y += 22;
                                    }
                                    outputRenderer->RenderInto(gpuCommands, target, width, height);
                                };
                            }
                        } else {
                            outputRenderer.reset();
                        }
                        menuMusic.SetVolume(g_bBgmVolume);
                        sceneRenderer->End(framePath ? &*framePath : nullptr, outputOverlay);
                        // The credits play under the music of the menus.
                        if (startCreditsRace) {
                            if (startCredits(false)) {
                                frontEnd = FrontEnd::Credits;
                            } else {
                                mainMenu->FinishCredits();
                            }
                        }
                        if (startRace) {
                            // The music of the menus stops when the race starts, and the race plays
                            // its own. The first step loads the race and with it the track and the
                            // cars.
                            openRaceData();
                            SDL_Log("Stopping the menu music.");
                            menuMusic.Stop();
                            coastline::RaceSetup setup = mainMenu->ChosenRace();
                            setup.waitForStart = config.GetBoolean("Game", "WaitForStart", false);
                            setup.autopilot = commands.autopilot;
                            raceSession.Start(setup, assets / "music", useCdMusic());
                            std::vector<coastline::RaceCar> cars = coastline::RaceCars();
                            for (auto &car : cars) {
                                car.images = mainMenu->CarImages(car.car);
                            }
                            for (std::size_t i = 0; i < cars.size() && i < setup.cars.size(); ++i) {
                                if (static_cast<int>(i) < setup.players) {
                                    if (setup.cars[i].images) {
                                        cars[i].images = setup.cars[i].images;
                                    }
                                    cars[i].otherLook = setup.cars[i].otherLook;
                                }
                            }
                            raceView = coastline::RaceView::Load(
                                *sceneRenderer, assets, coastline::RaceTrack(), cars);
                            raceTime = 0.0f;
                            if (raceView) {
                                frontEnd = FrontEnd::Race;
                            } else {
                                raceSession.Abandon();
                                mainMenu->FinishRace(coastline::RaceEnd::Menu, {});
                            }
                        }
                    } else {
                        // Without the splash screens, the clear colour follows the stick of the
                        // first controller.
                        float red = 0.5f + ports[0].stickX / 160.0f;
                        float green = 0.5f + ports[0].stickY / 160.0f;
                        float blue = ports[0].buttons != 0 ? 0.6f : 0.2f;
                        window->PresentClear(SDL_FColor{red, green, blue, 1.0f},
                                             framePath ? &*framePath : nullptr);
                    }
                    frameRateOverlay.Record();
                    const auto frameFinished = SDL_GetTicksNS();
                    if (performanceStats) {
                        const auto wait = window->PresentationWaitNanoseconds() - waitStarted;
                        const auto elapsed = frameFinished - frameStarted;
                        performanceStats->Frame(elapsed > wait ? elapsed - wait : 0);
                    }
                    // SDL_LOGGING=render=debug logs the frames of every 5 seconds.
                    ++rateFrames;
                    Uint64 now = SDL_GetTicksNS();
                    if (now - rateStart >= 5 * SDL_NS_PER_SECOND) {
                        SDL_LogDebug(SDL_LOG_CATEGORY_RENDER,
                                     "%.2f frames a second.",
                                     static_cast<double>(rateFrames) * 1e9 /
                                         static_cast<double>(now - rateStart));
                        rateStart = now;
                        rateFrames = 0;
                    }
                    // The longer of the limit and the refresh the presentation is paced to.
                    Uint64 period = window->FramerateUnlocked() ?
                                        0 :
                                        std::max(framePeriod, window->RefreshPeriod());
                    if (period > 0) {
                        nextFrame += period;
                        if (nextFrame > now) {
                            SDL_DelayPrecise(nextFrame - now);
                        } else {
                            // A late frame does not hurry the frames after it.
                            nextFrame = now;
                        }
                    }
                    if (frame == commands.quitFrame) {
                        running = false;
                    }
                }
                coastline::ShareRaceMusicPlayer(nullptr);
                if (raceDataOpen) {
                    ShutdownDirectSound();
                    FreeStringTable();
                }
                status = EXIT_SUCCESS;
            } while (reinitialize);
        }
    }
    // Quitting the setup wizard is not a failure.
    if (setupQuit && !setupFailed) {
        status = EXIT_SUCCESS;
    }
    if (!config.Save(configPath)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot write %s.", configPath.string().c_str());
    }
    return status;
}
