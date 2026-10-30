#include "menuscreen.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "frontend.h"
#include "gpu.h"
#include "menutext.h"

namespace coastline {

namespace {

// The time a fade of the screen in or out takes.
constexpr float FADE_TIME = 0.2f;

// The camera of the menus draws from 1 to 500 away.
constexpr float MENU_NEAR = 1.0f;
constexpr float MENU_FAR = 500.0f;

// The turn the model under the cursor starts at.
constexpr float START_ANGLE = 4.712388992f;

// How far the items slide before the spinning model changes to the next one.
constexpr float SPIN_SWAP_SLIDE = 0.75f;

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

// DrawRotatedModel() of the console. A mirrored item turns the other way and is flipped.
Matrix4 StageModelAt(float x, float angle, bool mirrored) {
    if (!mirrored) {
        return ScreenModelAt(x, STAGE_Y, 0.0f, angle, STAGE_MODEL_SCALE);
    }
    Matrix4 flip{-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    Matrix4 place = MultiplyMatrices(
        Translation(x, STAGE_Y, 0.0f),
        MultiplyMatrices(
            RotationX(-HALF_PI),
            MultiplyMatrices(Scale(STAGE_MODEL_SCALE), MultiplyMatrices(flip, RotationZ(-angle)))));
    return MultiplyMatrices(place, RotationX(HALF_PI));
}

} // namespace

struct MenuScreen::State {
    std::map<std::filesystem::path, std::unique_ptr<RenderModel>> models;
    std::unique_ptr<RenderTexture> background;
    std::unique_ptr<RenderTexture> mainTitle;
    std::unique_ptr<RenderTexture> titleBar;
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
    bool initialized = false;
    int cursor = 0;
    int previous = 0;
    int pending = -1;
    int lastActive = 0;
    int choosingPlayer = 0;
    float angle = START_ANGLE;
    float slide = 0.0f;
    float idleTime = 0.0f;
    bool chosen = false;

    void FadeOut() {
        fade.SetTarget(0.0f, FADE_TIME);
        sfxFade.SetTarget(0.0f, FADE_TIME);
    }

    // Move the cursor a step at a time past the items that may not be moved to, and slide the
    // items with a sound when it moves.
    bool Move(int step, float startSlide, const MenuSetup &setup) {
        int count = static_cast<int>(setup.items.size());
        previous = cursor;
        do {
            cursor = (cursor + count + step) % count;
        } while (setup.isSelectable && !setup.isSelectable(cursor));
        if (cursor == previous) {
            return false;
        }
        slide = startSlide;
        PlaySoundEffect(scrollSound, SfxVolume(32, 32));
        return true;
    }
};

MenuScreen::MenuScreen(std::unique_ptr<State> state) : m_state(std::move(state)) {
}

MenuScreen::~MenuScreen() = default;

std::unique_ptr<MenuScreen> MenuScreen::Load(SceneRenderer &renderer,
                                             const std::filesystem::path &release) {
    auto state = std::make_unique<State>();
    auto data = release / "data";
    try {
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
                     "%s is not the menus of a release: %s",
                     release.string().c_str(),
                     error.what());
        return nullptr;
    }
    state->background = RenderTexture::Load(renderer, data / "g_BgTextureDesc.png", true);
    state->titleBar = RenderTexture::Load(renderer, data / "g_TitleBarTextureDesc.png", true);
    // The Japanese release shows the plain title bar on its main menu.
    auto mainTitle = data / "g_MainMenuTitleTextureDesc.png";
    state->mainTitle = std::filesystem::exists(mainTitle) ?
                           RenderTexture::Load(renderer, mainTitle, true) :
                           RenderTexture::Load(renderer, data / "g_TitleBarTextureDesc.png", true);
    state->arrowLeft = RenderTexture::Load(renderer, data / "g_ArrowLeftTextureDesc.png", true);
    state->arrowRight = RenderTexture::Load(renderer, data / "g_ArrowRightTextureDesc.png", true);
    state->text = MenuText::Load(renderer, release / "fonts");
    auto scroll = ReadSound(release / "segments" / "scrollSound.wav");
    auto select = ReadSound(release / "segments" / "selectSound.wav");
    if (!state->background || !state->titleBar || !state->mainTitle || !state->arrowLeft ||
        !state->arrowRight || !state->text || !scroll || !select) {
        return nullptr;
    }
    state->scrollSound = std::move(*scroll);
    state->selectSound = std::move(*select);
    return std::unique_ptr<MenuScreen>(new MenuScreen(std::move(state)));
}

const RenderModel *MenuScreen::LoadModel(SceneRenderer &renderer,
                                         const std::filesystem::path &path) {
    auto &model = m_state->models[path];
    if (!model) {
        model = RenderModel::Load(renderer, path);
    }
    return model.get();
}

std::optional<std::vector<MenuItem>> MenuScreen::LoadItems(SceneRenderer &renderer,
                                                           const std::filesystem::path &directory,
                                                           std::span<const char *const> stems) {
    std::vector<MenuItem> items;
    try {
        std::ifstream recordsFile(directory / "items.json");
        auto records = nlohmann::json::parse(recordsFile);
        for (const char *stem : stems) {
            MenuItem item;
            item.name = records.at(stem).at("name").get<std::string>();
            item.flags = records.at(stem).at("flags").get<u32>();
            item.model = LoadModel(renderer, directory / (std::string(stem) + ".gltf"));
            if (!item.model) {
                return std::nullopt;
            }
            items.push_back(std::move(item));
        }
    } catch (const nlohmann::json::exception &error) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s does not have the items of a menu: %s",
                     directory.string().c_str(),
                     error.what());
        return std::nullopt;
    }
    return items;
}

