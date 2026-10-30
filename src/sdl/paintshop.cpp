#include "paintshop.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iterator>
#include <span>
#include <sstream>
#include <string_view>

#include <SDL3/SDL.h>

#include "common/input.h"
#include "consolesegment.h"
#include "frontend.h"
#include "gpu.h"
#include "menuscreen.h"

namespace coastline {

namespace {

// The decal slot the paint shop opens on, and the brush and colour it opens with.
constexpr u8 DEFAULT_DECAL_SLOT = 2;
constexpr u8 DEFAULT_BRUSH_SIZE = 2;
constexpr u8 DEFAULT_DECAL_COLOR = 3;

// Where the cursor starts.
constexpr s32 CURSOR_START_X = 244;
constexpr s32 CURSOR_START_Y = 272;

// Where the cursor may go.
constexpr s32 CURSOR_MIN_X = 0x1F;
constexpr s32 CURSOR_MAX_X = 0x261;
constexpr s32 CURSOR_MIN_Y = 0x16;
constexpr s32 CURSOR_MAX_Y = 0x1CA;

// The decal slots on each side of the car that a mirror copies between.
constexpr u8 SLOT_LEFT_DOOR = 0;
constexpr u8 SLOT_RIGHT_DOOR = 1;
constexpr u8 SLOT_LEFT_FRONT_FENDER = 2;
constexpr u8 SLOT_RIGHT_FRONT_FENDER = 3;
constexpr u8 SLOT_ROOF = 5;
constexpr u8 SLOT_LEFT_REAR_FENDER = 6;
constexpr u8 SLOT_RIGHT_REAR_FENDER = 7;

// The cars that sit higher and lower than the rest in the preview.
constexpr int CAR_RAISED_IN_PREVIEW = 1;
constexpr int CAR_LOWERED_IN_PREVIEW = 10;

// The length of a frame of the console the counts of frames below are in.
constexpr float CONSOLE_FRAME_SECONDS = 1.0f / 60.0f;

// The frames the preview takes to turn from one decal slot to the next.
constexpr u16 PREVIEW_TURN_FRAMES = 16;

// The frames between two writes of the painted images the preview shows.
constexpr int PREVIEW_WRITE_FRAMES = 8;

// The size of the button pictures of the tools, and of the pictures of the styles.
constexpr s32 TOOL_ICON_WIDTH = 64;
constexpr s32 TOOL_ICON_HEIGHT = 14;
constexpr s32 STYLE_ICON_WIDTH = 64;
constexpr s32 STYLE_ICON_HEIGHT = 42;

// The keys of the keyboard that end and shorten the text.
constexpr u8 KEY_BACKSPACE = 47;
constexpr u8 KEY_ENTER = 49;
constexpr int KEYBOARD_COLUMNS = 10;

// The frames a pressed key flashes for.
constexpr u16 KEY_FLASH_FRAMES = 3;

// The longest text of the text tool.
constexpr u8 DECAL_TEXT_LENGTH = 16;

// The note of the decals on the Controller Pak, its size, and the bytes of the palette of the
// paint saved after the decals.
constexpr const char *DECAL_NOTE = "TOP GEAR DECALS";
constexpr std::size_t DECAL_FILE_SIZE = 0x3E00;
constexpr std::size_t DECAL_PALETTE_BYTES = 0x20;

// The bytes of the decals at which the progress bar is full, and the width of the bar.
constexpr s32 DECAL_PROGRESS_FULL = 0x3D00;
constexpr s32 DECAL_PROGRESS_WIDTH = 0xB2;

// The bytes of the texels of the largest decal.
constexpr std::size_t DECAL_BUFFER_SIZE = 0x800;

// The size and the baseline of the title.
constexpr int TITLE_SIZE = 34;
constexpr s32 TITLE_Y = 38;

// The style of an 8-bit sprite that draws it in the primitive colour times its texels.
constexpr u8 SPRITE_STYLE_TINTED = 99;

// The states of the dialog of the Controller Pak. A pak of files is never missing, never another
// pak, and never a Rumble Pak, and the dialog never enters the states for those.
enum PakState : u8 {
    PAK_STATE_FADE_OUT,
    PAK_STATE_WAIT_FADE,
    PAK_STATE_CONFIRM,
    PAK_STATE_INIT_PAK,
    PAK_STATE_REPAIR_ID,
    PAK_STATE_CONFIRM_OVERWRITE,
    PAK_STATE_FIND_FILE,
    PAK_STATE_TRANSFER_DECALS,
    PAK_STATE_TRANSFER_PALETTE,
    PAK_STATE_NEW_PAK,
    PAK_STATE_REMOVE_RUMBLE_PAK,
    PAK_STATE_DONE,
    PAK_STATE_FINISH,
};

// The frames the dialog shows its progress before it moves the first decal, and the frames after
// the sounds fade before the pak is read.
constexpr u8 PAK_TRANSFER_DELAY_FRAMES = 2;
constexpr u8 PAK_FADE_WAIT_FRAMES = 3;

// The buttons that dismiss the messages of the dialog.
constexpr u32 PAK_DISMISS_BUTTONS = INPUT_Z | INPUT_B | INPUT_A;

// The result of the dialog when the player cancels it, and the error of a transfer the pak
// rejected.
constexpr s32 PAK_DIALOG_CANCELLED = 999;
constexpr s32 PAK_ERROR_TRANSFER = 99999;

// The choices of the menu of the clear tool: bit 1 for every decal, bit 0 for the body colour.
constexpr u8 CLEAR_OPTIONS = 4;
constexpr u8 CLEAR_ALL_DECALS = 2;
constexpr u8 CLEAR_TO_BODY_COLOR = 1;

// The choices of the menu of the mirror tool.
constexpr u8 MIRROR_OPTIONS = 3;
constexpr u8 MIRROR_OPTION_MIRROR = 0;
constexpr u8 MIRROR_OPTION_FLIP_HORIZONTAL = 1;
constexpr u8 MIRROR_OPTION_FLIP_VERTICAL = 2;

// The stick presses a direction past 0.6, then again after half a second and every 0.2 seconds
// while it stays past 0.3.
constexpr f32 STICK_PRESS = 0.6f;
constexpr f32 STICK_RELEASE = 0.3f;
constexpr s32 STICK_FIRST_DELAY = 500;
constexpr s32 STICK_REPEAT_DELAY = 200;

// The size of a ScreenRect of the console and of a DecalGlyph.
constexpr u32 RECT_SIZE = 0x10;
constexpr u32 GLYPH_SIZE = 0x1C;

// The pictures and the tables of the paint shop in the main segment of a release.
struct PaintShopAddresses {
    std::string_view release;
    u32 background;
    u32 logo;
    u32 paintWholeCar;
    u32 panel8Overlay;
    u32 panel9Overlay;
    u32 brushSizeRound;
    u32 brushSizeSquare;
    u32 paintCursor;
    u32 fillCursor;
    u32 brushCursor;
    u32 crosshairCursor;
    u32 textCursor;
    u32 eyedropperCursor;
    u32 keyboardPanel;
    u32 keyboardBackground;
    u32 keyboardHighlight;
    u32 buttonA;
    u32 buttonB;
    u32 decalFont;
    u32 shapeFilledCircle;
    u32 decalCanvasFrame;
    u32 paletteFrameLeft;
    u32 paletteFrameRight;
    u32 brushSizeRects;
    u32 brushSizeButtonRects;
    u32 decalGlyphs;
    u32 decalSlotIcons;
    u32 rectStyleIcons;
    u32 ovalStyleIcons;
    u32 paintToolIcons;
    u32 colorEditorBars;
    u32 colorChannelNames;
    u32 brushStyleNames;
    u32 textStyleNames;
    u32 rectStyleNames;
    u32 ovalStyleNames;
    u32 panelViewDirections;
    u32 dashColorA;
};

const std::array<PaintShopAddresses, 3> RELEASE_ADDRESSES{{
    {"usa",      0x8028CB40, 0x8028CB70, 0x8028CBA0, 0x8028CDB0, 0x8028CDE0, 0x8028CE10, 0x8028CE40,
     0x8028D110, 0x8028D140, 0x8028D170, 0x8028D1A0, 0x8028D1D0, 0x8028D200, 0x8028D230, 0x8028D260,
     0x8028D290, 0x8028D0B0, 0x8028D0E0, 0x8028D2C0, 0x8028D410, 0x8028D470, 0x8028D480, 0x8028D490,
     0x8028D4A0, 0x8028D4F0, 0x8028D540, 0x8028DB0C, 0x8028DB34, 0x8028DB44, 0x8028DAD4, 0x8028DCC8,
     0x8028DCD4, 0x8028DCE4, 0x8028DCEC, 0x8028DCF4, 0x8028DD04, 0x8028DC08, 0x8028DAB8},
    {"eur",      0x8028F2A0, 0x8028F2D0, 0x8028F300, 0x8028F510, 0x8028F540, 0x8028F570, 0x8028F5A0,
     0x8028F870, 0x8028F8A0, 0x8028F8D0, 0x8028F900, 0x8028F930, 0x8028F960, 0x8028F990, 0x8028F9C0,
     0x8028F9F0, 0x8028F810, 0x8028F840, 0x8028FA20, 0x8028FB70, 0x8028FBD0, 0x8028FBE0, 0x8028FBF0,
     0x8028FC00, 0x8028FC50, 0x8028FCA0, 0x8029026C, 0x80290294, 0x802902A4, 0x80290234, 0x80290468,
     0x80290474, 0x80290484, 0x8029048C, 0x80290494, 0x802904A4, 0x80290368, 0x80290218},
    // The Japanese release shows no names of the styles.
    {"jpn",      0x8028ECF0, 0x8028ED20, 0x8028ED50, 0x8028EF60, 0x8028EF90, 0x8028EFC0, 0x8028EFF0,
     0x8028F2E0, 0x8028F310, 0x8028F340, 0x8028F370, 0x8028F3A0, 0x8028F3D0, 0x8028F400, 0x8028F430,
     0x8028F460, 0x8028F280, 0x8028F2B0, 0x8028F490, 0x8028F5E0, 0x8028F640, 0x8028F650, 0x8028F660,
     0x8028F670, 0x8028F6C0, 0x8028F710, 0x8028FCDC, 0x8028FD04, 0x8028FD14, 0x8028FCA4, 0x8028FEE8,
     0x8028FEF4, 0,          0,          0,          0,          0x8028FDD8, 0x8028FC88},
}};

PaintRect ReadRect(const ConsoleSegment &segment, u32 address) {
    return {segment.Signed(address),
            segment.Signed(address + 4),
            segment.Signed(address + 8),
            segment.Signed(address + 12)};
}

// A Sprite of the console as the main segment initialises it.
PaintSprite ReadSprite(const ConsoleSegment &segment, u32 address) {
    PaintSprite sprite;
    sprite.asset = segment.AssetName(segment.Word(address + 4)).value_or("");
    sprite.bitsPerTexel = segment.Byte(address + 12);
    sprite.width = segment.Signed(address + 16);
    sprite.height = segment.Signed(address + 20);
    sprite.originX = segment.Signed(address + 28);
    sprite.originY = segment.Signed(address + 32);
    sprite.drawWidth = segment.Signed(address + 36);
    sprite.drawHeight = segment.Signed(address + 40);
    sprite.style = segment.Byte(address + 44);
    return sprite;
}

std::array<u8, 4> PaletteRgba(u16 color) {
    auto expand = [](u32 bits) { return static_cast<u8>(bits << 3 | bits >> 2); };
    return {expand(color >> 11 & 0x1F),
            expand(color >> 6 & 0x1F),
            expand(color >> 1 & 0x1F),
            static_cast<u8>((color & 1) != 0 ? 0xFF : 0)};
}

// A colour of 8 bits a channel as the fill colour of the console stores it, 5 bits a channel.
u8 FillChannel(u8 value) {
    u8 bits = value >> 3;
    return static_cast<u8>(bits << 3 | bits >> 2);
}

// The colour of a sprite of 4 bits a texel is the primitive colour, its alpha the intensity.
constexpr std::array<std::uint32_t, 2> INTENSITY_COMBINE = CombineLerp(ZERO_AB,
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

// The background lerps from the environment colour, black, to the primitive colour by its
// intensity, opaque.
constexpr std::array<std::uint32_t, 2> BACKGROUND_COMBINE = CombineLerp(PRIMITIVE,
                                                                        ENVIRONMENT,
                                                                        TEXEL0,
                                                                        ENVIRONMENT,
                                                                        ZERO_ALPHA,
                                                                        ZERO_ALPHA,
                                                                        ZERO_ALPHA,
                                                                        6,
                                                                        PRIMITIVE,
                                                                        ENVIRONMENT,
                                                                        TEXEL0,
                                                                        ENVIRONMENT,
                                                                        ZERO_ALPHA,
                                                                        ZERO_ALPHA,
                                                                        ZERO_ALPHA,
                                                                        6);

// A tinted sprite is its texels times the primitive colour.
constexpr std::array<std::uint32_t, 2> TINTED_COMBINE = CombineLerp(TEXEL0,
                                                                    ZERO_AB,
                                                                    PRIMITIVE,
                                                                    ZERO_D,
                                                                    ZERO_ALPHA,
                                                                    ZERO_ALPHA,
                                                                    ZERO_ALPHA,
                                                                    TEXEL0,
                                                                    TEXEL0,
                                                                    ZERO_AB,
                                                                    PRIMITIVE,
                                                                    ZERO_D,
                                                                    ZERO_ALPHA,
                                                                    ZERO_ALPHA,
                                                                    ZERO_ALPHA,
                                                                    TEXEL0);

// Any other sprite is its texels as they are.
constexpr std::array<std::uint32_t, 2> TEXEL_COMBINE = CombineLerp(ZERO_AB,
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

// The decal is its texels, opaque.
constexpr std::array<std::uint32_t, 2> DECAL_COMBINE = CombineLerp(ZERO_AB,
                                                                   ZERO_AB,
                                                                   ZERO_D,
                                                                   TEXEL0,
                                                                   ZERO_ALPHA,
                                                                   ZERO_ALPHA,
                                                                   ZERO_ALPHA,
                                                                   6,
                                                                   ZERO_AB,
                                                                   ZERO_AB,
                                                                   ZERO_D,
                                                                   TEXEL0,
                                                                   ZERO_ALPHA,
                                                                   ZERO_ALPHA,
                                                                   ZERO_ALPHA,
                                                                   6);

// The rectangles of fill colour are the colour of their vertices, opaque.
constexpr std::array<std::uint32_t, 2> FILL_COMBINE = CombineLerp(ZERO_AB,
                                                                  ZERO_AB,
                                                                  ZERO_D,
                                                                  SHADE,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  6,
                                                                  ZERO_AB,
                                                                  ZERO_AB,
                                                                  ZERO_D,
                                                                  SHADE,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  6);

// The camera of the preview: the camera of the race with a frame of 320 by 200, its field of view
// and its far plane those of SetupCameraMatrix(), squeezed into a viewport of the 320 by 240
// screen centred at 4:3 in the frame.
Matrix4 PreviewCamera(float aspect, const PaintVector &eye, int x, int y, int width, int height) {
    constexpr float CAMERA_FOV = 0.5235987901687622f;
    constexpr float ASPECT_4_3 = 1.3333334f;
    constexpr float CAMERA_NEAR = 2.4f;
    constexpr float CAMERA_FAR = 400.0f * 0.2f;
    constexpr float FRAME_WIDTH = 320.0f;
    constexpr float FRAME_HEIGHT = 200.0f;
    float length = std::sqrt(eye.x * eye.x + eye.y * eye.y + eye.z * eye.z);
    std::array<float, 3> forward{-eye.x / length, -eye.y / length, -eye.z / length};
    std::array<float, 3> side{forward[1], -forward[0], 0.0f};
    float sideLength = std::hypot(side[0], side[1]);
    side = {side[0] / sideLength, side[1] / sideLength, 0.0f};
    std::array<float, 3> up{side[1] * forward[2] - side[2] * forward[1],
                            side[2] * forward[0] - side[0] * forward[2],
                            side[0] * forward[1] - side[1] * forward[0]};
    auto dot = [&](const std::array<float, 3> &a) {
        return a[0] * eye.x + a[1] * eye.y + a[2] * eye.z;
    };
    Matrix4 view{side[0],
                 up[0],
                 -forward[0],
                 0,
                 side[1],
                 up[1],
                 -forward[1],
                 0,
                 side[2],
                 up[2],
                 -forward[2],
                 0,
                 -dot(side),
                 -dot(up),
                 dot(forward),
                 1};
    float fovy = CAMERA_FOV * ASPECT_4_3 * (FRAME_HEIGHT / FRAME_WIDTH);
    float focal = 1.0f / std::tan(fovy * 0.5f);
    Matrix4 projection{focal / (FRAME_WIDTH / FRAME_HEIGHT),
                       0,
                       0,
                       0,
                       0,
                       focal,
                       0,
                       0,
                       0,
                       0,
                       CAMERA_FAR / (CAMERA_NEAR - CAMERA_FAR),
                       -1,
                       0,
                       0,
                       CAMERA_NEAR * CAMERA_FAR / (CAMERA_NEAR - CAMERA_FAR),
                       0};
    auto [across, down] = ScreenExtent(aspect);
    float left = (static_cast<float>(x) / SCREEN_WIDTH * 2.0f - 1.0f) * across;
    float right = (static_cast<float>(x + width) / SCREEN_WIDTH * 2.0f - 1.0f) * across;
    float top = (1.0f - static_cast<float>(y) / SCREEN_HEIGHT * 2.0f) * down;
    float bottom = (1.0f - static_cast<float>(y + height) / SCREEN_HEIGHT * 2.0f) * down;
    Matrix4 viewport{(right - left) / 2.0f,
                     0,
                     0,
                     0,
                     0,
                     (top - bottom) / 2.0f,
                     0,
                     0,
                     0,
                     0,
                     1,
                     0,
                     (right + left) / 2.0f,
                     (top + bottom) / 2.0f,
                     0,
                     1};
    return MultiplyMatrices(viewport, MultiplyMatrices(projection, view));
}

void Normalize(PaintVector &v) {
    float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (length > 0.0f) {
        v = {v.x / length, v.y / length, v.z / length};
    }
}

PaintVector Cross(const PaintVector &a, const PaintVector &b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

float Dot(const PaintVector &a, const PaintVector &b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

} // namespace

PaintShop::PaintShop(MenuScreen &screen,
                     std::optional<std::array<int, 6>> &textColors,
                     ControllerPak pak,
                     std::filesystem::path work)
    : m_screen(screen), m_textColors(textColors), m_pak(std::move(pak)), m_work(std::move(work)) {
}

PaintShop::~PaintShop() = default;

std::unique_ptr<PaintShop> PaintShop::Load(SceneRenderer &renderer,
                                           const std::filesystem::path &release,
                                           MenuScreen &screen,
                                           std::optional<std::array<int, 6>> &textColors,
                                           ControllerPak pak,
                                           std::filesystem::path work) {
    auto name = release.filename().string();
    const auto at = std::ranges::find(RELEASE_ADDRESSES, name, &PaintShopAddresses::release);
    auto loaded = ConsoleSegment::Load(release);
    if (!loaded || at == RELEASE_ADDRESSES.end() || !loaded->Has(at->dashColorA, 12) ||
        !loaded->Has(at->decalGlyphs, 50 * GLYPH_SIZE)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s has no main segment of a release with the paint shop.",
                     release.string().c_str());
        return nullptr;
    }
    std::unique_ptr<PaintShop> shop(
        new PaintShop(screen, textColors, std::move(pak), std::move(work)));
    PaintShop &s = *shop;
    s.m_release = release;
    const ConsoleSegment &segment = *loaded;
    auto sprite = [&](u32 address) {
        auto [entry, added] = s.m_sprites.try_emplace(address);
        if (added) {
            entry->second = ReadSprite(segment, address);
        }
        return &entry->second;
    };
    s.m_background = sprite(at->background);
    s.m_logo = sprite(at->logo);
    s.m_paintWholeCar = sprite(at->paintWholeCar);
    s.m_panel8Overlay = sprite(at->panel8Overlay);
    s.m_panel9Overlay = sprite(at->panel9Overlay);
    s.m_brushSizeRound = sprite(at->brushSizeRound);
    s.m_brushSizeSquare = sprite(at->brushSizeSquare);
    s.m_paintCursor = sprite(at->paintCursor);
    s.m_fillCursor = sprite(at->fillCursor);
    s.m_brushCursor = sprite(at->brushCursor);
    s.m_crosshairCursor = sprite(at->crosshairCursor);
    s.m_textCursor = sprite(at->textCursor);
    s.m_eyedropperCursor = sprite(at->eyedropperCursor);
    s.m_keyboardPanel = sprite(at->keyboardPanel);
    s.m_keyboardBackground = sprite(at->keyboardBackground);
    s.m_keyboardHighlight = sprite(at->keyboardHighlight);
    s.m_buttonA = sprite(at->buttonA);
    s.m_buttonB = sprite(at->buttonB);
    s.m_decalFont = sprite(at->decalFont);
    s.m_shapeFilledCircle = sprite(at->shapeFilledCircle);
    for (std::size_t i = 0; i < s.m_decalSlotIcons.size(); ++i) {
        s.m_decalSlotIcons[i] = sprite(segment.Word(at->decalSlotIcons + static_cast<u32>(i) * 4));
    }
    for (std::size_t i = 0; i < s.m_paintToolIcons.size(); ++i) {
        s.m_paintToolIcons[i] = sprite(segment.Word(at->paintToolIcons + static_cast<u32>(i) * 4));
    }
    for (std::size_t i = 0; i < 4; ++i) {
        s.m_rectStyleIcons[i] = sprite(segment.Word(at->rectStyleIcons + static_cast<u32>(i) * 4));
        s.m_ovalStyleIcons[i] = sprite(segment.Word(at->ovalStyleIcons + static_cast<u32>(i) * 4));
    }
    s.m_decalCanvasFrame = ReadRect(segment, at->decalCanvasFrame);
    s.m_paletteFrameLeft = ReadRect(segment, at->paletteFrameLeft);
    s.m_paletteFrameRight = ReadRect(segment, at->paletteFrameRight);
    for (u32 i = 0; i < 5; ++i) {
        s.m_brushSizeRects[i] = ReadRect(segment, at->brushSizeRects + i * RECT_SIZE);
        s.m_brushSizeButtonRects[i] = ReadRect(segment, at->brushSizeButtonRects + i * RECT_SIZE);
    }
    for (u32 i = 0; i < s.m_decalGlyphs.size(); ++i) {
        u32 glyph = at->decalGlyphs + i * GLYPH_SIZE;
        s.m_decalGlyphs[i] = {segment.Signed(glyph),
                              segment.Signed(glyph + 4),
                              segment.Byte(glyph + 8),
                              segment.Signed(glyph + 12),
                              segment.Signed(glyph + 16),
                              segment.Signed(glyph + 20),
                              segment.Signed(glyph + 24)};
    }
    for (u32 i = 0; i < 9; ++i) {
        s.m_colorEditorBars[i / 3][i % 3] = segment.Byte(at->colorEditorBars + i);
    }
    auto names3 = [&](u32 table, auto &into) {
        for (u32 i = 0; i < into.size(); ++i) {
            into[i] = table == 0 ? std::string() : segment.Text(segment.Word(table + i * 4));
        }
    };
    names3(at->colorChannelNames, s.m_colorChannelNames);
    names3(at->brushStyleNames, s.m_brushStyleNames);
    names3(at->textStyleNames, s.m_textStyleNames);
    names3(at->rectStyleNames, s.m_rectStyleNames);
    names3(at->ovalStyleNames, s.m_ovalStyleNames);
    for (u32 i = 0; i < DECAL_SLOTS; ++i) {
        u32 direction = at->panelViewDirections + i * 12;
        s.m_panelViewDirections[i] = {
            segment.Float(direction), segment.Float(direction + 4), segment.Float(direction + 8)};
    }
    s.m_dashColorA = segment.Byte(at->dashColorA);
    s.m_dashColorB = segment.Byte(at->dashColorA + 4);
    s.m_brushSize = segment.Byte(at->dashColorA + 8);
    s.m_undoBuffer.assign(DECAL_BUFFER_SIZE, 0);
    s.m_scratchTexels.assign(DECAL_BUFFER_SIZE, 0);
    // AllocateEditorBuffers() of the console loads the buttons for the dialogs of the car select
    // screen.
    if (!s.LoadTextureFromRom(*s.m_buttonA) || !s.LoadTextureFromRom(*s.m_buttonB)) {
        return nullptr;
    }
    // The reflection wraps across the sky, as its tile wraps.
    s.m_previewReflection =
        RenderTexture::Load(renderer, release / "data" / "menu-reflection.png", true, true);
    if (!s.m_previewReflection) {
        return nullptr;
    }
    s.ResetPaintShopState();
    return shop;
}

void PaintShop::SetCar(int car, const std::filesystem::path &directory) {
    m_carModel = car;
    m_carDirectory = directory;
}

// The drawing of the console.

void PaintShop::BeginDraw(SceneRenderer &renderer) {
    m_renderer = &renderer;
    m_flat = MenuScreen::FlatFor(renderer);
    m_fills.clear();
    m_lastFrameTextures = std::move(m_frameTextures);
    m_frameTextures.clear();
}

void PaintShop::FlushFills() {
    if (m_fills.empty()) {
        return;
    }
    FlatMaterial fill;
    fill.combine = FILL_COMBINE;
    m_renderer->DrawQuads(m_fills, fill, m_flat);
    m_fills.clear();
}

void PaintShop::FillRect(s32 x0, s32 y0, s32 x1, s32 y1, u8 red, u8 green, u8 blue) {
    // A rectangle of the fill cycle includes its far column and row.
    std::array<u8, 4> color{FillChannel(red), FillChannel(green), FillChannel(blue), 0xFF};
    auto left = static_cast<float>(x0);
    auto top = static_cast<float>(y0);
    auto right = static_cast<float>(x1 + 1);
    auto bottom = static_cast<float>(y1 + 1);
    m_fills.insert(m_fills.end(),
                   {{left, top, 0.0f, 0.0f, color},
                    {right, top, 0.0f, 0.0f, color},
                    {right, bottom, 0.0f, 0.0f, color},
                    {left, bottom, 0.0f, 0.0f, color}});
}

void PaintShop::DrawFilledRectHiRes(
    s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue) {
    if (width < 2) {
        width = 1;
    }
    if (height < 2) {
        height = 1;
    }
    FillRect(x, y, x + width, y + height, red, green, blue);
}

void PaintShop::DrawRectOutline(
    s32 x, s32 y, s32 width, s32 height, s32 thickness, u8 red, u8 green, u8 blue) {
    if (thickness < 2) {
        thickness = 1;
    }
    FillRect(x, y, x + width, y + thickness, red, green, blue);
    FillRect(x, y + height - thickness, x + width, y + height, red, green, blue);
    FillRect(x, y, x + thickness, y + height, red, green, blue);
    FillRect(x + width - thickness, y, x + width, y + height, red, green, blue);
}

void PaintShop::DrawRectBorder(s32 x,
                               s32 y,
                               s32 width,
                               s32 height,
                               s32 border,
                               u8 transparent,
                               u8 sunken,
                               u8 red,
                               u8 green,
                               u8 blue) {
    u8 light = 0xE0;
    u8 dark = 0x30;
    if (border < 2) {
        border = 1;
    }
    if (!transparent) {
        FillRect(x, y, x + width, y + height, red, green, blue);
    }
    if (sunken == 1) {
        std::swap(light, dark);
    }
    for (s32 i = 1; i <= border; ++i) {
        FillRect(x - i, y - i, x - i + 1, y + height + i, light, light, light);
    }
    for (s32 i = 1; i <= border; ++i) {
        FillRect(x - i, y - i, x + width + i, y - i + 1, light, light, light);
    }
    for (s32 i = 1; i <= border; ++i) {
        FillRect(x + width + i - 1, y - i, x + width + i, y + height + i, dark, dark, dark);
    }
    for (s32 i = 1; i <= border; ++i) {
        FillRect(x - i, y + height + i - 1, x + width + i, y + height + i, dark, dark, dark);
    }
}

void PaintShop::DrawPixel(s32 x, s32 y, u8 shade) {
    FillRect(x, y, x + 1, y + 1, shade, shade, shade);
}

void PaintShop::DrawSpriteQuad(PaintSprite &sprite,
                               float x,
                               float y,
                               float width,
                               float height,
                               float s0,
                               float t0,
                               float s1,
                               float t1,
                               u8 red,
                               u8 green,
                               u8 blue) {
    if (sprite.pixels.empty() || sprite.width <= 0 || sprite.height <= 0) {
        return;
    }
    if (!sprite.texture) {
        // The texels upright, from the bottom row of the cartridge up.
        std::vector<u8> rgba;
        rgba.reserve(static_cast<std::size_t>(sprite.width * sprite.height * 4));
        for (s32 row = sprite.height - 1; row >= 0; --row) {
            for (s32 column = 0; column < sprite.width; ++column) {
                auto index = static_cast<std::size_t>(row * sprite.width + column);
                if (sprite.bitsPerTexel == 4) {
                    u8 pair = sprite.pixels[index / 2];
                    auto level = static_cast<u8>(((column & 1) != 0 ? pair & 0xF : pair >> 4) * 17);
                    rgba.insert(rgba.end(), {level, level, level, level});
                } else if (sprite.bitsPerTexel == 8) {
                    u8 texel = sprite.pixels[index];
                    auto level = static_cast<u8>((texel >> 4) * 17);
                    rgba.insert(rgba.end(),
                                {level, level, level, static_cast<u8>((texel & 0xF) * 17)});
                } else {
                    auto color = static_cast<u16>(sprite.pixels[index * 2] << 8 |
                                                  sprite.pixels[index * 2 + 1]);
                    // DrawSpriteStrip() loads the logo as intensity and alpha.
                    if (&sprite == m_logo) {
                        u8 level = sprite.pixels[index * 2];
                        rgba.insert(rgba.end(),
                                    {level, level, level, sprite.pixels[index * 2 + 1]});
                    } else {
                        auto texel = PaletteRgba(color);
                        rgba.insert(rgba.end(), texel.begin(), texel.end());
                    }
                }
            }
        }
        sprite.texture =
            RenderTexture::FromTexels(*m_renderer, sprite.width, sprite.height, rgba, true, false);
        if (!sprite.texture) {
            return;
        }
    }
    FlushFills();
    FlatMaterial material;
    material.texel0 = sprite.texture.get();
    if (sprite.bitsPerTexel == 4) {
        material.combine = &sprite == m_background ? BACKGROUND_COMBINE : INTENSITY_COMBINE;
        material.env = {0.0f, 0.0f, 0.0f, 1.0f};
    } else if (sprite.bitsPerTexel == 8 && sprite.style == SPRITE_STYLE_TINTED) {
        material.combine = TINTED_COMBINE;
    } else {
        material.combine = TEXEL_COMBINE;
    }
    material.prim = {red / 255.0f, green / 255.0f, blue / 255.0f, 1.0f};
    auto w = static_cast<float>(sprite.width);
    auto h = static_cast<float>(sprite.height);
    std::array<FlatVertex, 4> corners{{{x, y, s0 / w, t0 / h},
                                       {x + width, y, s1 / w, t0 / h},
                                       {x + width, y + height, s1 / w, t1 / h},
                                       {x, y + height, s0 / w, t1 / h}}};
    m_renderer->DrawQuads(corners, material, m_flat);
}

void PaintShop::DrawSpriteStrips(
    PaintSprite &sprite, s32 x, s32 y, s32 drawWidth, s32 drawHeight, u8 red, u8 green, u8 blue) {
    if (sprite.bitsPerTexel != 4 && sprite.bitsPerTexel != 8 && sprite.bitsPerTexel != 16) {
        return;
    }
    // DrawSpriteStrip() steps one texel short of the width and of the height of each strip over
    // the rectangle, the top strip being the last in memory. The strips are drawn as one quad.
    bool mirrored = drawWidth < 0;
    if (mirrored) {
        drawWidth = -drawWidth;
    }
    auto width = static_cast<float>(sprite.width);
    auto height = static_cast<float>(sprite.height);
    float s0 = mirrored ? width - 0.5f : 0.5f;
    float s1 = mirrored ? 0.5f : width - 0.5f;
    DrawSpriteQuad(sprite,
                   static_cast<float>(x),
                   static_cast<float>(y),
                   static_cast<float>(drawWidth),
                   static_cast<float>(drawHeight),
                   s0,
                   0.5f,
                   s1,
                   height - 0.5f,
                   red,
                   green,
                   blue);
}

void PaintShop::DrawSpriteAtOrigin(PaintSprite &sprite) {
    DrawSpriteStrips(
        sprite, sprite.originX, sprite.originY, sprite.drawWidth, sprite.drawHeight, 0, 0, 0);
}

void PaintShop::DrawTexturedSprite(PaintSprite &sprite, s32 x, s32 y) {
    DrawSpriteStrips(sprite, x, y, sprite.drawWidth, sprite.drawHeight, 0, 0, 0);
}

void PaintShop::DrawSpriteTinted(PaintSprite &sprite, u8 red, u8 green, u8 blue) {
    DrawSpriteStrips(sprite,
                     sprite.originX,
                     sprite.originY,
                     sprite.drawWidth,
                     sprite.drawHeight,
                     red,
                     green,
                     blue);
}

void PaintShop::DrawSpriteRegion(
    PaintSprite &sprite, s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue) {
    DrawSpriteStrips(sprite, x, y, width, height, red, green, blue);
}

void PaintShop::DrawScaledTexturedRect(PaintSprite &sprite,
                                       s32 srcX,
                                       s32 srcY,
                                       s32 srcWidth,
                                       s32 srcHeight,
                                       s32 x,
                                       s32 y,
                                       s32 width,
                                       s32 height,
                                       u8 red,
                                       u8 green,
                                       u8 blue) {
    // The first column drawn is the one before the source rectangle, and the rows run up from
    // the bottom of the source rectangle in memory.
    float top = static_cast<float>(sprite.height - srcY - srcHeight);
    DrawSpriteQuad(sprite,
                   static_cast<float>(x),
                   static_cast<float>(y),
                   static_cast<float>(width),
                   static_cast<float>(height),
                   static_cast<float>(srcX - 1) + 0.5f,
                   top + 0.5f,
                   static_cast<float>(srcX + srcWidth - 2) + 0.5f,
                   top + static_cast<float>(srcHeight) - 0.5f,
                   red,
                   green,
                   blue);
}

void PaintShop::EmitScaledTextureRect(s32 x, s32 y, s32 width, s32 height) {
    // The decal of the slot in the colours of its palette, the rows from the last upwards.
    DecalTexture texture = m_car->Texture(m_decalSlot);
    std::vector<u8> rgba;
    rgba.reserve(static_cast<std::size_t>(texture.width * texture.height * 4));
    for (s32 row = texture.height - 1; row >= 0; --row) {
        for (s32 column = 0; column < texture.width; ++column) {
            u8 texel = ReadPixel4Bit(m_car->Texels(m_decalSlot), column, row);
            auto color = PaletteRgba(m_car->PaletteColor(m_decalSlot, texel));
            rgba.insert(rgba.end(), color.begin(), color.end());
        }
    }
    auto decal =
        RenderTexture::FromTexels(*m_renderer, texture.width, texture.height, rgba, true, false);
    if (!decal) {
        return;
    }
    FlushFills();
    FlatMaterial material;
    material.texel0 = decal.get();
    material.combine = DECAL_COMBINE;
    auto left = static_cast<float>(x);
    auto top = static_cast<float>(y);
    auto right = static_cast<float>(x + width);
    auto bottom = static_cast<float>(y + height);
    std::array<FlatVertex, 4> corners{{{left, top, 0.0f, 0.0f},
                                       {right, top, 1.0f, 0.0f},
                                       {right, bottom, 1.0f, 1.0f},
                                       {left, bottom, 0.0f, 1.0f}}};
    m_renderer->DrawQuads(corners, material, m_flat);
    m_frameTextures.push_back(std::move(decal));
}

void PaintShop::DrawText(const std::string &text, s32 x, s32 y) {
    FlushFills();
    TextStyle style = m_text;
    style.colors = m_textColors;
    m_screen.Text().Draw(*m_renderer, text, x, y, style, m_flat);
}

void PaintShop::SetTextCustomColors(u8 red0, u8 green0, u8 blue0, u8 red1, u8 green1, u8 blue1) {
    m_textColors = std::array<int, 6>{red0, green0, blue0, red1, green1, blue1};
}

// The input of the player.

bool PaintShop::DialogShows() const {
    return m_paintTool == PAINT_TOOL_MIRROR || m_paintTool == PAINT_TOOL_CLEAR ||
           m_paintTool == PAINT_TOOL_LOAD || m_paintTool == PAINT_TOOL_SAVE ||
           m_paintTool == PAINT_TOOL_EXIT || m_styleMenuOpen || m_colorEditorOpen;
}

void PaintShop::TranslateInput(float seconds, const PaintShopInput &input, bool dialog) {
    m_frameMilliseconds = static_cast<s32>(seconds * 1000.0f);
    m_consoleFrameTime += seconds;
    m_consoleFrame = m_consoleFrameTime >= CONSOLE_FRAME_SECONDS;
    if (m_consoleFrame) {
        // A slow frame advances a count once, not once for each 60th it took.
        m_consoleFrameTime = std::fmod(m_consoleFrameTime, CONSOLE_FRAME_SECONDS);
    }
    m_input.buttons = input.buttons;
    // In a menu or a dialog of the console, Enter chooses as A does. The keyboard of the text tool
    // reads Enter from the typed text instead.
    if (dialog && input.enter) {
        m_input.buttons |= INPUT_A;
    }
    // A press of Escape cancels a dialog and the text of the text tool as B does. Elsewhere it
    // chooses DONE, as the button does.
    bool escape = input.escape && !m_escapeDown;
    m_escapeDown = input.escape;
    bool typing = m_paintTool == PAINT_TOOL_TEXT && (m_decalDragActive || m_paintToolStarted);
    if ((dialog || typing) && escape) {
        m_input.buttons |= INPUT_B;
    } else if (escape && m_initialized && !typing && !m_paintShopExiting) {
        m_paintToolStarted = 0;
        m_previousPaintTool = m_paintTool;
        m_paintTool = PAINT_TOOL_EXIT;
    }
    m_input.stickX = std::clamp(input.stickX, -1.0f, 1.0f);
    m_input.stickY = std::clamp(input.stickY, -1.0f, 1.0f);
    ConsumeInputFlags(&m_input);
    m_typedText = input.text;
    m_pointerMoved = input.pointer.has_value();
    if (!m_pointerMoved) {
        return;
    }
    m_pointer = *input.pointer;
    m_pointerSeen = true;
    // The mouse moves the cursor of the paint shop, except while a menu or a dialog shows. The
    // cursor of the console stays still there, and the mouse picks the options of the menu.
    if (!dialog && !m_paintShopExiting) {
        m_paintCursor->originX = std::clamp(m_pointer[0], CURSOR_MIN_X, CURSOR_MAX_X);
        m_paintCursor->originY = std::clamp(m_pointer[1], CURSOR_MIN_Y, CURSOR_MAX_Y);
    }
}

void PaintShop::Transfer(u32 mask) {
    TransferFlags(&m_input, mask);
}

void PaintShop::CheckStick(s32 &timer, f32 axis, f32 sign, u32 button) {
    // CheckAnalogStickPositive() and CheckAnalogStickNegative() of the console. A positive time
    // counts down to a press, and a negative one counts up to it.
    axis *= sign;
    s32 time = timer;
    if (time == 0) {
        if (axis > STICK_PRESS) {
            m_input.buttons |= button;
            timer = STICK_FIRST_DELAY;
        }
    } else if (axis < STICK_RELEASE) {
        timer = 0;
    } else if (time > 0) {
        timer = time - m_frameMilliseconds;
        if (timer <= 0) {
            m_input.buttons |= button;
            timer = -STICK_REPEAT_DELAY;
        }
    } else {
        timer = time + m_frameMilliseconds;
        if (timer >= 0) {
            m_input.buttons |= button;
            timer = -STICK_REPEAT_DELAY;
        }
    }
}

void PaintShop::UpdateStickRepeat() {
    CheckStick(m_input.repeatUp, m_input.stickY, 1.0f, INPUT_UP);
    CheckStick(m_input.repeatDown, m_input.stickY, -1.0f, INPUT_DOWN);
    CheckStick(m_input.repeatLeft, m_input.stickX, -1.0f, INPUT_LEFT);
    CheckStick(m_input.repeatRight, m_input.stickX, 1.0f, INPUT_RIGHT);
}

void PaintShop::PickMenuOption(const PaintRect *options, int count, u8 &cursor) {
    if (!m_pointerMoved) {
        return;
    }
    for (int i = 0; i < count; ++i) {
        const PaintRect &rect = options[i];
        if (m_pointer[0] >= rect.x && m_pointer[0] <= rect.x + rect.width &&
            m_pointer[1] >= rect.y && m_pointer[1] <= rect.y + rect.height) {
            cursor = static_cast<u8>(i);
        }
    }
}

// paintshop.c of the console.

bool PaintShop::LoadTextureFromRom(PaintSprite &sprite) {
    auto expected =
        static_cast<std::size_t>(sprite.width * sprite.height * sprite.bitsPerTexel) >> 3;
    sprite.texture.reset();
    auto path = m_release / (sprite.asset + ".bin");
    std::ifstream file(path, std::ios::binary);
    if (file) {
        sprite.pixels.assign(std::istreambuf_iterator<char>(file), {});
    } else if (!sprite.asset.empty()) {
        // The import converts a picture it found a texture descriptor of to PNG, and the texels
        // are built again from the image.
        path.replace_extension(".png");
        SDL_Surface *loaded = SDL_LoadPNG(path.string().c_str());
        SDL_Surface *rgba = loaded ? SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32) : nullptr;
        SDL_DestroySurface(loaded);
        sprite.pixels.clear();
        if (rgba != nullptr && rgba->w == sprite.width && rgba->h == sprite.height) {
            sprite.pixels.assign(expected, 0);
            for (s32 row = 0; row < sprite.height; ++row) {
                const auto *line = static_cast<const u8 *>(rgba->pixels) +
                                   static_cast<std::ptrdiff_t>(row) * rgba->pitch;
                for (s32 column = 0; column < sprite.width; ++column) {
                    auto index =
                        static_cast<std::size_t>((sprite.height - 1 - row) * sprite.width + column);
                    const u8 *texel = line + column * 4;
                    if (sprite.bitsPerTexel == 4) {
                        u8 level = texel[0] >> 4;
                        sprite.pixels[index / 2] |= (column & 1) != 0 ? level : level << 4;
                    } else if (sprite.bitsPerTexel == 8) {
                        sprite.pixels[index] = static_cast<u8>((texel[0] & 0xF0) | texel[3] >> 4);
                    } else if (&sprite == m_logo) {
                        sprite.pixels[index * 2] = texel[0];
                        sprite.pixels[index * 2 + 1] = texel[3];
                    } else {
                        auto color = static_cast<u16>((texel[0] >> 3) << 11 | (texel[1] >> 3) << 6 |
                                                      (texel[2] >> 3) << 1 | texel[3] >> 7);
                        sprite.pixels[index * 2] = static_cast<u8>(color >> 8);
                        sprite.pixels[index * 2 + 1] = static_cast<u8>(color);
                    }
                }
            }
        }
        SDL_DestroySurface(rgba);
    }
    if (sprite.pixels.size() != expected) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "AllocPaintShopGfxMem given image %dx%dx%d = %zu bytes but %s is %zu bytes.",
                     sprite.width,
                     sprite.height,
                     sprite.bitsPerTexel,
                     expected,
                     path.string().c_str(),
                     sprite.pixels.size());
        sprite.pixels.clear();
        return false;
    }
    return true;
}

void PaintShop::RenderDecalPreview3D() {
    // The car turns in front of a still camera.
    if (m_decalSlotChanged) {
        PaintVector from = m_panelViewDirections[m_previousDecalSlot];
        PaintVector to = m_panelViewDirections[m_decalSlot];
        if (m_consoleFrame) {
            ++m_screenFrameCount;
        }
        Normalize(from);
        Normalize(to);
        if (Dot(to, from) < -0.9) {
            // Nearly opposite directions turn about the vertical through a side step.
            PaintVector side = Cross({0.0f, 0.0f, 1.0f}, from);
            if (m_screenFrameCount < PREVIEW_TURN_FRAMES / 2) {
                to = {to.x + side.x, to.y + side.y, to.z + side.z};
            } else {
                from = {from.x + side.x, from.y + side.y, from.z + side.z};
            }
        }
        auto interpolate = [&](const PaintVector &a, const PaintVector &b) {
            auto frame = static_cast<float>(m_screenFrameCount);
            return PaintVector{a.x + frame * ((b.x - a.x) / 16.0f),
                               a.y + frame * ((b.y - a.y) / 16.0f),
                               a.z + frame * ((b.z - a.z) / 16.0f)};
        };
        auto upOf = [](u8 slot) {
            // The up axis of the car that shows a decal slot, tilted for the roof.
            return slot == SLOT_ROOF ? PaintVector{5.0f, 5.0f, 1.5f} :
                                       PaintVector{0.0f, 0.0f, 1.0f};
        };
        m_frameForward = interpolate(from, to);
        Normalize(m_frameForward);
        PaintVector toUp = upOf(m_decalSlot);
        PaintVector fromUp = upOf(m_previousDecalSlot);
        Normalize(fromUp);
        Normalize(toUp);
        m_frameUp = interpolate(fromUp, toUp);
        Normalize(m_frameUp);
        if (m_screenFrameCount == PREVIEW_TURN_FRAMES) {
            m_screenFrameCount = 0;
            m_decalSlotChanged = 0;
        }
    } else {
        m_frameForward = m_panelViewDirections[m_decalSlot];
        Normalize(m_frameForward);
    }
    m_frameSide = Cross(m_frameUp, m_frameForward);
    Normalize(m_frameSide);
    m_frameUp = Cross(m_frameForward, m_frameSide);
    m_framePosition = {m_frameUp.x * -0.65f, m_frameUp.y * -0.65f, m_frameUp.z * -0.65f};

    // The images of the painted car are written again for the preview a few frames apart.
    if (m_previewWait > 0 && m_consoleFrame) {
        --m_previewWait;
    }
    if ((m_previewDirty && m_previewWait == 0) || !m_preview) {
        m_previewDirty = false;
        m_previewWait = PREVIEW_WRITE_FRAMES;
        if (auto images = m_car->WriteImages(m_work)) {
            ModelOptions options;
            options.parts = [](const std::string &part) {
                return std::ranges::find(STAGE_CAR_PARTS, part) != STAGE_CAR_PARTS.end();
            };
            options.images = *images;
            if (auto model =
                    RenderModel::Load(*m_renderer, m_carDirectory / "model.gltf", options)) {
                m_preview = std::move(model);
            }
        }
    }
    if (!m_preview) {
        return;
    }
    // The viewport of the preview, on the 320 by 240 screen.
    int halfHeight = 120;
    int halfWidth = 160;
    int top = halfHeight - 6;
    if (m_carModel == CAR_RAISED_IN_PREVIEW) {
        top = halfHeight - 3;
    } else if (m_carModel == CAR_LOWERED_IN_PREVIEW) {
        top = halfHeight + 2;
    }
    GpuWindow &window = m_renderer->Window();
    float aspect = static_cast<float>(window.FrameWidth()) /
                   static_cast<float>(std::max<Uint32>(window.FrameHeight(), 1));
    Matrix4 camera = PreviewCamera(aspect, m_cameraPosition, 0, top, halfWidth - 4, halfHeight - 3);
    SDL_Rect scissor =
        FrameRect(window.FrameWidth(), window.FrameHeight(), 0, top, halfWidth - 4, halfHeight - 3);
    Matrix4 frame{m_frameForward.x,
                  m_frameForward.y,
                  m_frameForward.z,
                  0,
                  m_frameSide.x,
                  m_frameSide.y,
                  m_frameSide.z,
                  0,
                  m_frameUp.x,
                  m_frameUp.y,
                  m_frameUp.z,
                  0,
                  m_framePosition.x,
                  m_framePosition.y,
                  m_framePosition.z,
                  1};
    FlushFills();
    m_renderer->DrawReflectingModel(
        *m_preview,
        MultiplyMatrices(frame, RotationX(HALF_PI)),
        camera,
        STAGE_CAR_LIGHTING,
        StageReflection(m_previewReflection.get(),
                        {m_cameraPosition.x, m_cameraPosition.y, m_cameraPosition.z}),
        scissor);
}

void PaintShop::SelectDecalTexels(int slot) {
    DecalTexture texture = m_car->Texture(slot);
    m_decalTexels = m_car->Texels(slot);
    m_decalTexelsEnd = m_car->End();
    m_decalMask = m_car->Mask(slot);
    m_decalWidth = static_cast<u8>(texture.width);
    m_decalHeight = static_cast<u8>(texture.height);
}

void PaintShop::SetUpPaintShopCar() {
    m_car = DecalCar::Load(m_carDirectory);
    m_preview.reset();
    m_previewDirty = true;
    m_framePosition = {0.0f, 0.0f, 0.0f};
    m_frameUp = {0.0f, 0.0f, 1.0f};
    m_frameForward = {0.0f, 1.0f, 0.0f};
    m_frameSide = {-1.0f, 0.0f, 0.0f};
    m_cameraPosition = m_carModel == CAR_LOWERED_IN_PREVIEW ? PaintVector{7.3f, 7.3f, 2.0f} :
                                                              PaintVector{6.5f, 6.5f, 2.0f};
}

void PaintShop::LoadDecalSlotIcons() {
    // The right panels share the texels of the left ones.
    LoadTextureFromRom(*m_decalSlotIcons[SLOT_LEFT_DOOR]);
    m_decalSlotIcons[SLOT_RIGHT_DOOR]->pixels = m_decalSlotIcons[SLOT_LEFT_DOOR]->pixels;
    LoadTextureFromRom(*m_decalSlotIcons[SLOT_LEFT_FRONT_FENDER]);
    m_decalSlotIcons[SLOT_RIGHT_FRONT_FENDER]->pixels =
        m_decalSlotIcons[SLOT_LEFT_FRONT_FENDER]->pixels;
    LoadTextureFromRom(*m_decalSlotIcons[4]);
    LoadTextureFromRom(*m_decalSlotIcons[5]);
    LoadTextureFromRom(*m_decalSlotIcons[SLOT_LEFT_REAR_FENDER]);
    m_decalSlotIcons[SLOT_RIGHT_REAR_FENDER]->pixels =
        m_decalSlotIcons[SLOT_LEFT_REAR_FENDER]->pixels;
    LoadTextureFromRom(*m_decalSlotIcons[8]);
    LoadTextureFromRom(*m_decalSlotIcons[9]);
    LoadTextureFromRom(*m_panel8Overlay);
    m_panel8Overlay->originX = m_decalSlotIcons[8]->originX;
    m_panel8Overlay->originY = m_decalSlotIcons[8]->originY;
    LoadTextureFromRom(*m_panel9Overlay);
    m_panel9Overlay->originX = m_decalSlotIcons[9]->originX;
    m_panel9Overlay->originY = m_decalSlotIcons[9]->originY + 8;
    for (int i = 0; i < DECAL_SLOTS; ++i) {
        PaintSprite &icon = *m_decalSlotIcons[static_cast<std::size_t>(i)];
        icon.texture.reset();
        PaintRect &rect = m_decalSlotRects[static_cast<std::size_t>(i)];
        switch (i) {
        case SLOT_LEFT_DOOR:
        case SLOT_RIGHT_DOOR:
            rect = {icon.originX + 5, icon.originY + 2, icon.width - 10, icon.height - 4};
            break;
        case SLOT_LEFT_FRONT_FENDER:
        case SLOT_RIGHT_FRONT_FENDER:
            rect = {icon.originX + 4, icon.originY + 1, icon.width - 9, icon.height - 4};
            break;
        case 4:
            rect = {icon.originX + 6, icon.originY + 4, icon.width - 12, icon.height - 8};
            break;
        case SLOT_ROOF:
            rect = {icon.originX + 4, icon.originY + 2, icon.width - 8, icon.height - 4};
            break;
        case SLOT_LEFT_REAR_FENDER:
        case SLOT_RIGHT_REAR_FENDER:
            rect = {icon.originX + 2, icon.originY + 2, icon.width - 6, icon.height - 4};
            break;
        case 8:
            rect = {icon.originX + 2, icon.originY + 2, icon.width - 4, icon.height - 4};
            break;
        default:
            rect = {icon.originX + 4, icon.originY + 2, icon.width - 8, icon.height - 4};
            break;
        }
        icon.style = SPRITE_STYLE_TINTED;
    }
}

void PaintShop::LayOutPaintShopButtons() {
    PaintRect &sizes = m_paintToolButtonRects[0];
    sizes = {m_decalCanvasFrame.x, m_decalCanvasFrame.y + m_decalCanvasFrame.height + 6, 32, 94};
    m_paletteFrameLeft.x = sizes.x + sizes.width + 2;
    m_paletteFrameLeft.y = sizes.y;
    m_paletteFrameRight.x = m_paletteFrameLeft.x + m_paletteFrameLeft.width + 14;
    m_paletteFrameRight.y = sizes.y;
    s32 y = sizes.y + m_paletteFrameLeft.height + 2;
    for (int row = 0; row < 3; ++row, y += 24) {
        s32 x = m_paletteFrameLeft.x;
        for (int column = 0; column < 4; ++column, x += 74) {
            m_paintToolButtonRects[static_cast<std::size_t>(1 + row * 4 + column)] = {x, y, 72, 22};
        }
    }
}

void PaintShop::LoadPaintToolIcons() {
    m_brushSizeRound->originY = m_paintToolButtonRects[0].y + 4;
    m_brushSizeSquare->originY = m_paintToolButtonRects[0].y + 4;
    m_brushSizeRound->originX = m_paintToolButtonRects[0].x;
    m_brushSizeSquare->originX = m_paintToolButtonRects[0].x;
    LoadTextureFromRom(*m_brushSizeRound);
    LoadTextureFromRom(*m_brushSizeSquare);
    for (std::size_t i = 1; i < PAINT_TOOLS; ++i) {
        PaintSprite &icon = *m_paintToolIcons[i];
        icon.width = TOOL_ICON_WIDTH;
        icon.height = TOOL_ICON_HEIGHT;
        icon.drawWidth = icon.width;
        icon.drawHeight = icon.height;
        icon.originX = m_paintToolButtonRects[i].x + 4;
        icon.originY = m_paintToolButtonRects[i].y + 4;
        icon.style = 0;
        LoadTextureFromRom(icon);
    }
}

void PaintShop::LoadStyleIcons() {
    for (std::size_t i = 0; i < 4; ++i) {
        for (PaintSprite *icon : {m_rectStyleIcons[i], m_ovalStyleIcons[i]}) {
            icon->bitsPerTexel = 4;
            icon->width = STYLE_ICON_WIDTH;
            icon->height = STYLE_ICON_HEIGHT;
            icon->drawWidth = icon->width;
            icon->drawHeight = icon->height;
            LoadTextureFromRom(*icon);
        }
    }
}

void PaintShop::LayOutPaintShopControls() {
    m_brushSizeButtonRects[0].x = m_brushSizeRound->originX;
    m_brushSizeButtonRects[0].y = m_brushSizeRound->originY;
    for (std::size_t i = 1; i < m_brushSizeButtonRects.size(); ++i) {
        m_brushSizeButtonRects[i].x = m_brushSizeButtonRects[0].x;
        m_brushSizeButtonRects[i].y =
            m_brushSizeButtonRects[i - 1].y + m_brushSizeButtonRects[i - 1].height;
    }
    for (s32 i = 0; i < 2; ++i) {
        m_swatches[static_cast<std::size_t>(i)].rect = {
            m_paletteFrameLeft.x + 4 + 20 * i, m_paletteFrameLeft.y + 4, 20, 14};
    }
    for (s32 i = 2; i < static_cast<s32>(m_swatches.size()); ++i) {
        m_swatches[static_cast<std::size_t>(i)].rect = {
            m_paletteFrameRight.x + 4 + 16 * (i - 2), m_paletteFrameRight.y + 4, 16, 14};
    }
    ConvertPaletteToRgb();
    s32 pitchX = m_keyboardHighlight->drawWidth + 4;
    s32 pitchY = m_keyboardHighlight->drawHeight + 4;
    for (int row = 0; row < static_cast<int>(m_decalGlyphs.size()) / KEYBOARD_COLUMNS; ++row) {
        for (int column = 0; column < KEYBOARD_COLUMNS; ++column) {
            PaintGlyph &key =
                m_decalGlyphs[static_cast<std::size_t>(row * KEYBOARD_COLUMNS + column)];
            key.x = m_keyboardBackground->originX + column * pitchX;
            key.y = m_keyboardBackground->originY + row * pitchY;
        }
    }
}

bool PaintShop::InitPaintShop() {
    SetUpPaintShopCar();
    if (!m_car) {
        return false;
    }
    bool loaded = LoadTextureFromRom(*m_background) && LoadTextureFromRom(*m_logo) &&
                  LoadTextureFromRom(*m_paintWholeCar);
    m_paintWholeCar->style = SPRITE_STYLE_TINTED;
    LoadDecalSlotIcons();
    LayOutPaintShopButtons();
    LoadPaintToolIcons();
    for (PaintSprite *sprite : {m_paintCursor,
                                m_fillCursor,
                                m_brushCursor,
                                m_crosshairCursor,
                                m_textCursor,
                                m_eyedropperCursor,
                                m_keyboardPanel,
                                m_keyboardBackground,
                                m_keyboardHighlight,
                                m_buttonA,
                                m_buttonB,
                                m_decalFont}) {
        loaded = LoadTextureFromRom(*sprite) && loaded;
    }
    m_keyboardBackground->originX = m_keyboardPanel->originX + 8;
    m_keyboardBackground->originY = m_keyboardPanel->originY + 8;
    m_keyboardBackground->drawWidth = m_keyboardPanel->drawWidth - 16;
    m_keyboardBackground->drawHeight = m_keyboardPanel->drawHeight - 16;
    LoadStyleIcons();
    SelectDecalTexels(m_decalSlot);
    m_decalPaletteMaterial = m_decalSlot;
    LayOutPaintShopControls();
    m_decalCanvas.width = m_decalWidth * 4;
    m_decalCanvas.height = m_decalHeight * 4;
    m_decalCanvas.x =
        m_decalCanvasFrame.x + ((m_decalCanvasFrame.width - m_decalCanvas.width) >> 1);
    m_decalCanvas.y =
        m_decalCanvasFrame.y + ((m_decalCanvasFrame.height - m_decalCanvas.height) >> 1);
    m_initialized = true;
    m_controllerPakState = 0;
    m_screen.FadeIn();
    return loaded;
}

void PaintShop::DrawPaintShopButton(const PaintRect &rect, u8 red, u8 green, u8 blue) {
    // A button is a frame in a colour around a yellow face.
    DrawFilledRectHiRes(rect.x, rect.y, rect.width, rect.height, red, green, blue);
    DrawFilledRectHiRes(rect.x + 4, rect.y + 4, rect.width - 8, rect.height - 8, 0xFF, 0xCA, 0);
}

void PaintShop::DrawBrushSizes(PaintSprite &sizes, const PaintRect &chosen) {
    // The rows of the picture run from the bottom up.
    DrawScaledTexturedRect(sizes,
                           0,
                           sizes.originY + sizes.height - chosen.y - chosen.height,
                           sizes.width,
                           chosen.height,
                           sizes.originX - 1,
                           chosen.y,
                           sizes.width,
                           chosen.height,
                           0xFF,
                           0xCA,
                           0);
}

void PaintShop::DrawPaintShop() {
    DrawSpriteTinted(*m_background, 0x71, 0x0B, 0x40);
    DrawSpriteAtOrigin(*m_logo);
    m_text.size = TITLE_SIZE;
    m_text.align = TextStyle::Align::Center;
    m_text.shadow = false;
    DrawText("%ryPAINT SHOP", 160, TITLE_Y);
    for (std::size_t i = 0; i < DECAL_SLOTS; ++i) {
        if (i == m_decalSlot) {
            DrawSpriteTinted(*m_decalSlotIcons[i], 0x20, 0xC8, 0xFF);
        } else {
            DrawSpriteTinted(*m_decalSlotIcons[i], 0x20, 0x10, 0xFF);
        }
    }
    DrawSpriteTinted(*m_panel8Overlay, 0xFF, 0xFF, 0xFF);
    DrawSpriteTinted(*m_panel9Overlay, 0xC0, 0, 0);
    DrawFilledRectHiRes(m_decalCanvasFrame.x,
                        m_decalCanvasFrame.y,
                        m_decalCanvasFrame.width,
                        m_decalCanvasFrame.height,
                        0,
                        0,
                        0);
    EmitScaledTextureRect(
        m_decalCanvas.x, m_decalCanvas.y, m_decalCanvas.width, m_decalCanvas.height);
    DrawDecalColorPalette();
    DrawPaintShopButton(m_paintToolButtonRects[0], 0, 0, 0);
    PaintSprite &sizes =
        m_paintToolIcons[PAINT_TOOL_BRUSH]->style == 0 ? *m_brushSizeRound : *m_brushSizeSquare;
    DrawSpriteAtOrigin(sizes);
    const PaintRect &chosen = m_brushSizeButtonRects[m_brushSize];
    DrawFilledRectHiRes(chosen.x, chosen.y, chosen.width, chosen.height, 0, 0, 0);
    DrawBrushSizes(sizes, chosen);
    for (std::size_t i = PAINT_TOOL_BRUSH; i < PAINT_TOOLS; ++i) {
        if (i == m_paintTool) {
            DrawPaintShopButton(m_paintToolButtonRects[i], 0x20, 0xC8, 0xFF);
        } else {
            DrawPaintShopButton(m_paintToolButtonRects[i], 0, 0, 0);
        }
        DrawSpriteAtOrigin(*m_paintToolIcons[i]);
    }
}

void PaintShop::DrawToolCursor() {
    PaintSprite &cursor = *m_paintCursor;
    switch (m_paintTool) {
    case PAINT_TOOL_BRUSH:
        DrawTexturedSprite(m_eyedropperActive ? *m_eyedropperCursor : *m_brushCursor,
                           cursor.originX + 1,
                           cursor.originY - 24);
        break;
    case PAINT_TOOL_FILL:
        if (m_eyedropperActive) {
            DrawTexturedSprite(*m_eyedropperCursor, cursor.originX, cursor.originY - 24);
        } else {
            DrawTexturedSprite(*m_fillCursor, cursor.originX - 6, cursor.originY - 23);
        }
        break;
    case PAINT_TOOL_LINE:
    case PAINT_TOOL_RECTANGLE:
    case PAINT_TOOL_OVAL:
        if (m_eyedropperActive) {
            DrawTexturedSprite(*m_eyedropperCursor, cursor.originX, cursor.originY - 24);
        } else {
            DrawTexturedSprite(*m_crosshairCursor, cursor.originX - 8, cursor.originY - 8);
        }
        break;
    case PAINT_TOOL_TEXT:
        if (m_decalDragActive) {
            DrawSpriteAtOrigin(cursor);
        } else if (m_eyedropperActive) {
            DrawTexturedSprite(*m_eyedropperCursor, cursor.originX, cursor.originY - 24);
        } else {
            DrawTexturedSprite(*m_textCursor,
                               cursor.originX - (m_textCursor->drawWidth >> 1),
                               cursor.originY - m_textCursor->drawHeight);
        }
        break;
    default:
        DrawSpriteAtOrigin(cursor);
        break;
    }
}

void PaintShop::RunPaintShopPakDialog(u8 operation) {
    if (m_controllerPakState == 0) {
        bool done = false;
        m_controllerPakState = ProcessDecalSaveLoad(*m_car, operation, 0, done);
        return;
    }
    DrawPakErrorMessage(m_controllerPakState, operation, 0);
    if ((m_input.buttons & (INPUT_A | INPUT_B)) != 0) {
        Transfer(INPUT_A | INPUT_B);
        m_controllerPakState = 0;
        m_paintTool = m_previousPaintTool;
    }
}

void PaintShop::RunPaintShopMenus() {
    switch (m_paintTool) {
    case PAINT_TOOL_MIRROR:
        ProcessMirrorFlipMenu();
        break;
    case PAINT_TOOL_CLEAR:
        ProcessFillClearMenu();
        break;
    case PAINT_TOOL_LOAD:
    case PAINT_TOOL_SAVE:
        RunPaintShopPakDialog(m_paintTool);
        break;
    case PAINT_TOOL_EXIT:
        ProcessPaintShopExitConfirm();
        break;
    default:
        if (m_styleMenuOpen) {
            if (m_paintTool == PAINT_TOOL_BRUSH) {
                ProcessBrushStyleMenu();
            } else if (m_paintTool == PAINT_TOOL_TEXT) {
                ProcessTextStyleMenu();
            } else if (m_paintTool == PAINT_TOOL_RECTANGLE) {
                ProcessRectStyleMenu();
            } else if (m_paintTool == PAINT_TOOL_OVAL) {
                ProcessOvalStyleMenu();
            }
        } else if (m_colorEditorOpen) {
            ProcessColorEditorDialog();
        } else {
            ProcessDecalEditorInput();
            UpdateCursorFromInput();
            DrawSpriteAtOrigin(*m_paintCursor);
        }
        break;
    }
}

void PaintShop::HandlePaintShopB() {
    if ((m_input.buttons & INPUT_B) == 0) {
        m_eyedropperActive = 0;
        return;
    }
    if (IsCursorInRect(m_decalCanvas) && !m_paintToolStarted && !m_decalDragActive) {
        // The rows of the decal run from the bottom up.
        m_decalColor =
            GetPixel4Bit((m_paintCursor->originX - m_decalCanvas.x) >> 2,
                         ((m_decalCanvas.y + m_decalCanvas.height) - m_paintCursor->originY) >> 2);
        m_eyedropperActive = 1;
    } else if (m_paintToolStarted || m_decalDragActive) {
        Transfer(INPUT_B);
        m_decalDragActive = 0;
        m_paintToolStarted = 0;
        m_showPreview3D = 1;
        m_paletteEnabled = 1;
        m_decalTextLength = 0;
        m_decalTextWidth = 0;
    }
}

bool PaintShop::Frame(SceneRenderer &renderer, float seconds, const PaintShopInput &input) {
    BeginDraw(renderer);
    bool dialog = DialogShows();
    TranslateInput(seconds, input, dialog);
    if (!m_initialized && !InitPaintShop()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "The paint shop cannot open %s.",
                     m_carDirectory.string().c_str());
        m_initialized = false;
        return true;
    }
    m_screen.UpdateFade(seconds);
    DrawPaintShop();
    if (m_showPreview3D) {
        RenderDecalPreview3D();
    }
    // A dialog runs wherever the cursor is, as Escape opens the exit prompt with the cursor over
    // the decal.
    if (IsCursorInRect(m_decalCanvas) && !DialogShows()) {
        ProcessDecalEditorInput();
        UpdateCursorFromInput();
        DrawToolCursor();
    } else {
        RunPaintShopMenus();
    }
    // The console draws no cursor over a menu or a dialog, where the mouse picks the options. The
    // cursor shows at the mouse whenever a mouse is connected.
    if (dialog && !m_paintShopExiting && (m_pointerSeen || SDL_HasMouse())) {
        if (m_pointerSeen) {
            DrawTexturedSprite(*m_paintCursor, m_pointer[0], m_pointer[1]);
        } else {
            DrawSpriteAtOrigin(*m_paintCursor);
        }
    }
    FlushFills();
    m_screen.DrawFade(renderer);
    HandlePaintShopB();
    if (m_paintShopExiting && m_screen.FadingOut() && m_screen.FadeComplete()) {
        m_paintShopExiting = 0;
        ResetPaintShopState();
        m_initialized = false;
        // The preview drawn this frame stays until the next entry, as the frame draws it at End().
        m_controllerPakState = 0;
        return true;
    }
    return false;
}

