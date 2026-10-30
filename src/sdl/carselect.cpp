#include <algorithm>
#include <array>
#include <fstream>
#include <string>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "frontend.h"
#include "gpu.h"
#include "menus.h"
#include "menutext.h"

namespace coastline {

namespace {

// The steps of choosing a car.
enum CarSelectState {
    CAR_SELECT_CAR,
    CAR_SELECT_HANDLING,
    CAR_SELECT_TRANSMISSION,
    CAR_SELECT_TIRES,
    CAR_SELECT_SUSPENSION,
    CAR_SELECT_DECALS,
    CAR_SELECT_READY,
};

// The decals, by their options.
enum Decals {
    DECALS_DEFAULT,
    DECALS_LOAD,
    DECALS_CUSTOM,
};

// The scale of an option on a stage.
constexpr float STAGE_MODEL_SCALE = 0.001302083372f;

// The cars the paint shop allows, and the first car whose decals have no custom choice.
constexpr int PAINT_SHOP_CARS = 9;

// The distance the players' views spread the cars apart by when they slide.
constexpr float SPREAD_ONE_VIEW = 8.0f;
constexpr float SPREAD_TWO_VIEWS = 12.0f;

// The size of the bars of the speed and acceleration of a car, and their full values.
constexpr int STAT_BAR_WIDTH = 80;
constexpr int STAT_BAR_HEIGHT = 11;
constexpr float FULL_ACCELERATION = 500.0f;
constexpr float FULL_SPEED = 10.0f;

// The low-resolution screen the layouts of the console are written for.
constexpr int WIDTH = 320;
constexpr int HEIGHT = 240;

// The colours of a progress bar: its frame, its background, and its filling. The fill colour of
// the console sets only the second of the two pixels of each word, the first black, and these are
// the colours of the two pixels together.
constexpr std::array<std::uint8_t, 4> BAR_FRAME_COLOR{0x00, 0x00, 0x00, 0xFF};
constexpr std::array<std::uint8_t, 4> BAR_BACK_COLOR{0x00, 0x1C, 0x00, 0xFF};
constexpr std::array<std::uint8_t, 4> BAR_FILL_COLOR{0x00, 0x7B, 0x00, 0xFF};

// A rectangle of one colour on the 320 by 240 screen, cut at its edges as the RDP cuts it.
void FillRect(SceneRenderer &renderer,
              int left,
              int top,
              int right,
              int bottom,
              const std::array<std::uint8_t, 4> &color) {
    auto x0 = static_cast<float>(std::clamp(left, 0, WIDTH) * 2);
    auto y0 = static_cast<float>(top * 2);
    auto x1 = static_cast<float>(std::clamp(right, 0, WIDTH) * 2);
    auto y1 = static_cast<float>(bottom * 2);
    std::array<FlatVertex, 4> corners{{{x0, y0, 0.0f, 0.0f, color},
                                       {x1, y0, 0.0f, 0.0f, color},
                                       {x1, y1, 0.0f, 0.0f, color},
                                       {x0, y1, 0.0f, 0.0f, color}}};
    renderer.DrawQuads(corners, FlatMaterial{}, MenuScreen::FlatFor(renderer));
}

// DrawProgressBar() of the console: a frame, its background, and the filling to a fraction.
void DrawProgressBar(SceneRenderer &renderer, int x, int y, int width, int height, float fraction) {
    FillRect(renderer, x, y, x + width, y + height, BAR_FRAME_COLOR);
    x += 3;
    width -= 6;
    int bottom = y + height - 3;
    y += 3;
    FillRect(renderer, x, y, x + width, bottom, BAR_BACK_COLOR);
    FillRect(renderer,
             x,
             y,
             x + static_cast<int>(static_cast<float>(width) * fraction),
             bottom,
             BAR_FILL_COLOR);
}

// An option on a stage, turned and placed as DRAW_STAGE_MODEL() of the console places it.
Matrix4 StageOptionAt(float x, float y, float angle) {
    return MultiplyMatrices(
        Translation(x, y, 0.0f),
        MultiplyMatrices(RotationZ(angle),
                         MultiplyMatrices(Scale(STAGE_MODEL_SCALE), RotationX(HALF_PI))));
}

} // namespace

bool Menus::LoadCarSelect(SceneRenderer &renderer, const std::filesystem::path &release) {
    try {
        std::ifstream file(release / "data" / "menus.json");
        auto carSelect = nlohmann::json::parse(file).at("carSelect");
        m_engineLayouts = carSelect.at("engineLayouts").get<std::vector<std::string>>();
        m_driveTypes = carSelect.at("driveTypes").get<std::vector<std::string>>();
        auto read = [&](const char *key, std::vector<MenuItem> &options) {
            for (const auto &record : carSelect.at(key)) {
                MenuItem item;
                item.name = record.at("name").get<std::string>();
                item.flags = record.at("flags").get<u32>();
                if (!record.at("model").is_null()) {
                    item.model = m_screen->LoadModel(
                        renderer, release / record.at("model").get<std::string>());
                    if (!item.model) {
                        return false;
                    }
                }
                options.push_back(std::move(item));
            }
            return !options.empty();
        };
        if (!read("handling", m_handlingOptions) || !read("transmission", m_transmissionOptions) ||
            !read("tires", m_tiresOptions) || !read("suspension", m_suspensionOptions) ||
            !read("decals", m_decalOptions)) {
            return false;
        }
    } catch (const nlohmann::json::exception &error) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s does not have the car select screen: %s",
                     release.string().c_str(),
                     error.what());
        return false;
    }
    m_carModels.resize(m_cars.size());
    return true;
}

