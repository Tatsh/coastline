// The display lists the front end of the Nintendo 64 release builds, drawn as the RSP and the RDP
// of the console draw them. The texture memory, the tiles, and the colour combiner of the RDP are
// emulated. A texture rectangle is drawn as a texture of its pixels, each pixel sampled and
// combined as the RDP combines it. A triangle takes the texture of its tile, and the combiner at a
// white texel gives the colour of its vertices.

#include "n64displaylist.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <format>
#include <unordered_map>
#include <vector>

#include <PR/gbi.h>
#include <PR/gu.h>
#include <SDL3/SDL.h>

#include "screen.h"
#include "sdl/cartridgedata.h"
#include "sdl/d3dcompat.h"
#include "sdl/renderhost.h"
#include "sdl/tlrenderer.h"
#include "win32/displaylist.h"
#include "win32/render.h"

namespace {

constexpr std::size_t kTmemBytes = 4096;
constexpr std::size_t kVertexCount = 64;
constexpr std::size_t kDisplayListDepth = 32;
// The frames a baked texture is not drawn in before its handle is reused.
constexpr std::uint32_t kTextureIdleFrames = 120;

struct Color {
    float r = 0; /*!< The red, from 0 to 1. */
    float g = 0; /*!< The green, from 0 to 1. */
    float b = 0; /*!< The blue, from 0 to 1. */
    float a = 0; /*!< The alpha, from 0 to 1. */
};

struct Tile {
    int format = 0;      /*!< The G_IM_FMT_* of the texels. */
    int size = 0;        /*!< The G_IM_SIZ_* of the texels. */
    int line = 0;        /*!< The double words of a row in the texture memory. */
    int tmem = 0;        /*!< The double word of the texture memory the tile starts at. */
    int palette = 0;     /*!< The palette of 16 colours of a 4-bit tile. */
    int clampS = 0;      /*!< The G_TX_* clamp and mirror bits of s. */
    int clampT = 0;      /*!< The G_TX_* clamp and mirror bits of t. */
    int maskS = 0;       /*!< The bits of s that wrap, or 0. */
    int maskT = 0;       /*!< The bits of t that wrap, or 0. */
    int shiftS = 0;      /*!< The shift of s. */
    int shiftT = 0;      /*!< The shift of t. */
    int upperLeftS = 0;  /*!< The first s, in 10.2 fixed point. */
    int upperLeftT = 0;  /*!< The first t, in 10.2 fixed point. */
    int lowerRightS = 0; /*!< The last s, in 10.2 fixed point. */
    int lowerRightT = 0; /*!< The last t, in 10.2 fixed point. */
};

struct Vertex {
    float x = 0, y = 0, z = 0, w = 1; /*!< The position after the projection. */
    float s = 0, t = 0;               /*!< The texture coordinates, in texels of the tile. */
    Color shade;                      /*!< The colour of the vertex. */
};

using Matrix = std::array<std::array<float, 4>, 4>;

struct BakedTexture {
    coastline::TextureHandle handle = 0; /*!< The texture of the renderer. */
    std::uint32_t lastFrame = 0;         /*!< The frame the texture was drawn last in. */
    int width = 0;                       /*!< The width in texels. */
    int height = 0;                      /*!< The height in texels. */
};

struct Light {
    float color[3] = {};     /*!< The red, green, and blue, from 0 to 1. */
    float direction[3] = {}; /*!< The unit direction towards the light. */
};

struct RdpState {
    std::array<Light, 8> lights{};
    // The directional lights. The ambient light follows them.
    int lightCount = 1;
    std::array<std::uint8_t, kTmemBytes> tmem{};
    std::array<std::uint16_t, 256> tlut{};
    bool textureMemoryChanged = true;
    std::uint64_t textureMemoryHash = 0;
    std::array<Tile, 8> tiles{};
    const std::uint8_t *image = nullptr;
    int imageSize = 0;
    int imageWidth = 0;
    std::uint32_t combineHigh = 0;
    std::uint32_t combineLow = 0;
    std::uint32_t otherHigh = 0;
    std::uint32_t otherLow = 0;
    std::uint32_t geometry = 0;
    Color primitive, environment, blend, fog;
    float primitiveLodFraction = 0;
    std::uint32_t fill = 0;
    const void *colorImage = nullptr;
    const void *depthImage = nullptr;
    Matrix projection{};
    std::vector<Matrix> modelview;
    float viewportScale[3] = {160, 120, 0.5f};
    float viewportTranslate[3] = {160, 120, 0.5f};
    std::array<Vertex, kVertexCount> vertices{};
    float textureScaleS = 1;
    float textureScaleT = 1;
    int textureTile = 0;
    bool textureOn = false;
    std::unordered_map<std::uint64_t, BakedTexture> textures;
    std::vector<coastline::TextureHandle> idleHandles;
    std::uint32_t frame = 0;
};

RdpState g_rdp;
coastline::TlRenderer *g_renderer;
std::filesystem::path g_textureDirectory;
bool g_dumpTextures;

Matrix Identity() {
    Matrix m{};
    for (int i = 0; i < 4; ++i) {
        m[i][i] = 1;
    }
    return m;
}

Matrix Multiply(const Matrix &a, const Matrix &b) {
    Matrix r{};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 4; ++k) {
                r[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    return r;
}

// A matrix of the console in s15.16 fixed point. A matrix of the cartridge has big-endian words.
Matrix ReadMatrix(const void *source) {
    Mtx native;
    std::memcpy(&native, source, sizeof(native));
    if (coastline::CartridgeData::IsSegmentCopy(source)) {
        for (auto &row : native.m) {
            for (auto &word : row) {
                word = static_cast<s32>(__builtin_bswap32(static_cast<u32>(word)));
            }
        }
    }
    float floats[4][4];
    guMtxL2F(floats, &native);
    Matrix m;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m[i][j] = floats[i][j];
        }
    }
    return m;
}

s16 ReadHalf(const void *source, std::size_t index, bool bigEndian) {
    s16 value;
    std::memcpy(&value, static_cast<const std::uint8_t *>(source) + index * 2, sizeof(value));
    return bigEndian ? static_cast<s16>(__builtin_bswap16(static_cast<u16>(value))) : value;
}

Color Unpack8888(std::uint32_t rgba) {
    return {static_cast<float>(rgba >> 24) / 255.0f,
            static_cast<float>(rgba >> 16 & 0xff) / 255.0f,
            static_cast<float>(rgba >> 8 & 0xff) / 255.0f,
            static_cast<float>(rgba & 0xff) / 255.0f};
}

Color Unpack5551(std::uint16_t value) {
    return {static_cast<float>(value >> 11 & 0x1f) / 31.0f,
            static_cast<float>(value >> 6 & 0x1f) / 31.0f,
            static_cast<float>(value >> 1 & 0x1f) / 31.0f,
            (value & 1) ? 1.0f : 0.0f};
}

Color UnpackIntensityAlpha(float intensity, float alpha) {
    return {intensity, intensity, intensity, alpha};
}