void MenuScreen::Reset() {
    idleTimeout = 0.0f;
    postDraw = nullptr;
    preDraw = nullptr;
}

void MenuScreen::SetPendingSelection(int item) {
    m_state->pending = item;
}

int MenuScreen::LastActiveIndex() const {
    return m_state->lastActive;
}

void MenuScreen::SetLastActiveIndex(int item) {
    m_state->lastActive = item;
}

int MenuScreen::ChoosingPlayer() const {
    return m_state->choosingPlayer;
}

const RenderTexture &MenuScreen::MainMenuTitle() const {
    return *m_state->mainTitle;
}

const RenderTexture &MenuScreen::TitleBar() const {
    return *m_state->titleBar;
}

const MenuText &MenuScreen::Text() const {
    return *m_state->text;
}

const Lighting &MenuScreen::MenuLighting() const {
    return m_state->lighting;
}

Matrix4 MenuScreen::FlatFor(SceneRenderer &renderer) {
    GpuWindow &window = renderer.Window();
    return FlatProjection(static_cast<float>(window.FrameWidth()) /
                          static_cast<float>(std::max<Uint32>(window.FrameHeight(), 1)));
}

void MenuScreen::DrawTexture(
    SceneRenderer &renderer, const RenderTexture &texture, int x, int y, int width, int height) {
    FlatMaterial plain;
    plain.combine = TEXTURE_COMBINE;
    DrawTexturedRect(renderer, texture, x, y, width, height, plain, FlatFor(renderer));
}

void MenuScreen::DrawFadeOverlay(
    SceneRenderer &renderer, float level, int x, int y, int width, int height) {
    auto alpha = static_cast<std::uint8_t>(0xFF - static_cast<int>(255.0f * level));
    std::array<std::uint8_t, 4> black{0, 0, 0, alpha};
    auto left = static_cast<float>(x * 2);
    auto top = static_cast<float>(y * 2);
    auto right = static_cast<float>((x + width) * 2);
    auto bottom = static_cast<float>((y + height) * 2);
    std::array<FlatVertex, 4> corners{{{left, top, 0.0f, 0.0f, black},
                                       {right, top, 0.0f, 0.0f, black},
                                       {right, bottom, 0.0f, 0.0f, black},
                                       {left, bottom, 0.0f, 0.0f, black}}};
    FlatMaterial shaded;
    shaded.combine = SHADE_COMBINE;
    renderer.DrawQuads(corners, shaded, FlatFor(renderer));
}

void MenuScreen::DrawBackground(SceneRenderer &renderer,
                                const std::array<int, 3> &top,
                                const std::array<int, 3> &bottom) const {
    FlatMaterial background;
    background.combine = BACKGROUND_COMBINE;
    background.prim = Color(top[0], top[1], top[2], 0xFF);
    background.env = Color(bottom[0], bottom[1], bottom[2], 0xFF);
    DrawTexturedRect(renderer, *m_state->background, 0, 0, 320, 240, background, FlatFor(renderer));
}

void MenuScreen::DrawFooter(SceneRenderer &renderer) const {
    const State &s = *m_state;
    DrawTexture(renderer, *s.arrowLeft, 100, 216, 12, 12);
    DrawTexture(renderer, *s.arrowRight, 160, 216, 12, 12);
    TextStyle footer;
    footer.size = 11;
    Matrix4 flat = FlatFor(renderer);
    s.text->Draw(renderer, "%wwSelect", 115, 240 * 19 / 20 - 3, footer, flat);
    s.text->Draw(renderer, "%wwGo Back", 175, 240 * 19 / 20 - 3, footer, flat);
}

