#pragma once

/**
 * @file
 * The paint shop of the Nintendo 64 release, ProcessPaintShopScreen() of the console, with its
 * decal editor and the dialog that saves and loads the decals on the Controller Pak. The cursor
 * follows the stick of the controller and the mouse alike, and the buttons of the mouse are the A
 * and B buttons.
 */

#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "common/playerinput.h"
#include "common/types.h"
#include "controllerpak.h"
#include "decalcar.h"
#include "menutext.h"
#include "scenerenderer.h"

namespace coastline {

class MenuScreen;

/** The tools of the paint shop, each a button, as the console numbers them. */
enum PaintTool : u8 {
    PAINT_TOOL_BRUSH_SIZE, /*!< The buttons that pick the size of the brush. */
    PAINT_TOOL_BRUSH,      /*!< The brush, painting round or square dots. */
    PAINT_TOOL_FILL,       /*!< The flood fill. */
    PAINT_TOOL_LINE,       /*!< A straight line from an anchor to the cursor. */
    PAINT_TOOL_TEXT,       /*!< A text typed on the keyboard of the font of the decals. */
    PAINT_TOOL_RECTANGLE,  /*!< A rectangle in one of four styles. */
    PAINT_TOOL_OVAL,       /*!< An ellipse or a circle in one of four styles. */
    PAINT_TOOL_MIRROR,     /*!< The menu that mirrors or flips the decal. */
    PAINT_TOOL_CLEAR,      /*!< The menu that fills or clears the decal. */
    PAINT_TOOL_LOAD,       /*!< The dialog that loads decals from the Controller Pak. */
    PAINT_TOOL_SAVE,       /*!< The dialog that saves decals to the Controller Pak. */
    PAINT_TOOL_UNDO,       /*!< Undo the last edit and return to the tool before. */
    PAINT_TOOL_EXIT,       /*!< The dialog that confirms the exit from the paint shop. */
    PAINT_TOOLS,           /*!< The number of tools. */
};

/** The input of the player of the paint shop for a frame. */
struct PaintShopInput {
    u32 buttons = 0;     /*!< The INPUT_ bits down, with the buttons of the mouse as A and B. */
    float stickX = 0.0f; /*!< The stick across, from -1 for the left to 1 for the right. */
    float stickY = 0.0f; /*!< The stick up and down, from -1 for down to 1 for up. */
    /** Where the mouse moved to this frame on the 640 by 480 screen, or none when it is still. */
    std::optional<std::array<int, 2>> pointer;
    bool enter = false; /*!< Whether Return or keypad Enter is down, A in a dialog. */
    /** Whether Escape is down, B in a dialog and on the keyboard of the text tool. */
    bool escape = false;
    /**
     * The characters typed on a keyboard this frame in UTF-8, with Backspace as `'\b'` and Enter
     * as `'\r'`, in the order typed.
     */
    std::string text;
};

/** A rectangle of the 640 by 480 screen, ScreenRect of the console. */
struct PaintRect {
    s32 x = 0;      /*!< The left edge. */
    s32 y = 0;      /*!< The top edge. */
    s32 width = 0;  /*!< The width. */
    s32 height = 0; /*!< The height. */
};

/** A picture of the paint shop, Sprite of the console. */
struct PaintSprite {
    std::vector<u8> pixels; /*!< The texels as the cartridge has them, the bottom row first. */
    u8 bitsPerTexel = 0;    /*!< The bits of a texel: 4, 8, or 16. */
    s32 width = 0;          /*!< The width in texels. */
    s32 height = 0;         /*!< The height in texels. */
    s32 originX = 0;        /*!< The x the picture is drawn from. */
    s32 originY = 0;        /*!< The y the picture is drawn from. */
    s32 drawWidth = 0;      /*!< The width it is drawn at, negative to flip it. */
    s32 drawHeight = 0;     /*!< The height it is drawn at. */
    u8 style = 0;           /*!< The style of a tool, or 99 for a picture drawn in a colour. */
    std::string asset;      /*!< The asset of the cartridge the texels come from. */
    std::unique_ptr<RenderTexture> texture; /*!< The texels upright, made at the first draw. */
};

/** A colour of the palette of the decals and its swatch, PaletteSwatch of the console. */
struct PaintSwatch {
    PaintRect rect;            /*!< Where the swatch is. */
    std::array<u8, 3> color{}; /*!< The red, green, and blue, from 0 to 255. */
};

/** A key of the keyboard of the text tool and its glyph, DecalGlyph of the console. */
struct PaintGlyph {
    s32 x = 0;         /*!< The left of the key. */
    s32 y = 0;         /*!< The top of the key. */
    u8 character = 0;  /*!< The character. */
    s32 cellX = 0;     /*!< Where the glyph starts in its cell of 16 texels of the font. */
    s32 width = 0;     /*!< The width of the glyph in texels. */
    s32 kernLeft = 0;  /*!< How far the glyph may move to the one before it. */
    s32 kernRight = 0; /*!< How far the glyph may move to the one after it. */
};

/** A direction of the console. */
struct PaintVector {
    f32 x = 0.0f; /*!< Across. */
    f32 y = 0.0f; /*!< Along. */
    f32 z = 0.0f; /*!< Up. */
};

/** The paint shop of the console. */
class PaintShop {
public:
    /**
     * Read the pictures and the tables of the paint shop from the import of a release.
     *
     * @param renderer The renderer that draws the paint shop.
     * @param release The directory of the import of the release, such as n64/usa.
     * @param screen The menu screens, whose fades and fonts the paint shop shares.
     * @param textColors The custom colours of the text of the menus, set by the paint shop.
     * @param pak The Controller Pak the decals are saved on.
     * @param work The directory the images of the painted car are written to for the preview.
     * @return The paint shop, or nothing with the reason logged.
     */
    static std::unique_ptr<PaintShop> Load(SceneRenderer &renderer,
                                           const std::filesystem::path &release,
                                           MenuScreen &screen,
                                           std::optional<std::array<int, 6>> &textColors,
                                           ControllerPak pak,
                                           std::filesystem::path work);
    ~PaintShop();
    PaintShop(const PaintShop &) = delete;
    PaintShop &operator=(const PaintShop &) = delete;