std::uint16_t ReadBigEndianHalf(const std::uint8_t *bytes) {
    return static_cast<std::uint16_t>(bytes[0] << 8 | bytes[1]);
}

// The texel of a tile at whole texel coordinates, after the wrap of the tile.
int WrapCoordinate(int value, int clamp, int mask, int last) {
    if ((clamp & G_TX_CLAMP) || mask == 0) {
        value = std::clamp(value, 0, std::max(last, 0));
    }
    if (mask != 0) {
        if ((clamp & G_TX_MIRROR) && (value >> mask & 1)) {
            value = ~value;
        }
        value &= (1 << mask) - 1;
    }
    return value;
}

Color ReadTexel(const Tile &tile, int s, int t) {
    s = WrapCoordinate(s, tile.clampS, tile.maskS, (tile.lowerRightS - tile.upperLeftS) >> 2);
    t = WrapCoordinate(t, tile.clampT, tile.maskT, (tile.lowerRightT - tile.upperLeftT) >> 2);
    int bits = 4 << tile.size;
    std::size_t address = static_cast<std::size_t>(tile.tmem) * 8 +
                          static_cast<std::size_t>(t) * tile.line * 8 +
                          static_cast<std::size_t>(s) * bits / 8;
    // The texture memory swaps the words of each double word of an odd row.
    if (t & 1) {
        address ^= 4;
    }
    const auto &tmem = g_rdp.tmem;
    auto byteAt = [&tmem](std::size_t offset) { return tmem[offset & (kTmemBytes - 1)]; };
    std::uint8_t byte = byteAt(address);
    int nibble = (s & 1) ? byte & 0xf : byte >> 4;
    bool paletted = (g_rdp.otherHigh & (3u << G_MDSFT_TEXTLUT)) != 0;
    bool intensityPalette = (g_rdp.otherHigh & (3u << G_MDSFT_TEXTLUT)) == G_TT_IA16;
    auto paletteColor = [&](int index) {
        std::uint16_t entry = g_rdp.tlut[index & 0xff];
        if (intensityPalette) {
            return UnpackIntensityAlpha(static_cast<float>(entry >> 8) / 255.0f,
                                        static_cast<float>(entry & 0xff) / 255.0f);
        }
        return Unpack5551(entry);
    };
    if (tile.format == G_IM_FMT_CI || (paletted && tile.size <= G_IM_SIZ_8b)) {
        return tile.size == G_IM_SIZ_4b ? paletteColor(tile.palette * 16 + nibble) :
                                          paletteColor(byte);
    }
    switch (tile.size) {
    case G_IM_SIZ_4b:
        if (tile.format == G_IM_FMT_IA) {
            return UnpackIntensityAlpha(static_cast<float>(nibble >> 1) / 7.0f,
                                        (nibble & 1) ? 1.0f : 0.0f);
        }
        return UnpackIntensityAlpha(static_cast<float>(nibble) / 15.0f,
                                    static_cast<float>(nibble) / 15.0f);
    case G_IM_SIZ_8b:
        if (tile.format == G_IM_FMT_IA) {
            return UnpackIntensityAlpha(static_cast<float>(byte >> 4) / 15.0f,
                                        static_cast<float>(byte & 0xf) / 15.0f);
        }
        return UnpackIntensityAlpha(static_cast<float>(byte) / 255.0f,
                                    static_cast<float>(byte) / 255.0f);
    default: {
        std::uint8_t second = byteAt(address + 1);
        if (tile.format == G_IM_FMT_IA) {
            return UnpackIntensityAlpha(static_cast<float>(byte) / 255.0f,
                                        static_cast<float>(second) / 255.0f);
        }
        return Unpack5551(static_cast<std::uint16_t>(byte << 8 | second));
    }
    }
}

// A coordinate in texels after the shift of the tile, from the start of the tile.
float ShiftCoordinate(float value, int shift, int upperLeft) {
    if (shift > 0 && shift <= 10) {
        value /= static_cast<float>(1 << shift);
    } else if (shift > 10) {
        value *= static_cast<float>(1 << (16 - shift));
    }
    return value - static_cast<float>(upperLeft) / 4.0f;
}

// Whether the other modes filter the texels, which the copy mode never does.
bool FiltersTexels() {
    return (g_rdp.otherHigh & (3u << G_MDSFT_TEXTFILT)) != G_TF_POINT &&
           (g_rdp.otherHigh & (3u << G_MDSFT_CYCLETYPE)) != G_CYC_COPY;
}

struct CombineInputs {
    Color combined, texel0, texel1, shade;
};

float Channel(const Color &color, int channel) {
    switch (channel) {
    case 0:
        return color.r;
    case 1:
        return color.g;
    case 2:
        return color.b;
    default:
        return color.a;
    }
}

// The inputs of the colour combiner, by the selectors of the a, b, c, and d of a cycle. The second
// cycle reads the texels the other way round.
float ColorInput(int slot, int selector, const CombineInputs &in, int channel, bool second) {
    const Color &texel0 = second ? in.texel1 : in.texel0;
    const Color &texel1 = second ? in.texel0 : in.texel1;
    switch (selector) {
    case G_CCMUX_COMBINED:
        return Channel(in.combined, channel);
    case G_CCMUX_TEXEL0:
        return Channel(texel0, channel);
    case G_CCMUX_TEXEL1:
        return Channel(texel1, channel);
    case G_CCMUX_PRIMITIVE:
        return Channel(g_rdp.primitive, channel);
    case G_CCMUX_SHADE:
        return Channel(in.shade, channel);
    case G_CCMUX_ENVIRONMENT:
        return Channel(g_rdp.environment, channel);
    default:
        break;
    }
    if (slot == 0) {
        return selector == 6 ? 1.0f : 0.0f;
    }
    if (slot == 3) {
        return selector == 6 ? 1.0f : 0.0f;
    }
    if (slot == 2) {
        switch (selector) {
        case 7:
            return in.combined.a;
        case 8:
            return texel0.a;
        case 9:
            return texel1.a;
        case 10:
            return g_rdp.primitive.a;
        case 11:
            return in.shade.a;
        case 12:
            return g_rdp.environment.a;
        case 14:
            return g_rdp.primitiveLodFraction;
        default:
            return 0.0f;
        }
    }
    return 0.0f;
}

float AlphaInput(int slot, int selector, const CombineInputs &in, bool second) {
    const Color &texel0 = second ? in.texel1 : in.texel0;
    const Color &texel1 = second ? in.texel0 : in.texel1;
    switch (selector) {
    case 0:
        return slot == 2 ? 0.0f : in.combined.a;
    case 1:
        return texel0.a;
    case 2:
        return texel1.a;
    case 3:
        return g_rdp.primitive.a;
    case 4:
        return in.shade.a;
    case 5:
        return g_rdp.environment.a;
    case 6:
        return slot == 2 ? g_rdp.primitiveLodFraction : 1.0f;
    default:
        return 0.0f;
    }
}