void MenuScreen::PlayScrollSound() {
    PlaySoundEffect(m_state->scrollSound, SfxVolume(32, 32));
}

void MenuScreen::PlaySelectSound() {
    PlaySoundEffect(m_state->selectSound, SfxVolume(32, 32));
}

void MenuScreen::FadeIn() {
    State &s = *m_state;
    s.fade = {};
    s.fade.SetTarget(1.0f, FADE_TIME);
    s.sfxFade = {.level = s.sfxFade.level, .target = s.sfxFade.target};
    s.sfxFade.SetTarget(1.0f, FADE_TIME);
}

void MenuScreen::FadeOut() {
    m_state->FadeOut();
}

void MenuScreen::UpdateFade(float seconds) {
    State &s = *m_state;
    s.fade.Update(seconds);
    s.sfxFade.Update(seconds);
    g_bSfxVolume = static_cast<u8>(static_cast<int>(s.sfxFade.level * MAX_SFX_VOLUME));
}

bool MenuScreen::FadingOut() const {
    return m_state->fade.FadingOut();
}

bool MenuScreen::FadeComplete() const {
    return m_state->fade.Complete();
}

void MenuScreen::DrawFade(SceneRenderer &renderer) const {
    m_state->fade.Draw(renderer);
}

void MenuScreen::SilenceSounds() {
    for (auto &channel : g_aSoundEffectChannels) {
        channel.volume = 0;
        channel.mixVolume = 0;
        channel.step = 0;
    }
}

MenuResult MenuScreen::Process(SceneRenderer &renderer,
                               float seconds,
                               std::span<const MenuButtons> players,
                               const MenuSetup &setup) {
    State &s = *m_state;
    if (!s.initialized) {
        FadeIn();
        s.cursor = *setup.selection;
        s.previous = s.cursor;
        s.angle = START_ANGLE;
        s.slide = 0.0f;
        s.idleTime = 0.0f;
        s.pending = -1;
        s.chosen = false;
        s.initialized = true;
    }
    if (s.pending >= 0) {
        s.previous = s.cursor;
        s.cursor = s.pending;
        if (s.pending != s.previous) {
            s.slide = -1.0f;
            PlaySoundEffect(s.scrollSound, SfxVolume(32, 32));
        }
        s.pending = -1;
    }
    MenuResult result = Update(seconds, players, setup);
    Draw(renderer, setup);
    return result;
}

MenuResult
MenuScreen::Update(float seconds, std::span<const MenuButtons> players, const MenuSetup &setup) {
    State &s = *m_state;
    UpdateFade(seconds);

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
            // The screen is done: silence its sounds and exit.
            s.initialized = false;
            *setup.selection = s.cursor;
            SilenceSounds();
            return s.chosen ? MenuResult::Chosen : MenuResult::Cancelled;
        }
        s.lastActive = s.cursor;
        return MenuResult::None;
    }
    MenuResult result = MenuResult::None;
    for (std::size_t player = 0; player < players.size(); ++player) {
        const MenuButtons &buttons = players[player];
        bool upDown = true;
        bool selected = false;
        if (buttons.held) {
            s.idleTime = 0.0f;
        }
        if (s.slide == 0.0f) {
            // A move to the left or the right takes the presses of up and down too, and right is
            // not tried once left has moved.
            bool moved = false;
            if (buttons.left) {
                upDown = false;
                moved = s.Move(-1, 1.0f, setup);
            }
            if (!moved && buttons.right) {
                upDown = false;
                s.Move(1, -1.0f, setup);
            }
            const MenuItem &item = setup.items[static_cast<std::size_t>(s.cursor)];
            if (upDown && (item.flags & MENU_ITEM_ADJUSTABLE) && (buttons.up || buttons.down)) {
                *setup.selection = s.cursor;
                result = buttons.up ? MenuResult::Up : MenuResult::Down;
                selected = true;
            } else if (buttons.back) {
                s.chosen = false;
                s.FadeOut();
                selected = true;
            }
        }
        const MenuItem &item = setup.items[static_cast<std::size_t>(s.cursor)];
        if (!selected) {
            if ((item.flags & MENU_ITEM_UNAVAILABLE) || (item.flags & MENU_ITEM_LOCKED) ||
                !buttons.choose) {
                continue;
            }
            if (player != 0 && (item.flags & MENU_ITEM_FIRST_PLAYER)) {
                continue;
            }
            s.choosingPlayer = static_cast<int>(player);
            if (item.flags & MENU_ITEM_IMMEDIATE) {
                *setup.selection = s.cursor;
                result = MenuResult::Chosen;
            } else {
                s.chosen = true;
                s.FadeOut();
            }
        }
        PlaySoundEffect(s.selectSound, SfxVolume(32, 32));
    }

    // A screen without input for long enough exits by itself.
    if (idleTimeout != 0.0f && result == MenuResult::None) {
        s.idleTime += seconds;
        if (idleTimeout <= s.idleTime && !s.fade.FadingOut()) {
            s.chosen = false;
            s.FadeOut();
        }
    }
    s.lastActive = s.cursor;
    return result;
}

