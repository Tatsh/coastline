#include "n64model.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <format>
#include <map>
#include <optional>

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
    bool lit;             // Whether the renderer turns G_LIGHTING on before the display lists.
};

constexpr Layout CAR_LAYOUT{0x803c8000, 0x10, 0x14, false, true};
constexpr Layout TRACK_LAYOUT{0x80025c00, 0x18, 0x1c, true, false};

constexpr int LODS = 3;
constexpr int BODY_LISTS = 10;
constexpr int WHEEL_LISTS = 3;
constexpr int WHEELS = 4;
constexpr std::size_t BODY_LISTS_AT = 0x18;
constexpr std::size_t WHEEL_LISTS_AT = 0xbc;
constexpr std::size_t WHEELS_AT = WHEEL_LISTS_AT + LODS * WHEEL_LISTS * 4;
constexpr std::size_t CAR_HEADER_END = WHEELS_AT + WHEELS * 12;
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
constexpr std::array<const char *, WHEEL_LISTS> WHEEL_LIST_NAMES{"wheel", "wheel1", "wheelSnow"};

constexpr std::size_t TRACK_SKY_AT = 0x50;
constexpr std::size_t TRACK_INSTANCE_NAMES_AT = 0x5c;
constexpr std::size_t TRACK_INSTANCES_AT = 0x60;
constexpr std::size_t TRACK_INSTANCE_COUNT_AT = 0x64;
constexpr std::size_t TRACK_HEADER_END = 0x230;
constexpr std::size_t TRACK_INSTANCE_SIZE = 0x54;
constexpr std::size_t TRACK_INSTANCE_DISPLAY_LIST_AT = 0x44;

// The G_ opcodes of F3DEX the walk follows.
constexpr std::uint8_t OP_VTX = 0x04;
constexpr std::uint8_t OP_DL = 0x06;
constexpr std::uint8_t OP_TRI2 = 0xb1;
constexpr std::uint8_t OP_ENDDL = 0xb8;
constexpr std::uint8_t OP_TEXTURE = 0xbb;
constexpr std::uint8_t OP_TRI1 = 0xbf;
constexpr std::uint8_t OP_NOOP = 0xc0;
constexpr std::uint8_t OP_LOADTLUT = 0xf0;
constexpr std::uint8_t OP_SETTILESIZE = 0xf2;
constexpr std::uint8_t OP_LOADBLOCK = 0xf3;
constexpr std::uint8_t OP_LOADTILE = 0xf4;
constexpr std::uint8_t OP_SETTILE = 0xf5;
constexpr std::uint8_t OP_SETTIMG = 0xfd;

// The tile the triangles sample.
constexpr std::uint32_t RENDER_TILE = 0;

std::uint32_t Big32(std::span<const std::uint8_t> image, std::size_t at) {
    return static_cast<std::uint32_t>(image[at]) << 24 | image[at + 1] << 16 | image[at + 2] << 8 |
           image[at + 3];
}

std::uint16_t Big16(std::span<const std::uint8_t> image, std::size_t at) {
    return static_cast<std::uint16_t>(image[at] << 8 | image[at + 1]);
}

// What a triangle is drawn with: the texels and the palette it samples, how its texture
// coordinates map to them, and the name the display list gives its material.
struct Material {
    std::uint32_t image = 0;
    std::uint32_t palette = 0;
    std::uint32_t scaleS = 0x10000;
    std::uint32_t scaleT = 0x10000;
    std::uint32_t shiftS = 0;
    std::uint32_t shiftT = 0;
    std::uint32_t uls = 0;
    std::uint32_t ult = 0;
    // The entry of the texture table the renderer binds before the display list, until the list
    // loads a texture.
    std::optional<std::size_t> bound;
    // Whether the colour bytes of a vertex are a normal, as under G_LIGHTING.
    bool lit = false;
    std::string name;

    auto operator<=>(const Material &) const = default;
};

constexpr std::uint8_t OP_CLEARGEOMETRYMODE = 0xb6;
constexpr std::uint8_t OP_SETGEOMETRYMODE = 0xb7;
constexpr std::uint32_t G_LIGHTING = 0x20000;

