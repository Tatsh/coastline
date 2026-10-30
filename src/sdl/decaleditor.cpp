#include <cmath>

#include <SDL3/SDL.h>

#include "common/input.h"
#include "menuscreen.h"
#include "paintshop.h"

namespace coastline {

namespace {

// The buttons that pick and paint.
constexpr u32 INPUT_PICK = INPUT_Z | INPUT_A;

// A second click within this many milliseconds is a double click.
constexpr u32 DOUBLE_CLICK_MS = 350;

// How far the stick must be pushed to move the cursor or a bar of the colour editor.
constexpr f32 CURSOR_DEAD_ZONE = 0.05f;
constexpr f32 COLOR_DEAD_ZONE = 0.1f;

// The pixels the cursor moves for the whole push of the stick, slower over the decal.
constexpr f32 CURSOR_SPEED = 12.0f;
constexpr f32 CURSOR_SPEED_FINE = 6.0f;

// The steps a bar of the colour editor moves for the whole push of the stick.
constexpr f32 COLOR_SPEED = 10.0f;

// Where the cursor may go.
constexpr s32 CURSOR_MIN_X = 0x1F;
constexpr s32 CURSOR_MAX_X = 0x261;
constexpr s32 CURSOR_MIN_Y = 0x16;
constexpr s32 CURSOR_MAX_Y = 0x1CA;

// The second colour of the palette is the first at half brightness, and runs to 127.
constexpr u8 DARK_SWATCH = 1;
constexpr u8 DARK_SWATCH_MAX = 0x7F;

// The key of the letter O. The cursor starts on it when the keyboard opens.
constexpr std::size_t KEYBOARD_START_KEY = 24;

// The cell of the letter A in the font. The text menu shows its styles with it.
constexpr s32 PREVIEW_GLYPH = 10;

// The highlight of the paint shop.
constexpr u8 HIGHLIGHT_RED = 0x20;
constexpr u8 HIGHLIGHT_GREEN = 0xC8;
constexpr u8 HIGHLIGHT_BLUE = 0xFF;

// The bars of the colour editor.
constexpr s32 COLOR_BAR_X = 0x11D;
constexpr s32 COLOR_BAR_WIDTH = 0x80;
constexpr s32 COLOR_BAR_HEIGHT = 0x10;
constexpr s32 COLOR_BAR_Y = 0xC2;
constexpr s32 COLOR_BAR_PITCH = 0x16;

// The styles of the rectangle tool and of the oval tool.
enum RectStyle : u8 {
    RECT_STYLE_FILLED,
    RECT_STYLE_OUTLINED,
    RECT_STYLE_FILLED_ROUND,
    RECT_STYLE_OUTLINED_ROUND,
};
enum OvalStyle : u8 {
    OVAL_STYLE_FILLED_ELLIPSE,
    OVAL_STYLE_OUTLINED_ELLIPSE,
    OVAL_STYLE_FILLED_CIRCLE,
    OVAL_STYLE_OUTLINED_CIRCLE,
};

// Whether a decal has a mirror image.
bool HasMirror(u8 slot) {
    return slot != 4 && slot != 5 && slot != 8 && slot != 9;
}

// The tools whose styles a double click opens.
bool HasStyleMenu(u8 tool) {
    return tool == PAINT_TOOL_RECTANGLE || tool == PAINT_TOOL_OVAL || tool == PAINT_TOOL_BRUSH ||
           tool == PAINT_TOOL_TEXT;
}

// A colour of the palette in RGBA 5551 from 8-bit channels, with the alpha of the old entry.
u16 PackPaletteColor(u16 old, int red, int green, int blue) {
    return static_cast<u16>((old & 1) | (red & 0xF8) << 8 | (green & 0xF8) << 3 |
                            (blue & 0xF8) >> 2);
}

} // namespace

void PaintShop::SetPalette(int entry, u8 red, u8 green, u8 blue) {
    u16 old = m_car->PaletteColor(m_decalPaletteMaterial, entry);
    m_car->SetPaletteColor(m_decalPaletteMaterial, entry, PackPaletteColor(old, red, green, blue));
    m_previewDirty = true;
}

void PaintShop::CopyLightPalettes() {
    // The first two colours of the palette are copied into the palettes of the lights. The second
    // colour of the rear lights is never set, and the first is set again.
    u16 first = m_car->PaletteColor(m_decalPaletteMaterial, 0);
    u16 second = m_car->PaletteColor(m_decalPaletteMaterial, 1);
    m_car->SetPaletteColor(CAR_MATERIAL_HEADLIGHTS, 0, first);
    m_car->SetPaletteColor(CAR_MATERIAL_HEADLIGHTS, 1, second);
    m_car->SetPaletteColor(CAR_MATERIAL_REAR_LIGHTS, 0, first);
}

void PaintShop::SelectDecalSlot(s32 slot) {
    // The decal edited, centred in its frame.
    m_decalSlotChanged = 1;
    m_screenFrameCount = 0;
    DecalTexture texture = m_car->Texture(slot);
    m_decalTexels = m_car->Texels(slot);
    m_decalTexelsEnd = m_car->End();
    // The mask is indexed by the slot as stored.
    m_decalMask = m_car->Mask(m_decalSlot);
    m_decalWidth = static_cast<u8>(texture.width);
    m_decalHeight = static_cast<u8>(texture.height);
    SDL_Log("decal_width = %d", m_decalWidth);
    SDL_Log("decal_height = %d", m_decalHeight);
    m_decalCanvas.x = m_decalCanvasFrame.x + ((m_decalCanvasFrame.width - m_decalWidth * 4) >> 1);
    m_decalCanvas.y = m_decalCanvasFrame.y + ((m_decalCanvasFrame.height - m_decalHeight * 4) >> 1);
    m_decalCanvas.width = m_decalWidth * 4;
    m_decalCanvas.height = m_decalHeight * 4;
}

s32 PaintShop::AnchorDistance() const {
    s32 dx = m_paintCursor->originX - m_paintAnchorX;
    s32 dy = m_paintCursor->originY - m_paintAnchorY;
    return static_cast<s32>(std::sqrt(static_cast<f32>(dx * dx + dy * dy)));
}

void PaintShop::ProcessColorEditorDialog() {
    s32 cancelX = 0x151 - m_buttonB->width;
    s32 buttonY = 0x122 - m_buttonA->height;
    s32 step = 0;
    DrawRectBorder(0xAD, 0xB6, 0x126, 0x74, 3, 0, 0, 0x80, 0x80, 0x80);
    PaintSwatch &swatch = m_swatches[m_decalColor];
    DrawRectBorder(
        0xBB, 0xC2, 0x3C, 0x3C, 1, 0, 1, swatch.color[0], swatch.color[1], swatch.color[2]);

    // The dark colour fills a bar at 127, and the others are shown at half.
    for (s32 i = 0, y = COLOR_BAR_Y; i < 3; ++i, y += COLOR_BAR_PITCH) {
        const auto &bar = m_colorEditorBars[static_cast<std::size_t>(i)];
        if (i == m_colorEditorChannel) {
            DrawRectBorder(
                COLOR_BAR_X, y, COLOR_BAR_WIDTH, COLOR_BAR_HEIGHT, 1, 0, 1, 0xB4, 0xB4, 0xB4);
        } else {
            DrawRectBorder(
                COLOR_BAR_X, y, COLOR_BAR_WIDTH, COLOR_BAR_HEIGHT, 1, 1, 1, 0x80, 0x80, 0x80);
        }
        s32 level = swatch.color[static_cast<std::size_t>(i)];
        if (level <= 0) {
            continue;
        }
        if (m_decalColor != DARK_SWATCH) {
            level >>= 1;
        }
        DrawFilledRectHiRes(COLOR_BAR_X, y, level, COLOR_BAR_HEIGHT, bar[0], bar[1], bar[2]);
        if (i == m_colorEditorChannel) {
            DrawRectBorder(
                COLOR_BAR_X, y, level, COLOR_BAR_HEIGHT, 1, 0, 0, bar[0], bar[1], bar[2]);
        }
    }

    for (s32 i = 0, y = 0x68; i < 3; ++i, y += 11) {
        if (i == m_colorEditorChannel) {
            SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0);
        } else {
            SetTextCustomColors(0xC8, 0xC8, 0xC8, 0x80, 0x80, 0x80);
        }
        m_text.size = 11;
        m_text.align = TextStyle::Align::Left;
        DrawText(m_colorChannelNames[static_cast<std::size_t>(i)], 0x83, y);
        m_text.size = 10;
        m_text.align = TextStyle::Align::Right;
        DrawText(std::to_string(swatch.color[static_cast<std::size_t>(i)]), 0xE2, y);
    }

