#include "n64model.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <map>
#include <optional>
#include <set>

#include <fmt/format.h>

namespace coastline {

namespace {

// The buffer of glTF is little-endian, and the columns are copied into it as they are in memory.
static_assert(std::endian::native == std::endian::little);

constexpr std::size_t VERTEX_SIZE = 16;
constexpr std::size_t VERTEX_SLOTS = 32;
// The display list stack of F3DEX.
constexpr int MAX_DEPTH = 10;

// Where a kind of model is loaded, and where its header lists its textures.
struct Layout {
    std::uint32_t base;
    std::size_t textureCountAt;
    std::size_t texturesAt;
    bool streamsTextures; // Whether textures stream from a bank after the size in the first word.
};

constexpr Layout CAR_LAYOUT{0x803c8000, 0x10, 0x14, false};
constexpr Layout TRACK_LAYOUT{0x80025c00, 0x18, 0x1c, true};

constexpr int LODS = 3;
constexpr int BODY_LISTS = 10;
constexpr int WHEEL_LISTS = 3;
constexpr int WHEELS = 4;
constexpr std::size_t BODY_LISTS_AT = 0x18;
constexpr std::size_t WHEEL_LISTS_AT = 0xbc;
constexpr std::size_t WHEELS_AT = WHEEL_LISTS_AT + LODS * WHEEL_LISTS * 4;
// The textures of the materials of a car, whose palettes its colour paints, follow the wheels.
constexpr std::size_t MATERIAL_TEXTURES_AT = WHEELS_AT + WHEELS * 12;
constexpr int VEHICLE_MATERIALS = 12;
constexpr std::size_t CAR_HEADER_END = MATERIAL_TEXTURES_AT + VEHICLE_MATERIALS;
// The bytes of the 16 colours of a palette of 4-bit texels.
constexpr std::size_t PALETTE_4_BYTES = 32;
// The renderer scales the vertices of the body and of each wheel by 1/255 to metres.
constexpr float CAR_SCALE = 1.0f / 255.0f;

constexpr std::array<const char *, BODY_LISTS> BODY_LIST_NAMES{"body0",
                                                               "body1",
                                                               "body2",
                                                               "nearDetail",
                                                               "paint",
                                                               "headlightBeams",
                                                               "underside",
                                                               "body7",
                                                               "textured",
                                                               "highlighted"};
constexpr int VEHICLE_DL_NEAR_DETAIL = 3;
constexpr int VEHICLE_DL_HEADLIGHT_BEAMS = 5;
constexpr int VEHICLE_DL_UNDERSIDE = 6;
constexpr std::size_t NEAR_DETAIL_TEXTURE = 3;
constexpr std::size_t UNDERSIDE_TEXTURE = 6;
constexpr int VEHICLE_DL_HIGHLIGHTED = 9;
constexpr int VEHICLE_DL_BODY = 4;
// gDPSetCombineLERP(TEXEL0, ENVIRONMENT, ENV_ALPHA, ENVIRONMENT, 0, 0, 0, TEXEL0) in both cycles,
// the combiner of the reflection of the sky on a car.
constexpr std::array<std::uint32_t, 2> REFLECTION_COMBINE{0x00167e2c, 0x55fef379};
constexpr std::array<const char *, WHEEL_LISTS> WHEEL_LIST_NAMES{"wheel", "wheel1", "wheelSnow"};

constexpr std::size_t TRACK_SKY_AT = 0x50;
// gDPSetCombineLERP(0, 0, 0, TEXEL0, 0, 0, 0, SHADE) in both cycles, the combiner of the sky.
constexpr std::array<std::uint32_t, 2> SKY_COMBINE{0x00ffffff, 0xfffcf87c};
constexpr std::size_t TRACK_INSTANCE_NAMES_AT = 0x5c;
constexpr std::size_t TRACK_INSTANCES_AT = 0x60;
constexpr std::size_t TRACK_INSTANCE_COUNT_AT = 0x64;
constexpr std::size_t TRACK_FOG_COLOR_AT = 0x80;
constexpr std::size_t TRACK_HEADER_END = 0x230;
constexpr std::size_t TRACK_INSTANCE_SIZE = 0x54;
constexpr std::size_t TRACK_INSTANCE_DISPLAY_LIST_AT = 0x44;
constexpr std::size_t TRACK_INSTANCE_FLAGS_AT = 0x4c;
constexpr std::uint16_t TRACK_INSTANCE_TWO_SIDED = 0x0004;

// The G_ opcodes of F3DEX the walk follows.
constexpr std::uint8_t OP_VTX = 0x04;
constexpr std::uint8_t OP_DL = 0x06;
constexpr std::uint8_t OP_TRI2 = 0xb1;
constexpr std::uint8_t OP_CLEARGEOMETRYMODE = 0xb6;
constexpr std::uint8_t OP_SETGEOMETRYMODE = 0xb7;
constexpr std::uint8_t OP_ENDDL = 0xb8;
constexpr std::uint8_t OP_SETOTHERMODE_L = 0xb9;
constexpr std::uint8_t OP_SETOTHERMODE_H = 0xba;
constexpr std::uint8_t OP_TEXTURE = 0xbb;
constexpr std::uint8_t OP_TRI1 = 0xbf;
constexpr std::uint8_t OP_NOOP = 0xc0;
constexpr std::uint8_t OP_LOADTLUT = 0xf0;
constexpr std::uint8_t OP_SETTILESIZE = 0xf2;
constexpr std::uint8_t OP_LOADBLOCK = 0xf3;
constexpr std::uint8_t OP_LOADTILE = 0xf4;
constexpr std::uint8_t OP_SETTILE = 0xf5;
constexpr std::uint8_t OP_SETPRIMCOLOR = 0xfa;
constexpr std::uint8_t OP_SETENVCOLOR = 0xfb;
constexpr std::uint8_t OP_SETCOMBINE = 0xfc;
constexpr std::uint8_t OP_SETTIMG = 0xfd;

constexpr std::uint8_t OP_RDPLOADSYNC = 0xe6;
constexpr std::uint8_t OP_RDPPIPESYNC = 0xe7;
constexpr std::uint8_t OP_RDPTILESYNC = 0xe8;
constexpr std::uint8_t OP_RDPFULLSYNC = 0xe9;

// The inputs of the colour combiner the coverage reads, and the codes of 0 and 1 of each slot.
constexpr std::uint32_t G_CCMUX_TEXEL0 = 1;
constexpr std::uint32_t G_CCMUX_TEXEL1 = 2;
constexpr std::uint32_t G_CCMUX_PRIMITIVE = 3;
constexpr std::uint32_t G_CCMUX_SHADE = 4;
constexpr std::uint32_t G_CCMUX_ENVIRONMENT = 5;
constexpr std::uint32_t CC_A_ONE = 6;
constexpr std::uint32_t CC_A_B_ZERO = 15;
constexpr std::uint32_t CC_C_ZERO = 31;
constexpr std::uint32_t CC_D_ZERO = 7;
constexpr std::uint32_t G_FOG = 0x10000;
constexpr std::uint32_t G_MDSFT_CYCLETYPE = 20;
constexpr std::uint32_t G_CYC_2CYCLE = 1;

std::array<std::uint8_t, 4> Color(std::uint32_t rgba) {
    return {static_cast<std::uint8_t>(rgba >> 24),
            static_cast<std::uint8_t>(rgba >> 16),
            static_cast<std::uint8_t>(rgba >> 8),
            static_cast<std::uint8_t>(rgba)};
}

// The bits of the geometry mode of F3DEX the walk follows.
constexpr std::uint32_t G_CULL_FRONT = 0x1000;
constexpr std::uint32_t G_CULL_BACK = 0x2000;
constexpr std::uint32_t G_LIGHTING = 0x20000;
constexpr std::uint32_t G_TEXTURE_GEN = 0x40000;

// The formats and sizes of texels of the RDP.
constexpr std::uint32_t G_IM_FMT_RGBA = 0;
constexpr std::uint32_t G_IM_FMT_CI = 2;
constexpr std::uint32_t G_IM_FMT_IA = 3;
constexpr std::uint32_t G_IM_FMT_I = 4;
constexpr std::uint32_t G_IM_SIZ_4b = 0;
constexpr std::uint32_t G_IM_SIZ_8b = 1;
constexpr std::uint32_t G_IM_SIZ_16b = 2;
constexpr std::uint32_t G_IM_SIZ_32b = 3;

// Where the palettes start in the texture memory, in its words of 64 bits, and the tiles the game
// sets up for them before a model.
constexpr std::uint32_t PALETTE_TMEM = 0x100;
constexpr std::uint32_t LAST_PALETTE_TMEM = 0x1f0;
constexpr std::size_t PALETTE_TILE = 6;
constexpr std::size_t SECOND_PALETTE_TILE = 5;
// RenderVehicleModel() loads g_awVehicleShadePalette, a ramp of greys from black to white in
// RGBA5551, into palette 14 before the lists of a car. The shading textures of the decals are
// 4-bit texels of that palette, which the second texel of their combiner multiplies.
constexpr std::uint32_t SHADE_PALETTE = 14;
constexpr std::array<std::uint16_t, 16> SHADE_PALETTE_COLORS{0x0001,
                                                             0x1085,
                                                             0x2109,
                                                             0x318d,
                                                             0x4211,
                                                             0x5295,
                                                             0x6319,
                                                             0x739d,
                                                             0x8c63,
                                                             0x9ce7,
                                                             0xad6b,
                                                             0xbdef,
                                                             0xce73,
                                                             0xdef7,
                                                             0xef7b,
                                                             0xffff};
// The texture LOD mode of the other modes that samples the detail tile after the render tile.
constexpr std::uint32_t G_TL_LOD = 1;
constexpr std::uint32_t G_TD_DETAIL = 2;

std::uint32_t Big32(std::span<const std::uint8_t> image, std::size_t at) {
    return static_cast<std::uint32_t>(image[at]) << 24 | image[at + 1] << 16 | image[at + 2] << 8 |
           image[at + 3];
}

std::uint16_t Big16(std::span<const std::uint8_t> image, std::size_t at) {
    return static_cast<std::uint16_t>(image[at] << 8 | image[at + 1]);
}

// Everything that decides the texels of a texture: where its texels and its palette are, as
// offsets in the image, its format and size of texel, its size, and the stride of its rows.
struct TextureKey {
    std::size_t address = 0;
    std::optional<std::size_t> palette;
    // Whether the palette is the one the renderer loads into SHADE_PALETTE, in place of palette.
    bool shadePalette = false;
    std::uint32_t format = 0;
    std::uint32_t texelSize = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t line = 0; // The stride of a row in words of 64 bits, or 0 for packed rows.

    auto operator<=>(const TextureKey &) const = default;
};

// How the render mode of the RDP draws the alpha of a texel.
enum class Alpha {
    Opaque,
    Cut,
    Blend,
};

// The bits of the low word of the other modes the walk follows.
constexpr std::uint32_t G_AC_MASK = 3;
constexpr std::uint32_t G_AC_THRESHOLD = 1;
constexpr std::uint32_t CVG_X_ALPHA = 0x1000;
constexpr std::uint32_t FORCE_BL = 0x4000;
constexpr std::uint32_t G_BL_A_IN = 0;
constexpr std::uint32_t G_BL_CLR_MEM = 1;

// What a triangle is drawn with: its texture, how its texture coordinates map to it, whether it
// is lit and drawn from both sides, and the name the display list gives its material.
struct Material {
    std::optional<TextureKey> texture;
    std::uint32_t clampMirrorS = 0; // The cms of the tile, mirror in bit 0 and clamp in bit 1.
    std::uint32_t clampMirrorT = 0;
    std::uint32_t tileWidth = 0; // The texels the tile spans, which clamping limits coordinates to.
    std::uint32_t tileHeight = 0;
    std::uint32_t shiftS = 0;
    std::uint32_t shiftT = 0;
    std::uint32_t uls = 0;
    std::uint32_t ult = 0;
    double scaleS = 1.0;
    double scaleT = 1.0;
    bool lit = false;
    bool twoSided = false;
    Alpha alpha = Alpha::Opaque;
    // The primitive or environment colour the colour combiner reads, red, green, blue, and alpha.
    std::optional<std::array<std::uint8_t, 4>> color;
    // The state of the RDP the triangle is drawn with, for a renderer to draw it as the console
    // does: the words of the combiner, the other modes, the geometry mode, the primitive and the
    // environment colour, and the texture of the tile after the render tile.
    std::uint32_t combineHigh = 0;
    std::uint32_t combineLow = 0;
    std::uint32_t otherModeH = 0;
    std::uint32_t otherModeL = 0;
    std::uint32_t geometryMode = 0;
    std::uint32_t primColor = 0;
    std::uint32_t envColor = 0;
    std::optional<TextureKey> texture1;
    std::string name;

