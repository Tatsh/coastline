#include "mainmenu.h"

#include <array>
#include <cmath>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "frontend.h"
#include "gpu.h"
#include "menutext.h"
#include "scenerenderer.h"

namespace coastline {

namespace {

// The time a fade of the screen in or out takes.
constexpr float FADE_TIME = 0.2f;

// The camera of the menus draws from 1 to 500 away.
constexpr float MENU_NEAR = 1.0f;
constexpr float MENU_FAR = 500.0f;

// The turn the model under the cursor starts at, and how long the menu waits for a press before it
// leaves by itself.
constexpr float START_ANGLE = 4.712388992f;
constexpr float IDLE_TIMEOUT = 20.0f;

// The items of the main menu in their order, by the names of their models in the import.
constexpr std::array<const char *, 7> ITEMS{
    "championship", "arcade", "time-attack", "practice", "paint-shop", "load-save", "options"};

// The flags of an item that keep it from being chosen.
constexpr int MENU_ITEM_UNAVAILABLE = 0x08;
constexpr int MENU_ITEM_LOCKED = 0x40;

// The scale of a model on the stage, the tilt of the stage towards the camera, and the middle of
// the stage on the 320 by 240 screen.
constexpr float STAGE_MODEL_SCALE = 0.03125f;
constexpr float STAGE_TILT = 12.0f * PI / 180.0f;
constexpr float STAGE_X = SCREEN_WIDTH / 2.0f;
constexpr float STAGE_Y = SCREEN_HEIGHT * 6.0f / 16.0f;

// The points of the beam of the spotlight, the turn between them, and its radius.
constexpr int SPOTLIGHT_POINTS = 31;
constexpr float SPOTLIGHT_STEP = 0.2094395161f;
constexpr float SPOTLIGHT_RADIUS = 64.0f;

// The background lerps from the primitive to the environment colour by the intensity of its
// texels, opaque.
constexpr std::array<std::uint32_t, 2> BACKGROUND_COMBINE = CombineLerp(ENVIRONMENT,
                                                                        PRIMITIVE,
                                                                        TEXEL0,
                                                                        PRIMITIVE,
                                                                        ZERO_ALPHA,
                                                                        ZERO_ALPHA,
                                                                        ZERO_ALPHA,
                                                                        6,
                                                                        ENVIRONMENT,
                                                                        PRIMITIVE,
                                                                        TEXEL0,
                                                                        PRIMITIVE,
                                                                        ZERO_ALPHA,
                                                                        ZERO_ALPHA,
                                                                        ZERO_ALPHA,
                                                                        6);

// The texels as they are, blended over the picture.
constexpr std::array<std::uint32_t, 2> TEXTURE_COMBINE = CombineLerp(ZERO_AB,
                                                                     ZERO_AB,
                                                                     ZERO_D,
                                                                     TEXEL0,
                                                                     ZERO_ALPHA,
                                                                     ZERO_ALPHA,
                                                                     ZERO_ALPHA,
                                                                     TEXEL0,
                                                                     ZERO_AB,
                                                                     ZERO_AB,
                                                                     ZERO_D,
                                                                     TEXEL0,
                                                                     ZERO_ALPHA,
                                                                     ZERO_ALPHA,
                                                                     ZERO_ALPHA,
                                                                     TEXEL0);

// The colour of the vertices as it is.
constexpr std::array<std::uint32_t, 2> SHADE_COMBINE = CombineLerp(ZERO_AB,
                                                                   ZERO_AB,
                                                                   ZERO_D,
                                                                   SHADE,
                                                                   ZERO_ALPHA,
                                                                   ZERO_ALPHA,
                                                                   ZERO_ALPHA,
                                                                   SHADE,
                                                                   ZERO_AB,
                                                                   ZERO_AB,
                                                                   ZERO_D,
                                                                   SHADE,
                                                                   ZERO_ALPHA,
                                                                   ZERO_ALPHA,
                                                                   ZERO_ALPHA,
                                                                   SHADE);

// A vertex of a coloured triangle of the stage.
struct StageVertex {
    std::array<float, 3> position;
    std::array<std::uint8_t, 4> color;
};

// A triangle drawn as a quad whose last corner repeats its third.
void AddTriangle(std::vector<FlatVertex> &quads,
                 const StageVertex &a,
                 const StageVertex &b,
                 const StageVertex &c) {
    for (const StageVertex *corner : {&a, &b, &c, &c}) {
        quads.push_back({corner->position[0],
                         corner->position[1],
                         0.0f,
                         0.0f,
                         corner->color,
                         corner->position[2]});
    }
}

// A texture stretched over a rectangle of the 320 by 240 screen, as DrawTexturedRect() draws it.
void DrawTexturedRect(SceneRenderer &renderer,
                      const RenderTexture &texture,
                      int x,
                      int y,
                      int width,
                      int height,
                      FlatMaterial material,
                      const Matrix4 &flat) {
    material.texel0 = &texture;
    auto left = static_cast<float>(x * 2);
    auto top = static_cast<float>(y * 2);
    auto right = static_cast<float>((x + width) * 2);
    auto bottom = static_cast<float>((y + height) * 2);
    std::array<FlatVertex, 4> corners{{{left, top, 0.0f, 0.0f},
                                       {right, top, 1.0f, 0.0f},
                                       {right, bottom, 1.0f, 1.0f},
                                       {left, bottom, 0.0f, 1.0f}}};
    renderer.DrawQuads(corners, material, flat);
}

// The colour of the RDP from its 8-bit components.
std::array<float, 4> Color(int red, int green, int blue, int alpha) {
    return {static_cast<float>(red) / 255.0f,
            static_cast<float>(green) / 255.0f,
            static_cast<float>(blue) / 255.0f,
            static_cast<float>(alpha) / 255.0f};
}

} // namespace

struct MainMenu::State {
    struct Item {
        std::string name;
        int flags = 0;
        std::unique_ptr<RenderModel> model;
    };
    std::vector<Item> items;
    std::unique_ptr<RenderTexture> background;
    std::unique_ptr<RenderTexture> title;
    std::unique_ptr<RenderTexture> arrowLeft;
    std::unique_ptr<RenderTexture> arrowRight;
    std::unique_ptr<MenuText> text;
    Lighting lighting;
    std::vector<StageVertex> marker;
    std::vector<std::array<int, 3>> markerTriangles;
    std::vector<s8> scrollSound;
    std::vector<s8> selectSound;