// A triangle, as offsets of its vertex records in the image, and the index of its material.
struct Triangle {
    std::array<std::uint32_t, 3> corners;
    std::size_t material;
};

// The triangles of one display list.
using Triangles = std::vector<Triangle>;

class DisplayListWalk {
public:
    DisplayListWalk(std::span<const std::uint8_t> image, std::uint32_t base)
        : m_image(image), m_base(base) {
    }

    std::optional<std::string>
    Walk(std::uint32_t address, Triangles &triangles, bool lit, std::optional<std::size_t> bound) {
        m_slots.fill(std::nullopt);
        m_current = {};
        m_current.lit = lit;
        m_current.bound = bound;
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

private:
    std::optional<std::uint32_t> Offset(std::uint32_t address, std::size_t size) const {
        if (address < m_base || address - m_base > m_image.size() ||
            size > m_image.size() - (address - m_base)) {
            return std::nullopt;
        }
        return address - m_base;
    }

    std::optional<std::string> Triangle(std::uint32_t packed, Triangles &triangles) {
        std::array<std::uint32_t, 3> corners{};
        for (int i = 0; i < 3; ++i) {
            std::uint32_t slot = (packed >> (16 - 8 * i) & 0xff) / 2;
            if (slot >= VERTEX_SLOTS || !m_slots[slot]) {
                return std::format("a triangle uses the empty vertex slot {}", slot);
            }
            corners[static_cast<std::size_t>(i)] = *m_slots[slot];
        }
        auto found = std::ranges::find(m_materials, m_current);
        if (found == m_materials.end()) {
            m_materials.push_back(m_current);
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
                    m_current.name = m_comment.substr(open + 1, close - open - 1);
                }
                m_comment.clear();
                return;
            }
            m_comment.push_back(static_cast<char>(c));
        }
    }

    std::optional<std::string> Run(std::uint32_t address, Triangles &triangles, int depth) {
        if (depth >= MAX_DEPTH) {
            return std::format("the display lists nest deeper than {}", MAX_DEPTH);
        }
        for (;;) {
            auto at = Offset(address, 8);
            if (!at) {
                return std::format("the display list command at {:#x} is outside the model",
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
                    return std::format("the vertices at {:#x} are outside the model", w1);
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
            case OP_SETTIMG:
                m_textureImage = w1;
                break;
            case OP_LOADBLOCK:
            case OP_LOADTILE:
                m_current.image = m_textureImage;
                m_current.bound = std::nullopt;
                break;
            case OP_LOADTLUT:
                m_current.palette = m_textureImage;
                break;
            case OP_SETGEOMETRYMODE:
                m_current.lit = m_current.lit || (w1 & G_LIGHTING) != 0;
                break;
            case OP_CLEARGEOMETRYMODE:
                m_current.lit = m_current.lit && (w1 & G_LIGHTING) == 0;
                break;
            case OP_TEXTURE:
                // A scale of 0xffff stands for 1.
                m_current.scaleS = (w1 >> 16) == 0xffff ? 0x10000 : w1 >> 16;
                m_current.scaleT = (w1 & 0xffff) == 0xffff ? 0x10000 : w1 & 0xffff;
                break;
            case OP_SETTILE:
                if ((w1 >> 24 & 7) == RENDER_TILE) {
                    m_current.shiftS = w1 & 0xf;
                    m_current.shiftT = w1 >> 10 & 0xf;
                }
                break;
            case OP_SETTILESIZE:
                if ((w1 >> 24 & 7) == RENDER_TILE) {
                    m_current.uls = w0 >> 12 & 0xfff;
                    m_current.ult = w0 & 0xfff;
                }
                break;
            default:
                break;
            }
        }
    }

    std::span<const std::uint8_t> m_image;
    std::uint32_t m_base;
    std::array<std::optional<std::uint32_t>, VERTEX_SLOTS> m_slots{};
    std::map<std::uint32_t, bool> m_vertices;
    std::vector<Material> m_materials;
    Material m_current;
    std::uint32_t m_textureImage = 0;
    std::string m_comment;
};