bool PaintShop::TakesText() const {
    return m_initialized && m_paintTool == PAINT_TOOL_TEXT && m_decalDragActive &&
           !m_paintToolStarted;
}

void PaintShop::ResetPaintShopState() {
    m_paintCursor->originX = CURSOR_START_X;
    m_paintCursor->originY = CURSOR_START_Y;
    m_decalSlot = DEFAULT_DECAL_SLOT;
    m_previousDecalSlot = m_decalSlot;
    m_undoDecalSlot = m_decalSlot;
    m_brushSize = DEFAULT_BRUSH_SIZE;
    m_savedColor = 0;
    m_decalColor = DEFAULT_DECAL_COLOR;
    m_previousPaintTool = PAINT_TOOL_BRUSH;
    m_paintTool = PAINT_TOOL_BRUSH;
    m_screenFrameCount = 0;
    m_paintToolIcons[PAINT_TOOL_RECTANGLE]->style = 0;
    m_paintToolIcons[PAINT_TOOL_OVAL]->style = 0;
    m_decalEditCount = 0;
    m_paintToolStarted = 0;
    m_decalDragActive = 0;
    m_paletteEnabled = 1;
    m_decalSlotHasMirror = 1;
    m_decalSlotChanged = 0;
    m_colorEditorOpen = 0;
    m_showPreview3D = 1;
    m_undoAvailable = 0;
    m_styleMenuOpen = 0;
    m_textShadowPickerOpen = 0;
    m_eyedropperActive = 0;
}

