#pragma once

/**
 * @file
 * The draw of a TextGrid with the font embedded in the program, and the mouse pointer the
 * program draws over it.
 */

#include <array>
#include <memory>
#include <optional>

#include "scenerenderer.h"
#include "textgrid.h"

namespace coastline {

/** The width of the grid in pixels, 640. */
inline constexpr int GRID_WIDTH = GRID_COLUMNS * CELL_WIDTH;

/** The height of the grid in pixels, 400. */
inline constexpr int GRID_HEIGHT = GRID_ROWS * CELL_HEIGHT;

/**
 * @param aspect The width of the frame over its height.
 * @return The matrix from the pixels of the grid to the frame, the grid fitted in the middle of
 * the frame with bars at the sides or at the top and bottom.
 */
Matrix4 TextGridProjection(float aspect);

/**
 * The place of the mouse on the grid, with the frame fitted to the window and the grid to the
 * frame as TextGridProjection() fits it.
 *
 * @param windowWidth The width of the window.
 * @param windowHeight The height of the window.
 * @param frameAspect The width of the frame over its height.
 * @param x The mouse across the window, in the units of the window.
 * @param y The mouse down the window, in the units of the window.
 * @return The place across and down the grid in pixels, outside it past its edges.
 */
std::array<int, 2>
GridPointInWindow(float windowWidth, float windowHeight, float frameAspect, float x, float y);

/** The texture of the 256 glyphs of the font and of the mouse pointer. */
class TextFont {
public:
    /**
     * Upload the glyphs of unscii-16 and the pointer.
     *
     * @param renderer The renderer that draws the grid.
     * @return The font, or nothing when an upload failed.
     */
    static std::unique_ptr<TextFont> Create(SceneRenderer &renderer);

    /**
     * Draw every cell of the grid and its text cursor, and the pointer last over them.
     *
     * @param renderer The renderer of the frame, between Begin() and End().
     * @param grid The grid.
     * @param pointer The tip of the pointer in pixels of the grid, or none to draw no pointer.
     */
    void Draw(SceneRenderer &renderer,
              const TextGrid &grid,
              std::optional<std::array<int, 2>> pointer) const;

private:
    TextFont() = default;
    std::unique_ptr<RenderTexture> m_glyphs;
    std::unique_ptr<RenderTexture> m_pointer;
};

} // namespace coastline
