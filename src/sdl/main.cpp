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

#include "config.h"
#include "earsound.h"
#include "enginecheck.h"
#include "gpu.h"
#include "importer.h"
#include "input.h"
#include "intro.h"
#include "log.h"
#include "mainmenu.h"
#include "modelview.h"
#include "music.h"
#include "paths.h"
#include "racedemo.h"
#include "raceview.h"
#include "scenerenderer.h"
#include "tlrenderer.h"

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
                                             const argparse::ArgumentParser &parser) {
    coastline::DisplayOptions options;
    options.width = static_cast<int>(config.GetInteger("Video", "Width", options.width));
    options.height = static_cast<int>(config.GetInteger("Video", "Height", options.height));
    options.fullscreen = config.GetBoolean("Video", "Fullscreen", options.fullscreen);
    options.vsync = config.GetBoolean("Video", "VSync", options.vsync);
    options.antialias =
        static_cast<int>(config.GetInteger("Video", "Antialias", options.antialias));
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
    if (parser.get<bool>("--fullscreen")) {
        options.fullscreen = true;
    }
    if (parser.get<bool>("--windowed")) {
        options.fullscreen = false;
    }
    if (parser.get<bool>("--no-vsync")) {
        options.vsync = false;
    }
    if (auto driver = parser.present("--gpu-driver")) {
        options.driver = *driver;
    }
    return options;
}

// The XM module of the music pack default for a module of the USA cartridge.
std::optional<std::vector<std::uint8_t>> LoadMusicModule(const std::filesystem::path &data,
                                                         std::uint32_t offset) {
    const auto *module =
        std::ranges::find(coastline::USA_MUSIC_MODULES, offset, &coastline::UsaMusicModule::offset);
    if (module == std::end(coastline::USA_MUSIC_MODULES)) {
        return std::nullopt;
    }
    std::ifstream file(coastline::DefaultAssets(data) / "music" /
                           (std::string(module->name) + ".xm"),
                       std::ios::binary);
    if (!file) {
        SDL_LogWarn(SDL_LOG_CATEGORY_AUDIO,
                    "The USA cartridge has not been imported, and the music is silent.");
        return std::nullopt;
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), {});
}

std::optional<std::vector<std::uint8_t>> LoadMusicByIndex(const std::filesystem::path &data,
                                                          int index) {
    if (index < 0 || index >= static_cast<int>(std::size(coastline::USA_MUSIC_MODULES))) {
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "There is no music module %d.", index);
        return std::nullopt;
    }
    return LoadMusicModule(data, coastline::USA_MUSIC_MODULES[index].offset);
}

// A press of --press: the buttons held from a frame for SCRIPTED_PRESS_FRAMES frames.
struct ScriptedPress {
    int frame;
    std::uint16_t buttons;
};

constexpr int SCRIPTED_PRESS_FRAMES = 6;

// A malformed press is logged and left out.
std::vector<ScriptedPress> ParsePresses(const argparse::ArgumentParser &parser) {
    static constexpr std::array<std::pair<std::string_view, std::uint16_t>, 14> BUTTONS{
        {{"a", coastline::BUTTON_A},
         {"b", coastline::BUTTON_B},
         {"z", coastline::BUTTON_Z},
         {"start", coastline::BUTTON_START},
         {"l", coastline::BUTTON_L},
         {"r", coastline::BUTTON_R},
         {"up", coastline::BUTTON_D_UP},
         {"down", coastline::BUTTON_D_DOWN},
         {"left", coastline::BUTTON_D_LEFT},
         {"right", coastline::BUTTON_D_RIGHT},
         {"cu", coastline::BUTTON_C_UP},
         {"cd", coastline::BUTTON_C_DOWN},
         {"cl", coastline::BUTTON_C_LEFT},
         {"cr", coastline::BUTTON_C_RIGHT}}};
    std::vector<ScriptedPress> presses;
    for (const auto &value : parser.get<std::vector<std::string>>("--press")) {
        auto colon = value.find(':');
        auto found = std::ranges::find(
            BUTTONS,
            std::string_view(value).substr(colon == std::string::npos ? 0 : colon + 1),
            &std::pair<std::string_view, std::uint16_t>::first);
        int frame = std::atoi(value.substr(0, colon).c_str());
        if (colon == std::string::npos || found == BUTTONS.end() || frame <= 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s is not FRAME:BUTTON.", value.c_str());
            continue;
        }
        presses.push_back({frame, found->second});
    }
    return presses;
}