void PaintShop::SaveDecalUndo() {
    std::memcpy(m_undoBuffer.data(),
                m_decalTexels,
                std::min<std::size_t>(DECAL_BUFFER_SIZE,
                                      static_cast<std::size_t>(m_decalTexelsEnd - m_decalTexels)));
    m_undoDecalSlot = m_decalSlot;
    ++m_decalEditCount;
    m_undoAvailable = 1;
}

void PaintShop::UndoDecalEdit() {
    if (!m_undoAvailable) {
        return;
    }
    u8 *texels = m_car->Texels(m_undoDecalSlot);
    std::memcpy(
        texels,
        m_undoBuffer.data(),
        std::min<std::size_t>(DECAL_BUFFER_SIZE, static_cast<std::size_t>(m_car->End() - texels)));
    --m_decalEditCount;
    m_undoAvailable = 0;
    m_previewDirty = true;
}

void PaintShop::DrawDialogBox(s32 x, s32 y, s32 width, s32 height) {
    DrawRectBorder(x, y, width, height, 3, 0, 0, 0x80, 0x80, 0x80);
}

void PaintShop::DrawSunkenFrame(s32 x, s32 y, s32 width, s32 height) {
    DrawRectBorder(x, y, width, height, 1, 1, 1, 0x80, 0x80, 0x80);
}

