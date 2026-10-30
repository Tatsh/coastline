#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "config.h"
#include "controls.h"
#include "menus.h"
#include "menutext.h"

namespace coastline {

namespace {

// The layouts of the console, TYPE A to D and the steering wheel, before the two presets of the
// SDL game, and the preset of the Controls section each item chooses.
constexpr int CONSOLE_LAYOUTS = 5;
constexpr std::array<std::size_t, CONSOLE_LAYOUTS + 2> ITEM_PRESETS{2, 3, 4, 5, 6, 0, 1};
constexpr std::array<const char *, 4> TYPE_NAMES{"TYPE A", "TYPE B", "TYPE C", "TYPE D"};
constexpr std::array<const char *, 2> PRESET_NAMES{"DEFAULT", "KART"};

// TYPE A to D share the model of the Controller Configuration item of the options menu, filed by
// the import under the options menu.
constexpr const char *CONTROLLER_MODEL = "menu-items/options/controller-configuration.gltf";

// The size of the labels.
constexpr int LABEL_TEXT_SIZE = 8;

// The rectangle of the pictures of the first four layouts. The fifth sits a little higher.
constexpr int PICTURE_X = 0x6F;
constexpr int PICTURE_Y = 0x55;
constexpr int PICTURE_WIDTH = 0x68;
constexpr int PICTURE_HEIGHT = 0x30;

// A label of a picture, as DrawControllerLayoutDiagram() of the console places it.
struct LayoutLabel {
    TextStyle::Align align;
    int x;
    int y;
    const char *text;
};

constexpr auto LEFT = TextStyle::Align::Left;
constexpr auto RIGHT = TextStyle::Align::Right;
constexpr auto CENTER = TextStyle::Align::Center;

constexpr LayoutLabel TYPE_A_LABELS[] = {
    {LEFT, 0xC9, 0x7F, "%wwACCELERATE"},
    {LEFT, 0xC1, 0x87, "%wwPLUS BRAKE = E-BRAKE"},
    {LEFT, 0xD7, 0x68, "%wwCHANGE VIEW"},
    {LEFT, 0xC7, 0x56, "%wwGEAR UP"},
    {RIGHT, 0x6B, 0x63, "%wwSTEERING / REVERSE"},
    {RIGHT, 0x75, 0x83, "%wwGEAR DOWN"},
    {CENTER, 0x9C, 0x56, "%wwBRAKE"},
};
constexpr LayoutLabel TYPE_B_LABELS[] = {
    {LEFT, 0xC9, 0x80, "%wwACCELERATE"},
    {LEFT, 0xD7, 0x68, "%wwCHANGE VIEW"},
    {LEFT, 0xC7, 0x56, "%wwGEAR UP"},
    {LEFT, 0xD3, 0x76, "%wwE-BRAKE"},
    {RIGHT, 0x6B, 0x63, "%wwSTEERING / REVERSE"},
    {RIGHT, 0x75, 0x83, "%wwGEAR DOWN"},
    {CENTER, 0x9C, 0x56, "%wwBRAKE"},
};
constexpr LayoutLabel TYPE_C_LABELS[] = {
    {LEFT, 0xC9, 0x7F, "%wwACCELERATE"},
    {LEFT, 0xC1, 0x87, "%wwPLUS BRAKE = E-BRAKE"},
    {LEFT, 0xD7, 0x68, "%wwCHANGE VIEW"},
    {LEFT, 0xC7, 0x56, "%wwGEAR UP"},
    {RIGHT, 0x6B, 0x7D, "%wwSTEERING, REVERSE"},
    {RIGHT, 0x74, 0x58, "%wwGEAR DOWN"},
    {CENTER, 0x9C, 0x56, "%wwBRAKE"},
};
constexpr LayoutLabel TYPE_D_LABELS[] = {
    {LEFT, 0xC9, 0x80, "%wwGEAR DOWN"},
    {LEFT, 0xD8, 0x67, "%wwCHANGE VIEW"},
    {RIGHT, 0x7C, 0x59, "%wwACCELERATE / REVERSE,"},
    {RIGHT, 0x7C, 0x61, "%wwSTEERING "},
    {RIGHT, 0x6E, 0x72, "%wwGEAR UP/DOWN"},
    {RIGHT, 0x77, 0x81, "%wwBRAKE"},
    {RIGHT, 0x77, 0x89, "%wwPLUS ACCELERATE = E-BRAKE "},
    {CENTER, 0x9C, 0x56, "%wwGEAR UP"},
};
constexpr LayoutLabel STEERING_WHEEL_LABELS[] = {
    {LEFT, 0xC7, 0x59, "%wwBRAKE"},
    {LEFT, 0xD0, 0x6E, "%wwACCELERATE"},
    {LEFT, 0xC8, 0x76, "%wwPLUS BRAKE = E-BRAKE"},
    {CENTER, 0xA7, 0x55, "%wwCHANGE VIEW"},
    {RIGHT, 0x72, 0x7C, "%wwSTEERING"},
    {RIGHT, 0x7F, 0x57, "%wwREVERSE"},
    {RIGHT, 0x74, 0x65, "%wwGEAR UP/DOWN"},
};
constexpr std::array<std::span<const LayoutLabel>, CONSOLE_LAYOUTS> LAYOUT_LABELS{
    TYPE_A_LABELS, TYPE_B_LABELS, TYPE_C_LABELS, TYPE_D_LABELS, STEERING_WHEEL_LABELS};

// The presets of the SDL game have no picture, and list their bindings in the place of one.
constexpr int PRESET_LINE_Y = 0x5A;
constexpr int PRESET_LINE_STEP = 9;
constexpr std::array<std::array<const char *, 6>, 2> PRESET_LINES{{
    {"%wwACCELERATE  SOUTH BUTTON, RIGHT TRIGGER",
     "%wwBRAKE  EAST BUTTON, LEFT TRIGGER",
     "%wwREVERSE  WEST BUTTON",
     "%wwGEAR UP/DOWN  RIGHT/LEFT SHOULDER",
     "%wwSTEERING  LEFT STICK, D-PAD",
     "%wwCHANGE VIEW  RIGHT STICK, NORTH BUTTON"},
    {"%wwACCELERATE  EAST BUTTON",
     "%wwBRAKE  SOUTH BUTTON",
     "%wwREVERSE  WEST BUTTON",
     "%wwGEAR UP/DOWN  RIGHT/LEFT SHOULDER OR TRIGGER",
     "%wwSTEERING  LEFT STICK, D-PAD",
     "%wwCHANGE VIEW  RIGHT STICK, NORTH BUTTON"},
}};

// TEXRECT_BLEND_COLOR_THROUGH_TEXTURE of DrawTexturedRect(): the primitive colour, its alpha the
// texels.
constexpr std::array<std::uint32_t, 2> COLOR_THROUGH_TEXTURE_COMBINE = CombineLerp(ZERO_AB,
                                                                                   ZERO_AB,
                                                                                   ZERO_D,
                                                                                   PRIMITIVE,
                                                                                   ZERO_ALPHA,
                                                                                   ZERO_ALPHA,
                                                                                   ZERO_ALPHA,
                                                                                   TEXEL0,
                                                                                   ZERO_AB,
                                                                                   ZERO_AB,
                                                                                   ZERO_D,
                                                                                   PRIMITIVE,
                                                                                   ZERO_ALPHA,
                                                                                   ZERO_ALPHA,
                                                                                   ZERO_ALPHA,
                                                                                   TEXEL0);

// A picture of a layout. The RDP reads the intensity of its texels as their alpha too, and the
// import writes them opaque.
std::unique_ptr<RenderTexture> LoadLayoutPicture(SceneRenderer &renderer,
                                                 const std::filesystem::path &path) {
    SDL_Surface *loaded = SDL_LoadPNG(path.string().c_str());
    SDL_Surface *rgba = loaded ? SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32) : nullptr;
    SDL_DestroySurface(loaded);
    if (rgba == nullptr) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s cannot be read.", path.string().c_str());
        return nullptr;
    }
    std::vector<std::uint8_t> texels;
    texels.reserve(static_cast<std::size_t>(rgba->w * rgba->h * 4));
    for (int y = 0; y < rgba->h; ++y) {
        const auto *row = static_cast<const std::uint8_t *>(rgba->pixels) + y * rgba->pitch;
        for (int x = 0; x < rgba->w; ++x) {
            std::uint8_t level = row[x * 4];
            texels.insert(texels.end(), {level, level, level, level});
        }
    }
    int width = rgba->w;
    int height = rgba->h;
    SDL_DestroySurface(rgba);
    return RenderTexture::FromTexels(renderer, width, height, texels, true, false);
}