    auto operator<=>(const Material &) const = default;
};

// A triangle, as offsets of its vertex records in the image, and the index of its material.
struct Triangle {
    std::array<std::uint32_t, 3> corners;
    std::size_t material;
};

// The triangles of one display list.
using Triangles = std::vector<Triangle>;

// A tile of the RDP, as G_SETTILE and G_SETTILESIZE describe it.
struct Tile {
    std::uint32_t format = 0;
    std::uint32_t texelSize = 0;
    std::uint32_t line = 0;
    std::uint32_t tmem = 0;
    std::uint32_t palette = 0;
    std::uint32_t cmt = 0;
    std::uint32_t maskt = 0;
    std::uint32_t shiftt = 0;
    std::uint32_t cms = 0;
    std::uint32_t masks = 0;
    std::uint32_t shifts = 0;
    std::uint32_t uls = 0;
    std::uint32_t ult = 0;
    std::uint32_t lrs = 0;
    std::uint32_t lrt = 0;
};

// The state a display list starts from: lighting, culling, and a texture the renderer binds.
struct StartState {
    bool lit = false;
    bool twoSided = false;
    std::optional<TextureKey> bound;
    // The combiner the caller of the display list sets before it, in the two words of the command.
    std::array<std::uint32_t, 2> combine{};
    // Whether the caller turns on the generation of the texture coordinates from the normals.
    bool textureGen = false;
    // Whether the caller loads the shade palette into SHADE_PALETTE.
    bool shadePalette = false;
};

class DisplayListWalk {
public:
    DisplayListWalk(std::span<const std::uint8_t> image, std::uint32_t base)
        : m_image(image), m_base(base) {
    }

    std::optional<std::string>
    Walk(std::uint32_t address, Triangles &triangles, const StartState &start) {
        m_slots.fill(std::nullopt);
        m_tiles = {};
        m_tiles[PALETTE_TILE].tmem = LAST_PALETTE_TMEM;
        m_tiles[SECOND_PALETTE_TILE].tmem = PALETTE_TMEM;
        m_tmem.clear();
        m_shadePalette = start.shadePalette;
        m_geometryMode = (start.lit ? G_LIGHTING : 0) | (start.twoSided ? 0 : G_CULL_BACK) |
                         (start.textureGen ? G_TEXTURE_GEN : 0);
        m_otherModeH = 0;
        m_otherModeL = 0;
        m_primColor = 0xffffffff;
        m_envColor = 0xffffffff;
        m_combineHigh = start.combine[0];
        m_combineLow = start.combine[1];
        m_textureOn = true;
        m_textureTile = 0;
        m_scaleS = 1.0;
        m_scaleT = 1.0;
        m_bound = start.bound;
        m_name.clear();
        return Run(address, triangles, 0);
    }

    // The offsets of every vertex record a display list loaded.
    const std::map<std::uint32_t, bool> &Vertices() const {
        return m_vertices;
    }

    // The materials of the triangles, by their index.
    const std::vector<Material> &Materials() const {
        return m_materials;
    }

    // What of the display lists glTF does not express: the commands the walk does not follow,
    // by opcode, and the triangles drawn with a combiner other than a product of a texel, the
    // shade, and a constant colour, with fog, or in two cycles.
    nlohmann::ordered_json Coverage() const {
        nlohmann::ordered_json commands = nlohmann::ordered_json::object();
        for (const auto &[opcode, count] : m_ignored) {
            commands[fmt::format("{:#04x}", opcode)] = count;
        }
        nlohmann::ordered_json combiners = nlohmann::ordered_json::object();
        for (const auto &[combiner, count] : m_combiners) {
            combiners[fmt::format("{:06x}:{:08x}", combiner >> 32, combiner & 0xffffffff)] = count;
        }
        std::size_t inexact = m_inexact;
        return {{"triangles", m_triangles},
                {"exactTriangles", m_triangles - inexact},
                {"fogTriangles", m_fogTriangles},
                {"twoCycleTriangles", m_twoCycleTriangles},
                {"rendererTexelTriangles", m_rendererTexelTriangles},
                {"combiners", std::move(combiners)},
                {"ignoredCommands", std::move(commands)}};
    }

private:
    std::optional<std::uint32_t> Offset(std::uint32_t address, std::size_t size) const {
        if (address < m_base || address - m_base > m_image.size() ||
            size > m_image.size() - (address - m_base)) {
            return std::nullopt;
        }
        return address - m_base;
    }

    // The address loaded into the texture memory at a word, from the load nearest below it.
    std::optional<std::uint32_t> Lookup(std::uint32_t tmem) const {
        auto loaded = m_tmem.upper_bound(tmem);
        if (loaded == m_tmem.begin()) {
            return std::nullopt;
        }
        --loaded;
        return loaded->second + (tmem - loaded->first) * 8;
    }

    // The material of the next triangle, from the tile the RDP samples.
    Material Current() const {
        Material material;
        material.lit = (m_geometryMode & G_LIGHTING) != 0;
        material.twoSided = (m_geometryMode & (G_CULL_BACK | G_CULL_FRONT)) == 0;
        // A combiner that reads the primitive or the environment colour into the colour of the
        // first cycle colours the material with it.
        std::array<std::uint32_t, 4> inputs{m_combineHigh >> 20 & 0xf,
                                            m_combineLow >> 28 & 0xf,
                                            m_combineHigh >> 15 & 0x1f,
                                            m_combineLow >> 15 & 7};
        if (std::ranges::contains(inputs, G_CCMUX_PRIMITIVE)) {
            material.color = Color(m_primColor);
        } else if (std::ranges::contains(inputs, G_CCMUX_ENVIRONMENT)) {
            material.color = Color(m_envColor);
        }
        // The render mode cuts texels out by their alpha, or blends them over the frame by it.
        auto blendsByAlpha = [this](std::uint32_t cycle) {
            std::uint32_t bits = m_otherModeL >> (cycle == 1 ? 18 : 16);
            return (bits >> 8 & 3) == G_BL_A_IN && (bits >> 4 & 3) == G_BL_CLR_MEM;
        };
        if ((m_otherModeL & G_AC_MASK) == G_AC_THRESHOLD || (m_otherModeL & CVG_X_ALPHA) != 0) {
            material.alpha = Alpha::Cut;
        } else if ((m_otherModeL & FORCE_BL) != 0 && (blendsByAlpha(1) || blendsByAlpha(2))) {
            material.alpha = Alpha::Blend;
        }
        material.name = m_name;
        material.scaleS = m_scaleS;
        material.scaleT = m_scaleT;
        material.combineHigh = m_combineHigh;
        material.combineLow = m_combineLow;
        material.otherModeH = m_otherModeH;
        material.otherModeL = m_otherModeL;
        material.geometryMode = m_geometryMode;
        material.primColor = m_primColor;
        material.envColor = m_envColor;
        // A material with G_TEXTURE_GEN takes its texture coordinates from its normals, which the
        // renderer generates from the geometry mode the material records.
        if (!m_textureOn) {
            return material;
        }
        bool detail =
            (m_otherModeH >> 16 & 1) == G_TL_LOD && (m_otherModeH >> 17 & 3) == G_TD_DETAIL;
        const Tile &tile = m_tiles[(m_textureTile + (detail ? 1 : 0)) & 7];
        material.clampMirrorS = tile.cms;
        material.clampMirrorT = tile.cmt;
        material.tileWidth = ((tile.lrs - tile.uls) >> 2) + 1;
        material.tileHeight = ((tile.lrt - tile.ult) >> 2) + 1;
        material.shiftS = tile.shifts;
        material.shiftT = tile.shiftt;
        material.uls = tile.uls;
        material.ult = tile.ult;
        // The second texel of a combiner that reads it samples the tile after the render tile.
        auto readsTexel1 = [this] {
            return (m_combineHigh >> 20 & 0xf) == G_CCMUX_TEXEL1 ||
                   (m_combineHigh >> 15 & 0x1f) == G_CCMUX_TEXEL1 ||
                   (m_combineLow >> 28 & 0xf) == G_CCMUX_TEXEL1 ||
                   (m_combineLow >> 15 & 7) == G_CCMUX_TEXEL1;
        };
        if (readsTexel1()) {
            material.texture1 = TileTexture(m_tiles[(m_textureTile + (detail ? 2 : 1)) & 7]);
        }
        material.texture = m_bound ? m_bound : TileTexture(tile);
        return material;
    }

    // The texture a tile samples, from the loads into the texture memory.
    std::optional<TextureKey> TileTexture(const Tile &tile) const {
        auto image = Lookup(tile.tmem);
        auto address = image ? Offset(*image, 1) : std::nullopt;
        if (!address) {
            return std::nullopt;
        }
        TextureKey key{.address = *address,
                       .palette = std::nullopt,
                       .format = tile.format,
                       .texelSize = tile.texelSize,
                       .width = ((tile.lrs - tile.uls) >> 2) + 1,
                       .height = ((tile.lrt - tile.ult) >> 2) + 1,
                       .line = tile.line};
        if (tile.masks != 0) {
            key.width = std::min(key.width, 1u << tile.masks);
        }
        if (tile.maskt != 0) {
            key.height = std::min(key.height, 1u << tile.maskt);
        }
        if (tile.format == G_IM_FMT_CI && tile.palette == SHADE_PALETTE && m_shadePalette) {
            key.shadePalette = true;
        } else if (tile.format == G_IM_FMT_CI) {
            auto palette = Lookup(PALETTE_TMEM + (tile.palette << 4));
            auto at = palette ? Offset(*palette, 1) : std::nullopt;
            if (!at) {
                return std::nullopt;
            }
            key.palette = *at;
        }
        if (key.width == 0 || key.height == 0 || key.width > 1024 || key.height > 1024) {
            return std::nullopt;
        }
        return key;
    }

    // Counts a triangle by whether glTF expresses the state it is drawn with. The colour of the
    // first cycle, (a - b) * c + d, is a product glTF expresses where b and d are 0 and a and c
    // are each a texel, the shade, a constant colour, or 1.
    void Tally() {
        ++m_triangles;
        std::uint32_t a = m_combineHigh >> 20 & 0xf;
        std::uint32_t b = m_combineLow >> 28 & 0xf;
        std::uint32_t c = m_combineHigh >> 15 & 0x1f;
        std::uint32_t d = m_combineLow >> 15 & 7;
        bool product = b == CC_A_B_ZERO && d == CC_D_ZERO &&
                       (a == G_CCMUX_TEXEL0 || a == G_CCMUX_PRIMITIVE || a == G_CCMUX_SHADE ||
                        a == G_CCMUX_ENVIRONMENT || a == CC_A_ONE) &&
                       (c == G_CCMUX_TEXEL0 || c == G_CCMUX_PRIMITIVE || c == G_CCMUX_SHADE ||
                        c == G_CCMUX_ENVIRONMENT);
        bool constant = a == CC_A_B_ZERO && b == CC_A_B_ZERO && c == CC_C_ZERO &&
                        (d == G_CCMUX_TEXEL0 || d == G_CCMUX_PRIMITIVE || d == G_CCMUX_SHADE ||
                         d == G_CCMUX_ENVIRONMENT);
        bool fog = (m_geometryMode & G_FOG) != 0;
        bool twoCycle = (m_otherModeH >> G_MDSFT_CYCLETYPE & 3) == G_CYC_2CYCLE;
        bool combiner = m_combineHigh != 0 || m_combineLow != 0;
        if (combiner && !product && !constant) {
            ++m_combiners[static_cast<std::uint64_t>(m_combineHigh) << 32 | m_combineLow];
        }
        m_fogTriangles += fog ? 1 : 0;
        m_twoCycleTriangles += twoCycle ? 1 : 0;
        // glTF has no texture coordinates generated from the normals.
        bool generated = m_textureOn && (m_geometryMode & G_TEXTURE_GEN) != 0;
        m_inexact += (combiner && !product && !constant) || fog || twoCycle || generated ? 1 : 0;
    }

