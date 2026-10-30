#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "common/vehicle.h"
#include "frontend.h"
#include "gpu.h"
#include "input.h"
#include "menus.h"
#include "menutext.h"
#include "stringresources.h"

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

// The frames of the console each second. FlushDisplayList() waits for a retrace of the 60 of NTSC
// before and after it swaps the frame buffers, and the paint steps once a frame.
constexpr int CONSOLE_FRAME_RATE = 30;
constexpr float CONSOLE_FRAME_SECONDS = 1.0f / CONSOLE_FRAME_RATE;

// UpdateStickRepeat() of the console: the stick presses past 0.6 of its range of 70, releases
// below 0.3, and presses again after 500 milliseconds and then every 200.
constexpr float STICK_RANGE = 70.0f;
constexpr float STICK_PRESS = 0.6f;
constexpr float STICK_RELEASE = 0.3f;
constexpr int STICK_FIRST_DELAY = 500;
constexpr int STICK_REPEAT_DELAY = 200;

// CheckAnalogStickPositive() of the console for an axis turned to the direction it presses:
// whether the stick presses this frame.
bool StepStickRepeat(int &timer, float axis, int frameMs) {
    if (timer == 0) {
        if (axis > STICK_PRESS) {
            timer = STICK_FIRST_DELAY;
            return true;
        }
        return false;
    }
    if (axis < STICK_RELEASE) {
        timer = 0;
        return false;
    }
    // A positive time counts down to a press, and a negative one counts up to it.
    bool pressed = false;
    if (timer > 0) {
        timer -= frameMs;
        pressed = timer <= 0;
    } else {
        timer += frameMs;
        pressed = timer >= 0;
    }
    if (pressed) {
        timer = -STICK_REPEAT_DELAY;
    }
    return pressed;
}

// The parts of a car on a stage: the other look does not draw the textured parts.
std::function<bool(const std::string &)> StagePartsOf(bool otherLook) {
    return [otherLook](const std::string &name) {
        return !(otherLook && name == "lod0-textured") &&
               std::ranges::find(STAGE_CAR_PARTS, name) != STAGE_CAR_PARTS.end();
    };
}

// The low-resolution screen the layouts of the console are written for.
constexpr int WIDTH = 320;
constexpr int HEIGHT = 240;

// The colours of a progress bar: its frame, its background, and its filling. The fill colour of
// the console sets only the second of the two pixels of each word, the first black, and these are
// the colours of the two pixels together.
constexpr std::array<std::uint8_t, 4> BAR_FRAME_COLOR{0x00, 0x00, 0x00, 0xFF};
constexpr std::array<std::uint8_t, 4> BAR_BACK_COLOR{0x00, 0x1C, 0x00, 0xFF};
constexpr std::array<std::uint8_t, 4> BAR_FILL_COLOR{0x00, 0x7B, 0x00, 0xFF};