    DrawTexturedSprite(*m_buttonA, 0xE5, buttonY);
    DrawTexturedSprite(*m_buttonB, cancelX, buttonY);
    m_text.align = TextStyle::Align::Left;
    m_text.size = 10;
    DrawText("%wwOK", (m_buttonA->width + 0xEB) >> 1, (buttonY + 0x12) >> 1);
    DrawText("%wwCANCEL", (cancelX + m_buttonB->width + 6) >> 1, (buttonY + 0x12) >> 1);

    CheckStick(m_input.repeatUp, m_input.stickY, 1.0f, INPUT_UP);
    CheckStick(m_input.repeatDown, m_input.stickY, -1.0f, INPUT_DOWN);
    if (std::fabs(m_input.stickX) >= COLOR_DEAD_ZONE) {
        step = static_cast<s32>(m_input.stickX * COLOR_SPEED);
    } else if ((m_input.buttons & INPUT_RIGHT) != 0) {
        step = 4;
    } else if ((m_input.buttons & INPUT_LEFT) != 0) {
        step = -4;
    } else if ((m_input.buttons & INPUT_C_RIGHT) != 0) {
        step = 1;
    } else if ((m_input.buttons & INPUT_C_LEFT) != 0) {
        step = -1;
    }
    u32 buttons = m_input.buttons;
    if ((buttons & (INPUT_DOWN | INPUT_C_DOWN)) != 0) {
        Transfer(INPUT_DOWN | INPUT_C_DOWN);
        m_colorEditorChannel = m_colorEditorChannel == 2 ? 0 : m_colorEditorChannel + 1;
    } else if ((buttons & (INPUT_UP | INPUT_C_UP)) != 0) {
        Transfer(INPUT_UP | INPUT_C_UP);
        m_colorEditorChannel = m_colorEditorChannel == 0 ? 2 : m_colorEditorChannel - 1;
    }

    u8 maximum = m_decalColor == DARK_SWATCH ? DARK_SWATCH_MAX : 0xFF;
    // A press of the mouse on a bar chooses its channel and sets its level where the mouse is, and
    // the press is not the A button that closes the editor.
    for (s32 i = 0; i < 3; ++i) {
        s32 top = COLOR_BAR_Y + i * COLOR_BAR_PITCH;
        bool held = ((m_input.buttons | m_input.heldButtons) & INPUT_A) != 0;
        if (!held || m_pointer[0] < COLOR_BAR_X - 2 ||
            m_pointer[0] > COLOR_BAR_X + COLOR_BAR_WIDTH + 2 || m_pointer[1] < top ||
            m_pointer[1] > top + COLOR_BAR_HEIGHT) {
            continue;
        }
        Transfer(INPUT_A);
        m_colorEditorChannel = static_cast<u8>(i);
        s32 level = (m_pointer[0] - COLOR_BAR_X) * (m_decalColor == DARK_SWATCH ? 1 : 2);
        step = level - swatch.color[static_cast<std::size_t>(i)];
        if (step == 0) {
            step = level <= 0 ? -1 : 1;
        }
    }

    // Each branch indexes the bar afresh.
    s32 level = swatch.color[m_colorEditorChannel] + step;
    swatch.color[m_colorEditorChannel] = static_cast<u8>(std::clamp<s32>(level, 0, maximum));

    if (step != 0) {
        SetPalette(m_decalColor, swatch.color[0], swatch.color[1], swatch.color[2]);
        // The first two colours follow each other, the second at half the first.
        if (m_decalColor == 0) {
            const auto &first = m_swatches[0].color;
            SetPalette(1, first[0] >> 1, first[1] >> 1, first[2] >> 1);
            for (std::size_t i = 0; i < 3; ++i) {
                m_swatches[1].color[i] =
                    first[i] == 0xFF ? DARK_SWATCH_MAX : static_cast<u8>(first[i] >> 1);
            }
        } else if (m_decalColor == DARK_SWATCH) {
            const auto &dark = m_swatches[1].color;
            SetPalette(0,
                       static_cast<u8>(dark[0] << 1),
                       static_cast<u8>(dark[1] << 1),
                       static_cast<u8>(dark[2] << 1));
            for (std::size_t i = 0; i < 3; ++i) {
                m_swatches[0].color[i] =
                    dark[i] == DARK_SWATCH_MAX ? 0xFF : static_cast<u8>(dark[i] << 1);
            }
        }
        if (m_decalColor == 0 || m_decalColor == DARK_SWATCH) {
            CopyLightPalettes();
        }
    }