MenuProfile &Menus::ProfileOf(int player) {
    return player == 0 ? m_profile : m_secondProfile;
}

bool Menus::IsCarUnlocked(int car) const {
    return (!m_paintMode || car < PAINT_SHOP_CARS) && (m_profile.unlocks & (1 << car)) != 0 &&
           m_cars[static_cast<std::size_t>(car)].exists;
}

// LoadVehicleModel() of the console: the model of a car into a slot. A model read once is read
// again from memory.
void Menus::LoadCarModel(SceneRenderer &renderer, int slot, int car) {
    m_slotCars[static_cast<std::size_t>(slot)] = car;
    auto &model = m_carModels[static_cast<std::size_t>(car)];
    if (!model) {
        model = RenderModel::Load(
            renderer, m_release / m_cars[static_cast<std::size_t>(car)].directory / "model.gltf");
    }
}

void Menus::StartCarSelect(SceneRenderer &renderer) {
    m_screen->FadeIn();
    m_carSelectAngle = {0.0f, 1.0f};
    for (int i = 0; i < m_numLocalPlayers; ++i) {
        CarSelectPlayer &player = m_carSelect[static_cast<std::size_t>(i)];
        player.slot = i;
        player.state = CAR_SELECT_CAR;
        player.slide = 0.0f;
        player.customDecals = false;

        // ChooseVehicleCar() of the console: the first car of the season in a championship.
        if (m_gameMode == GameMode::Championship) {
            player.carModel = m_seasons[m_profile.season].cars[0];
        }
        int car = player.carModel;
        while (!IsCarUnlocked(car)) {
            car = (car + 1) % static_cast<int>(m_cars.size());
        }
        player.carModel = car;
        player.selectedCar = car;
        LoadCarModel(renderer, i, car);
    }
    m_carSelectInitialized = true;
    for (CarSelectPlayer &player : m_carSelect) {
        player.options = &m_handlingOptions;
        player.cursor = 0;
    }
}

void Menus::SetCarSelectOptions(int player,
                                const std::vector<MenuItem> &options,
                                const char *name,
                                int cursor) {
    CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(player)];
    choice.cursor = cursor;
    choice.options = &options;
    choice.optionCount = static_cast<int>(options.size());
    choice.category = name;
}