    ScreenFade fade;
    ScreenFade sfxFade{.level = 1.0f, .target = 1.0f};
    bool started = false;
    int cursor = 0;
    int previous = 0;
    float angle = START_ANGLE;
    float slide = 0.0f;
    float idleTime = 0.0f;
    bool chosen = false;

    void FadeOut() {
        fade.SetTarget(0.0f, FADE_TIME);
        sfxFade.SetTarget(0.0f, FADE_TIME);
    }

    // Move the cursor by a step across the items, and slide them with a sound when it moves.
    void Move(int step, float startSlide) {
        int count = static_cast<int>(items.size());
        previous = cursor;
        cursor = (cursor + count + step) % count;
        if (cursor != previous) {
            slide = startSlide;
            PlaySoundEffect(scrollSound, SfxVolume(32, 32));
        }
    }
};

MainMenu::MainMenu(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

MainMenu::~MainMenu() = default;

std::unique_ptr<MainMenu> MainMenu::Load(SceneRenderer &renderer,
                                         const std::filesystem::path &release) {
    auto state = std::make_unique<State>();
    auto items = release / "menu-items" / "main";
    auto data = release / "data";
    try {
        std::ifstream recordsFile(items / "items.json");
        auto records = nlohmann::json::parse(recordsFile);
        for (const char *stem : ITEMS) {
            State::Item item;
            item.name = records.at(stem).at("name").get<std::string>();
            item.flags = records.at(stem).at("flags").get<int>();
            item.model = RenderModel::Load(renderer, items / (std::string(stem) + ".gltf"));
            if (!item.model) {
                return nullptr;
            }
            state->items.push_back(std::move(item));
        }
        std::ifstream lightsFile(release / "lights.json");
        state->lighting = ReadLighting(nlohmann::json::parse(lightsFile).at("menu"));
        std::ifstream markerFile(data / "menu-choice-marker.json");
        auto marker = nlohmann::json::parse(markerFile);
        for (const auto &vertex : marker.at("vertices")) {
            state->marker.push_back({vertex.at("position").get<std::array<float, 3>>(),
                                     vertex.at("color").get<std::array<std::uint8_t, 4>>()});
        }
        state->markerTriangles = marker.at("triangles").get<std::vector<std::array<int, 3>>>();
    } catch (const nlohmann::json::exception &error) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s is not the main menu of a release: %s",
                     release.string().c_str(),
                     error.what());
        return nullptr;
    }
    state->background = RenderTexture::Load(renderer, data / "g_BgTextureDesc.png", true);
    state->title = RenderTexture::Load(renderer, data / "g_MainMenuTitleTextureDesc.png", true);
    state->arrowLeft = RenderTexture::Load(renderer, data / "g_ArrowLeftTextureDesc.png", true);
    state->arrowRight = RenderTexture::Load(renderer, data / "g_ArrowRightTextureDesc.png", true);
    state->text = MenuText::Load(renderer, release / "fonts");
    auto scroll = ReadSound(release / "segments" / "scrollSound.wav");
    auto select = ReadSound(release / "segments" / "selectSound.wav");
    if (!state->background || !state->title || !state->arrowLeft || !state->arrowRight ||
        !state->text || !scroll || !select) {
        return nullptr;
    }
    state->scrollSound = std::move(*scroll);
    state->selectSound = std::move(*select);
    std::unique_ptr<MainMenu> menu(new MainMenu(std::move(state)));
    menu->Restart();
    return menu;
}