    buttons = m_input.buttons;
    if ((buttons & INPUT_A) != 0) {
        Transfer(INPUT_A);
        m_colorEditorChannel = 0;
        m_colorEditorOpen = 0;
        ++m_decalEditCount;
    } else if ((buttons & INPUT_B) != 0) {
        Transfer(INPUT_B);
        const auto &saved = m_savedSwatchColor;
        SetPalette(m_decalColor, saved[0], saved[1], saved[2]);
        for (std::size_t i = 0; i < 3; ++i) {
            m_swatches[m_decalColor].color[i] = saved[i];
        }
        // The test is on the restored colour, the value from the saved one.
        if (m_decalColor == 0) {
            SetPalette(1, saved[0] >> 1, saved[1] >> 1, saved[2] >> 1);
            for (std::size_t i = 0; i < 3; ++i) {
                m_swatches[1].color[i] = m_swatches[0].color[i] == 0xFF ?
                                             DARK_SWATCH_MAX :
                                             static_cast<u8>(saved[i] >> 1);
            }
        } else if (m_decalColor == DARK_SWATCH) {
            SetPalette(0,
                       static_cast<u8>(saved[0] << 1),
                       static_cast<u8>(saved[1] << 1),
                       static_cast<u8>(saved[2] << 1));
            for (std::size_t i = 0; i < 3; ++i) {
                m_swatches[0].color[i] = m_swatches[1].color[i] == DARK_SWATCH_MAX ?
                                             0xFF :
                                             static_cast<u8>(saved[i] << 1);
            }
        }
        if (m_decalColor == 0 || m_decalColor == DARK_SWATCH) {
            CopyLightPalettes();
        }
        m_colorEditorChannel = 0;
        m_colorEditorOpen = 0;
    }
}

void PaintShop::UpdateCursorFromInput() {
    if (m_paintShopExiting) {
        return;
    }
    PaintSprite &cursor = *m_paintCursor;
    bool fine = IsCursorInRect(m_decalCanvas) && !m_decalDragActive;
    if (std::fabs(m_input.stickX) >= CURSOR_DEAD_ZONE) {
        cursor.originX +=
            static_cast<s32>(m_input.stickX * (fine ? CURSOR_SPEED_FINE : CURSOR_SPEED));
    } else if ((m_input.buttons & (INPUT_RIGHT | INPUT_C_RIGHT)) != 0) {
        ++cursor.originX;
    } else if ((m_input.buttons & (INPUT_LEFT | INPUT_C_LEFT)) != 0) {
        --cursor.originX;
    }
    // Up on the stick is up the screen, where y goes down.
    fine = IsCursorInRect(m_decalCanvas) && !m_decalDragActive;
    if (std::fabs(m_input.stickY) >= CURSOR_DEAD_ZONE) {
        cursor.originY -=
            static_cast<s32>(m_input.stickY * (fine ? CURSOR_SPEED_FINE : CURSOR_SPEED));
    } else if ((m_input.buttons & (INPUT_DOWN | INPUT_C_DOWN)) != 0) {
        ++cursor.originY;
    } else if ((m_input.buttons & (INPUT_UP | INPUT_C_UP)) != 0) {
        --cursor.originY;
    }
    cursor.originX = std::clamp(cursor.originX, CURSOR_MIN_X, CURSOR_MAX_X);
    cursor.originY = std::clamp(cursor.originY, CURSOR_MIN_Y, CURSOR_MAX_Y);
}

bool PaintShop::PickDecalSlot() {
    // A click on a decal makes it the one edited.
    if (m_decalDragActive || m_decalSlotChanged || (m_input.buttons & INPUT_PICK) == 0) {
        return false;
    }
    for (u8 i = 0; i < DECAL_SLOTS; ++i) {
        if (!IsCursorInRect(m_decalSlotRects[i])) {
            continue;
        }
        Transfer(INPUT_PICK);
        m_previousDecalSlot = m_decalSlot;
        m_decalSlot = i;
        if (m_previousDecalSlot != m_decalSlot) {
            SelectDecalSlot(i);
        }
        m_decalSlotHasMirror = HasMirror(m_decalSlot);
        return true;
    }
    return false;
}

bool PaintShop::PickPaletteColor() {
    // A click on the palette picks a colour, and a second on the same colour opens the editor.
    if (!IsCursorInRect(m_paletteFrameLeft) && !IsCursorInRect(m_paletteFrameRight)) {
        return false;
    }
    if (!m_paletteEnabled || (m_input.buttons & INPUT_PICK) == 0) {
        return false;
    }
    for (u8 i = 0; i < m_swatches.size(); ++i) {
        if (!IsCursorInRect(m_swatches[i].rect)) {
            continue;
        }
        Transfer(INPUT_PICK);
        m_savedColor = m_decalColor;
        m_decalColor = i;
        u32 now = static_cast<u32>(SDL_GetTicks());
        if (i != m_savedColor) {
            m_lastClickTime = now;
        } else if (now - m_lastClickTime < DOUBLE_CLICK_MS) {
            m_savedSwatchColor = m_swatches[m_decalColor].color;
            m_colorEditorOpen = 1;
        } else {
            m_lastClickTime = now;
        }
        return true;
    }
    return false;
}

bool PaintShop::PickPaintTool() {
    // A click on a tool picks it, and a second on a tool with styles opens their menu.
    if (m_decalDragActive) {
        return false;
    }
    for (u8 i = 0; i < PAINT_TOOLS; ++i) {
        if (!IsCursorInRect(m_paintToolButtonRects[i]) || (m_input.buttons & INPUT_PICK) == 0) {
            continue;
        }
        Transfer(INPUT_PICK);
        m_paintToolStarted = 0;
        if (i != PAINT_TOOL_BRUSH_SIZE) {
            m_previousPaintTool = m_paintTool;
            m_paintTool = i;
            u32 now = static_cast<u32>(SDL_GetTicks());
            if (m_previousPaintTool != m_paintTool) {
                if (HasStyleMenu(m_paintTool)) {
                    m_lastClickTime = now;
                }
            } else if (HasStyleMenu(m_paintTool)) {
                if (now - m_lastClickTime < DOUBLE_CLICK_MS) {
                    m_paintToolStarted = 1;
                    m_styleMenuOpen = 1;
                    return true;
                }
                m_lastClickTime = now;
            }
        }
        switch (i) {
        case PAINT_TOOL_BRUSH_SIZE:
            for (u8 size = 0; size < m_brushSizeButtonRects.size(); ++size) {
                if (IsCursorInRect(m_brushSizeButtonRects[size])) {
                    m_brushSize = size;
                    break;
                }
            }
            break;
        case PAINT_TOOL_MIRROR:
            m_paintToolStarted = 1;
            break;
        case PAINT_TOOL_CLEAR:
            m_paintToolIcons[PAINT_TOOL_CLEAR]->style = 0;
            break;
        case PAINT_TOOL_UNDO:
            UndoDecalEdit();
            m_paintTool = m_previousPaintTool;
            break;
        default:
            break;
        }
        return true;
    }
    return false;
}