// DrawTexturedRect() of the console in one colour through the picture, on the 320 by 240 screen.
void DrawPicture(SceneRenderer &renderer, const RenderTexture &picture, int x, int y, u8 level) {
    FlatMaterial material;
    material.texel0 = &picture;
    material.combine = COLOR_THROUGH_TEXTURE_COMBINE;
    float value = static_cast<float>(level) / 255.0f;
    material.prim = {value, value, value, 1.0f};
    auto left = static_cast<float>(x * 2);
    auto top = static_cast<float>(y * 2);
    auto right = static_cast<float>((x + PICTURE_WIDTH) * 2);
    auto bottom = static_cast<float>((y + PICTURE_HEIGHT) * 2);
    std::array<FlatVertex, 4> corners{{{left, top, 0.0f, 0.0f},
                                       {right, top, 1.0f, 0.0f},
                                       {right, bottom, 1.0f, 1.0f},
                                       {left, bottom, 0.0f, 1.0f}}};
    renderer.DrawQuads(corners, material, MenuScreen::FlatFor(renderer));
}

} // namespace

bool Menus::LoadControllerScreen(SceneRenderer &renderer, const std::filesystem::path &release) {
    const char *const wheel[] = {"steering-wheel"};
    auto items =
        m_screen->LoadItems(renderer, release / "menu-items" / "controllerscreenitems", wheel);
    const RenderModel *controller = m_screen->LoadModel(renderer, release / CONTROLLER_MODEL);
    if (!items || controller == nullptr) {
        return false;
    }
    for (const char *name : TYPE_NAMES) {
        m_controllerScreenItems.push_back({name, MENU_ITEM_SPINNING, controller});
    }
    m_controllerScreenItems.push_back(std::move(items->front()));
    for (const char *name : PRESET_NAMES) {
        m_controllerScreenItems.push_back({name, MENU_ITEM_SPINNING, controller});
    }
    for (std::size_t i = 0; i < m_controllerLayoutPictures.size(); ++i) {
        m_controllerLayoutPictures[i] = LoadLayoutPicture(
            renderer, release / "data" / ("g_texControllerLayout" + std::to_string(i) + ".png"));
        if (!m_controllerLayoutPictures[i]) {
            return false;
        }
    }
    return true;
}

