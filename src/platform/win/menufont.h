#pragma once

#include <windows.h>

static inline int ConvertMenuCoordinate(float value) {
    if (!(static_cast<long double>(value) >= -0x1p63L &&
          static_cast<long double>(value) < 0x1p63L)) {
        return 0;
    }
    return static_cast<int>(static_cast<long long>(value));
}

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

/**
 * @ghidraAddress BRD3D: 0x10aa2a54
 */
extern int g_dwGlyphThrottleAccumMs;
/**
 * @ghidraAddress BRD3D: 0x10aa2a58
 */
extern unsigned g_dwGlyphThrottleLastTick;

struct MenuFontVTable {
    char abUnknown0[4]; /* +0x00 */
    void(__thiscall *MeasureText)(MenuFont *);
    void(__thiscall *MeasureTextAltFont)(MenuFont *);
    char abUnknownC[0xc]; /* +0x0c */
    int(__thiscall *DrawGlyph)(MenuFont *, short, float, float, int);
    char abUnknown1c[4]; /* +0x1c */
    int(__thiscall *DrawGlyphAltFont)(MenuFont *, short, float, float, int);
    void(__thiscall *DrawThrottledMenuGlyph)(MenuFont *, float, float);
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
    /**
     * @ghidraAddress BRD3D: 0x1005b730
     */
    int DrawGlyph(short nImage, float x, float y, int nStyle);
    /**
     * @ghidraAddress BRD3D: 0x1005b7a0
     */
    int DrawGlyphAltFont(short nImage, float x, float y, int nStyle);
    /**
     * @ghidraAddress BRD3D: 0x1005b250
     */
    void DrawThrottledMenuGlyph(float x, float y);

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
