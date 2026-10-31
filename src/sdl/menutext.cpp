#include "menutext.h"

#include <algorithm>
#include <fstream>
#include <map>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "frontend.h"
#include "stringresources.h"

namespace coastline {

namespace {

// The size below which text takes the small font, the advance of a space, and the height of the
// ascent, in fortieths of the size.
constexpr int SMALL_FONT_MAX_SIZE = 25;
constexpr int SPACE_ADVANCE = 12;
constexpr int ASCENT = 30;
constexpr int SIZE_UNITS = 40;
// The rows of a gradient image.
constexpr int GRADIENT_ROWS = 40;

// The glyph lerps from the environment to the primitive colour by the gradient, takes the alpha of
// the glyph, and the second cycle multiplies by the glyph.
constexpr std::array<std::uint32_t, 2> TEXT_COMBINE = CombineLerp(PRIMITIVE,
                                                                  ENVIRONMENT,
                                                                  TEXEL1,
                                                                  ENVIRONMENT,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  TEXEL0,
                                                                  COMBINED,
                                                                  ZERO_AB,
                                                                  TEXEL1,
                                                                  ZERO_D,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  ZERO_ALPHA,
                                                                  TEXEL1);
// With the shadow, the second cycle takes the gradient back out of the glyph.
constexpr std::array<std::uint32_t, 2> SHADOW_COMBINE = CombineLerp(PRIMITIVE,
                                                                    ENVIRONMENT,
                                                                    TEXEL1,
                                                                    ENVIRONMENT,
                                                                    ZERO_ALPHA,
                                                                    ZERO_ALPHA,
                                                                    ZERO_ALPHA,
                                                                    TEXEL0,
                                                                    COMBINED,
                                                                    TEXEL0,
                                                                    TEXEL1,
                                                                    ZERO_D,
                                                                    ZERO_ALPHA,
                                                                    ZERO_ALPHA,
                                                                    ZERO_ALPHA,
                                                                    TEXEL1);

std::array<float, 4> Color(int red, int green, int blue) {
    return {static_cast<float>(red) / 255.0f,
            static_cast<float>(green) / 255.0f,
            static_cast<float>(blue) / 255.0f,
            1.0f};
}

// The primitive colour of the first letter of a colour code, and the environment colour of the
// second.
std::optional<std::array<float, 4>> PrimitiveOfCode(char code) {
    switch (code) {
    case 'r':
        return Color(0xBE, 0x00, 0x00);
    case 'o':
        return Color(0xCD, 0x5F, 0x00);
    case 'O':
        return Color(0xFF, 0x78, 0x00);
    case 'y':
        return Color(0xFF, 0xF5, 0x00);
    case 'Y':
        return Color(0xFF, 0xFA, 0x80);
    case 'g':
        return Color(0x00, 0x96, 0x00);
    case 'b':
        return Color(0x00, 0x00, 0xC8);
    case 'p':
        return Color(0xC8, 0x00, 0xC8);
    case 'w':
    case '1':
        return Color(0xFF, 0xFF, 0xFF);
    case '5':
        return Color(0x80, 0x80, 0x80);
    case '0':
        return Color(0x00, 0x00, 0x00);
    default:
        return std::nullopt;
    }
}

std::optional<std::array<float, 4>> EnvironmentOfCode(char code) {
    switch (code) {
    case 'r':
        return Color(0xC8, 0x00, 0x00);
    case 'o':
        return Color(0xCD, 0x5F, 0x00);
    case 'O':
        return Color(0xFF, 0x78, 0x00);
    case 'y':
        return Color(0xD2, 0xBE, 0x00);
    case 'Y':
        return Color(0xD2, 0xC8, 0x69);
    case 'g':
        return Color(0x00, 0x96, 0x00);
    case 'b':
        return Color(0x00, 0x00, 0xC8);
    case 'p':
        return Color(0xC8, 0x00, 0xC8);
    case 'w':
    case '1':
        return Color(0xFF, 0xFF, 0xFF);
    case '5':
        return Color(0x80, 0x80, 0x80);
    case '0':
        return Color(0x00, 0x00, 0x00);
    default:
        return std::nullopt;
    }
}

// A character of a string, or a code: %% for a percent sign, %i and %n for nothing, and % with
// two letters for the primitive and the environment colour.
struct TextToken {
    char32_t character = 0;
    bool code = false;
    char primitive = 0;
    char environment = 0;
};

TextToken NextToken(std::string_view text, std::size_t &position) {
    if (text[position] == '%' && position + 1 < text.size()) {
        char next = text[position + 1];
        if (next == '%') {
            position += 2;
            return {U'%'};
        }
        if (next == 'i' || next == 'n') {
            position += 2;
            return {0, true};
        }
        if (position + 2 < text.size()) {
            TextToken token{0, true, next, text[position + 2]};
            position += 3;
            return token;
        }
    }
    return {NextCodePoint(text, position)};
}

} // namespace

MenuText::~MenuText() = default;

std::unique_ptr<MenuText> MenuText::Load(SceneRenderer &renderer,
                                         const std::filesystem::path &fonts) {
    std::unique_ptr<MenuText> text(new MenuText());
    nlohmann::json layout;
    try {
        std::ifstream file(fonts / "fonts.json");
        layout = nlohmann::json::parse(file);
        // The glyphs are by code point, a decimal number for a key.
        auto readGlyphs = [](const nlohmann::json &glyphs) {
            std::unordered_map<char32_t, int> map;
            for (const auto &[codePoint, glyph] : glyphs.items()) {
                map.emplace(static_cast<char32_t>(std::stoul(codePoint)), glyph.get<int>());
            }
            return map;
        };
        text->m_glyphs = readGlyphs(layout.at("glyphs"));
        if (layout.contains("japanese")) {
            const auto &entry = layout.at("japanese");
            JapaneseFont japanese;
            japanese.width = entry.at("width").get<int>();
            japanese.columns = entry.at("columns").get<int>();
            japanese.glyphs = readGlyphs(entry.at("glyphs"));
            japanese.image =
                RenderTexture::Load(renderer, fonts / entry.at("image").get<std::string>(), true);
            if (!japanese.image) {
                return nullptr;
            }
            text->m_japanese = std::move(japanese);
        }
        text->m_firstRowGlyphs = layout.at("firstRowGlyphs").get<int>();
        for (auto [name, font] : {std::pair{"large", &text->m_large}, {"small", &text->m_small}}) {
            const auto &entry = layout.at(name);
            font->cell = entry.at("cell").get<int>();
            font->padding = entry.at("padding").get<int>();
            font->glyphStarts = entry.at("glyphStarts").get<std::vector<int>>();
            font->image =
                RenderTexture::Load(renderer, fonts / entry.at("image").get<std::string>(), true);
            font->gradient = RenderTexture::Load(
                renderer, fonts / entry.at("gradient").get<std::string>(), true);
            font->shadowGradient = RenderTexture::Load(
                renderer, fonts / entry.at("shadowGradient").get<std::string>(), true);
            if (!font->image || !font->gradient || !font->shadowGradient) {
                return nullptr;
            }
        }
    } catch (const std::exception &error) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s is not the layout of the fonts: %s",
                     (fonts / "fonts.json").string().c_str(),
                     error.what());
        return nullptr;
    }
    return text;
}