struct CycleSelectors {
    int a, b, c, d, alphaA, alphaB, alphaC, alphaD;
};

CycleSelectors Selectors(int cycle) {
    std::uint32_t high = g_rdp.combineHigh;
    std::uint32_t low = g_rdp.combineLow;
    if (cycle == 0) {
        return {static_cast<int>(high >> 20 & 0xf),
                static_cast<int>(low >> 28 & 0xf),
                static_cast<int>(high >> 15 & 0x1f),
                static_cast<int>(low >> 15 & 7),
                static_cast<int>(high >> 12 & 7),
                static_cast<int>(low >> 12 & 7),
                static_cast<int>(high >> 9 & 7),
                static_cast<int>(low >> 9 & 7)};
    }
    return {static_cast<int>(high >> 5 & 0xf),
            static_cast<int>(low >> 24 & 0xf),
            static_cast<int>(high & 0x1f),
            static_cast<int>(low >> 6 & 7),
            static_cast<int>(low >> 21 & 7),
            static_cast<int>(low >> 3 & 7),
            static_cast<int>(low >> 18 & 7),
            static_cast<int>(low & 7)};
}

Color CombineCycle(int cycle, const CombineInputs &in) {
    CycleSelectors s = Selectors(cycle);
    bool second = cycle == 1;
    Color out;
    for (int channel = 0; channel < 3; ++channel) {
        float value =
            (ColorInput(0, s.a, in, channel, second) - ColorInput(1, s.b, in, channel, second)) *
                ColorInput(2, s.c, in, channel, second) +
            ColorInput(3, s.d, in, channel, second);
        value = std::clamp(value, 0.0f, 1.0f);
        if (channel == 0) {
            out.r = value;
        } else if (channel == 1) {
            out.g = value;
        } else {
            out.b = value;
        }
    }
    out.a = std::clamp((AlphaInput(0, s.alphaA, in, second) - AlphaInput(1, s.alphaB, in, second)) *
                               AlphaInput(2, s.alphaC, in, second) +
                           AlphaInput(3, s.alphaD, in, second),
                       0.0f,
                       1.0f);
    return out;
}

std::uint32_t CycleType() {
    return g_rdp.otherHigh & (3u << G_MDSFT_CYCLETYPE);
}

Color Combine(const Color &texel0, const Color &texel1, const Color &shade) {
    CombineInputs in{{}, texel0, texel1, shade};
    in.combined = CombineCycle(0, in);
    if (CycleType() == G_CYC_2CYCLE) {
        return CombineCycle(1, in);
    }
    return in.combined;
}

// Whether the blender of the render mode mixes the colour drawn with the frame buffer by the alpha
// drawn.
bool BlendsWithMemory() {
    if (!(g_rdp.otherLow & FORCE_BL)) {
        return false;
    }
    std::uint32_t mode = g_rdp.otherLow;
    int memory = CycleType() == G_CYC_2CYCLE ? mode >> 20 & 3 : mode >> 22 & 3;
    return memory == G_BL_CLR_MEM;
}

bool AlphaCompares() {
    return (g_rdp.otherLow & 3) != G_AC_NONE;
}

std::uint32_t PackColor(const Color &color) {
    auto byte = [](float value) {
        return static_cast<std::uint32_t>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
    };
    return byte(color.a) << 24 | byte(color.r) << 16 | byte(color.g) << 8 | byte(color.b);
}

float ScreenScale() {
    return g_dwScreenScaleShift ? 1.0f : 2.0f;
}

constexpr std::uint64_t kHashSeed = 0xcbf29ce484222325ull;

std::uint64_t HashBytes(std::uint64_t hash, const void *data, std::size_t size) {
    const auto *bytes = static_cast<const std::uint8_t *>(data);
    for (std::size_t i = 0; i < size; ++i) {
        hash = (hash ^ bytes[i]) * 0x100000001b3ull;
    }
    return hash;
}

template <typename T>
std::uint64_t Hash(std::uint64_t hash, const T &value) {
    return HashBytes(hash, &value, sizeof(value));
}

// The hash of the texture memory and the palettes, computed again only after a load.
std::uint64_t TextureMemoryHash() {
    if (g_rdp.textureMemoryChanged) {
        g_rdp.textureMemoryHash =
            HashBytes(HashBytes(kHashSeed, g_rdp.tmem.data(), g_rdp.tmem.size()),
                      g_rdp.tlut.data(),
                      g_rdp.tlut.size() * sizeof(g_rdp.tlut[0]));
        g_rdp.textureMemoryChanged = false;
    }
    return g_rdp.textureMemoryHash;
}

// The texture made for the inputs a key hashes, or null when none is made yet.
const BakedTexture *FindBakedTexture(std::uint64_t key) {
    auto found = g_rdp.textures.find(key);
    if (found == g_rdp.textures.end()) {
        return nullptr;
    }
    found->second.lastFrame = g_rdp.frame;
    return &found->second;
}

// A texture of the renderer for pixels made from the inputs a key hashes, reused while they do not
// change.
const BakedTexture &StoreBakedTexture(std::uint64_t key,
                                      const std::vector<std::uint8_t> &rgba,
                                      std::uint32_t width,
                                      std::uint32_t height) {
    coastline::TextureHandle handle = 0;
    if (!g_rdp.idleHandles.empty()) {
        handle = g_rdp.idleHandles.back();
        g_rdp.idleHandles.pop_back();
        g_renderer->UpdateTexture(handle, width, height, rgba);
    } else {
        handle = g_renderer->CreateTexture(width, height, rgba, true);
    }
    return g_rdp.textures[key] = {
               handle, g_rdp.frame, static_cast<int>(width), static_cast<int>(height)};
}

void RetireIdleTextures() {
    for (auto it = g_rdp.textures.begin(); it != g_rdp.textures.end();) {
        if (g_rdp.frame - it->second.lastFrame > kTextureIdleFrames) {
            g_rdp.idleHandles.push_back(it->second.handle);
            it = g_rdp.textures.erase(it);
        } else {
            ++it;
        }
    }
}

void ApplyStates(bool textured, bool depth) {
    coastline::RenderStates &states = g_renderer->States();
    bool copy = CycleType() == G_CYC_COPY;
    states.depthTest = depth && (g_rdp.otherLow & Z_CMP);
    states.depthWrite = depth && (g_rdp.otherLow & Z_UPD);
    states.depthFunction = coastline::CompareFunction::LessEqual;
    states.blend = !copy && BlendsWithMemory();
    states.sourceBlend = coastline::BlendFactor::SourceAlpha;
    states.destinationBlend = coastline::BlendFactor::InverseSourceAlpha;
    states.alphaTest = textured && (copy || AlphaCompares() || !states.blend);
    states.alphaFunction = coastline::CompareFunction::Greater;
    states.alphaReference = 0;
    states.cull = coastline::CullMode::None;
    states.fog = false;
    states.specular = false;
    states.colorKey = false;
    states.linearFilter = false;
    states.wrapU = false;
    states.wrapV = false;
}

