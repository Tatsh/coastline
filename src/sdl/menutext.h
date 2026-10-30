#pragma once

/**
 * @file
 * The text of the menus of the console, drawn with the fonts of the import.
 */

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "scenerenderer.h"

namespace coastline {

/** How a string of the menus is drawn. */
struct TextStyle {
    /** Where a string stands from its x. */
    enum class Align { Left, Center, Right };

    int size = 10;             /*!< The height of the text, in pixels of the 320 by 240 screen. */
    Align align = Align::Left; /*!< Where the string stands from its x. */
    bool shadow = false;       /*!< Whether the glyphs are coloured by the shadow gradient. */
    bool bold = false;         /*!< Whether the default colours are those of bold text. */
    /** Whether the place and the size are of the 320 by 240 screen a frame of high resolution
     * doubles, or of the 640 by 480 screen as they are, as the frames of a race of the PC release
     * draw them. */
    bool doubled = true;
    /** The red, green, and blue of the bottom and then of the top of the gradient, if not default.
     */
    std::optional<std::array<int, 6>> colors;
};

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
    ~MenuText();
    MenuText(const MenuText &) = delete;
    MenuText &operator=(const MenuText &) = delete;

    /**
     * Draw a string as DrawText() of the console draws it on a frame of high resolution.
     *
     * @param renderer The renderer of the frame.
     * @param text The string in UTF-8, with the colour codes of the console. A character without
     * a glyph is not drawn.
     * @param x The left, centre, or right of the string, in pixels of the 320 by 240 screen.
     * @param y The baseline of the string, in pixels of the 320 by 240 screen.
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
     * @return The width of the string as MeasureTextWidth() of the console measures it.
     */
    int Width(std::string_view text, int size, bool doubled = true) const;

private:
    struct Font {
        std::unique_ptr<RenderTexture> image;
        std::unique_ptr<RenderTexture> gradient;
        std::unique_ptr<RenderTexture> shadowGradient;
        int cell = 0;
        int padding = 0;
        std::vector<int> glyphStarts;
    };
    // The glyphs of the Japanese text of the Japanese release, of one width, in rows of an image.
    struct JapaneseFont {
        std::unique_ptr<RenderTexture> image;
        int width = 0;
        int columns = 0;
        std::unordered_map<char32_t, int> glyphs;
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