// A rectangle of one colour, cut at its edges as the RDP cuts it. The console fills it in pixels of
// the 640 by 480 frame buffer, not in the 320 by 240 units of the layouts, and the fill mode covers
// the right and bottom edges as well.
void FillRect(SceneRenderer &renderer,
              int left,
              int top,
              int right,
              int bottom,
              const std::array<std::uint8_t, 4> &color) {
    auto x0 = static_cast<float>(std::clamp(left, 0, WIDTH * 2));
    auto y0 = static_cast<float>(top);
    auto x1 = static_cast<float>(std::clamp(right + 1, 0, WIDTH * 2));
    auto y1 = static_cast<float>(bottom + 1);
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

// The points of the fan of the disc of the stage, the turn between them, its radius, and the scale
// and the turn of g_CarStageMtx.
constexpr int STAGE_DISC_POINTS = 31;
constexpr float STAGE_DISC_STEP = 0.2094395161f;
constexpr float STAGE_DISC_RADIUS = 256.0f;
constexpr float STAGE_DISC_SCALE = 0.009765625f;
constexpr float STAGE_DISC_TURN = -45.0f * PI / 180.0f;

// The units of the shadow of a car for each unit of its size, and its drop below the car.
constexpr float SHADOW_UNITS = 256.0f;
constexpr float SHADOW_DROP = 0.18f;

// Each axis of the unit direction of the lights of a stage.
constexpr float STAGE_LIGHT_AXIS = 0.5773502692f;

// LIGHT_STAGE_MODEL() of the console lights an option with an ambient light of 64 and the sum of
// four white lights from (112, 112, 112).
const Lighting STAGE_MODEL_LIGHTING{{STAGE_LIGHT_AXIS, STAGE_LIGHT_AXIS, STAGE_LIGHT_AXIS},
                                    {64.0f / 255.0f, 64.0f / 255.0f, 64.0f / 255.0f},
                                    {4.0f, 4.0f, 4.0f}};

// The shaded triangles of a display list of the stage, from a record of data/car-stage.json.
StageMesh ReadStageMesh(const nlohmann::json &record) {
    StageMesh mesh;
    for (const auto &vertex : record.at("vertices")) {
        mesh.vertices.push_back({vertex.at("position").get<std::array<float, 3>>(),
                                 vertex.at("color").get<std::array<std::uint8_t, 4>>()});
    }
    mesh.triangles = record.at("triangles").get<std::vector<std::array<int, 3>>>();
    return mesh;
}

// The triangles of a mesh of the stage as flat quads.
std::vector<FlatVertex> StageQuads(const StageMesh &mesh) {
    std::vector<FlatVertex> quads;
    for (const auto &triangle : mesh.triangles) {
        AddTriangle(quads,
                    mesh.vertices.at(static_cast<std::size_t>(triangle[0])),
                    mesh.vertices.at(static_cast<std::size_t>(triangle[1])),
                    mesh.vertices.at(static_cast<std::size_t>(triangle[2])));
    }
    return quads;
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
        std::ifstream stageFile(release / "data" / "car-stage.json");
        auto stage = nlohmann::json::parse(stageFile);
        m_stageChoiceMarker = ReadStageMesh(stage.at("choiceMarker"));
        m_stageShadow = ReadStageMesh(stage.at("shadow"));
    } catch (const nlohmann::json::exception &error) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s does not have the car select screen: %s",
                     release.string().c_str(),
                     error.what());
        return false;
    }
    for (auto &models : m_carModels) {
        models.resize(m_cars.size());
    }
    for (auto &looks : m_carModelLooks) {
        looks.resize(m_cars.size());
    }
    for (auto &models : m_carHighlightModels) {
        models.resize(m_cars.size());
    }
    for (int player = 0; player < 2; ++player) {
        ResetCarPaint(player);
    }
    // The reflection wraps across the sky, as its tile wraps.
    m_stageReflection =
        RenderTexture::Load(renderer, release / "data" / "menu-reflection.png", true, true);
    return m_stageReflection != nullptr;
}

MenuProfile &Menus::ProfileOf(int player) {
    return player == 0 ? m_profile : m_secondProfile;
}

bool Menus::IsCarUnlocked(int car) const {
    return (!m_paintMode || car < PAINT_SHOP_CARS) && (m_profile.unlocks & (1 << car)) != 0 &&
           m_cars[static_cast<std::size_t>(car)].exists;
}