void PaintShop::SetPakTextColors(u8 inCarSelect) {
    // The second colour differs on the car select screen.
    if (inCarSelect == 1) {
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    } else {
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
    }
}

void PaintShop::DrawPakErrorMessage(s32 error, u8 operation, u8 inCarSelect) {
    m_text.size = 12;
    SetPakTextColors(inCarSelect);
    m_text.align = TextStyle::Align::Left;
    m_text.shadow = false;
    s32 x = 0;
    s32 y = 0;
    s32 width = 0;
    s32 textY = 0;
    // The widths test for a save and the texts for a load.
    switch (error) {
    case PFS_ERR_NOPACK:
    case PFS_ERR_DEVICE:
        y = inCarSelect == 1 ? 0xEA : 0xCC;
        DrawDialogBox(0xD5, y, 0xD5, 0x48);
        textY = (y + 30) >> 1;
        DrawText("CONTROLLER PAK", 0x72, textY);
        DrawText("IS NOT INSERTED", 0x72, textY + 14);
        break;
    case PFS_DATA_FULL:
    case PFS_DIR_FULL:
        y = inCarSelect == 1 ? 0xC3 : 0x97;
        DrawDialogBox(0xA5, y, 0x135, 0xB2);
        textY = (y + 30) >> 1;
        DrawText("INSUFFICIENT FREE PAGES", 0x5A, textY);
        DrawText("OR FREE NOTES IN THE", 0x5A, textY + 12);
        DrawText("CONTROLLER PAK.", 0x5A, textY + 24);
        DrawText("62 PAGES AND ONE NOTE", 0x5A, textY + 42);
        DrawText("ARE NEEDED TO SAVE THE", 0x5A, textY + 54);
        DrawText("CUSTOM DECALS.", 0x5A, textY + 66);
        break;
    case PFS_ERR_INCONSISTENT:
        width = operation == PAINT_TOOL_SAVE ? 0x106 : 0x116;
        y = inCarSelect == 1 ? 0xE2 : 0xCC;
        x = (640 - width) >> 1;
        DrawDialogBox(x, y, width, 0x48);
        textY = (y + 30) >> 1;
        DrawText(operation == PAINT_TOOL_LOAD ? "UNABLE TO READ FROM" : "UNABLE TO WRITE TO",
                 (x + 16) >> 1,
                 textY);
        DrawText("THE CONTROLLER PAK", (x + 16) >> 1, textY + 14);
        break;
    case PFS_ERR_INVALID:
        width = operation == PAINT_TOOL_SAVE ? 0x10C : 0xF2;
        y = inCarSelect == 1 ? 0xE2 : 0xCC;
        x = (640 - width) >> 1;
        DrawDialogBox(x, y, width, 0x48);
        textY = (y + 30) >> 1;
        if (operation == PAINT_TOOL_LOAD) {
            DrawText("DECALS NOT FOUND", (x + 16) >> 1, textY);
            DrawText("IN CONTROLLER PAK", (x + 16) >> 1, textY + 14);
        } else {
            DrawText("ERROR ENCOUNTERED", (x + 16) >> 1, textY);
            DrawText("WHILE SAVING DECALS", (x + 16) >> 1, textY + 14);
        }
        break;
    case PAK_ERROR_TRANSFER:
        width = operation == PAINT_TOOL_SAVE ? 0x10B : 0x114;
        y = inCarSelect == 1 ? 0xE2 : 0xCC;
        x = (640 - width) >> 1;
        DrawDialogBox(x, y, width, 0x48);
        textY = (y + 30) >> 1;
        DrawText("ERROR ENCOUNTERED", (x + 15) >> 1, textY);
        DrawText(operation == PAINT_TOOL_LOAD ? "WHILE LOADING DECALS" : "WHILE SAVING DECALS",
                 (x + 15) >> 1,
                 textY + 14);
        break;
    case PFS_ERR_BAD_DATA:
        y = inCarSelect == 1 ? 0xE4 : 0xCC;
        DrawDialogBox(0xAD, y, 0x126, 0x48);
        textY = (y + 30) >> 1;
        DrawText("BAD DATA ENCOUNTERED", 0x5D, textY);
        DrawText(operation == PAINT_TOOL_LOAD ? "WHILE LOADING DECALS" : "WHILE SAVING DECALS",
                 0x5D,
                 textY + 14);
        break;
    case PFS_ERR_CONTRFAIL:
        y = inCarSelect == 1 ? 0xDC : 0xB0;
        DrawDialogBox(0xAE, y, 0x124, 0x80);
        textY = (y + 30) >> 1;
        DrawText("CONTROLLER ERROR HAS", 0x5E, textY);
        DrawText("BEEN DETECTED.", 0x5E, textY + 12);
        DrawText("CUSTOM DECALS CANNOT", 0x5E, textY + 30);
        DrawText(operation == PAINT_TOOL_LOAD ? "BE LOADED." : "BE SAVED.", 0x5E, textY + 42);
        break;
    case PFS_ERR_ID_FATAL:
        y = inCarSelect == 1 ? 0xF8 : 0xCC;
        DrawDialogBox(0xCD, y, 0xE6, 0x48);
        textY = (y + 30) >> 1;
        DrawText("CONTROLLER PAK", 0x6E, textY);
        DrawText("IS NONFUNCTIONAL", 0x6E, textY + 14);
        break;
    default:
        break;
    }
}

