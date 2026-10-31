#include "wizardrun.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "assetpack.h"
#include "importer.h"
#include "importversion.h"
#include "scenerenderer.h"
#include "setupwizard.h"
#include "textfont.h"

namespace coastline {

namespace {

// The disc counts as this many cartridges in the progress of an import, as it takes longer.
constexpr double DISC_SHARE = 4.0;

constexpr std::array<std::string_view, WIZARD_CARTRIDGES> CARTRIDGE_NAMES{
    "USA", "European", "Japanese"};
constexpr std::array<ImportSource, WIZARD_CARTRIDGES> CARTRIDGE_SOURCES{
    ImportSource::Usa, ImportSource::Eur, ImportSource::Jpn};

std::optional<WizardCartridge> IdentifyCartridge(const std::filesystem::path &path) {
    auto release = IdentifyRomFile(path);
    if (!release) {
        return std::nullopt;
    }
    switch (*release) {
    case RomRelease::Usa:
        return WizardCartridge::Usa;
    case RomRelease::Eur:
        return WizardCartridge::Eur;
    case RomRelease::Jpn:
        return WizardCartridge::Jpn;
    }
    return std::nullopt;
}

std::optional<std::string> ImportFiles(const std::filesystem::path &data,
                                       const WizardImport &files,
                                       const WizardProgress &progress) {
    const std::string cancelled = "The import was cancelled.";
    for (std::string_view pack : {"default", "boss", "eur", "jpn"}) {
        std::error_code error;
        std::filesystem::remove_all(data / "assets" / pack, error);
        if (error) {
            return Utf8FromPath(data / "assets" / pack) + " cannot be deleted: " + error.message();
        }
    }
    const std::filesystem::path defaults = DefaultAssets(data);
    auto record = [&](ImportSource source) -> std::optional<std::string> {
        if (!RecordImport(defaults, source)) {
            return Utf8FromPath(defaults / "import.ini") + " cannot be written.";
        }
        return std::nullopt;
    };
    const double total = static_cast<double>(files.cartridges.size()) + DISC_SHARE;
    double done = 0.0;
    for (const auto &[release, path] : files.cartridges) {
        auto index = static_cast<std::size_t>(release);
        if (!progress("The " + std::string(CARTRIDGE_NAMES[index]) + " ROM image", done / total)) {
            return cancelled;
        }
        if (auto imported = ImportRom(path,
                                      defaults,
                                      [&](const std::string &step, double fraction) {
                                          return progress(step, (done + fraction) / total);
                                      });
            !imported) {
            return imported.error();
        }
        if (auto failure = record(CARTRIDGE_SOURCES[index])) {
            return failure;
        }
        SDL_Log("Imported the %s ROM image from %s.",
                CARTRIDGE_NAMES[index].data(),
                Utf8FromPath(path).c_str());
        done += 1.0;
    }
    auto written = ImportDisc(files.disc, defaults, [&](const std::string &step, double fraction) {
        return progress("The disc: " + step, (done + fraction * DISC_SHARE) / total);
    });
    if (!written) {
        return written.error();
    }
    SDL_Log("Imported %zu files from %s.", *written, Utf8FromPath(files.disc).c_str());
    if (auto failure = record(ImportSource::Disc)) {
        return failure;
    }
    if (!progress("The branded images", 1.0)) {
        return cancelled;
    }
    if (auto failure = CurateAssetPacks(defaults)) {
        return failure;
    }
    if (!RecordImportFiles(defaults)) {
        return "The inventory of imported files cannot be written.";
    }
    return std::nullopt;
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
                        const GamePaths &paths,
                        GameConfig &config,
                        const Script &script,
                        std::span<const SDL_Scancode> scriptKeys,
                        const std::filesystem::path &captures,
                        bool fixedTime) {
    SetupRun run;
    std::unique_ptr<SceneRenderer> sceneRenderer = SceneRenderer::Create(window);
    if (!sceneRenderer) {
        return run;
    }
    SceneRenderer &renderer = *sceneRenderer;
    std::unique_ptr<TextFont> font = TextFont::Create(renderer);
    if (!font) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "The font of the setup wizard cannot be made.");
        return run;
    }
    WizardServices services;
    services.checkDisc = CheckDisc;
    services.identifyCartridge = IdentifyCartridge;
    const std::filesystem::path data = paths.data;
    services.import = [data](const WizardImport &files, const WizardProgress &progress) {
        return ImportFiles(data, files, progress);
    };
    if (const char *home = SDL_GetUserFolder(SDL_FOLDER_HOME)) {
        services.home = PathFromUtf8(home);
    }
    if (auto last = config.Get("SetupWizard", "LastDirectory")) {
        services.lastDirectory = PathFromUtf8(*last);
    }
    SetupWizard wizard(std::move(services));
    run.failed = false;
    SDL_Log("The game data is not imported, and the setup wizard starts.");

    SDL_Window *sdlWindow = window.Window();
    SDL_StartTextInput(sdlWindow);
    SDL_HideCursor();
    const Uint64 start = SDL_GetTicksNS();
    Uint64 nextFrame = start;
    bool scriptMouseDown = false;
    std::optional<std::array<int, 2>> scriptMouse;
    bool quit = false;
    for (int frame = 1; !quit; ++frame) {
        const Uint64 now = SDL_GetTicksNS();
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
                keyDown(sdlEvent.key.key, sdlEvent.key.mod);
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
                if (script.mouseMoves.empty() && sdlEvent.button.button == SDL_BUTTON_LEFT) {
                    wizard.Handle(mouseEvent(sdlEvent.button.down ? WizardEvent::Type::MouseDown :
                                                                    WizardEvent::Type::MouseUp,
                                             gridPoint(sdlEvent.button.x, sdlEvent.button.y)));
                }
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (script.mouseMoves.empty()) {
                    wizard.Handle(mouseEvent(WizardEvent::Type::MouseMove,
                                             gridPoint(sdlEvent.motion.x, sdlEvent.motion.y)));
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                if (script.mouseMoves.empty()) {
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
            if (frame >= key.frame && frame < key.frame + key.frames) {
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
                keyDown(SDL_GetKeyFromScancode(scriptKeys[i], SDL_KMOD_NONE, false),
                        static_cast<SDL_Keymod>(scriptMod));
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
        if (!script.mouseMoves.empty()) {
            std::optional<std::array<int, 2>> at;
            bool down = false;
            for (const ScriptMouse &move : script.mouseMoves) {
                if (frame >= move.frame && frame < move.frame + move.frames) {
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
        font->Draw(renderer, grid, pointer);
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
        // The wizard draws at most 60 frames a second, or at the refresh it is paced to.
        const Uint64 period = std::max<Uint64>(window.RefreshPeriod(), SDL_NS_PER_SECOND / 60);
        nextFrame += period;
        const Uint64 after = SDL_GetTicksNS();
        if (nextFrame > after) {
            SDL_DelayPrecise(nextFrame - after);
        } else {
            nextFrame = after;
        }
    }
    if (!wizard.LastDirectory().empty()) {
        config.Set("SetupWizard", "LastDirectory", Utf8FromPath(wizard.LastDirectory()));
    }
    SDL_StopTextInput(sdlWindow);
    SDL_ShowCursor();
    return run;
}

} // namespace coastline