coastline::TextureHandle TileTexture(int index, int &width, int &height);

// The colour combiner of the display list, for the shader of the renderer. The copy mode writes the
// texel unchanged.
coastline::CombinerState GpuCombiner(bool copy) {
    coastline::CombinerState combiner;
    combiner.enabled = true;
    if (copy) {
        // The selectors take 0 for a, b, and c, and the first texel for d.
        combiner.color[0][0] = combiner.color[0][1] = 15;
        combiner.color[0][2] = 31;
        combiner.color[0][3] = G_CCMUX_TEXEL0;
        combiner.alpha[0][0] = combiner.alpha[0][1] = combiner.alpha[0][2] = 7;
        combiner.alpha[0][3] = 1;
        combiner.alphaThreshold = AlphaCompares() ? 1.0f / 255.0f : -1.0f;
        return combiner;
    }
    for (int cycle = 0; cycle < 2; ++cycle) {
        CycleSelectors s = Selectors(cycle);
        const std::int32_t color[4] = {s.a, s.b, s.c, s.d};
        const std::int32_t alpha[4] = {s.alphaA, s.alphaB, s.alphaC, s.alphaD};
        std::memcpy(combiner.color[cycle], color, sizeof(color));
        std::memcpy(combiner.alpha[cycle], alpha, sizeof(alpha));
    }
    combiner.twoCycles = CycleType() == G_CYC_2CYCLE;
    const Color colors[2] = {g_rdp.primitive, g_rdp.environment};
    float *targets[2] = {combiner.primitive, combiner.environment};
    for (int i = 0; i < 2; ++i) {
        targets[i][0] = colors[i].r;
        targets[i][1] = colors[i].g;
        targets[i][2] = colors[i].b;
        targets[i][3] = colors[i].a;
    }
    combiner.lodFraction = g_rdp.primitiveLodFraction;
    combiner.alphaThreshold = AlphaCompares() ? g_rdp.blend.a : -1.0f;
    return combiner;
}

// An axis of a tile as texel = coordinate * scale - offset, after its shift and from its start.
struct TileAxis {
    float scale;
    float offset;
};

TileAxis AxisOf(const Tile &tile, bool s) {
    int shift = s ? tile.shiftS : tile.shiftT;
    float scale = 1.0f;
    if (shift > 0 && shift <= 10) {
        scale = 1.0f / static_cast<float>(1 << shift);
    } else if (shift > 10) {
        scale = static_cast<float>(1 << (16 - shift));
    }
    return {scale, static_cast<float>(s ? tile.upperLeftS : tile.upperLeftT) / 4.0f};
}

// The scale and offset that take the texture coordinates of one tile to those of the next, as both
// sample the same s and t.
void SetSecondTile(coastline::CombinerState &combiner,
                   int index,
                   int width0,
                   int height0,
                   coastline::TextureHandle texture1,
                   int width1,
                   int height1) {
    const Tile &tile0 = g_rdp.tiles[static_cast<std::size_t>(index) & 7];
    const Tile &tile1 = g_rdp.tiles[static_cast<std::size_t>(index + 1) & 7];
    const int sizes0[2] = {width0, height0};
    const int sizes1[2] = {width1, height1};
    for (int axis = 0; axis < 2; ++axis) {
        TileAxis a0 = AxisOf(tile0, axis == 0);
        TileAxis a1 = AxisOf(tile1, axis == 0);
        float ratio = a1.scale / a0.scale;
        combiner.texture1Transform[axis] =
            static_cast<float>(sizes0[axis]) * ratio / static_cast<float>(sizes1[axis]);
        combiner.texture1Transform[2 + axis] =
            (a0.offset * ratio - a1.offset) / static_cast<float>(sizes1[axis]);
    }
    combiner.texture1 = texture1;
}

void DrawQuad(float left, float top, float right, float bottom, std::uint32_t color) {
    float scale = ScreenScale();
    const std::array<coastline::TlVertex, 4> quad = {{
        {left * scale, top * scale, 0, 1, color, 0, 0, 0},
        {right * scale, top * scale, 0, 1, color, 0, 1, 0},
        {right * scale, bottom * scale, 0, 1, color, 0, 1, 1},
        {left * scale, bottom * scale, 0, 1, color, 0, 0, 1},
    }};
    const std::array<std::uint16_t, 6> indices = {0, 1, 2, 0, 2, 3};
    g_renderer->DrawIndexed(quad, indices);
}

void DrawTextureRectangle(const GbiCommand *command, bool flip) {
    float lowerRightX = static_cast<float>(command[0].w0 >> 12 & 0xfff) / 4.0f;
    float lowerRightY = static_cast<float>(command[0].w0 & 0xfff) / 4.0f;
    int tile = static_cast<int>(command[0].w1 >> 24 & 7);
    float upperLeftX = static_cast<float>(command[0].w1 >> 12 & 0xfff) / 4.0f;
    float upperLeftY = static_cast<float>(command[0].w1 & 0xfff) / 4.0f;
    float s = static_cast<float>(static_cast<s16>(command[1].w1 >> 16)) / 32.0f;
    float t = static_cast<float>(static_cast<s16>(command[1].w1 & 0xffff)) / 32.0f;
    float dsdx = static_cast<float>(static_cast<s16>(command[2].w1 >> 16)) / 1024.0f;
    float dtdy = static_cast<float>(static_cast<s16>(command[2].w1 & 0xffff)) / 1024.0f;
    bool copy = CycleType() == G_CYC_COPY;
    if (copy) {
        // The copy mode draws four pixels a step, and its rectangles include their far edges.
        dsdx /= 4.0f;
        lowerRightX += 1;
        lowerRightY += 1;
    }
    int width = static_cast<int>(std::ceil(lowerRightX - upperLeftX));
    int height = static_cast<int>(std::ceil(lowerRightY - upperLeftY));
    if (width <= 0 || height <= 0 || width > 2048 || height > 2048) {
        return;
    }
    int width0 = 1;
    int height0 = 1;
    int width1 = 1;
    int height1 = 1;
    coastline::TextureHandle texture0 = TileTexture(tile, width0, height0);
    coastline::CombinerState combiner = GpuCombiner(copy);
    if (!copy) {
        coastline::TextureHandle texture1 = TileTexture(tile + 1, width1, height1);
        SetSecondTile(combiner, tile, width0, height0, texture1, width1, height1);
    }
    const Tile &tiled = g_rdp.tiles[static_cast<std::size_t>(tile)];
    TileAxis axisS = AxisOf(tiled, true);
    TileAxis axisT = AxisOf(tiled, false);
    // The coordinates at a corner of the rectangle. A pixel samples at its centre, and the RDP
    // steps from the first coordinate at the left edge of the first pixel, so the corners take half
    // a step back.
    auto corner = [&](float dx, float dy, float &u, float &v) {
        float across = flip ? dy : dx;
        float down = flip ? dx : dy;
        float sCoordinate = s + dsdx * (across - 0.5f);
        float tCoordinate = t + dtdy * (down - 0.5f);
        u = (sCoordinate * axisS.scale - axisS.offset) / static_cast<float>(width0);
        v = (tCoordinate * axisT.scale - axisT.offset) / static_cast<float>(height0);
    };
    float w = static_cast<float>(width);
    float h = static_cast<float>(height);
    float scale = ScreenScale();
    std::array<coastline::TlVertex, 4> quad{};
    const float offsets[4][2] = {{0, 0}, {w, 0}, {w, h}, {0, h}};
    for (std::size_t i = 0; i < 4; ++i) {
        quad[i].sx = (upperLeftX + offsets[i][0]) * scale;
        quad[i].sy = (upperLeftY + offsets[i][1]) * scale;
        quad[i].rhw = 1;
        quad[i].color = 0xffffffff;
        corner(offsets[i][0], offsets[i][1], quad[i].tu, quad[i].tv);
    }
    ApplyStates(true, false);
    coastline::RenderStates &states = g_renderer->States();
    states.wrapU = tiled.maskS != 0 && !(tiled.clampS & G_TX_CLAMP);
    states.wrapV = tiled.maskT != 0 && !(tiled.clampT & G_TX_CLAMP);
    states.linearFilter = FiltersTexels();
    states.alphaTest = false;
    g_renderer->SetTexture(texture0);
    g_renderer->SetCombiner(combiner);
    const std::array<std::uint16_t, 6> indices = {0, 1, 2, 0, 2, 3};
    g_renderer->DrawIndexed(quad, indices);
}