    std::optional<std::string> Triangle(std::uint32_t packed, Triangles &triangles) {
        std::array<std::uint32_t, 3> corners{};
        for (int i = 0; i < 3; ++i) {
            std::uint32_t slot = (packed >> (16 - 8 * i) & 0xff) / 2;
            if (slot >= VERTEX_SLOTS || !m_slots[slot]) {
                return fmt::format("a triangle uses the empty vertex slot {}", slot);
            }
            corners[static_cast<std::size_t>(i)] = *m_slots[slot];
        }
        Tally();
        Material material = Current();
        // A second texel no load of the display lists provides comes from the renderer.
        bool readsTexel1 = (m_combineHigh >> 20 & 0xf) == G_CCMUX_TEXEL1 ||
                           (m_combineHigh >> 15 & 0x1f) == G_CCMUX_TEXEL1 ||
                           (m_combineLow >> 28 & 0xf) == G_CCMUX_TEXEL1 ||
                           (m_combineLow >> 15 & 7) == G_CCMUX_TEXEL1;
        m_rendererTexelTriangles += readsTexel1 && !material.texture1 ? 1 : 0;
        auto found = std::ranges::find(m_materials, material);
        if (found == m_materials.end()) {
            m_materials.push_back(std::move(material));
            found = std::prev(m_materials.end());
        }
        triangles.push_back(
            {corners, static_cast<std::size_t>(std::distance(m_materials.begin(), found))});
        return std::nullopt;
    }

    // The text of a G_NOOP, two bytes in its first word and four in its second.
    void Comment(std::uint32_t w0, std::uint32_t w1) {
        for (std::uint32_t c :
             {w0 >> 8 & 0xff, w0 & 0xff, w1 >> 24, w1 >> 16 & 0xff, w1 >> 8 & 0xff, w1 & 0xff}) {
            if (c == 0) {
                // The text of the material, such as material 19 = "Hood".
                auto open = m_comment.find('"');
                auto close = m_comment.rfind('"');
                if (open != std::string::npos && close > open) {
                    m_name = m_comment.substr(open + 1, close - open - 1);
                }
                m_comment.clear();
                return;
            }
            m_comment.push_back(static_cast<char>(c));
        }
    }

    std::optional<std::string> Run(std::uint32_t address, Triangles &triangles, int depth) {
        if (depth >= MAX_DEPTH) {
            return fmt::format("the display lists nest deeper than {}", MAX_DEPTH);
        }
        for (;;) {
            auto at = Offset(address, 8);
            if (!at) {
                return fmt::format("the display list command at {:#x} is outside the model",
                                   address);
            }
            std::uint32_t w0 = Big32(m_image, *at);
            std::uint32_t w1 = Big32(m_image, *at + 4);
            address += 8;
            switch (w0 >> 24) {
            case OP_VTX: {
                std::uint32_t count = w0 >> 10 & 0x3f;
                std::uint32_t first = (w0 >> 16 & 0xff) / 2;
                auto source = Offset(w1, count * VERTEX_SIZE);
                if (!source || first + count > VERTEX_SLOTS) {
                    return fmt::format("the vertices at {:#x} are outside the model", w1);
                }
                for (std::uint32_t i = 0; i < count; ++i) {
                    std::uint32_t vertex = *source + i * static_cast<std::uint32_t>(VERTEX_SIZE);
                    m_slots[first + i] = vertex;
                    m_vertices[vertex] = true;
                }
                break;
            }
            case OP_TRI1:
                if (auto failure = Triangle(w1, triangles)) {
                    return failure;
                }
                break;
            case OP_TRI2:
                if (auto failure = Triangle(w0, triangles)) {
                    return failure;
                }
                if (auto failure = Triangle(w1, triangles)) {
                    return failure;
                }
                break;
            case OP_DL:
                if (auto failure = Run(w1, triangles, depth + 1)) {
                    return failure;
                }
                // A branch does not return to the list.
                if ((w0 >> 16 & 0xff) != 0) {
                    return std::nullopt;
                }
                break;
            case OP_ENDDL:
                return std::nullopt;
            case OP_NOOP:
                Comment(w0, w1);
                break;
            case OP_SETGEOMETRYMODE:
                m_geometryMode |= w1;
                break;
            case OP_CLEARGEOMETRYMODE:
                m_geometryMode &= ~w1;
                break;
            case OP_SETOTHERMODE_H:
            case OP_SETOTHERMODE_L: {
                std::uint32_t shift = w0 >> 8 & 0xff;
                std::uint32_t length = w0 & 0xff;
                std::uint32_t mask = length >= 32 ? 0xffffffff : ((1u << length) - 1) << shift;
                std::uint32_t &mode = (w0 >> 24) == OP_SETOTHERMODE_H ? m_otherModeH : m_otherModeL;
                mode = (mode & ~mask) | (w1 & mask);
                break;
            }
            case OP_TEXTURE:
                m_textureOn = (w0 & 0xff) != 0;
                m_textureTile = w0 >> 8 & 7;
                m_scaleS = (w1 >> 16) / 65536.0;
                m_scaleT = (w1 & 0xffff) / 65536.0;
                m_bound = std::nullopt;
                break;
            case OP_SETTIMG:
                m_textureImage = w1;
                break;
            case OP_SETPRIMCOLOR:
                m_primColor = w1;
                break;
            case OP_SETENVCOLOR:
                m_envColor = w1;
                break;
            case OP_SETCOMBINE:
                m_combineHigh = w0 & 0xffffff;
                m_combineLow = w1;
                break;
            case OP_SETTILE: {
                Tile &tile = m_tiles[w1 >> 24 & 7];
                tile.format = w0 >> 21 & 7;
                tile.texelSize = w0 >> 19 & 3;
                tile.line = w0 >> 9 & 0x1ff;
                tile.tmem = w0 & 0x1ff;
                tile.palette = w1 >> 20 & 0xf;
                tile.cmt = w1 >> 18 & 3;
                tile.maskt = w1 >> 14 & 0xf;
                tile.shiftt = w1 >> 10 & 0xf;
                tile.cms = w1 >> 8 & 3;
                tile.masks = w1 >> 4 & 0xf;
                tile.shifts = w1 & 0xf;
                m_bound = std::nullopt;
                break;
            }
            case OP_SETTILESIZE: {
                Tile &tile = m_tiles[w1 >> 24 & 7];
                tile.uls = w0 >> 12 & 0xfff;
                tile.ult = w0 & 0xfff;
                tile.lrs = w1 >> 12 & 0xfff;
                tile.lrt = w1 & 0xfff;
                m_bound = std::nullopt;
                break;
            }
            case OP_LOADBLOCK:
            case OP_LOADTILE:
            case OP_LOADTLUT:
                m_tmem[m_tiles[w1 >> 24 & 7].tmem] = m_textureImage;
                if (m_tiles[w1 >> 24 & 7].tmem == PALETTE_TMEM + (SHADE_PALETTE << 4)) {
                    m_shadePalette = false;
                }
                m_bound = std::nullopt;
                break;
            case OP_RDPLOADSYNC:
            case OP_RDPPIPESYNC:
            case OP_RDPTILESYNC:
            case OP_RDPFULLSYNC:
                break;
            default:
                ++m_ignored[static_cast<std::uint8_t>(w0 >> 24)];
                break;
            }
        }
    }

