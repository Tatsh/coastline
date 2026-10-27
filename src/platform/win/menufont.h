#pragma once

#include <windows.h>

struct MenuFont;

struct MenuFontVTable {
    char abUnknown0[4]; /* +0x00 */
    void(__thiscall *MeasureText)(MenuFont *);
    void(__thiscall *MeasureTextAltFont)(MenuFont *);
    char abUnknownC[0x1c]; /* +0x0c */
    long double(__thiscall *ComputeCenteredX)(MenuFont *);
};

struct MenuFont {
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