void PaintShop::DrawMenuButtons(s32 buttonY, s32 cancelX) {
    m_text.size = 10;
    m_text.align = TextStyle::Align::Left;
    s32 textY = (buttonY + 0x12) >> 1;
    DrawText("%wwSELECT", (m_buttonA->width + 0xD9) >> 1, textY);
    DrawText("%wwCANCEL", (cancelX + m_buttonB->width + 6) >> 1, textY);
    DrawTexturedSprite(*m_buttonA, 0xD3, buttonY);
    DrawTexturedSprite(*m_buttonB, cancelX, buttonY);
}

void PaintShop::ClearDecal(u8 coloredOption) {
    // Every texel of the decal selected takes the colour painted with when the choice of the menu
    // is the one given, and the colour of the body otherwise.
    for (s32 y = 0; y < m_decalHeight; ++y) {
        for (s32 x = 0; x < m_decalWidth; ++x) {
            SetPixel4Bit(x,
                         y,
                         m_paintToolIcons[PAINT_TOOL_CLEAR]->style == coloredOption ? m_decalColor :
                                                                                      0);
        }
    }
}

void PaintShop::ProcessFillClearMenu() {
    static constexpr std::array<const char *, 2> SCOPE_TEXTS{"CLEAR CURRENT DECAL",
                                                             "CLEAR ALL DECALS"};
    static constexpr std::array<const char *, 2> COLOR_TEXTS{"TO SELECTED COLOR", "TO BODY COLOR"};
    PaintSprite &clear = *m_paintToolIcons[PAINT_TOOL_CLEAR];
    std::array<PaintRect, CLEAR_OPTIONS> options{};
    s32 buttonY = 0x160 - m_buttonA->height;
    s32 cancelX = 0x161 - m_buttonB->width;
    for (s32 i = 0, x = 0xB9; x < 0x1D1; ++i, x += 0x46) {
        options[static_cast<std::size_t>(i)] = {x, 0xAC, 0x3C, 0x4E};
    }
    PickMenuOption(options.data(), CLEAR_OPTIONS, clear.style);
    DrawDialogBox(0xA7, 0x78, 0x132, 0xF0);
    for (std::size_t i = 0; i < CLEAR_OPTIONS; ++i) {
        const PaintRect &option = options[i];
        if (i == clear.style) {
            DrawRectOutline(option.x, option.y, option.width, option.height, 1, 0, 0xFF, 0xFF);
        } else {
            DrawRectOutline(option.x, option.y, option.width, option.height, 1, 0x20, 0x20, 0x20);
        }
    }
    // The slot is shown at half its size, centred by half its size again.
    PaintSprite &slot = *m_decalSlotIcons[m_decalSlot];
    PaintSprite &car = *m_paintWholeCar;
    s32 slotX = (options[0].width - (slot.width >> 1)) >> 1;
    s32 slotY = (options[0].height - (slot.height >> 1)) >> 1;
    s32 carX = (options[2].width - car.width) >> 1;
    s32 carY = (options[2].height - car.height) >> 1;
    const auto &color = m_swatches[m_decalColor].color;
    const auto &body = m_swatches[0].color;
    DrawSpriteRegion(slot,
                     options[0].x + slotX,
                     options[0].y + slotY,
                     slot.drawWidth >> 1,
                     slot.drawHeight >> 1,
                     color[0],
                     color[1],
                     color[2]);
    DrawSpriteRegion(car,
                     options[2].x + carX,
                     options[2].y + carY,
                     car.drawWidth,
                     car.drawHeight,
                     color[0],
                     color[1],
                     color[2]);
    DrawSpriteRegion(slot,
                     options[1].x + slotX,
                     options[1].y + slotY,
                     slot.drawWidth >> 1,
                     slot.drawHeight >> 1,
                     body[0],
                     body[1],
                     body[2]);
    DrawSpriteRegion(car,
                     options[3].x + carX,
                     options[3].y + carY,
                     car.drawWidth,
                     car.drawHeight,
                     body[0],
                     body[1],
                     body[2]);
    m_text.align = TextStyle::Align::Center;
    m_text.shadow = false;
    m_text.size = 16;
    DrawText("%rySELECT OPTION", 0x9E, 0x4E);
    DrawSunkenFrame(0xBA, options[0].y + options[0].height + 12, 0x10C, 0x3A);
    m_text.size = 11;
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    DrawText(SCOPE_TEXTS[clear.style >> 1], 0x9E, 0x8E);
    DrawText(COLOR_TEXTS[clear.style & CLEAR_TO_BODY_COLOR], 0x9E, 0x9B);
    DrawMenuButtons(buttonY, cancelX);
    UpdateStickRepeat();
    if ((m_input.buttons & INPUT_LEFT) != 0) {
        Transfer(INPUT_LEFT);
        clear.style = clear.style == 0 ? CLEAR_OPTIONS - 1 : clear.style - 1;
    } else if ((m_input.buttons & INPUT_RIGHT) != 0) {
        Transfer(INPUT_RIGHT);
        clear.style = clear.style == CLEAR_OPTIONS - 1 ? 0 : clear.style + 1;
    }
    if ((m_input.buttons & INPUT_A) != 0) {
        Transfer(INPUT_A);
        switch (clear.style) {
        case 0:
        case CLEAR_TO_BODY_COLOR:
            SaveDecalUndo();
            ClearDecal(0);
            m_paintTool = m_previousPaintTool;
            break;
        case CLEAR_ALL_DECALS:
        case CLEAR_ALL_DECALS | CLEAR_TO_BODY_COLOR:
            m_savedDecalSlot = m_decalSlot;
            // The palette and the button of the slot are not switched with its texels.
            for (int i = 0; i < DECAL_SLOTS; ++i) {
                SelectDecalTexels(i);
                ClearDecal(CLEAR_ALL_DECALS);
            }
            m_decalSlot = m_savedDecalSlot;
            SelectDecalTexels(m_decalSlot);
            // The undo buffer is saved after the clear. Undo cannot bring the decal back.
            SaveDecalUndo();
            clear.style = 0;
            m_paintTool = m_previousPaintTool;
            break;
        default:
            clear.style = 0;
            m_paintTool = m_previousPaintTool;
            break;
        }
    } else if ((m_input.buttons & INPUT_B) != 0) {
        Transfer(INPUT_B);
        clear.style = 0;
        m_paintTool = m_previousPaintTool;
    }
}