const MenuText::Font &MenuText::FontFor(int size) const {
    return size < SMALL_FONT_MAX_SIZE ? m_small : m_large;
}

int CenterLines(std::string_view text, int y, int lines, int lineHeight) {
    auto count = static_cast<int>(std::ranges::count(text, '\n')) + 1;
    return y + (lines - count) * lineHeight / 2;
}

int MenuText::Width(std::string_view text, int size, bool doubled) const {
    auto newline = text.find('\n');
    if (newline != std::string_view::npos) {
        return std::max(Width(text.substr(0, newline), size, doubled),
                        Width(text.substr(newline + 1), size, doubled));
    }
    // A frame of high resolution doubles every size, and the width halves at the end.
    int scale = doubled ? 2 : 1;
    size *= scale;
    const Font &font = FontFor(size);
    int width = 0;
    for (std::size_t i = 0; i < text.size();) {
        TextToken token = NextToken(text, i);
        if (token.code) {
            continue;
        }
        // Every Japanese glyph has the same advance.
        if (m_japanese && m_japanese->glyphs.contains(token.character)) {
            width +=
                ((font.cell * m_japanese->width / SIZE_UNITS - font.padding) * size) / font.cell;
            continue;
        }
        auto found = m_glyphs.find(token.character);
        if (found == m_glyphs.end()) {
            width += (size * SPACE_ADVANCE) / SIZE_UNITS;
            continue;
        }
        auto glyph = static_cast<std::size_t>(found->second);
        width += ((font.glyphStarts.at(glyph + 1) - font.glyphStarts.at(glyph) - font.padding + 1) *
                  size) /
                 font.cell;
    }
    return width / scale;
}