void DrawFillRectangle(const GbiCommand *command) {
    float lowerRightX = static_cast<float>(command->w0 >> 12 & 0xfff) / 4.0f;
    float lowerRightY = static_cast<float>(command->w0 & 0xfff) / 4.0f;
    float upperLeftX = static_cast<float>(command->w1 >> 12 & 0xfff) / 4.0f;
    float upperLeftY = static_cast<float>(command->w1 & 0xfff) / 4.0f;
    Color color;
    if (CycleType() == G_CYC_FILL) {
        // Every frame of the renderer starts with its depth cleared, and a fill of the depth buffer
        // is skipped.
        if (g_rdp.colorImage != nullptr && g_rdp.colorImage == g_rdp.depthImage) {
            return;
        }
        color = Unpack5551(static_cast<std::uint16_t>(g_rdp.fill >> 16));
        color.a = 1;
        lowerRightX += 1;
        lowerRightY += 1;
    } else {
        color = Combine({}, {}, g_rdp.primitive);
        if (!BlendsWithMemory()) {
            color.a = 1;
        }
    }
    ApplyStates(false, false);
    g_renderer->SetTexture(0);
    g_renderer->SetCombiner({});
    DrawQuad(upperLeftX, upperLeftY, lowerRightX, lowerRightY, PackColor(color));
}

void LoadBlock(const GbiCommand *command) {
    const Tile &tile = g_rdp.tiles[command->w1 >> 24 & 7];
    int upperLeftS = static_cast<int>(command->w0 >> 12 & 0xfff);
    int upperLeftT = static_cast<int>(command->w0 & 0xfff);
    int lastTexel = static_cast<int>(command->w1 >> 12 & 0xfff);
    int dxt = static_cast<int>(command->w1 & 0xfff);
    if (g_rdp.image == nullptr) {
        return;
    }
    int bits = 4 << g_rdp.imageSize;
    std::size_t bytes = static_cast<std::size_t>(lastTexel - upperLeftS + 1) * bits / 8;
    std::size_t start = static_cast<std::size_t>(tile.tmem) * 8;
    bytes = std::min(bytes, kTmemBytes - std::min(start, kTmemBytes));
    const std::uint8_t *source =
        g_rdp.image +
        (static_cast<std::size_t>(upperLeftT) * g_rdp.imageWidth + upperLeftS) * bits / 8;
    std::memcpy(&g_rdp.tmem[start], source, bytes);
    g_rdp.textureMemoryChanged = true;
    if (dxt == 0) {
        return;
    }
    // The load counts rows by the step of each double word, and swaps the words of odd rows.
    for (std::size_t word = 0; word * 8 < bytes; ++word) {
        if ((word * static_cast<std::size_t>(dxt) >> 11) & 1) {
            std::uint8_t *at = &g_rdp.tmem[start + word * 8];
            std::swap_ranges(at, at + 4, at + 4);
        }
    }
}

void LoadTile(const GbiCommand *command) {
    const Tile &tile = g_rdp.tiles[command->w1 >> 24 & 7];
    int upperLeftS = static_cast<int>(command->w0 >> 12 & 0xfff) >> 2;
    int upperLeftT = static_cast<int>(command->w0 & 0xfff) >> 2;
    int lowerRightS = static_cast<int>(command->w1 >> 12 & 0xfff) >> 2;
    int lowerRightT = static_cast<int>(command->w1 & 0xfff) >> 2;
    if (g_rdp.image == nullptr) {
        return;
    }
    int bits = 4 << g_rdp.imageSize;
    std::size_t rowBytes = static_cast<std::size_t>(lowerRightS - upperLeftS + 1) * bits / 8;
    std::size_t lineBytes = static_cast<std::size_t>(tile.line) * 8;
    for (int row = 0; row <= lowerRightT - upperLeftT; ++row) {
        std::size_t start = static_cast<std::size_t>(tile.tmem) * 8 + row * lineBytes;
        if (start >= kTmemBytes) {
            break;
        }
        std::size_t count = std::min(rowBytes, kTmemBytes - start);
        const std::uint8_t *source =
            g_rdp.image +
            (static_cast<std::size_t>(upperLeftT + row) * g_rdp.imageWidth + upperLeftS) * bits / 8;
        std::memcpy(&g_rdp.tmem[start], source, count);
        g_rdp.textureMemoryChanged = true;
        if (row & 1) {
            for (std::size_t word = 0; word + 8 <= count; word += 8) {
                std::uint8_t *at = &g_rdp.tmem[start + word];
                std::swap_ranges(at, at + 4, at + 4);
            }
        }
    }
}

void LoadPalette(const GbiCommand *command) {
    const Tile &tile = g_rdp.tiles[command->w1 >> 24 & 7];
    int count = static_cast<int>(command->w1 >> 14 & 0x3ff) + 1;
    int first = tile.tmem - 256;
    if (g_rdp.image == nullptr || first < 0) {
        return;
    }
    for (int i = 0; i < count && first + i < 256; ++i) {
        g_rdp.tlut[static_cast<std::size_t>(first + i)] = ReadBigEndianHalf(g_rdp.image + i * 2);
        g_rdp.textureMemoryChanged = true;
    }
}