// LoadVehicleModel() of the console: the model of a car into a slot. A model read once is read
// again from memory. Vehicle 0 draws slots 0 and 2, and vehicle 1 slots 1 and 3.
// SetVehicleModelSlot() paints the model of each draw in the paint of its player, and
// RenderVehicleModel() draws it in the look of the player.
void Menus::LoadCarModel(SceneRenderer &renderer, int slot, int car) {
    m_slotCars[static_cast<std::size_t>(slot)] = car;
    std::size_t vehicle = static_cast<std::size_t>(slot & 1);
    const CarSelectPlayer &player = m_carSelect[vehicle];
    std::array<int, 4> look{
        player.paint[0] >> 3, player.paint[1] >> 3, player.paint[2] >> 3, player.otherLook};
    auto &model = m_carModels[vehicle][static_cast<std::size_t>(car)];
    auto &loaded = m_carModelLooks[vehicle][static_cast<std::size_t>(car)];
    if (model && loaded == look) {
        return;
    }
    auto file = m_release / m_cars[static_cast<std::size_t>(car)].directory / "model.gltf";
    ModelOptions options;
    options.paint = player.paint;
    bool otherLook = player.otherLook;
    options.parts = StagePartsOf(otherLook);
    m_retiredCarModels.push_back(std::move(model));
    model = RenderModel::Load(renderer, file, options);
    // The other look draws the highlighted parts with the sky reflected, as the painted body. They
    // are a node outside the scene, drawn without the turn of its root.
    auto &highlighted = m_carHighlightModels[vehicle][static_cast<std::size_t>(car)];
    m_retiredCarModels.push_back(std::move(highlighted));
    if (otherLook) {
        ModelOptions highlightOptions;
        highlightOptions.node = "lod0-highlighted";
        highlightOptions.optional = true;
        highlightOptions.reflects = true;
        highlighted = RenderModel::Load(renderer, file, highlightOptions);
    }
    loaded = look;
}

// InitVehicleColor() of the console: the car of a player in the colour of the player, in its usual
// look.
void Menus::ResetCarPaint(int player) {
    CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(player)];
    choice.paint = {g_abPlayerColorsRgb[player][0],
                    g_abPlayerColorsRgb[player][1],
                    g_abPlayerColorsRgb[player][2]};
    choice.otherLook = false;
}

// The car and the decals steps of HandleCarSelectInput() of the console: C-down switches the look
// once the unlock of the mirrored cars allows it, and up or down raises or lowers by 4 the red with
// C-left, the green with C-up, and the blue with C-right held. The pad presses up and down each
// frame of the console it is held, and the stick as UpdateStickRepeat() repeats it. Both slots of
// the player show the change.
void Menus::AdjustCarPaint(SceneRenderer &renderer,
                           int player,
                           const MenuButtons &buttons,
                           std::array<bool, 2> stick) {
    // The unlock of the profile that "Bonus: Mirrored Cars Added!" reports.
    constexpr u16 UNLOCK_OTHER_LOOK = 0x8000;
    constexpr int PAINT_STEP = 4;
    CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(player)];
    std::array<u8, 3> paint = choice.paint;
    bool otherLook = choice.otherLook;
    if (buttons.cDown && (ProfileOf(player).unlocks & UNLOCK_OTHER_LOOK) != 0) {
        choice.otherLook = !choice.otherLook;
    }
    bool up = stick[0] || (m_carSelectConsoleFrame && (buttons.heldButtons & BUTTON_D_UP) != 0);
    bool down = stick[1] || (m_carSelectConsoleFrame && (buttons.heldButtons & BUTTON_D_DOWN) != 0);
    constexpr std::array<std::uint16_t, 3> PARTS{BUTTON_C_LEFT, BUTTON_C_UP, BUTTON_C_RIGHT};
    for (std::size_t i = 0; i < PARTS.size(); ++i) {
        if ((buttons.heldButtons & PARTS[i]) == 0) {
            continue;
        }
        u8 &part = choice.paint[i];
        // Up raises the part first, and down lowers it after.
        if (up) {
            part = part < 0x100 - PAINT_STEP ? static_cast<u8>(part + PAINT_STEP) : 0xFF;
        }
        if (down) {
            part = part < PAINT_STEP ? 0 : static_cast<u8>(part - PAINT_STEP);
        }
    }
    if (choice.paint != paint || choice.otherLook != otherLook) {
        RepaintCars(renderer, player);
    }
}

// SetVehicleModelSlot() of the console paints the model of each slot of a player, the custom
// decals loaded into it included, in the paint of the player as it draws it.
void Menus::RepaintCars(SceneRenderer &renderer, int player) {
    const CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(player)];
    for (int slot : {choice.slot ^ 2, choice.slot}) {
        int car = m_slotCars[static_cast<std::size_t>(slot)];
        if (m_carModels[static_cast<std::size_t>(player)][static_cast<std::size_t>(car)]) {
            LoadCarModel(renderer, slot, car);
        }
    }
    std::array<int, 4> look{
        choice.paint[0] >> 3, choice.paint[1] >> 3, choice.paint[2] >> 3, choice.otherLook};
    if (m_customModels[static_cast<std::size_t>(player)] &&
        m_customModelLooks[static_cast<std::size_t>(player)] != look) {
        LoadCustomModel(renderer, player);
    }
}