void Menus::StepCarSelectOption(int player, const MenuButtons &buttons) {
    CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(player)];
    if (buttons.left) {
        choice.prevCursor = choice.cursor;
        choice.cursor = (choice.cursor + choice.optionCount - 1) % choice.optionCount;
        choice.slide = 1.0f;
        m_screen->PlayScrollSound();
    } else if (buttons.right) {
        choice.prevCursor = choice.cursor;
        choice.cursor = (choice.cursor + 1) % choice.optionCount;
        choice.slide = -1.0f;
        m_screen->PlayScrollSound();
    }
}

void Menus::HandleCarSelectInput(SceneRenderer &renderer, int player, const MenuButtons &buttons) {
    CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(player)];
    MenuProfile &profile = ProfileOf(player);
    bool playSound = true;
    if (choice.state == CAR_SELECT_READY) {
        if (buttons.back) {
            --choice.state;
        }
        return;
    }
    if (choice.slide != 0.0f) {
        return;
    }

    // Left and right turn to another car, loaded into the other slot.
    if (choice.state == CAR_SELECT_CAR) {
        int count = static_cast<int>(m_cars.size());
        int previous = choice.selectedCar;
        if (buttons.left) {
            do {
                choice.selectedCar = (choice.selectedCar + count - 1) % count;
            } while (!IsCarUnlocked(choice.selectedCar));
        }
        if (previous != choice.selectedCar) {
            choice.slide = 1.0f;
        } else if (buttons.right) {
            do {
                choice.selectedCar = (choice.selectedCar + 1) % count;
            } while (!IsCarUnlocked(choice.selectedCar));
            if (previous != choice.selectedCar) {
                choice.slide = -1.0f;
            }
        }
        if (previous != choice.selectedCar) {
            m_screen->PlayScrollSound();
            choice.slot ^= 2;
            LoadCarModel(renderer, choice.slot, choice.selectedCar);
        }
    }

    // ponytail: the other look on C-down and the paint on up and down with the C buttons held are
    // not read yet. The other look needs an unlock a new profile does not have.

    // Left and right move through the options of the other steps.
    if (choice.state != CAR_SELECT_CAR) {
        StepCarSelectOption(player, buttons);
    }

    if (buttons.back) {
        // B exits the screen from the car, and steps back from the options.
        if (choice.state == CAR_SELECT_CAR) {
            m_carSelectDone = false;
            m_screen->FadeOut();
        } else {
            switch (--choice.state) {
            case CAR_SELECT_HANDLING:
                SetCarSelectOptions(player, m_handlingOptions, "Handling", profile.handling);
                break;
            case CAR_SELECT_TRANSMISSION:
                SetCarSelectOptions(
                    player, m_transmissionOptions, "Transmission", profile.transmission);
                break;
            case CAR_SELECT_TIRES:
                SetCarSelectOptions(player, m_tiresOptions, "Tires", profile.tires);
                break;
            case CAR_SELECT_SUSPENSION:
                SetCarSelectOptions(player, m_suspensionOptions, "Suspension", profile.suspension);
                break;
            case CAR_SELECT_DECALS:
                // The options stay those of the suspension.
                choice.cursor = 0;
                choice.category = "Decals";
                break;
            default:
                break;
            }
            choice.prevCursor = choice.cursor;
        }
    } else if (buttons.choose) {
        bool loadDecals = false;
        switch (choice.state) {
        case CAR_SELECT_CAR:
            choice.carModel = choice.selectedCar;
            SetCarSelectOptions(player, m_handlingOptions, "Handling", profile.handling);
            break;
        case CAR_SELECT_HANDLING:
            profile.handling = choice.cursor;
            SetCarSelectOptions(
                player, m_transmissionOptions, "Transmission", profile.transmission);
            break;
        case CAR_SELECT_TRANSMISSION:
            profile.transmission = choice.cursor;
            SetCarSelectOptions(player, m_tiresOptions, "Tires", profile.tires);
            break;
        case CAR_SELECT_TIRES:
            profile.tires = choice.cursor;
            SetCarSelectOptions(player, m_suspensionOptions, "Suspension", profile.suspension);
            break;
        case CAR_SELECT_SUSPENSION:
            // The decals show the car, loaded at once into both slots.
            profile.suspension = choice.cursor;
            choice.cursor = 0;
            choice.options = &m_decalOptions;
            choice.optionCount = static_cast<int>(m_decalOptions.size()) -
                                 (choice.carModel < PAINT_SHOP_CARS ? 1 : 2);
            choice.category = "Decals";
            choice.slot = player ^ 2;
            m_screen->PlaySelectSound();
            playSound = false;
            LoadCarModel(renderer, choice.slot, choice.carModel);
            LoadCarModel(renderer, choice.slot ^ 2, choice.carModel);
            break;
        case CAR_SELECT_DECALS:
            if (choice.cursor == DECALS_LOAD) {
                loadDecals = true;
                break;
            }
            choice.customDecals = choice.cursor == DECALS_CUSTOM;
            break;
        default:
            break;
        }
        if (loadDecals) {
            // ponytail: the dialog of the controller pak is not built yet, and the load is
            // cancelled at once.
            SDL_Log("The controller pak dialog is not built yet.");
        } else {
            choice.prevCursor = choice.cursor;

            // Start and the paint shop skip the rest.
            if (buttons.start || m_paintMode) {
                choice.optionCount = static_cast<int>(m_decalOptions.size()) - 1;
                choice.cursor = 0;
                choice.prevCursor = 0;
                choice.state = CAR_SELECT_READY;
            } else {
                ++choice.state;
            }

            // Once every player is ready, the screen fades out.
            bool ready = true;
            for (int i = 0; i < m_numLocalPlayers; ++i) {
                ready = ready && m_carSelect[static_cast<std::size_t>(i)].state == CAR_SELECT_READY;
            }
            if (ready) {
                // The console fades the music out here for the race. The music of the menus
                // plays on until a race starts.
                m_carSelectDone = true;
                m_screen->FadeOut();
            }
        }
    } else {
        return;
    }
    if (playSound) {
        m_screen->PlaySelectSound();
    }
}