void SetTile(const GbiCommand *command) {
    Tile &tile = g_rdp.tiles[command->w1 >> 24 & 7];
    tile.format = static_cast<int>(command->w0 >> 21 & 7);
    tile.size = static_cast<int>(command->w0 >> 19 & 3);
    tile.line = static_cast<int>(command->w0 >> 9 & 0x1ff);
    tile.tmem = static_cast<int>(command->w0 & 0x1ff);
    tile.palette = static_cast<int>(command->w1 >> 20 & 0xf);
    tile.clampT = static_cast<int>(command->w1 >> 18 & 3);
    tile.maskT = static_cast<int>(command->w1 >> 14 & 0xf);
    tile.shiftT = static_cast<int>(command->w1 >> 10 & 0xf);
    tile.clampS = static_cast<int>(command->w1 >> 8 & 3);
    tile.maskS = static_cast<int>(command->w1 >> 4 & 0xf);
    tile.shiftS = static_cast<int>(command->w1 & 0xf);
}

void SetTileSize(const GbiCommand *command) {
    Tile &tile = g_rdp.tiles[command->w1 >> 24 & 7];
    tile.upperLeftS = static_cast<int>(command->w0 >> 12 & 0xfff);
    tile.upperLeftT = static_cast<int>(command->w0 & 0xfff);
    tile.lowerRightS = static_cast<int>(command->w1 >> 12 & 0xfff);
    tile.lowerRightT = static_cast<int>(command->w1 & 0xfff);
}

void LoadMatrix(const GbiCommand *command) {
    int flags = static_cast<int>(command->w0 >> 16 & 0xff);
    Matrix matrix = ReadMatrix(coastline::DecodePtr32(command->w1));
    if (flags & G_MTX_PROJECTION) {
        g_rdp.projection = (flags & G_MTX_LOAD) ? matrix : Multiply(matrix, g_rdp.projection);
        return;
    }
    if (g_rdp.modelview.empty()) {
        g_rdp.modelview.push_back(Identity());
    }
    if (flags & G_MTX_PUSH) {
        g_rdp.modelview.push_back(g_rdp.modelview.back());
    }
    g_rdp.modelview.back() =
        (flags & G_MTX_LOAD) ? matrix : Multiply(matrix, g_rdp.modelview.back());
}

void LoadViewport(const GbiCommand *command) {
    const void *viewport = coastline::DecodePtr32(command->w1);
    bool bigEndian = coastline::CartridgeData::IsSegmentCopy(viewport);
    for (std::size_t i = 0; i < 3; ++i) {
        g_rdp.viewportScale[i] = static_cast<float>(ReadHalf(viewport, i, bigEndian)) / 4.0f;
        g_rdp.viewportTranslate[i] =
            static_cast<float>(ReadHalf(viewport, i + 4, bigEndian)) / 4.0f;
    }
}

void LoadLight(const GbiCommand *command) {
    std::size_t index = ((command->w0 >> 16 & 0xff) - G_MV_L0) / 2;
    const auto *light = static_cast<const std::uint8_t *>(coastline::DecodePtr32(command->w1));
    if (light == nullptr || index >= g_rdp.lights.size()) {
        return;
    }
    Light &out = g_rdp.lights[index];
    for (std::size_t i = 0; i < 3; ++i) {
        out.color[i] = static_cast<float>(light[i]) / 255.0f;
    }
    // The ambient light, after the directional lights, is an Ambient of 8 bytes with no direction.
    if (index >= static_cast<std::size_t>(g_rdp.lightCount)) {
        return;
    }
    float length = 0;
    for (std::size_t i = 0; i < 3; ++i) {
        out.direction[i] = static_cast<float>(static_cast<std::int8_t>(light[8 + i]));
        length += out.direction[i] * out.direction[i];
    }
    if (length > 0) {
        for (float &axis : out.direction) {
            axis /= std::sqrt(length);
        }
    }
}

void MoveWord(const GbiCommand *command) {
    std::uint32_t index = command->w0 & 0xff;
    std::uint32_t offset = command->w0 >> 8 & 0xffff;
    if (index == G_MW_NUMLIGHT) {
        g_rdp.lightCount = std::clamp(static_cast<int>((command->w1 - 0x80000000u) / 32) - 1, 0, 7);
    } else if (index == G_MW_LIGHTCOL && offset % 0x20 == 0 && offset / 0x20 < 8) {
        Color color = Unpack8888(command->w1);
        Light &light = g_rdp.lights[offset / 0x20];
        light.color[0] = color.r;
        light.color[1] = color.g;
        light.color[2] = color.b;
    }
}

// The colour the RSP lights a vertex with: the ambient light, and each directional light by the
// cosine of its angle to the normal in the space of the modelview matrix.
Color LightVertex(const Matrix &modelview, const std::int8_t *normal, float alpha) {
    float transformed[3] = {};
    float length = 0;
    for (int j = 0; j < 3; ++j) {
        for (int k = 0; k < 3; ++k) {
            transformed[j] += static_cast<float>(normal[k]) * modelview[k][j];
        }
        length += transformed[j] * transformed[j];
    }
    if (length > 0) {
        for (float &axis : transformed) {
            axis /= std::sqrt(length);
        }
    }
    const Light &ambient = g_rdp.lights[static_cast<std::size_t>(g_rdp.lightCount)];
    float rgb[3] = {ambient.color[0], ambient.color[1], ambient.color[2]};
    for (int i = 0; i < g_rdp.lightCount; ++i) {
        const Light &light = g_rdp.lights[static_cast<std::size_t>(i)];
        float intensity = transformed[0] * light.direction[0] +
                          transformed[1] * light.direction[1] + transformed[2] * light.direction[2];
        if (intensity > 0) {
            for (int c = 0; c < 3; ++c) {
                rgb[c] += intensity * light.color[c];
            }
        }
    }
    return {std::min(rgb[0], 1.0f), std::min(rgb[1], 1.0f), std::min(rgb[2], 1.0f), alpha};
}

void LoadVertices(const GbiCommand *command) {
    std::size_t first = (command->w0 >> 16 & 0xff) / 2;
    std::size_t count = command->w0 >> 10 & 0x3f;
    const auto *source = static_cast<const std::uint8_t *>(coastline::DecodePtr32(command->w1));
    if (source == nullptr) {
        return;
    }
    bool bigEndian = coastline::CartridgeData::IsSegmentCopy(source);
    Matrix modelview = g_rdp.modelview.empty() ? Identity() : g_rdp.modelview.back();
    Matrix transform = Multiply(modelview, g_rdp.projection);
    bool lighting = (g_rdp.geometry & G_LIGHTING) != 0;
    for (std::size_t i = 0; i < count && first + i < kVertexCount; ++i) {
        const std::uint8_t *vertex = source + i * sizeof(Vtx);
        float position[3];
        for (std::size_t axis = 0; axis < 3; ++axis) {
            position[axis] = static_cast<float>(ReadHalf(vertex, axis, bigEndian));
        }
        Vertex &out = g_rdp.vertices[first + i];
        float clip[4];
        for (int j = 0; j < 4; ++j) {
            clip[j] = position[0] * transform[0][j] + position[1] * transform[1][j] +
                      position[2] * transform[2][j] + transform[3][j];
        }
        out.x = clip[0];
        out.y = clip[1];
        out.z = clip[2];
        out.w = clip[3];
        out.s = static_cast<float>(ReadHalf(vertex, 4, bigEndian)) * g_rdp.textureScaleS / 32.0f;
        out.t = static_cast<float>(ReadHalf(vertex, 5, bigEndian)) * g_rdp.textureScaleT / 32.0f;
        const std::uint8_t *color = vertex + 12;
        out.shade = lighting ? LightVertex(modelview,
                                           reinterpret_cast<const std::int8_t *>(color),
                                           static_cast<float>(color[3]) / 255.0f) :
                               Color{static_cast<float>(color[0]) / 255.0f,
                                     static_cast<float>(color[1]) / 255.0f,
                                     static_cast<float>(color[2]) / 255.0f,
                                     static_cast<float>(color[3]) / 255.0f};
    }
}

