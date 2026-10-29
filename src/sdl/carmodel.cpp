#include "carmodel.h"

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

constexpr std::uint32_t IMAGE_BASE = 0x803c8000;
constexpr std::size_t VERTEX_SIZE = 16;
constexpr std::size_t VERTEX_SLOTS = 32;
constexpr int LODS = 3;
constexpr int BODY_LISTS = 10;
constexpr int WHEEL_LISTS = 3;
constexpr int WHEELS = 4;
constexpr std::size_t BODY_LISTS_AT = 0x18;
constexpr std::size_t WHEEL_LISTS_AT = 0xbc;
constexpr std::size_t WHEELS_AT = WHEEL_LISTS_AT + LODS * WHEEL_LISTS * 4;
constexpr std::size_t HEADER_END = WHEELS_AT + WHEELS * 12;
// The display list stack of F3DEX.
constexpr int MAX_DEPTH = 10;
// The renderer scales the vertices of the body and of each wheel by 1/255 to metres.
constexpr std::array<float, 3> MODEL_SCALE{1.0f / 255.0f, 1.0f / 255.0f, 1.0f / 255.0f};

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
constexpr int VEHICLE_DL_HEADLIGHT_BEAMS = 5;
constexpr int VEHICLE_DL_HIGHLIGHTED = 9;
constexpr std::array<const char *, WHEEL_LISTS> WHEEL_LIST_NAMES{"wheel", "wheel1", "wheelSnow"};

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

// The tile the triangles of a car sample.
constexpr std::uint32_t RENDER_TILE = 0;

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
    std::string name;

    auto operator<=>(const Material &) const = default;
};

// A triangle, as offsets of its vertex records in the image, and the index of its material.
struct Triangle {
    std::array<std::uint32_t, 3> corners;
    std::size_t material;
};

std::uint32_t Big32(std::span<const std::uint8_t> image, std::size_t at) {
    return static_cast<std::uint32_t>(image[at]) << 24 | image[at + 1] << 16 | image[at + 2] << 8 |
           image[at + 3];
}

std::uint16_t Big16(std::span<const std::uint8_t> image, std::size_t at) {
    return static_cast<std::uint16_t>(image[at] << 8 | image[at + 1]);
}

// The triangles of one display list.
using Triangles = std::vector<Triangle>;

class DisplayListWalk {
public:
    explicit DisplayListWalk(std::span<const std::uint8_t> image) : m_image(image) {
    }