void MainMenu::Restart() {
    State &s = *m_state;
    s.fade = {};
    s.fade.SetTarget(1.0f, FADE_TIME);
    s.sfxFade = {.level = s.sfxFade.level, .target = s.sfxFade.target};
    s.sfxFade.SetTarget(1.0f, FADE_TIME);
    if (!s.started) {
        s.cursor = 0;
        s.started = true;
    }
    s.previous = s.cursor;
    s.angle = START_ANGLE;
    s.slide = 0.0f;
    s.idleTime = 0.0f;
    s.chosen = false;
}

int MainMenu::Selection() const {
    return m_state->cursor;
}

const std::string &MainMenu::SelectionName() const {
    return m_state->items[static_cast<std::size_t>(m_state->cursor)].name;
}

MainMenuEnd MainMenu::Update(float seconds, const MenuButtons &buttons) {
    State &s = *m_state;
    s.fade.Update(seconds);
    s.sfxFade.Update(seconds);
    g_bSfxVolume = static_cast<u8>(static_cast<int>(s.sfxFade.level * MAX_SFX_VOLUME));

    // The model under the cursor turns twice a second about, and the items slide to rest.
    float step = seconds + seconds;
    s.angle += step;
    while (TWO_PI < s.angle) {
        s.angle -= TWO_PI;
    }
    if (s.slide < 0.0f) {
        s.slide = std::min(s.slide + step, 0.0f);
    } else if (0.0f < s.slide) {
        s.slide = std::max(s.slide - step, 0.0f);
    }

    if (s.fade.FadingOut()) {
        if (s.fade.Complete()) {
            for (auto &channel : g_aSoundEffectChannels) {
                channel.volume = 0;
                channel.mixVolume = 0;
                channel.step = 0;
            }
            return s.chosen ? MainMenuEnd::Chosen : MainMenuEnd::Left;
        }
        return MainMenuEnd::Running;
    }
    if (buttons.held) {
        s.idleTime = 0.0f;
    }
    bool pressed = false;
    if (s.slide == 0.0f) {
        // Right is not tried once left has moved.
        if (buttons.left) {
            s.Move(-1, 1.0f);
        }
        if (s.slide == 0.0f && buttons.right) {
            s.Move(1, -1.0f);
        }
        if (buttons.back) {
            s.chosen = false;
            s.FadeOut();
            pressed = true;
        }
    }
    const State::Item &item = s.items[static_cast<std::size_t>(s.cursor)];
    if (!pressed && buttons.choose && !(item.flags & MENU_ITEM_UNAVAILABLE) &&
        !(item.flags & MENU_ITEM_LOCKED)) {
        s.chosen = true;
        s.FadeOut();
        pressed = true;
    }
    if (pressed) {
        PlaySoundEffect(s.selectSound, SfxVolume(32, 32));
        return MainMenuEnd::Running;
    }

    // A menu left alone long enough leaves by itself.
    s.idleTime += seconds;
    if (IDLE_TIMEOUT <= s.idleTime) {
        s.chosen = false;
        s.FadeOut();
    }
    return MainMenuEnd::Running;
}

