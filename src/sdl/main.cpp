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
#include <tuple>
#include <vector>

#include <argparse/argparse.hpp>
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "config.h"
#include "gpu.h"
#include "importer.h"
#include "input.h"
#include "log.h"
#include "music.h"
#include "paths.h"
#include "tlrenderer.h"
#ifdef COASTLINE_PC_GAME
#include "cartridgedata.h"
#include "dinputcompat.h"
#include "n64displaylist.h"
#include "n64frontend.h"
#include "pcgamerun.h"
#include "renderhost.h"
#include "scene.h"
#endif

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
    std::ifstream file(data / "music" / "default" / (std::string(module->name) + ".xm"),
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

// A key held over a range of frames, from a test script.
struct ScriptedKey {
    int scancode;
    int first;
    int last;
};

// Read a script such as SPACE@300-305,UP@320-900. An entry with one frame holds the key for that
// frame.
std::vector<ScriptedKey> ParseKeyScript(const std::string &script) {
    std::vector<ScriptedKey> keys;
    std::size_t start = 0;
    while (start < script.size()) {
        std::size_t end = script.find(',', start);
        std::string entry = script.substr(start, end - start);
        start = end == std::string::npos ? script.size() : end + 1;
        std::size_t at = entry.find('@');
        if (at == std::string::npos) {
            SDL_LogWarn(SDL_LOG_CATEGORY_INPUT, "The key %s has no frame.", entry.c_str());
            continue;
        }
        SDL_Scancode scancode = SDL_GetScancodeFromName(entry.substr(0, at).c_str());
        int first = 0;
        int last = 0;
        int count = std::sscanf(entry.c_str() + at + 1, "%d-%d", &first, &last);
        if (scancode == SDL_SCANCODE_UNKNOWN || count < 1) {
            SDL_LogWarn(SDL_LOG_CATEGORY_INPUT, "The key %s is not valid.", entry.c_str());
            continue;
        }
        keys.push_back({scancode, first, count == 2 ? last : first});
    }
    return keys;
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

#ifdef COASTLINE_PC_GAME
// The pictures of the front end, from the imported USA cartridge, as the texture pack default.
void BuildDefaultTexturePack(const std::filesystem::path &data) {
    if (!coastline::CartridgeData::Load(data / "n64" / "usa")) {
        return;
    }
    ReadN64FrontEndData();
    int written = WriteN64TexturePack(data / "textures" / "default");
    SDL_Log("Wrote %d textures to the texture pack default.", written);
}
#endif

int ImportFromCommandLine(const argparse::ArgumentParser &parser,
                          const std::filesystem::path &data) {
    int status = EXIT_SUCCESS;
    for (const auto &rom : parser.get<std::vector<std::string>>("--import-rom")) {
        if (auto release = coastline::ImportRom(rom, data)) {
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
        auto written =
            coastline::ImportDisc(*disc, data, [](const std::string &step, double fraction) {
                SDL_Log("[%3d%%] %s", static_cast<int>(fraction * 100), step.c_str());
            });
        if (written) {
            SDL_Log("Imported %zu files from %s.", *written, disc->c_str());
        } else {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", written.error().c_str());
            status = EXIT_FAILURE;
        }
    }
#ifdef COASTLINE_PC_GAME
    BuildDefaultTexturePack(data);
#endif
    coastline::StopGameLog();
    return status;
}

} // namespace

int main(int argc, char *argv[]) {
    argparse::ArgumentParser parser(APPLICATION, COASTLINE_VERSION);
    parser.add_description("Top Gear Rally and Boss Rally in one stand-alone game.");
    parser.add_argument("--data-dir").help("read the game data from this directory");
    parser.add_argument("--width").help("width of the window in pixels").scan<'i', int>();
    parser.add_argument("--height").help("height of the window in pixels").scan<'i', int>();
    parser.add_argument("--fullscreen").help("cover the display").flag();
    parser.add_argument("--windowed").help("draw in a window").flag();
    parser.add_argument("--dump-textures")
        .help("write each texture of the front end to the textures directory as a PNG file")
        .flag();
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
    parser.add_argument("--scene").help(
        "scene to start in: n64-frontend (the default), race, renderer-test, or n64-data");
    for (const char *name : {"--game-mode", "--track", "--car", "--weather"}) {
        parser.add_argument(name)
            .help("choice of the race scene, as the menus number it")
            .scan<'i', int>();
    }
    parser.add_argument("--keys").help(
        "keys to hold in the race and n64-frontend scenes, such as SPACE@300-305,UP@320-900 for "
        "frame ranges");
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
    // The game starts at the front end of the Nintendo 64 release unless a test scene is chosen.
    const std::string scene = parser.present("--scene").value_or("n64-frontend");
#ifdef COASTLINE_PC_GAME
    // The data of the front end of the Nintendo 64 release, read and written to the log.
    if (scene == "n64-data") {
        if (!coastline::CartridgeData::Load(paths->data / "n64" / "usa")) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "The USA cartridge has not been imported.");
            return EXIT_FAILURE;
        }
        ReadN64FrontEndData();
        LogN64FrontEndData();
        SDL_Log("The front end starts at %p.", reinterpret_cast<void *>(&InitGameAndShowMenusN64));
        return EXIT_SUCCESS;
    }
#endif
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
            // The PC game and the front end choose their modules.
            if (music && playMusic && scene != "race" && scene != "n64-frontend") {
                audio.Play(*music);
            }
            std::optional<std::filesystem::path> screenshot;
            if (auto path = parser.present("--screenshot")) {
                screenshot = *path;
            }
            int screenshotFrame = parser.get<int>("--screenshot-frame");
            int screenshotEvery = parser.get<int>("--screenshot-every");
            bool quitAfterScreenshot = parser.get<bool>("--quit-after-screenshot");
            std::unique_ptr<coastline::TlRenderer> renderer;
            coastline::TextureHandle testTexture = 0;
            bool pcGame = false;
            if (scene == "renderer-test") {
                renderer = coastline::TlRenderer::Create(*window);
                testTexture = renderer ? CreateCheckerTexture(*renderer) : 0;
            }
#ifdef COASTLINE_PC_GAME
            if (scene == "race") {
                renderer = coastline::TlRenderer::Create(*window);
                bool pcMusic = config.Get("Game", "Music").value_or("N64") == "PC";
                for (auto [name, choice] : {std::pair{"--game-mode", &g_SdlScene.gameMode},
                                            {"--track", &g_SdlScene.track},
                                            {"--car", &g_SdlScene.car},
                                            {"--weather", &g_SdlScene.weather}}) {
                    if (auto value = parser.present<int>(name)) {
                        *choice = *value;
                    }
                }
                pcGame = renderer &&
                         coastline::StartPcGame(*renderer, paths->data, paths->state, pcMusic);
            }
            // The front end of the Nintendo 64 release runs its own loop, and the frames it
            // presents pump the events, the keys of the script, and the screenshot.
            if (scene == "n64-frontend") {
                renderer = coastline::TlRenderer::Create(*window);
                if (!renderer) {
                    return EXIT_FAILURE;
                }
                if (!coastline::CartridgeData::Load(paths->data / "n64" / "usa")) {
                    SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION,
                                    "The USA cartridge has not been imported. Run coastline "
                                    "--import-rom with the image of the USA release.");
                    return EXIT_FAILURE;
                }
                if (!coastline::StartPcGame(*renderer, paths->data, paths->state, false)) {
                    return EXIT_FAILURE;
                }
                ReadN64FrontEndData();
                // An import from before the texture packs wrote none.
                if (std::error_code error;
                    !std::filesystem::exists(paths->data / "textures" / "default", error)) {
                    WriteN64TexturePack(paths->data / "textures" / "default");
                }
                SetN64FrontEndInput(&input);
                SetN64FrontEndAudio(playMusic ? &audio : nullptr);
                coastline::SetN64TextureDirectory(
                    paths->data / "textures",
                    config.Get("Video", "TexturePack").value_or("default"),
                    parser.get<bool>("--dump-textures"));
                coastline::StartPcGameForFrontEnd();
                auto keys = ParseKeyScript(parser.present("--keys").value_or(""));
                int frame = 0;
                std::optional<std::filesystem::path> framePath;
                coastline::SetFramePump([&]() {
                    ++frame;
                    SDL_Event event;
                    while (SDL_PollEvent(&event)) {
                        if (event.type == SDL_EVENT_QUIT) {
                            return false;
                        }
                        input.HandleEvent(event);
                    }
                    std::vector<int> held;
                    for (const ScriptedKey &key : keys) {
                        if (frame + 1 >= key.first && frame + 1 <= key.last) {
                            held.push_back(key.scancode);
                        }
                    }
                    coastline::SetScriptedKeys(std::move(held));
                    framePath.reset();
                    if (screenshot && frame + 1 == screenshotFrame) {
                        framePath = screenshot;
                    } else if (screenshot && screenshotEvery > 0 &&
                               (frame + 1) % screenshotEvery == 0) {
                        char number[16];
                        std::snprintf(number, sizeof(number), "-%06d", frame + 1);
                        framePath =
                            screenshot->parent_path() / (screenshot->stem().string() + number +
                                                         screenshot->extension().string());
                    }
                    coastline::SetPcGameCapture(framePath ? &*framePath : nullptr);
                    return !(quitAfterScreenshot && screenshot && frame >= screenshotFrame);
                });
                InitGameAndShowMenusN64();
            }
#endif
            bool running = true;
            std::uint32_t pcMusicModule = 0;
            auto keyScript = ParseKeyScript(parser.present("--keys").value_or(""));
            for (int frame = 1; running; ++frame) {
                SDL_Event event;
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_EVENT_QUIT) {
                        running = false;
                    }
                    input.HandleEvent(event);
                }
                auto ports = input.Poll();
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
                if (pcGame) {
#ifdef COASTLINE_PC_GAME
                    std::vector<int> held;
                    for (const ScriptedKey &key : keyScript) {
                        if (frame >= key.first && frame <= key.last) {
                            held.push_back(key.scancode);
                        }
                    }
                    coastline::SetScriptedKeys(std::move(held));
                    running =
                        coastline::RunPcGameFrame(framePath ? &*framePath : nullptr) && running;
                    std::uint32_t module = coastline::PcGameMusicModule();
                    if (music && module != 0 && module != pcMusicModule) {
                        pcMusicModule = module;
                        if (auto data = LoadMusicModule(paths->data, module)) {
                            audio.Play(*data);
                        }
                    }
#endif
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