void PaintShop::ProcessMirrorFlipMenu() {
    static constexpr std::array<const char *, 2> FLIP_TEXTS{"FLIP CURRENT DECAL",
                                                            "FLIP CURRENT DECAL"};
    static constexpr std::array<const char *, 2> AXIS_TEXTS{"HORIZONTALLY", "VERTICALLY"};
    PaintSprite &mirror = *m_paintToolIcons[PAINT_TOOL_MIRROR];
    std::array<PaintRect, MIRROR_OPTIONS> options{};
    // The row of the buttons is placed by the width of the A button.
    s32 buttonY = 0x155 - m_buttonA->width;
    s32 cancelX = 0x163 - m_buttonB->width;
    if (m_paintToolStarted) {
        m_styleMenuCursor = mirror.style;
        m_paintToolStarted = 0;
    }
    options[0] = {0xAB, 0xBF, 0x66, 0x2C};
    for (s32 i = 1; i < MIRROR_OPTIONS; ++i) {
        options[static_cast<std::size_t>(i)] = {
            options[0].x + options[0].width + 98 * i - 90, options[0].y, 90, options[0].height};
    }
    PickMenuOption(options.data(), MIRROR_OPTIONS, m_styleMenuCursor);
    DrawDialogBox(0x99, 0x83, 0x14E, 0xDA);
    m_text.size = 16;
    m_text.align = TextStyle::Align::Center;
    m_text.shadow = false;
    DrawText("%rySELECT OPTION", 0x9F, 0x55);
    m_text.size = 14;
    DrawText(m_decalSlotHasMirror ? "%wwMIRROR" : "%55MIRROR", 0x6E, 0x6E);
    DrawText("%wwFLIP H", 0xA2, 0x6E);
    DrawText("%wwFLIP V", 0xD3, 0x6E);
    for (std::size_t i = 0; i < MIRROR_OPTIONS; ++i) {
        const PaintRect &option = options[i];
        if (i == m_styleMenuCursor) {
            DrawRectOutline(option.x, option.y, option.width, option.height, 1, 0x20, 0xC8, 0xFF);
        } else {
            DrawRectOutline(option.x, option.y, option.width, option.height, 1, 0x20, 0x20, 0x20);
        }
    }
    DrawSunkenFrame(0xAC, 0xF8, 0x128, 0x3A);
    m_text.size = 11;
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    if (m_styleMenuCursor != MIRROR_OPTION_MIRROR) {
        DrawText(FLIP_TEXTS[m_styleMenuCursor - 1], 0x9F, 0x87);
        DrawText(AXIS_TEXTS[m_styleMenuCursor - 1], 0x9F, 0x94);
    } else if (!m_decalSlotHasMirror) {
        DrawText("MIRROR IS NOT APPLICABLE", 0x9F, 0x87);
        DrawText("TO THE CURRENT DECAL", 0x9F, 0x94);
    } else {
        switch (m_decalSlot) {
        case SLOT_LEFT_DOOR:
            DrawText("MIRROR CURRENT DECAL", 0x9F, 0x87);
            DrawText("TO THE RIGHT DOOR", 0x9F, 0x94);
            break;
        case SLOT_RIGHT_DOOR:
            DrawText("MIRROR CURRENT DECAL", 0x9F, 0x87);
            DrawText("TO THE LEFT DOOR", 0x9F, 0x94);
            break;
        case SLOT_LEFT_FRONT_FENDER:
            DrawText("MIRROR CURRENT DECAL TO", 0x9F, 0x87);
            DrawText("THE RIGHT FRONT FENDER", 0x9F, 0x94);
            break;
        case SLOT_RIGHT_FRONT_FENDER:
            DrawText("MIRROR CURRENT DECAL TO", 0x9F, 0x87);
            DrawText("THE LEFT FRONT FENDER", 0x9F, 0x94);
            break;
        case SLOT_LEFT_REAR_FENDER:
            DrawText("MIRROR CURRENT DECAL TO", 0x9F, 0x87);
            DrawText("THE RIGHT REAR FENDER", 0x9F, 0x94);
            break;
        case SLOT_RIGHT_REAR_FENDER:
            DrawText("MIRROR CURRENT DECAL TO", 0x9F, 0x87);
            DrawText("THE LEFT REAR FENDER", 0x9F, 0x94);
            break;
        default:
            break;
        }
    }
    DrawMenuButtons(buttonY, cancelX);
    UpdateStickRepeat();
    if ((m_input.buttons & INPUT_RIGHT) != 0) {
        Transfer(INPUT_RIGHT);
        m_styleMenuCursor = m_styleMenuCursor == MIRROR_OPTIONS - 1 ? 0 : m_styleMenuCursor + 1;
    } else if ((m_input.buttons & INPUT_LEFT) != 0) {
        Transfer(INPUT_LEFT);
        m_styleMenuCursor = m_styleMenuCursor == 0 ? MIRROR_OPTIONS - 1 : m_styleMenuCursor - 1;
    }
    if ((m_input.buttons & INPUT_A) != 0) {
        Transfer(INPUT_A);
        mirror.style = m_styleMenuCursor;
        if (m_styleMenuCursor == MIRROR_OPTION_MIRROR) {
            if (m_decalSlotHasMirror) {
                MirrorDecalToOppositePanel();
            }
        } else {
            FlipDecalPixels();
        }
    } else if ((m_input.buttons & INPUT_B) != 0) {
        Transfer(INPUT_B);
        mirror.style = 0;
        m_styleMenuOpen = 0;
        m_paintTool = m_previousPaintTool;
    }
}

