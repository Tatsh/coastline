#pragma once

/**
 * @file
 * The text of the PC menus, drawn with the glyphs of the menu fonts, and its entry from the
 * keyboard.
 */

#include <cstdlib>
#include <cstring>

#include <windows.h>

#include "common/fontdata.h"

#undef DrawText

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Truncate a floating-point coordinate using the native signed 64-bit conversion range.
 * @param value The coordinate in menu pixels.
 * @return The converted integer's low 32 bits, or zero for NaN or an out-of-range 64-bit
 * conversion.
 */
static inline int ConvertMenuCoordinate(float value) {
    if (!(static_cast<long double>(value) >= -0x1p63L &&
          static_cast<long double>(value) < 0x1p63L)) {
        return 0;
    }
    return static_cast<int>(static_cast<long long>(value));
}
#ifdef __cplusplus
}
#endif

class MenuFont;

/**
 * Menu text with a 1024-byte internal buffer, glyph metrics, drawing style, and text-entry state.
 * Direct setters do not validate values or synchronise position, bounds, and measured dimensions.
 * Callers must preserve string termination and configure bounds before alignment or bounds queries.
 */
class MenuFont {
public:
    /**
     * Initialise empty text, style one, zero dimensions and position, and an inactive caret.
     * Bounds and application item data require assignment before queries.
     * @ghidraAddress BRD3D: 0x1005b050
     * @ghidraAddress BRGlide: 0x10053e70
     */
    MenuFont();

    /**
     * Destroy embedded text and state without releasing external resources.
     * @ghidraAddress BRD3D: 0x1005b0c0
     * @ghidraAddress BRGlide: 0x10053ee0
     */
    virtual ~MenuFont();

    /**
     * Recalculate primary-font width and raise height to the tallest recognised glyph if needed.
     * Existing height is not reset; invalid character bytes end measurement.
     * @ghidraAddress BRD3D: 0x1005b0d0
     * @ghidraAddress BRGlide: 0x10053ef0
     */
    virtual void MeasureText();
    /**
     * Recalculate alternate-font width and raise height without resetting prior height.
     * Native measurement subtracts four pixels per glyph; alternate drawing subtracts six.
     * @ghidraAddress BRD3D: 0x1005b160
     * @ghidraAddress BRGlide: 0x10053f80
     */
    virtual void MeasureTextAltFont();
    /**
     * Draw primary-font text at supplied coordinates, with optional centring and caret rendering.
     * @param x The horizontal menu-pixel coordinate, replaced by centring when flag one is set.
     * @param y The vertical menu-pixel coordinate.
     * @ghidraAddress BRD3D: 0x1005b390
     * @ghidraAddress BRGlide: 0x100541b0
     */
    virtual void DrawMenuItemText(float x, float y);
    /**
     * Draw primary-font text at the stored position, applying centring and an enabled caret.
     * @ghidraAddress BRD3D: 0x1005b2b0
     * @ghidraAddress BRGlide: 0x100540d0
     */
    virtual void DrawText();
    /**
     * Process pending character and key input, including network-setup sound shortcuts.
     * The text buffer must be terminated and have capacity for any accepted character.
     * @return -1 for cancellation, zero for accepted nonempty text, or one to continue editing.
     * @ghidraAddress BRD3D: 0x1005b570
     * @ghidraAddress BRGlide: 0x10054390
     */
    virtual signed char HandleTextInput();

    /**
     * Centre measured text between configured left and right bounds and store the horizontal
     * position.
     * @return The computed horizontal menu-pixel coordinate before storage rounding to float.
     * @ghidraAddress BRD3D: 0x1005b200
     * @ghidraAddress BRGlide: 0x10054020
     */
    virtual long double ComputeCenteredX();
    /**
     * Draw alternate-font text at the stored position, applying centring and an enabled caret.
     * @ghidraAddress BRD3D: 0x1005b460
     * @ghidraAddress BRGlide: 0x10054280
     */
    virtual void DrawTextAltFont();