// The model of the car of a player with the custom decals, in the paint and the look of the
// player. A model the frame drew is released at the start of the next frame.
void Menus::LoadCustomModel(SceneRenderer &renderer, int player) {
    auto index = static_cast<std::size_t>(player);
    const CarSelectPlayer &choice = m_carSelect[index];
    m_retiredCarModels.push_back(std::move(m_customModels[index]));
    if (!m_customDecals[index] || !m_customDecalImages[index]) {
        return;
    }
    ModelOptions options;
    options.parts = StagePartsOf(choice.otherLook);
    options.images = m_customDecalImages[index];
    options.paint = choice.paint;
    m_customModels[index] =
        RenderModel::Load(renderer, m_customDecals[index]->Directory() / "model.gltf", options);
    m_customModelLooks[index] = {
        choice.paint[0] >> 3, choice.paint[1] >> 3, choice.paint[2] >> 3, choice.otherLook};
}

// ExtractVehicleBaseColor() of the console: the paint of a player becomes the colour of the paint
// of the custom decals loaded.
void Menus::TakeDecalPaint(int player) {
    auto index = static_cast<std::size_t>(player);
    if (!m_customDecals[index]) {
        return;
    }
    u16 paint = m_customDecals[index]->PaletteColor(CAR_MATERIAL_PAINT, 0);
    m_carSelect[index].paint = {static_cast<u8>(((paint >> 13) & 7) | ((paint >> 8) & 0xF8)),
                                static_cast<u8>(((paint >> 8) & 7) | ((paint >> 3) & 0xF8)),
                                static_cast<u8>(((paint >> 3) & 7) | ((paint << 2) & 0xF8))};
}

