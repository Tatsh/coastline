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
#include <span>
#include <string>
#include <string_view>
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
    /** Whether Ctrl+Z was pressed this frame to undo an edit outside a dialog. */
    bool undo = false;
    /** Whether Ctrl+Shift+Z was pressed this frame to redo an edit outside a dialog. */
    bool redo = false;
    bool constrain = false; /*!< Whether Shift or controller Z constrains a shape. */
    /**
     * The characters typed on a keyboard this frame in UTF-8, with Backspace as code point 8 and
     * Enter as code point 13, in the order typed.
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
     * @param release The selected cartridge directory, n64/base of the prepared assets.
     * @param screen The menu screens, whose fades and fonts the paint shop shares.
     * @param textColors The custom colours of the text of the menus, set by the paint shop.
     * @param pak The Controller Pak the decals are saved on.
     * @return The paint shop, or nothing with the reason logged.
     */
    static std::unique_ptr<PaintShop> Load(SceneRenderer &renderer,
                                           const std::filesystem::path &release,
                                           MenuScreen &screen,
                                           std::optional<std::array<int, 6>> &textColors,
                                           ControllerPak pak);
    /** Release paint previews and editing resources before the menu screen and GPU device. */
    ~PaintShop();
    /** Prevent copying active editing and preview resources. */
    PaintShop(const PaintShop &) = delete;
    /**
     * Prevent assignment of active editing and preview resources.
     *
     * @return This object. The operator is deleted and is never called.
     */
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
     * @return Whether the paint shop has closed for car select.
     */
    bool Frame(SceneRenderer &renderer, float seconds, const PaintShopInput &input);

    /**
     * Report whether typed text goes to the paint shop, for the caller to start or stop text input.
     *
     * @return Whether the keyboard of the text tool shows and reads PaintShopInput::text.
     */
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

    // Store the shared menu objects. Load() fills the pictures and the tables. The paint shop sets
    // the custom text colours of the menus through textColors.
    PaintShop(MenuScreen &screen, std::optional<std::array<int, 6>> &textColors, ControllerPak pak);

    // The drawing of the console on the 640 by 480 screen. Colour channels run from 0 to 255.

    // Start drawing a frame. The fill queue is emptied, and the decal textures of the frame before
    // are retained until the next call. The renderer is used until the next call.
    void BeginDraw(SceneRenderer &renderer);
    // Draw the queued fill rectangles in one batch and empty the queue.
    void FlushFills();
    // Queue a rectangle of fill colour, as the fill cycle of the RDP draws it, with the far column
    // and row included. Each channel is reduced to 5 bits.
    void FillRect(s32 x0, s32 y0, s32 x1, s32 y1, u8 red, u8 green, u8 blue);
    // Queue a filled rectangle by its corner and size, with a size below 2 taken as 1. The size
    // extends to a right column and a bottom row FillRect() includes.
    void DrawFilledRectHiRes(s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue);
    // Queue the four edges of a rectangle, each drawn inside it. A thickness below 2 is taken as 1.
    void
    DrawRectOutline(s32 x, s32 y, s32 width, s32 height, s32 thickness, u8 red, u8 green, u8 blue);
    // Queue a bevelled border around a rectangle, light at the left and the top and dark at the
    // right and the bottom, and optionally the face of the rectangle. The border is the pixels
    // outside the face, with a value below 2 taken as 1. A nonzero transparent omits the face, and
    // a sunken of 1 swaps the light and the dark edges.
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
    // Queue a grey point of a dashed outline, through FillRect() from x to x + 1 and y to y + 1.
    // The shade is the level of red, green, and blue.
    void DrawPixel(s32 x, s32 y, u8 shade);
    // Draw part of a picture over a rectangle after the queued fills. The texture of the picture
    // is created at the first draw. A picture of 4-bit texels is the colour times their intensity,
    // a tinted 8-bit picture is its texels times the colour, and other pictures are their texels.
    // Nothing is drawn for a picture without texels. The texel rows t0 and t1 are counted from the
    // top of the upright picture.
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
    // Draw a whole picture over a rectangle, as DrawSpriteStrip() of the console steps its strips.
    // A picture of other than 4, 8, or 16 bits a texel is not drawn. A negative drawWidth mirrors
    // the picture.
    void DrawSpriteStrips(PaintSprite &sprite,
                          s32 x,
                          s32 y,
                          s32 drawWidth,
                          s32 drawHeight,
                          u8 red,
                          u8 green,
                          u8 blue);
    // Draw a picture at its origin and draw size, in black.
    void DrawSpriteAtOrigin(PaintSprite &sprite);
    // Draw a picture at a place, at its draw size, in black.
    void DrawTexturedSprite(PaintSprite &sprite, s32 x, s32 y);
    // Draw a picture at its origin and draw size in a colour.
    void DrawSpriteTinted(PaintSprite &sprite, u8 red, u8 green, u8 blue);
    // Draw the background in a colour. With wide menus on a frame wider than 4:3, the background
    // is scaled about the centre of the screen to the width of the frame.
    void DrawWideBackground(u8 red, u8 green, u8 blue);
    // Draw a whole picture over a rectangle in a colour, through DrawSpriteStrips(). A negative
    // width mirrors the picture.
    void DrawSpriteRegion(
        PaintSprite &sprite, s32 x, s32 y, s32 width, s32 height, u8 red, u8 green, u8 blue);
    // Draw a rectangle of the texels of a picture over a rectangle of the screen. As on the
    // console, the first column drawn is the one before the source rectangle. The source row srcY
    // is counted from the top of the upright picture.
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
    // Draw the decal of the selected slot in the colours of its palette, or the image of a
    // numbered panel, over a rectangle. The texture is released at the second BeginDraw() after it.
    void EmitScaledTextureRect(s32 x, s32 y, s32 width, s32 height);
    // Find whether a UTF-8 message is Japanese, to choose its size and place. A message is
    // Japanese when a code point of it is from U+3000 to U+9FFF.
    static bool JapaneseMessage(std::string_view text);
    // Draw a UTF-8 message, with the colour codes of the console, in the style of m_text and the
    // custom colours, after the queued fills. The x places each line by the alignment of m_text,
    // and y is the baseline of the first line, both in pixels of the 320 by 240 screen. Each line
    // after a newline is lineHeight below the line before, or the size of the text for 0. A
    // non-empty lineGaps gives the distance of each line in place of lineHeight.
    void DrawText(std::string_view text,
                  s32 x,
                  s32 y,
                  int lineHeight = 0,
                  std::span<const int> lineGaps = {});
    // Set the custom colours of the text of the menus, shared with the menu screens. The first
    // three channels are the bottom of the gradient and the last three the top.
    void SetTextCustomColors(u8 red0, u8 green0, u8 blue0, u8 red1, u8 green1, u8 blue1);

    // The input of the player as the console reads it.

    // Read the input of a frame into m_input. Enter adds A in a dialog, a press of Escape adds B in
    // a dialog or while typing and opens the exit dialog elsewhere, and the mouse moves the cursor
    // outside a dialog. The counts of console frames advance once a 60th of a second. The dialog
    // flag records whether a menu or a dialog shows.
    void TranslateInput(float seconds, const PaintShopInput &input, bool dialog);
    // Find whether a menu or a dialog takes the input. That is, whether the mirror, clear, load,
    // save, or exit tool is chosen, or the style menu or the colour editor is open.
    bool DialogShows() const;
    // Move the INPUT_ buttons of mask out of the input until they are released. One press then
    // acts once.
    void Transfer(u32 mask);
    // Add the directions the stick presses this frame, with its repeat, to the buttons.
    void UpdateStickRepeat();
    // Press a direction of the stick past 0.6, then again after 500 milliseconds and every 200
    // milliseconds while the stick stays past 0.3. The timer is the milliseconds to the next press,
    // positive before the first repeat and negative after it, or 0 while the stick is released.
    // The axis runs from -1 to 1, and a sign of 1 or -1 selects the direction that presses.
    void CheckStick(s32 &timer, f32 axis, f32 sign, u32 button);

    // paintshop.c of the console.

    // Read the texels of a picture, with its asset, size, and bits a texel set, from the import:
    // from its .bin file or, failing that, from its PNG converted back to the texels of the
    // cartridge. Any texture made before is released. Returns whether the texels have the size of
    // the picture. Otherwise the error is logged and the texels are cleared.
    bool LoadTextureFromRom(PaintSprite &sprite);
    // Turn the preview of the car toward the selected decal slot over 16 console frames, reload
    // the model with the painted images at most every 8 console frames after an edit, and draw it
    // in the viewport at the bottom left of the screen.
    void RenderDecalPreview3D();
    // Point the texels, the mask, and the size of the edited decal at a slot of the car, without
    // changing the selected slot.
    void SelectDecalTexels(int slot);
    // Load the car of SetCar() and reset the preview. A selected slot the car does not have falls
    // back to the paint.
    void SetUpPaintShopCar();
    // Load the buttons of the decal slots, share the texels of each left panel with its right
    // panel, place the overlays of slots 8 and 9, and set the rectangle each slot is clicked in.
    void LoadDecalSlotIcons();
    // Place the brush-size panel, the two palette frames, and the three rows of tool buttons.
    void LayOutPaintShopButtons();
    // Place and load the brush-size pictures and the pictures of the tool buttons.
    void LoadPaintToolIcons();
    // Load the pictures of the four styles of the rectangle tool and of the oval tool.
    void LoadStyleIcons();
    // Place the brush-size buttons, the palette swatches, and the keys of the keyboard, and read
    // the palette into the swatches.
    void LayOutPaintShopControls();
    // Open the paint shop for the car of SetCar(). The car and the pictures load, the controls are
    // placed, and the fade in starts. Returns whether the car and every picture loaded.
    bool InitPaintShop();
    // Draw a button as a frame in a colour around a yellow face.
    void DrawPaintShopButton(const PaintRect &rect, u8 red, u8 green, u8 blue);
    // Draw the row of the round or square brush-size picture of the chosen size in yellow, over
    // its black highlight.
    void DrawBrushSizes(PaintSprite &sizes, const PaintRect &chosen);
    // Draw the background, the logo, the title, the decal slots, the decal, the palette, the brush
    // sizes, and the tool buttons.
    void DrawPaintShop();
    // Draw the cursor of the chosen tool, or the eyedropper while B picks a colour.
    void DrawToolCursor();
    // Run the load or save dialog of the paint shop (PAINT_TOOL_LOAD or PAINT_TOOL_SAVE), then its
    // error message until A or B dismisses it and the tool before returns.
    void RunPaintShopPakDialog(u8 operation);
    // Run the menu or the dialog of the chosen tool, the style menu, or the colour editor, or the
    // input of the editor when none is open.
    void RunPaintShopMenus();
    // Pick the colour under the cursor while B is pressed over the decal, or cancel a started
    // shape or text with B.
    void HandlePaintShopB();
    // Reset the cursor, the slot, the brush, the colour, the tool, the styles, and the undo history
    // to those the paint shop opens with.
    void ResetPaintShopState();
    // Record the car before an edit for UndoDecalEdit(), clear the redo history, and copy the
    // texels of the edited decal to m_undoBuffer.
    void SaveDecalUndo();
    // Restore the car before the last edit, and record the current car for RedoDecalEdit().
    void UndoDecalEdit();
    // Restore the car of the last undone edit, and record the current car for UndoDecalEdit().
    void RedoDecalEdit();
    // Draw the grey raised box of a dialog with a border of 3 pixels around the face.
    void DrawDialogBox(s32 x, s32 y, s32 width, s32 height);
    // Draw a sunken frame of 1 pixel without a face around the inside rectangle.
    void DrawSunkenFrame(s32 x, s32 y, s32 width, s32 height);
    // Set the text colours of the dialogs of the Controller Pak, white over yellow. An inCarSelect
    // of 1 selects the paler yellow of the car select screen.
    void SetPakTextColors(u8 inCarSelect);
    // Draw the message of an error of the Controller Pak in its box. The error is a PFS_ error, or
    // 99999 for a note too short for the decals, and an unknown error draws nothing. The operation
    // (PAINT_TOOL_LOAD or PAINT_TOOL_SAVE) chooses the message and the width, and an inCarSelect of
    // 1 selects the places of the car select screen.
    void DrawPakErrorMessage(s32 error, u8 operation, u8 inCarSelect);
    // Draw the A button labelled SELECT and the B button labelled CANCEL of a menu.
    void DrawMenuButtons(s32 buttonY, s32 cancelX);
    // Set every texel of the edited decal outside its mask to the chosen colour or to the body
    // colour. The style coloredOption of the clear tool fills with the chosen colour, and any other
    // style fills with colour 0, the body colour.
    void ClearDecal(u8 coloredOption);
    // Run the menu of the clear tool. Its options fill the decal or every decal with the chosen
    // colour or the body colour.
    void ProcessFillClearMenu();
    // Run the menu of the mirror tool. Its options mirror the decal to the opposite panel, or flip
    // it across or up and down.
    void ProcessMirrorFlipMenu();
    // Copy the edited decal mirrored to the panel on the other side of the car and select that
    // panel. The size of the source is retained.
    void MirrorDecalToOppositePanel();
    // Flip the edited decal across or up and down by the style of the mirror tool, reading from
    // the copy SaveDecalUndo() makes.
    void FlipDecalPixels();
    // Draw the box of a transfer of the Controller Pak with its UTF-8 message and an empty progress
    // bar. A Japanese message is centred. The message place is in pixels of the 320 by 240 screen.
    void DrawPakProgressBox(s32 y, std::string_view text, s32 textX, s32 textY);
    // Copy the texels from m_undoBuffer where the mask bit is clear. A texel copies the whole byte
    // it shares with its neighbour. The mask has a bit for each texel, the first texel of a byte in
    // its highest bit.
    void CopyUnmaskedTexels(u8 *texels, const u8 *mask, s32 width, s32 height);
    // Move the decal of m_pakDecalIndex between the car and m_pakNote at m_pakFileOffset, and
    // advance the offset. A slot the car does not have moves nothing. A load goes through
    // m_undoBuffer and the mask. A note too short sets m_pakError to 99999.
    void TransferDecal(DecalCar &car, u8 operation);
    // Load the 16 colours of the paint from m_pakNote at offset, and copy the first two to the
    // lights of a car with decals.
    void LoadDecalPalette(DecalCar &car, std::size_t offset);
    // Draw the A button labelled OK and the B button labelled CANCEL of a dialog of the Controller
    // Pak. The label places are in pixels of the 640 by 480 screen.
    void DrawPakButtons(s32 buttonY, u32 okTextX, u32 cancelTextX, s32 okX, s32 cancelX);
    // Run a frame of the dialog that saves or loads the decals and the palette, through the states
    // of the dialog of the console. An inCarSelect of 1 marks the car select screen, where the
    // sounds fade first and the dialog opens without its question. Sets done to whether the dialog
    // finished without an error or was cancelled, and returns 0 or the error it finished with.
    s32 ProcessDecalSaveLoad(DecalCar &car, u8 operation, u8 inCarSelect, bool &done);
    // Draw the keyboard of the text tool, press the key clicked with A or Z, type the keys of the
    // computer, and draw the text typed.
    void ProcessTextInputKeyboard();
    // Press the key of m_decalGlyphs at index key. Enter ends the text, Backspace removes the last
    // character, and another key adds its character when the text fits the decal and has fewer
    // than 16 characters.
    void PressKeyboardKey(u8 key);
    // Press the keys of the characters typed on the computer this frame. A letter the font has
    // only in capitals types the capital, and other characters the font lacks are dropped.
    void TypeOnKeyboard();

    // decaleditor.c of the console.

    // Edit the texels of a decal slot, centred in the canvas, and start the preview turning to
    // it. As on the console, the mask is that of m_decalSlot.
    void SelectDecalSlot(s32 slot);
    // Find whether a decal slot can be mirrored to the panel on the other side of the car. That
    // is, whether the car has decals, the slot is a door or a fender, and the slot paired with it
    // exists with the same size.
    bool HasOppositePanel(int slot) const;
    // Find the rectangle a decal slot is clicked in. For the index of a numbered panel, the result
    // is the place of the panel in its grid of four columns.
    PaintRect DecalSlotRect(int slot) const;
    // Size the canvas to the edited decal and centre it in its frame. The zoom is 4 screen pixels
    // a texel, halved for a numbered panel until the panel fits.
    void LayOutDecalCanvas();
    // Find the Controller Pak the decals of a car are saved on. That is, the pak for the decals of
    // the console, or its directory of custom decals for the car.
    ControllerPak DecalPak(const DecalCar &car) const;
    // Read the decal note of a car from its pak. The result is empty when the note is missing or
    // not valid for the car.
    std::optional<std::vector<u8>> ReadDecalNote(const DecalCar &car) const;
    // Measure from the first corner of a shape to the cursor, in pixels of the 640 by 480 screen,
    // rounded down.
    s32 AnchorDistance() const;
    // Find the second corner of a shape on the 640 by 480 screen. While a shape is constrained,
    // other than a circle, the cursor is snapped by ConstrainPaintEndpoint().
    std::array<s32, 2> PaintEndpoint() const;
    // Run the colour editor of the chosen swatch. The stick, the directions, the C buttons, and the
    // mouse set the levels. The first two colours follow each other at double and half. A applies
    // the colour as one edit, and B restores it.
    void ProcessColorEditorDialog();
    // Move the cursor with the stick, at half speed over the decal, or a pixel with the directions
    // and the C buttons, within the bounds of the screen.
    void UpdateCursorFromInput();
    // Select the decal slot under the cursor when it is clicked, and return whether a slot was
    // clicked.
    bool PickDecalSlot();
    // Choose the swatch under the cursor when it is clicked, and open the colour editor on a second
    // click of the same swatch within 350 milliseconds. Returns whether a swatch was clicked.
    bool PickPaletteColor();
    // Choose the tool or the brush size under the cursor when it is clicked, and open the style
    // menu on a second click of a tool with styles within 350 milliseconds. The undo tool undoes at
    // once. Returns whether a button was clicked.
    bool PickPaintTool();
    // Run the text tool. A click on the decal opens the keyboard, the text typed then follows the
    // cursor, and a second click paints it.
    void ProcessTextTool();
    // Paint a dot of the brush at the cursor, round or square by the style. The first dot of a
    // stroke records the decal for undo.
    void PaintBrush();
    // Take the first corner of a shape at the first click, or record the decal for undo at the
    // second. Returns whether the shape is to be painted now.
    bool StartShape();
    // Paint the rectangle from the first corner to PaintEndpoint() in the style of the tool.
    void PaintRectangle();
    // Paint the ellipse or the circle of the oval tool. A circle is centred on the first corner,
    // with its radius in texels from AnchorDistance().
    void PaintOval();
    // Paint with the chosen tool where the cursor clicks the decal. Returns whether the click was
    // taken. The brush, the fill, and the second click of a shape paint.
    bool PaintWithTool();
    // Draw the dashed outline of a started shape to the cursor, or end the stroke of the brush,
    // while A is released over the decal.
    void DrawShapeOutline();
    // Run the input of the editor for a frame. The first click goes to a decal slot, a swatch, or
    // a tool, then to the text tool or the other tools on the decal.
    void ProcessDecalEditorInput();
    // Find whether the cursor is over a picture at its origin and draw size, edges included.
    bool IsCursorInButton(const PaintSprite &sprite) const;
    // Find whether the cursor is in a rectangle, edges included.
    bool IsCursorInRect(const PaintRect &rect) const;
    // Find whether the cursor is on a key of the keyboard, at the size of the highlight.
    bool IsCursorOnKeyboardKey(const PaintGlyph &key) const;
    // Read the 16 colours of the edited palette into the swatches, as 8 bits a channel.
    void ConvertPaletteToRgb();
    // Draw the palette frames, the swatches, and the highlight of the chosen swatch.
    void DrawDecalColorPalette();
    // Move the cursor of a menu of four styles with left and right, wrapping at the ends.
    void MoveStyleMenuCursor();
    // Close a style menu with A, setting the style of the tool to the cursor, or with B.
    void CloseStyleMenu(PaintSprite &tool);
    // Run the style menu of the brush, round or square.
    void ProcessBrushStyleMenu();
    // Run the style menu of the text tool, normal or with a drop shadow, and its shadow colour.
    void ProcessTextStyleMenu();
    // Run the style menu of the rectangle tool, of four styles.
    void ProcessRectStyleMenu();
    // Run the style menu of the oval tool, of four styles.
    void ProcessOvalStyleMenu();
    // Fade out of an unchanged paint shop at once, or confirm leaving it without saving the
    // changes.
    void ProcessPaintShopExitConfirm();
    // Set entry 0 to 15 of the edited palette, retaining the alpha bit of the entry, and mark the
    // preview for reloading. Each channel is reduced to 5 bits.
    void SetPalette(int entry, u8 red, u8 green, u8 blue);
    // Copy the first two colours of the edited palette to the headlights and the first to the rear
    // lights, as the console does. Nothing is copied for a car of numbered panels.
    void CopyLightPalettes();
    // Move the cursor of a menu to the option the mouse moved over this frame, the last option
    // under the mouse.
    void PickMenuOption(const PaintRect *options, int count, u8 &cursor);

    // decaldraw.c of the console. Texel rows count from the bottom of the decal, and screen
    // places are on the 640 by 480 screen.

    // Read a texel of the edited decal, a palette index from 0 to 15.
    u8 GetPixel4Bit(s32 x, s32 y) const;
    // Read a texel of 4-bit texels stored at the width of the edited decal. A texel past the
    // decal reads the bytes after it, as on the console, within the texels of the model, and 0
    // outside them.
    u8 ReadPixel4Bit(const u8 *texels, s32 x, s32 y) const;
    // Flood fill the region of one colour under a point of the screen with the chosen colour. The
    // fill runs on a copy without the mask and is copied back through it. Runs past the room of
    // the stack of the console are dropped.
    void FloodFill4Bit(s32 x, s32 y);
    // Paint the typed text onto the edited decal in the chosen colour, with the shadow colour a
    // texel to the right and below first when the text tool has its drop shadow. The x is the
    // centre and y the bottom of the text.
    void StampTextOnDecal(s32 x, s32 y);
    // Paint a line in the chosen colour with the brush, a texel at a time or a round dot of the
    // brush size. As on the console, an exact diagonal drawn downwards paints nothing.
    void DrawLine4Bit(s32 x0, s32 y0, s32 x1, s32 y1);
    // Write a texel of the edited decal and mark the preview for reloading. Texels outside the
    // decal and texels set in the mask are not written. As on the console, the colour is not
    // masked to 4 bits for an odd column.
    void SetPixel4Bit(s32 x, s32 y, u8 color);
    // Paint a rectangle of texels in the chosen colour, with x1 and y1 after the last column and
    // row.
    void FillTexels(s32 x0, s32 y0, s32 x1, s32 y1);
    // Paint a filled rectangle between two corners of the screen in the chosen colour.
    void FillRect4Bit(s32 x0, s32 y0, s32 x1, s32 y1);
    // Paint the edges of a rectangle between two corners of the screen, as thick as the brush.
    void StrokeDecalRectangle(s32 x0, s32 y0, s32 x1, s32 y1);
    // Paint a filled rectangle with rounded corners between two corners of the screen. The radius
    // is a quarter of the shorter side.
    void FillDecalRoundRect(s32 x0, s32 y0, s32 x1, s32 y1);
    // Paint the edges of a rectangle with rounded corners between two corners of the screen, as
    // thick as the brush.
    void DrawDecalRoundedRectOutline(s32 x0, s32 y0, s32 x1, s32 y1);
    // Paint a filled ellipse in the rectangle between two corners of the screen.
    void DrawDecalFilledEllipse(s32 x0, s32 y0, s32 x1, s32 y1);
    // Paint the outline of an ellipse in the rectangle between two corners of the screen, as
    // thick as the brush, inward from the edge.
    void DrawDecalEllipseOutline(s32 x0, s32 y0, s32 x1, s32 y1);
    // Paint a filled circle in the chosen colour, with the radius in texels. Nothing is painted
    // for a negative radius. With fromScreen, the centre is on the screen rather than in texels.
    void FillCircle4Bit(s32 x, s32 y, s32 radius, bool fromScreen);
    // Paint the outline of a circle, as thick as the brush, inward from the edge. The radius is in
    // texels, to the middle of the outline.
    void DrawDecalCircleOutline(s32 x, s32 y, s32 radius);
    // Advance the count of console frames, and swap the colours of the dashes every eighth.
    void AdvanceDashes();
    // Draw the dashed preview of a line on the screen, on its odd pixels.
    void DrawDashedLine(s32 x0, s32 y0, s32 x1, s32 y1);
    // Draw the dashed preview of a rectangle on the screen, in dashes of 4 pixels.
    void DrawDashedRectangle(s32 x0, s32 y0, s32 x1, s32 y1);
    // Draw the dashed preview of a rectangle with rounded corners on the screen.
    void DrawDashedRoundedRect(s32 x0, s32 y0, s32 x1, s32 y1);
    // Draw the dashed preview of an ellipse on the screen.
    void DrawDashedEllipse(s32 x0, s32 y0, s32 x1, s32 y1);
    // Draw the dashed preview of a circle on the screen, over the decal only. The radius is in
    // pixels of the screen.
    void DrawDashedCircle(s32 x, s32 y, s32 radius);
    // Draw the typed text over the decal at the zoom of the canvas, with its drop shadow when the
    // text tool has one, where it will be painted. The x is the centre and y the bottom of the
    // text.
    void DrawDecalTextPreview(s32 x, s32 y);
    // Convert a column of the screen to a texel column of the edited decal, rounded down.
    s32 ScreenToTexelX(s32 x) const;
    // Convert a row of the screen to a texel row of the edited decal, rounded down.
    s32 ScreenToTexelY(s32 y) const;
    // Read a texel of the font of the decals, without the swap of odd rows. The column runs across
    // every cell of the font. The level is from 0 to 15, with 15 for ink, or 0 outside the font.
    u8 FontTexel(s32 column, s32 row) const;
    // Measure the advance in texels from a glyph to the next, with their kerning. Both are keys
    // in m_decalGlyphs.
    s32 GlyphAdvance(int glyph, int next) const;

    MenuScreen &m_screen;                            // Shares its fades and fonts.
    std::optional<std::array<int, 6>> &m_textColors; // The custom text colours of the menus.
    ControllerPak m_pak;
    std::filesystem::path m_release;      // The cartridge directory of the import.
    std::filesystem::path m_carDirectory; // Set by SetCar().

    // The drawing of the frame.
    SceneRenderer *m_renderer = nullptr; // Set by BeginDraw().
    Matrix4 m_flat{};                    // From the 640 by 480 screen to the frame.
    std::vector<FlatVertex> m_fills;     // Queued for FlushFills().
    std::vector<std::unique_ptr<RenderTexture>> m_frameTextures;
    // The decal textures drawn the frame before, released at the next BeginDraw().
    std::vector<std::unique_ptr<RenderTexture>> m_lastFrameTextures;
    TextStyle m_text; // The style DrawText() draws with, set before each message.

    // The input of the player, and the time of the frame.
    PlayerInputState m_input{};  // The input of the frame as the console reads it.
    std::string m_typedText;     // The characters typed on the computer this frame, in UTF-8.
    s32 m_frameMilliseconds = 0; // Read by the stick repeat.
    // The counts of frames of the console advance once a 60th of a second at any frame rate: the
    // time toward the next count, and whether one is due this frame.
    float m_consoleFrameTime = 0.0f; // In seconds.
    bool m_consoleFrame = true;
    // Where the mouse is on the 640 by 480 screen, and whether it moved this frame.
    std::array<int, 2> m_pointer{};
    bool m_pointerMoved = false;
    bool m_pointerSeen = false; // Whether the mouse has moved since the paint shop loaded.
    bool m_escapeDown = false;  // Whether Escape was down the frame before.

    // The pictures, by their address in the main segment of the console.
    std::map<u32, PaintSprite> m_sprites;
    PaintSprite *m_background = nullptr;
    PaintSprite *m_logo = nullptr;
    PaintSprite *m_paintWholeCar = nullptr; // The car of two clear options.
    PaintSprite *m_panel8Overlay = nullptr;
    PaintSprite *m_panel9Overlay = nullptr;
    PaintSprite *m_brushSizeRound = nullptr;
    PaintSprite *m_brushSizeSquare = nullptr;
    PaintSprite *m_paintCursor = nullptr; // The origin of the picture is the place of the cursor.
    PaintSprite *m_fillCursor = nullptr;
    PaintSprite *m_brushCursor = nullptr;
    PaintSprite *m_crosshairCursor = nullptr; // The cursor of the shape tools.
    PaintSprite *m_textCursor = nullptr;
    PaintSprite *m_eyedropperCursor = nullptr; // The cursor while B picks a colour.
    PaintSprite *m_keyboardPanel = nullptr;
    PaintSprite *m_keyboardBackground = nullptr; // The face of the keys inside the panel.
    PaintSprite *m_keyboardHighlight = nullptr;  // The highlight of the key under the cursor.
    PaintSprite *m_buttonA = nullptr;
    PaintSprite *m_buttonB = nullptr;
    PaintSprite *m_decalFont = nullptr;         // 16 texels a cell.
    PaintSprite *m_shapeFilledCircle = nullptr; // The round choice of the brush style menu.
    std::array<PaintSprite *, DECAL_SLOTS> m_decalSlotIcons{};
    // The pictures of the tool buttons by PaintTool. Entry 0 is not drawn.
    std::array<PaintSprite *, 13> m_paintToolIcons{};
    std::array<PaintSprite *, 4> m_rectStyleIcons{};
    std::array<PaintSprite *, 4> m_ovalStyleIcons{};

    // The tables of the console.
    PaintRect m_decalCanvasFrame;  // The black frame the decal is centred in.
    PaintRect m_paletteFrameLeft;  // The frame of the first two swatches.
    PaintRect m_paletteFrameRight; // The frame of the other 14 swatches.
    // The sizes of the brush by m_brushSize, half the width being the radius in texels.
    std::array<PaintRect, 5> m_brushSizeRects{};
    std::array<PaintRect, 5> m_brushSizeButtonRects{};
    // The keys of the text keyboard, ten a row, with Backspace at 47 and Enter at 49.
    std::array<PaintGlyph, 50> m_decalGlyphs{};
    // The red, green, and blue of the bar of each channel of the colour editor.
    std::array<std::array<u8, 3>, 3> m_colorEditorBars{};
    std::array<std::string, 3> m_colorChannelNames;
    // The labels of the styles are empty in the Japanese release.
    std::array<std::string, 2> m_brushStyleNames;
    std::array<std::string, 2> m_textStyleNames;
    std::array<std::string, 4> m_rectStyleNames;
    std::array<std::string, 4> m_ovalStyleNames;
    // The direction the preview views each decal slot from.
    std::array<PaintVector, DECAL_SLOTS> m_panelViewDirections{};

    // The state of the paint shop, as the globals of the console.
    std::optional<DecalCar> m_car; // Loaded by InitPaintShop().
    int m_carModel = 0;            // Set by SetCar(), an index of the cars of the menus.
    bool m_initialized = false;    // Whether InitPaintShop() opened the paint shop.
    bool m_previewDirty = true;    // Whether the car changed since the preview reloaded.
    int m_previewWait = 0;         // The console frames before the preview may reload again.
    std::unique_ptr<RenderModel> m_preview;
    std::unique_ptr<RenderTexture> m_previewReflection;
    // The axes and the place of the car in the preview.
    PaintVector m_frameForward;
    PaintVector m_frameUp;
    PaintVector m_frameSide;
    PaintVector m_framePosition;
    PaintVector m_cameraPosition;
    std::array<PaintSwatch, 16> m_swatches{};
    // The tool buttons by PaintTool, with the brush-size panel at 0.
    std::array<PaintRect, 13> m_paintToolButtonRects{};
    std::array<PaintRect, DECAL_SLOTS> m_decalSlotRects{}; // Where the decal slots are clicked.
    PaintRect m_decalCanvas;                               // The decal at its zoom on the screen.
    PaintRect m_brush;                                     // Never read or written.
    // The key of each typed character in m_decalGlyphs, -1 after the last.
    std::array<int, 16> m_decalTextGlyphs{};
    std::string m_decalText;
    // The texels of the edited decal before the last edit, and the staging of a pak load.
    std::vector<u8> m_undoBuffer;
    // The editable data of the car before each edit, the last edit at the back.
    std::vector<std::vector<u8>> m_undoHistory;
    // The editable data of the car before each undo, the last undo at the back.
    std::vector<std::vector<u8>> m_redoHistory;
    // The editable data of the car as the colour editor opened, or empty while it is closed.
    std::optional<std::vector<u8>> m_colorEditorSnapshot;
    // The editable data of the car as it opened or as it was last saved or loaded.
    std::vector<u8> m_savedEditableData;
    std::vector<u8> m_scratchTexels;      // The copy of the decal the flood fill runs on.
    u8 *m_decalTexels = nullptr;          // Points into the car.
    const u8 *m_decalTexelsEnd = nullptr; // The end of the texels of the car.
    const u8 *m_decalMask = nullptr;      // nullptr for none.
    int m_decalPaletteMaterial = 0;       // The material whose palette the swatches edit.
    u8 m_decalWidth = 0;                  // In texels.
    int m_decalZoom = 4;                  // The screen pixels of a texel on the canvas.
    u8 m_decalHeight = 0;                 // In texels.
    u8 m_dashColorA = 0;                  // The grey of half of the dashes, swapped with B.
    u8 m_dashColorB = 0;
    u8 m_brushSize = 0; // From 0 to 4.
    // The colour chosen before the last click of the palette, or the shadow colour as the text
    // style menu opened.
    u8 m_savedColor = 0;
    u8 m_decalColor = 0; // The chosen palette index.
    u8 m_textShadowColor = 0;
    u8 m_paintTool = 0;         // The chosen PaintTool.
    u8 m_previousPaintTool = 0; // The tool a menu or a dialog returns to.
    u8 m_decalSlot = 0;
    u8 m_previousDecalSlot = 0; // The slot before the last change, for the preview turn.
    u8 m_savedDecalSlot = 0;    // The slot restored after the clear of every decal.
    u8 m_decalTextLength = 0;
    u8 m_decalTextWidth = 0;  // In texels.
    u8 m_styleMenuCursor = 0; // Also used by the mirror menu.
    // Set while a brush stroke, the first corner of a shape, or placed text waits for the next
    // click, and as a menu opens for it to read its option.
    u8 m_paintToolStarted = 0;
    // Set from a click of the text tool on the decal until the text is painted or cancelled.
    u8 m_decalDragActive = 0;
    u8 m_paletteEnabled = 0;     // Whether a click picks a swatch, cleared while typing.
    u8 m_decalSlotHasMirror = 0; // Whether the edited slot has an opposite panel.
    u8 m_decalSlotChanged = 0;   // Set while the preview turns to a newly selected slot.
    u8 m_colorEditorOpen = 0;
    u8 m_styleMenuOpen = 0;
    u8 m_textShadowPickerOpen = 0; // Whether the shadow colours of the text menu show.
    u8 m_paintShopExiting = 0;     // Set while the paint shop fades out to car select.
    u8 m_colorEditorChannel = 0;   // From 0 to 2.
    u8 m_keyboardHoverKey = 0;     // The key last under the cursor on the keyboard.
    u8 m_showPreview3D = 0;        // Cleared while typing.
    u8 m_eyedropperActive = 0;     // Whether B is picking the colour under the cursor.
    u8 m_mirrorTargetPanel = 0;    // The slot the last mirror copied to.
    // The colour of the swatch as the colour editor opened, restored by B.
    std::array<u8, 3> m_savedSwatchColor{};
    s32 m_paintAnchorX = 0; // The first corner of a shape, or the place of the text.
    s32 m_paintAnchorY = 0;
    bool m_constrainShape = false;     // Whether Shift or Z constrains the shape this frame.
    u32 m_lastClickTime = 0;           // The SDL ticks of the last click, in milliseconds.
    bool m_anchorXInitialized = false; // Whether the editor set m_paintAnchorX at its start.
    bool m_anchorYInitialized = false;
    // The console frames of the turn of the preview, the flash of a key, or the dashes.
    u16 m_screenFrameCount = 0;
    // The error of the load or save dialog of the paint shop, shown until dismissed, or 0.
    s32 m_controllerPakState = 0;

    // The dialog of the Controller Pak.
    u8 m_pakDialogActive = 0; // Whether the dialog has started and not finished.
    u8 m_pakFileNeedsCreate = 0;
    u8 m_pakDialogState = 0; // The step of the dialog, as the console numbers it.
    u8 m_pakDecalIndex = 0;  // The decal slot transferred next.
    u8 m_pakWaitFrames = 0;  // The console frames waited in the current step.
    s32 m_pakFileOffset = 0; // The offset in m_pakNote of the next transfer.
    s32 m_pakError = 0;
    s32 m_pakChunkSize = 0;    // The bytes of the last decal transferred.
    std::vector<u8> m_pakNote; // The bytes of the decal note read or to be written.
    bool m_pakDone = false;    // Whether the dialog of car select has finished.
    s32 m_pakResult = 0;       // The result of the dialog of car select, or 0.
};

} // namespace coastline