void Menus::DrawControllerLayoutDiagram(SceneRenderer &renderer, int item) const {
    TextStyle style;
    style.size = LABEL_TEXT_SIZE;
    style.colors = m_textCustomColors;
    const Matrix4 flat = MenuScreen::FlatFor(renderer);
    if (item >= CONSOLE_LAYOUTS) {
        style.align = TextStyle::Align::Center;
        int y = PRESET_LINE_Y;
        for (const char *line : PRESET_LINES[static_cast<std::size_t>(item - CONSOLE_LAYOUTS)]) {
            m_screen->Text().Draw(renderer, line, 160, y, style, flat);
            y += PRESET_LINE_STEP;
        }
        return;
    }
    // The picture twice in black, as a shadow, and once in white over it, all in the same place.
    const RenderTexture &picture = *m_controllerLayoutPictures[static_cast<std::size_t>(item)];
    int x = item == CONSOLE_LAYOUTS - 1 ? PICTURE_X - 1 : PICTURE_X;
    int y = item == CONSOLE_LAYOUTS - 1 ? PICTURE_Y - 4 : PICTURE_Y;
    DrawPicture(renderer, picture, x, y, 0x00);
    DrawPicture(renderer, picture, x, y, 0x00);
    DrawPicture(renderer, picture, x, y, 0xFF);
    for (const LayoutLabel &label : LAYOUT_LABELS[static_cast<std::size_t>(item)]) {
        style.align = label.align;
        m_screen->Text().Draw(renderer, label.text, label.x, label.y, style, flat);
    }
}

void Menus::ProcessControllerConfigScreen(SceneRenderer &renderer,
                                          float seconds,
                                          const MenuButtons &first) {
    int player = m_screen->ChoosingPlayer();
    if (m_controllerScreenInitState <= 0) {
        m_screen->Reset();
        m_screen->titleImage = MenuTitleImage{&m_screen->TitleBar(), 22, 9, 280, 42};
        m_screen->postDraw = [this](SceneRenderer &frame, int item) {
            DrawControllerLayoutDiagram(frame, item);
        };
        // Open on the preset of the Controls section, or on the layout of the player without one.
        if (m_controllerScreenInitState == 0) {
            m_controllerScreenSelection =
                std::clamp(ProfileOf(player).controllerLayout, 0, CONSOLE_LAYOUTS - 1);
            std::optional<std::string> name;
            if (m_config != nullptr) {
                name = m_config->Get("Controls", CONTROL_PRESET_KEY);
            }
            for (std::size_t item = 0; name && item < ITEM_PRESETS.size(); ++item) {
                if (SDL_strcasecmp(name->c_str(),
                                   CONTROL_PRESETS[ITEM_PRESETS[item]].name.data()) == 0) {
                    m_controllerScreenSelection = static_cast<int>(item);
                }
            }
        }
        m_controllerScreenInitState = 1;
    }

    MenuResult result = m_screen->Process(renderer,
                                          seconds,
                                          {&first, 1},
                                          {.title = player != 0 ? "Controller 2" : "Controller 1",
                                           .items = m_controllerScreenItems,
                                           .selection = &m_controllerScreenSelection,
                                           .isSelectable = {},
                                           .top = {0x08, 0x00, 0x00},
                                           .bottom = {0xA0, 0x80, 0x20}});
    if (result == MenuResult::Chosen) {
        int item = m_controllerScreenSelection;
        if (item < CONSOLE_LAYOUTS) {
            ProfileOf(player).controllerLayout = item;
        }
        // The race of the SDL game reads the bindings of the preset, at once and at every start.
        const ControlPreset &preset = CONTROL_PRESETS[ITEM_PRESETS[static_cast<std::size_t>(item)]];
        if (m_config != nullptr) {
            m_config->Set("Controls", CONTROL_PRESET_KEY, std::string(preset.name));
            LoadControls(*m_config);
            if (!m_config->Save(m_configPath)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                             "%s cannot be written.",
                             m_configPath.string().c_str());
            }
        }
    }
    // Either way, go back to the options.
    if (result == MenuResult::Chosen || result == MenuResult::Cancelled) {
        SetCurrentScreen(Screen::OptionsMenu);
        m_controllerScreenInitState = 0;
    }
}

} // namespace coastline
