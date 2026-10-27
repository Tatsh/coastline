#include "platform/win/menufont.h"

static inline void MeasureMenuText(MenuFont *pFont, bool alternate) {
    unsigned short width = 0;
    short height = pFont->nHeight;
    short index = 0;
    for (;;) {
        signed char character = pFont->szText[index];
        if (!character) {
            break;
        }
        int glyphIndex = character - 0x20;
        if ((glyphIndex < 0 || glyphIndex > 0x7f) && character != ' ') {
            break;
        }
        if (character >= '!' && character <= '~' && g_MenuFontGlyphTable[glyphIndex].nWidth != -1 &&
            g_MenuFontGlyphTable[glyphIndex].nHeight != -1) {
            const MenuGlyph &glyph =
                alternate ? g_AltFontGlyphTable[glyphIndex] : g_MenuFontGlyphTable[glyphIndex];
            width = static_cast<unsigned short>(width + glyph.nWidth - (alternate ? 4 : 0));
            if (height < glyph.nHeight) {
                height = glyph.nHeight;
            }
        } else if (character == ' ') {
            width = static_cast<unsigned short>(width + 6);
        }
        index = static_cast<short>(static_cast<unsigned short>(index) + 1);
    }
    pFont->nHeight = height;
    pFont->nWidth = static_cast<short>(width);
}

void MenuFont::MeasureText() {
    MeasureMenuText(this, false);
}

void MenuFont::MeasureTextAltFont() {
    MeasureMenuText(this, true);
}

long double MenuFont::ComputeCenteredX() {
    int span = static_cast<int>(static_cast<unsigned>(rcBounds.right) -
                                static_cast<unsigned>(rcBounds.left));
    long double centered = (static_cast<long double>(span) - nWidth) * 0.5f + rcBounds.left;
    fX = centered;
    return centered;
}