void PaintShop::ProcessTextTool() {
    // A click on the decal opens the keyboard, the text typed then follows the cursor, and a
    // second click paints it.
    bool clicked = false;
    if (IsCursorInRect(m_decalCanvas) && !m_decalDragActive &&
        (m_input.buttons & INPUT_PICK) != 0) {
        Transfer(INPUT_PICK);
        m_paintAnchorX = m_paintCursor->originX;
        m_paintAnchorY = m_paintCursor->originY;
        clicked = true;
        m_decalDragActive = 1;
    }
    if (m_decalDragActive && !m_paintToolStarted) {
        if (clicked) {
            const PaintGlyph &key = m_decalGlyphs[KEYBOARD_START_KEY];
            m_paintCursor->originX = key.x + (m_keyboardHighlight->drawWidth >> 1);
            m_paintCursor->originY = key.y + (m_keyboardHighlight->drawHeight >> 1);
            m_screenFrameCount = 0;
        }
        ProcessTextInputKeyboard();
        return;
    }
    if (m_paintToolStarted && !IsCursorInRect(m_paletteFrameLeft) &&
        !IsCursorInRect(m_paletteFrameRight) && (m_input.buttons & INPUT_PICK) == 0) {
        if (m_decalTextLength > 0) {
            if (m_textShadowPickerOpen) {
                m_paintCursor->originX = m_paintAnchorX;
                m_textShadowPickerOpen = 0;
                m_paintCursor->originY = m_paintAnchorY;
            }
            DrawDecalTextPreview(m_paintCursor->originX, m_paintCursor->originY + 1);
        } else {
            m_paintToolStarted = 0;
            m_decalDragActive = 0;
        }
        return;
    }
    if (!m_paintToolStarted || (m_input.buttons & INPUT_PICK) == 0) {
        return;
    }
    Transfer(INPUT_PICK);
    SaveDecalUndo();
    StampTextOnDecal(m_paintCursor->originX, m_paintCursor->originY + 1);
    m_decalTextLength = 0;
    m_decalTextWidth = 0;
    m_paintToolStarted = 0;
    m_decalDragActive = 0;
}

void PaintShop::PaintBrush() {
    // The brush paints while the button is pressed, on a copy for undo taken as the stroke starts.
    if (!m_paintToolStarted) {
        SaveDecalUndo();
        m_paintToolStarted = 1;
    }
    const PaintSprite &cursor = *m_paintCursor;
    if (m_brushSize == 0) {
        SetPixel4Bit((cursor.originX - m_decalCanvas.x) >> 2,
                     (m_decalCanvas.y + m_decalCanvas.height - cursor.originY) >> 2,
                     m_decalColor);
    } else if (m_paintToolIcons[PAINT_TOOL_BRUSH]->style == 0) {
        FillCircle4Bit(
            cursor.originX, cursor.originY, m_brushSizeRects[m_brushSize].width >> 1, true);
    } else {
        s32 half = m_brushSizeRects[m_brushSize].width * 2;
        FillRect4Bit(cursor.originX - half,
                     cursor.originY - half,
                     cursor.originX + half,
                     cursor.originY + half);
    }
}

bool PaintShop::StartShape() {
    // A shape tool takes its first corner at the first click and draws at the second.
    if (!m_paintToolStarted) {
        m_paintAnchorX = m_paintCursor->originX;
        m_paintAnchorY = m_paintCursor->originY;
        m_paintToolStarted = 1;
        return false;
    }
    SaveDecalUndo();
    return true;
}

void PaintShop::PaintRectangle() {
    s32 x = m_paintCursor->originX;
    s32 y = m_paintCursor->originY;
    switch (m_paintToolIcons[PAINT_TOOL_RECTANGLE]->style) {
    case RECT_STYLE_FILLED:
        FillRect4Bit(m_paintAnchorX, m_paintAnchorY, x, y);
        break;
    case RECT_STYLE_OUTLINED:
        StrokeDecalRectangle(m_paintAnchorX, m_paintAnchorY, x, y);
        break;
    case RECT_STYLE_FILLED_ROUND:
        FillDecalRoundRect(m_paintAnchorX, m_paintAnchorY, x, y);
        break;
    case RECT_STYLE_OUTLINED_ROUND:
        DrawDecalRoundedRectOutline(m_paintAnchorX, m_paintAnchorY, x, y);
        break;
    default:
        break;
    }
}

void PaintShop::PaintOval() {
    u8 style = m_paintToolIcons[PAINT_TOOL_OVAL]->style;
    // The radius of a circle is taken before the style is tested again.
    s32 radius = 0;
    if (style == OVAL_STYLE_FILLED_CIRCLE || style == OVAL_STYLE_OUTLINED_CIRCLE) {
        radius = AnchorDistance() >> 2;
    }
    s32 x = m_paintCursor->originX;
    s32 y = m_paintCursor->originY;
    switch (style) {
    case OVAL_STYLE_FILLED_ELLIPSE:
        DrawDecalFilledEllipse(m_paintAnchorX, m_paintAnchorY, x, y);
        break;
    case OVAL_STYLE_OUTLINED_ELLIPSE:
        DrawDecalEllipseOutline(m_paintAnchorX, m_paintAnchorY, x, y);
        break;
    case OVAL_STYLE_FILLED_CIRCLE:
        FillCircle4Bit(m_paintAnchorX, m_paintAnchorY, radius, true);
        break;
    case OVAL_STYLE_OUTLINED_CIRCLE:
        DrawDecalCircleOutline(m_paintAnchorX, m_paintAnchorY, radius);
        break;
    default:
        break;
    }
}

