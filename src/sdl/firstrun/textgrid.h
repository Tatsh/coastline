#pragma once

/**
 * @file
 * A text-mode screen of 80 by 25 cells, each a character of code page 437 with a foreground and
 * a background colour of the 16 colours of CGA.
 */

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace coastline {

/** The columns of the grid. */
inline constexpr int GRID_COLUMNS = 80;

/** The rows of the grid. */
inline constexpr int GRID_ROWS = 25;

/** The width of a cell in pixels. */
inline constexpr int CELL_WIDTH = 8;

/** The height of a cell in pixels. */
inline constexpr int CELL_HEIGHT = 16;

/** The 16 colours of CGA, by their number. */
enum class CgaColor : std::uint8_t {
    Black,        /*!< #000000. */
    Blue,         /*!< #0000AA. */
    Green,        /*!< #00AA00. */
    Cyan,         /*!< #00AAAA. */
    Red,          /*!< #AA0000. */
    Magenta,      /*!< #AA00AA. */
    Brown,        /*!< #AA5500. */
    LightGray,    /*!< #AAAAAA. */
    DarkGray,     /*!< #555555. */
    LightBlue,    /*!< #5555FF. */
    LightGreen,   /*!< #55FF55. */
    LightCyan,    /*!< #55FFFF. */
    LightRed,     /*!< #FF5555. */
    LightMagenta, /*!< #FF55FF. */
    Yellow,       /*!< #FFFF55. */
    White,        /*!< #FFFFFF. */
};

/** The red, green, and blue of each colour of CGA, by its number. */
inline constexpr std::array<std::array<std::uint8_t, 3>, 16> CGA_PALETTE{{
    {0x00, 0x00, 0x00},
    {0x00, 0x00, 0xAA},
    {0x00, 0xAA, 0x00},
    {0x00, 0xAA, 0xAA},
    {0xAA, 0x00, 0x00},
    {0xAA, 0x00, 0xAA},
    {0xAA, 0x55, 0x00},
    {0xAA, 0xAA, 0xAA},
    {0x55, 0x55, 0x55},
    {0x55, 0x55, 0xFF},
    {0x55, 0xFF, 0x55},
    {0x55, 0xFF, 0xFF},
    {0xFF, 0x55, 0x55},
    {0xFF, 0x55, 0xFF},
    {0xFF, 0xFF, 0x55},
    {0xFF, 0xFF, 0xFF},
}};

/** The glyph drawn for a character code page 437 does not have. */
inline constexpr std::uint8_t REPLACEMENT_GLYPH = 0;

/** A cell of the grid. */
struct TextCell {
    std::uint8_t glyph = ' ';                  /*!< The character, in code page 437. */
    CgaColor foreground = CgaColor::LightGray; /*!< The colour of the character. */
    CgaColor background = CgaColor::Blue;      /*!< The colour behind the character. */
    bool underline = false;
};

/**
 * @param codepoint A Unicode code point.
 * @return The character of code page 437 that shows it, or #REPLACEMENT_GLYPH.
 */
std::uint8_t Cp437FromCodepoint(char32_t codepoint);

/**
 * @param text UTF-8 text. A byte that does not start a valid sequence reads as U+FFFD.
 * @return The code points of the text.
 */
std::u32string DecodeUtf8(std::string_view text);

/**
 * @param text Unicode code points.
 * @return The text in UTF-8.
 */
std::string EncodeUtf8(std::u32string_view text);

/** The cells of the screen and the place of the text cursor. */
class TextGrid {
public:
    /**
     * @param column The column, from 0 at the left.
     * @param row The row, from 0 at the top.
     * @return The cell, or a scratch cell outside the grid.
     */
    TextCell &At(int column, int row);

    /**
     * @param column The column, from 0 at the left.
     * @param row The row, from 0 at the top.
     * @return The cell, or a blank cell outside the grid.
     */
    const TextCell &At(int column, int row) const;

    /**
     * Fill a rectangle of cells with one character in two colours.
     *
     * @param column The left column.
     * @param row The top row.
     * @param width The columns.
     * @param height The rows.
     * @param glyph The character, in code page 437.
     * @param foreground The colour of the character.
     * @param background The colour behind it.
     */
    void Fill(int column,
              int row,
              int width,
              int height,
              std::uint8_t glyph,
              CgaColor foreground,
              CgaColor background);

    /**
     * Write text along a row, a cell for each code point, cut at @p width cells.
     *
     * @param column The first column.
     * @param row The row.
     * @param text The text, in code points.
     * @param foreground The colour of the characters.
     * @param background The colour behind them.
     * @param width The most cells written, or below 0 for no limit.
     */
    void Write(int column,
               int row,
               std::u32string_view text,
               CgaColor foreground,
               CgaColor background,
               int width = -1);

    /**
     * Write UTF-8 text along a row, as Write() writes code points.
     *
     * @param column The first column.
     * @param row The row.
     * @param text The text, in UTF-8.
     * @param foreground The colour of the characters.
     * @param background The colour behind them.
     * @param width The most cells written, or below 0 for no limit.
     */
    void WriteUtf8(int column,
                   int row,
                   std::string_view text,
                   CgaColor foreground,
                   CgaColor background,
                   int width = -1);

    /**
     * Draw the frame of a box with the lines of code page 437 and fill its inside.
     *
     * @param column The left column of the frame.
     * @param row The top row of the frame.
     * @param width The columns of the box, the frame included.
     * @param height The rows of the box, the frame included.
     * @param foreground The colour of the lines.
     * @param background The colour of the box.
     * @param doubled Whether the lines are double.
     */
    void Box(int column,
             int row,
             int width,
             int height,
             CgaColor foreground,
             CgaColor background,
             bool doubled);

    /**
     * Darken the cells one row below and two columns right of a box, as the shadow of a dialog of
     * the QBASIC editor darkens the screen under it.
     *
     * @param column The left column of the box.
     * @param row The top row of the box.
     * @param width The columns of the box.
     * @param height The rows of the box.
     */
    void Shadow(int column, int row, int width, int height);

    /** The cell the text cursor blinks in, or none when it does not show. */
    std::optional<std::array<int, 2>> cursor;

private:
    std::array<TextCell, GRID_COLUMNS * GRID_ROWS> m_cells{};
    TextCell m_outside{};
};

} // namespace coastline