    /**
     * Choose the car the paint shop opens with, as the car select screen exits to it.
     *
     * @param car The car, an index of the cars of the menus.
     * @param directory The directory of the car in the import.
     */
    void SetCar(int car, const std::filesystem::path &directory);

    /**
     * Run a frame of the paint shop and draw it.
     *
     * @param renderer The renderer of the frame, between Begin() and End().
     * @param seconds The time of the frame.
     * @param input The input of the player.
     * @return Whether the paint shop has closed for the main menu.
     */
    bool Frame(SceneRenderer &renderer, float seconds, const PaintShopInput &input);

    /** @return Whether the keyboard of the text tool shows and reads PaintShopInput::text. */
    bool TakesText() const;

    /**
     * Run a frame of the dialog that loads the decals on the car select screen, as the car select
     * screen of the console runs ProcessDecalSaveLoad() and DrawPakErrorMessage().
     *
     * @param renderer The renderer of the frame, between Begin() and End().
     * @param seconds The time of the frame.
     * @param input The input of the player.
     * @param car The car the decals load into.
     * @return Whether the dialog has closed, and with it whether the decals loaded.
     */
    std::optional<bool> LoadDecalsFrame(SceneRenderer &renderer,
                                        float seconds,
                                        const PaintShopInput &input,
                                        DecalCar &car);

    /**
     * Load the decals of the Controller Pak into a car at once, without the dialog, for the
     * custom decals a player chose before.
     *
     * @param car The car the decals load into.
     * @return Whether the decals loaded.
     */
    bool LoadDecalsQuietly(DecalCar &car);

private:
    // The screens of the Controller Pak of the menus draw with the drawing and the pictures of the
    // paint shop, as the console shares them.
    friend class Menus;

    PaintShop(MenuScreen &screen,
              std::optional<std::array<int, 6>> &textColors,
              ControllerPak pak,
              std::filesystem::path work);

    // The drawing of the console on the 640 by 480 screen.
    void BeginDraw(SceneRenderer &renderer);
    void FlushFills();
    void FillRect(s32 x0, s32 y0, s32 x1, s32 y1, u8 red, u8 green, u8 blue);
    void DrawFilledRectHiRes(s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue);
    void
    DrawRectOutline(s32 x, s32 y, s32 width, s32 height, s32 thickness, u8 red, u8 green, u8 blue);
    void DrawRectBorder(s32 x,
                        s32 y,
                        s32 width,
                        s32 height,
                        s32 border,
                        u8 transparent,
                        u8 sunken,
                        u8 red,
                        u8 green,
                        u8 blue);
    void DrawPixel(s32 x, s32 y, u8 shade);
    void DrawSpriteQuad(PaintSprite &sprite,
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
                        u8 blue);
    void DrawSpriteStrips(PaintSprite &sprite,
                          s32 x,
                          s32 y,
                          s32 drawWidth,
                          s32 drawHeight,
                          u8 red,
                          u8 green,
                          u8 blue);
    void DrawSpriteAtOrigin(PaintSprite &sprite);
    void DrawTexturedSprite(PaintSprite &sprite, s32 x, s32 y);
    void DrawSpriteTinted(PaintSprite &sprite, u8 red, u8 green, u8 blue);
    void DrawSpriteRegion(
        PaintSprite &sprite, s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue);
    void DrawScaledTexturedRect(PaintSprite &sprite,
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
                                u8 blue);
    void EmitScaledTextureRect(s32 x, s32 y, s32 width, s32 height);
    void DrawText(const std::string &text, s32 x, s32 y);
    void SetTextCustomColors(u8 red0, u8 green0, u8 blue0, u8 red1, u8 green1, u8 blue1);