void PaintShop::MirrorDecalToOppositePanel() {
    if (!m_decalSlotHasMirror) {
        m_paintTool = m_previousPaintTool;
        return;
    }
    switch (m_decalSlot) {
    case SLOT_LEFT_DOOR:
        m_mirrorTargetPanel = SLOT_RIGHT_DOOR;
        break;
    case SLOT_RIGHT_DOOR:
        m_mirrorTargetPanel = SLOT_LEFT_DOOR;
        break;
    case SLOT_LEFT_FRONT_FENDER:
        m_mirrorTargetPanel = SLOT_RIGHT_FRONT_FENDER;
        break;
    case SLOT_RIGHT_FRONT_FENDER:
        m_mirrorTargetPanel = SLOT_LEFT_FRONT_FENDER;
        break;
    case SLOT_LEFT_REAR_FENDER:
        m_mirrorTargetPanel = SLOT_RIGHT_REAR_FENDER;
        break;
    case SLOT_RIGHT_REAR_FENDER:
        m_mirrorTargetPanel = SLOT_LEFT_REAR_FENDER;
        break;
    default:
        break;
    }
    m_previousDecalSlot = m_decalSlot;
    const u8 *source = m_car->Texels(m_previousDecalSlot);
    m_decalSlot = m_mirrorTargetPanel;
    // The size and the palette stay those of the source. Each pair of slots shares them.
    m_decalTexels = m_car->Texels(m_decalSlot);
    m_decalMask = m_car->Mask(m_decalSlot);
    SaveDecalUndo();
    for (s32 y = 0; y < m_decalHeight; ++y) {
        for (s32 x = 0; x < m_decalWidth; ++x) {
            SetPixel4Bit(m_decalWidth - x - 1, y, ReadPixel4Bit(source, x, y));
        }
    }
    m_decalSlotChanged = 1;
    m_screenFrameCount = 0;
    m_paintTool = m_previousPaintTool;
}

void PaintShop::FlipDecalPixels() {
    // The undo buffer is the copy the flip reads from.
    SaveDecalUndo();
    PaintSprite &mirror = *m_paintToolIcons[PAINT_TOOL_MIRROR];
    for (s32 y = 0; y < m_decalHeight; ++y) {
        for (s32 x = 0; x < m_decalWidth; ++x) {
            if (mirror.style == MIRROR_OPTION_FLIP_HORIZONTAL) {
                SetPixel4Bit(m_decalWidth - x - 1, y, ReadPixel4Bit(m_undoBuffer.data(), x, y));
            } else if (mirror.style == MIRROR_OPTION_FLIP_VERTICAL) {
                SetPixel4Bit(x, m_decalHeight - y - 1, ReadPixel4Bit(m_undoBuffer.data(), x, y));
            }
        }
    }
    mirror.style = 0;
    m_paintTool = m_previousPaintTool;
}

void PaintShop::DrawPakProgressBox(s32 y, const char *text, s32 textX, s32 textY) {
    DrawDialogBox(0xD4, y, 0xD8, 0x56);
    m_text.align = TextStyle::Align::Left;
    m_text.shadow = false;
    m_text.size = 13;
    SetPakTextColors(0);
    DrawText(text, textX, textY);
    DrawSunkenFrame(0xE6, y + 0x34, 0xB4, 0x14);
}

void PaintShop::CopyUnmaskedTexels(u8 *texels, const u8 *mask, s32 width, s32 height) {
    // A texel copies the whole byte it shares with its neighbour.
    for (s32 y = 0; y < height; ++y) {
        for (s32 x = 0; x < width; ++x) {
            if ((mask[y * ((width + 7) >> 3) + (x >> 3)] & (1 << ((x ^ 7) & 7))) == 0) {
                s32 offset = TexelOffset(x, y, width);
                texels[offset] = m_undoBuffer[static_cast<std::size_t>(offset)];
            }
        }
    }
}

void PaintShop::TransferDecal(DecalCar &car, u8 operation) {
    DecalTexture texture = car.Texture(m_pakDecalIndex);
    s32 width = texture.width;
    s32 height = texture.height;
    // The doors move one row more than they have.
    if (m_pakDecalIndex == SLOT_LEFT_DOOR || m_pakDecalIndex == SLOT_RIGHT_DOOR) {
        ++height;
    }
    m_pakChunkSize = (width * height) >> 1;
    auto offset = static_cast<std::size_t>(m_pakFileOffset);
    auto size = static_cast<std::size_t>(m_pakChunkSize);
    u8 *texels = car.Texels(m_pakDecalIndex);
    if (offset + size > m_pakNote.size() || size > static_cast<std::size_t>(car.End() - texels) ||
        size > m_undoBuffer.size()) {
        m_pakError = PAK_ERROR_TRANSFER;
        return;
    }
    if (operation == PAINT_TOOL_LOAD) {
        std::copy_n(
            m_pakNote.begin() + static_cast<std::ptrdiff_t>(offset), size, m_undoBuffer.begin());
        SDL_Log("Loading decal %d...", m_pakDecalIndex);
        const u8 *mask = car.Mask(m_pakDecalIndex);
        if (mask == nullptr) {
            std::memcpy(texels, m_undoBuffer.data(), size);
        } else {
            CopyUnmaskedTexels(texels, mask, width, height);
        }
        m_previewDirty = true;
    } else {
        std::copy_n(texels, size, m_pakNote.begin() + static_cast<std::ptrdiff_t>(offset));
        SDL_Log("Saving decal %d...", m_pakDecalIndex);
    }
    m_pakFileOffset += m_pakChunkSize;
}

void PaintShop::LoadDecalPalette(DecalCar &car, std::size_t offset) {
    for (int entry = 0; entry < 16; ++entry) {
        auto at = offset + static_cast<std::size_t>(entry) * 2;
        car.SetPaletteColor(
            CAR_MATERIAL_PAINT, entry, static_cast<u16>(m_pakNote[at] << 8 | m_pakNote[at + 1]));
    }
    // The lights take the first two colours of the paint.
    for (int material : {CAR_MATERIAL_HEADLIGHTS, CAR_MATERIAL_REAR_LIGHTS}) {
        car.SetPaletteColor(material, 0, car.PaletteColor(CAR_MATERIAL_PAINT, 0));
        car.SetPaletteColor(material, 1, car.PaletteColor(CAR_MATERIAL_PAINT, 1));
    }
    m_previewDirty = true;
}

bool PaintShop::LoadDecalsQuietly(DecalCar &car) {
    auto note = m_pak.ReadNote(DECAL_NOTE);
    if (!note) {
        return false;
    }
    m_pakNote = std::move(*note);
    m_pakError = 0;
    m_pakFileOffset = 0;
    for (m_pakDecalIndex = 0; m_pakDecalIndex < DECAL_SLOTS && m_pakError == 0; ++m_pakDecalIndex) {
        TransferDecal(car, PAINT_TOOL_LOAD);
    }
    auto offset = static_cast<std::size_t>(m_pakFileOffset);
    bool loaded = m_pakError == 0 && offset + DECAL_PALETTE_BYTES <= m_pakNote.size();
    if (loaded) {
        LoadDecalPalette(car, offset);
    }
    m_pakError = 0;
    m_pakDecalIndex = 0;
    m_pakFileOffset = 0;
    m_pakChunkSize = 0;
    return loaded;
}

void PaintShop::DrawPakButtons(s32 buttonY, u32 okTextX, u32 cancelTextX, s32 okX, s32 cancelX) {
    m_text.size = 10;
    s32 textY = (buttonY + 0x12) >> 1;
    DrawText("%wwOK", static_cast<s32>(okTextX >> 1), textY);
    DrawText("%wwCANCEL", static_cast<s32>(cancelTextX >> 1), textY);
    DrawTexturedSprite(*m_buttonA, okX, buttonY);
    DrawTexturedSprite(*m_buttonB, cancelX, buttonY);
}

