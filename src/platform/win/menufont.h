#pragma once

#include <cstdlib>
#include <cstring>

#include <windows.h>

static inline int ConvertMenuCoordinate(float value) {
    if (!(static_cast<long double>(value) >= -0x1p63L &&
          static_cast<long double>(value) < 0x1p63L)) {
        return 0;
    }
    return static_cast<int>(static_cast<long long>(value));
}

class MenuFont;

struct MenuGlyph {
    short nWidth;
    short nHeight;
    short nImage;
    char abUnknown6[6]; /* +0x06 */
};

/**
 * @ghidraAddress BRD3D: 0x100ac6e4
 */
extern MenuGlyph g_MenuFontGlyphTable[95];
/**
 * @ghidraAddress BRD3D: 0x100acb5c
 */
extern MenuGlyph g_AltFontGlyphTable[95];

class MenuFont {
public:
    /**
     * @ghidraAddress BRD3D: 0x1005b050
     */
    MenuFont();

    /**
     * @ghidraAddress BRD3D: 0x1005b0c0
     */
    virtual ~MenuFont();

    /**
     * @ghidraAddress BRD3D: 0x1005b0d0
     */
    virtual void MeasureText();
    /**
     * @ghidraAddress BRD3D: 0x1005b160
     */
    virtual void MeasureTextAltFont();
    /**
     * @ghidraAddress BRD3D: 0x1005b390
     */
    virtual void DrawMenuItemText(float x, float y);
    /**
     * @ghidraAddress BRD3D: 0x1005b2b0
     */
    virtual void DrawText();
    /**
     * @ghidraAddress BRD3D: 0x1005b570
     */
    virtual signed char HandleTextInput();

    /**
     * @ghidraAddress BRD3D: 0x1005b200
     */
    virtual long double ComputeCenteredX();
    /**
     * @ghidraAddress BRD3D: 0x1005b460
     */
    virtual void DrawTextAltFont();

    const char *GetText() const {
        return szText;
    }
    void SetText(const char *text) {
        std::strcpy(szText, text);
    }
    void AppendText(const char *text) {
        std::strcat(szText, text);
    }
    void CopyTextPrefix(const char *text, size_t size) {
        std::strncpy(szText, text, size);
    }
    void SetNumber(int value) {
        _itoa(value, szText, 10);
    }
    unsigned GetFlags() const {
        return dwFlags;
    }
    unsigned SetFlags(unsigned value) {
        return dwFlags = value;
    }
    unsigned char SetStyle(unsigned char value) {
        return bStyle = value;
    }
    unsigned char GetStyle() const {
        return bStyle;
    }
    short GetWidth() const {
        return nWidth;
    }
    short SetWidth(short value) {
        return nWidth = value;
    }
    short GetHeight() const {
        return nHeight;
    }
    short SetHeight(short value) {
        return nHeight = value;
    }
    float SetX(float value) {
        return fX = value;
    }
    float GetX() const {
        return fX;
    }
    float GetY() const {
        return fY;
    }
    float SetY(float value) {
        return fY = value;
    }
    float SetZ(float value) {
        return fZ = value;
    }
    short SetInputMaxWidth(short value) {
        return nInputMaxWidth = value;
    }
    int GetDrawCaret() const {
        return bDrawCaret;
    }
    int SetDrawCaret(int value) {
        return bDrawCaret = value;
    }
    LONG GetBoundsLeft() const {
        return rcBounds.left;
    }
    const RECT &GetBounds() const {
        return rcBounds;
    }
    LONG SetBoundsLeft(LONG value) {
        return rcBounds.left = value;
    }
    LONG GetBoundsTop() const {
        return rcBounds.top;
    }
    LONG SetBoundsTop(LONG value) {
        return rcBounds.top = value;
    }
    LONG GetBoundsRight() const {
        return rcBounds.right;
    }
    LONG SetBoundsRight(LONG value) {
        return rcBounds.right = value;
    }
    LONG SetBoundsBottom(LONG value) {
        return rcBounds.bottom = value;
    }
    unsigned GetItemData() const {
        return dwItemData;
    }
    unsigned SetItemData(unsigned value) {
        return dwItemData = value;
    }

private:
    /**
     * @ghidraAddress BRD3D: 0x1005b730
     */
    virtual int DrawGlyph(short nImage, float x, float y, int nStyle);
    /**
     * @ghidraAddress BRD3D: 0x1005b7e0
     */
    virtual int DrawGlyphNoOp(short nImage, float x, float y);
    /**
     * @ghidraAddress BRD3D: 0x1005b7a0
     */
    virtual int DrawGlyphAltFont(short nImage, float x, float y, int nStyle);
    /**
     * @ghidraAddress BRD3D: 0x1005b250
     */
    virtual void DrawThrottledMenuGlyph(float x, float y);

    void MeasureMenuText(bool alternate);
    void DrawMenuText(float x, float y, bool alternate, bool useFontY);

    unsigned dwFlags;
    unsigned char bStyle;
    char szText[1024];
    short nWidth;
    short nHeight;
    char abUnknown40e[2]; /* +0x40e */
    float fX;
    float fY;
    float fZ;
    short nInputMaxWidth;
    char abUnknown41e[2]; /* +0x41e */
    int bDrawCaret;
    RECT rcBounds;
    unsigned dwItemData;
};
