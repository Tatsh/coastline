#include "textgrid.h"

#include <algorithm>
#include <cstddef>

namespace coastline {

namespace {

// The code point of each character of code page 437. cmake/unscii.cmake reads this list to take
// the glyphs from the font, in this order. The first is the replacement glyph.
constexpr std::array<char32_t, 256> CP437_CODEPOINTS{
    0xFFFD, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022, 0x25D8, 0x25CB, 0x25D9, 0x2642,
    0x2640, 0x266A, 0x266B, 0x263C, 0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
    0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC, 0x0020, 0x0021, 0x0022, 0x0023,
    0x0024, 0x0025, 0x0026, 0x0027, 0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B,
    0x003C, 0x003D, 0x003E, 0x003F, 0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F, 0x0050, 0x0051, 0x0052, 0x0053,
    0x0054, 0x0055, 0x0056, 0x0057, 0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067, 0x0068, 0x0069, 0x006A, 0x006B,
    0x006C, 0x006D, 0x006E, 0x006F, 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2302, 0x00C7, 0x00FC, 0x00E9, 0x00E2,
    0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00A2,
    0x00A3, 0x00A5, 0x20A7, 0x0192, 0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
    0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB, 0x2591, 0x2592, 0x2593, 0x2502,
    0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566,
    0x2560, 0x2550, 0x256C, 0x2567, 0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580, 0x03B1, 0x00DF, 0x0393, 0x03C0,
    0x03A3, 0x03C3, 0x00B5, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A,
    0x207F, 0x00B2, 0x25A0, 0x00A0};

const TextCell BLANK_CELL{};

} // namespace

std::uint8_t Cp437FromCodepoint(char32_t codepoint) {
    if (codepoint >= 0x20 && codepoint < 0x7F) {
        return static_cast<std::uint8_t>(codepoint);
    }
    const auto *found = std::ranges::find(CP437_CODEPOINTS, codepoint);
    return found == CP437_CODEPOINTS.end() ?
               REPLACEMENT_GLYPH :
               static_cast<std::uint8_t>(found - CP437_CODEPOINTS.begin());
}

std::u32string DecodeUtf8(std::string_view text) {
    std::u32string decoded;
    for (std::size_t at = 0; at < text.size();) {
        auto lead = static_cast<unsigned char>(text[at]);
        int length = lead < 0x80         ? 1 :
                     (lead >> 5) == 0x6  ? 2 :
                     (lead >> 4) == 0xE  ? 3 :
                     (lead >> 3) == 0x1E ? 4 :
                                           0;
        char32_t codepoint = length == 1 ? lead :
                             length == 2 ? lead & 0x1F :
                             length == 3 ? lead & 0x0F :
                                           lead & 0x07;
        bool valid = length > 0 && at + length <= text.size();
        for (int i = 1; valid && i < length; ++i) {
            auto next = static_cast<unsigned char>(text[at + i]);
            valid = (next & 0xC0) == 0x80;
            codepoint = (codepoint << 6) | (next & 0x3F);
        }
        if (!valid) {
            decoded += U'�';
            ++at;
            continue;
        }
        decoded += codepoint;
        at += length;
    }
    return decoded;
}

std::string EncodeUtf8(std::u32string_view text) {
    std::string encoded;
    for (char32_t codepoint : text) {
        if (codepoint < 0x80) {
            encoded += static_cast<char>(codepoint);
        } else if (codepoint < 0x800) {
            encoded += static_cast<char>(0xC0 | (codepoint >> 6));
            encoded += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else if (codepoint < 0x10000) {
            encoded += static_cast<char>(0xE0 | (codepoint >> 12));
            encoded += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            encoded += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else {
            encoded += static_cast<char>(0xF0 | (codepoint >> 18));
            encoded += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
            encoded += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            encoded += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    }
    return encoded;
}

TextCell &TextGrid::At(int column, int row) {
    if (column < 0 || column >= GRID_COLUMNS || row < 0 || row >= GRID_ROWS) {
        return m_outside;
    }
    return m_cells[static_cast<std::size_t>(row * GRID_COLUMNS + column)];
}

const TextCell &TextGrid::At(int column, int row) const {
    if (column < 0 || column >= GRID_COLUMNS || row < 0 || row >= GRID_ROWS) {
        return BLANK_CELL;
    }
    return m_cells[static_cast<std::size_t>(row * GRID_COLUMNS + column)];
}

void TextGrid::Fill(int column,
                    int row,
                    int width,
                    int height,
                    std::uint8_t glyph,
                    CgaColor foreground,
                    CgaColor background) {
    for (int y = row; y < row + height; ++y) {
        for (int x = column; x < column + width; ++x) {
            At(x, y) = {glyph, foreground, background};
        }
    }
}

void TextGrid::Write(int column,
                     int row,
                     std::u32string_view text,
                     CgaColor foreground,
                     CgaColor background,
                     int width) {
    std::size_t count = width < 0 ? text.size() : std::min(text.size(), std::size_t(width));
    for (std::size_t i = 0; i < count; ++i) {
        At(column + static_cast<int>(i),
           row) = {Cp437FromCodepoint(text[i]), foreground, background};
    }
}

void TextGrid::WriteUtf8(int column,
                         int row,
                         std::string_view text,
                         CgaColor foreground,
                         CgaColor background,
                         int width) {
    Write(column, row, DecodeUtf8(text), foreground, background, width);
}

void TextGrid::Box(int column,
                   int row,
                   int width,
                   int height,
                   CgaColor foreground,
                   CgaColor background,
                   bool doubled) {
    // The corners, the line across, and the line down, single then double.
    const std::uint8_t topLeft = doubled ? 0xC9 : 0xDA;
    const std::uint8_t topRight = doubled ? 0xBB : 0xBF;
    const std::uint8_t bottomLeft = doubled ? 0xC8 : 0xC0;
    const std::uint8_t bottomRight = doubled ? 0xBC : 0xD9;
    const std::uint8_t across = doubled ? 0xCD : 0xC4;
    const std::uint8_t down = doubled ? 0xBA : 0xB3;
    Fill(column, row, width, height, ' ', foreground, background);
    Fill(column + 1, row, width - 2, 1, across, foreground, background);
    Fill(column + 1, row + height - 1, width - 2, 1, across, foreground, background);
    Fill(column, row + 1, 1, height - 2, down, foreground, background);
    Fill(column + width - 1, row + 1, 1, height - 2, down, foreground, background);
    At(column, row) = {topLeft, foreground, background};
    At(column + width - 1, row) = {topRight, foreground, background};
    At(column, row + height - 1) = {bottomLeft, foreground, background};
    At(column + width - 1, row + height - 1) = {bottomRight, foreground, background};
}

void TextGrid::Shadow(int column, int row, int width, int height) {
    auto darken = [this](int x, int y) {
        TextCell &cell = At(x, y);
        cell.foreground = CgaColor::DarkGray;
        cell.background = CgaColor::Black;
    };
    for (int y = row + 1; y <= row + height; ++y) {
        darken(column + width, y);
        darken(column + width + 1, y);
    }
    for (int x = column + 2; x < column + width; ++x) {
        darken(x, row + height);
    }
}

} // namespace coastline