// RenderCameraView() of the console for the stage of a player: the car or the option under the
// cursor, and while they slide, the one the cursor departed coming in from the far side.
void Menus::DrawPlayerStage(SceneRenderer &renderer, int player, int y, int height, float spread) {
    const CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(player)];
    auto optionModel = [&](int option) -> const RenderModel * {
        if (choice.state == CAR_SELECT_CAR || choice.state == CAR_SELECT_READY) {
            return nullptr;
        }
        return (*choice.options)[static_cast<std::size_t>(option)].model;
    };
    const RenderModel *first = optionModel(choice.cursor);
    const RenderModel *second = optionModel(choice.prevCursor);
    float angle = m_carSelectAngle[static_cast<std::size_t>(player)];
    GpuWindow &window = renderer.Window();
    float aspect = static_cast<float>(window.FrameWidth()) /
                   static_cast<float>(std::max<Uint32>(window.FrameHeight(), 1));
    std::array<float, 3> eye = m_numLocalPlayers == 2 ? std::array<float, 3>{15.0f, 15.0f, 7.5f} :
                                                        std::array<float, 3>{10.0f, 10.0f, 5.0f};
    Matrix4 camera = StageCamera(aspect, eye, 0, y, WIDTH, height);
    SDL_Rect scissor = FrameRect(
        window.FrameWidth(), window.FrameHeight(), 0, y, WIDTH, std::min(height, HEIGHT - y));
    const Lighting &lighting = m_screen->MenuLighting();
    auto draw = [&](const RenderModel *option, int slot, float x, float along) {
        if (option) {
            renderer.DrawModel(*option,
                               StageOptionAt(x, along, angle),
                               camera,
                               lighting,
                               std::nullopt,
                               true,
                               scissor);
            return;
        }
        const auto &car =
            m_carModels[static_cast<std::size_t>(m_slotCars[static_cast<std::size_t>(slot)])];
        if (car) {
            renderer.DrawModel(
                *car, StageCarAt(x, along, angle), camera, lighting, std::nullopt, true, scissor);
        }
    };

    // The cars slide apart by the square of the slide.
    float offset = choice.slide < 0.0f ? choice.slide * choice.slide * -spread :
                                         choice.slide * choice.slide * spread;
    draw(first, choice.slot, offset, -offset);
    if (offset == 0.0f) {
        return;
    }
    float far = 0.0f < offset ? offset - spread : offset + spread;
    draw(second, choice.slot ^ 2, far, -far);
}