s32 PaintShop::ProcessDecalSaveLoad(DecalCar &car, u8 operation, u8 inCarSelect, bool &done) {
    if (!m_pakDialogActive) {
        m_pakDialogActive = 1;
        m_pakError = 0;
        m_pakDecalIndex = 0;
        m_pakFileOffset = 0;
        m_pakChunkSize = 0;
        m_pakWaitFrames = 0;
        m_pakDialogState = inCarSelect == 1 ? PAK_STATE_FADE_OUT : PAK_STATE_CONFIRM;
    }
    done = false;
    s32 y = 0;
    s32 textY = 0;
    switch (m_pakDialogState) {
    case PAK_STATE_FADE_OUT:
        m_screen.FadeSounds(0.0f);
        m_pakDialogState = PAK_STATE_WAIT_FADE;
        break;
    case PAK_STATE_WAIT_FADE:
        // The frames of the wait are counted once the sound effects are silent.
        if (m_screen.SoundsFaded() && m_consoleFrame && m_pakWaitFrames++ == PAK_FADE_WAIT_FRAMES) {
            m_pakDialogState = PAK_STATE_INIT_PAK;
            m_pakWaitFrames = 0;
        }
        break;
    case PAK_STATE_CONFIRM: {
        // The row of the buttons is placed by the width of the A button.
        s32 buttonY = 0x110 - m_buttonA->width;
        DrawDialogBox(0xDC, 0xC8, 0xC8, 0x50);
        m_text.align = TextStyle::Align::Left;
        m_text.shadow = false;
        m_text.size = 14;
        SetPakTextColors(0);
        DrawText(operation == PAINT_TOOL_LOAD ? "LOAD DECALS?" : "SAVE DECALS?", 0x77, 0x74);
        DrawPakButtons(buttonY,
                       static_cast<u32>(m_buttonA->width + 0xF2),
                       static_cast<u32>(m_buttonB->width + 0x136),
                       0xEC,
                       0x130);
        if ((m_input.buttons & INPUT_A) != 0) {
            Transfer(INPUT_A);
            m_pakDialogState = PAK_STATE_FADE_OUT;
        } else if ((m_input.buttons & INPUT_B) != 0) {
            Transfer(INPUT_B);
            // The dialog stays active and opens here again when it is next run.
            done = true;
            m_paintTool = m_previousPaintTool;
            return 0;
        }
        break;
    }
    case PAK_STATE_INIT_PAK:
        // The pak of files is always inserted, readable, and the same pak.
        m_pakDialogState = PAK_STATE_FIND_FILE;
        break;
    case PAK_STATE_CONFIRM_OVERWRITE: {
        s32 buttonY = 0x11D - m_buttonA->width;
        DrawDialogBox(0xD6, 0xBB, 0xD4, 0x6A);
        m_text.align = TextStyle::Align::Left;
        m_text.shadow = false;
        m_text.size = 13;
        SetPakTextColors(0);
        DrawText("OK TO OVERWRITE", 0x72, 0x6C);
        DrawText("SAVED DECALS?", 0x72, 0x7A);
        // The label of the B button is placed by its height.
        DrawPakButtons(buttonY,
                       static_cast<u32>(m_buttonA->width + 0xEE),
                       static_cast<u32>(m_buttonB->height + 0x13A),
                       0xE8,
                       0x134);
        if ((m_input.buttons & INPUT_A) != 0) {
            Transfer(INPUT_A);
            m_pakDialogState = PAK_STATE_TRANSFER_DECALS;
        } else if ((m_input.buttons & INPUT_B) != 0) {
            Transfer(INPUT_B);
            // The error is clear, and the dialog finishes as if the decals were saved.
            m_pakDialogState = PAK_STATE_FINISH;
        }
        break;
    }
    case PAK_STATE_FIND_FILE: {
        m_pakFileOffset = 0;
        m_pakChunkSize = 0;
        m_pakDecalIndex = 0;
        m_pakFileNeedsCreate = 0;
        m_pakWaitFrames = 0;
        auto note = m_pak.ReadNote(DECAL_NOTE);
        if (note) {
            m_pakNote = std::move(*note);
            if (operation == PAINT_TOOL_LOAD) {
                y = inCarSelect == 1 ? 0xDB : 0xC5;
                DrawPakProgressBox(y, "LOADING DECALS...", 0x72, (y + 30) >> 1);
                m_pakDialogState = PAK_STATE_TRANSFER_DECALS;
            } else {
                m_pakNote.resize(DECAL_FILE_SIZE);
                m_pakDialogState = PAK_STATE_CONFIRM_OVERWRITE;
            }
        } else if (operation == PAINT_TOOL_LOAD) {
            m_pakError = PFS_ERR_INVALID;
            m_pakDialogState = PAK_STATE_FINISH;
        } else if (m_pak.FreeBytes() < DECAL_FILE_SIZE) {
            m_pakError = PFS_DATA_FULL;
            m_pakDialogState = PAK_STATE_FINISH;
        } else {
            DrawPakProgressBox(0xC5, "SAVING DECALS...", 0x73, 0x71);
            m_pakFileNeedsCreate = 1;
            m_pakDialogState = PAK_STATE_TRANSFER_DECALS;
            m_pakWaitFrames = 0;
        }
        break;
    }
    case PAK_STATE_TRANSFER_DECALS:
        if (inCarSelect == 1) {
            y = 0xDB;
            textY = 0x7C;
        } else {
            y = 0xC5;
            textY = 0x71;
        }
        if (operation == PAINT_TOOL_LOAD) {
            DrawPakProgressBox(y, "LOADING DECALS...", 0x72, textY);
        } else {
            DrawPakProgressBox(y, "SAVING DECALS...", 0x73, textY);
        }
        if (!m_consoleFrame || m_pakWaitFrames++ < PAK_TRANSFER_DELAY_FRAMES) {
            break;
        }
        if (m_pakFileNeedsCreate) {
            m_pakFileNeedsCreate = 0;
            if (m_pak.Notes().size() >= PAK_NOTES) {
                m_pakError = PFS_DIR_FULL;
                m_pakDialogState = PAK_STATE_FINISH;
                break;
            }
            m_pakNote.assign(DECAL_FILE_SIZE, 0);
        }
        if (m_pakDecalIndex < DECAL_SLOTS) {
            TransferDecal(car, operation);
            if (m_pakError) {
                m_pakDialogState = PAK_STATE_FINISH;
            } else {
                DrawFilledRectHiRes(0xE7,
                                    y + 0x35,
                                    m_pakFileOffset * DECAL_PROGRESS_WIDTH / DECAL_PROGRESS_FULL,
                                    0x12,
                                    0,
                                    0,
                                    0xC0);
                ++m_pakDecalIndex;
            }
        } else {
            m_pakDecalIndex = 0;
            DrawFilledRectHiRes(0xE7, y + 0x35, DECAL_PROGRESS_WIDTH, 0x12, 0, 0, 0xC0);
            m_pakDialogState = PAK_STATE_TRANSFER_PALETTE;
        }
        break;
    case PAK_STATE_TRANSFER_PALETTE: {
        auto offset = static_cast<std::size_t>(m_pakFileOffset);
        if (offset + DECAL_PALETTE_BYTES > m_pakNote.size()) {
            m_pakError = PAK_ERROR_TRANSFER;
        } else if (operation == PAINT_TOOL_LOAD) {
            SDL_Log("Loading color palette...");
            LoadDecalPalette(car, offset);
            if (!inCarSelect) {
                ConvertPaletteToRgb();
                SaveDecalUndo();
            }
        } else {
            SDL_Log("Saving color palette...");
            for (int entry = 0; entry < 16; ++entry) {
                u16 color = car.PaletteColor(CAR_MATERIAL_PAINT, entry);
                m_pakNote[offset + static_cast<std::size_t>(entry) * 2] =
                    static_cast<u8>(color >> 8);
                m_pakNote[offset + static_cast<std::size_t>(entry) * 2 + 1] =
                    static_cast<u8>(color);
            }
            if (!m_pak.WriteNote(DECAL_NOTE, m_pakNote)) {
                m_pakError = PFS_ERR_INCONSISTENT;
            }
        }
        if (m_pakError == 0) {
            SDL_Log(operation == PAINT_TOOL_LOAD ? "Color palette loaded successfully!" :
                                                   "Color palette saved successfully!");
            m_pakDialogState = PAK_STATE_DONE;
            if (!inCarSelect) {
                m_decalEditCount = 0;
            }
            return 0;
        }
        SDL_Log("err_code = %d", m_pakError);
        m_pakDialogState = PAK_STATE_FINISH;
        done = false;
        break;
    }
    case PAK_STATE_DONE:
        m_pakDecalIndex = 0;
        m_pakFileOffset = 0;
        m_pakChunkSize = 0;
        m_pakWaitFrames = 0;
        m_pakDialogState = PAK_STATE_FINISH;
        break;
    case PAK_STATE_FINISH:
        m_pakDialogActive = 0;
        m_screen.FadeSounds(1.0f);
        if (!m_pakError) {
            done = true;
            if (!inCarSelect) {
                m_paintTool = m_previousPaintTool;
            }
            return 0;
        }
        if (!inCarSelect && (m_input.buttons & PAK_DISMISS_BUTTONS) != 0) {
            Transfer(PAK_DISMISS_BUTTONS);
            m_paintTool = m_previousPaintTool;
        }
        done = false;
        return m_pakError;
    default:
        break;
    }
    return 0;
}

std::optional<bool> PaintShop::LoadDecalsFrame(SceneRenderer &renderer,
                                               float seconds,
                                               const PaintShopInput &input,
                                               DecalCar &car) {
    // RunPakDialog() of the car select screen of the console.
    BeginDraw(renderer);
    TranslateInput(seconds, input, true);
    if (m_pakResult == 0) {
        if (!m_pakDone) {
            m_pakResult = ProcessDecalSaveLoad(car, PAINT_TOOL_LOAD, 1, m_pakDone);
        }
    } else if (m_pakResult == PAK_DIALOG_CANCELLED) {
        m_pakDone = true;
    } else {
        DrawPakErrorMessage(m_pakResult, PAINT_TOOL_LOAD, 1);
        if ((m_input.buttons & (INPUT_A | INPUT_B)) != 0) {
            Transfer(INPUT_A | INPUT_B);
            m_pakDone = true;
        }
    }
    FlushFills();
    if (!m_pakDone) {
        return std::nullopt;
    }
    bool loaded = m_pakResult == 0;
    m_pakResult = 0;
    m_pakDone = false;
    return loaded;
}

void PaintShop::PressKeyboardKey(u8 key) {
    if (key == KEY_ENTER) {
        if (m_decalTextLength > 0) {
            m_paintToolStarted = 1;
            m_textShadowPickerOpen = 1;
        } else {
            m_paintToolStarted = 0;
            m_decalDragActive = 0;
        }
        m_showPreview3D = 1;
        m_paletteEnabled = 1;
    } else if (key == KEY_BACKSPACE) {
        u8 length = 0;
        // An empty text reads the word before the keys of the text, and the console finds
        // nothing there.
        if (m_decalTextLength == 0 || m_decalTextGlyphs[m_decalTextLength - 1] < 0) {
            m_decalTextLength = 0;
            m_decalTextWidth = 0;
        } else {
            length = --m_decalTextLength;
            if (length == 0) {
                m_decalTextWidth = static_cast<u8>(
                    m_decalTextWidth -
                    m_decalGlyphs[static_cast<std::size_t>(m_decalTextGlyphs[0])].width);
            } else {
                const PaintGlyph &before =
                    m_decalGlyphs[static_cast<std::size_t>(m_decalTextGlyphs[length - 1])];
                const PaintGlyph &last =
                    m_decalGlyphs[static_cast<std::size_t>(m_decalTextGlyphs[length])];
                s32 overlap = before.kernRight < last.kernLeft ? before.kernRight : last.kernLeft;
                m_decalTextWidth = static_cast<u8>(m_decalTextWidth - last.width + overlap - 2);
            }
        }
        m_decalText.resize(length);
        m_decalTextGlyphs[length] = -1;
    } else {
        const PaintGlyph &pressed = m_decalGlyphs[key];
        s32 width = pressed.width + m_decalTextWidth;
        if (width + 2 < m_decalWidth && m_decalTextLength < DECAL_TEXT_LENGTH) {
            u8 length = m_decalTextLength;
            m_decalText.resize(length);
            m_decalText.push_back(static_cast<char>(pressed.character));
            m_decalTextGlyphs[length] = key;
            if (length == 0) {
                m_decalTextWidth = static_cast<u8>(width);
            } else {
                const PaintGlyph &before =
                    m_decalGlyphs[static_cast<std::size_t>(m_decalTextGlyphs[length - 1])];
                s32 overlap =
                    before.kernRight < pressed.kernLeft ? before.kernRight : pressed.kernLeft;
                m_decalTextWidth = static_cast<u8>(width - overlap + 2);
            }
            m_decalTextLength = static_cast<u8>(length + 1);
        }
    }
}

void PaintShop::TypeOnKeyboard() {
    // A keyboard of the computer types the keys of the font. A letter the font has only in capitals
    // types the capital, and a character the font lacks is dropped.
    auto keyOf = [&](unsigned char character) -> std::optional<u8> {
        for (u8 i = 0; i < m_decalGlyphs.size(); ++i) {
            if (i != KEY_BACKSPACE && i != KEY_ENTER && m_decalGlyphs[i].character == character) {
                return i;
            }
        }
        return std::nullopt;
    };
    // The keys typed after Enter in the same frame are dropped with the keyboard.
    for (char typed : m_typedText) {
        if (!TakesText()) {
            return;
        }
        if (typed == '\r') {
            PressKeyboardKey(KEY_ENTER);
            continue;
        }
        if (typed == '\b') {
            PressKeyboardKey(KEY_BACKSPACE);
            continue;
        }
        auto c = static_cast<unsigned char>(typed);
        if (c < 0x20 || c >= 0x80) {
            continue;
        }
        std::optional<u8> key = keyOf(c);
        if (!key) {
            key = keyOf(static_cast<unsigned char>(std::toupper(c)));
        }
        if (key) {
            PressKeyboardKey(*key);
        }
    }
}

void PaintShop::ProcessTextInputKeyboard() {
    PaintSprite &highlight = *m_keyboardHighlight;
    m_showPreview3D = 0;
    m_paletteEnabled = 0;
    DrawSpriteTinted(*m_keyboardBackground, 0xE0, 0xE0, 0xE0);
    if (highlight.style) {
        highlight.style = 0;
        if (m_screenFrameCount > 0 && m_consoleFrame) {
            --m_screenFrameCount;
        }
        if (m_screenFrameCount == 0) {
            DrawSpriteTinted(highlight, 0x20, 0xC8, 0xFF);
        } else {
            DrawSpriteTinted(highlight, 0xFF, 0xEB, 0);
        }
    }
    DrawSpriteAtOrigin(*m_keyboardPanel);
    s32 x = m_keyboardPanel->originX;
    s32 y = m_keyboardPanel->originY + m_keyboardPanel->height + 0x20;
    DrawSpriteRegion(*m_keyboardBackground, x, y, 0xC8, 0x30, 0xE0, 0xE0, 0xE0);
    DrawRectOutline(x, y, 0xC8, 0x30, 2, 0xFF, 0xFF, 0xFF);
    if (IsCursorInButton(*m_keyboardBackground)) {
        for (std::size_t i = 0; i < m_decalGlyphs.size(); ++i) {
            const PaintGlyph &key = m_decalGlyphs[i];
            if (IsCursorOnKeyboardKey(key)) {
                m_keyboardHoverKey = static_cast<u8>(i);
                highlight.style = 1;
                highlight.originX = key.x;
                highlight.originY = key.y;
            }
            if (!IsCursorInButton(highlight) || (m_input.buttons & (INPUT_Z | INPUT_A)) == 0) {
                continue;
            }
            Transfer(INPUT_Z | INPUT_A);
            m_screenFrameCount = KEY_FLASH_FRAMES;
            PressKeyboardKey(m_keyboardHoverKey);
        }
    }
    TypeOnKeyboard();
    if (m_decalTextLength > 0) {
        const auto &color = m_swatches[m_decalColor].color;
        SetTextCustomColors(color[0], color[1], color[2], color[0], color[1], color[2]);
        m_text.size = 20;
        m_text.align = TextStyle::Align::Center;
        m_text.shadow = false;
        DrawText(m_decalText, (x + 0x64) >> 1, (y + 0x24) >> 1);
    }
}

} // namespace coastline