    /**
     * Get the text of the menu item.
     * @return Borrowed internal text, valid until text mutation or object destruction.
     */
    const char *GetText() const {
        return szText;
    }
    /**
     * Copy terminated text without checking capacity or measuring the result.
     * @param text A non-null string of at most 1023 bytes, copied during the call.
     */
    void SetText(const char *text) {
        std::strcpy(szText, text);
    }
    /**
     * Append terminated text without checking capacity or measuring the result.
     * @param text A non-null string whose appended result, including its terminator, fits
     * 1024 bytes.
     */
    void AppendText(const char *text) {
        std::strcat(szText, text);
    }
    /**
     * Copy a text prefix with strncpy() semantics, including zero padding after a source
     * terminator.
     * A source at least size bytes long does not produce a terminator; callers must terminate it.
     * @param text A non-null source with readable bytes through its terminator or size bytes.
     * @param size The destination byte count, at most 1024.
     */
    void CopyTextPrefix(const char *text, size_t size) {
        std::strncpy(szText, text, size);
    }
    /**
     * Replace text with a terminated signed decimal integer without measuring the result.
     * @param value The integer to format.
     */
    void SetNumber(int value) {
        _itoa(value, szText, 10);
    }
    /**
     * Get the flags of the text.
     * @return The stored text flag bits, including bit zero for centring.
     */
    unsigned GetFlags() const {
        return dwFlags;
    }
    /**
     * Store text flags without measuring or drawing.
     * @param value The flag bits, including bit zero for centring.
     * @return The stored flag bits.
     */
    unsigned SetFlags(unsigned value) {
        return dwFlags = value;
    }
    /**
     * Store a drawing style without changing font measurements.
     * @param value The atlas style, normally zero, one, two, three, or four.
     * @return The stored style.
     */
    unsigned char SetStyle(unsigned char value) {
        return bStyle = value;
    }
    /**
     * Get the style DrawGlyph() selects the atlas image of the primary font with.
     * @return The stored drawing style.
     */
    unsigned char GetStyle() const {
        return bStyle;
    }
    /**
     * Get the width the last measurement or SetWidth() stored.
     * @return The stored text width in menu pixels.
     */
    short GetWidth() const {
        return nWidth;
    }
    /**
     * Store width without measuring text or updating bounds.
     * @param value The width in menu pixels.
     * @return The stored width.
     */
    short SetWidth(short value) {
        return nWidth = value;
    }
    /**
     * Get the height the last measurement or SetHeight() stored.
     * @return The stored text height in menu pixels.
     */
    short GetHeight() const {
        return nHeight;
    }
    /**
     * Store height without measuring text or updating bounds.
     * @param value The height in menu pixels; zero permits later measurement to shrink
     * prior height.
     * @return The stored height.
     */
    short SetHeight(short value) {
        return nHeight = value;
    }
    /**
     * Store horizontal position without updating bounds.
     * @param value The horizontal menu-pixel coordinate.
     * @return The stored coordinate.
     */
    float SetX(float value) {
        return fX = value;
    }
    /**
     * Get the horizontal position DrawText() and DrawTextAltFont() draw at.
     * @return The stored horizontal position in menu pixels.
     */
    float GetX() const {
        return fX;
    }
    /**
     * Get the vertical position DrawText() and DrawTextAltFont() draw at.
     * @return The stored vertical position in menu pixels.
     */
    float GetY() const {
        return fY;
    }
    /**
     * Store vertical position without updating bounds.
     * @param value The vertical menu-pixel coordinate.
     * @return The stored coordinate.
     */
    float SetY(float value) {
        return fY = value;
    }
    /**
     * Store the legacy depth field without changing device state; text drawing does not read it.
     * @param value The floating-point depth value, stored without validation.
     * @return The stored depth.
     */
    float SetZ(float value) {
        return fZ = value;
    }
    /**
     * Store the width threshold used before accepting an input character.
     * @param value The threshold in menu pixels; the next character can extend text past
     * the threshold.
     * @return The stored threshold.
     */
    short SetInputMaxWidth(short value) {
        return nInputMaxWidth = value;
    }
    /**
     * Get whether the text draws a blinking caret after its last glyph.
     * @return The stored caret marker, with nonzero enabling caret drawing.
     */
    int GetDrawCaret() const {
        return bDrawCaret;
    }
    /**
     * Store caret state without resetting its shared blink timer.
     * @param value The marker, with nonzero enabling caret drawing.
     * @return The stored marker.
     */
    int SetDrawCaret(int value) {
        return bDrawCaret = value;
    }
    /**
     * Get the left bound ComputeCenteredX() centres from.
     * @return The stored left menu-pixel bound; bounds must be initialised first.
     */
    LONG GetBoundsLeft() const {
        return rcBounds.left;
    }
    /**
     * Get the bounds of the text.
     * @return A borrowed reference to bounds storage, valid until destruction and changed by
     * setters.
     */
    const RECT &GetBounds() const {
        return rcBounds;
    }
    /**
     * Store the left bound without updating position or dimensions.
     * @param value The left menu-pixel coordinate.
     * @return The stored coordinate.
     */
    LONG SetBoundsLeft(LONG value) {
        return rcBounds.left = value;
    }
    /**
     * Get the top bound of the text.
     * @return The stored top menu-pixel bound; bounds must be initialised first.
     */
    LONG GetBoundsTop() const {
        return rcBounds.top;
    }
    /**
     * Store the top bound without updating position or dimensions.
     * @param value The top menu-pixel coordinate.
     * @return The stored coordinate.
     */
    LONG SetBoundsTop(LONG value) {
        return rcBounds.top = value;
    }
    /**
     * Get the right bound ComputeCenteredX() centres to.
     * @return The stored right menu-pixel bound; bounds must be initialised first.
     */
    LONG GetBoundsRight() const {
        return rcBounds.right;
    }
    /**
     * Store the right bound without updating position or dimensions.
     * @param value The right menu-pixel coordinate.
     * @return The stored coordinate.
     */
    LONG SetBoundsRight(LONG value) {
        return rcBounds.right = value;
    }
    /**
     * Store the bottom bound without updating position or dimensions.
     * @param value The bottom menu-pixel coordinate.
     * @return The stored coordinate.
     */
    LONG SetBoundsBottom(LONG value) {
        return rcBounds.bottom = value;
    }
    /**
     * Get the value a caller attached with SetItemData().
     * @return The stored application item data, initialised by callers before querying.
     */
    unsigned GetItemData() const {
        return dwItemData;
    }
    /**
     * Store application-specific item data without interpreting it.
     * @param value The unsigned application value.
     * @return The stored value.
     */
    unsigned SetItemData(unsigned value) {
        return dwItemData = value;
    }

private:
    // Copy the glyph rectangle nImage of g_aMenuGlyphRects in the primary font to the menu
    // surface, with the coordinates truncated to integers. bStyle selects the atlas image, 2 for
    // style 0, 3 for style 1, 4 for style 2, 52 for style 4, and 0 for other styles. nStyle is not
    // read, and the routine always returns 1.
    // @ghidraAddress BRD3D: 0x1005b730
    // @ghidraAddress BRGlide: 0x10054550
    virtual int DrawGlyph(short nImage, float x, float y, int nStyle);
    // Draw nothing and return 0. The routine occupies a slot of the virtual table that no caller
    // uses, and its parameters are not read.
    // @ghidraAddress BRD3D: 0x1005b7e0
    // @ghidraAddress BRGlide: 0x10054600
    virtual int DrawGlyphNoOp(short nImage, float x, float y);
    // Copy the glyph rectangle nImage of g_aMenuAltGlyphRects in the alternate font from atlas
    // image 5 to the menu surface, with the coordinates truncated to integers. nStyle is not read,
    // and the routine always returns 1.
    // @ghidraAddress BRD3D: 0x1005b7a0
    // @ghidraAddress BRGlide: 0x100545c0
    virtual int DrawGlyphAltFont(short nImage, float x, float y, int nStyle);
    // Draw the underscore glyph of the primary font as the caret once 120 milliseconds have
    // accumulated on a timer shared by every MenuFont, then restart the timer.
    // @ghidraAddress BRD3D: 0x1005b250
    // @ghidraAddress BRGlide: 0x10054070
    virtual void DrawThrottledMenuGlyph(float x, float y);