bool PaintShop::PaintWithTool() {
    // The text tool never arrives here, but the console tests for it again.
    if (m_paintTool == PAINT_TOOL_TEXT || !IsCursorInRect(m_decalCanvas) ||
        (m_input.buttons & INPUT_PICK) == 0) {
        return false;
    }
    switch (m_paintTool) {
    case PAINT_TOOL_BRUSH:
        PaintBrush();
        return true;
    case PAINT_TOOL_FILL:
        SaveDecalUndo();
        FloodFill4Bit(m_paintCursor->originX, m_paintCursor->originY);
        break;
    case PAINT_TOOL_LINE:
        if (StartShape()) {
            DrawLine4Bit(
                m_paintAnchorX, m_paintAnchorY, m_paintCursor->originX, m_paintCursor->originY);
            m_paintToolStarted = 0;
        }
        break;
    case PAINT_TOOL_RECTANGLE:
        if (StartShape()) {
            PaintRectangle();
            m_paintToolStarted = 0;
        }
        break;
    case PAINT_TOOL_OVAL:
        if (StartShape()) {
            PaintOval();
            m_paintToolStarted = 0;
        }
        break;
    default:
        return true;
    }
    Transfer(INPUT_PICK);
    return true;
}

void PaintShop::DrawShapeOutline() {
    // The dashed outline of a shape from its first corner to the cursor.
    if (m_paintTool == PAINT_TOOL_TEXT || !m_paintToolStarted || !IsCursorInRect(m_decalCanvas) ||
        (m_input.buttons & INPUT_PICK) != 0) {
        return;
    }
    s32 x = m_paintCursor->originX;
    s32 y = m_paintCursor->originY;
    switch (m_paintTool) {
    case PAINT_TOOL_BRUSH:
        // The stroke of the brush ends when the button is let go.
        m_paintToolStarted = 0;
        break;
    case PAINT_TOOL_LINE:
        DrawDashedLine(m_paintAnchorX, m_paintAnchorY, x, y);
        break;
    case PAINT_TOOL_RECTANGLE:
        if (m_paintToolIcons[PAINT_TOOL_RECTANGLE]->style == RECT_STYLE_FILLED ||
            m_paintToolIcons[PAINT_TOOL_RECTANGLE]->style == RECT_STYLE_OUTLINED) {
            DrawDashedRectangle(m_paintAnchorX, m_paintAnchorY, x, y);
        } else {
            DrawDashedRoundedRect(m_paintAnchorX, m_paintAnchorY, x, y);
        }
        break;
    case PAINT_TOOL_OVAL:
        if (m_paintToolIcons[PAINT_TOOL_OVAL]->style == OVAL_STYLE_FILLED_ELLIPSE ||
            m_paintToolIcons[PAINT_TOOL_OVAL]->style == OVAL_STYLE_OUTLINED_ELLIPSE) {
            DrawDashedEllipse(m_paintAnchorX, m_paintAnchorY, x, y);
        } else {
            // In pixels of the screen, where the circle painted is in texels.
            DrawDashedCircle(m_paintAnchorX, m_paintAnchorY, AnchorDistance());
        }
        break;
    default:
        break;
    }
}

void PaintShop::ProcessDecalEditorInput() {
    if (!m_anchorXInitialized) {
        m_anchorXInitialized = true;
        m_paintAnchorX = m_paintCursor->originX;
    }
    if (!m_anchorYInitialized) {
        m_anchorYInitialized = true;
        m_paintAnchorY = m_paintCursor->originY;
    }
    if (m_paintShopExiting) {
        return;
    }
    if (PickDecalSlot() || PickPaletteColor() || PickPaintTool()) {
        return;
    }
    if (m_paintTool == PAINT_TOOL_TEXT) {
        ProcessTextTool();
        return;
    }
    if (PaintWithTool()) {
        return;
    }
    DrawShapeOutline();
}

bool PaintShop::IsCursorInButton(const PaintSprite &sprite) const {
    const PaintSprite &cursor = *m_paintCursor;
    return cursor.originX >= sprite.originX &&
           std::abs(sprite.drawWidth) + sprite.originX >= cursor.originX &&
           cursor.originY >= sprite.originY && sprite.originY + sprite.drawHeight >= cursor.originY;
}

bool PaintShop::IsCursorInRect(const PaintRect &rect) const {
    const PaintSprite &cursor = *m_paintCursor;
    return cursor.originX >= rect.x && rect.x + rect.width >= cursor.originX &&
           cursor.originY >= rect.y && rect.y + rect.height >= cursor.originY;
}

bool PaintShop::IsCursorOnKeyboardKey(const PaintGlyph &key) const {
    const PaintSprite &cursor = *m_paintCursor;
    return cursor.originX >= key.x && key.x + m_keyboardHighlight->drawWidth >= cursor.originX &&
           cursor.originY >= key.y && key.y + m_keyboardHighlight->drawHeight >= cursor.originY;
}

void PaintShop::ConvertPaletteToRgb() {
    // Each 5-bit channel fills the 8 bits by repeating its highest bits.
    for (std::size_t i = 0; i < m_swatches.size(); ++i) {
        u16 color = m_car->PaletteColor(m_decalPaletteMaterial, static_cast<int>(i));
        m_swatches[i].color = {static_cast<u8>(((color >> 8) & 0xF8) | ((color >> 13) & 7)),
                               static_cast<u8>(((color >> 3) & 0xF8) | ((color >> 8) & 7)),
                               static_cast<u8>(((color << 2) & 0xF8) | ((color >> 3) & 7))};
    }
}

void PaintShop::DrawDecalColorPalette() {
    for (const PaintRect *frame : {&m_paletteFrameLeft, &m_paletteFrameRight}) {
        DrawFilledRectHiRes(frame->x, frame->y, frame->width, frame->height, 0, 0, 0);
    }
    for (const PaintSwatch &swatch : m_swatches) {
        DrawFilledRectHiRes(swatch.rect.x,
                            swatch.rect.y,
                            swatch.rect.width,
                            swatch.rect.height,
                            swatch.color[0],
                            swatch.color[1],
                            swatch.color[2]);
    }
    const PaintSwatch &chosen = m_swatches[m_decalColor];
    DrawRectOutline(chosen.rect.x - 2,
                    chosen.rect.y - 2,
                    chosen.rect.width + 4,
                    chosen.rect.height + 4,
                    2,
                    HIGHLIGHT_RED,
                    HIGHLIGHT_GREEN,
                    HIGHLIGHT_BLUE);
}

void PaintShop::MoveStyleMenuCursor() {
    // The cursor of a menu of four styles moves with left and right, around the ends.
    if ((m_input.buttons & INPUT_LEFT) != 0) {
        Transfer(INPUT_LEFT);
        m_styleMenuCursor = m_styleMenuCursor == 0 ? 3 : m_styleMenuCursor - 1;
    } else if ((m_input.buttons & INPUT_RIGHT) != 0) {
        Transfer(INPUT_RIGHT);
        m_styleMenuCursor = m_styleMenuCursor == 3 ? 0 : m_styleMenuCursor + 1;
    }
}

