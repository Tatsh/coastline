#include "textfont.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "firstrun/unscii16.h"
#include "gpu.h"

namespace coastline {

namespace {

constexpr float GRID_ASPECT = static_cast<float>(GRID_WIDTH) / GRID_HEIGHT;
// The glyphs lie 16 across and 16 down the texture.
constexpr int ATLAS_WIDTH = 16 * CELL_WIDTH;
constexpr int ATLAS_HEIGHT = 16 * CELL_HEIGHT;

// The pointer arrow has its tip at the top left. X is black, o is white, and . is clear.
constexpr std::array<std::string_view, 16> POINTER{
    "X...............",
    "XX..............",
    "XoX.............",
    "XooX............",
    "XoooX...........",
    "XooooX..........",
    "XoooooX.........",
    "XooooooX........",
    "XoooooooX.......",
    "XooooooooX......",
    "XoooooXXXXX.....",
    "XooXooX.........",
    "XoX.XooX........",
    "XX..XooX........",
    "X....XooX.......",
    ".....XXXX.......",
};

std::array<float, 2> GridExtent(float aspect) {
    return {std::min(GRID_ASPECT / aspect, 1.0f), std::min(aspect / GRID_ASPECT, 1.0f)};
}

std::array<std::uint8_t, 4> ColorOf(CgaColor color) {
    const auto &rgb = CGA_PALETTE[static_cast<std::size_t>(color)];
    return {rgb[0], rgb[1], rgb[2], 255};
}

void AddQuad(std::vector<FlatVertex> &corners,
             float x,
             float y,
             float width,
             float height,
             float u,
             float v,
             float uWidth,
             float vHeight,
             std::array<std::uint8_t, 4> color) {
    corners.push_back({x, y, u, v, color});
    corners.push_back({x + width, y, u + uWidth, v, color});
    corners.push_back({x + width, y + height, u + uWidth, v + vHeight, color});
    corners.push_back({x, y + height, u, v + vHeight, color});
}

} // namespace

Matrix4 TextGridProjection(float aspect) {
    auto [across, down] = GridExtent(aspect);
    return {2.0f * across / GRID_WIDTH,
            0,
            0,
            0,
            0,
            -2.0f * down / GRID_HEIGHT,
            0,
            0,
            0,
            0,
            1,
            0,
            -across,
            down,
            0,
            1};
}

std::array<int, 2>
GridPointInWindow(float windowWidth, float windowHeight, float frameAspect, float x, float y) {
    windowWidth = std::max(windowWidth, 1.0f);
    windowHeight = std::max(windowHeight, 1.0f);
    // The frame is fitted to the window as the window presents it.
    float fit = std::min(windowWidth / frameAspect, windowHeight);
    float frameX = (x - (windowWidth - fit * frameAspect) / 2.0f) / (fit * frameAspect);
    float frameY = (y - (windowHeight - fit) / 2.0f) / fit;
    auto [across, down] = GridExtent(frameAspect);
    return {static_cast<int>(((frameX * 2.0f - 1.0f) / across + 1.0f) * GRID_WIDTH / 2.0f),
            static_cast<int>(((frameY * 2.0f - 1.0f) / down + 1.0f) * GRID_HEIGHT / 2.0f)};
}

std::unique_ptr<TextFont> TextFont::Create(SceneRenderer &renderer) {
    static_assert(sizeof(UNSCII_16) == 256 * CELL_HEIGHT);
    std::vector<std::uint8_t> atlas(static_cast<std::size_t>(ATLAS_WIDTH * ATLAS_HEIGHT * 4));
    for (int glyph = 0; glyph < 256; ++glyph) {
        for (int row = 0; row < CELL_HEIGHT; ++row) {
            std::uint8_t bits = UNSCII_16[glyph * CELL_HEIGHT + row];
            for (int column = 0; column < CELL_WIDTH; ++column) {
                int x = glyph % 16 * CELL_WIDTH + column;
                int y = glyph / 16 * CELL_HEIGHT + row;
                std::uint8_t *texel = &atlas[static_cast<std::size_t>((y * ATLAS_WIDTH + x) * 4)];
                bool set = (bits & (0x80 >> column)) != 0;
                texel[0] = texel[1] = texel[2] = 255;
                texel[3] = set ? 255 : 0;
            }
        }
    }
    std::vector<std::uint8_t> pointer;
    for (std::string_view line : POINTER) {
        for (char c : line) {
            std::uint8_t level = c == 'o' ? 255 : 0;
            std::uint8_t alpha = c == '.' ? 0 : 255;
            pointer.insert(pointer.end(), {level, level, level, alpha});
        }
    }
    std::unique_ptr<TextFont> font(new TextFont());
    font->m_glyphs =
        RenderTexture::FromTexels(renderer, ATLAS_WIDTH, ATLAS_HEIGHT, atlas, false, false);
    font->m_pointer = RenderTexture::FromTexels(renderer, 16, 16, pointer, false, false);
    if (!font->m_glyphs || !font->m_pointer) {
        return nullptr;
    }
    return font;
}

void TextFont::Draw(SceneRenderer &renderer,
                    const TextGrid &grid,
                    std::optional<std::array<int, 2>> pointer) const {
    const GpuWindow &window = renderer.Window();
    const Matrix4 projection =
        TextGridProjection(static_cast<float>(window.FrameWidth()) /
                           static_cast<float>(std::max(window.FrameHeight(), 1u)));
    std::vector<FlatVertex> backgrounds;
    std::vector<FlatVertex> glyphs;
    backgrounds.reserve(GRID_COLUMNS * GRID_ROWS * 4);
    glyphs.reserve(GRID_COLUMNS * GRID_ROWS * 4);
    constexpr float GLYPH_U = static_cast<float>(CELL_WIDTH) / ATLAS_WIDTH;
    constexpr float GLYPH_V = static_cast<float>(CELL_HEIGHT) / ATLAS_HEIGHT;
    auto addGlyph = [&](int column, int row, std::uint8_t glyph, CgaColor color) {
        AddQuad(glyphs,
                static_cast<float>(column * CELL_WIDTH),
                static_cast<float>(row * CELL_HEIGHT),
                CELL_WIDTH,
                CELL_HEIGHT,
                static_cast<float>(glyph % 16) * GLYPH_U,
                static_cast<float>(glyph / 16) * GLYPH_V,
                GLYPH_U,
                GLYPH_V,
                ColorOf(color));
    };
    for (int row = 0; row < GRID_ROWS; ++row) {
        for (int column = 0; column < GRID_COLUMNS; ++column) {
            const TextCell &cell = grid.At(column, row);
            AddQuad(backgrounds,
                    static_cast<float>(column * CELL_WIDTH),
                    static_cast<float>(row * CELL_HEIGHT),
                    CELL_WIDTH,
                    CELL_HEIGHT,
                    0.0f,
                    0.0f,
                    1.0f,
                    1.0f,
                    ColorOf(cell.background));
            if (cell.glyph != ' ') {
                addGlyph(column, row, cell.glyph, cell.foreground);
            }
            if (cell.underline) {
                AddQuad(backgrounds,
                        static_cast<float>(column * CELL_WIDTH),
                        static_cast<float>((row + 1) * CELL_HEIGHT - 2),
                        CELL_WIDTH,
                        1.0f,
                        0.0f,
                        0.0f,
                        1.0f,
                        1.0f,
                        ColorOf(cell.foreground));
            }
        }
    }
    if (grid.cursor) {
        auto [column, row] = *grid.cursor;
        addGlyph(column, row, '_', grid.At(column, row).foreground);
    }
    renderer.DrawQuads(backgrounds, FlatMaterial{}, projection);
    FlatMaterial text;
    text.texel0 = m_glyphs.get();
    renderer.DrawQuads(glyphs, text, projection);
    if (pointer) {
        std::vector<FlatVertex> arrow;
        AddQuad(arrow,
                static_cast<float>((*pointer)[0]),
                static_cast<float>((*pointer)[1]),
                16.0f,
                16.0f,
                0.0f,
                0.0f,
                1.0f,
                1.0f,
                {255, 255, 255, 255});
        FlatMaterial material;
        material.texel0 = m_pointer.get();
        renderer.DrawQuads(arrow, material, projection);
    }
}

} // namespace coastline