void Menus::DrawCarStats(SceneRenderer &renderer, int player, int y, int barY) {
    const MenuCar &car =
        m_cars[static_cast<std::size_t>(m_carSelect[static_cast<std::size_t>(player)].selectedCar)];
    Matrix4 flat = MenuScreen::FlatFor(renderer);
    const MenuText &text = m_screen->Text();
    TextStyle style;
    style.size = 8;
    style.colors = m_textCustomColors;
    style.align = TextStyle::Align::Right;
    text.Draw(renderer, "%wwAcceleration", WIDTH / 4 - 5, y + 2, style, flat);
    text.Draw(renderer, "%wwMax Speed", WIDTH / 4 - 5, y - 6, style, flat);
    style.align = TextStyle::Align::Center;
    text.Draw(renderer,
              m_engineLayouts[static_cast<std::size_t>(car.engineLayout)],
              WIDTH * 3 / 4,
              y + 2,
              style,
              flat);
    text.Draw(renderer,
              "%ww" + std::to_string(car.gearCount) + "-Speed " +
                  m_driveTypes[static_cast<std::size_t>(car.driveType)],
              WIDTH * 3 / 4,
              y - 6,
              style,
              flat);
    DrawProgressBar(renderer,
                    WIDTH / 2,
                    barY - 23,
                    STAT_BAR_WIDTH,
                    STAT_BAR_HEIGHT,
                    car.acceleration / FULL_ACCELERATION);
    DrawProgressBar(
        renderer, WIDTH / 2, barY - 6, STAT_BAR_WIDTH, STAT_BAR_HEIGHT, car.maxSpeed / FULL_SPEED);
}

