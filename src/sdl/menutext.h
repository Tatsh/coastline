#pragma once

/**
 * @file
 * The text of the menus of the console, drawn with the fonts of the import.
 */

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "scenerenderer.h"

namespace coastline {

/** How a string of the menus is drawn. */
struct TextStyle {
    /** Where a string stands from its x. */
    enum class Align {
        Left,   /*!< Position the string's left edge at the requested x coordinate. */
        Center, /*!< Centre the string at the requested x coordinate. */
        Right   /*!< Position the string's right edge at the requested x coordinate. */
    };

    int size = 10;             /*!< The height of the text, in pixels of the 320 by 240 screen. */
    Align align = Align::Left; /*!< Where the string stands from its x. */
    bool shadow = false;       /*!< Whether the glyphs are coloured by the shadow gradient. */
    bool bold = false;         /*!< Whether the default colours are those of bold text. */
    /** Whether the place and the size are of the 320 by 240 screen a frame of high resolution
     * doubles, or of the 640 by 480 screen as they are, as the frames of a race of the PC release
     * draw them. */
    bool doubled = true;
    /** The distance between the baselines of two lines of a string with a newline, in the units of
     * size, or 0 for size. */
    int lineHeight = 0;
    /** The distance from the baseline of each line to the next, in place of lineHeight, for a
     * string of one line more than it lists. A string of another number of lines takes lineHeight.
     */
    std::span<const int> lineGaps;
    /** The red, green, and blue of the bottom and then of the top of the gradient, if not default.
     */
    std::optional<std::array<int, 6>> colors;
};

/**
 * Centre a string on the place of a block of a number of lines, as a release that draws a message
 * in fewer lines than the USA release centres it.
 *
 * @param text The string, with a newline between lines.
 * @param y The baseline of the first line of the block.
 * @param lines The lines of the block.
 * @param lineHeight The distance between the baselines of two lines.
 * @return The baseline of the first line of the string.
 */
int CenterLines(std::string_view text, int y, int lines, int lineHeight);

/** The two fonts of the menus, their gradients, and the layout of their glyphs. */
class MenuText {
public:
    /**
     * Read the fonts of a release.
     *
     * @param renderer The renderer that draws the text.
     * @param fonts The fonts directory of the import, with fonts.json.
     * @return The fonts, or nothing with the reason logged.
     */
    static std::unique_ptr<MenuText> Load(SceneRenderer &renderer,
                                          const std::filesystem::path &fonts);
    /** Release font textures before the renderer's GPU device is destroyed. */
    ~MenuText();
    /** Prevent copying GPU-backed font resources. */
    MenuText(const MenuText &) = delete;
    /** Prevent assignment of GPU-backed font resources. */
    MenuText &operator=(const MenuText &) = delete;

    /**
     * Draw a string as DrawText() of the console draws it on a frame of high resolution.
     *
     * @param renderer The renderer of the frame.
     * @param text The string in UTF-8, with the colour codes of the console. A character without
     * a glyph is not drawn. Each line after a newline is drawn as a string of its own, at
     * TextStyle::lineGaps or TextStyle::lineHeight below the line before.
     * @param x The left, centre, or right of each line, in pixels of the 320 by 240 screen.
     * @param y The baseline of the first line, in pixels of the 320 by 240 screen.
     * @param style How the string is drawn.
     * @param projection The matrix from the 640 by 480 screen to the frame.
     */
    void Draw(SceneRenderer &renderer,
              std::string_view text,
              int x,
              int y,
              const TextStyle &style,
              const Matrix4 &projection) const;

    /**
     * @param text The string in UTF-8.
     * @param size The height of the text, in pixels of the 320 by 240 screen.
     * @param doubled Whether the size is of the 320 by 240 screen a frame of high resolution
     * doubles, or of the 640 by 480 screen as it is.
     * @return The width of the string as MeasureTextWidth() of the console measures it, or of its
     * widest line when it has a newline.
     */
    int Width(std::string_view text, int size, bool doubled = true) const;

private:
    /** A rasterised menu font and its text gradients. */
    struct Font {
        std::unique_ptr<RenderTexture> image;          /*!< The glyph atlas. */
        std::unique_ptr<RenderTexture> gradient;       /*!< The foreground gradient. */
        std::unique_ptr<RenderTexture> shadowGradient; /*!< The shadow gradient. */
        int cell = 0;                                  /*!< The atlas cell size. */
        int padding = 0;                               /*!< The padding around each glyph. */
        std::vector<int> glyphStarts; /*!< The horizontal start of each glyph in its cell. */
    };
    /** The fixed-width Japanese glyphs of the Japanese release, stored in atlas rows. */
    struct JapaneseFont {
        std::unique_ptr<RenderTexture> image;     /*!< The glyph atlas. */
        int width = 0;                            /*!< The width of one glyph. */
        int columns = 0;                          /*!< The number of atlas columns. */
        std::unordered_map<char32_t, int> glyphs; /*!< Atlas positions indexed by code point. */
    };
    MenuText() = default;
    const Font &FontFor(int size) const;

    Font m_large;
    Font m_small;
    std::unordered_map<char32_t, int> m_glyphs;
    int m_firstRowGlyphs = 0;
    std::optional<JapaneseFont> m_japanese;
};

} // namespace coastline