    // Measure the text with the glyph table of either font, for MeasureText() and
    // MeasureTextAltFont(). The alternate font is four pixels narrower for each glyph.
    inline void MeasureMenuText(bool alternate);
    // Draw the text glyph by glyph, for DrawText(), DrawMenuItemText(), and DrawTextAltFont().
    // Flag bit zero replaces x with ComputeCenteredX(), and an enabled caret follows the text. The
    // alternate font is six pixels narrower for each glyph, and useFontY draws at fY in place of y.
    inline void DrawMenuText(float x, float y, bool alternate, bool useFontY);

    unsigned dwFlags;     // The flags of the text, bit zero centring it between the bounds.
    unsigned char bStyle; // The style that selects the atlas image of DrawGlyph(), 1 at first.
    char szText[1024];    // The text, terminated by a zero byte.
    short nWidth;         // The width of the text in menu pixels.
    short nHeight;        // The height of the tallest glyph in menu pixels.
    float fX;             // The horizontal position in menu pixels.
    float fY;             // The vertical position in menu pixels.
    float fZ;             // Written by SetZ() and the constructor, and not read.
    short nInputMaxWidth; // The width past which HandleTextInput() does not accept characters.
    int bDrawCaret;       // Whether a blinking caret follows the text.
    RECT rcBounds;        // The rectangle ComputeCenteredX() centres the text in.
    unsigned dwItemData;  // A value of the caller, not read by MenuFont.
};