void PaintShop::CloseStyleMenu(PaintSprite &tool) {
    // A sets the style of a tool to the cursor of its menu, and A or B closes the menu.
    if ((m_input.buttons & INPUT_A) != 0) {
        Transfer(INPUT_A);
        tool.style = m_styleMenuCursor;
        m_styleMenuOpen = 0;
    } else if ((m_input.buttons & INPUT_B) != 0) {
        Transfer(INPUT_B);
        m_styleMenuOpen = 0;
    }
}

void PaintShop::ProcessBrushStyleMenu() {
    // The button A is placed by its width.
    s32 buttonY = 0x142 - m_buttonA->width;
    s32 cancelX = 0x15D - m_buttonB->width;
    DrawRectBorder(0xC8, 0x96, 0xF0, 0xB4, 3, 0, 0, 0x80, 0x80, 0x80);
    if (m_paintToolStarted) {
        m_styleMenuCursor = m_paintToolIcons[PAINT_TOOL_BRUSH]->style;
        m_paintToolStarted = 0;
    }
    PaintSprite &circle = *m_shapeFilledCircle;
    std::array<PaintRect, 2> choices{{{0xFA, 0xC8, circle.drawWidth, circle.drawHeight},
                                      {circle.drawWidth + 0x114, 0xCA, 0x26, 0x26}}};
    PickMenuOption(choices.data(), 2, m_styleMenuCursor);
    m_text.size = 15;
    m_text.align = TextStyle::Align::Center;
    m_text.shadow = false;
    DrawText("%rySELECT STYLE", 0x9F, 0x5B);
    bool round = m_styleMenuCursor == 0;
    DrawSpriteRegion(circle,
                     0xFA,
                     0xC8,
                     circle.drawWidth,
                     circle.drawHeight,
                     round ? HIGHLIGHT_RED : 0,
                     round ? HIGHLIGHT_GREEN : 0,
                     round ? HIGHLIGHT_BLUE : 0);
    DrawFilledRectHiRes(circle.drawWidth + 0x114,
                        0xCA,
                        0x26,
                        0x26,
                        round ? 0 : HIGHLIGHT_RED,
                        round ? 0 : HIGHLIGHT_GREEN,
                        round ? 0 : HIGHLIGHT_BLUE);
    bool names = !m_brushStyleNames[0].empty();
    if (names) {
        DrawRectBorder(0xE0, 0xFF, 0xC0, 0x1E, 1, 1, 1, 0x80, 0x80, 0x80);
    }
    m_text.size = 11;
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    if (names) {
        DrawText(m_brushStyleNames[m_styleMenuCursor & 1], 0x9F, 0x8A);
    }
    DrawTexturedSprite(*m_buttonA, 0xD8, buttonY);
    DrawTexturedSprite(*m_buttonB, cancelX, buttonY);
    m_text.size = 10;
    m_text.align = TextStyle::Align::Left;
    s32 textY = (buttonY + 0x12) >> 1;
    DrawText("%wwSELECT", (m_buttonA->width + 0xDE) >> 1, textY);
    DrawText("%wwCANCEL", (cancelX + m_buttonB->width + 6) >> 1, textY);

    UpdateStickRepeat();
    if ((m_input.buttons & (INPUT_RIGHT | INPUT_LEFT)) != 0) {
        Transfer(INPUT_RIGHT | INPUT_LEFT);
        m_styleMenuCursor ^= 1;
    }
    CloseStyleMenu(*m_paintToolIcons[PAINT_TOOL_BRUSH]);
}