void Menus::StartCarSelect(SceneRenderer &renderer) {
    m_screen->FadeIn();
    m_carSelectAngle = {0.0f, 1.0f};
    for (int i = 0; i < m_numLocalPlayers; ++i) {
        CarSelectPlayer &player = m_carSelect[static_cast<std::size_t>(i)];
        player.slot = i;
        player.state = CAR_SELECT_CAR;
        player.slide = 0.0f;
        ResetCarPaint(i);

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
    // UpdateStickRepeat() of the console: up and down of the stick, each frame of the console.
    std::array<bool, 2> stick{};
    if (m_carSelectConsoleFrame) {
        float axis = std::clamp(static_cast<float>(buttons.stickY) / STICK_RANGE, -1.0f, 1.0f);
        stick = {StepStickRepeat(choice.stickRepeat[0], axis, m_carSelectFrameMs),
                 StepStickRepeat(choice.stickRepeat[1], -axis, m_carSelectFrameMs)};
    }
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

    if (choice.state == CAR_SELECT_CAR || choice.state == CAR_SELECT_DECALS) {
        AdjustCarPaint(renderer, player, buttons, stick);
    }

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
            // The custom decals the player chose last are offered again, loaded from the pak.
            choice.customDecals = choice.customDecals && RestoreCustomDecals(renderer, player);
            if (choice.customDecals) {
                choice.optionCount = static_cast<int>(m_decalOptions.size());
                choice.cursor = DECALS_CUSTOM;
            }
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
            // The decals load into the model of the car of the player, as the console loads them
            // into the slot of the race.
            auto &custom = m_customDecals[static_cast<std::size_t>(player)];
            custom = DecalCar::Load(m_release /
                                    m_cars[static_cast<std::size_t>(choice.carModel)].directory);
            if (custom && m_paintShop) {
                m_pakDialogPlayer = player;
            }
        } else {
            choice.prevCursor = choice.cursor;

            // Start and the paint shop skip the rest. A skip before the decals races the decals
            // chosen last.
            if (buttons.start && !m_paintMode && choice.state < CAR_SELECT_DECALS) {
                choice.customDecals = choice.customDecals && RestoreCustomDecals(renderer, player);
            }
            if (buttons.start || m_paintMode) {
                choice.optionCount = static_cast<int>(m_decalOptions.size()) - 1;
                choice.cursor = 0;
                choice.prevCursor = 0;
                choice.state = CAR_SELECT_READY;
            } else {
                ++choice.state;
            }
            SaveCarChoice(player);

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

// DrawCarStageDisc() and DrawCarStageChoiceMarker() of the console: the white disc of the stage,
// and the arrows of the choice unless they are hidden, under g_CarStageMtx and over what is drawn
// before them, as their display lists do not test the depth.
void Menus::DrawCarStage(SceneRenderer &renderer,
                         const Matrix4 &camera,
                         bool hideChoiceMarker) const {
    Matrix4 stage = MultiplyMatrices(
        camera, MultiplyMatrices(RotationZ(STAGE_DISC_TURN), Scale(STAGE_DISC_SCALE)));
    StageMesh disc;
    for (int i = 0; i < STAGE_DISC_POINTS; ++i) {
        float angle = static_cast<float>(i) * STAGE_DISC_STEP;
        disc.vertices.push_back(
            {{static_cast<float>(static_cast<s16>(std::cos(angle) * STAGE_DISC_RADIUS)),
              static_cast<float>(static_cast<s16>(std::sin(angle) * STAGE_DISC_RADIUS)),
              0.0f},
             {0xFF, 0xFF, 0xFF, 0xFF}});
    }
    disc.vertices[0].position = {0.0f, 0.0f, 0.0f};
    for (int i = 1; i < STAGE_DISC_POINTS; ++i) {
        disc.triangles.push_back({0, i, i + 1 < STAGE_DISC_POINTS ? i + 1 : 1});
    }
    renderer.DrawQuads(StageQuads(disc), FlatMaterial{}, stage);
    if (!hideChoiceMarker) {
        renderer.DrawQuads(StageQuads(m_stageChoiceMarker), FlatMaterial{}, stage);
    }
}

// DRAW_STAGE_VEHICLE() of the console: DrawCarStageShadow() places the octagon of the shadow,
// scaled to the car of a slot, below the car without testing the depth, and RenderVehicleModel()
// draws the car over it with the sky reflected on its painted body.
void Menus::DrawStageCar(SceneRenderer &renderer,
                         const Matrix4 &camera,
                         const std::array<float, 3> &eye,
                         int slot,
                         float x,
                         float y,
                         float angle,
                         const SDL_Rect &scissor) const {
    int car = m_slotCars[static_cast<std::size_t>(slot)];
    const RenderModel *model =
        m_carModels[static_cast<std::size_t>(slot & 1)][static_cast<std::size_t>(car)].get();
    // The slot of a player with the decals loaded from the Controller Pak shows them.
    if (slot < 2) {
        const CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(slot)];
        const auto &custom = m_customModels[static_cast<std::size_t>(slot)];
        if (custom && (choice.state == CAR_SELECT_DECALS || choice.state == CAR_SELECT_READY)) {
            model = custom.get();
        }
    }
    if (model == nullptr) {
        return;
    }
    const MenuCar &record = m_cars[static_cast<std::size_t>(car)];
    Matrix4 shadow{record.shadowLength / SHADOW_UNITS,
                   0,
                   0,
                   0,
                   0,
                   (record.shadowHalfWidth + record.shadowHalfWidth) / SHADOW_UNITS,
                   0,
                   0,
                   0,
                   0,
                   1.0f / SHADOW_UNITS,
                   0,
                   0,
                   0,
                   -SHADOW_DROP,
                   1};
    renderer.DrawQuads(
        StageQuads(m_stageShadow),
        FlatMaterial{},
        MultiplyMatrices(camera, MultiplyMatrices(StageFrameAt(x, y, angle), shadow)));
    Reflection reflection = StageReflection(m_stageReflection.get(), eye);
    renderer.DrawReflectingModel(
        *model, StageCarAt(x, y, angle), camera, STAGE_CAR_LIGHTING, reflection, scissor);
    const auto &highlighted =
        m_carHighlightModels[static_cast<std::size_t>(slot & 1)][static_cast<std::size_t>(car)];
    if (highlighted) {
        renderer.DrawReflectingModel(*highlighted,
                                     StageFrameAt(x, y, angle),
                                     camera,
                                     STAGE_CAR_LIGHTING,
                                     reflection,
                                     scissor);
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
    DrawCarStage(renderer, camera, choice.state == CAR_SELECT_READY);
    auto draw = [&](const RenderModel *option, int slot, float x, float along) {
        if (option) {
            renderer.DrawModel(*option,
                               StageOptionAt(x, along, angle),
                               camera,
                               STAGE_MODEL_LIGHTING,
                               std::nullopt,
                               true,
                               scissor);
            return;
        }
        DrawStageCar(renderer, camera, eye, slot, x, along, angle, scissor);
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
    // The Japanese release draws its note on one line of a larger size.
    const std::string *note = FindString("n64/802a90a8");
    if (note != nullptr && decalsNote(0)) {
        style.size = 14;
        text.Draw(renderer, *note, WIDTH / 2, HEIGHT * 3 / 8 - 1, style, flat);
        style.size = 8;
    } else if (decalsNote(0)) {
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

// RunPakDialog() of the console: the dialog that loads the decals of a player, and the custom
// decals it adds once they have loaded.
void Menus::RunPakDialog(SceneRenderer &renderer, float seconds) {
    auto player = static_cast<std::size_t>(m_pakDialogPlayer);
    CarSelectPlayer &choice = m_carSelect[player];
    auto loaded =
        m_paintShop->LoadDecalsFrame(renderer, seconds, m_pointer, *m_customDecals[player]);
    if (!loaded) {
        return;
    }
    m_pakDialogPlayer = -1;
    int options = static_cast<int>(m_decalOptions.size());
    if (!*loaded) {
        // The custom decals are out of reach without loaded ones.
        choice.optionCount = options - 1;
        return;
    }
    // Loaded decals add the custom choice, and the cursor moves to it.
    choice.optionCount = options;
    choice.prevCursor = choice.cursor;
    choice.cursor = (choice.cursor + 1) % options;
    choice.slide = 1.0f;
    UseCustomDecals(renderer, static_cast<int>(player));
}

// The images of the car of a player with the custom decals loaded into it, and the model of the
// car that shows them.
void Menus::UseCustomDecals(SceneRenderer &renderer, int player) {
    auto index = static_cast<std::size_t>(player);
    const DecalCar &car = *m_customDecals[index];
    m_customDecalImages[index] =
        car.WriteImages(m_cache / "decals" / ("player" + std::to_string(player + 1)));
    TakeDecalPaint(player);
    LoadCustomModel(renderer, player);
    RepaintCars(renderer, player);
}

// The custom decals of the car a player chose, loaded from the pak without the dialog when the
// decals loaded are of another car or none have loaded.
bool Menus::RestoreCustomDecals(SceneRenderer &renderer, int player) {
    auto index = static_cast<std::size_t>(player);
    int car = m_carSelect[index].carModel;
    if (!m_paintShop || car >= PAINT_SHOP_CARS) {
        return false;
    }
    auto directory = m_release / m_cars[static_cast<std::size_t>(car)].directory;
    auto &custom = m_customDecals[index];
    if (custom && custom->Directory() == directory && m_customDecalImages[index]) {
        TakeDecalPaint(player);
        RepaintCars(renderer, player);
        return true;
    }
    custom = DecalCar::Load(directory);
    if (!custom || !m_paintShop->LoadDecalsQuietly(*custom)) {
        custom.reset();
        return false;
    }
    UseCustomDecals(renderer, player);
    return m_customDecalImages[index].has_value();
}

// The car and the decals each player chose last, and the equipment of the second player. The note
// of the options has the equipment of the first player, as the console saves it there.
void Menus::LoadCarChoices() {
    for (std::size_t player = 0; player < m_carSelect.size(); ++player) {
        CarSelectPlayer &choice = m_carSelect[player];
        std::string section = "Player" + std::to_string(player + 1);
        choice.carModel = m_config->GetIndex(section, "Car", m_cars.size(), choice.carModel);
        choice.selectedCar = choice.carModel;
        choice.customDecals = m_config->GetBoolean(section, "CustomDecals", false);
    }
    MenuProfile &second = m_secondProfile;
    second.handling =
        m_config->GetIndex("Player2", "Handling", m_handlingOptions.size(), second.handling);
    second.transmission = m_config->GetIndex(
        "Player2", "Transmission", m_transmissionOptions.size(), second.transmission);
    second.tires = m_config->GetIndex("Player2", "Tires", m_tiresOptions.size(), second.tires);
    second.suspension =
        m_config->GetIndex("Player2", "Suspension", m_suspensionOptions.size(), second.suspension);
}

void Menus::SaveCarChoice(int player) {
    const CarSelectPlayer &choice = m_carSelect[static_cast<std::size_t>(player)];
    std::string section = "Player" + std::to_string(player + 1);
    m_config->Set(section, "Car", std::to_string(choice.carModel));
    m_config->Set(section, "CustomDecals", choice.customDecals ? "1" : "0");
    if (player == 0) {
        if (!SaveOptionsNote(0)) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "The options were not saved.");
        }
    } else {
        const MenuProfile &profile = m_secondProfile;
        m_config->Set(section, "Handling", std::to_string(profile.handling));
        m_config->Set(section, "Transmission", std::to_string(profile.transmission));
        m_config->Set(section, "Tires", std::to_string(profile.tires));
        m_config->Set(section, "Suspension", std::to_string(profile.suspension));
    }
    if (!m_config->Save(m_configPath)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "BossRally.ini cannot be written.");
    }
}

void Menus::LeaveCarSelect() {
    m_carSelectInitialized = false;
    MenuScreen::SilenceSounds();
    if (m_carSelectDone && !m_paintMode) {
        m_end = MenusEnd::Race;
    } else if (m_carSelectDone && m_paintShop) {
        const CarSelectPlayer &choice = m_carSelect[0];
        m_paintShop->SetCar(choice.carModel,
                            m_release /
                                m_cars[static_cast<std::size_t>(choice.carModel)].directory);
        SetCurrentScreen(Screen::PaintShopScreen);
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
                                   const std::array<MenuButtons, 2> &players) {
    if (!m_carSelectInitialized) {
        StartCarSelect(renderer);
    }
    m_screen->UpdateFade(seconds);
    m_screen->DrawBackground(renderer, {0x00, 0x00, 0x00}, {0x50, 0x00, 0x00});
    // A slow frame advances the console a frame once, not once for each 30th it took. The
    // milliseconds of the frame are those UpdateFrameTiming() counts from a whole elapsed time.
    m_carSelectFrameTime += seconds;
    m_carSelectConsoleFrame = m_carSelectFrameTime >= CONSOLE_FRAME_SECONDS;
    if (m_carSelectConsoleFrame) {
        m_carSelectFrameTime = std::fmod(m_carSelectFrameTime, CONSOLE_FRAME_SECONDS);
        ++m_carSelectConsoleFrames;
        m_carSelectFrameMs =
            static_cast<int>(m_carSelectConsoleFrames * 1000 / CONSOLE_FRAME_RATE -
                             (m_carSelectConsoleFrames - 1) * 1000 / CONSOLE_FRAME_RATE);
    }

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

    // The dialog of the Controller Pak that loads the decals takes the input until it closes.
    if (m_pakDialogPlayer >= 0) {
        RunPakDialog(renderer, seconds);
        return;
    }
    if (m_screen->FadingOut()) {
        if (m_screen->FadeComplete()) {
            LeaveCarSelect();
        }
        return;
    }
    // Each player chooses with the controller of the player.
    for (int player = 0; player < m_numLocalPlayers; ++player) {
        HandleCarSelectInput(renderer, player, players[static_cast<std::size_t>(player)]);
    }
}

} // namespace coastline
