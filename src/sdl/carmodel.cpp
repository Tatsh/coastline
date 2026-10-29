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
constexpr std::array<const char *, WHEEL_LISTS> WHEEL_LIST_NAMES{"wheel", "wheel1", "wheelSnow"};

// The G_ opcodes of F3DEX the walk follows.
constexpr std::uint8_t OP_VTX = 0x04;
constexpr std::uint8_t OP_DL = 0x06;
constexpr std::uint8_t OP_TRI2 = 0xb1;
constexpr std::uint8_t OP_ENDDL = 0xb8;
constexpr std::uint8_t OP_TRI1 = 0xbf;

std::uint32_t Big32(std::span<const std::uint8_t> image, std::size_t at) {
    return static_cast<std::uint32_t>(image[at]) << 24 | image[at + 1] << 16 | image[at + 2] << 8 |
           image[at + 3];
}

std::uint16_t Big16(std::span<const std::uint8_t> image, std::size_t at) {
    return static_cast<std::uint16_t>(image[at] << 8 | image[at + 1]);
}

// The triangles of one display list, as offsets of vertex records in the image.
using Triangles = std::vector<std::array<std::uint32_t, 3>>;

class DisplayListWalk {
public:
    explicit DisplayListWalk(std::span<const std::uint8_t> image) : m_image(image) {
    }

    std::optional<std::string> Walk(std::uint32_t address, Triangles &triangles) {
        m_slots.fill(std::nullopt);
        return Run(address, triangles, 0);
    }

    // The offsets of every vertex record a display list loaded.
    const std::map<std::uint32_t, bool> &Vertices() const {
        return m_vertices;
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
        triangles.push_back(corners);
        return std::nullopt;
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
            default:
                break;
            }
        }
    }

    std::span<const std::uint8_t> m_image;
    std::array<std::optional<std::uint32_t>, VERTEX_SLOTS> m_slots{};
    std::map<std::uint32_t, bool> m_vertices;
};

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

    GltfModel model;
    model.json = {{"asset", {{"version", "2.0"}, {"generator", "coastline"}}},
                  {"buffers", nlohmann::ordered_json::array()},
                  {"bufferViews", nlohmann::ordered_json::array()},
                  {"accessors", nlohmann::ordered_json::array()},
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

    auto addMesh = [&](const Triangles &triangles, std::string name) -> std::optional<std::size_t> {
        if (triangles.empty()) {
            return std::nullopt;
        }
        std::vector<std::uint32_t> indices;
        for (const auto &triangle : triangles) {
            for (std::uint32_t corner : triangle) {
                indices.push_back(indexOf.at(corner));
            }
        }
        std::size_t accessor = AddAccessor(model,
                                           {{"bufferView",
                                             AddView(model,
                                                     indices.data(),
                                                     indices.size() * sizeof(std::uint32_t),
                                                     ELEMENT_ARRAY_BUFFER)},
                                            {"componentType", UNSIGNED_INT},
                                            {"count", indices.size()},
                                            {"type", "SCALAR"}});
        model.json["meshes"].push_back(
            {{"name", name},
             {"primitives",
              {{{"attributes", attributes}, {"indices", accessor}, {"mode", TRIANGLES}}}}});
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
                children.push_back(*node);
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
