#include <algorithm>
#include <array>
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
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <argparse/argparse.hpp>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#if !defined(_WIN32)
#include <sys/resource.h>
#endif

#include "assetpack.h"
#include "common/framestatistics.h"
#include "common/game.h"
#include "common/hud.h"
#include "common/input.h"
#include "config.h"
#include "controllerpak.h"
#include "controls.h"
#include "frontend.h"
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
#include "scenerenderer.h"
#include "stringresources.h"

namespace {

constexpr const char *APPLICATION = "coastline";

struct SdlSession {
    bool ready = false;
    explicit SdlSession(SDL_InitFlags flags) : ready(SDL_Init(flags)) {
    }
    ~SdlSession() {
        SDL_Quit();
    }
    SdlSession(const SdlSession &) = delete;
    SdlSession &operator=(const SdlSession &) = delete;
};

coastline::DisplayOptions DisplayOptionsFrom(const coastline::GameConfig &config,
                                             const argparse::ArgumentParser &parser,
                                             bool fixedFrame) {
    coastline::DisplayOptions options;
    options.width = static_cast<int>(config.GetInteger("Video", "Width", options.width));
    options.height = static_cast<int>(config.GetInteger("Video", "Height", options.height));
    options.fullscreen = config.GetBoolean("Video", "Fullscreen", options.fullscreen);
    options.exclusive = config.Get("Video", "FullscreenMode") == "exclusive";
    options.refreshRate = static_cast<int>(config.GetInteger("Video", "RefreshRate", 0));
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
                        "AspectRatio takes auto, a ratio such as 4:3, or a number above 0, and %s "
                        "follows the window.",
                        aspectText->c_str());
        }
    }
    if (config.Get("Video", "ScaleFilter") == "nearest") {
        options.scaleFilter = coastline::ScaleFilter::Nearest;
    }
    auto post = config.Get("Video", "AntialiasPost");
    if (post == "fxaa") {
        options.postAntialias = coastline::PostAntialias::Fxaa;
    } else if (post && *post != "none") {
        SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                    "AntialiasPost takes none or fxaa, and %s is drawn as none.",
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
                    "TextureFilter takes n64, modern, or nearest, and %s filters as modern.",
                    filter->c_str());
    }
    options.antialias =
        static_cast<int>(config.GetInteger("Video", "Antialias", options.antialias));
    // A preset of --gpu-preset replaces the filtering and the anti-aliasing of the settings for
    // this run, and the options of the command line after it still apply. best covers the display
    // at its desktop mode, its largest, and worst draws the smallest frame: 320 by 240 in a window,
    // and the shape of the display in the full screen, at least 320 across and 240 down.
    if (auto preset = parser.present("--gpu-preset")) {
        if (*preset == "best") {
            options.antialias = 8;
            options.alphaToCoverage = true;
            options.anisotropy = 16;
            options.textureFilter = coastline::TextureFilter::Modern;
            options.fullscreen = true;
            options.exclusive = false;
        } else if (*preset == "worst") {
            options.antialias = 1;
            options.alphaToCoverage = false;
            options.postAntialias = coastline::PostAntialias::None;
            options.anisotropy = 0;
            options.textureFilter = coastline::TextureFilter::Nearest;
            options.lowestFrame = true;
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_VIDEO,
                        "--gpu-preset takes best or worst, and %s is ignored.",
                        preset->c_str());
        }
    }
    if (auto antialias = parser.present<int>("--antialias")) {
        options.antialias = *antialias;
    }
    if (auto driver = config.Get("Video", "GpuDriver"); driver && !driver->empty()) {
        options.driver = *driver;
    }
    if (auto width = parser.present<int>("--width")) {
        options.width = *width;
    }
    if (auto height = parser.present<int>("--height")) {
        options.height = *height;
    }
    // best draws at the size the command line gives, scaled to the display it covers.
    if (parser.present("--gpu-preset") == "best" &&
        (parser.is_used("--width") || parser.is_used("--height"))) {
        options.frameWidth = options.width;
        options.frameHeight = options.height;
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
    return options;
}

// The shortcuts of the window on every screen: Alt and Enter or F11 for the full screen, F for
// the frame rate, and V for vertical sync.
enum class DisplayKey { None, Fullscreen, FrameRate, VSync };

