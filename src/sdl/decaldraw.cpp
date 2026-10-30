#include <cstring>
#include <utility>

#include "paintshop.h"

namespace coastline {

namespace {

// The runs the flood fill may have pending, and the room past them that the pushes of one run take
// before the count is tested.
constexpr int FILL_SPANS = 400;
constexpr int FILL_SPAN_ROOM = 256;

// The texel of the font that is ink.
constexpr u8 FONT_INK = 15;

// The texels of a cell of the font.
constexpr s32 FONT_CELL = 16;

// The dashes repeat every 8 steps, 4 in each colour.
constexpr int DASH_PERIOD = 8;
constexpr int DASH_LENGTH = 4;

// A run of texels the flood fill has still to look beyond, on the row dy away.
struct FillSpan {
    s32 y;
    s32 x1;
    s32 x2;
    s32 dy;
};

s32 Midpoint(s32 a, s32 b) {
    return (a + b) >> 1;
}

} // namespace

s32 PaintShop::ScreenToTexelX(s32 x) const {
    return (x - m_decalCanvas.x) >> 2;
}

s32 PaintShop::ScreenToTexelY(s32 y) const {
    return (m_decalCanvas.y + m_decalCanvas.height - y) >> 2;
}

u8 PaintShop::ReadPixel4Bit(const u8 *texels, s32 x, s32 y) const {
    s32 offset = TexelOffset(x, y, m_decalWidth);
    // A texel past the decal reads the bytes after it, as the console reads them, within the model.
    if (offset < 0 || texels + offset >= m_decalTexelsEnd) {
        return 0;
    }
    return (x & 1) != 0 ? texels[offset] & 0xF : texels[offset] >> 4;
}

u8 PaintShop::GetPixel4Bit(s32 x, s32 y) const {
    return ReadPixel4Bit(m_decalTexels, x, y);
}

void PaintShop::SetPixel4Bit(s32 x, s32 y, u8 color) {
    if (x < 0 || x >= m_decalWidth || y < 0 || y >= m_decalHeight) {
        return;
    }
    s32 offset = TexelOffset(x, y, m_decalWidth);
    // The mask is a bit a texel, the first texel of a byte in its highest bit.
    if (m_decalMask != nullptr &&
        (m_decalMask[(x >> 3) + y * ((m_decalWidth + 7) >> 3)] & (1 << ((x ^ 7) & 7))) != 0) {
        return;
    }
    // The colour is not masked to 4 bits for the low texel.
    if ((x & 1) != 0) {
        m_decalTexels[offset] = static_cast<u8>((m_decalTexels[offset] & 0xF0) | color);
    } else {
        m_decalTexels[offset] = static_cast<u8>((m_decalTexels[offset] & 0x0F) | color << 4);
    }
    m_previewDirty = true;
}

void PaintShop::FloodFill4Bit(s32 x, s32 y) {
    // The console overruns its stack with more runs than it has room for, and the runs past the
    // room here are dropped.
    std::vector<FillSpan> stack(FILL_SPANS + FILL_SPAN_ROOM);
    x = ScreenToTexelX(x);
    y = ScreenToTexelY(y);
    u8 old = GetPixel4Bit(x, y);
    if (m_decalColor == old) {
        return;
    }
    stack[0] = {y, x, x, 1};
    stack[1] = {y + 1, x, x, -1};
    int count = 2;
    auto push = [&](s32 row, s32 from, s32 to, s32 step) {
        if (count < static_cast<int>(stack.size())) {
            stack[static_cast<std::size_t>(count)] = {row, from, to, step};
        }
        ++count;
    };

    // The fill runs in a copy with the mask off, and is copied back through the mask.
    const u8 *mask = m_decalMask;
    m_decalMask = nullptr;
    std::memcpy(m_scratchTexels.data(), m_decalTexels, m_scratchTexels.size());
    u8 *texels = m_decalTexels;
    const u8 *end = m_decalTexelsEnd;
    m_decalTexels = m_scratchTexels.data();
    m_decalTexelsEnd = m_scratchTexels.data() + m_scratchTexels.size();
    do {
        --count;
        const FillSpan &top = stack[static_cast<std::size_t>(count)];
        s32 dy = top.dy;
        y = top.y + dy;
        s32 x1 = top.x1;
        s32 x2 = top.x2;
        // The scan to the left does not check the width.
        for (x = x1; x >= 0 && y >= 0 && y < m_decalHeight && GetPixel4Bit(x, y) == old; --x) {
            SetPixel4Bit(x, y, m_decalColor);
        }
        s32 left = 0;
        bool skip = x >= x1;
        if (!skip) {
            left = x + 1;
            if (left < x1) {
                push(y, left, x1 - 1, -dy);
            }
            x = x1 + 1;
        }
        do {
            if (!skip) {
                for (; x < m_decalWidth && y >= 0 && y < m_decalHeight && GetPixel4Bit(x, y) == old;
                     ++x) {
                    SetPixel4Bit(x, y, m_decalColor);
                }
                push(y, left, x - 1, dy);
                if (x2 + 1 < x) {
                    push(y, x2 + 1, x - 1, -dy);
                }
            }
            skip = false;
            for (++x; x <= x2 && x < m_decalWidth && y >= 0 && y < m_decalHeight &&
                      GetPixel4Bit(x, y) != old;
                 ++x) {
            }
            left = x;
        } while (x <= x2);
    } while (count > 0 && count < FILL_SPANS);

    m_decalMask = mask;
    for (y = 0; y < m_decalHeight; ++y) {
        for (x = 0; x < m_decalWidth; ++x) {
            u8 color = GetPixel4Bit(x, y);
            std::swap(m_decalTexels, texels);
            std::swap(m_decalTexelsEnd, end);
            SetPixel4Bit(x, y, color);
            std::swap(m_decalTexels, texels);
            std::swap(m_decalTexelsEnd, end);
        }
    }
    m_decalTexels = texels;
    m_decalTexelsEnd = end;
}

u8 PaintShop::FontTexel(s32 column, s32 row) const {
    // A texel of the font, read without the swap of odd rows.
    const auto &pixels = m_decalFont->pixels;
    auto at = static_cast<std::size_t>((column >> 1) + row * (m_decalFont->width >> 1));
    if (at >= pixels.size()) {
        return 0;
    }
    return (column & 1) != 0 ? pixels[at] & 0xF : pixels[at] >> 4;
}

s32 PaintShop::GlyphAdvance(int glyph, int next) const {
    const PaintGlyph &a = m_decalGlyphs[static_cast<std::size_t>(glyph)];
    const PaintGlyph &b = m_decalGlyphs[static_cast<std::size_t>(next)];
    return a.width - (a.kernRight < b.kernLeft ? a.kernRight : b.kernLeft) + 2;
}

void PaintShop::StampTextOnDecal(s32 x, s32 y) {
    x = ScreenToTexelX(x) + 1;
    y = ScreenToTexelY(y);
    s32 penX = x - (m_decalTextWidth >> 1);
    s32 glyphIndex = 0;
    for (int i = 0; i < m_decalTextLength; ++i) {
        const PaintGlyph &glyph = m_decalGlyphs[static_cast<std::size_t>(m_decalTextGlyphs[i])];
        // A character missing from the font reuses the index of the glyph before.
        for (int entry = 0; entry < static_cast<int>(m_decalGlyphs.size()); ++entry) {
            if (glyph.character == m_decalGlyphs[static_cast<std::size_t>(entry)].character) {
                glyphIndex = entry;
                break;
            }
        }
        s32 firstColumn = glyph.cellX + glyphIndex * FONT_CELL;
        s32 lastColumn = glyph.width + firstColumn;
        // The bounds are checked before the shadow is moved off by one texel.
        for (int pass = m_paintToolIcons[4]->style != 0 ? 0 : 1; pass < 2; ++pass) {
            for (s32 row = 0; row < m_decalFont->height; ++row) {
                for (s32 column = firstColumn; column <= lastColumn; ++column) {
                    s32 texelX = column - firstColumn + penX;
                    s32 texelY = row + y;
                    if (texelX < 0 || texelX >= m_decalWidth || texelY < 0 ||
                        texelY >= m_decalHeight || FontTexel(column, row) != FONT_INK) {
                        continue;
                    }
                    if (pass == 0) {
                        SetPixel4Bit(texelX + 1, texelY - 1, m_textShadowColor);
                    } else {
                        SetPixel4Bit(texelX, texelY, m_decalColor);
                    }
                }
            }
        }
        if (i < m_decalTextLength - 1) {
            penX += GlyphAdvance(m_decalTextGlyphs[i], m_decalTextGlyphs[i + 1]);
        }
    }
}

void PaintShop::DrawLine4Bit(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 radius = m_brushSizeRects[m_brushSize].width >> 1;
    x0 = ScreenToTexelX(x0);
    y0 = ScreenToTexelY(y0);
    x1 = ScreenToTexelX(x1);
    y1 = ScreenToTexelY(y1);
    s32 dx = x1 - x0 < 0 ? -(x1 - x0) : x1 - x0;
    s32 dy = y1 - y0 < 0 ? -(y1 - y0) : y1 - y0;
    // An exact diagonal drawn downwards is not swapped, and draws nothing.
    if ((dy < dx && x1 < x0) || (dx < dy && y1 < y0)) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    s32 stepY = y1 < y0 ? -1 : 1;
    s32 stepX = x1 < x0 ? -1 : 1;
    s32 x = x0;
    s32 y = y0;
    auto paint = [&]() {
        if (m_brushSize == 0) {
            SetPixel4Bit(x, y, m_decalColor);
        } else {
            FillCircle4Bit(x, y, radius, false);
        }
    };
    s32 error = 0;
    if (dy < dx) {
        for (; x <= x1; ++x) {
            if (error >= dx) {
                error -= dx;
                y += stepY;
            }
            paint();
            error += dy;
        }
        return;
    }
    for (; y <= y1; ++y) {
        if (error >= dy) {
            error -= dy;
            x += stepX;
        }
        paint();
        error += dx;
    }
}

void PaintShop::FillTexels(s32 x0, s32 y0, s32 x1, s32 y1) {
    for (s32 y = y0; y < y1; ++y) {
        for (s32 x = x0; x < x1; ++x) {
            SetPixel4Bit(x, y, m_decalColor);
        }
    }
}

void PaintShop::FillRect4Bit(s32 x0, s32 y0, s32 x1, s32 y1) {
    x0 = ScreenToTexelX(x0);
    y0 = ScreenToTexelY(y0);
    x1 = ScreenToTexelX(x1);
    y1 = ScreenToTexelY(y1);
    if (x1 < x0) {
        std::swap(x0, x1);
    }
    if (y1 < y0) {
        std::swap(y0, y1);
    }
    FillTexels(x0, y0, x1, y1);
}

void PaintShop::StrokeDecalRectangle(s32 x0, s32 y0, s32 x1, s32 y1) {
    x0 = ScreenToTexelX(x0);
    y0 = ScreenToTexelY(y0);
    x1 = ScreenToTexelX(x1);
    y1 = ScreenToTexelY(y1);
    if (x1 < x0) {
        std::swap(x0, x1);
    }
    if (y1 < y0) {
        std::swap(y0, y1);
    }
    s32 half = m_brushSizeRects[m_brushSize].width >> 1;
    if (half == 0) {
        // Only the edge at y1 includes the corner at x1.
        FillTexels(x0, y0, x0 + 1, y1);
        FillTexels(x0, y1, x1 + 1, y1 + 1);
        FillTexels(x1, y0, x1 + 1, y1);
        FillTexels(x0, y0, x1, y0 + 1);
        return;
    }
    FillTexels(x0 - half, y0 - half, x0 + half, y1 + half);
    FillTexels(x0, y1 - half, x1, y1 + half);
    FillTexels(x1 - half, y0 - half, x1 + half, y1 + half);
    FillTexels(x0, y0 - half, x1, y0 + half);
}

void PaintShop::FillDecalRoundRect(s32 x0, s32 y0, s32 x1, s32 y1) {
    x0 = ScreenToTexelX(x0);
    y0 = ScreenToTexelY(y0);
    x1 = ScreenToTexelX(x1);
    y1 = ScreenToTexelY(y1);
    if (x1 < x0) {
        std::swap(x0, x1);
    }
    if (y1 < y0) {
        std::swap(y0, y1);
    }
    s32 radius = (x1 - x0 < y1 - y0 ? x1 - x0 : y1 - y0) >> 2;
    FillTexels(x0, y0 + radius, x1, y1 - radius);
    x0 += radius;
    x1 -= radius;
    y0 += radius;
    y1 -= radius;
    // The corners are stepped at twice the resolution and drawn on the even steps. Most rows are
    // drawn twice.
    s32 j = radius * 2;
    s32 i = 0;
    s32 error = -j;
    if (j < 0) {
        return;
    }
    do {
        if ((i & 1) == 0) {
            s32 a = i >> 1;
            s32 b = j >> 1;
            FillTexels(x0 - a, y0 - b, x1 + a, y0 - b + 1);
            FillTexels(x0 - b, y0 - a, x1 + b, y0 - a + 1);
            FillTexels(x0 - a, y1 + b, x1 + a, y1 + b + 1);
            FillTexels(x0 - b, y1 + a, x1 + b, y1 + a + 1);
        }
        error += i;
        ++i;
        if (error >= 0) {
            error -= j;
            --j;
        }
    } while (j >= i);
}

void PaintShop::DrawDecalRoundedRectOutline(s32 x0, s32 y0, s32 x1, s32 y1) {
    x0 = ScreenToTexelX(x0);
    y0 = ScreenToTexelY(y0);
    x1 = ScreenToTexelX(x1);
    y1 = ScreenToTexelY(y1);
    if (x1 < x0) {
        std::swap(x0, x1);
    }
    if (y1 < y0) {
        std::swap(y0, y1);
    }
    s32 half = m_brushSizeRects[m_brushSize].width >> 1;
    s32 quarter = half >> 1;
    s32 radius = ((x1 - x0 < y1 - y0 ? x1 - x0 : y1 - y0) >> 2) + quarter;
    if (half == 0) {
        FillTexels(x0, y0 + radius, x0 + 1, y1 - radius);
        FillTexels(x1, y0 + radius, x1 + 1, y1 - radius);
        FillTexels(x0 + radius, y1, x1 - radius, y1 + 1);
        FillTexels(x0 + radius, y0, x1 - radius, y0 + 1);
    } else {
        // The thick edges extend one texel further than the thin ones.
        FillTexels(x0 - half, y0 + radius, x0 + half, y1 - radius + 1);
        FillTexels(x1 - half, y0 + radius, x1 + half, y1 - radius + 1);
        FillTexels(x0 + radius, y1 - half, x1 - radius + 1, y1 + half);
        FillTexels(x0 + radius, y0 - half, x1 - radius + 1, y0 + half);
    }
    x0 += radius;
    x1 -= radius;
    y0 += radius;
    y1 -= radius;
    // The two largest brushes move the corners on the left and at y0 by a texel.
    if (m_brushSize >= 3) {
        --x0;
        --y0;
    }
    radius += quarter;
    // A run of texels from a point in a direction as long as the brush is wide, or the point alone
    // for the smallest brush.
    auto run = [&](s32 x, s32 y, s32 dx, s32 dy) {
        if (half == 0) {
            SetPixel4Bit(x, y, m_decalColor);
            return;
        }
        for (s32 k = 0; k < half * 2; ++k) {
            SetPixel4Bit(x + k * dx, y + k * dy, m_decalColor);
        }
    };
    s32 j = radius * 2;
    s32 i = 0;
    s32 error = -j;
    if (j < 0) {
        return;
    }
    do {
        if ((i & 1) == 0) {
            s32 a = i >> 1;
            s32 b = j >> 1;
            run(x0 - a, y0 - b, 0, 1);
            run(x0 - a, y1 + b, 0, -1);
            run(x1 + a, y1 + b, 0, -1);
            run(x1 + a, y0 - b, 0, 1);
            run(x1 + b, y1 + a, -1, 0);
            run(x1 + b, y0 - a, -1, 0);
            run(x0 - b, y0 - a, 1, 0);
            run(x0 - b, y1 + a, 1, 0);
        }
        error += i;
        ++i;
        if (error >= 0) {
            error -= j;
            --j;
        }
    } while (j >= i);
}

void PaintShop::DrawDecalFilledEllipse(s32 x0, s32 y0, s32 x1, s32 y1) {
    x0 = ScreenToTexelX(x0);
    y0 = ScreenToTexelY(y0);
    x1 = ScreenToTexelX(x1);
    y1 = ScreenToTexelY(y1);
    if (x1 < x0) {
        std::swap(x0, x1);
    }
    if (y1 < y0) {
        std::swap(y0, y1);
    }
    s32 radiusY = (y1 - y0) >> 1;
    if (radiusY < 2) {
        radiusY = 1;
    }
    s32 radiusX = (x1 - x0) >> 1;
    if (radiusX < 2) {
        radiusX = 1;
    }
    s32 squareY = radiusY * radiusY;
    s32 squareX = radiusX * radiusX;
    auto row = [&](s32 half, s32 at) {
        FillTexels(Midpoint(x0, x1) - half, at, Midpoint(x0, x1) + half, at + 1);
    };

    // Both regions step at twice the resolution and draw on the even steps. The step adds the
    // square of the radius where the midpoint method adds twice it.
    s32 x = 0;
    s32 y = radiusY * 2;
    s32 error = -squareX * (radiusY * 2);
    if (squareX * (radiusY * 2) >= 0) {
        s32 stepY = squareX * (radiusY * 2);
        s32 stepX = 0;
        s32 increment = 0;
        do {
            if ((x & 1) == 0) {
                row(x >> 1, (y >> 1) + Midpoint(y0, y1));
                row(x >> 1, Midpoint(y0, y1) - (y >> 1));
            }
            error += increment;
            stepX += squareY;
            ++x;
            increment += squareY;
            error += increment;
            if (error > 0) {
                error -= squareX * y;
                --y;
                stepY -= squareX;
                error -= squareX * y;
            }
        } while (stepY >= stepX);
    }

    y = 0;
    x = radiusX * 2;
    error = -squareY * x;
    if (x * squareY < 0) {
        return;
    }
    s32 stepX = squareY * x;
    s32 stepY = 0;
    s32 increment = 0;
    do {
        if ((y & 1) == 0) {
            row(x >> 1, (y >> 1) + Midpoint(y0, y1));
            row(x >> 1, Midpoint(y0, y1) - (y >> 1));
        }
        error += increment;
        ++y;
        stepY += squareX;
        increment += squareX;
        error += increment;
        if (error > 0) {
            error -= squareY * x;
            --x;
            stepX -= squareY;
            error -= squareY * x;
        }
    } while (stepX >= stepY);
}

void PaintShop::DrawDecalEllipseOutline(s32 x0, s32 y0, s32 x1, s32 y1) {
    x0 = ScreenToTexelX(x0);
    x1 = ScreenToTexelX(x1);
    y0 = ScreenToTexelY(y0);
    y1 = ScreenToTexelY(y1);
    if (x1 - x0 < 0) {
        std::swap(x0, x1);
    }
    if (y1 - y0 < 0) {
        std::swap(y0, y1);
    }
    s32 brush = m_brushSizeRects[m_brushSize].width;
    s32 radiusY = (y1 - y0 + brush) >> 1;
    if (radiusY < 2) {
        radiusY = 1;
    }
    s32 radiusX = (x1 - x0 + brush) >> 1;
    if (radiusX < 2) {
        radiusX = 1;
    }
    s32 squareY = radiusY * radiusY;
    s32 squareX = radiusX * radiusX;
    // The runs of the brush point into the ellipse.
    auto run = [&](s32 x, s32 y, s32 dx, s32 dy) {
        for (s32 k = 0; k < brush; ++k) {
            SetPixel4Bit(x + k * dx, y + k * dy, m_decalColor);
        }
    };
    s32 cx = Midpoint(x0, x1);
    s32 cy = Midpoint(y0, y1);

    s32 x = 0;
    s32 y = radiusY * 2;
    s32 error = -squareX * (radiusY * 2);
    if (squareX * (radiusY * 2) >= 0) {
        s32 stepY = squareX * (radiusY * 2);
        s32 stepX = 0;
        s32 increment = 0;
        do {
            if ((x & 1) == 0) {
                run(cx + (x >> 1), cy + (y >> 1), 0, -1);
                run(cx + (x >> 1), cy - (y >> 1), 0, 1);
                run(cx - (x >> 1), cy - (y >> 1), 0, 1);
                run(cx - (x >> 1), cy + (y >> 1), 0, -1);
            }
            error += increment;
            stepX += squareY;
            ++x;
            increment += squareY;
            error += increment;
            if (error > 0) {
                error -= squareX * y;
                --y;
                stepY -= squareX;
                error -= squareX * y;
            }
        } while (stepY >= stepX);
    }

    y = 0;
    x = radiusX * 2;
    error = -squareY * x;
    if (x * squareY < 0) {
        return;
    }
    s32 stepX = squareY * x;
    s32 stepY = 0;
    s32 increment = 0;
    do {
        if ((y & 1) == 0) {
            run(cx + (x >> 1), cy + (y >> 1), -1, 0);
            run(cx + (x >> 1), cy - (y >> 1), -1, 0);
            run(cx - (x >> 1), cy - (y >> 1), 1, 0);
            run(cx - (x >> 1), cy + (y >> 1), 1, 0);
        }
        error += increment;
        stepY += squareX;
        ++y;
        increment += squareX;
        error += increment;
        if (error > 0) {
            error -= squareY * x;
            --x;
            stepX -= squareY;
            error -= squareY * x;
        }
    } while (stepX >= stepY);
}

void PaintShop::FillCircle4Bit(s32 x, s32 y, s32 radius, bool fromScreen) {
    if (fromScreen) {
        x = ScreenToTexelX(x);
        y = ScreenToTexelY(y);
    }
    s32 i = 0;
    s32 j = radius * 2;
    s32 error = -j;
    if (j < 0) {
        return;
    }
    do {
        if ((i & 1) == 0) {
            s32 a = i >> 1;
            s32 b = j >> 1;
            FillTexels(x - a, y + b, x + a + 1, y + b + 1);
            FillTexels(x - b, y + a, x + b + 1, y + a + 1);
            FillTexels(x - a, y - b, x + a + 1, y - b + 1);
            FillTexels(x - b, y - a, x + b + 1, y - a + 1);
        }
        error += i;
        ++i;
        if (error >= 0) {
            error -= j;
            --j;
        }
    } while (i <= j);
}

void PaintShop::DrawDecalCircleOutline(s32 x, s32 y, s32 radius) {
    s32 brush = m_brushSizeRects[m_brushSize].width;
    x = ScreenToTexelX(x);
    y = ScreenToTexelY(y);
    auto run = [&](s32 px, s32 py, s32 dx, s32 dy) {
        for (s32 k = 0; k < brush; ++k) {
            SetPixel4Bit(px + k * dx, py + k * dy, m_decalColor);
        }
    };
    s32 j = (radius + (brush >> 1)) * 2;
    s32 i = 0;
    s32 error = -j;
    if (j < 0) {
        return;
    }
    do {
        if ((i & 1) == 0) {
            s32 a = i >> 1;
            s32 b = j >> 1;
            run(x + a, y + b, 0, -1);
            run(x + a, y - b, 0, 1);
            run(x - a, y - b, 0, 1);
            run(x - a, y + b, 0, -1);
            run(x + b, y + a, -1, 0);
            run(x + b, y - a, -1, 0);
            run(x - b, y - a, 1, 0);
            run(x - b, y + a, 1, 0);
        }
        error += i;
        ++i;
        if (error >= 0) {
            error -= j;
            --j;
        }
    } while (i <= j);
}

void PaintShop::AdvanceDashes() {
    // The colours of the dashes swap every 8 frames.
    ++m_screenFrameCount;
    if ((m_screenFrameCount & 7) == 0) {
        std::swap(m_dashColorA, m_dashColorB);
    }
}

void PaintShop::DrawDashedLine(s32 x0, s32 y0, s32 x1, s32 y1) {
    int phase = 0;
    s32 startX = x0;
    s32 startY = y0;
    s32 endX = x1;
    s32 endY = y1;
    s32 dx = x1 - x0 > 0 ? x1 - x0 : -(x1 - x0);
    s32 dy = y1 - y0 > 0 ? y1 - y0 : -(y1 - y0);
    AdvanceDashes();
    if ((dx >= dy && x1 < x0) || (dx < dy && y1 < y0)) {
        startX = x1;
        endX = x0;
        startY = y1;
        endY = y0;
    }
    s32 stepY = endY - startY < 0 ? -1 : 1;
    s32 stepX = endX - startX < 0 ? -1 : 1;
    // Only the odd pixels along the line are drawn.
    s32 error = 0;
    s32 x = startX;
    s32 y = startY;
    if (dx >= dy) {
        for (; x <= endX; ++x) {
            if (error >= dx) {
                error -= dx;
                y += stepY;
            }
            phase = (phase + 1) % DASH_PERIOD;
            if ((x & 1) != 0) {
                DrawPixel(x, y, phase < DASH_LENGTH ? m_dashColorA : m_dashColorB);
            }
            error += dy;
        }
        return;
    }
    for (; y <= endY; ++y) {
        if (error >= dy) {
            error -= dy;
            x += stepX;
        }
        phase = (phase + 1) % DASH_PERIOD;
        if ((y & 1) != 0) {
            DrawPixel(x, y, phase < DASH_LENGTH ? m_dashColorA : m_dashColorB);
        }
        error += dx;
    }
}

void PaintShop::DrawDashedRectangle(s32 x0, s32 y0, s32 x1, s32 y1) {
    int phase = 0;
    AdvanceDashes();
    // With no dash drawn, the corners take the colour of the first phase. The console reads a
    // colour it never set there.
    u8 color = phase != 0 ? m_dashColorA : m_dashColorB;
    if (x1 < x0) {
        std::swap(x0, x1);
    }
    if (y1 < y0) {
        std::swap(y0, y1);
    }
    auto next = [&]() {
        phase = (phase + 1) & 1;
        color = phase != 0 ? m_dashColorA : m_dashColorB;
    };
    for (s32 x = x0; x < x1 - 4; x += 4) {
        next();
        DrawFilledRectHiRes(x, y0, 4, 1, color, color, color);
    }
    for (s32 y = y0; y < y1 - 4; y += 4) {
        next();
        DrawFilledRectHiRes(x1, y, 1, 4, color, color, color);
    }
    DrawFilledRectHiRes(x1, y1 - 1, 1, 2, color, color, color);
    for (s32 x = x0; x < x1 - 4; x += 4) {
        next();
        DrawFilledRectHiRes(x, y1, 4, 1, color, color, color);
    }
    for (s32 y = y0; y < y1 - 4; y += 4) {
        next();
        DrawFilledRectHiRes(x0, y, 1, 4, color, color, color);
    }
    DrawFilledRectHiRes(x0, y1 - 1, 1, 2, color, color, color);
}

void PaintShop::DrawDashedRoundedRect(s32 x0, s32 y0, s32 x1, s32 y1) {
    u8 phase = 0;
    AdvanceDashes();
    if (x1 < x0) {
        std::swap(x0, x1);
    }
    if (y1 < y0) {
        std::swap(y0, y1);
    }
    s32 radius = (x1 - x0 < y1 - y0 ? x1 - x0 : y1 - y0) >> 2;
    auto alternate = [&]() {
        phase = static_cast<u8>((phase + 1) & 1);
        return phase != 0 ? m_dashColorA : m_dashColorB;
    };
    for (s32 y = y0 + radius; y <= y1 - radius; y += 4) {
        u8 color = alternate();
        DrawFilledRectHiRes(x0, y, 1, 4, color, color, color);
    }
    for (s32 y = y0 + radius; y <= y1 - radius; y += 4) {
        u8 color = alternate();
        DrawFilledRectHiRes(x1, y, 1, 4, color, color, color);
    }
    for (s32 x = x0 + radius; x <= x1 - radius; x += 4) {
        u8 color = alternate();
        DrawFilledRectHiRes(x, y0, 4, 1, color, color, color);
    }
    for (s32 x = x0 + radius; x <= x1 - radius; x += 4) {
        u8 color = alternate();
        DrawFilledRectHiRes(x, y1, 4, 1, color, color, color);
    }
    x0 += radius;
    x1 -= radius;
    y0 += radius;
    y1 -= radius;
    // Each corner point is drawn only where its coordinate along the arc is odd.
    s32 j = radius * 2;
    s32 i = 0;
    s32 error = -j;
    if (j < 0) {
        return;
    }
    auto point = [&](s32 along, s32 x, s32 y, u8 color) {
        if ((along & 1) != 0) {
            DrawPixel(x, y, color);
        }
    };
    do {
        if ((i & 1) == 0) {
            phase = static_cast<u8>((phase + 1) % DASH_PERIOD);
            u8 color = phase < DASH_LENGTH ? m_dashColorA : m_dashColorB;
            s32 a = i >> 1;
            s32 b = j >> 1;
            point(x0 - a, x0 - a, y0 - b, color);
            point(x0 - a, x0 - a, y1 + b, color);
            point(x1 + a, x1 + a, y1 + b, color);
            point(x1 + a, x1 + a, y0 - b, color);
            point(y1 + a, x1 + b, y1 + a, color);
            point(y0 - a, x1 + b, y0 - a, color);
            point(y0 - a, x0 - b, y0 - a, color);
            point(y1 + a, x0 - b, y1 + a, color);
        }
        error += i;
        ++i;
        if (error >= 0) {
            error -= j;
            --j;
        }
    } while (i <= j);
}

void PaintShop::DrawDashedEllipse(s32 x0, s32 y0, s32 x1, s32 y1) {
    int phase = 0;
    if (x1 - x0 < 0) {
        std::swap(x0, x1);
    }
    if (y1 - y0 < 0) {
        std::swap(y0, y1);
    }
    s32 radiusY = (y1 - y0) >> 1;
    if (radiusY < 2) {
        radiusY = 1;
    }
    s32 radiusX = (x1 - x0) >> 1;
    if (radiusX < 2) {
        radiusX = 1;
    }
    s32 squareY = radiusY * radiusY;
    s32 squareX = radiusX * radiusX;
    s32 cx = Midpoint(x0, x1);
    s32 cy = Midpoint(y0, y1);
    auto point = [&](s32 along, s32 x, s32 y, u8 color) {
        if ((along & 1) != 0) {
            DrawPixel(x, y, color);
        }
    };

    // The points are drawn where x is odd in the first region and where y is odd in the second.
    s32 x = 0;
    s32 y = radiusY * 2;
    s32 error = -squareX * y;
    AdvanceDashes();
    if (squareX * y >= 0) {
        s32 stepY = squareX * y;
        s32 stepX = 0;
        s32 increment = 0;
        do {
            if ((x & 1) == 0) {
                phase = (phase + 1) % DASH_PERIOD;
                u8 color = phase < DASH_LENGTH ? m_dashColorA : m_dashColorB;
                point(cx + (x >> 1), cx + (x >> 1), cy + (y >> 1), color);
                point(cx + (x >> 1), cx + (x >> 1), cy - (y >> 1), color);
                point(cx - (x >> 1), cx - (x >> 1), cy - (y >> 1), color);
                point(cx - (x >> 1), cx - (x >> 1), cy + (y >> 1), color);
            }
            error += increment;
            stepX += squareY;
            ++x;
            increment += squareY;
            error += increment;
            if (error > 0) {
                error -= squareX * y;
                --y;
                stepY -= squareX;
                error -= squareX * y;
            }
        } while (stepY >= stepX);
    }

    y = 0;
    x = radiusX * 2;
    error = -squareY * x;
    if (x * squareY < 0) {
        return;
    }
    s32 stepY = 0;
    s32 increment = 0;
    s32 stepX = x * squareY;
    do {
        if ((y & 1) == 0) {
            phase = (phase + 1) % DASH_PERIOD;
            u8 color = phase < DASH_LENGTH ? m_dashColorA : m_dashColorB;
            point(cy + (y >> 1), cx + (x >> 1), cy + (y >> 1), color);
            point(cy - (y >> 1), cx + (x >> 1), cy - (y >> 1), color);
            point(cy - (y >> 1), cx - (x >> 1), cy - (y >> 1), color);
            point(cy + (y >> 1), cx - (x >> 1), cy + (y >> 1), color);
        }
        error += increment;
        stepY += squareX;
        ++y;
        increment += squareX;
        error += increment;
        if (error > 0) {
            error -= squareY * x;
            --x;
            stepX -= squareY;
            error -= squareY * x;
        }
    } while (stepX >= stepY);
}

void PaintShop::DrawDashedCircle(s32 x, s32 y, s32 radius) {
    // The bounds of the decal are read once, before the dashes advance.
    s32 left = m_decalCanvas.x;
    s32 top = m_decalCanvas.y;
    s32 right = left + m_decalCanvas.width;
    s32 bottom = top + m_decalCanvas.height;
    int phase = 0;
    AdvanceDashes();
    s32 i = 0;
    s32 j = radius * 2;
    s32 error = -j;
    if (j < 0) {
        return;
    }
    // A point of the circle where it is odd along the arc and over the decal.
    auto point = [&](s32 px, s32 py, s32 along, u8 color) {
        if ((along & 1) != 0 && px >= left && px < right && py >= top && py < bottom) {
            DrawPixel(px, py, color);
        }
    };
    do {
        if ((i & 1) == 0) {
            phase = (phase + 1) % DASH_PERIOD;
            u8 color = phase < DASH_LENGTH ? m_dashColorA : m_dashColorB;
            s32 a = i >> 1;
            s32 b = j >> 1;
            point(x + a, y + b, x + a, color);
            point(x + a, y - b, x + a, color);
            point(x - a, y - b, x - a, color);
            point(x - a, y + b, x - a, color);
            point(x + b, y + a, y + a, color);
            point(x + b, y - a, y - a, color);
            point(x - b, y - a, y - a, color);
            point(x - b, y + a, y + a, color);
        }
        error += i;
        ++i;
        if (error >= 0) {
            error -= j;
            --j;
        }
    } while (i <= j);
}

void PaintShop::DrawDecalTextPreview(s32 x, s32 y) {
    s32 left = x - ((m_decalTextWidth >> 1) << 2);
    s32 bottom = y - (m_decalFont->height << 2);
    s32 glyphIndex = 0;
    for (int i = 0; i < m_decalTextLength; ++i) {
        const PaintGlyph &glyph = m_decalGlyphs[static_cast<std::size_t>(m_decalTextGlyphs[i])];
        // A character missing from the font reuses the index of the glyph before.
        for (int entry = 0; entry < static_cast<int>(m_decalGlyphs.size()); ++entry) {
            if (glyph.character == m_decalGlyphs[static_cast<std::size_t>(entry)].character) {
                glyphIndex = entry;
                break;
            }
        }
        // The glyph is tested at its cell moved right, but drawn moved left. The test at the top
        // of the decal is unsigned.
        s32 glyphLeft = left + (glyph.cellX << 2);
        if (m_decalCanvas.x < glyphLeft + (glyph.width << 2) &&
            glyphLeft < m_decalCanvas.x + m_decalCanvas.width &&
            static_cast<u32>(m_decalCanvas.y) <
                static_cast<u32>(bottom + (m_decalFont->height << 2)) &&
            bottom < m_decalCanvas.y + m_decalCanvas.height) {
            if (m_paintToolIcons[4]->style != 0) {
                const auto &shadow = m_swatches[m_textShadowColor].color;
                DrawScaledTexturedRect(*m_decalFont,
                                       glyphIndex * FONT_CELL,
                                       0,
                                       FONT_CELL,
                                       FONT_CELL,
                                       left - (glyph.cellX << 2) + 4,
                                       bottom + 4,
                                       FONT_CELL * 4,
                                       FONT_CELL * 4,
                                       shadow[0],
                                       shadow[1],
                                       shadow[2]);
            }
            const auto &color = m_swatches[m_decalColor].color;
            DrawScaledTexturedRect(*m_decalFont,
                                   glyphIndex * FONT_CELL,
                                   0,
                                   FONT_CELL,
                                   FONT_CELL,
                                   left - (glyph.cellX << 2),
                                   bottom,
                                   FONT_CELL * 4,
                                   FONT_CELL * 4,
                                   color[0],
                                   color[1],
                                   color[2]);
        }
        if (i < m_decalTextLength - 1) {
            left += GlyphAdvance(m_decalTextGlyphs[i], m_decalTextGlyphs[i + 1]) << 2;
        }
    }
}

} // namespace coastline