// Replace decoded pixels with the PNG file of the same pixels in the directory of the textures, or
// write the file there when dumping. A replacement may have any size, as the coordinates of the
// draws span the texture.
void ReplaceWithPng(std::vector<std::uint8_t> &rgba, std::uint32_t &width, std::uint32_t &height) {
    if (g_textureDirectory.empty()) {
        return;
    }
    std::uint64_t hash = HashBytes(kHashSeed, rgba.data(), rgba.size());
    std::filesystem::path path =
        g_textureDirectory / std::format("{}x{}-{:016x}.png", width, height, hash);
    std::error_code error;
    if (std::filesystem::exists(path, error)) {
        SDL_Surface *loaded = SDL_LoadPNG(path.string().c_str());
        SDL_Surface *converted =
            loaded != nullptr ? SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32) : nullptr;
        if (converted != nullptr) {
            width = static_cast<std::uint32_t>(converted->w);
            height = static_cast<std::uint32_t>(converted->h);
            rgba.resize(static_cast<std::size_t>(width) * height * 4);
            for (std::uint32_t row = 0; row < height; ++row) {
                std::memcpy(&rgba[static_cast<std::size_t>(row) * width * 4],
                            static_cast<const std::uint8_t *>(converted->pixels) +
                                static_cast<std::size_t>(row) * converted->pitch,
                            static_cast<std::size_t>(width) * 4);
            }
        } else {
            SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
                        "The texture %s was not read: %s",
                        path.string().c_str(),
                        SDL_GetError());
        }
        SDL_DestroySurface(converted);
        SDL_DestroySurface(loaded);
        return;
    }
    if (!g_dumpTextures) {
        return;
    }
    std::filesystem::create_directories(g_textureDirectory, error);
    SDL_Surface *surface = SDL_CreateSurfaceFrom(static_cast<int>(width),
                                                 static_cast<int>(height),
                                                 SDL_PIXELFORMAT_RGBA32,
                                                 rgba.data(),
                                                 static_cast<int>(width * 4));
    if (surface == nullptr || !SDL_SavePNG(surface, path.string().c_str())) {
        SDL_LogWarn(SDL_LOG_CATEGORY_RENDER,
                    "The texture %s was not written: %s",
                    path.string().c_str(),
                    SDL_GetError());
    }
    SDL_DestroySurface(surface);
}

// The texture of the tile triangles sample, at the size of its mask or of the tile.
coastline::TextureHandle TileTexture(int index, int &width, int &height) {
    const Tile &tile = g_rdp.tiles[static_cast<std::size_t>(index) & 7];
    width = tile.maskS ? 1 << tile.maskS : ((tile.lowerRightS - tile.upperLeftS) >> 2) + 1;
    height = tile.maskT ? 1 << tile.maskT : ((tile.lowerRightT - tile.upperLeftT) >> 2) + 1;
    // A mirrored axis repeats every two masks, and the texture holds both halves.
    if (tile.maskS && (tile.clampS & G_TX_MIRROR)) {
        width *= 2;
    }
    if (tile.maskT && (tile.clampT & G_TX_MIRROR)) {
        height *= 2;
    }
    width = std::clamp(width, 1, 1024);
    height = std::clamp(height, 1, 1024);
    std::uint64_t key = Hash(Hash(kHashSeed, TextureMemoryHash()), tile);
    key = Hash(key, g_rdp.otherHigh & (3u << G_MDSFT_TEXTLUT));
    if (const BakedTexture *baked = FindBakedTexture(key)) {
        return baked->handle;
    }
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(width) * height * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            std::uint32_t packed = PackColor(ReadTexel(tile, x, y));
            std::uint8_t *out = &rgba[(static_cast<std::size_t>(y) * width + x) * 4];
            out[0] = static_cast<std::uint8_t>(packed >> 16);
            out[1] = static_cast<std::uint8_t>(packed >> 8);
            out[2] = static_cast<std::uint8_t>(packed);
            out[3] = static_cast<std::uint8_t>(packed >> 24);
        }
    }
    std::uint32_t uploadWidth = static_cast<std::uint32_t>(width);
    std::uint32_t uploadHeight = static_cast<std::uint32_t>(height);
    ReplaceWithPng(rgba, uploadWidth, uploadHeight);
    return StoreBakedTexture(key, rgba, uploadWidth, uploadHeight).handle;
}

void DrawTriangle(std::size_t a, std::size_t b, std::size_t c) {
    const std::array<const Vertex *, 3> corners = {&g_rdp.vertices[a % kVertexCount],
                                                   &g_rdp.vertices[b % kVertexCount],
                                                   &g_rdp.vertices[c % kVertexCount]};
    for (const Vertex *vertex : corners) {
        if (vertex->w <= 0.0f) {
            return;
        }
    }
    bool textured = g_rdp.textureOn;
    int width = 1;
    int height = 1;
    coastline::TextureHandle texture = textured ? TileTexture(g_rdp.textureTile, width, height) : 0;
    coastline::CombinerState combiner = GpuCombiner(false);
    if (textured) {
        int width1 = 1;
        int height1 = 1;
        coastline::TextureHandle texture1 = TileTexture(g_rdp.textureTile + 1, width1, height1);
        SetSecondTile(combiner, g_rdp.textureTile, width, height, texture1, width1, height1);
    }
    const Tile &tile = g_rdp.tiles[static_cast<std::size_t>(g_rdp.textureTile) & 7];
    float scale = ScreenScale();
    std::array<coastline::TlVertex, 3> out{};
    for (std::size_t i = 0; i < 3; ++i) {
        const Vertex &vertex = *corners[i];
        float inverseW = 1.0f / vertex.w;
        out[i].sx =
            (g_rdp.viewportTranslate[0] + vertex.x * inverseW * g_rdp.viewportScale[0]) * scale;
        out[i].sy =
            (g_rdp.viewportTranslate[1] - vertex.y * inverseW * g_rdp.viewportScale[1]) * scale;
        out[i].sz = std::clamp(vertex.z * inverseW * 0.5f + 0.5f, 0.0f, 1.0f);
        out[i].rhw = inverseW;
        out[i].color = PackColor(vertex.shade);
        out[i].specular = 0;
        out[i].tu =
            ShiftCoordinate(vertex.s, tile.shiftS, tile.upperLeftS) / static_cast<float>(width);
        out[i].tv =
            ShiftCoordinate(vertex.t, tile.shiftT, tile.upperLeftT) / static_cast<float>(height);
    }
    ApplyStates(textured, true);
    coastline::RenderStates &states = g_renderer->States();
    states.wrapU = !(tile.clampS & G_TX_CLAMP);
    states.wrapV = !(tile.clampT & G_TX_CLAMP);
    states.linearFilter = FiltersTexels();
    states.alphaTest = false;
    g_renderer->SetTexture(texture);
    g_renderer->SetCombiner(combiner);
    const std::array<std::uint16_t, 3> indices = {0, 1, 2};
    g_renderer->DrawIndexed(out, indices);
}