// The keys of the shortcuts held the frame before, from the keyboard and the scripted keys.
struct DisplayKeys {
    bool f11 = false;
    bool f = false;
    bool v = false;

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
        bool toggleRate = frameRate && fNow && !f;
        bool toggleVSync = vsync && vNow && !v;
        f11 = f11Now;
        f = fNow;
        v = vNow;
        if (fullscreen) {
            return DisplayKey::Fullscreen;
        }
        if (toggleVSync) {
            return DisplayKey::VSync;
        }
        return toggleRate ? DisplayKey::FrameRate : DisplayKey::None;
    }
};

// RecordFrameTime() of BRD3D: the milliseconds since the frame before, in the samples
// DrawFpsCounterOverlay() averages.
void RecordFrameTime() {
    static unsigned last = GetHighResolutionTime();
    unsigned now = GetHighResolutionTime();
    unsigned delta = now - last;
    last = now;
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

// A line of 15-point text at @p y of the 640 by 480 screen, as DrawFpsCounterOverlay() draws it,
// centred by default, and in the white of @p white when given.
void DrawOverlayLine(coastline::SceneRenderer &renderer,
                     const coastline::MenuText &text,
                     const char *line,
                     int y,
                     int x = 320,
                     coastline::TextStyle::Align align = coastline::TextStyle::Align::Center,
                     bool white = false) {
    const coastline::GpuWindow &window = renderer.Window();
    coastline::TextStyle style;
    style.size = 15;
    style.align = align;
    style.doubled = false;
    if (white) {
        style.colors = std::array<int, 6>{255, 255, 255, 255, 255, 255};
    }
    text.Draw(renderer,
              line,
              x,
              y,
              style,
              coastline::FlatProjection(static_cast<float>(window.FrameWidth()) /
                                        static_cast<float>(std::max(window.FrameHeight(), 1u))));
}

// DrawFpsCounterOverlay() on the screens outside a race: the rate of the samples, updated as they
// wrap, 10 above the bottom of the screen.
void DrawFrameRate(coastline::SceneRenderer &renderer, const coastline::MenuText &text) {
    static float rate = 30.0f;
    if (g_dwFpsFrameCountdown == 0) {
        unsigned total = 0;
        for (int index = 0; index < g_dwFpsSampleFrames; ++index) {
            total += g_adwFpsFrameTimeSamples[index];
        }
        rate = static_cast<float>(g_dwFpsSampleFrames) * 1000.0f /
               static_cast<float>(std::max(total, 1u));
    }
    char line[32];
    std::snprintf(line, sizeof(line), "%6.2f FPS", static_cast<double>(rate));
    DrawOverlayLine(renderer, text, line, 470);
}

// A new file in a directory of screenshots, named by the local time to the millisecond, with the
// directory made when it does not exist.
std::optional<std::filesystem::path> ScreenshotPath(const std::filesystem::path &directory) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "%s cannot be made: %s",
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

// The XM module of the asset pack for a module of the USA cartridge.
std::optional<std::vector<std::uint8_t>> LoadMusicModule(const std::filesystem::path &assets,
                                                         std::uint32_t offset) {
    const auto *module =
        std::ranges::find(coastline::USA_MUSIC_MODULES, offset, &coastline::UsaMusicModule::offset);
    if (module == std::end(coastline::USA_MUSIC_MODULES)) {
        return std::nullopt;
    }
    std::ifstream file(assets / "music" / (std::string(module->name) + ".xm"), std::ios::binary);
    if (!file) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                    "The USA cartridge has not been imported, and the music is silent.");
        return std::nullopt;
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), {});
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
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s cannot be read.", path.string().c_str());
        return std::nullopt;
    }
    const std::string text(std::istreambuf_iterator<char>(file), {});
    auto script = coastline::ParseScript(text);
    if (!script) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s:%d: %s",
                     path.string().c_str(),
                     script.error().line,
                     script.error().message.c_str());
        return std::nullopt;
    }
    LoadedScript loaded{std::move(*script), {}};
    for (const coastline::ScriptKey &key : loaded.script.keys) {
        SDL_Scancode scancode = SDL_GetScancodeFromName(key.name.c_str());
        if (scancode == SDL_SCANCODE_UNKNOWN) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "%s:%d: %s is not the SDL name of a key, such as Return or F9.",
                         path.string().c_str(),
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
                    "The window has no icon, as %s has no images. Import the disc again for it.",
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
                        "%s cannot be read: %s",
                        path.string().c_str(),
                        SDL_GetError());
        } else if (!icon) {
            icon = std::move(image);
        } else {
            SDL_AddSurfaceAlternateImage(icon.get(), image.get());
        }
    }
    if (!icon || !SDL_SetWindowIcon(window, icon.get())) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_VIDEO, "The icon of the window was not set: %s", SDL_GetError());
        return;
    }
    int count = 0;
    SDL_free(SDL_GetSurfaceImages(icon.get(), &count));
    SDL_Log("The icon of the window is %dx%d, with %d sizes, from %s.",
            icon->w,
            icon->h,
            count,
            (pack / "icon").string().c_str());
}