void MenuScreen::Draw(SceneRenderer &renderer, const MenuSetup &setup) const {
    const State &s = *m_state;
    GpuWindow &window = renderer.Window();
    float aspect = static_cast<float>(window.FrameWidth()) /
                   static_cast<float>(std::max<Uint32>(window.FrameHeight(), 1));
    Matrix4 flat = FlatProjection(aspect);
    Matrix4 projection = MenuProjection(aspect, MENU_NEAR, MENU_FAR);

    DrawBackground(renderer, setup.top, setup.bottom);
    FlatMaterial plain;
    plain.combine = TEXTURE_COMBINE;
    if (titleImage) {
        DrawTexturedRect(renderer,
                         *titleImage->texture,
                         titleImage->x,
                         titleImage->y,
                         titleImage->width,
                         titleImage->height,
                         plain,
                         flat);
    }
    TextStyle style;
    style.size = 30;
    style.align = TextStyle::Align::Center;
    s.text->Draw(renderer, "%ry" + std::string(setup.title), 160, 240 / 6 - 2, style, flat);

    if (preDraw) {
        preDraw(renderer);
    } else {
        // The stage is tilted towards the camera, under a spotlight, with the arrows of the
        // choice while there is another item to move to.
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
        int count = static_cast<int>(setup.items.size());
        int other = s.cursor;
        do {
            other = (other + count - 1) % count;
        } while (setup.isSelectable && !setup.isSelectable(other));
        if (other != s.cursor) {
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

        // An item is its model turning on the stage, or its name when it has no model.
        auto drawItem = [&](int index, float offset, float angle) {
            const MenuItem &item = setup.items[static_cast<std::size_t>(index)];
            if (item.model) {
                Matrix4 world =
                    MultiplyMatrices(RotationX(STAGE_TILT),
                                     StageModelAt(STAGE_X - offset * SCREEN_WIDTH / 6.0f,
                                                  angle,
                                                  (item.flags & MENU_ITEM_MIRRORED_MODEL) != 0));
                renderer.DrawModel(*item.model, world, projection, s.lighting, std::nullopt, true);
            } else {
                s.text->Draw(renderer,
                             "%ww" + item.name,
                             160 - static_cast<int>(offset * SCREEN_WIDTH / 6.0f),
                             240 * 9 / 16,
                             style,
                             flat);
            }
        };
        const MenuItem &current = setup.items[static_cast<std::size_t>(s.cursor)];
        float ease = s.slide < 0.0f ? s.slide * s.slide * -5.0f : s.slide * s.slide * 5.0f;
        bool showName = true;
        if (current.flags & MENU_ITEM_SPINNING) {
            // The model spins round as the items slide, changing halfway.
            int shown =
                SPIN_SWAP_SLIDE < s.slide || s.slide < -SPIN_SWAP_SLIDE ? s.previous : s.cursor;
            drawItem(shown, 0.0f, -HALF_PI - s.slide * TWO_PI);
            showName = s.slide == 0.0f;
        } else if (ease != 0.0f) {
            // The item slides in, and the one the cursor departed slides out.
            drawItem(s.cursor, ease, s.angle);
            drawItem(s.previous, (ease < 0.0f ? 5.0f : -5.0f) + ease, s.angle);
            showName = false;
        } else {
            drawItem(s.cursor, 0.0f, s.angle);
        }
        if (showName) {
            style.size = 20;
            s.text->Draw(renderer, "%ry" + current.name, 160, 240 * 20 / 64, style, flat);
            if (postDraw) {
                postDraw(renderer, s.cursor);
            }
        }
    }

    DrawFooter(renderer);
    s.fade.Draw(renderer);
}

} // namespace coastline