coastline::TextureHandle CreateCheckerTexture(coastline::TlRenderer &renderer) {
    constexpr std::uint32_t SIZE = 64;
    std::vector<std::uint8_t> texels(SIZE * SIZE * 4);
    for (std::uint32_t y = 0; y < SIZE; ++y) {
        for (std::uint32_t x = 0; x < SIZE; ++x) {
            bool light = ((x / 8) + (y / 8)) % 2 == 0;
            std::uint8_t *texel = &texels[(y * SIZE + x) * 4];
            texel[0] = light ? 0xF0 : 0x30;
            texel[1] = light ? 0xC0 : 0x30;
            texel[2] = light ? 0x40 : 0x80;
            texel[3] = 0xFF;
        }
    }
    return renderer.CreateTexture(SIZE, SIZE, texels, false);
}

// The scene exercises the renderer with the primitives of the game (a textured quad in
// perspective, a fogged quad behind it, and an alpha-blended triangle in front). The triangle
// turns with the frame number.
void DrawRendererTestScene(coastline::TlRenderer &renderer,
                           coastline::TextureHandle texture,
                           int frame,
                           const std::filesystem::path *capture) {
    using coastline::TlVertex;
    constexpr std::uint16_t QUAD[] = {0, 1, 2, 0, 2, 3};
    renderer.BeginScene(0xFF203050);
    auto &states = renderer.States();
    states = {};
    states.cull = coastline::CullMode::None;

    // The far quad has rhw 0.25 and z 0.9, and fades to the fog colour at its top.
    states.fog = true;
    states.fogColor = 0xFF8090A0;
    renderer.SetTexture(0);
    TlVertex fogged[] = {{40, 40, 0.9f, 0.25f, 0xFF20A020, 0x00000000, 0, 0},
                         {600, 40, 0.9f, 0.25f, 0xFF20A020, 0x00000000, 0, 0},
                         {600, 300, 0.9f, 0.25f, 0xFF20A020, 0xFF000000, 0, 0},
                         {40, 300, 0.9f, 0.25f, 0xFF20A020, 0xFF000000, 0, 0}};
    renderer.DrawIndexed(fogged, QUAD);

    // A floor in perspective: the near edge has rhw 1 and the far edge rhw 0.2, so the checks
    // shrink with the distance when the texture coordinates interpolate correctly.
    states.fog = false;
    renderer.SetTexture(texture);
    TlVertex floor[] = {{220, 180, 0.6f, 0.2f, 0xFFFFFFFF, 0, 0, 0},
                        {420, 180, 0.6f, 0.2f, 0xFFFFFFFF, 0, 4, 0},
                        {620, 460, 0.1f, 1.0f, 0xFFFFFFFF, 0, 4, 4},
                        {20, 460, 0.1f, 1.0f, 0xFFFFFFFF, 0, 0, 4}};
    renderer.DrawIndexed(floor, QUAD);

    states.blend = true;
    states.sourceBlend = coastline::BlendFactor::SourceAlpha;
    states.destinationBlend = coastline::BlendFactor::InverseSourceAlpha;
    renderer.SetTexture(0);
    float angle = static_cast<float>(frame) * 0.05f;
    TlVertex triangle[3];
    for (int i = 0; i < 3; ++i) {
        float a = angle + static_cast<float>(i) * 2.0943951f;
        triangle[i] = {320 + 120 * SDL_cosf(a),
                       240 + 120 * SDL_sinf(a),
                       0.05f,
                       1.0f,
                       i == 0 ? 0x80FF2020u :
                       i == 1 ? 0x8020FF20u :
                                0x802020FFu,
                       0,
                       0,
                       0};
    }
    constexpr std::uint16_t TRIANGLE[] = {0, 1, 2};
    renderer.DrawIndexed(triangle, TRIANGLE);
    renderer.EndScene(capture);
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

int ImportFromCommandLine(const argparse::ArgumentParser &parser,
                          const std::filesystem::path &data) {
    int status = EXIT_SUCCESS;
    for (const auto &rom : parser.get<std::vector<std::string>>("--import-rom")) {
        if (auto release = coastline::ImportRom(rom, coastline::DefaultAssets(data))) {
            static constexpr const char *names[] = {"USA", "Japanese", "European"};
            SDL_Log("Imported the %s cartridge from %s.",
                    names[static_cast<int>(*release)],
                    rom.c_str());
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", release.error().c_str());
            status = EXIT_FAILURE;
        }
    }
    if (auto disc = parser.present("--import-disc")) {
        auto written = coastline::ImportDisc(
            *disc, coastline::DefaultAssets(data), [](const std::string &step, double fraction) {
                SDL_Log("[%3d%%] %s", static_cast<int>(fraction * 100), step.c_str());
            });
        if (written) {
            SDL_Log("Imported %zu files from %s.", *written, disc->c_str());
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", written.error().c_str());
            status = EXIT_FAILURE;
        }
    }
    coastline::StopGameLog();
    return status;
}

// A sound named relative to the data takes its file from the default assets.
int PlaySoundFromCommandLine(const argparse::ArgumentParser &parser,
                             const std::filesystem::path &data) {
    std::filesystem::path path = parser.get<std::string>("--play-sound");
    std::error_code error;
    if (path.is_relative() && !std::filesystem::exists(path, error)) {
        path = coastline::DefaultAssets(data) / path;
    }
    coastline::EarSoundOptions options;
    options.direction = parser.get<int>("--sound-direction");
    options.whole = parser.get<bool>("--sound-whole");
    if (auto wav = parser.present("--sound-wav")) {
        options.output = *wav;
    }
    int status = EXIT_FAILURE;
    {
        SdlSession sdl(SDL_INIT_EVENTS);
        if (!sdl.ready) {
            SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, "SDL did not start: %s", SDL_GetError());
        } else if (coastline::PlayEarSound(path, options)) {
            status = EXIT_SUCCESS;
        }
    }
    coastline::StopGameLog();
    return status;
}

} // namespace