int ImportFromCommandLine(const argparse::ArgumentParser &import,
                          const std::filesystem::path &data) {
    int status = EXIT_SUCCESS;
    // Records that a source was imported with the current version of its conversion.
    auto record = [&](coastline::ImportSource source) {
        if (!coastline::RecordImport(coastline::DefaultAssets(data), source)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "%s cannot be written.",
                         (coastline::DefaultAssets(data) / "import.ini").string().c_str());
            status = EXIT_FAILURE;
        }
    };
    for (const auto &rom : import.get<std::vector<std::string>>("--rom")) {
        if (auto release = coastline::ImportRom(rom, coastline::DefaultAssets(data))) {
            static constexpr const char *names[] = {"USA", "Japanese", "European"};
            static constexpr coastline::ImportSource sources[] = {coastline::ImportSource::Usa,
                                                                  coastline::ImportSource::Jpn,
                                                                  coastline::ImportSource::Eur};
            SDL_Log("Imported the %s cartridge from %s.",
                    names[static_cast<int>(*release)],
                    rom.c_str());
            record(sources[static_cast<int>(*release)]);
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", release.error().c_str());
            status = EXIT_FAILURE;
        }
    }
    if (auto disc = import.present("--cd")) {
        auto written = coastline::ImportDisc(
            *disc, coastline::DefaultAssets(data), [](const std::string &step, double fraction) {
                SDL_Log("[%3d%%] %s", static_cast<int>(fraction * 100), step.c_str());
            });
        if (written) {
            SDL_Log("Imported %zu files from %s.", *written, disc->c_str());
            record(coastline::ImportSource::Disc);
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", written.error().c_str());
            status = EXIT_FAILURE;
        }
    }
    if (auto failure = coastline::CurateAssetPacks(coastline::DefaultAssets(data))) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", failure->c_str());
        status = EXIT_FAILURE;
    }
    coastline::StopGameLog();
    return status;
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
    coastline::StopGameLog();
    return failure ? EXIT_FAILURE : EXIT_SUCCESS;
}

} // namespace