    // The input of the player as the console reads it.
    void TranslateInput(float seconds, const PaintShopInput &input, bool dialog);
    bool DialogShows() const;
    void Transfer(u32 mask);
    void UpdateStickRepeat();
    void CheckStick(s32 &timer, f32 axis, f32 sign, u32 button);

    // paintshop.c of the console.
    bool LoadTextureFromRom(PaintSprite &sprite);
    void RenderDecalPreview3D();
    void SelectDecalTexels(int slot);
    void SetUpPaintShopCar();
    void LoadDecalSlotIcons();
    void LayOutPaintShopButtons();
    void LoadPaintToolIcons();
    void LoadStyleIcons();
    void LayOutPaintShopControls();
    bool InitPaintShop();
    void DrawPaintShopButton(const PaintRect &rect, u8 red, u8 green, u8 blue);
    void DrawBrushSizes(PaintSprite &sizes, const PaintRect &chosen);
    void DrawPaintShop();
    void DrawToolCursor();
    void RunPaintShopPakDialog(u8 operation);
    void RunPaintShopMenus();
    void HandlePaintShopB();
    void ResetPaintShopState();
    void SaveDecalUndo();
    void UndoDecalEdit();
    void DrawDialogBox(s32 x, s32 y, s32 width, s32 height);
    void DrawSunkenFrame(s32 x, s32 y, s32 width, s32 height);
    void SetPakTextColors(u8 inCarSelect);
    void DrawPakErrorMessage(s32 error, u8 operation, u8 inCarSelect);
    void DrawMenuButtons(s32 buttonY, s32 cancelX);
    void ClearDecal(u8 coloredOption);
    void ProcessFillClearMenu();
    void ProcessMirrorFlipMenu();
    void MirrorDecalToOppositePanel();
    void FlipDecalPixels();
    void DrawPakProgressBox(s32 y, const char *text, s32 textX, s32 textY);
    void CopyUnmaskedTexels(u8 *texels, const u8 *mask, s32 width, s32 height);
    void TransferDecal(DecalCar &car, u8 operation);
    void LoadDecalPalette(DecalCar &car, std::size_t offset);
    void DrawPakButtons(s32 buttonY, u32 okTextX, u32 cancelTextX, s32 okX, s32 cancelX);
    s32 ProcessDecalSaveLoad(DecalCar &car, u8 operation, u8 inCarSelect, bool &done);
    void ProcessTextInputKeyboard();
    void PressKeyboardKey(u8 key);
    void TypeOnKeyboard();

    // decaleditor.c of the console.
    void SelectDecalSlot(s32 slot);
    s32 AnchorDistance() const;
    void ProcessColorEditorDialog();
    void UpdateCursorFromInput();
    bool PickDecalSlot();
    bool PickPaletteColor();
    bool PickPaintTool();
    void ProcessTextTool();
    void PaintBrush();
    bool StartShape();
    void PaintRectangle();
    void PaintOval();
    bool PaintWithTool();
    void DrawShapeOutline();
    void ProcessDecalEditorInput();
    bool IsCursorInButton(const PaintSprite &sprite) const;
    bool IsCursorInRect(const PaintRect &rect) const;
    bool IsCursorOnKeyboardKey(const PaintGlyph &key) const;
    void ConvertPaletteToRgb();
    void DrawDecalColorPalette();
    void MoveStyleMenuCursor();
    void CloseStyleMenu(PaintSprite &tool);
    void ProcessBrushStyleMenu();
    void ProcessTextStyleMenu();
    void ProcessRectStyleMenu();
    void ProcessOvalStyleMenu();
    void ProcessPaintShopExitConfirm();
    void SetPalette(int entry, u8 red, u8 green, u8 blue);
    void CopyLightPalettes();
    void PickMenuOption(const PaintRect *options, int count, u8 &cursor);