int main(int argc, char *argv[]) {
    RaiseOpenFileLimit();
    argparse::ArgumentParser parser(APPLICATION, COASTLINE_VERSION);
    parser.add_description("Top Gear Rally and Boss Rally in one stand-alone game.");
    parser.add_argument("--data-dir").help("read the game data from this directory");
    parser.add_argument("--width").help("width of the window in pixels").scan<'i', int>();
    parser.add_argument("--height").help("height of the window in pixels").scan<'i', int>();
    parser.add_argument("--fullscreen").help("cover the display").flag();
    parser.add_argument("--windowed").help("draw in a window").flag();
    parser.add_argument("--antialias")
        .help("samples of each pixel for anti-aliasing: 1 for none, or 2, 4, or 8")
        .scan<'i', int>();
    parser.add_argument("--no-vsync").help("present frames without waiting").flag();
    parser.add_argument("--gpu-driver")
        .help("SDL GPU driver, such as vulkan, metal, or direct3d12");
    parser.add_argument("--list-gpu-drivers").help("write the GPU drivers and exit").flag();
    parser.add_argument("--import-disc")
        .help("copy the game data and music from a Boss Rally CUE sheet or ISO image, and exit");
    parser.add_argument("--import-rom")
        .help("copy a Top Gear Rally cartridge image, checked by SHA-1, and exit")
        .append();
    parser.add_argument("--music")
        .help("music module to play: 0 for the menus, 1 to 5 for the tracks")
        .default_value(0)
        .scan<'i', int>();
    parser.add_argument("--music-wav").help("render the music to this WAV file and exit");
    parser.add_argument("--music-seconds")
        .help("length of the WAV file in seconds")
        .default_value(30)
        .scan<'i', int>();
    parser.add_argument("--play-sound")
        .help("play a wave file through EAR, from the default assets when relative, and exit");
    parser.add_argument("--sound-wav")
        .help("write the mix of --play-sound to this WAV file in place of the device");
    parser.add_argument("--sound-direction")
        .help("direction of --play-sound: 0 for the centre, 3 for the left, or 5 for the right")
        .default_value(0)
        .scan<'i', int>();
    parser.add_argument("--sound-whole")
        .help("read the whole file of --play-sound before it plays, rather than streaming it")
        .flag();
    parser.add_argument("--check-engine")
        .help("load a track, a car, and the sounds and music of a race from the imported data "
              "through the race engine, write the results to the log, and exit")
        .flag();
    parser.add_argument("--scene").help(
        "scene to start in: intro, the default, main-menu, attract-demo, renderer-test, or "
        "model:FILE.gltf for a converted model or track, from the imported data when relative");
    parser.add_argument("--screenshot").help("save a frame to this PNG file");
    parser.add_argument("--screenshot-frame")
        .help("number of the frame to save, from 1")
        .default_value(60)
        .scan<'i', int>();
    parser.add_argument("--screenshot-every")
        .help("also save every this many frames, numbered after the name of --screenshot")
        .default_value(0)
        .scan<'i', int>();
    parser.add_argument("--quit-after-screenshot").help("exit once the frame is saved").flag();
    parser.add_argument("--press")
        .help("press a button of the first controller for 6 frames from a frame, as FRAME:BUTTON "
              "with BUTTON one of a, b, z, start, l, r, up, down, left, right, cu, cd, cl, or cr")
        .append();
    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n' << parser;
        return EXIT_FAILURE;
    }

    if (parser.get<bool>("--list-gpu-drivers")) {
        for (int i = 0; i < SDL_GetNumGPUDrivers(); ++i) {
            std::cout << SDL_GetGPUDriver(i) << '\n';
        }
        return EXIT_SUCCESS;
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
    SDL_SetAppMetadata("Coastline", COASTLINE_VERSION, "io.github.tatsh.coastline");
    if (!coastline::StartGameLog(paths->state / "game.log")) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "The log file cannot be opened.");
    }
    SDL_Log("Coastline %s", COASTLINE_VERSION);
    SDL_Log("Configuration: %s", paths->config.string().c_str());
    SDL_Log("Data: %s", paths->data.string().c_str());
    SDL_Log("State: %s", paths->state.string().c_str());
    SDL_Log("Cache: %s", paths->cache.string().c_str());

    if (parser.is_used("--import-disc") || parser.is_used("--import-rom")) {
        return ImportFromCommandLine(parser, paths->data);
    }
    if (parser.is_used("--play-sound")) {
        return PlaySoundFromCommandLine(parser, paths->data);
    }
    if (parser.get<bool>("--check-engine")) {
        int checked = coastline::CheckEngine(coastline::DefaultAssets(paths->data));
        coastline::StopGameLog();
        return checked;
    }
    auto music = LoadMusicByIndex(paths->data, parser.get<int>("--music"));
    if (auto wav = parser.present("--music-wav")) {
        if (!music) {
            coastline::StopGameLog();
            return EXIT_FAILURE;
        }
        coastline::N64Audio audio;
        audio.Play(*music);
        auto samples = audio.Render(static_cast<std::size_t>(parser.get<int>("--music-seconds")) *
                                    coastline::N64_AUDIO_RATE);
        bool written = coastline::WriteWav(*wav, samples, coastline::N64_AUDIO_RATE);
        SDL_Log(written ? "Wrote %s." : "%s cannot be written.", wav->c_str());
        coastline::StopGameLog();
        return written ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    auto configPath = paths->config / "BossRally.ini";
    auto config = coastline::GameConfig::Load(configPath);
    // Every setting is written to the file on the first run: the keys of the PC release first,
    // then the keys of this game.
    for (const auto &[section, key, value] : {std::tuple{"Video", "Driver", "SDL"},
                                              {"Video", "Width", "1280"},
                                              {"Video", "Height", "720"},
                                              {"Video", "Fullscreen", "0"},
                                              {"Video", "VSync", "1"},
                                              {"Video", "GpuDriver", ""},
                                              {"Game", "PlayMusic", "1"},
                                              {"Game", "PlaySFX", "1"},
                                              {"Game", "Music", "N64"}}) {
        if (!config.Get(section, key)) {
            config.Set(section, key, value);
        }
    }
    const std::string scene = parser.present("--scene").value_or("");
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
        } else if (auto window = coastline::GpuWindow::Create("Coastline",
                                                              DisplayOptionsFrom(config, parser))) {
            coastline::InputBindings input;
            coastline::N64Audio audio;
            bool playMusic = config.GetBoolean("Game", "PlayMusic", true) && audio.OpenDevice();
            if (music && playMusic) {
                audio.Play(*music);
            }
            std::optional<std::filesystem::path> screenshot;
            if (auto path = parser.present("--screenshot")) {
                screenshot = *path;
            }
            int screenshotFrame = parser.get<int>("--screenshot-frame");
            int screenshotEvery = parser.get<int>("--screenshot-every");
            bool quitAfterScreenshot = parser.get<bool>("--quit-after-screenshot");
            std::vector<ScriptedPress> presses = ParsePresses(parser);
            std::unique_ptr<coastline::TlRenderer> renderer;
            std::unique_ptr<coastline::ModelView> modelView;
            std::unique_ptr<coastline::SceneRenderer> sceneRenderer;
            std::unique_ptr<coastline::Intro> intro;
            std::unique_ptr<coastline::MainMenu> mainMenu;
            // The screen of the front end that shows.
            enum class FrontEnd { Intro, AttractDemo, MainMenu };
            FrontEnd frontEnd = FrontEnd::Intro;
            // The race of the attract demo runs a 30th of a second a step, from the default assets.
            constexpr float RACE_STEP = 1.0f / 30.0f;
            const std::filesystem::path defaults = coastline::DefaultAssets(paths->data);
            std::unique_ptr<coastline::RaceView> raceView;
            bool raceDataOpen = false;
            float raceTime = 0.0f;
            bool leaveDemo = false;
            bool startInDemo = scene == "attract-demo";
            coastline::TextureHandle testTexture = 0;
            if (scene == "renderer-test") {
                renderer = coastline::TlRenderer::Create(*window);
                testTexture = renderer ? CreateCheckerTexture(*renderer) : 0;
            } else if (scene.starts_with("model:")) {
                std::filesystem::path model = scene.substr(6);
                if (model.is_relative()) {
                    model = coastline::DefaultAssets(paths->data) / model;
                }
                modelView = coastline::ModelView::Create(*window, model);
            } else if (scene.empty() || scene == "intro" || scene == "main-menu" ||
                       scene == "attract-demo") {
                sceneRenderer = coastline::SceneRenderer::Create(*window);
                if (sceneRenderer) {
                    auto release = coastline::DefaultAssets(paths->data) / "n64" / "usa";
                    intro = coastline::Intro::Load(*sceneRenderer, release);
                    mainMenu = coastline::MainMenu::Load(*sceneRenderer, release);
                    if (scene == "main-menu" && mainMenu) {
                        frontEnd = FrontEnd::MainMenu;
                    }
                }
            }
            // A capture steps the splash screens by a 60th of a second a frame, for the same
            // frames on every machine.
            bool fixedStep = screenshot.has_value();
            Uint64 lastTicks = SDL_GetTicksNS();
            std::uint16_t lastButtons = 0;
            bool running = true;
            for (int frame = 1; running; ++frame) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_EVENT_QUIT) {
                        running = false;
                    }
                    input.HandleEvent(event);
                }
                auto ports = input.Poll();
                for (const ScriptedPress &press : presses) {
                    if (frame >= press.frame && frame < press.frame + SCRIPTED_PRESS_FRAMES) {
                        ports[0].buttons |= press.buttons;
                    }
                }
                bool capture = screenshot && frame == screenshotFrame;
                std::optional<std::filesystem::path> framePath;
                if (capture) {
                    framePath = screenshot;
                } else if (screenshot && screenshotEvery > 0 && frame % screenshotEvery == 0) {
                    char number[16];
                    std::snprintf(number, sizeof(number), "-%06d", frame);
                    framePath = screenshot->parent_path() / (screenshot->stem().string() + number +
                                                             screenshot->extension().string());
                }
                if (intro) {
                    Uint64 now = SDL_GetTicksNS();
                    float seconds = fixedStep ?
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
                    constexpr int STICK_PUSH = 40;
                    std::uint16_t held = ports[0].buttons;
                    if (ports[0].stickX < -STICK_PUSH) {
                        held |= coastline::BUTTON_D_LEFT;
                    }
                    if (ports[0].stickX > STICK_PUSH) {
                        held |= coastline::BUTTON_D_RIGHT;
                    }
                    std::uint16_t pressed = held & ~lastButtons;
                    lastButtons = held;
                    if (frontEnd == FrontEnd::Intro) {
                        coastline::IntroButtons buttons{(pressed & CHOOSE) != 0,
                                                        (held & C_BUTTONS) == C_BUTTONS};
                        coastline::IntroEnd end = startInDemo ? coastline::IntroEnd::AttractDemo :
                                                                intro->Update(seconds, buttons);
                        startInDemo = false;
                        if (end == coastline::IntroEnd::MainMenu && mainMenu) {
                            mainMenu->Restart();
                            frontEnd = FrontEnd::MainMenu;
                        } else if (end != coastline::IntroEnd::Running) {
                            // The race engine reads its files from the default assets.
                            if (!raceDataOpen) {
                                std::error_code error;
                                std::filesystem::current_path(defaults, error);
                                coastline::OpenRaceData();
                                raceDataOpen = true;
                            }
                            // The first step loads the race and with it the track and the car.
                            coastline::StartAttractDemo();
                            coastline::StepAttractDemo(false);
                            raceView = coastline::RaceView::Load(*sceneRenderer,
                                                                 defaults,
                                                                 coastline::AttractDemoTrack(),
                                                                 coastline::AttractDemoCar(),
                                                                 coastline::AttractDemoColor());
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
                    }
                    if (frontEnd == FrontEnd::Intro) {
                        sceneRenderer->Begin({0.0f, 0.0f, 0.0f, 1.0f});
                        intro->Draw(*sceneRenderer);
                    } else if (frontEnd == FrontEnd::AttractDemo) {
                        sceneRenderer->Begin(raceView->ClearColor());
                        raceView->Draw(*sceneRenderer);
                    } else {
                        sceneRenderer->Begin({0.0f, 0.0f, 0.0f, 1.0f});
                        coastline::MenuButtons buttons{(pressed & coastline::BUTTON_D_LEFT) != 0,
                                                       (pressed & coastline::BUTTON_D_RIGHT) != 0,
                                                       (pressed & CHOOSE) != 0,
                                                       (pressed & coastline::BUTTON_B) != 0,
                                                       held != 0};
                        coastline::MainMenuEnd end = mainMenu->Update(seconds, buttons);
                        if (end == coastline::MainMenuEnd::Left) {
                            intro->Restart();
                            frontEnd = FrontEnd::Intro;
                        } else if (end == coastline::MainMenuEnd::Chosen) {
                            // ponytail: the screens after the main menu are not built yet, and the
                            // menu shows again on the item chosen.
                            SDL_Log("The %s screen is not built yet.",
                                    mainMenu->SelectionName().c_str());
                            mainMenu->Restart();
                        }
                        mainMenu->Draw(*sceneRenderer);
                    }
                    sceneRenderer->End(framePath ? &*framePath : nullptr);
                } else if (modelView) {
                    modelView->Draw(framePath ? &*framePath : nullptr);
                } else if (renderer) {
                    DrawRendererTestScene(
                        *renderer, testTexture, frame, framePath ? &*framePath : nullptr);
                } else {
                    // The clear colour follows the stick of the first controller until the
                    // game draws its frames here.
                    float red = 0.5f + ports[0].stickX / 160.0f;
                    float green = 0.5f + ports[0].stickY / 160.0f;
                    float blue = ports[0].buttons != 0 ? 0.6f : 0.2f;
                    window->PresentClear(SDL_FColor{red, green, blue, 1.0f},
                                         framePath ? &*framePath : nullptr);
                }
                if (capture && quitAfterScreenshot) {
                    running = false;
                }
            }
            status = EXIT_SUCCESS;
        }
    }
    if (!config.Save(configPath)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "BossRally.ini cannot be written.");
    }
    coastline::StopGameLog();
    return status;
}