void MainMenu::Draw(SceneRenderer &renderer) const {
    const State &s = *m_state;
    GpuWindow &window = renderer.Window();
    float aspect = static_cast<float>(window.FrameWidth()) /
                   static_cast<float>(std::max<Uint32>(window.FrameHeight(), 1));
    Matrix4 flat = FlatProjection(aspect);
    Matrix4 projection = MenuProjection(aspect, MENU_NEAR, MENU_FAR);

    // The background lerps from black to a dark grey by its texels.
    FlatMaterial background;
    background.combine = BACKGROUND_COMBINE;
    background.prim = Color(0x00, 0x00, 0x00, 0xFF);
    background.env = Color(0x40, 0x40, 0x40, 0xFF);
    DrawTexturedRect(renderer, *s.background, 0, 0, 320, 240, background, flat);
    FlatMaterial plain;
    plain.combine = TEXTURE_COMBINE;
    DrawTexturedRect(renderer, *s.title, 25, 14, 280, 32, plain, flat);
    TextStyle style;
    style.size = 30;
    style.align = TextStyle::Align::Center;
    s.text->Draw(renderer, "%ryTOP GEAR RALLY", 160, 240 / 6 - 2, style, flat);

    // The stage is tilted towards the camera, under a spotlight, with the arrows of the choice
    // while there is another item to move to.
    Matrix4 stage = MultiplyMatrices(projection, RotationX(STAGE_TILT));
    std::vector<StageVertex> beam;
    for (int i = 0; i < SPOTLIGHT_POINTS; ++i) {
        float turn = static_cast<float>(i) * SPOTLIGHT_STEP;
        beam.push_back(
            {{static_cast<float>(static_cast<s16>(std::sin(turn) * SPOTLIGHT_RADIUS)) + STAGE_X,
              STAGE_Y,
              static_cast<float>(static_cast<s32>(std::cos(turn) * SPOTLIGHT_RADIUS))},
             {0xFF, 0xFF, 0xFF, 0xFF}});
    }
    beam[0].position = {STAGE_X, SCREEN_HEIGHT * 5.0f / 16.0f, 0.0f};
    std::vector<FlatVertex> spotlight;
    for (int i = 1; i < SPOTLIGHT_POINTS; ++i) {
        int next = i + 1 < SPOTLIGHT_POINTS ? i + 1 : 1;
        AddTriangle(spotlight,
                    beam[0],
                    beam[static_cast<std::size_t>(i)],
                    beam[static_cast<std::size_t>(next)]);
    }
    FlatMaterial shaded;
    shaded.combine = SHADE_COMBINE;
    renderer.DrawQuads(spotlight, shaded, stage);
    if (s.items.size() > 1) {
        Matrix4 place = MultiplyMatrices(Translation(160.0f, STAGE_Y - 4.0f, 0.0f),
                                         MultiplyMatrices(RotationX(-HALF_PI), Scale(0.26f)));
        std::vector<FlatVertex> arrows;
        for (const auto &triangle : s.markerTriangles) {
            AddTriangle(arrows,
                        s.marker.at(static_cast<std::size_t>(triangle[0])),
                        s.marker.at(static_cast<std::size_t>(triangle[1])),
                        s.marker.at(static_cast<std::size_t>(triangle[2])));
        }
        renderer.DrawQuads(arrows, shaded, MultiplyMatrices(stage, place));
    }

    // The item slides in and the one the cursor left slides out, or the item turns at rest with
    // its name above it.
    auto drawItem = [&](int index, float offset) {
        const State::Item &item = s.items[static_cast<std::size_t>(index)];
        Matrix4 world = MultiplyMatrices(
            RotationX(STAGE_TILT),
            ScreenModelAt(
                STAGE_X - offset * SCREEN_WIDTH / 6.0f, STAGE_Y, 0.0f, s.angle, STAGE_MODEL_SCALE));
        renderer.DrawModel(*item.model, world, projection, s.lighting, std::nullopt, true);
    };
    float ease = s.slide < 0.0f ? s.slide * s.slide * -5.0f : s.slide * s.slide * 5.0f;
    if (ease != 0.0f) {
        drawItem(s.cursor, ease);
        drawItem(s.previous, (ease < 0.0f ? 5.0f : -5.0f) + ease);
    } else {
        drawItem(s.cursor, 0.0f);
        style.size = 20;
        s.text->Draw(renderer,
                     "%ry" + s.items[static_cast<std::size_t>(s.cursor)].name,
                     160,
                     240 * 20 / 64,
                     style,
                     flat);
    }

    // The buttons at the bottom and their names.
    DrawTexturedRect(renderer, *s.arrowLeft, 100, 216, 12, 12, plain, flat);
    DrawTexturedRect(renderer, *s.arrowRight, 160, 216, 12, 12, plain, flat);
    TextStyle footer;
    footer.size = 11;
    s.text->Draw(renderer, "%wwSelect", 115, 240 * 19 / 20 - 3, footer, flat);
    s.text->Draw(renderer, "%wwGo Back", 175, 240 * 19 / 20 - 3, footer, flat);

    s.fade.Draw(renderer);
}

} // namespace coastline