    // decaldraw.c of the console.
    u8 GetPixel4Bit(s32 x, s32 y) const;
    u8 ReadPixel4Bit(const u8 *texels, s32 x, s32 y) const;
    void FloodFill4Bit(s32 x, s32 y);
    void StampTextOnDecal(s32 x, s32 y);
    void DrawLine4Bit(s32 x0, s32 y0, s32 x1, s32 y1);
    void SetPixel4Bit(s32 x, s32 y, u8 color);
    void FillTexels(s32 x0, s32 y0, s32 x1, s32 y1);
    void FillRect4Bit(s32 x0, s32 y0, s32 x1, s32 y1);
    void StrokeDecalRectangle(s32 x0, s32 y0, s32 x1, s32 y1);
    void FillDecalRoundRect(s32 x0, s32 y0, s32 x1, s32 y1);
    void DrawDecalRoundedRectOutline(s32 x0, s32 y0, s32 x1, s32 y1);
    void DrawDecalFilledEllipse(s32 x0, s32 y0, s32 x1, s32 y1);
    void DrawDecalEllipseOutline(s32 x0, s32 y0, s32 x1, s32 y1);
    void FillCircle4Bit(s32 x, s32 y, s32 radius, bool fromScreen);
    void DrawDecalCircleOutline(s32 x, s32 y, s32 radius);
    void AdvanceDashes();
    void DrawDashedLine(s32 x0, s32 y0, s32 x1, s32 y1);
    void DrawDashedRectangle(s32 x0, s32 y0, s32 x1, s32 y1);
    void DrawDashedRoundedRect(s32 x0, s32 y0, s32 x1, s32 y1);
    void DrawDashedEllipse(s32 x0, s32 y0, s32 x1, s32 y1);
    void DrawDashedCircle(s32 x, s32 y, s32 radius);
    void DrawDecalTextPreview(s32 x, s32 y);
    s32 ScreenToTexelX(s32 x) const;
    s32 ScreenToTexelY(s32 y) const;
    u8 FontTexel(s32 column, s32 row) const;
    s32 GlyphAdvance(int glyph, int next) const;

    MenuScreen &m_screen;
    std::optional<std::array<int, 6>> &m_textColors;
    ControllerPak m_pak;
    std::filesystem::path m_work;
    std::filesystem::path m_release;
    std::filesystem::path m_carDirectory;

    // The drawing of the frame.
    SceneRenderer *m_renderer = nullptr;
    Matrix4 m_flat{};
    std::vector<FlatVertex> m_fills;
    std::vector<std::unique_ptr<RenderTexture>> m_frameTextures;
    std::vector<std::unique_ptr<RenderTexture>> m_lastFrameTextures;
    TextStyle m_text;

    // The input of the player, and the time of the frame.
    PlayerInputState m_input{};
    std::string m_typedText;
    s32 m_frameMilliseconds = 0;
    // The counts of frames of the console advance once a 60th of a second at any frame rate: the
    // time toward the next count, and whether one is due this frame.
    float m_consoleFrameTime = 0.0f;
    bool m_consoleFrame = true;
    // Where the mouse is on the 640 by 480 screen, and whether it moved this frame.
    std::array<int, 2> m_pointer{};
    bool m_pointerMoved = false;
    bool m_pointerSeen = false;
    bool m_escapeDown = false;

    // The pictures, by their address in the main segment of the console.
    std::map<u32, PaintSprite> m_sprites;
    PaintSprite *m_background = nullptr;
    PaintSprite *m_logo = nullptr;
    PaintSprite *m_paintWholeCar = nullptr;
    PaintSprite *m_panel8Overlay = nullptr;
    PaintSprite *m_panel9Overlay = nullptr;
    PaintSprite *m_brushSizeRound = nullptr;
    PaintSprite *m_brushSizeSquare = nullptr;
    PaintSprite *m_paintCursor = nullptr;
    PaintSprite *m_fillCursor = nullptr;
    PaintSprite *m_brushCursor = nullptr;
    PaintSprite *m_crosshairCursor = nullptr;
    PaintSprite *m_textCursor = nullptr;
    PaintSprite *m_eyedropperCursor = nullptr;
    PaintSprite *m_keyboardPanel = nullptr;
    PaintSprite *m_keyboardBackground = nullptr;
    PaintSprite *m_keyboardHighlight = nullptr;
    PaintSprite *m_buttonA = nullptr;
    PaintSprite *m_buttonB = nullptr;
    PaintSprite *m_decalFont = nullptr;
    PaintSprite *m_shapeFilledCircle = nullptr;
    std::array<PaintSprite *, DECAL_SLOTS> m_decalSlotIcons{};
    std::array<PaintSprite *, 13> m_paintToolIcons{};
    std::array<PaintSprite *, 4> m_rectStyleIcons{};
    std::array<PaintSprite *, 4> m_ovalStyleIcons{};