void MenuText::Draw(SceneRenderer &renderer,
                    std::string_view text,
                    int x,
                    int y,
                    const TextStyle &style,
                    const Matrix4 &projection) const {
    if (text.contains('\n')) {
        auto lines = static_cast<std::size_t>(std::ranges::count(text, '\n')) + 1;
        bool gaps = style.lineGaps.size() + 1 == lines;
        int lineHeight = style.lineHeight != 0 ? style.lineHeight : style.size;
        std::size_t start = 0;
        for (std::size_t line = 0; line < lines; ++line) {
            auto end = text.find('\n', start);
            Draw(renderer, text.substr(start, end - start), x, y, style, projection);
            if (line + 1 < lines) {
                y += gaps ? style.lineGaps[line] : lineHeight;
            }
            start = end + 1;
        }
        return;
    }
    if (style.align == TextStyle::Align::Center) {
        x -= Width(text, style.size, style.doubled) >> 1;
    } else if (style.align == TextStyle::Align::Right) {
        x -= Width(text, style.size, style.doubled);
    }
    int size = style.size;
    y -= (size * ASCENT) / SIZE_UNITS;
    // A frame of high resolution doubles the place and the size.
    int scale = style.doubled ? 2 : 1;
    x *= scale;
    y *= scale;
    size *= scale;
    const Font &font = FontFor(size);

    FlatMaterial material;
    material.texel0 = font.image.get();
    material.texel1 = style.shadow ? font.shadowGradient.get() : font.gradient.get();
    material.combine = style.shadow ? SHADOW_COMBINE : TEXT_COMBINE;
    material.twoCycle = true;
    if (style.colors) {
        const auto &c = *style.colors;
        material.env = Color(c[0], c[1], c[2]);
        material.prim = Color(c[3], c[4], c[5]);
    } else if (style.bold) {
        material.env = Color(0xFF, 0x7F, 0x00);
        material.prim = Color(0xFF, 0xFF, 0x7F);
    } else {
        material.env = Color(0xC8, 0x00, 0x00);
        material.prim = Color(0xE6, 0xE6, 0x00);
    }

    // The glyphs of each row of an atlas, which samples the gradient by its own transform.
    using Rows = std::map<int, std::vector<FlatVertex>>;
    Rows latinRows;
    Rows japaneseRows;
    auto flushRows = [&](Rows &rows, const RenderTexture *image, int cell) {
        float cellV = static_cast<float>(cell) / static_cast<float>(image->Height());
        material.texel0 = image;
        for (auto &[row, quads] : rows) {
            if (quads.empty()) {
                continue;
            }
            // A glyph samples its column of the gradient from the row of the gradient as far from
            // its bottom as the cell is tall: v of the gradient from v of the atlas.
            float rowTop = static_cast<float>(row) * cellV;
            float scale = static_cast<float>(cell) / GRADIENT_ROWS / cellV;
            float start = static_cast<float>(GRADIENT_ROWS - cell) / GRADIENT_ROWS;
            material.texel1Transform = {0.0f, scale, 0.5f, start - rowTop * scale};
            renderer.DrawQuads(quads, material, projection);
            quads.clear();
        }
    };
    auto flush = [&]() {
        flushRows(latinRows, font.image.get(), font.cell);
        if (m_japanese) {
            flushRows(japaneseRows, m_japanese->image.get(), m_large.cell);
        }
    };
    // The console loads each glyph as a tile of its own, and its filter clamps at the edges of the
    // tile. The glyphs here share one image, and the half texel within each edge keeps the filter
    // from reading the next glyph.
    auto addGlyph =
        [&](Rows &rows, const RenderTexture *image, int row, int left, int columns, int cell) {
            float atlasWidth = static_cast<float>(image->Width());
            float atlasHeight = static_cast<float>(image->Height());
            float cellV = static_cast<float>(cell) / atlasHeight;
            float u0 = (static_cast<float>(left) + 0.5f) / atlasWidth;
            float u1 = (static_cast<float>(left + columns) - 0.5f) / atlasWidth;
            float v0 = static_cast<float>(row) * cellV + 0.5f / atlasHeight;
            float v1 = static_cast<float>(row) * cellV + cellV - 0.5f / atlasHeight;
            auto fx = static_cast<float>(x);
            auto fy = static_cast<float>(y);
            auto right = static_cast<float>(x + (columns * size) / cell);
            auto bottom = static_cast<float>(y + size);
            rows[row].insert(rows[row].end(),
                             {{fx, fy, u0, v0},
                              {right, fy, u1, v0},
                              {right, bottom, u1, v1},
                              {fx, bottom, u0, v1}});
        };
    for (std::size_t i = 0; i < text.size();) {
        if (text[i] == ' ') {
            x += (size * SPACE_ADVANCE) / SIZE_UNITS + 1;
            ++i;
            continue;
        }
        TextToken token = NextToken(text, i);
        if (token.code) {
            if (token.primitive != 0) {
                flush();
                if (auto prim = PrimitiveOfCode(token.primitive)) {
                    material.prim = *prim;
                }
                if (auto env = EnvironmentOfCode(token.environment)) {
                    material.env = *env;
                }
            }
            continue;
        }
        // A Japanese glyph draws at the large cell, at every size.
        if (m_japanese) {
            auto found = m_japanese->glyphs.find(token.character);
            if (found != m_japanese->glyphs.end()) {
                int columns = m_large.cell * m_japanese->width / SIZE_UNITS;
                addGlyph(japaneseRows,
                         m_japanese->image.get(),
                         found->second / m_japanese->columns,
                         found->second % m_japanese->columns * m_japanese->width,
                         columns,
                         m_large.cell);
                x += ((columns - m_large.padding) * size) / m_large.cell;
                continue;
            }
        }
        auto found = m_glyphs.find(token.character);
        if (found == m_glyphs.end()) {
            continue;
        }
        auto glyph = static_cast<std::size_t>(found->second);
        int left = font.glyphStarts.at(glyph);
        int columns = font.glyphStarts.at(glyph + 1) - left + 1;
        // The glyphs before the first row of the console are in the upper row of the image.
        int row = static_cast<int>(glyph) < m_firstRowGlyphs ? 0 : 1;
        addGlyph(latinRows, font.image.get(), row, left, columns, font.cell);
        x += ((columns - font.padding) * size) / font.cell;
    }
    flush();
}

} // namespace coastline