void Menus::DrawCarSelectChoices(SceneRenderer &renderer) {
    Matrix4 flat = MenuScreen::FlatFor(renderer);
    const MenuText &text = m_screen->Text();
    TextStyle style;
    style.align = TextStyle::Align::Center;
    style.colors = m_textCustomColors;

    // What a player is choosing: the name of the car, ready, or the step and its option.
    auto choiceText = [&](int player, const char *ready) {
        const CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(player)];
        if (choice.state == CAR_SELECT_CAR) {
            return "%ry%i" + m_cars[static_cast<std::size_t>(choice.selectedCar)].name;
        }
        if (choice.state == CAR_SELECT_READY) {
            return std::string(ready);
        }
        return "%ry" + choice.category + ": %i" +
               (*choice.options)[static_cast<std::size_t>(choice.cursor)].name;
    };
    auto decalsNote = [&](int player) {
        const CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(player)];
        return choice.state == CAR_SELECT_DECALS && choice.cursor != DECALS_LOAD &&
               choice.slide == 0.0f;
    };

    if (m_numLocalPlayers == 2) {
        style.size = 17;
        text.Draw(renderer, choiceText(0, "%ryREADY!"), WIDTH / 2, HEIGHT / 4, style, flat);
        text.Draw(renderer, choiceText(1, "%ryREADY!"), WIDTH / 2, HEIGHT * 3 / 5, style, flat);
        style.size = 8;
        style.align = TextStyle::Align::Left;
        for (int player = 0; player < 2; ++player) {
            if (!decalsNote(player)) {
                continue;
            }
            int y = player == 0 ? HEIGHT / 4 : HEIGHT * 3 / 5;
            int x = WIDTH * 11 / 16;
            text.Draw(renderer, "%wwIf you wish to use", x, y + 19, style, flat);
            text.Draw(renderer, "%wwthe Rumble Pak, you", x, y + 28, style, flat);
            text.Draw(renderer, "%wwshould ensure that it", x, y + 37, style, flat);
            text.Draw(renderer, "%wwis plugged in before", x, y + 45, style, flat);
            text.Draw(renderer, "%wwpressing A.", x, y + 54, style, flat);
        }
        if (m_carSelect[0].state == CAR_SELECT_CAR) {
            DrawCarStats(renderer, 0, HEIGHT / 4, HEIGHT / 2);
        }
        if (m_carSelect[1].state == CAR_SELECT_CAR) {
            DrawCarStats(renderer, 1, HEIGHT * 3 / 5, HEIGHT * 3 * 2 / 5);
        }
        return;
    }

    style.size = 20;
    if (m_paintMode) {
        text.Draw(renderer,
                  "%ry%i" + m_cars[static_cast<std::size_t>(m_carSelect[0].selectedCar)].name,
                  WIDTH / 2,
                  HEIGHT * 22 / 64,
                  style,
                  flat);
        return;
    }
    text.Draw(renderer, choiceText(0, "%ryLET'S GO!"), WIDTH / 2, HEIGHT * 20 / 64, style, flat);
    style.size = 8;
    if (decalsNote(0)) {
        text.Draw(renderer,
                  "%wwIf you wish to use the Rumble Pak, you should",
                  WIDTH / 2,
                  HEIGHT * 3 / 8 - 6,
                  style,
                  flat);
        text.Draw(renderer,
                  "%wwensure that it is plugged in before pressing A.",
                  WIDTH / 2,
                  HEIGHT * 3 / 8 + 2,
                  style,
                  flat);
    }
    if (m_carSelect[0].state != CAR_SELECT_CAR) {
        return;
    }
    const MenuCar &car = m_cars[static_cast<std::size_t>(m_carSelect[0].selectedCar)];
    style.align = TextStyle::Align::Right;
    text.Draw(renderer, "%wwMax Speed", WIDTH * 3 / 8, HEIGHT * 3 / 8 - 6, style, flat);
    text.Draw(renderer, "%wwAcceleration", WIDTH * 3 / 8, HEIGHT * 3 / 8 + 2, style, flat);
    style.align = TextStyle::Align::Center;
    text.Draw(renderer,
              m_engineLayouts[static_cast<std::size_t>(car.engineLayout)],
              WIDTH * 11 / 16,
              HEIGHT * 3 / 8 - 6,
              style,
              flat);
    text.Draw(renderer,
              "%ww" + std::to_string(car.gearCount) + "-Speed " +
                  m_driveTypes[static_cast<std::size_t>(car.driveType)],
              WIDTH * 11 / 16,
              HEIGHT * 3 / 8 + 2,
              style,
              flat);
    DrawProgressBar(renderer,
                    WIDTH * 3 / 4 + 10,
                    HEIGHT * 3 / 4 - 23,
                    STAT_BAR_WIDTH,
                    STAT_BAR_HEIGHT,
                    car.acceleration / FULL_ACCELERATION);
    DrawProgressBar(renderer,
                    WIDTH * 3 / 4 + 10,
                    HEIGHT * 3 / 4 - 6,
                    STAT_BAR_WIDTH,
                    STAT_BAR_HEIGHT,
                    car.maxSpeed / FULL_SPEED);
}