constexpr std::size_t TEXTURE_SIZE = 0x24;
constexpr std::uint32_t FORMAT_CI4 = 1;
constexpr std::uint32_t FORMAT_I8 = 4;
constexpr int LINEAR = 9729;
constexpr int REPEAT = 10497;
constexpr int MIRRORED_REPEAT = 33648;
constexpr int CLAMP_TO_EDGE = 33071;

// A texture of the model: where its texels and palette are, its size and format, and how it
// repeats. The renderer reads a format other than CI4 and I8 as RGBA 5551.
struct Texture {
    std::uint32_t image = 0;
    std::uint32_t palette = 0;
    std::uint32_t source = 0;
    int width = 0;
    int height = 0;
    std::uint32_t format = 0;
    bool mirrorS = false;
    bool mirrorT = false;
    bool clampS = false;
    bool clampT = false;
    bool animated = false;
    bool cutout = false;
};

constexpr std::uint32_t ROM_NONE = 0xffffffff;
constexpr std::size_t ANIMATION_FRAME_COUNT_AT = 2;
constexpr std::size_t ANIMATION_FRAMES_AT = 8;
constexpr std::size_t ANIMATION_FRAME_SIZE = 12;

// Where the texels and the palette of a texture of a track are, as offsets in the image. The
// textures of the bank after the geometry replace those of the geometry as the race streams
// them: the first frame of an animation, the frame of the weathers other than snow for a texture
// of the snow, and for a texture that is not animated, its palette.
void StreamedTexture(std::span<const std::uint8_t> image,
                     const Layout &layout,
                     const Texture &texture,
                     std::size_t &imageAt,
                     std::size_t &paletteAt) {
    std::uint32_t bank = Big32(image, 0);
    auto inBank = [&](std::uint32_t rom) {
        return rom != ROM_NONE && bank <= image.size() && rom < image.size() - bank;
    };
    if (!texture.animated) {
        std::uint32_t rom = (texture.source & 0xfff) << 5;
        if (texture.palette != 0 && inBank(rom)) {
            paletteAt = bank + rom;
        }
        return;
    }
    std::size_t animation = texture.source - layout.base;
    if (texture.source < layout.base || animation > image.size() ||
        image.size() - animation < ANIMATION_FRAMES_AT + 2 * ANIMATION_FRAME_SIZE) {
        return;
    }
    std::uint16_t frames = Big16(image, animation + ANIMATION_FRAME_COUNT_AT);
    std::size_t first = animation + ANIMATION_FRAMES_AT;
    std::size_t frame =
        frames == 2 && Big32(image, first) == ROM_NONE ? first + ANIMATION_FRAME_SIZE : first;
    if (std::uint32_t rom = Big32(image, frame + 4); inBank(rom)) {
        imageAt = bank + rom;
    }
    if (std::uint32_t rom = Big32(image, frame + 8); texture.palette != 0 && inBank(rom)) {
        paletteAt = bank + rom;
    }
}

// The entry of the texture table that a material samples, and the texture. A CI4 texture matches
// the palette the material loads where an entry does.
std::optional<std::pair<std::size_t, Texture>>
FindTexture(std::span<const std::uint8_t> image, const Layout &layout, const Material &material) {
    std::uint32_t count = Big32(image, layout.textureCountAt);
    std::uint32_t table = Big32(image, layout.texturesAt) - layout.base;
    if ((material.image == 0 && !material.bound) || table > image.size() ||
        count > (image.size() - table) / TEXTURE_SIZE) {
        return std::nullopt;
    }
    auto read = [&](std::size_t entry) {
        std::size_t at = table + entry * TEXTURE_SIZE;
        std::uint32_t bits = Big32(image, at + 0x20);
        return Texture{.image = Big32(image, at),
                       .palette = Big32(image, at + 4),
                       .source = Big32(image, at + 8),
                       .width = Big16(image, at + 12),
                       .height = Big16(image, at + 14),
                       .format = bits >> 24 & 0xf,
                       .mirrorS = (bits >> 31 & 1) != 0,
                       .mirrorT = (bits >> 30 & 1) != 0,
                       .clampS = (bits >> 29 & 1) != 0,
                       .clampT = (bits >> 28 & 1) != 0,
                       .animated = (bits >> 20 & 1) != 0};
    };
    auto drawable = [](const Texture &texture) {
        return texture.image != 0 && texture.width > 0 && texture.height > 0;
    };
    if (material.bound) {
        if (*material.bound >= count || !drawable(read(*material.bound))) {
            return std::nullopt;
        }
        return std::pair{*material.bound, read(*material.bound)};
    }
    std::optional<std::pair<std::size_t, Texture>> found;
    for (std::size_t entry = 0; entry < count; ++entry) {
        Texture texture = read(entry);
        if (texture.image != material.image || !drawable(texture)) {
            continue;
        }
        if (texture.format != FORMAT_CI4 || texture.palette == material.palette) {
            return std::pair{entry, texture};
        }
        if (!found) {
            found = {entry, texture};
        }
    }
    return found;
}