    // The tables of the console.
    PaintRect m_decalCanvasFrame;
    PaintRect m_paletteFrameLeft;
    PaintRect m_paletteFrameRight;
    std::array<PaintRect, 5> m_brushSizeRects{};
    std::array<PaintRect, 5> m_brushSizeButtonRects{};
    std::array<PaintGlyph, 50> m_decalGlyphs{};
    std::array<std::array<u8, 3>, 3> m_colorEditorBars{};
    std::array<std::string, 3> m_colorChannelNames;
    std::array<std::string, 2> m_brushStyleNames;
    std::array<std::string, 2> m_textStyleNames;
    std::array<std::string, 4> m_rectStyleNames;
    std::array<std::string, 4> m_ovalStyleNames;
    std::array<PaintVector, DECAL_SLOTS> m_panelViewDirections{};

    // The state of the paint shop, as the globals of the console.
    std::optional<DecalCar> m_car;
    int m_carModel = 0;
    bool m_initialized = false;
    bool m_previewDirty = true;
    int m_previewWait = 0;
    std::unique_ptr<RenderModel> m_preview;
    std::unique_ptr<RenderTexture> m_previewReflection;
    PaintVector m_frameForward;
    PaintVector m_frameUp;
    PaintVector m_frameSide;
    PaintVector m_framePosition;
    PaintVector m_cameraPosition;
    std::array<PaintSwatch, 16> m_swatches{};
    std::array<PaintRect, 13> m_paintToolButtonRects{};
    std::array<PaintRect, DECAL_SLOTS> m_decalSlotRects{};
    PaintRect m_decalCanvas;
    PaintRect m_brush;
    std::array<int, 16> m_decalTextGlyphs{};
    std::string m_decalText;
    std::vector<u8> m_undoBuffer;
    std::vector<u8> m_scratchTexels;
    u8 *m_decalTexels = nullptr;
    const u8 *m_decalTexelsEnd = nullptr;
    const u8 *m_decalMask = nullptr;
    int m_decalPaletteMaterial = 0;
    u8 m_decalWidth = 0;
    u8 m_decalHeight = 0;
    u8 m_dashColorA = 0;
    u8 m_dashColorB = 0;
    u8 m_brushSize = 0;
    u8 m_savedColor = 0;
    u8 m_decalColor = 0;
    u8 m_textShadowColor = 0;
    u8 m_paintTool = 0;
    u8 m_previousPaintTool = 0;
    u8 m_decalSlot = 0;
    u8 m_previousDecalSlot = 0;
    u8 m_savedDecalSlot = 0;
    u8 m_undoDecalSlot = 0;
    u8 m_decalTextLength = 0;
    u8 m_decalTextWidth = 0;
    u8 m_decalEditCount = 0;
    u8 m_styleMenuCursor = 0;
    u8 m_paintToolStarted = 0;
    u8 m_decalDragActive = 0;
    u8 m_paletteEnabled = 0;
    u8 m_decalSlotHasMirror = 0;
    u8 m_decalSlotChanged = 0;
    u8 m_colorEditorOpen = 0;
    u8 m_styleMenuOpen = 0;
    u8 m_textShadowPickerOpen = 0;
    u8 m_paintShopExiting = 0;
    u8 m_colorEditorChannel = 0;
    u8 m_keyboardHoverKey = 0;
    u8 m_showPreview3D = 0;
    u8 m_undoAvailable = 0;
    u8 m_eyedropperActive = 0;
    u8 m_mirrorTargetPanel = 0;
    std::array<u8, 3> m_savedSwatchColor{};
    s32 m_paintAnchorX = 0;
    s32 m_paintAnchorY = 0;
    u32 m_lastClickTime = 0;
    bool m_anchorXInitialized = false;
    bool m_anchorYInitialized = false;
    u16 m_screenFrameCount = 0;
    s32 m_controllerPakState = 0;

    // The dialog of the Controller Pak.
    u8 m_pakDialogActive = 0;
    u8 m_pakFileNeedsCreate = 0;
    u8 m_pakDialogState = 0;
    u8 m_pakDecalIndex = 0;
    u8 m_pakWaitFrames = 0;
    s32 m_pakFileOffset = 0;
    s32 m_pakError = 0;
    s32 m_pakChunkSize = 0;
    std::vector<u8> m_pakNote;
    bool m_pakDone = false;
    s32 m_pakResult = 0;
};

} // namespace coastline
