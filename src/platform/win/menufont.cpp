#include "platform/win/menufont.h"

#include <cstring>

#include "platform/win/directplay.h"
#include "platform/win/frontend.h"
#include "platform/win/input.h"
#include "platform/win/sound.h"
#include "platform/win/timer.h"

int g_dwGlyphThrottleAccumMs;
unsigned g_dwGlyphThrottleLastTick;

MenuFont::~MenuFont() {
    pVTable = &g_MenuFontVTable;
}

MenuFont::MenuFont() {
    pVTable = &g_MenuFontVTable;
    std::memset(szText, 0, sizeof(szText));
    fZ = 0.0f;
    fY = 0.0f;
    fX = 0.0f;
    nHeight = 0;
    nWidth = 0;
    nInputMaxWidth = 0;
    bDrawCaret = 0;
    dwFlags = 0;
    bStyle = 1;
}

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

int MenuFont::DrawGlyph(short nImage, float x, float y, int nStyle) {
    unsigned short image = 0;
    switch (bStyle) {
    case 0:
        image = 2;
        break;
    case 1:
        image = 3;
        break;
    case 2:
        image = 4;
        break;
    case 4:
        image = 52;
        break;
    }
    DWORD flags = g_aMenuSprites[image].dwFlags;
    int destY = ConvertMenuCoordinate(y);
    int destX = ConvertMenuCoordinate(x);
    BlitSurfaceRegion(destX, destY, image, &g_aMenuGlyphRects[nImage], flags);
    return 1;
}

int MenuFont::DrawGlyphAltFont(short nImage, float x, float y, int nStyle) {
    DWORD flags = g_aMenuSprites[5].dwFlags;
    int destY = ConvertMenuCoordinate(y);
    int destX = ConvertMenuCoordinate(x);
    BlitSurfaceRegion(destX, destY, 5, &g_aMenuAltGlyphRects[nImage], flags);
    return 1;
}

void MenuFont::DrawThrottledMenuGlyph(float x, float y) {
    unsigned now = GetHighResolutionTime();
    unsigned delta = now - g_dwGlyphThrottleLastTick;
    g_dwGlyphThrottleLastTick = now;
    g_dwGlyphThrottleAccumMs =
        static_cast<int>(static_cast<unsigned>(g_dwGlyphThrottleAccumMs) + delta);
    if (g_dwGlyphThrottleAccumMs >= 120) {
        g_dwGlyphThrottleAccumMs = 0;
        pVTable->DrawGlyph(
            this, g_MenuFontGlyphTable['_' - ' '].nImage, x, y, static_cast<signed char>(bStyle));
    }
}

static inline void DrawMenuText(MenuFont *pFont, float x, float y, bool alternate, bool useFontY) {
    if (pFont->dwFlags & 1) {
        x = pFont->pVTable->ComputeCenteredX(pFont);
    }
    short index = 0;
    for (;;) {
        signed char character = pFont->szText[index];
        if (!character) {
            break;
        }
        int glyphIndex = character - ' ';
        if ((glyphIndex < 0 || glyphIndex > 0x7f) && character != ' ') {
            break;
        }
        if (character >= '!' && character <= (alternate ? 'z' : '~')) {
            const MenuGlyph &glyph =
                alternate ? g_AltFontGlyphTable[glyphIndex] : g_MenuFontGlyphTable[glyphIndex];
            if (glyph.nImage != -1) {
                auto draw =
                    alternate ? pFont->pVTable->DrawGlyphAltFont : pFont->pVTable->DrawGlyph;
                draw(pFont,
                     glyph.nImage,
                     x,
                     useFontY ? pFont->fY : y,
                     static_cast<signed char>(pFont->bStyle));
                int width = glyph.nWidth - (alternate ? 6 : 0);
                x = static_cast<long double>(x) + width;
            }
        } else if (character == ' ') {
            x = static_cast<long double>(x) + 6.0f;
        }
        index = static_cast<short>(static_cast<unsigned short>(index) + 1);
    }
    if (pFont->bDrawCaret) {
        pFont->pVTable->DrawThrottledMenuGlyph(pFont, x, useFontY ? pFont->fY : y);
    }
}

void MenuFont::DrawText() {
    DrawMenuText(this, fX, fY, false, true);
}

void MenuFont::DrawMenuItemText(float x, float y) {
    DrawMenuText(this, x, y, false, false);
}

void MenuFont::DrawTextAltFont() {
    DrawMenuText(this, fX, fY, true, true);
}

signed char MenuFont::HandleTextInput() {
    if (g_dwInNetworkCarSetup) {
        for (int i = 0; i < 4; i++) {
            if (g_adwKeyPressedEdge[DIK_F1 + i]) {
                SendNetworkRaceStateMessage(g_pDirectPlayInfo, i + 4);
                PlaySoundChannelOneOnce(i + 4, 0x200020);
                g_dwNetworkMenuState = i + 4;
                break;
            }
        }
    }
    if (g_adwKeyPressedEdge[DIK_ESCAPE]) {
        g_dwLastWmCharParam = 0;
        return -1;
    }
    if (g_adwKeyPressedEdge[DIK_RETURN] || g_adwKeyPressedEdge[DIK_NUMPADENTER] ||
        (IsInputOrEventActive() && !g_dwInNetworkCarSetup)) {
        if (std::strlen(szText) != 0) {
            g_dwLastWmCharParam = 0;
            return 0;
        }
    }
    if (g_dwLastWmCharParam != 0) {
        if (g_dwLastWmCharParam == 8) {
            size_t length = std::strlen(szText);
            if (length != 0) {
                szText[length - 1] = 0;
            }
            g_dwLastWmCharParam = 0;
            return 1;
        }
        char character = LookupKeyCharacter(static_cast<unsigned>(g_dwLastWmCharParam));
        if (character == 0) {
            // Unmapped input remains pending.
            return 1;
        }
        pVTable->MeasureText(this);
        if (nWidth < nInputMaxWidth) {
            size_t length = std::strlen(szText);
            szText[length] = character;
            szText[length + 1] = 0;
        }
    }
    g_dwLastWmCharParam = 0;
    return 1;
}