void Menus::LeaveCarSelect() {
    m_carSelectInitialized = false;
    MenuScreen::SilenceSounds();
    if (m_carSelectDone) {
        // ponytail: the paint shop and the race after the car select screen are not built yet,
        // and the menus return to the main menu.
        SDL_Log(m_paintMode ? "The paint shop screen is not built yet." :
                              "The race after the car select screen is not built yet.");
        SetCurrentScreen(Screen::MainMenu);
    } else if (m_paintMode) {
        SetCurrentScreen(Screen::MainMenu);
    } else if (m_gameMode == GameMode::Championship || m_gameMode == GameMode::TimeAttack) {
        m_continueFromResults = true;
        SetCurrentScreen(Screen::TrackMenu);
    } else {
        SetCurrentScreen(Screen::WeatherSelect);
    }
}

void Menus::ProcessCarSelectScreen(SceneRenderer &renderer,
                                   float seconds,
                                   const MenuButtons &first) {
    if (!m_carSelectInitialized) {
        StartCarSelect(renderer);
    }
    m_screen->UpdateFade(seconds);
    m_screen->DrawBackground(renderer, {0x00, 0x00, 0x00}, {0x50, 0x00, 0x00});

    // The stages turn, the second the other way, and the cars slide to rest.
    m_carSelectAngle[0] += seconds;
    m_carSelectAngle[1] += TWO_PI - seconds;
    for (int player = 0; player < m_numLocalPlayers; ++player) {
        while (TWO_PI < m_carSelectAngle[static_cast<std::size_t>(player)]) {
            m_carSelectAngle[static_cast<std::size_t>(player)] -= TWO_PI;
        }
    }
    for (int player = 0; player < m_numLocalPlayers; ++player) {
        CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(player)];
        if (choice.slide < 0.0f) {
            choice.slide = std::min(choice.slide + seconds + seconds, 0.0f);
        } else if (0.0f < choice.slide) {
            choice.slide = std::max(choice.slide - (seconds + seconds), 0.0f);
        }
    }

    // The slots of the cars shown when ready, and with the decals.
    for (int player = 0; player < m_numLocalPlayers; ++player) {
        CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(player)];
        if (choice.state == CAR_SELECT_READY) {
            if (!m_paintMode) {
                choice.slot = (static_cast<int>(choice.customDecals) << 1) ^ (player ^ 2);
            }
        } else if (choice.state == CAR_SELECT_DECALS) {
            if (choice.cursor == DECALS_DEFAULT) {
                choice.slot = player ^ 2;
            } else if (choice.cursor == DECALS_CUSTOM) {
                choice.slot = player;
            } else if (choice.cursor == DECALS_LOAD) {
                choice.slot = choice.prevCursor != 0 ? player ^ 2 : player;
            }
        }
    }

    if (m_numLocalPlayers == 1) {
        DrawPlayerStage(renderer, 0, 45, HEIGHT, SPREAD_ONE_VIEW);
    } else {
        DrawPlayerStage(renderer, 0, 0, HEIGHT - 24, SPREAD_TWO_VIEWS);
        DrawPlayerStage(renderer, 1, 71, HEIGHT, SPREAD_TWO_VIEWS);
    }

    // The title, redrawn every frame.
    MenuScreen::DrawTexture(renderer, m_screen->TitleBar(), 22, 9, 280, 42);
    TextStyle title;
    title.size = 30;
    title.align = TextStyle::Align::Center;
    title.colors = m_textCustomColors;
    m_screen->Text().Draw(renderer,
                          m_paintMode ? "%ryPaint car select" : "%ryCar Select",
                          WIDTH / 2,
                          HEIGHT / 6 - 2,
                          title,
                          MenuScreen::FlatFor(renderer));
    DrawCarSelectChoices(renderer);
    m_screen->DrawFooter(renderer);
    m_screen->DrawFade(renderer);

    if (m_screen->FadingOut()) {
        if (m_screen->FadeComplete()) {
            LeaveCarSelect();
        }
        return;
    }
    // ponytail: the menus read the first controller alone, and the second player has no input
    // until they read the second.
    for (int player = 0; player < m_numLocalPlayers; ++player) {
        HandleCarSelectInput(renderer, player, player == 0 ? first : MenuButtons{});
    }
}

} // namespace coastline
