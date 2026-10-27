#pragma once

#include <windows.h>

struct MenuFont;

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

struct MenuFontVTable {
    char abUnknown0[4]; /* +0x00 */
    void(__thiscall *MeasureText)(MenuFont *);
    void(__thiscall *MeasureTextAltFont)(MenuFont *);
    char abUnknownC[0x1c]; /* +0x0c */
    long double(__thiscall *ComputeCenteredX)(MenuFont *);
};

/**
 * @ghidraAddress BRD3D: 0x1008f728
 */
extern MenuFontVTable g_MenuFontVTable;

struct MenuFont {
    /**
     * @ghidraAddress BRD3D: 0x1005b050
     */
    MenuFont();

    /**
     * @ghidraAddress BRD3D: 0x1005b0d0
     */
    void MeasureText();
    /**
     * @ghidraAddress BRD3D: 0x1005b160
     */
    void MeasureTextAltFont();
    /**
     * @ghidraAddress BRD3D: 0x1005b200
     */
    long double ComputeCenteredX();

    MenuFontVTable *pVTable;
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
    char abUnknown434[4]; /* +0x434 */
};