    std::span<const std::uint8_t> m_image;
    std::uint32_t m_base;
    std::array<std::optional<std::uint32_t>, VERTEX_SLOTS> m_slots{};
    std::map<std::uint32_t, bool> m_vertices;
    std::vector<Material> m_materials;
    std::array<Tile, 8> m_tiles{};
    std::map<std::uint32_t, std::uint32_t> m_tmem;
    bool m_shadePalette = false;
    std::uint32_t m_geometryMode = 0;
    std::uint32_t m_otherModeH = 0;
    std::uint32_t m_otherModeL = 0;
    std::uint32_t m_primColor = 0xffffffff;
    std::uint32_t m_envColor = 0xffffffff;
    std::uint32_t m_combineHigh = 0;
    std::uint32_t m_combineLow = 0;
    std::map<std::uint8_t, std::size_t> m_ignored;
    std::map<std::uint64_t, std::size_t> m_combiners;
    std::size_t m_triangles = 0;
    std::size_t m_inexact = 0;
    std::size_t m_fogTriangles = 0;
    std::size_t m_twoCycleTriangles = 0;
    std::size_t m_rendererTexelTriangles = 0;
    bool m_textureOn = true;
    std::uint32_t m_textureTile = 0;
    double m_scaleS = 1.0;
    double m_scaleT = 1.0;
    std::optional<TextureKey> m_bound;
    std::uint32_t m_textureImage = 0;
    std::string m_name;
    std::string m_comment;
};

constexpr std::size_t TEXTURE_SIZE = 0x24;
constexpr std::uint32_t TEXTURE_FORMAT_CI4 = 1;
constexpr std::uint32_t TEXTURE_FORMAT_I8 = 4;
constexpr std::uint32_t TEXTURE_ANIMATED = 1u << 20;
constexpr std::uint32_t ROM_NONE = 0xffffffff;
constexpr std::size_t ANIMATION_FRAME_COUNT_AT = 2;
constexpr std::size_t ANIMATION_FRAMES_AT = 8;
constexpr std::size_t ANIMATION_FRAME_SIZE = 12;
constexpr std::size_t CI4_PALETTE_BYTES = 0x20;
constexpr std::size_t PALETTE_BYTES = 0x200;
constexpr int LINEAR = 9729;
constexpr int REPEAT = 10497;
constexpr int MIRRORED_REPEAT = 33648;
constexpr int CLAMP_TO_EDGE = 33071;

// The texture of an entry of the texture table, as the renderer binds it, the format of the table
// read as CI4, I8, or else RGBA 5551.
std::optional<TextureKey>
TableTexture(std::span<const std::uint8_t> image, const Layout &layout, std::size_t entry) {
    std::uint32_t count = Big32(image, layout.textureCountAt);
    std::uint32_t table = Big32(image, layout.texturesAt) - layout.base;
    if (entry >= count || table > image.size() || count > (image.size() - table) / TEXTURE_SIZE) {
        return std::nullopt;
    }
    std::size_t at = table + entry * TEXTURE_SIZE;
    std::uint32_t texels = Big32(image, at);
    std::uint32_t palette = Big32(image, at + 4);
    std::uint32_t format = Big32(image, at + 0x20) >> 24 & 0xf;
    TextureKey key{.address = texels - layout.base,
                   .palette = std::nullopt,
                   .format = 0,
                   .texelSize = 0,
                   .width = Big16(image, at + 12),
                   .height = Big16(image, at + 14),
                   .line = 0};
    if (texels < layout.base || key.width == 0 || key.height == 0) {
        return std::nullopt;
    }
    if (format == TEXTURE_FORMAT_CI4) {
        key.format = G_IM_FMT_CI;
        key.texelSize = G_IM_SIZ_4b;
        key.palette = palette - layout.base;
    } else if (format == TEXTURE_FORMAT_I8) {
        key.format = G_IM_FMT_I;
        key.texelSize = G_IM_SIZ_8b;
    } else {
        key.format = G_IM_FMT_RGBA;
        key.texelSize = G_IM_SIZ_16b;
    }
    return key;
}

// A frame of an animated texture: the time it starts at, in milliseconds, and where its texels and
// its palette are in the bank, or ROM_NONE for those of the geometry.
struct AnimationFrame {
    std::uint32_t time;
    std::uint32_t imageRom;
    std::uint32_t paletteRom;
};

// A texture of a track whose texels or palette stream from the bank after the geometry.
struct StreamedTexture {
    std::uint32_t texels;  // The address of its texels in the geometry.
    std::uint32_t palette; // The address of its palette in the geometry, or 0.
    std::size_t size;      // The size of its texels.
    std::size_t paletteSize;
    std::uint32_t paletteRom; // Where the palette of a texture that is not animated is in the bank.
    std::vector<AnimationFrame> frames; // The frames of an animated texture, or none.
    bool snow = false; // Whether the frames are a texture of the snow and then of other weathers.
};

std::vector<StreamedTexture> ReadStreamedTextures(std::span<const std::uint8_t> image,
                                                  const Layout &layout) {
    std::vector<StreamedTexture> textures;
    std::uint32_t bank = Big32(image, 0);
    std::uint32_t count = Big32(image, layout.textureCountAt);
    std::uint32_t table = Big32(image, layout.texturesAt) - layout.base;
    if (bank > image.size() || table > bank || count > (bank - table) / TEXTURE_SIZE) {
        return textures;
    }
    for (std::size_t entry = 0; entry < count; ++entry) {
        std::size_t at = table + entry * TEXTURE_SIZE;
        std::uint32_t source = Big32(image, at + 8);
        std::uint32_t bits = Big32(image, at + 0x20);
        StreamedTexture texture{.texels = Big32(image, at),
                                .palette = Big32(image, at + 4),
                                .size = bits & 0x3ffff,
                                .paletteSize = (bits >> 24 & 0xf) == TEXTURE_FORMAT_CI4 ?
                                                   CI4_PALETTE_BYTES :
                                                   PALETTE_BYTES,
                                .paletteRom = (source & 0xfff) << 5,
                                .frames = {},
                                .snow = false};
        if (texture.texels == 0) {
            continue;
        }
        if ((bits & TEXTURE_ANIMATED) != 0) {
            std::size_t animation = source - layout.base;
            if (source < layout.base || animation > bank ||
                bank - animation < ANIMATION_FRAMES_AT + 2 * ANIMATION_FRAME_SIZE) {
                continue;
            }
            std::uint16_t frames = Big16(image, animation + ANIMATION_FRAME_COUNT_AT);
            std::size_t first = animation + ANIMATION_FRAMES_AT;
            for (std::size_t frame = 0;
                 frame < frames && first + (frame + 1) * ANIMATION_FRAME_SIZE <= bank;
                 ++frame) {
                std::size_t record = first + frame * ANIMATION_FRAME_SIZE;
                texture.frames.push_back(
                    {Big32(image, record), Big32(image, record + 4), Big32(image, record + 8)});
            }
            texture.snow = frames == 2 && Big32(image, first) == ROM_NONE;
        }
        textures.push_back(std::move(texture));
    }
    return textures;
}

// Copies the texels and the palette of a frame, or of a texture that is not animated, from the
// bank over those of the geometry.
void CopyFrame(std::span<const std::uint8_t> image,
               const Layout &layout,
               const StreamedTexture &texture,
               const AnimationFrame *frame,
               std::vector<std::uint8_t> &memory) {
    std::uint32_t bank = Big32(image, 0);
    auto stream = image.subspan(bank);
    auto copy = [&](std::uint32_t rom, std::uint32_t target, std::size_t size) {
        std::size_t at = target - layout.base;
        if (rom == ROM_NONE || target < layout.base || at > bank || size > bank - at ||
            rom > stream.size() || size > stream.size() - rom) {
            return;
        }
        std::copy_n(stream.begin() + rom, size, memory.begin() + static_cast<std::ptrdiff_t>(at));
    };
    std::uint32_t paletteRom = texture.paletteRom;
    if (frame != nullptr) {
        copy(frame->imageRom, texture.texels, texture.size);
        paletteRom = frame->paletteRom;
    }
    if (texture.palette != 0) {
        copy(paletteRom, texture.palette, texture.paletteSize);
    }
}

// The frame a race starts with: the first, or for a texture of the snow, that of other weathers.
const AnimationFrame *StartFrame(const StreamedTexture &texture) {
    if (texture.frames.empty()) {
        return nullptr;
    }
    return texture.snow ? &texture.frames[1] : &texture.frames[0];
}

// The image of a track as a race starts, with the textures of the bank after the geometry copied
// over the textures of the geometry as the game streams them.
std::vector<std::uint8_t> StreamTextures(std::span<const std::uint8_t> image,
                                         const Layout &layout,
                                         const std::vector<StreamedTexture> &textures) {
    std::vector<std::uint8_t> memory(image.begin(), image.end());
    for (const StreamedTexture &texture : textures) {
        CopyFrame(image, layout, texture, StartFrame(texture), memory);
    }
    return memory;
}

std::array<std::uint8_t, 4> Rgba5551(std::uint32_t value) {
    return {static_cast<std::uint8_t>((value >> 11 & 0x1f) * 255 / 31),
            static_cast<std::uint8_t>((value >> 6 & 0x1f) * 255 / 31),
            static_cast<std::uint8_t>((value >> 1 & 0x1f) * 255 / 31),
            static_cast<std::uint8_t>(value & 1 ? 255 : 0)};
}

// The texels of a texture as 8-bit RGBA, row by row from the top. The game loads every texture
// with G_LOADBLOCK, and the texture memory reads each odd row with its 32-bit words swapped. The
// image of the console has them swapped already.
std::vector<std::uint8_t> DecodeTexture(std::span<const std::uint8_t> memory,
                                        const TextureKey &key) {
    std::uint32_t bits = 4u << key.texelSize;
    std::size_t stride =
        (key.line != 0 ? key.line : (static_cast<std::size_t>(key.width) * bits + 63) / 64) * 8;
    std::vector<std::array<std::uint8_t, 4>> palette;
    if (key.shadePalette) {
        for (std::uint16_t colour : SHADE_PALETTE_COLORS) {
            palette.push_back(Rgba5551(colour));
        }
    } else if (key.format == G_IM_FMT_CI) {
        std::size_t colours = key.texelSize == G_IM_SIZ_4b ? 16 : 256;
        for (std::size_t i = 0; i < colours; ++i) {
            std::size_t at = *key.palette + i * 2;
            palette.push_back(at + 2 <= memory.size() ? Rgba5551(Big16(memory, at)) :
                                                        std::array<std::uint8_t, 4>{});
        }
    }
    std::vector<std::uint8_t> rgba;
    rgba.reserve(static_cast<std::size_t>(key.width) * key.height * 4);
    for (std::uint32_t y = 0; y < key.height; ++y) {
        for (std::uint32_t x = 0; x < key.width; ++x) {
            std::size_t position = static_cast<std::size_t>(x) * bits;
            std::size_t at = key.address + y * stride + ((position >> 3) ^ (y % 2 ? 4 : 0));
            std::uint32_t value = 0;
            if (at + std::max<std::size_t>(bits / 8, 1) <= memory.size()) {
                switch (bits) {
                case 4:
                    value = memory[at] >> (position & 4 ? 0 : 4) & 0xf;
                    break;
                case 8:
                    value = memory[at];
                    break;
                case 16:
                    value = Big16(memory, at);
                    break;
                default:
                    value = Big32(memory, at);
                    break;
                }
            }
            std::array<std::uint8_t, 4> colour{};
            if (key.format == G_IM_FMT_CI) {
                colour = value < palette.size() ? palette[value] : std::array<std::uint8_t, 4>{};
            } else if (key.format == G_IM_FMT_IA && key.texelSize == G_IM_SIZ_4b) {
                auto level = static_cast<std::uint8_t>((value >> 1 & 7) * 255 / 7);
                colour = {level, level, level, static_cast<std::uint8_t>(value & 1 ? 255 : 0)};
            } else if (key.format == G_IM_FMT_IA && key.texelSize == G_IM_SIZ_8b) {
                auto level = static_cast<std::uint8_t>((value >> 4) * 17);
                colour = {level, level, level, static_cast<std::uint8_t>((value & 0xf) * 17)};
            } else if (key.format == G_IM_FMT_IA) {
                auto level = static_cast<std::uint8_t>(value >> 8);
                colour = {level, level, level, static_cast<std::uint8_t>(value)};
            } else if (key.format == G_IM_FMT_I && key.texelSize == G_IM_SIZ_4b) {
                auto level = static_cast<std::uint8_t>(value * 17);
                colour = {level, level, level, level};
            } else if (key.format == G_IM_FMT_I) {
                auto level = static_cast<std::uint8_t>(value);
                colour = {level, level, level, level};
            } else if (key.texelSize == G_IM_SIZ_32b) {
                colour = {static_cast<std::uint8_t>(value >> 24),
                          static_cast<std::uint8_t>(value >> 16),
                          static_cast<std::uint8_t>(value >> 8),
                          static_cast<std::uint8_t>(value)};
            } else {
                colour = Rgba5551(value);
            }
            rgba.insert(rgba.end(), colour.begin(), colour.end());
        }
    }
    return rgba;
}

// The 4-bit texels of a texture, two to a byte with the first in the high four bits, row by row
// from the top with each row starting on a byte, and the odd rows of the texture memory unswapped.
std::vector<std::uint8_t> DecodeIndices4(std::span<const std::uint8_t> memory,
                                         const TextureKey &key) {
    std::size_t stride = (key.line != 0 ? key.line : (key.width * 4 + 63) / 64) * 8;
    std::size_t rowBytes = (key.width + 1) / 2;
    std::vector<std::uint8_t> packed(rowBytes * key.height, 0);
    for (std::uint32_t y = 0; y < key.height; ++y) {
        for (std::uint32_t x = 0; x < key.width; ++x) {
            std::size_t position = static_cast<std::size_t>(x) * 4;
            std::size_t at = key.address + y * stride + ((position >> 3) ^ (y % 2 ? 4 : 0));
            std::uint8_t value =
                at < memory.size() ? memory[at] >> (position & 4 ? 0 : 4) & 0xf : 0;
            packed[y * rowBytes + x / 2] |= static_cast<std::uint8_t>(value << (x % 2 ? 0 : 4));
        }
    }
    return packed;
}

// The wrap of glTF nearest to a tile. The RDP repeats a texture within the span of the tile and
// clamps beyond it, so a tile wider than its texture repeats.
int WrapMode(std::uint32_t clampMirror, std::uint32_t tileSize, std::uint32_t textureSize) {
    if ((clampMirror & 2) != 0 && tileSize <= textureSize) {
        return CLAMP_TO_EDGE;
    }
    return clampMirror & 1 ? MIRRORED_REPEAT : REPEAT;
}

// The scale of a texture coordinate by a shift of a tile: right by 1 to 10, left by 16 less 11
// to 15.
double ShiftScale(std::uint32_t shift) {
    return shift == 0  ? 1.0 :
           shift <= 10 ? 1.0 / (1 << shift) :
                         static_cast<double>(1 << (16 - shift));
}

// Appends bytes to the buffer at an alignment of 4 and returns the index of their buffer view.
std::size_t AddView(GltfModel &model,
                    const void *data,
                    std::size_t size,
                    std::optional<int> target = std::nullopt,
                    std::optional<int> stride = std::nullopt) {
    model.buffer.resize((model.buffer.size() + 3) & ~std::size_t{3});
    std::size_t offset = model.buffer.size();
    model.buffer.resize(offset + size);
    std::memcpy(model.buffer.data() + offset, data, size);
    nlohmann::ordered_json view{{"buffer", 0}, {"byteOffset", offset}, {"byteLength", size}};
    if (stride) {
        view["byteStride"] = *stride;
    }
    if (target) {
        view["target"] = *target;
    }
    model.json["bufferViews"].push_back(view);
    return model.json["bufferViews"].size() - 1;
}

std::size_t AddAccessor(GltfModel &model, nlohmann::ordered_json accessor) {
    model.json["accessors"].push_back(std::move(accessor));
    return model.json["accessors"].size() - 1;
}

constexpr int ARRAY_BUFFER = 34962;
constexpr int ELEMENT_ARRAY_BUFFER = 34963;
constexpr int FLOAT = 5126;
constexpr int SHORT = 5122;
constexpr int UNSIGNED_SHORT = 5123;
constexpr int UNSIGNED_BYTE = 5121;
constexpr int UNSIGNED_INT = 5125;
constexpr int TRIANGLES = 4;
// A vertex attribute fills a multiple of 4 bytes. Each 16-bit flag is followed by 2 bytes of 0.
constexpr int FLAG_STRIDE = 4;

// The views of a vertex record in the image: position, flag, texture coordinate, then the colour
// or the normal and alpha.
struct VertexColumns {
    std::vector<float> positions;
    std::vector<std::uint16_t> flags;
    std::vector<std::int16_t> textureCoordinates;
    std::vector<std::uint8_t> colors;
};

// A place a display list is drawn at: the name of its node and the transform of the node.
struct Placement {
    std::string name;
    nlohmann::ordered_json transform;
};

// A display list, the places it is drawn at, the scene of the places, or none, and the state the
// renderer draws it from.
struct Draw {
    std::string name;
    std::uint32_t displayList;
    std::vector<Placement> placements;
    std::optional<std::size_t> scene;
    StartState start;
};

// A key of a vertex animation of a model: its time, and the offset of its positions, each
// followed by the normals of every vertex.
struct VertexKey {
    float time;
    std::size_t positions;
};

// An animation of the vertices of a model, which moves vertexCount records from vertices
// between its keys.
struct VertexAnimation {
    std::size_t vertices;
    std::size_t vertexCount;
    std::uint16_t flags;
    float startTime;
    float endTime;
    float time;
    std::vector<VertexKey> keys;
};

// The glTF of an image of the console: the meshes of the display lists of the draws, their
// textures and materials, a node for each place, and the bytes of the file outside the vertices.
// Each key of a vertex animation becomes a morph target of every mesh.
std::expected<GltfModel, std::string>
BuildModel(std::span<const std::uint8_t> bytes,
           std::size_t imageOffset,
           const Layout &layout,
           const std::vector<Draw> &draws,
           const std::vector<std::string> &scenes,
           std::string_view bufferUri,
           const std::vector<VertexAnimation> &animations = {},
           const std::map<std::size_t, int> &paintMaterialOf = {}) {
    auto image = bytes.subspan(imageOffset);
    DisplayListWalk walk(image, layout.base);
    // A draw is walked once for each state it starts from.
    std::map<std::pair<std::uint32_t, std::size_t>, Triangles> lists;
    auto keyOf = [&](const Draw &draw) {
        return std::pair{draw.displayList,
                         static_cast<std::size_t>(draw.start.lit) |
                             static_cast<std::size_t>(draw.start.twoSided) << 1 |
                             (draw.start.bound ? draw.start.bound->address << 2 : 0)};
    };
    for (const Draw &draw : draws) {
        auto key = keyOf(draw);
        if (!lists.contains(key)) {
            if (auto failure = walk.Walk(draw.displayList, lists[key], draw.start)) {
                return std::unexpected(fmt::format("{} ({})", *failure, draw.name));
            }
        }
    }

    // Every vertex once in the order of the image, and the runs of adjacent records.
    std::map<std::uint32_t, std::uint32_t> indexOf;
    VertexColumns columns;
    nlohmann::ordered_json runs = nlohmann::ordered_json::array();
    std::vector<std::uint8_t> residual(bytes.begin(), bytes.end());
    for (const auto &[offset, used] : walk.Vertices()) {
        if (!runs.empty() &&
            runs.back()[0].get<std::uint32_t>() +
                    runs.back()[1].get<std::uint32_t>() * static_cast<std::uint32_t>(VERTEX_SIZE) ==
                offset) {
            runs.back()[1] = runs.back()[1].get<std::uint32_t>() + 1;
        } else {
            runs.push_back({offset, 1});
        }
        indexOf[offset] = static_cast<std::uint32_t>(indexOf.size());
        for (int axis = 0; axis < 3; ++axis) {
            columns.positions.push_back(static_cast<std::int16_t>(
                Big16(image, offset + static_cast<std::size_t>(axis) * 2)));
        }
        columns.flags.push_back(Big16(image, offset + 6));
        columns.flags.push_back(0);
        columns.textureCoordinates.push_back(static_cast<std::int16_t>(Big16(image, offset + 8)));
        columns.textureCoordinates.push_back(static_cast<std::int16_t>(Big16(image, offset + 10)));
        for (int i = 12; i < 16; ++i) {
            columns.colors.push_back(image[offset + static_cast<std::size_t>(i)]);
        }
        std::fill_n(residual.begin() + static_cast<std::ptrdiff_t>(imageOffset + offset),
                    VERTEX_SIZE,
                    std::uint8_t{0});
    }
    std::size_t vertexCount = indexOf.size();
    if (vertexCount == 0) {
        return std::unexpected("the display lists of the model do not draw triangles");
    }

    // The image of each texture the materials sample, decoded from the memory as a race starts.
    std::vector<std::uint8_t> streamed;
    std::vector<StreamedTexture> streamedTextures;
    std::span<const std::uint8_t> memory = image;
    if (layout.streamsTextures) {
        streamedTextures = ReadStreamedTextures(image, layout);
        streamed = StreamTextures(image, layout, streamedTextures);
        memory = streamed;
    }
    std::string stem(bufferUri.substr(0, bufferUri.rfind('.')));
    std::vector<std::optional<std::size_t>> textureOf(walk.Materials().size());
    std::map<TextureKey, std::size_t> imageOf;
    GltfModel model;
    auto imageFor = [&](const TextureKey &key) {
        if (!imageOf.contains(key)) {
            imageOf[key] = model.images.size();
            model.images.push_back({fmt::format("{}-{}.png", stem, model.images.size()),
                                    static_cast<int>(key.width),
                                    static_cast<int>(key.height),
                                    DecodeTexture(memory, key),
                                    {},
                                    {},
                                    -1,
                                    key.address});
            // A texture of a material of a car keeps its indices for the colour of the car to
            // paint its first two colours.
            auto painted = paintMaterialOf.find(key.address);
            if (painted != paintMaterialOf.end() && key.format == G_IM_FMT_CI &&
                key.texelSize == G_IM_SIZ_4b && key.palette &&
                *key.palette + PALETTE_4_BYTES <= memory.size()) {
                GltfImage &picture = model.images.back();
                picture.indices = DecodeIndices4(memory, key);
                picture.palette.assign(
                    memory.begin() + static_cast<std::ptrdiff_t>(*key.palette),
                    memory.begin() + static_cast<std::ptrdiff_t>(*key.palette + PALETTE_4_BYTES));
                picture.paintMaterial = painted->second;
            }
        }
        return imageOf[key];
    };
    std::vector<std::optional<std::size_t>> texture1Of(walk.Materials().size());
    for (std::size_t m = 0; m < walk.Materials().size(); ++m) {
        if (const auto &key = walk.Materials()[m].texture1) {
            texture1Of[m] = imageFor(*key);
        }
        const auto &key = walk.Materials()[m].texture;
        if (!key) {
            continue;
        }
        textureOf[m] = imageFor(*key);
    }

    // An image of each frame of an animated texture, and the frames by the image of the texture.
    std::map<std::size_t, nlohmann::ordered_json> animationOf;
    std::vector<std::uint8_t> scratch = streamed;
    for (const auto &[key, index] : std::map<TextureKey, std::size_t>(imageOf)) {
        auto texture = std::ranges::find_if(streamedTextures, [&](const StreamedTexture &found) {
            return !found.frames.empty() && found.texels - layout.base == key.address;
        });
        if (texture == streamedTextures.end()) {
            continue;
        }
        nlohmann::ordered_json frames = nlohmann::ordered_json::array();
        for (std::size_t f = 0; f < texture->frames.size(); ++f) {
            const AnimationFrame &frame = texture->frames[f];
            CopyFrame(image, layout, *texture, &frame, scratch);
            frames.push_back({{"time",
                               frame.time == ROM_NONE ? nlohmann::ordered_json(nullptr) :
                                                        nlohmann::ordered_json(frame.time)},
                              {"image", model.images.size()}});
            model.images.push_back({fmt::format("{}-{}-{}.png", stem, index, f),
                                    static_cast<int>(key.width),
                                    static_cast<int>(key.height),
                                    DecodeTexture(scratch, key),
                                    {},
                                    {},
                                    -1,
                                    key.address});
            // The next frame starts from the texture as a race starts.
            for (auto [target, size] :
                 {std::pair{texture->texels, texture->size},
                  std::pair{texture->palette, texture->palette ? texture->paletteSize : 0}}) {
                std::size_t at = target - layout.base;
                if (size != 0 && target >= layout.base && at + size <= scratch.size()) {
                    std::copy_n(streamed.begin() + static_cast<std::ptrdiff_t>(at),
                                size,
                                scratch.begin() + static_cast<std::ptrdiff_t>(at));
                }
            }
        }
        animationOf[index] = {{"snow", texture->snow}, {"frames", std::move(frames)}};
    }

    // The vertices a viewer draws: each record once for each texture coordinate the materials that
    // draw it give it. A lit vertex has a normal of signed bytes where an unlit one has its colour.
    std::vector<float> drawnPositions;
    std::vector<float> drawnNormals;
    std::vector<float> drawnUv;
    std::vector<std::uint8_t> drawnColors;
    std::map<std::tuple<std::uint32_t, float, float>, std::uint32_t> drawnOf;
    auto drawnVertex = [&](std::uint32_t corner, const Material &material) {
        std::uint32_t record = indexOf.at(corner);
        float u = 0.0f;
        float v = 0.0f;
        if (material.texture) {
            double s = static_cast<std::int16_t>(Big16(image, corner + 8)) / 32.0 *
                           material.scaleS * ShiftScale(material.shiftS) -
                       material.uls / 4.0;
            double t = static_cast<std::int16_t>(Big16(image, corner + 10)) / 32.0 *
                           material.scaleT * ShiftScale(material.shiftT) -
                       material.ult / 4.0;
            u = static_cast<float>(s / material.texture->width);
            v = static_cast<float>(t / material.texture->height);
        }
        auto [found, added] =
            drawnOf.try_emplace({record, u, v}, static_cast<std::uint32_t>(drawnOf.size()));
        if (added) {
            float length = 0.0f;
            std::array<float, 3> normal{};
            for (std::size_t axis = 0; axis < 3; ++axis) {
                drawnPositions.push_back(columns.positions[record * 3 + axis]);
                normal[axis] = static_cast<std::int8_t>(columns.colors[record * 4 + axis]);
                length += normal[axis] * normal[axis];
            }
            length = std::sqrt(length);
            for (std::size_t axis = 0; axis < 3; ++axis) {
                drawnNormals.push_back(length > 0.0f ? normal[axis] / length :
                                                       (axis == 2 ? 1.0f : 0.0f));
            }
            for (std::size_t i = 0; i < 4; ++i) {
                drawnColors.push_back(columns.colors[record * 4 + i]);
            }
            drawnUv.push_back(u);
            drawnUv.push_back(v);
        }
        return found->second;
    };
    // The triangles of each display list by material, in the order the list first draws each.
    std::map<std::pair<std::uint32_t, std::size_t>,
             std::vector<std::pair<std::size_t, std::vector<std::uint32_t>>>>
        primitivesOf;
    for (const auto &[key, triangles] : lists) {
        auto &primitives = primitivesOf[key];
        for (const Triangle &triangle : triangles) {
            auto primitive =
                std::ranges::find(primitives,
                                  triangle.material,
                                  &std::pair<std::size_t, std::vector<std::uint32_t>>::first);
            if (primitive == primitives.end()) {
                primitives.push_back({triangle.material, {}});
                primitive = std::prev(primitives.end());
            }
            for (std::uint32_t corner : triangle.corners) {
                primitive->second.push_back(
                    drawnVertex(corner, walk.Materials()[triangle.material]));
            }
        }
    }

    model.json = {{"asset", {{"version", "2.0"}, {"generator", "coastline"}}},
                  {"buffers", nlohmann::ordered_json::array()},
                  {"bufferViews", nlohmann::ordered_json::array()},
                  {"accessors", nlohmann::ordered_json::array()},
                  {"materials", nlohmann::ordered_json::array()},
                  {"meshes", nlohmann::ordered_json::array()},
                  {"nodes", nlohmann::ordered_json::array()},
                  {"scenes", nlohmann::ordered_json::array()},
                  {"scene", 0}};
    std::array<float, 3> low{columns.positions[0], columns.positions[1], columns.positions[2]};
    std::array<float, 3> high = low;
    for (std::size_t i = 0; i < columns.positions.size(); ++i) {
        low[i % 3] = std::min(low[i % 3], columns.positions[i]);
        high[i % 3] = std::max(high[i % 3], columns.positions[i]);
    }
    nlohmann::ordered_json attributes{
        {"POSITION",
         AddAccessor(model,
                     {{"bufferView",
                       AddView(model,
                               columns.positions.data(),
                               columns.positions.size() * sizeof(float),
                               ARRAY_BUFFER)},
                      {"componentType", FLOAT},
                      {"count", vertexCount},
                      {"type", "VEC3"},
                      {"min", low},
                      {"max", high}})},
        {"_FLAG",
         AddAccessor(model,
                     {{"bufferView",
                       AddView(model,
                               columns.flags.data(),
                               columns.flags.size() * sizeof(std::uint16_t),
                               ARRAY_BUFFER,
                               FLAG_STRIDE)},
                      {"componentType", UNSIGNED_SHORT},
                      {"count", vertexCount},
                      {"type", "SCALAR"}})},
        {"_TEXCOORD_S10_5",
         AddAccessor(model,
                     {{"bufferView",
                       AddView(model,
                               columns.textureCoordinates.data(),
                               columns.textureCoordinates.size() * sizeof(std::int16_t),
                               ARRAY_BUFFER)},
                      {"componentType", SHORT},
                      {"count", vertexCount},
                      {"type", "VEC2"}})},
        {"_COLOR_OR_NORMAL",
         AddAccessor(model,
                     {{"bufferView",
                       AddView(model, columns.colors.data(), columns.colors.size(), ARRAY_BUFFER)},
                      {"componentType", UNSIGNED_BYTE},
                      {"count", vertexCount},
                      {"type", "VEC4"}})}};

    // The attributes of the vertices a viewer draws, derived from the records, which a rebuild
    // does not read.
    std::size_t drawnCount = drawnOf.size();
    nlohmann::ordered_json drawn{
        {"POSITION",
         AddAccessor(model,
                     {{"bufferView",
                       AddView(model,
                               drawnPositions.data(),
                               drawnPositions.size() * sizeof(float),
                               ARRAY_BUFFER)},
                      {"componentType", FLOAT},
                      {"count", drawnCount},
                      {"type", "VEC3"},
                      {"min", low},
                      {"max", high}})},
        {"TEXCOORD_0",
         AddAccessor(
             model,
             {{"bufferView",
               AddView(model, drawnUv.data(), drawnUv.size() * sizeof(float), ARRAY_BUFFER)},
              {"componentType", FLOAT},
              {"count", drawnCount},
              {"type", "VEC2"}})}};
    nlohmann::ordered_json lit = drawn;
    lit["NORMAL"] = AddAccessor(
        model,
        {{"bufferView",
          AddView(model, drawnNormals.data(), drawnNormals.size() * sizeof(float), ARRAY_BUFFER)},
         {"componentType", FLOAT},
         {"count", drawnCount},
         {"type", "VEC3"}});
    nlohmann::ordered_json shaded = drawn;
    shaded["COLOR_0"] = AddAccessor(
        model,
        {{"bufferView", AddView(model, drawnColors.data(), drawnColors.size(), ARRAY_BUFFER)},
         {"componentType", UNSIGNED_BYTE},
         {"normalized", true},
         {"count", drawnCount},
         {"type", "VEC4"}});

    // A morph target for each key of each vertex animation: the displacement of the position and
    // the normal of every drawn vertex of a record the animation moves, from its rest.
    std::multimap<std::uint32_t, std::uint32_t> drawnOfRecord;
    for (const auto &[drawnKey, index] : drawnOf) {
        drawnOfRecord.emplace(std::get<0>(drawnKey), index);
    }
    auto sparseAccessor =
        [&](const std::vector<std::pair<std::uint32_t, std::array<float, 3>>> &moved) {
            std::vector<std::uint32_t> indices;
            std::vector<float> values;
            std::array<float, 3> low{0.0f, 0.0f, 0.0f};
            std::array<float, 3> high{0.0f, 0.0f, 0.0f};
            for (const auto &[index, value] : moved) {
                indices.push_back(index);
                for (std::size_t axis = 0; axis < 3; ++axis) {
                    values.push_back(value[axis]);
                    low[axis] = std::min(low[axis], value[axis]);
                    high[axis] = std::max(high[axis], value[axis]);
                }
            }
            nlohmann::ordered_json accessor{
                {"componentType", FLOAT}, {"count", drawnCount}, {"type", "VEC3"}};
            accessor["min"] = low;
            accessor["max"] = high;
            if (!moved.empty()) {
                accessor["sparse"] = {
                    {"count", moved.size()},
                    {"indices",
                     {{"bufferView",
                       AddView(model, indices.data(), indices.size() * sizeof(std::uint32_t))},
                      {"componentType", UNSIGNED_INT}}},
                    {"values",
                     {{"bufferView",
                       AddView(model, values.data(), values.size() * sizeof(float))}}}};
            }
            return AddAccessor(model, std::move(accessor));
        };
    std::vector<std::pair<std::size_t, std::size_t>> positionAndNormalTargets;
    std::vector<std::size_t> firstTargetOf;
    for (const VertexAnimation &animation : animations) {
        firstTargetOf.push_back(positionAndNormalTargets.size());
        for (const VertexKey &key : animation.keys) {
            std::vector<std::pair<std::uint32_t, std::array<float, 3>>> positions;
            std::vector<std::pair<std::uint32_t, std::array<float, 3>>> normals;
            std::size_t normalsAt = key.positions + animation.vertexCount * 6;
            for (std::size_t vertex = 0; vertex < animation.vertexCount; ++vertex) {
                auto record = indexOf.find(
                    static_cast<std::uint32_t>(animation.vertices + vertex * VERTEX_SIZE));
                std::size_t at = key.positions + vertex * 6;
                if (record == indexOf.end() || normalsAt + vertex * 3 + 3 > image.size()) {
                    continue;
                }
                std::array<float, 3> position{};
                std::array<float, 3> normal{};
                float length = 0.0f;
                for (std::size_t axis = 0; axis < 3; ++axis) {
                    position[axis] =
                        static_cast<float>(static_cast<std::int16_t>(Big16(image, at + axis * 2)));
                    normal[axis] = static_cast<float>(
                        static_cast<std::int8_t>(image[normalsAt + vertex * 3 + axis]));
                    length += normal[axis] * normal[axis];
                }
                length = std::sqrt(length);
                auto [first, last] = drawnOfRecord.equal_range(record->second);
                for (auto it = first; it != last; ++it) {
                    std::array<float, 3> moved{};
                    std::array<float, 3> turned{};
                    for (std::size_t axis = 0; axis < 3; ++axis) {
                        moved[axis] = position[axis] - drawnPositions[it->second * 3 + axis];
                        turned[axis] = (length > 0.0f ? normal[axis] / length : 0.0f) -
                                       drawnNormals[it->second * 3 + axis];
                    }
                    positions.push_back({it->second, moved});
                    normals.push_back({it->second, turned});
                }
            }
            std::ranges::sort(
                positions, {}, &std::pair<std::uint32_t, std::array<float, 3>>::first);
            std::ranges::sort(normals, {}, &std::pair<std::uint32_t, std::array<float, 3>>::first);
            positionAndNormalTargets.push_back(
                {sparseAccessor(positions), sparseAccessor(normals)});
        }
    }
    model.json["images"] = nlohmann::ordered_json::array();
    model.json["samplers"] = nlohmann::ordered_json::array();
    model.json["textures"] = nlohmann::ordered_json::array();
    for (const GltfImage &picture : model.images) {
        nlohmann::ordered_json entry{{"uri", picture.uri}};
        if (picture.paintMaterial >= 0) {
            entry["extras"]["coastlinePaint"] = {{"material", picture.paintMaterial}};
        }
        model.json["images"].push_back(std::move(entry));
    }
    // A texture for each image and wrap the materials sample it with, and a sampler for each wrap.
    std::map<std::pair<int, int>, std::size_t> samplerOf;
    std::map<std::tuple<std::size_t, int, int>, std::size_t> gltfTextureOf;
    auto gltfTexture = [&](std::size_t picture, const Material &material) {
        int wrapS = WrapMode(material.clampMirrorS, material.tileWidth, material.texture->width);
        int wrapT = WrapMode(material.clampMirrorT, material.tileHeight, material.texture->height);
        auto [sampler, newSampler] = samplerOf.try_emplace({wrapS, wrapT}, samplerOf.size());
        if (newSampler) {
            model.json["samplers"].push_back(
                {{"magFilter", LINEAR}, {"minFilter", LINEAR}, {"wrapS", wrapS}, {"wrapT", wrapT}});
        }
        auto [texture, newTexture] =
            gltfTextureOf.try_emplace({picture, wrapS, wrapT}, gltfTextureOf.size());
        if (newTexture) {
            model.json["textures"].push_back({{"source", picture}, {"sampler", sampler->second}});
        }
        return texture->second;
    };
    for (std::size_t m = 0; m < walk.Materials().size(); ++m) {
        const Material &source = walk.Materials()[m];
        nlohmann::ordered_json material{
            {"name", source.name.empty() ? fmt::format("material{}", m) : source.name},
            {"pbrMetallicRoughness", {{"metallicFactor", 0.0}, {"roughnessFactor", 1.0}}}};
        // The state of the RDP a renderer draws the material with.
        nlohmann::ordered_json rdp{{"combine", {source.combineHigh, source.combineLow}},
                                   {"otherModeH", source.otherModeH},
                                   {"otherModeL", source.otherModeL},
                                   {"geometryMode", source.geometryMode},
                                   {"primColor", source.primColor},
                                   {"envColor", source.envColor},
                                   {"tile",
                                    {{"clampMirrorS", source.clampMirrorS},
                                     {"clampMirrorT", source.clampMirrorT},
                                     {"shiftS", source.shiftS},
                                     {"shiftT", source.shiftT},
                                     {"uls", source.uls},
                                     {"ult", source.ult},
                                     {"width", source.tileWidth},
                                     {"height", source.tileHeight}}}};
        if (texture1Of[m]) {
            rdp["texel1Image"] = *texture1Of[m];
        }
        material["extras"]["coastlineRdp"] = std::move(rdp);
        if (source.color) {
            // glTF takes the base colour in linear light, and the console colour is sRGB.
            nlohmann::ordered_json factor = nlohmann::ordered_json::array();
            for (std::size_t i = 0; i < 4; ++i) {
                double value = (*source.color)[i] / 255.0;
                factor.push_back(i == 3           ? value :
                                 value <= 0.04045 ? value / 12.92 :
                                                    std::pow((value + 0.055) / 1.055, 2.4));
            }
            material["pbrMetallicRoughness"]["baseColorFactor"] = std::move(factor);
        }
        if (source.twoSided) {
            material["doubleSided"] = true;
        }
        if (textureOf[m]) {
            material["pbrMetallicRoughness"]["baseColorTexture"] = {
                {"index", gltfTexture(*textureOf[m], source)}};
            // The frames the texture of the material shows, each from its time in milliseconds,
            // the frames of a texture of the snow with no time.
            if (auto animation = animationOf.find(*textureOf[m]); animation != animationOf.end()) {
                material["extras"]["coastlineAnimation"] = animation->second;
            }
        }
        if (source.alpha == Alpha::Blend) {
            material["alphaMode"] = "BLEND";
        } else if (source.alpha == Alpha::Cut) {
            material["alphaMode"] = "MASK";
        }
        model.json["materials"].push_back(std::move(material));
    }

    // A mesh for each display list, with a primitive for each material in the order the list
    // first draws it.
    std::map<std::pair<std::uint32_t, std::size_t>, std::optional<std::size_t>> meshOf;
    auto addMesh = [&](std::pair<std::uint32_t, std::size_t> key, const std::string &name) {
        if (lists.at(key).empty()) {
            meshOf[key] = std::nullopt;
            return;
        }
        nlohmann::ordered_json primitives = nlohmann::ordered_json::array();
        for (const auto &[material, list] : primitivesOf.at(key)) {
            std::size_t accessor = AddAccessor(model,
                                               {{"bufferView",
                                                 AddView(model,
                                                         list.data(),
                                                         list.size() * sizeof(std::uint32_t),
                                                         ELEMENT_ARRAY_BUFFER)},
                                                {"componentType", UNSIGNED_INT},
                                                {"count", list.size()},
                                                {"type", "SCALAR"}});
            bool litMaterial = walk.Materials()[material].lit;
            nlohmann::ordered_json primitive{{"attributes", litMaterial ? lit : shaded},
                                             {"indices", accessor},
                                             {"material", material},
                                             {"mode", TRIANGLES}};
            if (!positionAndNormalTargets.empty()) {
                nlohmann::ordered_json targets = nlohmann::ordered_json::array();
                for (const auto &[position, normal] : positionAndNormalTargets) {
                    nlohmann::ordered_json target{{"POSITION", position}};
                    if (litMaterial) {
                        target["NORMAL"] = normal;
                    }
                    targets.push_back(std::move(target));
                }
                primitive["targets"] = std::move(targets);
            }
            primitives.push_back(std::move(primitive));
        }
        nlohmann::ordered_json mesh{{"name", name}, {"primitives", std::move(primitives)}};
        if (!positionAndNormalTargets.empty()) {
            mesh["weights"] = std::vector<float>(positionAndNormalTargets.size(), 0.0f);
        }
        model.json["meshes"].push_back(std::move(mesh));
        meshOf[key] = model.json["meshes"].size() - 1;
    };

    std::vector<nlohmann::ordered_json> children(scenes.size(), nlohmann::ordered_json::array());
    for (const Draw &draw : draws) {
        auto key = keyOf(draw);
        if (!meshOf.contains(key)) {
            addMesh(key, draw.name);
        }
        if (!meshOf[key]) {
            continue;
        }
        for (const Placement &placement : draw.placements) {
            nlohmann::ordered_json node{{"name", placement.name}, {"mesh", *meshOf[key]}};
            for (const auto &[field, value] : placement.transform.items()) {
                node[field] = value;
            }
            model.json["nodes"].push_back(std::move(node));
            if (draw.scene) {
                children[*draw.scene].push_back(model.json["nodes"].size() - 1);
            }
        }
    }
    for (std::size_t scene = 0; scene < scenes.size(); ++scene) {
        // The console is z up and glTF is y up.
        model.json["nodes"].push_back({{"name", scenes[scene]},
                                       {"rotation", {-0.70710678f, 0.0f, 0.0f, 0.70710678f}},
                                       {"children", std::move(children[scene])}});
        model.json["scenes"].push_back(
            {{"name", scenes[scene]}, {"nodes", {model.json["nodes"].size() - 1}}});
    }

    // An animation of the weights of every node with a mesh for each vertex animation, from each
    // key to the next. Its flags and its span of time are how the game plays it.
    if (!animations.empty()) {
        model.json["animations"] = nlohmann::ordered_json::array();
    }
    for (std::size_t a = 0; a < animations.size(); ++a) {
        const VertexAnimation &animation = animations[a];
        if (animation.keys.empty()) {
            continue;
        }
        std::vector<float> times;
        std::vector<float> weights;
        for (std::size_t k = 0; k < animation.keys.size(); ++k) {
            times.push_back(animation.keys[k].time);
            for (std::size_t t = 0; t < positionAndNormalTargets.size(); ++t) {
                weights.push_back(t == firstTargetOf[a] + k ? 1.0f : 0.0f);
            }
        }
        std::size_t input =
            AddAccessor(model,
                        {{"bufferView", AddView(model, times.data(), times.size() * sizeof(float))},
                         {"componentType", FLOAT},
                         {"count", times.size()},
                         {"type", "SCALAR"},
                         {"min", {*std::ranges::min_element(times)}},
                         {"max", {*std::ranges::max_element(times)}}});
        std::size_t output = AddAccessor(
            model,
            {{"bufferView", AddView(model, weights.data(), weights.size() * sizeof(float))},
             {"componentType", FLOAT},
             {"count", weights.size()},
             {"type", "SCALAR"}});
        nlohmann::ordered_json channels = nlohmann::ordered_json::array();
        for (std::size_t node = 0; node < model.json["nodes"].size(); ++node) {
            if (model.json["nodes"][node].contains("mesh")) {
                channels.push_back(
                    {{"sampler", 0}, {"target", {{"node", node}, {"path", "weights"}}}});
            }
        }
        model.json["animations"].push_back(
            {{"name", fmt::format("vertices{}", a)},
             {"samplers", {{{"input", input}, {"interpolation", "LINEAR"}, {"output", output}}}},
             {"channels", std::move(channels)},
             {"extras",
              {{"coastlineVertexAnimation",
                {{"flags", animation.flags},
                 {"startTime", animation.startTime},
                 {"endTime", animation.endTime},
                 {"time", animation.time},
                 {"firstTarget", firstTargetOf[a]},
                 {"keyCount", animation.keys.size()}}}}}});
    }

    std::size_t residualView = AddView(model, residual.data(), residual.size());
    model.json["extras"] = {{"coastlineModel",
                             {{"imageOffset", imageOffset},
                              {"vertexRuns", std::move(runs)},
                              {"residualBufferView", residualView},
                              {"attributes", attributes}}},
                            {"coastlineCoverage", walk.Coverage()}};
    model.json["buffers"].push_back({{"uri", bufferUri}, {"byteLength", model.buffer.size()}});
    return model;
}

std::optional<std::span<const std::uint8_t>> View(const nlohmann::ordered_json &json,
                                                  std::span<const std::uint8_t> buffer,
                                                  std::size_t accessorIndex,
                                                  std::size_t elementSize,
                                                  std::size_t count) {
    const auto &accessors = json.at("accessors");
    if (accessorIndex >= accessors.size()) {
        return std::nullopt;
    }
    const auto &accessor = accessors[accessorIndex];
    if (accessor.at("count").get<std::size_t>() != count) {
        return std::nullopt;
    }
    const auto &view = json.at("bufferViews").at(accessor.at("bufferView").get<std::size_t>());
    std::size_t offset =
        view.at("byteOffset").get<std::size_t>() + accessor.value("byteOffset", std::size_t{0});
    std::size_t size = elementSize * count;
    if (offset > buffer.size() || size > buffer.size() - offset) {
        return std::nullopt;
    }
    return buffer.subspan(offset, size);
}

// The states of the loads of a texture as PreprocessDisplayListTextures() of the PC release follows
// them.
enum class TextureScan {
    Idle,
    ImageSet,
    LoadSynced,
    BlockLoaded,
    TileSynced,
    TileSet,
    TileSized,
    TlutLoaded,
    PipeSynced
};

// A texture entry of the PC release: the addresses of its texels and its palette, and the colours
// of the combine of the PC release that tells entries of the same image apart.
struct ScannedTexture {
    std::uint32_t texels = 0;
    std::uint32_t palette = 0;
    bool colorCombine = false;
    std::uint32_t primitive = 0;
    std::uint32_t environment = 0;
};

// The paint set of each texture of a model of a car, by the offset of its texels in the image.
// PreparePaintTexture() of the PC release counts a set, from 1, for each texture entry the car
// creates: those PreprocessDisplayListTextures() creates over the display lists of the body and
// then of the wheels, each list once, then those of the textures 6, 3, and 5 of the table that
// FindMatchingTextureEntry() does not find among them.
std::map<std::size_t, int> PaintSets(std::span<const std::uint8_t> image) {
    std::vector<ScannedTexture> entries;
    std::map<std::size_t, int> sets;
    auto create = [&](const ScannedTexture &texture) {
        bool found = std::ranges::any_of(entries, [&](const ScannedTexture &entry) {
            return entry.texels == texture.texels && entry.palette == texture.palette &&
                   (!entry.colorCombine || !texture.colorCombine ||
                    (entry.primitive == texture.primitive &&
                     entry.environment == texture.environment));
        });
        if (found) {
            return;
        }
        entries.push_back(texture);
        if (texture.texels >= CAR_LAYOUT.base) {
            sets.try_emplace(texture.texels - CAR_LAYOUT.base, static_cast<int>(entries.size()));
        }
    };
    std::vector<std::uint32_t> lists;
    for (int lod = 0; lod < LODS; ++lod) {
        for (int list = 0; list < BODY_LISTS; ++list) {
            lists.push_back(Big32(image, BODY_LISTS_AT + (lod * BODY_LISTS + list) * 4));
        }
    }
    for (int lod = 0; lod < LODS; ++lod) {
        for (int list = 0; list < WHEEL_LISTS; ++list) {
            lists.push_back(Big32(image, WHEEL_LISTS_AT + (lod * WHEEL_LISTS + list) * 4));
        }
    }
    // The source of the last block loaded and the colours persist from list to list.
    std::uint32_t imageAddress = 0;
    std::uint32_t blockSource = 0;
    std::uint32_t primitive = 0;
    std::uint32_t environment = 0;
    std::set<std::uint32_t> scanned;
    for (std::uint32_t list : lists) {
        if (list == 0 || !scanned.insert(list).second) {
            continue;
        }
        TextureScan state = TextureScan::Idle;
        std::uint32_t tlut = 0;
        bool colorCombine = false;
        for (std::size_t at = list - CAR_LAYOUT.base; at + 8 <= image.size(); at += 8) {
            std::uint32_t w0 = Big32(image, at);
            std::uint32_t w1 = Big32(image, at + 4);
            std::uint32_t opcode = w0 >> 24;
            if (opcode == OP_ENDDL) {
                break;
            }
            switch (opcode) {
            case OP_VTX:
            case OP_TRI2:
            case OP_TRI1:
                if (state != TextureScan::Idle) {
                    create({blockSource, tlut, colorCombine, primitive, environment});
                    state = TextureScan::Idle;
                }
                break;
            case OP_RDPLOADSYNC:
                if (state == TextureScan::ImageSet) {
                    state = TextureScan::LoadSynced;
                }
                break;
            case OP_RDPPIPESYNC:
                if (state == TextureScan::TlutLoaded) {
                    state = TextureScan::PipeSynced;
                }
                break;
            case OP_RDPTILESYNC:
                if (state == TextureScan::BlockLoaded || state == TextureScan::TlutLoaded) {
                    state = TextureScan::TileSynced;
                }
                break;
            case OP_LOADTLUT:
                if (state == TextureScan::ImageSet) {
                    tlut = imageAddress;
                    state = TextureScan::TlutLoaded;
                }
                break;
            case OP_SETTILESIZE:
                state = TextureScan::TileSized;
                break;
            case OP_LOADBLOCK:
                if (state == TextureScan::LoadSynced) {
                    blockSource = imageAddress;
                    state = TextureScan::BlockLoaded;
                }
                break;
            case OP_SETTILE:
                state = TextureScan::TileSet;
                break;
            case OP_SETPRIMCOLOR:
                primitive = w1;
                break;
            case OP_SETENVCOLOR:
                environment = w1;
                break;
            case OP_SETCOMBINE:
                colorCombine = w0 == 0xfc50fe04 && w1 == 0x3ffdf3f8;
                break;
            case OP_SETTIMG:
                if (state == TextureScan::Idle || state == TextureScan::BlockLoaded ||
                    state == TextureScan::TileSized) {
                    imageAddress = w1;
                    tlut = 0;
                    state = TextureScan::ImageSet;
                }
                break;
            default:
                break;
            }
        }
    }
    // CreateDeviceTextureFromImageHeader() passes over a texture without texels or with the
    // flag 0x100000.
    std::uint32_t table = Big32(image, CAR_LAYOUT.texturesAt) - CAR_LAYOUT.base;
    for (std::size_t entry : {6, 3, 5}) {
        std::size_t at = table + entry * TEXTURE_SIZE;
        if (table > image.size() || at + TEXTURE_SIZE > image.size()) {
            break;
        }
        std::uint32_t texels = Big32(image, at);
        if (texels != 0 && (Big32(image, at + 0x20) & 0x100000) == 0) {
            create({texels, Big32(image, at + 4), false, 0, 0});
        }
    }
    return sets;
}

} // namespace

std::expected<GltfModel, std::string> CarModelToGltf(std::span<const std::uint8_t> bytes,
                                                     std::size_t imageOffset,
                                                     std::string_view bufferUri) {
    if (bytes.size() < imageOffset + CAR_HEADER_END) {
        return std::unexpected("the file is too short for a model of a car");
    }
    auto image = bytes.subspan(imageOffset);
    nlohmann::ordered_json scale{{"scale", {CAR_SCALE, CAR_SCALE, CAR_SCALE}}};
    std::vector<Draw> draws;
    std::vector<std::string> scenes;
    for (int lod = 0; lod < LODS; ++lod) {
        scenes.push_back(fmt::format("lod{}", lod));
        for (int list = 0; list < BODY_LISTS; ++list) {
            std::uint32_t address = Big32(image, BODY_LISTS_AT + (lod * BODY_LISTS + list) * 4);
            if (address == 0) {
                continue;
            }
            auto name = fmt::format("lod{}-{}", lod, BODY_LIST_NAMES[list]);
            // The highlight adds to the colours under it, and the beams draw at night or in the
            // rain. A viewer of the scene omits them.
            bool shown = list != VEHICLE_DL_HIGHLIGHTED && list != VEHICLE_DL_HEADLIGHT_BEAMS;
            // The renderer turns on lighting before the lists of a car, and loads a texture of the
            // model before the underside and the near detail.
            StartState start{
                .lit = true, .twoSided = false, .bound = std::nullopt, .shadePalette = true};
            // The paint and the windows reflect the sky: the renderer generates their texture
            // coordinates from the normals and blends the reflection towards the fog.
            if (list == VEHICLE_DL_BODY) {
                start.combine = REFLECTION_COMBINE;
                start.textureGen = true;
            }
            if (list == VEHICLE_DL_UNDERSIDE) {
                start.bound = TableTexture(image, CAR_LAYOUT, UNDERSIDE_TEXTURE);
            } else if (list == VEHICLE_DL_NEAR_DETAIL) {
                start.bound = TableTexture(image, CAR_LAYOUT, NEAR_DETAIL_TEXTURE);
            }
            draws.push_back({name,
                             address,
                             {{name, scale}},
                             shown ? std::optional<std::size_t>(lod) : std::nullopt,
                             start});
        }
        for (int list = 0; list < WHEEL_LISTS; ++list) {
            std::uint32_t address = Big32(image, WHEEL_LISTS_AT + (lod * WHEEL_LISTS + list) * 4);
            if (address == 0) {
                continue;
            }
            // The mesh of a wheel is drawn at each of the four places of the wheels.
            Draw draw{
                fmt::format("lod{}-{}", lod, WHEEL_LIST_NAMES[list]),
                address,
                {},
                lod,
                {.lit = true, .twoSided = false, .bound = std::nullopt, .shadePalette = true}};
            for (int place = 0; place < WHEELS; ++place) {
                std::array<float, 3> at{};
                for (int axis = 0; axis < 3; ++axis) {
                    at[static_cast<std::size_t>(axis)] = std::bit_cast<float>(
                        Big32(image, WHEELS_AT + static_cast<std::size_t>(place * 12 + axis * 4)));
                }
                nlohmann::ordered_json transform = scale;
                transform["translation"] = at;
                draw.placements.push_back(
                    {fmt::format("{}{}", draw.name, place), std::move(transform)});
            }
            draws.push_back(std::move(draw));
        }
    }
    // The textures whose palettes SetMaterialPaletteColors() paints, by their material. A texture
    // of more than one material takes the alpha of the last.
    std::map<std::size_t, int> paintMaterialOf;
    for (int material = 0; material < VEHICLE_MATERIALS; ++material) {
        auto texture = TableTexture(image, CAR_LAYOUT, image[MATERIAL_TEXTURES_AT + material]);
        if (texture) {
            paintMaterialOf[texture->address] = material;
        }
    }
    auto model =
        BuildModel(bytes, imageOffset, CAR_LAYOUT, draws, scenes, bufferUri, {}, paintMaterialOf);
    if (!model) {
        return model;
    }
    // The paint set of each image, which IncrementDamageTextureLevel() damages by the side of the
    // set.
    std::map<std::size_t, int> sets = PaintSets(image);
    for (std::size_t i = 0; i < model->images.size(); ++i) {
        auto set = sets.find(model->images[i].address);
        if (set != sets.end()) {
            model->json["images"][i]["extras"]["coastlinePaint"]["set"] = set->second;
        }
    }
    return model;
}

std::expected<GltfModel, std::string> TrackToGltf(std::span<const std::uint8_t> bytes,
                                                  std::string_view bufferUri) {
    if (bytes.size() < TRACK_HEADER_END) {
        return std::unexpected("the file is too short for a track");
    }
    const std::uint32_t base = TRACK_LAYOUT.base;
    std::uint32_t count = Big32(bytes, TRACK_INSTANCE_COUNT_AT);
    std::uint32_t instances = Big32(bytes, TRACK_INSTANCES_AT) - base;
    if (instances > bytes.size() || count > (bytes.size() - instances) / TRACK_INSTANCE_SIZE) {
        return std::unexpected("the placed models of the track are outside the file");
    }
    std::uint32_t names = Big32(bytes, TRACK_INSTANCE_NAMES_AT);
    auto nameOf = [&](std::uint32_t index) {
        // The name of a placed model, where the track lists names.
        if (names != 0 && names - base <= bytes.size() - 4 * (index + 1)) {
            std::uint32_t text = Big32(bytes, names - base + 4 * index);
            if (text >= base && text - base < bytes.size()) {
                std::string name;
                for (std::size_t at = text - base; at < bytes.size() && bytes[at] != 0; ++at) {
                    name.push_back(static_cast<char>(bytes[at]));
                }
                if (!name.empty()) {
                    return fmt::format("{}-{}", index, name);
                }
            }
        }
        return fmt::format("instance{}", index);
    };
    std::vector<Draw> draws;
    for (std::uint32_t i = 0; i < count; ++i) {
        std::size_t at = instances + i * TRACK_INSTANCE_SIZE;
        std::uint32_t displayList = Big32(bytes, at + TRACK_INSTANCE_DISPLAY_LIST_AT);
        if (displayList == 0) {
            continue;
        }
        // The rows of the matrix of the console are the columns of glTF.
        nlohmann::ordered_json matrix = nlohmann::ordered_json::array();
        for (std::size_t k = 0; k < 16; ++k) {
            matrix.push_back(std::bit_cast<float>(Big32(bytes, at + k * 4)));
        }
        auto name = nameOf(i);
        // The renderer lights the placed models, and draws a two-sided one without culling.
        std::uint16_t flags = Big16(bytes, at + TRACK_INSTANCE_FLAGS_AT);
        bool twoSided = (flags & TRACK_INSTANCE_TWO_SIDED) != 0;
        // The flags choose the light of the model, its fog, and its shading as the race draws it.
        draws.push_back(
            {name,
             displayList,
             {{name,
               {{"matrix", matrix},
                {"extras", {{"coastlineInstance", {{"flags", flags}, {"index", i}}}}}}}},
             0,
             {.lit = true, .twoSided = twoSided, .bound = std::nullopt}});
    }
    if (std::uint32_t sky = Big32(bytes, TRACK_SKY_AT); sky != 0) {
        // RenderSkybox() sets the colour of the texels and the alpha of the shade before the sky.
        draws.push_back(
            {"sky",
             sky,
             {{"sky", nlohmann::ordered_json::object()}},
             std::nullopt,
             {.lit = false, .twoSided = false, .bound = std::nullopt, .combine = SKY_COMBINE}});
    }
    auto model = BuildModel(bytes, 0, TRACK_LAYOUT, draws, {"track"}, bufferUri);
    if (model) {
        // The fog colour of the track, which the race also tints the lights of its weathers by.
        model->json["extras"]["coastlineTrack"] = {{"fogColor",
                                                    {bytes[TRACK_FOG_COLOR_AT],
                                                     bytes[TRACK_FOG_COLOR_AT + 1],
                                                     bytes[TRACK_FOG_COLOR_AT + 2]}}};
    }
    return model;
}

std::expected<GltfModel, std::string> FrontEndModelToGltf(std::span<const std::uint8_t> bytes,
                                                          std::string_view bufferUri) {
    // The offsets of a model start from its first byte, and its textures are where its display
    // lists load them from.
    constexpr Layout MODEL_LAYOUT{0, 0, 0, false};
    constexpr std::size_t PARTS_AT = 8;
    constexpr std::size_t PART_SIZE = 20;
    constexpr std::uint16_t PART_TWO_SIDED = 0x0004;
    if (bytes.size() < PARTS_AT) {
        return std::unexpected("the file is too short for a model");
    }
    std::uint32_t partCount = Big32(bytes, 0);
    if (partCount > (bytes.size() - PARTS_AT) / PART_SIZE) {
        return std::unexpected("the parts of the model are outside the file");
    }
    std::vector<Draw> draws;
    for (std::uint32_t i = 0; i < partCount; ++i) {
        std::size_t at = PARTS_AT + i * PART_SIZE;
        std::uint32_t displayList = Big32(bytes, at);
        std::uint16_t flags = Big16(bytes, at + 4);
        // A part with no display list has an offset of 0.
        if (displayList == 0) {
            continue;
        }
        nlohmann::ordered_json translation = nlohmann::ordered_json::array();
        for (std::size_t axis = 0; axis < 3; ++axis) {
            translation.push_back(std::bit_cast<float>(Big32(bytes, at + 8 + axis * 4)));
        }
        auto name = fmt::format("part{}", i);
        draws.push_back(
            {name,
             displayList,
             {{name,
               {{"translation", translation},
                {"extras", {{"coastlinePart", {{"flags", flags}}}}}}}},
             0,
             {.lit = true, .twoSided = (flags & PART_TWO_SIDED) != 0, .bound = std::nullopt}});
    }

    // The animations of the vertices: their count, then a word for each, each a record of the
    // vertices it moves, its keys, and how it plays.
    std::vector<VertexAnimation> animations;
    if (std::uint32_t list = Big32(bytes, 4); list != 0) {
        if (list > bytes.size() - 4) {
            return std::unexpected("the animations of the model are outside the file");
        }
        std::uint32_t count = Big32(bytes, list);
        for (std::uint32_t i = 0; i < count && list + 8 + i * 4 <= bytes.size(); ++i) {
            std::uint32_t at = Big32(bytes, list + 4 + i * 4);
            if (at > bytes.size() || bytes.size() - at < 0x24) {
                return std::unexpected("an animation of the model is outside the file");
            }
            VertexAnimation animation{.vertices = Big32(bytes, at + 4),
                                      .vertexCount = Big32(bytes, at),
                                      .flags = Big16(bytes, at + 0x10),
                                      .startTime = std::bit_cast<float>(Big32(bytes, at + 0x14)),
                                      .endTime = std::bit_cast<float>(Big32(bytes, at + 0x18)),
                                      .time = std::bit_cast<float>(Big32(bytes, at + 0x1c)),
                                      .keys = {}};
            std::uint32_t keyCount = Big32(bytes, at + 0xc);
            for (std::uint32_t k = 0; k < keyCount && at + 0x20 + k * 4 + 4 <= bytes.size(); ++k) {
                std::uint32_t key = Big32(bytes, at + 0x20 + k * 4);
                if (key > bytes.size() - 4) {
                    return std::unexpected("a key of an animation is outside the file");
                }
                animation.keys.push_back({std::bit_cast<float>(Big32(bytes, key)), key + 4});
            }
            animations.push_back(std::move(animation));
        }
    }
    return BuildModel(bytes, 0, MODEL_LAYOUT, draws, {"model"}, bufferUri, animations);
}

std::expected<std::vector<std::uint8_t>, std::string>
ModelFromGltf(const nlohmann::ordered_json &json, std::span<const std::uint8_t> buffer) {
    try {
        const auto &extras = json.at("extras").at("coastlineModel");
        auto imageOffset = extras.at("imageOffset").get<std::size_t>();
        const auto &view =
            json.at("bufferViews").at(extras.at("residualBufferView").get<std::size_t>());
        auto offset = view.at("byteOffset").get<std::size_t>();
        auto size = view.at("byteLength").get<std::size_t>();
        if (offset > buffer.size() || size > buffer.size() - offset) {
            return std::unexpected("the residual bytes are outside the buffer");
        }
        std::vector<std::uint8_t> file(buffer.begin() + static_cast<std::ptrdiff_t>(offset),
                                       buffer.begin() + static_cast<std::ptrdiff_t>(offset + size));
        std::size_t vertexCount = 0;
        for (const auto &run : extras.at("vertexRuns")) {
            vertexCount += run.at(1).get<std::size_t>();
        }
        const auto &attributes = extras.at("attributes");
        auto positions = View(json, buffer, attributes.at("POSITION"), 12, vertexCount);
        auto flags = View(json, buffer, attributes.at("_FLAG"), FLAG_STRIDE, vertexCount);
        auto coordinates = View(json, buffer, attributes.at("_TEXCOORD_S10_5"), 4, vertexCount);
        auto colors = View(json, buffer, attributes.at("_COLOR_OR_NORMAL"), 4, vertexCount);
        if (!positions || !flags || !coordinates || !colors) {
            return std::unexpected("the vertex attributes do not match the vertex runs");
        }
        auto put16 = [&file](std::size_t at, std::uint16_t value) {
            file[at] = static_cast<std::uint8_t>(value >> 8);
            file[at + 1] = static_cast<std::uint8_t>(value);
        };
        std::size_t vertex = 0;
        for (const auto &run : extras.at("vertexRuns")) {
            auto start = imageOffset + run.at(0).get<std::size_t>();
            auto count = run.at(1).get<std::size_t>();
            if (start > file.size() || count * VERTEX_SIZE > file.size() - start) {
                return std::unexpected("a vertex run is outside the model");
            }
            for (std::size_t i = 0; i < count; ++i, ++vertex) {
                std::size_t at = start + i * VERTEX_SIZE;
                for (std::size_t axis = 0; axis < 3; ++axis) {
                    float value;
                    std::memcpy(&value, positions->data() + vertex * 12 + axis * 4, 4);
                    // A model of the console places vertices at whole units.
                    float whole = std::round(value);
                    if (!(whole >= -32768.0f && whole <= 32767.0f)) {
                        return std::unexpected(
                            fmt::format("vertex {} is outside the range of the console", vertex));
                    }
                    put16(at + axis * 2,
                          static_cast<std::uint16_t>(static_cast<std::int16_t>(whole)));
                }
                std::uint16_t flag;
                std::memcpy(&flag, flags->data() + vertex * FLAG_STRIDE, 2);
                put16(at + 6, flag);
                for (std::size_t component = 0; component < 2; ++component) {
                    std::uint16_t value;
                    std::memcpy(&value, coordinates->data() + vertex * 4 + component * 2, 2);
                    put16(at + 8 + component * 2, value);
                }
                std::memcpy(file.data() + at + 12, colors->data() + vertex * 4, 4);
            }
        }
        return file;
    } catch (const nlohmann::json::exception &error) {
        return std::unexpected(
            fmt::format("the glTF does not describe a model of the console ({})", error.what()));
    }
}

} // namespace coastline