// The texels of a texture in rows of their width. A row of the console is padded to 64 bits, and
// each odd row has the two words of each 64 bits swapped for the texture memory.
std::expected<GltfImage, std::string> DecodeTexture(std::span<const std::uint8_t> image,
                                                    const Layout &layout,
                                                    Texture &texture,
                                                    std::string uri) {
    GltfImage decoded;
    decoded.format = texture.format == FORMAT_CI4 ? GltfImageFormat::Ci4 :
                     texture.format == FORMAT_I8  ? GltfImageFormat::Intensity8 :
                                                    GltfImageFormat::Rgba5551;
    auto width = static_cast<std::size_t>(texture.width);
    auto height = static_cast<std::size_t>(texture.height);
    std::size_t rowBytes = decoded.format == GltfImageFormat::Ci4        ? (width + 1) / 2 :
                           decoded.format == GltfImageFormat::Intensity8 ? width :
                                                                           width * 2;
    std::size_t stride = (rowBytes + 7) / 8 * 8;
    std::size_t at = texture.image - layout.base;
    std::size_t paletteAt = texture.palette - layout.base;
    if (layout.streamsTextures) {
        StreamedTexture(image, layout, texture, at, paletteAt);
    }
    if (texture.image < layout.base || at > image.size() || stride * height > image.size() - at) {
        return std::unexpected(std::format("the texture {} is outside the model", uri));
    }
    decoded.uri = std::move(uri);
    decoded.width = texture.width;
    decoded.height = texture.height;
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < rowBytes; ++x) {
            decoded.texels.push_back(image[at + y * stride + (y % 2 ? x ^ 4 : x)]);
        }
    }
    if (decoded.format == GltfImageFormat::Ci4 && width % 2 != 0) {
        // The low half of the last byte of an odd row is not a texel, and the residual retains it.
        for (std::size_t y = 0; y < height; ++y) {
            decoded.texels[y * rowBytes + rowBytes - 1] &= 0xf0;
        }
    }
    if (decoded.format == GltfImageFormat::Ci4) {
        std::size_t palette = paletteAt;
        if (texture.palette < layout.base || palette > image.size() ||
            image.size() - palette < 32) {
            return std::unexpected(
                std::format("the palette of {} is outside the model", decoded.uri));
        }
        std::copy_n(
            image.begin() + static_cast<std::ptrdiff_t>(palette), 32, decoded.palette.begin());
        // A colour with an alpha of 0 cuts the texel out.
        for (std::size_t y = 0; y < height; ++y) {
            for (std::size_t x = 0; x < width; ++x) {
                unsigned index = decoded.texels[y * rowBytes + x / 2] >> (x % 2 == 0 ? 4 : 0) & 0xf;
                texture.cutout |= (decoded.palette[index * 2 + 1] & 1) == 0;
            }
        }
    } else if (decoded.format == GltfImageFormat::Rgba5551) {
        for (std::size_t i = 1; i < decoded.texels.size(); i += 2) {
            texture.cutout |= (decoded.texels[i] & 1) == 0;
        }
    }
    return decoded;
}