void PaintShop::ProcessTextStyleMenu() {
    PaintSprite &textTool = *m_paintToolIcons[PAINT_TOOL_TEXT];
    DrawRectBorder(0xB4, 0x82, 0x118, 0xD0, 3, 0, 0, 0x80, 0x80, 0x80);
    m_text.size = 16;
    m_text.align = TextStyle::Align::Center;
    m_text.shadow = false;
    DrawText("%rySELECT OPTION", 0x9F, 0x53);
    if (m_paintToolStarted) {
        m_styleMenuCursor = textTool.style;
        m_savedColor = m_textShadowColor;
        m_paintToolStarted = 0;
        m_textShadowPickerOpen = 0;
    }
    std::array<PaintRect, 2> choices{{{0xF0, 0xB8, 0x40, 0x40}, {0x150, 0xB8, 0x40, 0x40}}};
    if (!m_textShadowPickerOpen) {
        PickMenuOption(choices.data(), 2, m_styleMenuCursor);
    }
    for (std::size_t i = 0; i < choices.size(); ++i) {
        // The width is passed as the height too.
        const PaintRect &choice = choices[i];
        if (i == m_styleMenuCursor) {
            DrawRectOutline(choice.x,
                            choice.y,
                            choice.width,
                            choice.width,
                            1,
                            HIGHLIGHT_RED,
                            HIGHLIGHT_GREEN,
                            HIGHLIGHT_BLUE);
        } else {
            DrawRectOutline(choice.x, choice.y, choice.width, choice.width, 1, 0x20, 0x20, 0x20);
        }
    }
    const auto &color = m_swatches[m_decalColor].color;
    const auto &shadow = m_swatches[m_textShadowColor].color;
    DrawScaledTexturedRect(*m_decalFont,
                           PREVIEW_GLYPH * 16,
                           0,
                           16,
                           16,
                           choices[0].x + 7,
                           choices[0].y + 10,
                           0x30,
                           0x30,
                           color[0],
                           color[1],
                           color[2]);
    DrawScaledTexturedRect(*m_decalFont,
                           PREVIEW_GLYPH * 16,
                           0,
                           16,
                           16,
                           choices[1].x + 9,
                           choices[1].y + 12,
                           0x30,
                           0x30,
                           shadow[0],
                           shadow[1],
                           shadow[2]);
    DrawScaledTexturedRect(*m_decalFont,
                           PREVIEW_GLYPH * 16,
                           0,
                           16,
                           16,
                           choices[1].x + 5,
                           choices[1].y + 8,
                           0x30,
                           0x30,
                           color[0],
                           color[1],
                           color[2]);

    bool names = !m_textStyleNames[0].empty();
    if (!m_textShadowPickerOpen) {
        if (names) {
            DrawRectBorder(0xC6,
                           choices[0].height + choices[0].y + 0x11,
                           0xF4,
                           0x1E,
                           1,
                           1,
                           1,
                           0x80,
                           0x80,
                           0x80);
        }
        m_text.size = 11;
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
        if (names) {
            DrawText(m_textStyleNames[m_styleMenuCursor & 1],
                     0x9E,
                     (choices[1].height + choices[0].y + 0x26) >> 1);
        }
        m_text.size = 10;
        m_text.align = TextStyle::Align::Left;
        // The button A is placed by its width and the button B by its height.
        s32 buttonY = 0x14A - m_buttonA->width;
        s32 cancelX = 0x160 - m_buttonB->height;
        s32 textY = (buttonY + 0x12) >> 1;
        DrawText("%wwSELECT", (m_buttonA->width + 0xDC) >> 1, textY);
        DrawText("%wwCANCEL", (cancelX + m_buttonB->width + 6) >> 1, textY);
        DrawTexturedSprite(*m_buttonA, 0xD6, buttonY);
        DrawTexturedSprite(*m_buttonB, cancelX, buttonY);
        UpdateStickRepeat();
        if ((m_input.buttons & (INPUT_RIGHT | INPUT_LEFT)) != 0) {
            Transfer(INPUT_RIGHT | INPUT_LEFT);
            m_styleMenuCursor ^= 1;
        }
    } else {
        DrawRectBorder(0xC6, 0x106, 0x116, 0x84, 2, 0, 0, 0x80, 0x80, 0x80);
        m_text.size = 12;
        SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
        m_text.align = TextStyle::Align::Center;
        DrawText("SELECT SHADOW COLOR", 0xA7, 0x92);
        // Two rows of eight.
        std::array<PaintRect, 16> swatches{};
        for (s32 i = 0; i < 16; ++i) {
            PaintRect &rect = swatches[static_cast<std::size_t>(i)];
            rect = {(i % 8) * 30 + 0xDE, (i >> 3) * 26 + 0x134, 0x14, 0x10};
            const auto &swatch = m_swatches[static_cast<std::size_t>(i)].color;
            DrawRectBorder(rect.x, rect.y, 0x14, 0x10, 1, 0, 1, swatch[0], swatch[1], swatch[2]);
        }
        PickMenuOption(swatches.data(), 16, m_textShadowColor);
        const PaintRect &chosen = swatches[m_textShadowColor];
        DrawRectOutline(chosen.x - 3,
                        chosen.y - 3,
                        chosen.width + 6,
                        chosen.height + 6,
                        1,
                        HIGHLIGHT_RED,
                        HIGHLIGHT_GREEN,
                        HIGHLIGHT_BLUE);
        s32 buttonY = 0x182 - m_buttonA->height;
        s32 cancelX = 0x16E - m_buttonB->height;
        s32 selectX = 0xE6;
        DrawTexturedSprite(*m_buttonA, selectX, buttonY);
        DrawTexturedSprite(*m_buttonB, cancelX, buttonY);
        m_text.size = 10;
        m_text.align = TextStyle::Align::Left;
        s32 textY = (buttonY + 0x12) >> 1;
        DrawText("%wwSELECT", (selectX + m_buttonA->width + 6) >> 1, textY);
        DrawText("%wwCANCEL", (cancelX + m_buttonB->width + 6) >> 1, textY);

        UpdateStickRepeat();
        if ((m_input.buttons & INPUT_LEFT) != 0) {
            Transfer(INPUT_LEFT);
            if (m_textShadowColor == 0) {
                m_textShadowColor = 7;
            } else if (m_textShadowColor == 8) {
                m_textShadowColor = 15;
            } else {
                --m_textShadowColor;
            }
        } else if ((m_input.buttons & INPUT_RIGHT) != 0) {
            Transfer(INPUT_RIGHT);
            if (m_textShadowColor == 7) {
                m_textShadowColor = 0;
            } else if (m_textShadowColor == 15) {
                m_textShadowColor = 8;
            } else {
                ++m_textShadowColor;
            }
        } else if ((m_input.buttons & (INPUT_DOWN | INPUT_UP)) != 0) {
            Transfer(INPUT_DOWN | INPUT_UP);
            m_textShadowColor = static_cast<u8>(m_textShadowColor < 8 ? m_textShadowColor + 8 :
                                                                        m_textShadowColor - 8);
        }
        if ((m_input.buttons & INPUT_A) != 0) {
            Transfer(INPUT_A);
            textTool.style = m_styleMenuCursor;
            m_textShadowPickerOpen = 0;
            m_styleMenuOpen = 0;
        } else if ((m_input.buttons & INPUT_B) != 0) {
            Transfer(INPUT_B);
            m_textShadowColor = m_savedColor;
            m_textShadowPickerOpen = 0;
        }
    }

    // A on the drop shadow opens its colours. A on normal text closes the menu without setting the
    // style, and a drop shadow stays on. Past the colours, the buttons are already taken.
    if ((m_input.buttons & INPUT_A) != 0) {
        Transfer(INPUT_A);
        if (m_styleMenuCursor) {
            m_textShadowPickerOpen = 1;
        } else {
            m_styleMenuOpen = 0;
        }
    } else if ((m_input.buttons & INPUT_B) != 0) {
        Transfer(INPUT_B);
        m_styleMenuOpen = 0;
        m_textShadowPickerOpen = 0;
    }
}

void PaintShop::ProcessRectStyleMenu() {
    // Both buttons are placed by their widths.
    s32 buttonY = 0x143 - m_buttonA->width;
    s32 cancelX = 0x161 - m_buttonB->width;
    DrawRectBorder(0x9F, 0x95, 0x142, 0xB6, 3, 0, 0, 0x80, 0x80, 0x80);
    m_text.size = 16;
    m_text.align = TextStyle::Align::Center;
    m_text.shadow = false;
    DrawText("%rySELECT STYLE", 0x9F, 0x5C);
    if (m_paintToolStarted) {
        m_paintToolStarted = 0;
        m_styleMenuCursor = m_paintToolIcons[PAINT_TOOL_RECTANGLE]->style;
    }
    std::array<PaintRect, 4> choices{};
    for (s32 i = 0, x = 0xAF; i < 4; ++i, x += 0x4C) {
        const PaintSprite &icon = *m_rectStyleIcons[static_cast<std::size_t>(i)];
        choices[static_cast<std::size_t>(i)] = {x, 0xCB, icon.drawWidth, icon.drawHeight};
    }
    PickMenuOption(choices.data(), 4, m_styleMenuCursor);
    for (std::size_t i = 0; i < 4; ++i) {
        PaintSprite &icon = *m_rectStyleIcons[i];
        if (i == m_styleMenuCursor) {
            DrawSpriteRegion(icon,
                             choices[i].x,
                             0xCB,
                             icon.drawWidth,
                             icon.drawHeight,
                             HIGHLIGHT_RED,
                             HIGHLIGHT_GREEN,
                             HIGHLIGHT_BLUE);
        } else {
            DrawTexturedSprite(icon, choices[i].x, 0xCB);
        }
    }
    bool names = !m_rectStyleNames[0].empty();
    if (names) {
        DrawRectBorder(0xB1, 0x103, 0x11E, 0x1E, 1, 1, 1, 0x80, 0x80, 0x80);
    }
    m_text.size = 11;
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    if (names) {
        DrawText(m_rectStyleNames[m_styleMenuCursor & 3], 0x9F, 0x8C);
    }
    m_text.size = 10;
    m_text.align = TextStyle::Align::Left;
    s32 textY = (buttonY + 0x12) >> 1;
    DrawText("%wwSELECT", (m_buttonA->width + 0xDB) >> 1, textY);
    DrawText("%wwCANCEL", (cancelX + m_buttonB->width + 6) >> 1, textY);
    DrawTexturedSprite(*m_buttonA, 0xD5, buttonY);
    DrawTexturedSprite(*m_buttonB, cancelX, buttonY);

    UpdateStickRepeat();
    MoveStyleMenuCursor();
    CloseStyleMenu(*m_paintToolIcons[PAINT_TOOL_RECTANGLE]);
}

