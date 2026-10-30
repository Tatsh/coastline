#include "menutext.h"

#include <fstream>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "frontend.h"

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

} // namespace

MenuText::~MenuText() = default;

std::unique_ptr<MenuText> MenuText::Load(SceneRenderer &renderer,
                                         const std::filesystem::path &fonts) {
    std::unique_ptr<MenuText> text(new MenuText());
    nlohmann::json layout;
    try {
        std::ifstream file(fonts / "fonts.json");
        layout = nlohmann::json::parse(file);
        text->m_glyphOfCharacter = layout.at("glyphOfCharacter").get<std::vector<int>>();
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
    } catch (const nlohmann::json::exception &error) {
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

int MenuText::Width(std::string_view text, int size, bool doubled) const {
    // A frame of high resolution doubles every size, and the width halves at the end.
    int scale = doubled ? 2 : 1;
    size *= scale;
    const Font &font = FontFor(size);
    int width = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        auto c = static_cast<unsigned char>(text[i]);
        if (c < '!' || c >= 0x80) {
            width += (size * SPACE_ADVANCE) / SIZE_UNITS;
            continue;
        }
        if (c == '%' && i + 1 < text.size()) {
            char next = text[i + 1];
            if (next == '%') {
                ++i;
            } else if (next == 'i' || next == 'n') {
                ++i;
                continue;
            } else if (i + 2 < text.size()) {
                i += 2;
                continue;
            }
        }
        auto glyph = static_cast<std::size_t>(m_glyphOfCharacter.at(c - '!'));
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

    // The glyphs of each row of the atlas, which samples the gradient by its own transform.
    float atlasWidth = static_cast<float>(font.image->Width());
    float atlasHeight = static_cast<float>(font.image->Height());
    float cellV = static_cast<float>(font.cell) / atlasHeight;
    std::array<std::vector<FlatVertex>, 2> rows;
    auto flush = [&]() {
        for (std::size_t row = 0; row < rows.size(); ++row) {
            if (rows[row].empty()) {
                continue;
            }
            // A glyph samples its column of the gradient from the row of the gradient as far from
            // its bottom as the cell is tall: v of the gradient from v of the atlas.
            float rowTop = static_cast<float>(row) * cellV;
            float scale = static_cast<float>(font.cell) / GRADIENT_ROWS / cellV;
            float start = static_cast<float>(GRADIENT_ROWS - font.cell) / GRADIENT_ROWS;
            material.texel1Transform = {0.0f, scale, 0.5f, start - rowTop * scale};
            renderer.DrawQuads(rows[row], material, projection);
            rows[row].clear();
        }
    };
    for (std::size_t i = 0; i < text.size(); ++i) {
        auto c = static_cast<unsigned char>(text[i]);
        if (c == ' ') {
            x += (size * SPACE_ADVANCE) / SIZE_UNITS + 1;
            continue;
        }
        // %% draws a percent sign, %i and %n draw nothing, and two letters set the colours.
        if (c == '%' && i + 1 < text.size()) {
            char next = text[i + 1];
            if (next == '%') {
                ++i;
                c = '%';
            } else if (next == 'i' || next == 'n') {
                ++i;
                continue;
            } else if (i + 2 < text.size()) {
                flush();
                if (auto prim = PrimitiveOfCode(next)) {
                    material.prim = *prim;
                }
                if (auto env = EnvironmentOfCode(text[i + 2])) {
                    material.env = *env;
                }
                i += 2;
                continue;
            }
        }
        if (c < '!' || c >= 0x80) {
            continue;
        }
        auto glyph = static_cast<std::size_t>(m_glyphOfCharacter.at(c - '!'));
        int left = font.glyphStarts.at(glyph);
        int columns = font.glyphStarts.at(glyph + 1) - left + 1;
        int width = (columns * size) / font.cell;
        // The glyphs before the first row of the console are in the upper row of the image.
        std::size_t row = static_cast<int>(glyph) < m_firstRowGlyphs ? 0 : 1;
        float u0 = static_cast<float>(left) / atlasWidth;
        float u1 = static_cast<float>(left + columns) / atlasWidth;
        float v0 = static_cast<float>(row) * cellV;
        float v1 = v0 + cellV;
        auto fx = static_cast<float>(x);
        auto fy = static_cast<float>(y);
        auto right = static_cast<float>(x + width);
        auto bottom = static_cast<float>(y + size);
        rows[row].insert(
            rows[row].end(),
            {{fx, fy, u0, v0}, {right, fy, u1, v0}, {right, bottom, u1, v1}, {fx, bottom, u0, v1}});
        x += ((columns - font.padding) * size) / font.cell;
    }
    flush();
}

} // namespace coastline
