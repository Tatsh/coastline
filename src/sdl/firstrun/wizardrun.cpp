#include "wizardrun.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "assetimport.h"
#include "filebrowser.h"
#include "freecamera.h"
#include "importer.h"
#include "scenerenderer.h"
#include "setupwizard.h"
#include "textfont.h"
#include "ziproms.h"

namespace coastline {

namespace {

constexpr std::array<std::string_view, WIZARD_CARTRIDGES> CARTRIDGE_NAMES{
    "USA", "European", "Japanese"};

std::optional<WizardCartridge> WizardRelease(RomRelease release) {
    switch (release) {
    case RomRelease::Usa:
        return WizardCartridge::Usa;
    case RomRelease::Eur:
        return WizardCartridge::Eur;
    case RomRelease::Jpn:
        return WizardCartridge::Jpn;
    case RomRelease::Asia:
        return WizardCartridge::Usa;
    }
    return std::nullopt;
}

std::optional<WizardCartridge> IdentifyCartridge(const std::filesystem::path &path) {
    const auto release = IdentifyRomFile(path);
    return release ? WizardRelease(*release) : std::nullopt;
}

std::optional<std::string> ImportFiles(const std::filesystem::path &data,
                                       const WizardImport &files,
                                       const WizardProgress &progress) {
    std::vector<std::filesystem::path> roms;
    for (const auto &[release, path] : files.cartridges) {
        roms.push_back(path);
    }
    return ImportAssetImages(data, files.disc, roms, progress);
}

std::optional<WizardKey> KeyOf(SDL_Keycode key) {
    switch (key) {
    case SDLK_TAB:
        return WizardKey::Tab;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        return WizardKey::Enter;
    case SDLK_SPACE:
        return WizardKey::Space;
    case SDLK_ESCAPE:
        return WizardKey::Escape;
    case SDLK_UP:
        return WizardKey::Up;
    case SDLK_DOWN:
        return WizardKey::Down;
    case SDLK_LEFT:
        return WizardKey::Left;
    case SDLK_RIGHT:
        return WizardKey::Right;
    case SDLK_PAGEUP:
        return WizardKey::PageUp;
    case SDLK_PAGEDOWN:
        return WizardKey::PageDown;
    case SDLK_HOME:
        return WizardKey::Home;
    case SDLK_END:
        return WizardKey::End;
    case SDLK_BACKSPACE:
        return WizardKey::Backspace;
    case SDLK_DELETE:
        return WizardKey::Delete;
    default:
        return std::nullopt;
    }
}

} // namespace

SetupRun RunSetupWizard(GpuWindow &window,
                        GamePaths &paths,
                        GameConfig &config,
                        const Script &script,
                        std::span<const SDL_Scancode> scriptKeys,
                        const std::filesystem::path &captures,
                        bool fixedTime,
                        const std::string &reimportReason,
                        const std::filesystem::path &portableDirectory) {
    SetupRun run;
    std::unique_ptr<SceneRenderer> sceneRenderer = SceneRenderer::Create(window);
    if (!sceneRenderer) {
        return run;
    }
    SceneRenderer &renderer = *sceneRenderer;
    std::unique_ptr<TextFont> font = TextFont::Create(renderer);
    if (!font) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot create setup font.");
        return run;
    }
    WizardServices services;
    services.reimportReason = reimportReason;
#ifdef COASTLINE_GIT_SHA
    services.buildRevision = COASTLINE_GIT_SHA;
#endif
    if (services.buildRevision.empty()) {
        services.buildRevision = "v" COASTLINE_VERSION;
    } else if (!services.buildRevision.starts_with('v')) {
        services.buildRevision.resize(std::min<std::size_t>(7, services.buildRevision.size()));
    }
    services.buildDate = COASTLINE_BUILD_TIMESTAMP;
    services.openUrl = [](const std::string &url) {
        if (!SDL_OpenURL(url.c_str())) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot open browser: %s", SDL_GetError());
        }
    };
    services.checkDisc = CheckDisc;
    services.identifyCartridge = IdentifyCartridge;
    services.preferCartridge = [](const std::filesystem::path &candidate,
                                  const std::filesystem::path &current) {
        return IdentifyRomFile(candidate) == RomRelease::Usa &&
               IdentifyRomFile(current) == RomRelease::Asia;
    };
    services.scanArchive =
        [cache = paths.cache / "asset-search"](
            const std::filesystem::path &archive,
            const std::function<bool(WizardCartridge, const std::filesystem::path &)> &found,
            const WizardProgress &progress) {
            VisitZipRoms(
                archive,
                [&](std::vector<std::uint8_t> &bytes) {
                    if (!NormalizeRomByteOrder(bytes)) {
                        return true;
                    }
                    const auto release = IdentifyRom(bytes);
                    if (!release) {
                        return true;
                    }
                    const auto detected = WizardRelease(*release);
                    if (!detected) {
                        return true;
                    }
                    std::error_code error;
                    std::filesystem::create_directories(cache, error);
                    if (error) {
                        return true;
                    }
                    const auto output =
                        cache /
                        (std::string(*release == RomRelease::Asia ?
                                         "Asia" :
                                         CARTRIDGE_NAMES[static_cast<std::size_t>(*detected)]) +
                         ".z64");
                    std::ofstream file(output, std::ios::binary | std::ios::trunc);
                    file.write(reinterpret_cast<const char *>(bytes.data()),
                               static_cast<std::streamsize>(bytes.size()));
                    file.close();
                    return !file || found(*detected, output);
                },
                progress);
        };
    const std::filesystem::path data = paths.data;
    const bool portable = paths.portableConfig || paths.portableData;
    const bool fullyPortable =
        paths.portableConfig && paths.portableData && paths.config == paths.data;
    const auto restoredRoot = paths.portableConfig ? paths.config : paths.data;
    const auto portableRoot = paths.portableData   ? paths.data :
                              paths.portableConfig ? paths.config :
                                                     portableDirectory;
    services.import = [data, portableRoot](const WizardImport &files,
                                           const WizardProgress &progress) {
        return ImportFiles(files.portable ? portableRoot : data, files, progress);
    };
    if (const char *home = SDL_GetUserFolder(SDL_FOLDER_HOME)) {
        services.home = PathFromUtf8(home);
    }
    services.portable = fullyPortable && !portableDirectory.empty();
    services.portableAvailable =
        PortablePathsEnabled() && !fullyPortable && !portableDirectory.empty();
    const auto homeDirectory = services.home;
    auto restorePath = [&](const std::string &key) {
        auto path = PathFromUtf8(config.Get("SetupWizard", key).value_or(""));
        return portable ? RestorePortablePath(path, restoredRoot, homeDirectory) : path;
    };
    services.lastDirectory = restorePath("LastDirectory");
    services.disc = restorePath("DiscImage");
    for (std::size_t i = 0; i < services.roms.size(); ++i) {
        services.roms[i] = restorePath("RomImage" + std::to_string(i + 1));
    }
    services.searchDirectory = restorePath("SearchDirectory");
    SetupWizard wizard(std::move(services));
    run.failed = false;
    SDL_Log(reimportReason.empty() ? "Missing game data; starting setup." :
                                     "Re-import required; opening setup.");

    SDL_Window *sdlWindow = window.Window();
    SDL_StartTextInput(sdlWindow);
    SDL_HideCursor();
    const Uint64 start = SDL_GetTicksNS();
    Uint64 nextFrame = start;
    bool scriptMouseDown = false;
    std::optional<std::array<int, 2>> scriptMouse;
    bool quit = false;
    bool freeCamera = false;
    bool cameraEscapeHeld = false;
    FreeCamera camera;
    Uint64 lastCameraFrame = start;
    auto cameraKey = [&](SDL_Keycode key) {
        if (key == SDLK_F8) {
            freeCamera = !freeCamera;
            if (freeCamera) {
                camera.Reset();
            }
        } else if (key == SDLK_ESCAPE && freeCamera) {
            freeCamera = false;
            cameraEscapeHeld = true;
        } else {
            return false;
        }
        SDL_SetWindowRelativeMouseMode(sdlWindow, freeCamera);
        return true;
    };
    for (int frame = 1; !quit; ++frame) {
        const Uint64 now = SDL_GetTicksNS();
        std::array<float, 2> cameraMouse{};
        float cameraScroll = 0;
        const std::uint64_t milliseconds =
            fixedTime ? static_cast<std::uint64_t>(frame) * 1000 / 60 : now / 1000000;
        int windowWidth = 1;
        int windowHeight = 1;
        SDL_GetWindowSize(sdlWindow, &windowWidth, &windowHeight);
        const float frameAspect = static_cast<float>(window.FrameWidth()) /
                                  static_cast<float>(std::max(window.FrameHeight(), 1u));
        auto gridPoint = [&](float x, float y) {
            return GridPointInWindow(static_cast<float>(windowWidth),
                                     static_cast<float>(windowHeight),
                                     frameAspect,
                                     x,
                                     y);
        };
        auto mouseEvent = [&](WizardEvent::Type type, std::array<int, 2> at) {
            WizardEvent event;
            event.type = type;
            event.x = at[0];
            event.y = at[1];
            event.milliseconds = milliseconds;
            event.shift = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
            return event;
        };
        // Alt and Enter or F11 switch full screen, Alt and a letter press an accelerator, and
        // Control and V paste clipboard text.
        auto keyDown = [&](SDL_Keycode key, SDL_Keymod mod) {
            if ((mod & SDL_KMOD_CTRL) && key == SDLK_U) {
                window.SetFramerateUnlocked(!window.FramerateUnlocked());
                config.Set("Video", "UnlockedFramerate", window.FramerateUnlocked() ? "1" : "0");
                nextFrame = SDL_GetTicksNS();
                return;
            }
            if (key == SDLK_F11 ||
                ((mod & SDL_KMOD_ALT) && (key == SDLK_RETURN || key == SDLK_KP_ENTER))) {
                bool fullscreen = !window.Fullscreen();
                if (window.SetFullscreen(fullscreen)) {
                    config.Set("Video", "Fullscreen", fullscreen ? "1" : "0");
                }
                return;
            }
            WizardEvent event;
            event.milliseconds = milliseconds;
            event.shift = (mod & SDL_KMOD_SHIFT) != 0;
            const bool command = (mod & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0;
            if (command && (key == SDLK_C || key == SDLK_X)) {
                const auto selected = wizard.SelectedText();
                if (!selected.empty()) {
                    SDL_SetClipboardText(selected.c_str());
                    if (key == SDLK_X) {
                        event.key = WizardKey::Delete;
                        wizard.Handle(event);
                    }
                }
                return;
            }
            if (command && key == SDLK_A) {
                event.key = WizardKey::SelectAll;
            } else if ((command || (mod & SDL_KMOD_ALT)) && key == SDLK_BACKSPACE) {
                event.key = WizardKey::DeleteWord;
            } else if (command && key == SDLK_DELETE) {
                event.key = WizardKey::DeleteNextWord;
            } else if (command && key == SDLK_LEFT) {
                event.key = WizardKey::WordLeft;
            } else if (command && key == SDLK_RIGHT) {
                event.key = WizardKey::WordRight;
            } else if ((mod & SDL_KMOD_ALT) && key >= SDLK_0 && key <= SDLK_9) {
                event.type = WizardEvent::Type::Accelerator;
                event.letter = static_cast<char>('0' + key - SDLK_0);
            } else if ((mod & SDL_KMOD_ALT) && key >= SDLK_A && key <= SDLK_Z) {
                event.type = WizardEvent::Type::Accelerator;
                event.letter = static_cast<char>('a' + (key - SDLK_A));
            } else if (command && key == SDLK_V) {
                char *clipboard = SDL_GetClipboardText();
                event.type = WizardEvent::Type::Text;
                event.text = clipboard ? clipboard : "";
                SDL_free(clipboard);
            } else if (auto wizardKey = KeyOf(key)) {
                event.key = *wizardKey;
                event.shift = (mod & SDL_KMOD_SHIFT) != 0;
            } else {
                return;
            }
            wizard.Handle(event);
        };

        SDL_Event sdlEvent;
        while (SDL_PollEvent(&sdlEvent)) {
            switch (sdlEvent.type) {
            case SDL_EVENT_QUIT:
                quit = true;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (sdlEvent.key.key == SDLK_ESCAPE && cameraEscapeHeld) {
                    break;
                }
                if (!sdlEvent.key.repeat && cameraKey(sdlEvent.key.key)) {
                    break;
                }
                keyDown(sdlEvent.key.key, sdlEvent.key.mod);
                break;
            case SDL_EVENT_KEY_UP:
                if (sdlEvent.key.key == SDLK_ESCAPE) {
                    cameraEscapeHeld = false;
                }
                break;
            case SDL_EVENT_TEXT_INPUT:
                // Alt and a letter is an accelerator, not text.
                if ((SDL_GetModState() & SDL_KMOD_ALT) == 0) {
                    WizardEvent event;
                    event.type = WizardEvent::Type::Text;
                    event.text = sdlEvent.text.text;
                    event.milliseconds = milliseconds;
                    wizard.Handle(event);
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (!freeCamera && script.mouseMoves.empty() &&
                    sdlEvent.button.button == SDL_BUTTON_LEFT) {
                    wizard.Handle(mouseEvent(sdlEvent.button.down ? WizardEvent::Type::MouseDown :
                                                                    WizardEvent::Type::MouseUp,
                                             gridPoint(sdlEvent.button.x, sdlEvent.button.y)));
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                cameraMouse[0] += sdlEvent.motion.xrel;
                cameraMouse[1] += sdlEvent.motion.yrel;
                if (!freeCamera && script.mouseMoves.empty()) {
                    wizard.Handle(mouseEvent(WizardEvent::Type::MouseMove,
                                             gridPoint(sdlEvent.motion.x, sdlEvent.motion.y)));
                }
                break;
            case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                if (script.mouseMoves.empty()) {
                    wizard.Handle(mouseEvent(WizardEvent::Type::MouseMove, {-1, -1}));
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                cameraScroll += sdlEvent.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ?
                                    -sdlEvent.wheel.y :
                                    sdlEvent.wheel.y;
                if (!freeCamera && script.mouseMoves.empty()) {
                    WizardEvent event =
                        mouseEvent(WizardEvent::Type::Wheel,
                                   gridPoint(sdlEvent.wheel.mouse_x, sdlEvent.wheel.mouse_y));
                    event.wheel = static_cast<int>(sdlEvent.wheel.integer_y);
                    wizard.Handle(event);
                }
                break;
            default:
                break;
            }
        }

        // Scripted keys press on their first frame, with modifiers pressed by the script.
        int scriptMod = SDL_KMOD_NONE;
        for (std::size_t i = 0; i < script.keys.size() && i < scriptKeys.size(); ++i) {
            const ScriptKey &key = script.keys[i];
            if (frame >= key.frame && frame - key.frame < key.frames) {
                SDL_Keycode code = SDL_GetKeyFromScancode(scriptKeys[i], SDL_KMOD_NONE, false);
                scriptMod |= code == SDLK_LSHIFT || code == SDLK_RSHIFT ? SDL_KMOD_SHIFT :
                             code == SDLK_LALT || code == SDLK_RALT     ? SDL_KMOD_ALT :
                             code == SDLK_LCTRL || code == SDLK_RCTRL   ? SDL_KMOD_CTRL :
                             code == SDLK_LGUI || code == SDLK_RGUI     ? SDL_KMOD_GUI :
                                                                          SDL_KMOD_NONE;
            }
        }
        for (std::size_t i = 0; i < script.keys.size() && i < scriptKeys.size(); ++i) {
            if (script.keys[i].frame == frame) {
                const auto code = SDL_GetKeyFromScancode(scriptKeys[i], SDL_KMOD_NONE, false);
                if (!cameraKey(code)) {
                    keyDown(code, static_cast<SDL_Keymod>(scriptMod));
                }
            }
        }
        for (const ScriptTyping &typed : script.typing) {
            if (typed.frame != frame) {
                continue;
            }
            WizardEvent event;
            event.milliseconds = milliseconds;
            if (typed.text == "<backspace>") {
                event.key = WizardKey::Backspace;
            } else if (typed.text == "<enter>") {
                event.key = WizardKey::Enter;
            } else {
                event.type = WizardEvent::Type::Text;
                event.text = typed.text;
            }
            wizard.Handle(event);
        }
        // A move of the script places the mouse on the 320 by 240 screen, spread over the grid.
        if (!freeCamera && !script.mouseMoves.empty()) {
            std::optional<std::array<int, 2>> at;
            bool down = false;
            for (const ScriptMouse &move : script.mouseMoves) {
                if (frame >= move.frame && frame - move.frame < move.frames) {
                    at = std::array<int, 2>{move.x * GRID_WIDTH / 320, move.y * GRID_HEIGHT / 240};
                    down = move.button == ScriptMouseButton::Left;
                }
            }
            if (at && at != scriptMouse) {
                wizard.Handle(mouseEvent(WizardEvent::Type::MouseMove, *at));
            }
            if (at) {
                scriptMouse = at;
            }
            if (scriptMouse && down != scriptMouseDown) {
                wizard.Handle(
                    mouseEvent(down ? WizardEvent::Type::MouseDown : WizardEvent::Type::MouseUp,
                               *scriptMouse));
                scriptMouseDown = down;
            }
        }

        if (freeCamera) {
            int count = 0;
            const auto *keys = SDL_GetKeyboardState(&count);
            auto down = [&](SDL_Scancode key) {
                bool held = key < count && keys[key];
                for (std::size_t i = 0; i < script.keys.size() && i < scriptKeys.size(); ++i) {
                    const auto &press = script.keys[i];
                    held |= scriptKeys[i] == key && frame >= press.frame &&
                            frame - press.frame < press.frames;
                }
                return held;
            };
            auto axis = [&](SDL_Scancode positive, SDL_Scancode negative) {
                return static_cast<float>(down(positive)) - static_cast<float>(down(negative));
            };
            const float speed =
                down(SDL_SCANCODE_LSHIFT) || down(SDL_SCANCODE_RSHIFT) ? 200.0f : 20.0f;
            const float seconds =
                fixedTime ? 1.0f / 60 :
                            std::min(static_cast<float>(now - lastCameraFrame) / 1e9f, 0.1f);
            camera.Move(seconds,
                        {axis(SDL_SCANCODE_W, SDL_SCANCODE_S) * speed,
                         axis(SDL_SCANCODE_D, SDL_SCANCODE_A) * speed,
                         axis(SDL_SCANCODE_E, SDL_SCANCODE_Q) * speed},
                        {-cameraMouse[0] * 0.003f, -cameraMouse[1] * 0.003f});
            if (cameraScroll != 0) {
                camera.Move(0.1f, {cameraScroll * speed * 1.5f, 0, 0}, {});
            }
        }
        lastCameraFrame = now;
        renderer.SetFrontendCamera(freeCamera ? std::optional<Matrix4>(camera.View()) :
                                                std::nullopt);
        wizard.Update();
        if (wizard.End() != WizardEnd::Running) {
            run.finished = wizard.End() == WizardEnd::Finished;
            run.frames = frame;
            break;
        }
        std::optional<std::array<int, 2>> pointer = scriptMouse;
        if (script.mouseMoves.empty() && SDL_GetMouseFocus() == sdlWindow) {
            float x = 0.0f;
            float y = 0.0f;
            SDL_GetMouseState(&x, &y);
            pointer = gridPoint(x, y);
        }
        window.UpdateFrameSize();
        renderer.Begin({0.0f, 0.0f, 0.0f, 1.0f});
        TextGrid grid;
        wizard.Draw(grid, fixedTime ? frame / 60.0 : static_cast<double>(now - start) / 1e9);
        font->Draw(renderer, grid, freeCamera ? std::nullopt : pointer);
        std::optional<std::filesystem::path> capture;
        if (std::ranges::contains(script.screenshots, frame) ||
            (script.screenshotEvery > 0 && frame % script.screenshotEvery == 0)) {
            char name[32];
            std::snprintf(name, sizeof(name), "frame-%06d.png", frame);
            capture = captures / name;
        }
        renderer.End(capture ? &*capture : nullptr);
        if (script.quitFrame && frame >= *script.quitFrame) {
            quit = true;
        }
        run.frames = frame;
        // Unless unlocked, limit wizard drawing to 60 frames a second or the slower refresh rate.
        const Uint64 period = window.FramerateUnlocked() ?
                                  0 :
                                  std::max<Uint64>(window.RefreshPeriod(), SDL_NS_PER_SECOND / 60);
        nextFrame += period;
        const Uint64 after = SDL_GetTicksNS();
        if (nextFrame > after) {
            SDL_DelayPrecise(nextFrame - after);
        } else {
            nextFrame = after;
        }
    }
    SDL_SetWindowRelativeMouseMode(sdlWindow, false);
    auto savePath = [&](const std::string &key, const std::string &text) {
        std::string stored = text;
        if ((portable || (run.finished && wizard.Portable())) && !text.empty()) {
            stored = Utf8FromPath(
                StorePortablePath(ResolveUserPath(text, homeDirectory),
                                  run.finished && wizard.Portable() ? portableRoot : restoredRoot,
                                  homeDirectory));
        }
        config.Set("SetupWizard", key, stored);
    };
    if (!wizard.LastDirectory().empty()) {
        savePath("LastDirectory", Utf8FromPath(wizard.LastDirectory()));
    }
    savePath("DiscImage", wizard.Text(WizardControlId::DiscText));
    constexpr std::array romFields{
        WizardControlId::Rom1Text, WizardControlId::Rom2Text, WizardControlId::Rom3Text};
    for (std::size_t i = 0; i < romFields.size(); ++i) {
        savePath("RomImage" + std::to_string(i + 1), wizard.Text(romFields[i]));
    }
    savePath("SearchDirectory", wizard.Text(WizardControlId::SearchText));
    if (run.finished && wizard.Portable()) {
        GamePaths selected{
            portableRoot, portableRoot, portableRoot / "state", portableRoot / "cache", true, true};
        if (!CreateGamePaths(selected) || !config.Save(portableRoot / "BossRally.ini")) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Cannot save the portable installation.");
            run.finished = false;
            run.failed = true;
        } else {
            paths = std::move(selected);
            run.portable = true;
        }
    }
    SDL_StopTextInput(sdlWindow);
    return run;
}

} // namespace coastline