int WrapMode(bool mirror, bool clamp) {
    return clamp ? CLAMP_TO_EDGE : mirror ? MIRRORED_REPEAT : REPEAT;
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

// A display list, the places it is drawn at, and the scene of the places, or none.
struct Draw {
    std::string name;
    std::uint32_t displayList;
    std::vector<Placement> placements;
    std::optional<std::size_t> scene;
    std::optional<std::size_t> boundTexture = std::nullopt;
};

// The glTF of an image of the console: the meshes of the display lists of the draws, their
// textures and materials, a node for each place, and the bytes of the file outside the vertices.
std::expected<GltfModel, std::string> BuildModel(std::span<const std::uint8_t> bytes,
                                                 std::size_t imageOffset,
                                                 const Layout &layout,
                                                 const std::vector<Draw> &draws,
                                                 const std::vector<std::string> &scenes,
                                                 std::string_view bufferUri) {
    auto image = bytes.subspan(imageOffset);
    DisplayListWalk walk(image, layout.base);
    std::map<std::uint32_t, Triangles> lists;
    for (const Draw &draw : draws) {
        if (!lists.contains(draw.displayList)) {
            if (auto failure = walk.Walk(
                    draw.displayList, lists[draw.displayList], layout.lit, draw.boundTexture)) {
                return std::unexpected(std::format("{} ({})", *failure, draw.name));
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

    // The texture of each material, by the texels and the palette it loads.
    std::string stem(bufferUri.substr(0, bufferUri.rfind('.')));
    std::vector<std::optional<std::size_t>> textureOf(walk.Materials().size());
    std::vector<Texture> textures;
    std::map<std::size_t, std::size_t> imageOf;
    GltfModel model;
    for (std::size_t m = 0; m < walk.Materials().size(); ++m) {
        auto found = FindTexture(image, layout, walk.Materials()[m]);
        if (!found) {
            continue;
        }
        auto [entry, texture] = *found;
        if (!imageOf.contains(entry)) {
            auto decoded =
                DecodeTexture(image, layout, texture, std::format("{}-{}.png", stem, entry));
            if (!decoded) {
                return std::unexpected(decoded.error());
            }
            imageOf[entry] = model.images.size();
            model.images.push_back(std::move(*decoded));
            textures.push_back(texture);
        }
        textureOf[m] = imageOf[entry];
    }

    // The texture coordinates of each vertex in its texture, from the first material that draws
    // it. A vertex the console draws with two textures of other sizes takes the first.
    std::vector<float> uv(vertexCount * 2, 0.0f);
    std::vector<bool> placed(vertexCount, false);
    for (const auto &[address, triangles] : lists) {
        for (const Triangle &triangle : triangles) {
            const Material &material = walk.Materials()[triangle.material];
            if (!textureOf[triangle.material]) {
                continue;
            }
            const Texture &texture = textures[*textureOf[triangle.material]];
            for (std::uint32_t corner : triangle.corners) {
                std::uint32_t vertex = indexOf.at(corner);
                if (placed[vertex]) {
                    continue;
                }
                placed[vertex] = true;
                double s = static_cast<std::int16_t>(Big16(image, corner + 8)) / 32.0 *
                               (material.scaleS / 65536.0) * ShiftScale(material.shiftS) -
                           material.uls / 4.0;
                double t = static_cast<std::int16_t>(Big16(image, corner + 10)) / 32.0 *
                               (material.scaleT / 65536.0) * ShiftScale(material.shiftT) -
                           material.ult / 4.0;
                uv[vertex * 2] = static_cast<float>(s / texture.width);
                uv[vertex * 2 + 1] = static_cast<float>(t / texture.height);
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

    // The texture coordinates are derived for viewers, and a rebuild does not read them.
    nlohmann::ordered_json drawn = attributes;
    drawn["TEXCOORD_0"] = AddAccessor(
        model,
        {{"bufferView", AddView(model, uv.data(), uv.size() * sizeof(float), ARRAY_BUFFER)},
         {"componentType", FLOAT},
         {"count", vertexCount},
         {"type", "VEC2"}});
    // A lit vertex has a normal of signed bytes where an unlit one has its colour.
    std::vector<float> normals;
    for (std::size_t i = 0; i < vertexCount; ++i) {
        std::array<float, 3> normal{};
        float length = 0.0f;
        for (std::size_t axis = 0; axis < 3; ++axis) {
            normal[axis] = static_cast<std::int8_t>(columns.colors[i * 4 + axis]);
            length += normal[axis] * normal[axis];
        }
        length = std::sqrt(length);
        for (std::size_t axis = 0; axis < 3; ++axis) {
            normals.push_back(length > 0.0f ? normal[axis] / length : (axis == 2 ? 1.0f : 0.0f));
        }
    }
    nlohmann::ordered_json lit = drawn;
    lit["NORMAL"] =
        AddAccessor(model,
                    {{"bufferView",
                      AddView(model, normals.data(), normals.size() * sizeof(float), ARRAY_BUFFER)},
                     {"componentType", FLOAT},
                     {"count", vertexCount},
                     {"type", "VEC3"}});
    nlohmann::ordered_json shaded = drawn;
    shaded["COLOR_0"] = AddAccessor(
        model,
        {{"bufferView", AddView(model, columns.colors.data(), columns.colors.size(), ARRAY_BUFFER)},
         {"componentType", UNSIGNED_BYTE},
         {"normalized", true},
         {"count", vertexCount},
         {"type", "VEC4"}});
    model.json["images"] = nlohmann::ordered_json::array();
    model.json["samplers"] = nlohmann::ordered_json::array();
    model.json["textures"] = nlohmann::ordered_json::array();
    for (std::size_t i = 0; i < model.images.size(); ++i) {
        model.json["images"].push_back({{"uri", model.images[i].uri}});
        model.json["samplers"].push_back(
            {{"magFilter", LINEAR},
             {"minFilter", LINEAR},
             {"wrapS", WrapMode(textures[i].mirrorS, textures[i].clampS)},
             {"wrapT", WrapMode(textures[i].mirrorT, textures[i].clampT)}});
        model.json["textures"].push_back({{"source", i}, {"sampler", i}});
    }
    for (std::size_t m = 0; m < walk.Materials().size(); ++m) {
        nlohmann::ordered_json material{
            {"name",
             walk.Materials()[m].name.empty() ? std::format("material{}", m) :
                                                walk.Materials()[m].name},
            {"pbrMetallicRoughness", {{"metallicFactor", 0.0}, {"roughnessFactor", 1.0}}}};
        if (textureOf[m]) {
            material["pbrMetallicRoughness"]["baseColorTexture"] = {{"index", *textureOf[m]}};
            if (textures[*textureOf[m]].cutout) {
                material["alphaMode"] = "MASK";
            }
        }
        model.json["materials"].push_back(std::move(material));
    }

    // A mesh for each display list, with a primitive for each material in the order the list
    // first draws it.
    std::map<std::uint32_t, std::optional<std::size_t>> meshOf;
    auto addMesh = [&](std::uint32_t address, const std::string &name) {
        const Triangles &triangles = lists.at(address);
        if (triangles.empty()) {
            meshOf[address] = std::nullopt;
            return;
        }
        std::vector<std::size_t> order;
        std::map<std::size_t, std::vector<std::uint32_t>> indices;
        for (const auto &triangle : triangles) {
            if (!indices.contains(triangle.material)) {
                order.push_back(triangle.material);
            }
            for (std::uint32_t corner : triangle.corners) {
                indices[triangle.material].push_back(indexOf.at(corner));
            }
        }
        nlohmann::ordered_json primitives = nlohmann::ordered_json::array();
        for (std::size_t material : order) {
            const auto &list = indices[material];
            std::size_t accessor = AddAccessor(model,
                                               {{"bufferView",
                                                 AddView(model,
                                                         list.data(),
                                                         list.size() * sizeof(std::uint32_t),
                                                         ELEMENT_ARRAY_BUFFER)},
                                                {"componentType", UNSIGNED_INT},
                                                {"count", list.size()},
                                                {"type", "SCALAR"}});
            primitives.push_back({{"attributes", walk.Materials()[material].lit ? lit : shaded},
                                  {"indices", accessor},
                                  {"material", material},
                                  {"mode", TRIANGLES}});
        }
        model.json["meshes"].push_back({{"name", name}, {"primitives", std::move(primitives)}});
        meshOf[address] = model.json["meshes"].size() - 1;
    };

    std::vector<nlohmann::ordered_json> children(scenes.size(), nlohmann::ordered_json::array());
    for (const Draw &draw : draws) {
        if (!meshOf.contains(draw.displayList)) {
            addMesh(draw.displayList, draw.name);
        }
        if (!meshOf[draw.displayList]) {
            continue;
        }
        for (const Placement &placement : draw.placements) {
            nlohmann::ordered_json node{{"name", placement.name},
                                        {"mesh", *meshOf[draw.displayList]}};
            for (const auto &[key, value] : placement.transform.items()) {
                node[key] = value;
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

    std::size_t residualView = AddView(model, residual.data(), residual.size());
    model.json["extras"] = {{"coastlineModel",
                             {{"imageOffset", imageOffset},
                              {"vertexRuns", std::move(runs)},
                              {"residualBufferView", residualView},
                              {"attributes", attributes}}}};
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
        scenes.push_back(std::format("lod{}", lod));
        for (int list = 0; list < BODY_LISTS; ++list) {
            std::uint32_t address = Big32(image, BODY_LISTS_AT + (lod * BODY_LISTS + list) * 4);
            if (address == 0) {
                continue;
            }
            auto name = std::format("lod{}-{}", lod, BODY_LIST_NAMES[list]);
            // The highlight adds to the colours under it, and the beams draw at night or in the
            // rain. A viewer of the scene omits them.
            bool shown = list != VEHICLE_DL_HIGHLIGHTED && list != VEHICLE_DL_HEADLIGHT_BEAMS;
            // The renderer loads a texture of the model before the underside and the near detail.
            std::optional<std::size_t> bound;
            if (list == VEHICLE_DL_UNDERSIDE) {
                bound = UNDERSIDE_TEXTURE;
            } else if (list == VEHICLE_DL_NEAR_DETAIL) {
                bound = NEAR_DETAIL_TEXTURE;
            }
            draws.push_back({name,
                             address,
                             {{name, scale}},
                             shown ? std::optional<std::size_t>(lod) : std::nullopt,
                             bound});
        }
        for (int list = 0; list < WHEEL_LISTS; ++list) {
            std::uint32_t address = Big32(image, WHEEL_LISTS_AT + (lod * WHEEL_LISTS + list) * 4);
            if (address == 0) {
                continue;
            }
            // The mesh of a wheel is drawn at each of the four places of the wheels.
            Draw draw{std::format("lod{}-{}", lod, WHEEL_LIST_NAMES[list]), address, {}, lod};
            for (int place = 0; place < WHEELS; ++place) {
                std::array<float, 3> at{};
                for (int axis = 0; axis < 3; ++axis) {
                    at[static_cast<std::size_t>(axis)] = std::bit_cast<float>(
                        Big32(image, WHEELS_AT + static_cast<std::size_t>(place * 12 + axis * 4)));
                }
                nlohmann::ordered_json transform = scale;
                transform["translation"] = at;
                draw.placements.push_back(
                    {std::format("{}{}", draw.name, place), std::move(transform)});
            }
            draws.push_back(std::move(draw));
        }
    }
    return BuildModel(bytes, imageOffset, CAR_LAYOUT, draws, scenes, bufferUri);
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
                    return std::format("{}-{}", index, name);
                }
            }
        }
        return std::format("instance{}", index);
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
        draws.push_back({name, displayList, {{name, {{"matrix", matrix}}}}, 0});
    }
    if (std::uint32_t sky = Big32(bytes, TRACK_SKY_AT); sky != 0) {
        draws.push_back({"sky", sky, {{"sky", nlohmann::ordered_json::object()}}, std::nullopt});
    }
    return BuildModel(bytes, 0, TRACK_LAYOUT, draws, {"track"}, bufferUri);
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
                            std::format("vertex {} is outside the range of the console", vertex));
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
            std::format("the glTF does not describe a model of the console ({})", error.what()));
    }
}

} // namespace coastline