int main(int argc, char *argv[]) {
    RaiseOpenFileLimit();
    argparse::ArgumentParser parser(APPLICATION, COASTLINE_VERSION);
    parser.add_description("Top Gear Rally and Boss Rally in one stand-alone game.");
    parser.add_argument("--data-dir").help("read the game data from this directory");
    parser.add_argument("--asset-pack")
        .help("asset pack for this run, not saved: default, boss for the images of the PC "
              "release in place of the branded images of the cartridges, or the name of a mod "
              "pack under assets of the data directory, read over the default pack");
    parser.add_argument("--language")
        .help("language of the text for this run, not saved: en-us for the text of the USA "
              "cartridge, en-gb for the font and the text of the European cartridge, or ja for "
              "the font and the Japanese text of the Japanese cartridge, the strings of "
              "strings/LANGUAGE.json");
    parser.add_argument("--width").help("width of the window in pixels").scan<'i', int>();
    parser.add_argument("--height").help("height of the window in pixels").scan<'i', int>();
    parser.add_argument("--fullscreen").help("cover the display").flag();
    parser.add_argument("--windowed").help("draw in a window").flag();
    parser.add_argument("--antialias")
        .help("samples of each pixel for anti-aliasing: 1 for none, or 2, 4, or 8")
        .scan<'i', int>();
    parser.add_argument("--no-vsync").help("present frames without waiting").flag();
    parser.add_argument("--aspect-ratio")
        .help("shape of the frame for this run, in a window and in the full screen, with bars "
              "around it: auto for the shape of the window, a ratio such as 4:3, or a number such "
              "as 1.6; replaces AspectRatio of the settings");
    parser.add_argument("--gpu-preset")
        .help(
            "preset of the graphics for this run, not saved: best for 8 samples of anti-aliasing, "
            "alpha to coverage, 16 samples of anisotropic filtering, and the full screen at the "
            "desktop mode of the display, drawn at the size of the display or at --width by "
            "--height "
            "when given, or worst for no anti-aliasing, the nearest texel, 30 frames a second, and "
            "a "
            "frame of 320 by 240 in a window, or the shape of the display at least 320 by 240 in "
            "the "
            "full screen; "
            "--antialias, --fullscreen, and --windowed still apply");
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
    import.add_description("Copy the game data from a Boss Rally disc image and Top Gear Rally "
                           "cartridge images, and exit. At least one of --cd and --rom is "
                           "required.");
    import.add_argument("--cd").metavar("FILE").help(
        "copy the game data and music from a Boss Rally CUE sheet (.cue) or ISO image (.iso)");
    import.add_argument("--rom")
        .metavar("ROM")
        .help("copy a Top Gear Rally cartridge image, checked by SHA-1; repeat for each release")
        .append();
    parser.add_subparser(import);

    argparse::ArgumentParser importPak("import-pak", "", argparse::default_arguments::help);
    importPak.add_description("Copy the notes of any release of Top Gear Rally from a Controller "
                              "Pak image, and exit.");
    importPak.add_argument("file").help("Controller Pak image (.mpk)");
    parser.add_subparser(importPak);

    argparse::ArgumentParser exportPak("export-pak", "", argparse::default_arguments::help);
    exportPak.add_description("Write the saved notes to a Controller Pak image, and exit. The "
                              "first pak of an image of four paks of mupen64plus is replaced, and "
                              "the other paks are retained.");
    exportPak.add_argument("--release")
        .default_value(std::string("usa"))
        .nargs(1)
        .choices("usa", "eur", "jpn")
        .help("release of the cartridge that reads the notes: usa, eur, or jpn");
    exportPak.add_argument("file").help("Controller Pak image (.mpk)");
    parser.add_subparser(exportPak);

    argparse::ArgumentParser runScript("run-script", "", argparse::default_arguments::help);
    runScript.add_description(
        "Start the game driven by the input of a script, and save its frames to screenshots in "
        "the data directory. Each line of the script is a command, with # starting a comment, and "
        "a line that starts with a frame number acts at that frame. The commands are autopilot, "
        "screenshot-every N, FRAME press BUTTON [FRAMES [PORT]], FRAME key KEY [FRAMES], FRAME "
        "type TEXT, FRAME mouse X Y [BUTTON [FRAMES]], FRAME screenshot, and FRAME quit.");
    runScript.add_argument("script").help("script file");
    parser.add_subparser(runScript);

    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        for (const argparse::ArgumentParser *command :
             {&import, &importPak, &exportPak, &runScript}) {
            if (parser.is_subcommand_used(*command)) {
                std::cerr << *command;
                return EXIT_FAILURE;
            }
        }
        std::cerr << parser;
        return EXIT_FAILURE;
    }
    if (parser.is_subcommand_used(import) && !import.is_used("--cd") && !import.is_used("--rom")) {
        std::cerr << "import requires --cd, --rom, or both.\n" << import;
        return EXIT_FAILURE;
    }

    auto paths = coastline::ResolveGamePaths(APPLICATION);
    if (!paths) {
        std::cerr << "The home directory is not known, and the game cannot find its files.\n";
        return EXIT_FAILURE;
    }
    // The game moves to the data directory, and a relative directory would then name another.
    if (auto dataDirectory = parser.present("--data-dir")) {
        std::error_code error;
        paths->data = std::filesystem::absolute(*dataDirectory, error);
    }
    if (!coastline::CreateGamePaths(*paths)) {
        std::cerr << "The directories of the game cannot be created.\n";
        return EXIT_FAILURE;
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
    if (!coastline::StartGameLog(paths->state / "game.log")) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "The log file cannot be opened.");
    }
    SDL_Log("Coastline %s", COASTLINE_VERSION);
    SDL_Log("Configuration: %s", paths->config.string().c_str());
    SDL_Log("Data: %s", paths->data.string().c_str());
    SDL_Log("State: %s", paths->state.string().c_str());
    SDL_Log("Cache: %s", paths->cache.string().c_str());

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
            coastline::StopGameLog();
            return EXIT_FAILURE;
        }
        script = std::move(*loaded);
    }
    // A run that saves its frames draws them at Width by Height and steps the splash screens by a
    // 60th of a second a frame, for the same frames on every machine.
    const bool savesFrames =
        !script.script.screenshots.empty() || script.script.screenshotEvery > 0;
    const std::filesystem::path scriptFrames = paths->data / "screenshots";
    if (savesFrames) {
        std::error_code error;
        std::filesystem::create_directories(scriptFrames, error);
    }
    auto configPath = paths->config / "BossRally.ini";
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
                                              {"Video", "ScaleFilter", "linear"},
                                              {"Video", "FieldOfView", "1"},
                                              {"Video", "TextureFilter", "modern"},
                                              {"Video", "AnisotropicFiltering", "0"},
                                              {"Video", "Antialias", "1"},
                                              {"Video", "AlphaToCoverage", "1"},
                                              {"Video", "AntialiasPost", "none"},
                                              {"Video", "GpuDriver", ""},
                                              {"Video", "WideMenus", "0"},
                                              {"Game", "PlayMusic", "1"},
                                              {"Game", "PlaySFX", "1"},
                                              {"Game", "WaitForStart", "0"},
                                              {"Game", "Music", "N64"},
                                              {"Game", "AssetPack", "default"},
                                              {"Game", "Language", "en-us"},
                                              {"Controls", "Preset", "Default"},
                                              {"Controls", "Player1", "auto"},
                                              {"Controls", "Player2", "auto"}}) {
        if (!config.Get(section, key)) {
            config.Set(section, key, value);
        }
    }
    // The assets of the pack of the command line or of the configuration, else of the default pack.
    std::string pack =
        parser.present("--asset-pack")
            .value_or(
                config.Get("Game", "AssetPack").value_or(std::string(coastline::DEFAULT_PACK)));
    auto packAssets = coastline::PackAssets(paths->data, paths->cache, pack);
    if (!packAssets) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "The asset pack %s cannot be read, and the default pack is used.",
                    pack.c_str());
        pack = coastline::DEFAULT_PACK;
        packAssets = coastline::DefaultAssets(paths->data);
    }
    const std::filesystem::path assets = *packAssets;
    SDL_Log("Asset pack: %s (%s)", pack.c_str(), assets.string().c_str());
    // A source imported by an older conversion has files the game no longer draws correctly.
    std::string reimportNotice;
    const auto staleImports = coastline::StaleImports(coastline::DefaultAssets(paths->data));
    if (!staleImports.empty()) {
        reimportNotice = coastline::ReimportMessage(staleImports);
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "The data was imported by an older version of the game. %s",
                    reimportNotice.c_str());
    }
    coastline::SelectLanguage(
        parser.present("--language")
            .value_or(
                config.Get("Game", "Language").value_or(std::string(coastline::DEFAULT_LANGUAGE))));
    coastline::LoadStrings(assets, coastline::SelectedLanguage());
    const std::filesystem::path release =
        coastline::ReleaseDirectory(assets, coastline::SelectedLanguage());
    SDL_Log("Language: %s (%s)",
            coastline::SelectedLanguage().c_str(),
            release.lexically_relative(assets).string().c_str());
    const nlohmann::json packSettings = coastline::PackSettings(paths->data, pack);
    // XmMusicGain of the configuration replaces the gain of the pack when the user writes it.
    const float musicGain = config.GetFloat(
        "Game", "XmMusicGain", packSettings.value("xmMusicGain", coastline::DEFAULT_XM_MUSIC_GAIN));
    const coastline::UsaMusicModule &menuModule = coastline::USA_MUSIC_MODULES[0];
    auto music = LoadMusicModule(assets, menuModule.offset);
    // The keys of the actions are not written, as each key present replaces its preset binding.
    coastline::LoadControls(config);
    const coastline::DisplayOptions displayOptions =
        DisplayOptionsFrom(config, parser, savesFrames);
    int status = EXIT_FAILURE;
    {
        SdlSession sdl(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
        if (sdl.ready && !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                        "The game is silent. Audio did not start: %s",
                        SDL_GetError());
        }
        if (!sdl.ready) {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL did not start: %s", SDL_GetError());
        } else if (auto window = coastline::GpuWindow::Create("Coastline", displayOptions)) {
            SetWindowIcon(window->Window(), assets);
            coastline::SetWideMenus(config.GetBoolean("Video", "WideMenus", false));
            coastline::SetFieldOfViewScale(config.GetFloat("Video", "FieldOfView", 1.0f));
            coastline::InputBindings input;
            coastline::N64Audio audio;
            bool playMusic = config.GetBoolean("Game", "PlayMusic", true) && audio.OpenDevice();
            audio.SetMusicGain(musicGain);
            coastline::ShareRaceMusicPlayer(playMusic ? &audio : nullptr);
            auto playMenuMusic = [&]() {
                if (music && playMusic) {
                    SDL_Log("The menus play the music module %s.", menuModule.name);
                    audio.Play(*music);
                }
            };
            playMenuMusic();
            const coastline::Script &commands = script.script;
            MouseState mouse;
            std::unique_ptr<coastline::SceneRenderer> sceneRenderer;
            std::unique_ptr<coastline::Intro> intro;
            std::unique_ptr<coastline::Menus> mainMenu;
            std::unique_ptr<coastline::MenuText> frameRateText;
            // The statistics of the third state of F, sampled only while they show.
            std::unique_ptr<coastline::PerformanceStats> performanceStats;
            // The seconds the request to import again remains at the top of the splash screens
            // and the menus.
            float reimportNoticeSeconds = reimportNotice.empty() ? 0.0f : 10.0f;
            // The screen of the front end that shows.
            enum class FrontEnd { Intro, AttractDemo, MainMenu, Race, Credits };
            FrontEnd frontEnd = FrontEnd::Intro;
            // The races run a 30th of a second a step, from the assets of the pack.
            constexpr float RACE_STEP = 1.0f / 30.0f;
            std::unique_ptr<coastline::RaceView> raceView;
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
            // The first step of the race of the credits loads the race and with it the track and
            // the car of the recording. The ending credits scroll, and go on to the next season.
            bool endingCredits = false;
            auto startCredits = [&](bool ending) {
                endingCredits = ending;
                openRaceData();
                coastline::StartCreditsRace(ending);
                coastline::StartConsoleCredits(release);
                coastline::StepAttractDemo(false);
                const coastline::RaceCar car{
                    coastline::AttractDemoCar(), coastline::AttractDemoColor(), std::nullopt};
                raceView = coastline::RaceView::Load(
                    *sceneRenderer, assets, coastline::AttractDemoTrack(), {&car, 1});
                raceTime = 0.0f;
                leaveDemo = false;
                return raceView != nullptr;
            };
            sceneRenderer = coastline::SceneRenderer::Create(*window);
            if (sceneRenderer) {
                // The splash screens and the menus are those of the USA release in every
                // language. The Japanese release has no producer logo, no models of its title,
                // and no item to save the options.
                const std::filesystem::path frontEndRelease = assets / "n64" / "usa";
                intro = coastline::Intro::Load(*sceneRenderer, frontEndRelease);
                mainMenu = coastline::Menus::Load(
                    *sceneRenderer, frontEndRelease, paths->data, paths->cache, config, configPath);
                if (mainMenu) {
                    mainMenu->SetMenuMusic(playMenuMusic);
                }
                frameRateText = coastline::MenuText::Load(*sceneRenderer, release / "fonts");
            }
            // The vertical sync V returns to, fifo when the settings choose it.
            const bool fifo = displayOptions.fifo;
            Uint64 lastTicks = SDL_GetTicksNS();
            // FrameRateLimit spaces the frames, at no limit when 0. --gpu-preset worst draws 30
            // frames a second, the steps of the race, for a slow computer.
            const long frameLimit = parser.present("--gpu-preset") == "worst" ?
                                        30 :
                                        config.GetInteger("Video", "FrameRateLimit", 0);
            const Uint64 framePeriod = frameLimit > 0 ? SDL_NS_PER_SECOND / frameLimit : 0;
            Uint64 nextFrame = lastTicks;
            Uint64 rateStart = lastTicks;
            int rateFrames = 0;
            DisplayKeys displayKeys;
            std::uint16_t lastButtons = 0;
            std::uint16_t lastSecondButtons = 0;
            bool running = true;
            bool printScreenWasDown = false;
            for (int frame = 1; running; ++frame) {
                // The mixers take the volumes the screens and the race wrote in the last frame.
                coastline::PublishN64Volumes();
                PushScriptedTyping(commands.typing, frame, window->Window());
                // The text typed on a keyboard while a screen takes it, with Backspace and Enter
                // as control characters.
                std::string typed;
                bool printScreen = false;
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    // F9 takes the frame as well, as a desktop such as Plasma keeps Print Screen.
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
                        } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                            typed += '\r';
                        }
                    }
                    input.HandleEvent(event);
                }
                // The shortcuts of the window are read before the controllers, which then do not
                // see the Enter of Alt and Enter. A race, the attract demo, and the credits read F
                // themselves, and F and V type while a screen takes text.
                bool raceShows = frontEnd == FrontEnd::Race || frontEnd == FrontEnd::AttractDemo ||
                                 frontEnd == FrontEnd::Credits;
                bool typing = SDL_TextInputActive(window->Window());
                switch (displayKeys.Read(!raceShows && !typing, !typing)) {
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
                    config.Set("Video", "VSync", !vsync ? "immediate" : fifo ? "fifo" : "vsync");
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
                    if (frame >= press.frame && frame < press.frame + press.frames) {
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
                        if (frame >= move.frame && frame < move.frame + move.frames) {
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
                    if (frame >= key.frame && frame < key.frame + key.frames) {
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
                // Print Screen saves the frame to the screenshots of the data directory, named by
                // the time it was taken.
                if (printScreen && !framePath) {
                    framePath = ScreenshotPath(paths->data / "screenshots");
                }
                window->UpdateFrameSize();
                if (intro) {
                    Uint64 now = SDL_GetTicksNS();
                    float seconds = savesFrames ?
                                        1.0f / 60.0f :
                                        std::min(static_cast<float>(now - lastTicks) / 1e9f, 0.1f);
                    lastTicks = now;
                    // A press of A, Start, or Z skips or chooses, B goes back, and every C button
                    // together shows the date of the build on the splash screens.
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
                        coastline::IntroButtons buttons{(pressed & CHOOSE) != 0,
                                                        (held & C_BUTTONS) == C_BUTTONS || shiftI};
                        coastline::IntroEnd end = intro->Update(seconds, buttons);
                        if (end == coastline::IntroEnd::MainMenu && mainMenu) {
                            mainMenu->Restart();
                            frontEnd = FrontEnd::MainMenu;
                        } else if (end != coastline::IntroEnd::Running) {
                            openRaceData();
                            // The first step loads the race and with it the track and the car.
                            coastline::StartAttractDemo();
                            coastline::StepAttractDemo(false);
                            const coastline::RaceCar demoCar{coastline::AttractDemoCar(),
                                                             coastline::AttractDemoColor(),
                                                             std::nullopt};
                            raceView = coastline::RaceView::Load(*sceneRenderer,
                                                                 assets,
                                                                 coastline::AttractDemoTrack(),
                                                                 {&demoCar, 1});
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
                            demoRunning = coastline::StepAttractDemo(leaveDemo);
                            leaveDemo = false;
                        }
                        if (!demoRunning) {
                            raceView.reset();
                            if (coastline::AttractDemoInterrupted() && mainMenu) {
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
                            end = coastline::StepRace();
                        }
                        if (end != coastline::RaceEnd::Running) {
                            // The module of the track plays on after the race until the main
                            // menu starts the music of the menus. The credits play under the
                            // music of the menus.
                            raceView.reset();
                            frontEnd = FrontEnd::MainMenu;
                            if (end != coastline::RaceEnd::Credits) {
                                mainMenu->FinishRace(end, coastline::LastRaceResults());
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
                        // A press of A, Start, or Z ends the credits as it ends the attract demo.
                        leaveDemo = leaveDemo || (pressed & CHOOSE) != 0;
                        raceTime += seconds;
                        bool creditsRunning = true;
                        while (creditsRunning && raceTime >= RACE_STEP) {
                            raceTime -= RACE_STEP;
                            creditsRunning = coastline::StepAttractDemo(leaveDemo);
                            coastline::StepConsoleCredits(RACE_STEP);
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
                    } else if (frontEnd == FrontEnd::AttractDemo || frontEnd == FrontEnd::Race ||
                               frontEnd == FrontEnd::Credits) {
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
                        // A screen that draws a cursor for the mouse hides the cursor of the
                        // system.
                        if (mainMenu->DrawsPointer() == SDL_CursorVisible()) {
                            if (mainMenu->DrawsPointer()) {
                                SDL_HideCursor();
                            } else {
                                SDL_ShowCursor();
                            }
                        }
                        if (end == coastline::MenusEnd::Splash) {
                            intro->Restart();
                            frontEnd = FrontEnd::Intro;
                        }
                        startRace = end == coastline::MenusEnd::Race;
                        startCreditsRace = end == coastline::MenusEnd::Credits;
                    }
                    // The HUD of a race and of the attract demo draws the rate itself, and the
                    // statistics draw it in their state.
                    if (g_bShowFpsCounter == 1 && frameRateText &&
                        (frontEnd == FrontEnd::Intro || frontEnd == FrontEnd::MainMenu)) {
                        DrawFrameRate(*sceneRenderer, *frameRateText);
                    }
                    if (reimportNoticeSeconds > 0.0f && frameRateText &&
                        (frontEnd == FrontEnd::Intro || frontEnd == FrontEnd::MainMenu)) {
                        DrawOverlayLine(*sceneRenderer, *frameRateText, reimportNotice.c_str(), 20);
                        reimportNoticeSeconds -= seconds;
                    }
                    if (g_bShowFpsCounter != coastline::FRAME_RATE_STATISTICS) {
                        performanceStats.reset();
                    } else if (frameRateText) {
                        if (!performanceStats) {
                            performanceStats = std::make_unique<coastline::PerformanceStats>();
                        }
                        performanceStats->Frame();
                        // The lines start under the lap and the times of a race, and the request
                        // to import again.
                        int y = 80;
                        // The left of the frame, under the lap, where the frame is wider than
                        // the 640 across of the screen.
                        const coastline::GpuWindow &frame = sceneRenderer->Window();
                        float across = coastline::ScreenExtent(
                            static_cast<float>(frame.FrameWidth()) /
                            static_cast<float>(std::max(frame.FrameHeight(), 1u)))[0];
                        int x = 16 - static_cast<int>(320.0f * (1.0f / across - 1.0f));
                        for (const std::string &line :
                             performanceStats->Lines(coastline::GpuMemoryBytes())) {
                            DrawOverlayLine(*sceneRenderer,
                                            *frameRateText,
                                            line.c_str(),
                                            y,
                                            x,
                                            coastline::TextStyle::Align::Left,
                                            true);
                            y += 16;
                        }
                    }
                    sceneRenderer->End(framePath ? &*framePath : nullptr);
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
                        SDL_Log("The music of the menus stops.");
                        audio.Stop();
                        coastline::RaceSetup setup = mainMenu->ChosenRace();
                        setup.waitForStart = config.GetBoolean("Game", "WaitForStart", false);
                        setup.autopilot = commands.autopilot;
                        coastline::StartRace(
                            setup, assets / "music", config.Get("Game", "Music") == "CD");
                        std::vector<coastline::RaceCar> cars = coastline::RaceCars();
                        for (std::size_t i = 0; i < cars.size() && i < setup.cars.size(); ++i) {
                            if (static_cast<int>(i) < setup.players) {
                                cars[i].images = setup.cars[i].images;
                                cars[i].otherLook = setup.cars[i].otherLook;
                            }
                        }
                        raceView = coastline::RaceView::Load(
                            *sceneRenderer, assets, coastline::RaceTrack(), cars);
                        raceTime = 0.0f;
                        if (raceView) {
                            frontEnd = FrontEnd::Race;
                        } else {
                            coastline::AbandonRace();
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
                RecordFrameTime();
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
                Uint64 period = std::max(framePeriod, window->RefreshPeriod());
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
            status = EXIT_SUCCESS;
        }
    }
    if (!config.Save(configPath)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "BossRally.ini cannot be written.");
    }
    coastline::StopGameLog();
    return status;
}