void SetOtherMode(std::uint32_t &mode, const GbiCommand *command) {
    std::uint32_t shift = command->w0 >> 8 & 0xff;
    std::uint32_t length = command->w0 & 0xff;
    std::uint32_t mask = (length >= 32 ? 0xffffffffu : ((1u << length) - 1)) << shift;
    mode = (mode & ~mask) | (command->w1 & mask);
}

void RunDisplayList(const GbiCommand *list) {
    std::array<const GbiCommand *, kDisplayListDepth> stack{};
    std::size_t depth = 0;
    const GbiCommand *command = list;
    while (command != nullptr) {
        switch (command->w0 >> 24) {
        case G_ENDDL:
            command = depth > 0 ? stack[--depth] : nullptr;
            continue;
        case G_DL: {
            const auto *target =
                static_cast<const GbiCommand *>(coastline::DecodePtr32(command->w1));
            if (!(command->w0 >> 16 & 0xff) && depth < kDisplayListDepth) {
                stack[depth++] = command + 1;
            }
            command = target;
            continue;
        }
        case G_MTX:
            LoadMatrix(command);
            break;
        case G_POPMTX:
            if (g_rdp.modelview.size() > 1) {
                g_rdp.modelview.pop_back();
            }
            break;
        case G_MOVEMEM:
            if ((command->w0 >> 16 & 0xff) == G_MV_VIEWPORT) {
                LoadViewport(command);
            } else if ((command->w0 >> 16 & 0xff) >= G_MV_L0) {
                LoadLight(command);
            }
            break;
        case G_MOVEWORD:
            MoveWord(command);
            break;
        case G_VTX:
            LoadVertices(command);
            break;
        case G_TRI1:
            DrawTriangle((command->w1 >> 16 & 0xff) / 2,
                         (command->w1 >> 8 & 0xff) / 2,
                         (command->w1 & 0xff) / 2);
            break;
        case G_TRI2:
            DrawTriangle((command->w0 >> 16 & 0xff) / 2,
                         (command->w0 >> 8 & 0xff) / 2,
                         (command->w0 & 0xff) / 2);
            DrawTriangle((command->w1 >> 16 & 0xff) / 2,
                         (command->w1 >> 8 & 0xff) / 2,
                         (command->w1 & 0xff) / 2);
            break;
        case G_TEXTURE:
            g_rdp.textureTile = static_cast<int>(command->w0 >> 8 & 7);
            g_rdp.textureOn = (command->w0 & 0xff) != 0;
            g_rdp.textureScaleS = static_cast<float>(command->w1 >> 16) / 65536.0f;
            g_rdp.textureScaleT = static_cast<float>(command->w1 & 0xffff) / 65536.0f;
            break;
        case G_SETGEOMETRYMODE:
            g_rdp.geometry |= command->w1;
            break;
        case G_CLEARGEOMETRYMODE:
            g_rdp.geometry &= ~command->w1;
            break;
        case G_SETOTHERMODE_L:
            SetOtherMode(g_rdp.otherLow, command);
            break;
        case G_SETOTHERMODE_H:
            SetOtherMode(g_rdp.otherHigh, command);
            break;
        case G_TEXRECT:
        case G_TEXRECTFLIP:
            DrawTextureRectangle(command, (command->w0 >> 24) == G_TEXRECTFLIP);
            command += 3;
            continue;
        case G_FILLRECT:
            DrawFillRectangle(command);
            break;
        case G_SETTIMG:
            g_rdp.image = static_cast<const std::uint8_t *>(coastline::DecodePtr32(command->w1));
            g_rdp.imageSize = static_cast<int>(command->w0 >> 19 & 3);
            g_rdp.imageWidth = static_cast<int>(command->w0 & 0xfff) + 1;
            break;
        case G_SETTILE:
            SetTile(command);
            break;
        case G_SETTILESIZE:
            SetTileSize(command);
            break;
        case G_LOADBLOCK:
            LoadBlock(command);
            break;
        case G_LOADTILE:
            LoadTile(command);
            break;
        case G_LOADTLUT:
            LoadPalette(command);
            break;
        case G_SETCOMBINE:
            g_rdp.combineHigh = command->w0 & 0xffffff;
            g_rdp.combineLow = command->w1;
            break;
        case G_SETPRIMCOLOR:
            g_rdp.primitive = Unpack8888(command->w1);
            g_rdp.primitiveLodFraction = static_cast<float>(command->w0 & 0xff) / 255.0f;
            break;
        case G_SETENVCOLOR:
            g_rdp.environment = Unpack8888(command->w1);
            break;
        case G_SETBLENDCOLOR:
            g_rdp.blend = Unpack8888(command->w1);
            break;
        case G_SETFOGCOLOR:
            g_rdp.fog = Unpack8888(command->w1);
            break;
        case G_SETFILLCOLOR:
            g_rdp.fill = command->w1;
            break;
        case G_SETCIMG:
            g_rdp.colorImage = coastline::DecodePtr32(command->w1);
            break;
        case G_SETZIMG:
            g_rdp.depthImage = coastline::DecodePtr32(command->w1);
            break;
        default:
            break;
        }
        ++command;
    }
}

void SubmitN64DisplayList(GbiCommand *list) {
    if (g_pD3DDevice2 == nullptr) {
        return;
    }
    g_renderer = &g_pD3DDevice2->Renderer();
    ++g_rdp.frame;
    g_pD3DDevice2->BeginScene();
    RunDisplayList(list);
    g_pD3DDevice2->EndScene();
    RetireIdleTextures();
    g_pfnPresentFrame();
}

} // namespace

bool CartridgeDataIsBigEndian(const void *host) {
    return coastline::CartridgeData::IsSegmentCopy(host);
}

void coastline::SetN64TextureDirectory(std::filesystem::path directory, bool dump) {
    g_textureDirectory = std::move(directory);
    g_dumpTextures = dump;
}

void coastline::RenderN64DisplayLists() {
    g_pfnRenderBackendSubmitDisplayList = SubmitN64DisplayList;
}