void PaintShop::ProcessOvalStyleMenu() {
    s32 buttonY = 0x143 - m_buttonA->width;
    s32 cancelX = 0x160 - m_buttonB->width;
    DrawRectBorder(0xB8, 0x95, 0x110, 0xB6, 3, 0, 0, 0x80, 0x80, 0x80);
    m_text.size = 16;
    m_text.align = TextStyle::Align::Center;
    m_text.shadow = false;
    DrawText("%rySELECT STYLE", 0xA0, 0x5B);
    if (m_paintToolStarted) {
        m_styleMenuCursor = m_paintToolIcons[PAINT_TOOL_OVAL]->style;
        for (s32 i = 0, x = 0xC8; x < 0x158; ++i, x += 0x48) {
            m_ovalStyleIcons[static_cast<std::size_t>(i)]->originX = x;
            m_ovalStyleIcons[static_cast<std::size_t>(i)]->originY = 0xCB;
        }
        // The circles follow the ellipses, each placed from the second ellipse whatever its
        // column.
        for (s32 i = 0; i < 2; ++i) {
            m_ovalStyleIcons[static_cast<std::size_t>(i + 2)]->originX =
                m_ovalStyleIcons[static_cast<std::size_t>(i)]->drawWidth +
                m_ovalStyleIcons[1]->originX + i * 0x34;
            m_ovalStyleIcons[static_cast<std::size_t>(i + 2)]->originY = 0xCB;
        }
        m_paintToolStarted = 0;
    }
    std::array<PaintRect, 4> choices{};
    for (std::size_t i = 0; i < 4; ++i) {
        const PaintSprite &icon = *m_ovalStyleIcons[i];
        choices[i] = {icon.originX, icon.originY, icon.drawWidth, icon.drawHeight};
    }
    PickMenuOption(choices.data(), 4, m_styleMenuCursor);
    for (std::size_t i = 0; i < 4; ++i) {
        PaintSprite &icon = *m_ovalStyleIcons[i];
        if (i == m_styleMenuCursor) {
            DrawSpriteRegion(icon,
                             icon.originX,
                             icon.originY,
                             icon.drawWidth,
                             icon.drawHeight,
                             HIGHLIGHT_RED,
                             HIGHLIGHT_GREEN,
                             HIGHLIGHT_BLUE);
        } else {
            DrawSpriteAtOrigin(icon);
        }
    }
    bool names = !m_ovalStyleNames[0].empty();
    if (names) {
        DrawRectBorder(0xCE, 0x101, 0xE4, 0x1E, 1, 1, 1, 0x80, 0x80, 0x80);
    }
    m_text.size = 11;
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xF5, 0);
    if (names) {
        DrawText(m_ovalStyleNames[m_styleMenuCursor & 3], 0x9F, 0x8B);
    }
    m_text.size = 10;
    m_text.align = TextStyle::Align::Left;
    s32 textY = (buttonY + 0x12) >> 1;
    DrawText("%wwSELECT", (m_buttonA->width + 0xDC) >> 1, textY);
    DrawText("%wwCANCEL", (cancelX + m_buttonB->width + 6) >> 1, textY);
    DrawTexturedSprite(*m_buttonA, 0xD6, buttonY);
    DrawTexturedSprite(*m_buttonB, cancelX, buttonY);

    UpdateStickRepeat();
    MoveStyleMenuCursor();
    CloseStyleMenu(*m_paintToolIcons[PAINT_TOOL_OVAL]);
}

void PaintShop::ProcessPaintShopExitConfirm() {
    if (m_paintShopExiting) {
        return;
    }
    // The unsigned count is tested as signed.
    if (static_cast<s8>(m_decalEditCount) <= 0) {
        m_paintShopExiting = 1;
        m_screen.FadeOut();
        return;
    }
    // The button A is placed by its width.
    s32 buttonY = 0x11D - m_buttonA->width;
    s32 cancelX = 0x169 - m_buttonB->width;
    DrawRectBorder(0xBF, 0xBB, 0x102, 0x6A, 3, 0, 0, 0x80, 0x80, 0x80);
    m_text.size = 12;
    m_text.align = TextStyle::Align::Left;
    m_text.shadow = false;
    SetTextCustomColors(0xFF, 0xFF, 0xFF, 0xFF, 0xCA, 0);
    DrawText("CHANGES NOT SAVED.", 0x67, 0x6D);
    DrawText("EXIT ANYWAY?", 0x67, 0x7B);
    m_text.size = 10;
    s32 textY = (buttonY + 0x12) >> 1;
    DrawText("%wwYES", (m_buttonA->drawWidth + 0xF9) >> 1, textY);
    DrawText("%wwNO", (cancelX + m_buttonB->drawWidth + 8) >> 1, textY);
    DrawTexturedSprite(*m_buttonA, 0xF3, buttonY);
    DrawTexturedSprite(*m_buttonB, cancelX, buttonY);

    if ((m_input.buttons & INPUT_A) != 0) {
        Transfer(INPUT_A);
        m_paintShopExiting = 1;
        m_screen.FadeOut();
    } else if ((m_input.buttons & INPUT_B) != 0) {
        Transfer(INPUT_B);
        m_paintTool = m_previousPaintTool;
    }
}

} // namespace coastline