    std::optional<std::string> Walk(std::uint32_t address, Triangles &triangles) {
        m_slots.fill(std::nullopt);
        m_current = {};
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
        if (address < IMAGE_BASE || address - IMAGE_BASE > m_image.size() ||
            size > m_image.size() - (address - IMAGE_BASE)) {
            return std::nullopt;
        }
        return address - IMAGE_BASE;
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
                break;
            case OP_LOADTLUT:
                m_current.palette = m_textureImage;
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
    std::array<std::optional<std::uint32_t>, VERTEX_SLOTS> m_slots{};
    std::map<std::uint32_t, bool> m_vertices;
    std::vector<Material> m_materials;
    Material m_current;
    std::uint32_t m_textureImage = 0;
    std::string m_comment;
};

constexpr std::size_t TEXTURE_COUNT_AT = 0x10;
constexpr std::size_t TEXTURES_AT = 0x14;
constexpr std::size_t TEXTURE_SIZE = 0x24;
constexpr std::uint32_t FORMAT_CI4 = 1;
constexpr std::uint32_t FORMAT_I8 = 4;
constexpr int LINEAR = 9729;
constexpr int REPEAT = 10497;
constexpr int MIRRORED_REPEAT = 33648;
constexpr int CLAMP_TO_EDGE = 33071;

// A texture of the model: where its texels and palette are, its size and format, and how it
// repeats.
struct Texture {
    std::uint32_t image = 0;
    std::uint32_t palette = 0;
    int width = 0;
    int height = 0;
    std::uint32_t format = 0;
    bool mirrorS = false;
    bool mirrorT = false;
    bool clampS = false;
    bool clampT = false;
    bool cutout = false;
};

// The entry of the texture table that a material samples, and the texture. A CI4 texture matches
// the palette the material loads where an entry does.
std::optional<std::pair<std::size_t, Texture>> FindTexture(std::span<const std::uint8_t> image,
                                                           const Material &material) {
    std::uint32_t count = Big32(image, TEXTURE_COUNT_AT);
    std::uint32_t table = Big32(image, TEXTURES_AT) - IMAGE_BASE;
    if (material.image == 0 || table > image.size() ||
        count > (image.size() - table) / TEXTURE_SIZE) {
        return std::nullopt;
    }
    std::optional<std::pair<std::size_t, Texture>> found;
    for (std::size_t entry = 0; entry < count; ++entry) {
        std::size_t at = table + entry * TEXTURE_SIZE;
        std::uint32_t bits = Big32(image, at + 0x20);
        Texture texture{.image = Big32(image, at),
                        .palette = Big32(image, at + 4),
                        .width = Big16(image, at + 12),
                        .height = Big16(image, at + 14),
                        .format = bits >> 24 & 0xf,
                        .mirrorS = (bits >> 31 & 1) != 0,
                        .mirrorT = (bits >> 30 & 1) != 0,
                        .clampS = (bits >> 29 & 1) != 0,
                        .clampT = (bits >> 28 & 1) != 0};
        bool drawable =
            (texture.format == FORMAT_CI4 && texture.width % 2 == 0) || texture.format == FORMAT_I8;
        if (texture.image != material.image || !drawable || texture.width < 2 ||
            texture.height < 1) {
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
std::expected<GltfImage, std::string>
DecodeTexture(std::span<const std::uint8_t> image, Texture &texture, std::string uri) {
    bool ci4 = texture.format == FORMAT_CI4;
    auto width = static_cast<std::size_t>(texture.width);
    auto height = static_cast<std::size_t>(texture.height);
    std::size_t rowBytes = ci4 ? width / 2 : width;
    std::size_t stride = (rowBytes + 7) / 8 * 8;
    std::size_t at = texture.image - IMAGE_BASE;
    if (texture.image < IMAGE_BASE || at > image.size() || stride * height > image.size() - at) {
        return std::unexpected(std::format("the texture {} is outside the model", uri));
    }
    GltfImage decoded;
    decoded.uri = std::move(uri);
    decoded.width = texture.width;
    decoded.height = texture.height;
    decoded.intensity = !ci4;
    for (std::size_t y = 0; y < height; ++y) {
        for (std::size_t x = 0; x < rowBytes; ++x) {
            decoded.texels.push_back(image[at + y * stride + (y % 2 ? x ^ 4 : x)]);
        }
    }
    if (ci4) {
        std::size_t palette = texture.palette - IMAGE_BASE;
        if (texture.palette < IMAGE_BASE || palette > image.size() || image.size() - palette < 32) {
            return std::unexpected(
                std::format("the palette of {} is outside the model", decoded.uri));
        }
        std::copy_n(
            image.begin() + static_cast<std::ptrdiff_t>(palette), 32, decoded.palette.begin());
        // A colour with an alpha of 0 cuts the texel out.
        for (std::uint8_t pair : decoded.texels) {
            for (unsigned index : {unsigned{pair} >> 4, unsigned{pair} & 0xfu}) {
                texture.cutout |= (decoded.palette[index * 2 + 1] & 1) == 0;
            }
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
    if (bytes.size() < imageOffset + HEADER_END) {
        return std::unexpected("the file is too short for a model of a car");
    }
    auto image = bytes.subspan(imageOffset);
    DisplayListWalk walk(image);
    std::array<std::array<Triangles, BODY_LISTS>, LODS> body;
    std::array<std::array<Triangles, WHEEL_LISTS>, LODS> wheel;
    for (int lod = 0; lod < LODS; ++lod) {
        for (int list = 0; list < BODY_LISTS; ++list) {
            std::uint32_t address = Big32(image, BODY_LISTS_AT + (lod * BODY_LISTS + list) * 4);
            if (address != 0) {
                if (auto failure = walk.Walk(address, body[lod][list])) {
                    return std::unexpected(*failure);
                }
            }
        }
        for (int list = 0; list < WHEEL_LISTS; ++list) {
            std::uint32_t address = Big32(image, WHEEL_LISTS_AT + (lod * WHEEL_LISTS + list) * 4);
            if (address != 0) {
                if (auto failure = walk.Walk(address, wheel[lod][list])) {
                    return std::unexpected(*failure);
                }
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
        auto found = FindTexture(image, walk.Materials()[m]);
        if (!found) {
            continue;
        }
        auto [entry, texture] = *found;
        if (!imageOf.contains(entry)) {
            auto decoded = DecodeTexture(image, texture, std::format("{}-{}.png", stem, entry));
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
    auto place = [&](const Triangles &triangles) {
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
    };
    for (int lod = 0; lod < LODS; ++lod) {
        std::ranges::for_each(body[lod], place);
        std::ranges::for_each(wheel[lod], place);
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

    auto addMesh = [&](const Triangles &triangles, std::string name) -> std::optional<std::size_t> {
        if (triangles.empty()) {
            return std::nullopt;
        }
        // A primitive for each material, in the order the display list first draws it.
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
            primitives.push_back({{"attributes", drawn},
                                  {"indices", accessor},
                                  {"material", material},
                                  {"mode", TRIANGLES}});
        }
        model.json["meshes"].push_back({{"name", name}, {"primitives", std::move(primitives)}});
        model.json["nodes"].push_back({{"name", std::move(name)},
                                       {"mesh", model.json["meshes"].size() - 1},
                                       {"scale", MODEL_SCALE}});
        return model.json["nodes"].size() - 1;
    };

    for (int lod = 0; lod < LODS; ++lod) {
        nlohmann::ordered_json children = nlohmann::ordered_json::array();
        for (int list = 0; list < BODY_LISTS; ++list) {
            if (auto node =
                    addMesh(body[lod][list], std::format("lod{}-{}", lod, BODY_LIST_NAMES[list]))) {
                // The highlight adds to the colours under it, and the beams draw at night or in
                // the rain. A viewer of the scene omits them.
                if (list != VEHICLE_DL_HIGHLIGHTED && list != VEHICLE_DL_HEADLIGHT_BEAMS) {
                    children.push_back(*node);
                }
            }
        }
        for (int list = 0; list < WHEEL_LISTS; ++list) {
            if (auto node = addMesh(wheel[lod][list],
                                    std::format("lod{}-{}", lod, WHEEL_LIST_NAMES[list]))) {
                // The mesh of a wheel is drawn at each of the four places of the wheels.
                for (int place = 0; place < WHEELS; ++place) {
                    std::array<float, 3> at{};
                    for (int axis = 0; axis < 3; ++axis) {
                        at[static_cast<std::size_t>(axis)] = std::bit_cast<float>(Big32(
                            image, WHEELS_AT + static_cast<std::size_t>(place * 12 + axis * 4)));
                    }
                    model.json["nodes"].push_back(
                        {{"name", std::format("lod{}-{}{}", lod, WHEEL_LIST_NAMES[list], place)},
                         {"mesh", model.json["nodes"][*node]["mesh"]},
                         {"translation", at},
                         {"scale", MODEL_SCALE}});
                    children.push_back(model.json["nodes"].size() - 1);
                }
                // The node of the mesh itself is not part of a scene.
                model.json["nodes"][*node]["name"] =
                    std::format("lod{}-{}-unplaced", lod, WHEEL_LIST_NAMES[list]);
            }
        }
        // The car is z up and x forward, and glTF is y up.
        model.json["nodes"].push_back({{"name", std::format("lod{}", lod)},
                                       {"rotation", {-0.70710678f, 0.0f, 0.0f, 0.70710678f}},
                                       {"children", std::move(children)}});
        model.json["scenes"].push_back(
            {{"name", std::format("lod{}", lod)}, {"nodes", {model.json["nodes"].size() - 1}}});
    }

    std::size_t residualView = AddView(model, residual.data(), residual.size());
    model.json["extras"] = {{"coastlineCarModel",
                             {{"imageOffset", imageOffset},
                              {"vertexRuns", std::move(runs)},
                              {"residualBufferView", residualView},
                              {"attributes", attributes}}}};
    model.json["buffers"].push_back({{"uri", bufferUri}, {"byteLength", model.buffer.size()}});
    return model;
}

std::expected<std::vector<std::uint8_t>, std::string>
CarModelFromGltf(const nlohmann::ordered_json &json, std::span<const std::uint8_t> buffer) {
    try {
        const auto &extras = json.at("extras").at("coastlineCarModel");
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
        return std::unexpected(std::format("the glTF does not describe a car ({})", error.what()));
    }
}

} // namespace coastline
