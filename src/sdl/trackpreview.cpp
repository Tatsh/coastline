#include "trackpreview.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include <meshoptimizer.h>

namespace coastline {
namespace {

using Point = std::array<float, 3>;
using Key = std::array<long long, 3>;

// The menu scales model units by 1/32; raise the miniature 12 stage units above its silhouette.
constexpr float PREVIEW_LIFT = 384.0f;
constexpr float SHADOW_HEIGHT = 4.0f;

struct Vertex {
    Point position{};
    std::array<float, 3> color{1, 1, 1};
};
using Triangle = std::array<Vertex, 3>;

Point Subtract(const Point &a, const Point &b) {
    return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

Point Cross(const Point &a, const Point &b) {
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

float Dot(const Point &a, const Point &b) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

Key PositionKey(const Point &p) {
    return {std::llround(p[0] * 1024), std::llround(p[1] * 1024), std::llround(p[2] * 1024)};
}

std::uint32_t Word(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("Track-preview accessor exceeds its buffer.");
    }
    return bytes[offset] | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

struct Accessor {
    std::span<const std::uint8_t> bytes;
    std::size_t count;
    std::size_t stride;
    int component;

    float Read(std::size_t vertex, std::size_t axis) const {
        if (vertex >= count) {
            throw std::runtime_error("Track-preview vertex index exceeds its accessor.");
        }
        const auto offset = vertex * stride + axis * (component == 5121 ? 1 : 4);
        if (component == 5121) {
            return bytes[offset] / 255.0f;
        }
        const auto word = Word(bytes, offset);
        return component == 5126 ? std::bit_cast<float>(word) : static_cast<float>(word);
    }
};

Accessor ReadAccessor(const GltfModel &model, std::size_t index, std::size_t axes) {
    const auto &accessor = model.json.at("accessors").at(index);
    const auto &view =
        model.json.at("bufferViews").at(accessor.at("bufferView").get<std::size_t>());
    const int component = accessor.at("componentType");
    if (component != 5121 && component != 5125 && component != 5126) {
        throw std::runtime_error("Unsupported track-preview component type.");
    }
    const auto size = axes * (component == 5121 ? 1u : 4u);
    const auto stride = view.value("byteStride", size);
    const auto count = accessor.at("count").get<std::size_t>();
    const auto start = view.value("byteOffset", std::size_t{});
    const auto length = view.at("byteLength").get<std::size_t>();
    const auto offset = accessor.value("byteOffset", std::size_t{});
    if (stride < size || start > model.buffer.size() || length > model.buffer.size() - start ||
        offset > length ||
        (count && (size > length - offset || count - 1 > (length - offset - size) / stride))) {
        throw std::runtime_error("Invalid track-preview accessor bounds.");
    }
    return {
        std::span(model.buffer).subspan(start + offset, length - offset), count, stride, component};
}

std::array<float, 3> MaterialColor(const GltfModel &track, std::size_t index) {
    const auto &material = track.json.at("materials").at(index);
    const auto &pbr = material.at("pbrMetallicRoughness");
    std::array<float, 3> color{1, 1, 1};
    if (pbr.contains("baseColorTexture")) {
        const auto texture = pbr.at("baseColorTexture").at("index").get<std::size_t>();
        const auto image = track.json.at("textures").at(texture).at("source").get<std::size_t>();
        const auto &rgba = track.images.at(image).rgba;
        std::array<double, 3> sum{};
        double alpha = 0;
        for (std::size_t i = 0; i + 3 < rgba.size(); i += 4) {
            for (std::size_t axis = 0; axis < 3; ++axis) {
                sum[axis] += rgba[i + axis] * static_cast<double>(rgba[i + 3]);
            }
            alpha += rgba[i + 3];
        }
        if (alpha > 0) {
            for (std::size_t axis = 0; axis < 3; ++axis) {
                color[axis] = static_cast<float>(sum[axis] / (255 * alpha));
            }
        }
    }
    if (pbr.contains("baseColorFactor")) {
        for (std::size_t axis = 0; axis < 3; ++axis) {
            color[axis] *= pbr.at("baseColorFactor").at(axis).get<float>();
        }
    }
    if (std::ranges::any_of(color, [](float value) { return !std::isfinite(value); })) {
        throw std::runtime_error("Invalid track-preview material color.");
    }
    for (auto &value : color) {
        value = std::clamp(value, 0.0f, 1.0f);
    }
    return color;
}

struct Edge {
    Vertex from;
    Vertex to;
    int count = 0;
    bool upward = false;
};

std::map<std::pair<Key, Key>, Edge> Edges(const std::vector<Triangle> &triangles) {
    std::map<std::pair<Key, Key>, Edge> edges;
    for (const auto &triangle : triangles) {
        const auto normal = Cross(Subtract(triangle[1].position, triangle[0].position),
                                  Subtract(triangle[2].position, triangle[0].position));
        for (std::size_t i = 0; i < 3; ++i) {
            const auto &from = triangle[i];
            const auto &to = triangle[(i + 1) % 3];
            auto a = PositionKey(from.position);
            auto b = PositionKey(to.position);
            if (a == b) {
                continue;
            }
            auto &edge = edges[std::minmax(a, b)];
            edge.from = from;
            edge.to = to;
            ++edge.count;
            edge.upward = normal[2] > std::sqrt(Dot(normal, normal)) * 0.35f;
        }
    }
    return edges;
}

void CloseOpenings(std::vector<Triangle> &triangles) {
    const auto edges = Edges(triangles);
    std::map<Key, std::vector<Vertex>> neighbors;
    for (const auto &[key, edge] : edges) {
        if (edge.count == 1) {
            neighbors[key.first].push_back(
                PositionKey(edge.from.position) == key.first ? edge.to : edge.from);
            neighbors[key.second].push_back(
                PositionKey(edge.from.position) == key.second ? edge.to : edge.from);
        }
    }
    std::set<Key> visited;
    for (const auto &[start, adjacent] : neighbors) {
        if (adjacent.size() != 2 || visited.contains(start)) {
            continue;
        }
        std::vector<Vertex> loop;
        auto current = start;
        auto previous = start;
        bool closed = false;
        for (std::size_t step = 0; step < 64; ++step) {
            const auto entry = neighbors.find(current);
            if (entry == neighbors.end() || entry->second.size() != 2 ||
                visited.contains(current)) {
                break;
            }
            visited.insert(current);
            const auto &next = PositionKey(entry->second[0].position) != previous ?
                                   entry->second[0] :
                                   entry->second[1];
            loop.push_back(next);
            previous = current;
            current = PositionKey(next.position);
            if (current == start) {
                closed = true;
                break;
            }
        }
        if (!closed || loop.size() < 3) {
            continue;
        }
        Point normal{};
        for (std::size_t i = 1; i + 1 < loop.size(); ++i) {
            const auto cross = Cross(Subtract(loop[i].position, loop[0].position),
                                     Subtract(loop[i + 1].position, loop[0].position));
            if (Dot(cross, cross) > Dot(normal, normal)) {
                normal = cross;
            }
        }
        const float length = std::sqrt(Dot(normal, normal));
        if (length < 0.001f) {
            continue;
        }
        for (auto &value : normal) {
            value /= length;
        }
        const auto distance = [&](const Point &p) {
            return std::abs(Dot(Subtract(p, loop[0].position), normal));
        };
        if (std::ranges::any_of(loop,
                                [&](const Vertex &v) { return distance(v.position) > 0.01f; }) ||
            std::ranges::all_of(triangles, [&](const Triangle &t) {
                return std::ranges::all_of(
                    t, [&](const Vertex &v) { return distance(v.position) < 0.01f; });
            })) {
            continue;
        }
        // Only convex planar openings are capped; terrain contours need a skirt instead.
        float direction = 0;
        bool convex = true;
        for (std::size_t i = 0; i < loop.size(); ++i) {
            const float turn =
                Dot(Cross(Subtract(loop[(i + 1) % loop.size()].position, loop[i].position),
                          Subtract(loop[(i + 2) % loop.size()].position,
                                   loop[(i + 1) % loop.size()].position)),
                    normal);
            if (std::abs(turn) > 0.001f) {
                if (direction * turn < 0) {
                    convex = false;
                }
                direction = turn;
            }
        }
        if (convex) {
            for (std::size_t i = 1; i + 1 < loop.size(); ++i) {
                triangles.push_back({loop[0], loop[i + 1], loop[i]});
            }
        }
    }
}

void AppendWord(std::vector<std::uint8_t> &buffer, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        buffer.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

} // namespace

std::expected<GltfModel, std::string> TrackPreviewToGltf(const GltfModel &track,
                                                         std::string_view bufferUri) {
    try {
        std::vector<Triangle> triangles;
        std::size_t capTriangles = 0;
        for (const auto &node : track.json.at("nodes")) {
            if (!node.contains("mesh") || !node.contains("extras") ||
                !node.at("extras").contains("coastlineInstance")) {
                continue;
            }
            const auto matrix = node.at("matrix").get<std::array<float, 16>>();
            std::vector<Triangle> instance;
            for (const auto &primitive :
                 track.json.at("meshes").at(node.at("mesh").get<std::size_t>()).at("primitives")) {
                const auto &attributes = primitive.at("attributes");
                const auto positions = ReadAccessor(track, attributes.at("POSITION"), 3);
                const auto indices = ReadAccessor(track, primitive.at("indices"), 1);
                if (indices.component != 5125 || positions.component != 5126 || indices.count % 3) {
                    return std::unexpected("Invalid track-preview triangle accessor.");
                }
                const auto color = MaterialColor(track, primitive.at("material"));
                for (std::size_t i = 0; i < indices.count; i += 3) {
                    Triangle triangle;
                    for (std::size_t corner = 0; corner < 3; ++corner) {
                        const auto vertex = Word(indices.bytes, (i + corner) * indices.stride);
                        const Point p{positions.Read(vertex, 0),
                                      positions.Read(vertex, 1),
                                      positions.Read(vertex, 2)};
                        auto &v = triangle[corner];
                        v.color = color;
                        for (std::size_t axis = 0; axis < 3; ++axis) {
                            v.position[axis] = matrix[axis] * p[0] + matrix[4 + axis] * p[1] +
                                               matrix[8 + axis] * p[2] + matrix[12 + axis];
                            if (!std::isfinite(v.position[axis]) ||
                                std::abs(v.position[axis]) > 1e7f) {
                                return std::unexpected("Invalid track-preview coordinate.");
                            }
                        }
                    }
                    if (attributes.contains("COLOR_0")) {
                        const auto colors = ReadAccessor(track, attributes.at("COLOR_0"), 4);
                        for (std::size_t corner = 0; corner < 3; ++corner) {
                            const auto vertex = Word(indices.bytes, (i + corner) * indices.stride);
                            for (std::size_t axis = 0; axis < 3; ++axis) {
                                triangle[corner].color[axis] *= colors.Read(vertex, axis);
                                if (!std::isfinite(triangle[corner].color[axis])) {
                                    return std::unexpected("Invalid track-preview vertex color.");
                                }
                                triangle[corner].color[axis] =
                                    std::clamp(triangle[corner].color[axis], 0.0f, 1.0f);
                            }
                        }
                    }
                    instance.push_back(triangle);
                }
            }
            const auto originalCount = instance.size();
            CloseOpenings(instance);
            capTriangles += instance.size() - originalCount;
            triangles.insert(triangles.end(), instance.begin(), instance.end());
        }
        if (triangles.empty()) {
            return std::unexpected("Track preview geometry is empty.");
        }
        Point low{1e10f, 1e10f, 1e10f};
        Point high{-1e10f, -1e10f, -1e10f};
        for (const auto &triangle : triangles) {
            for (const auto &v : triangle) {
                for (std::size_t axis = 0; axis < 3; ++axis) {
                    low[axis] = std::min(low[axis], v.position[axis]);
                    high[axis] = std::max(high[axis], v.position[axis]);
                }
            }
        }
        const float extent = std::max(high[0] - low[0], high[1] - low[1]);
        if (extent <= 0) {
            return std::unexpected("Track preview horizontal extent is invalid.");
        }
        const float scale = 3200 / extent;
        if (!std::isfinite(scale)) {
            return std::unexpected("Track preview horizontal extent is invalid.");
        }
        for (auto &triangle : triangles) {
            for (auto &v : triangle) {
                v.position[0] = (v.position[0] - (low[0] + high[0]) / 2) * scale;
                v.position[1] = (v.position[1] - (low[1] + high[1]) / 2) * scale;
                // Raise the terrain above the menu spotlight to avoid coplanar depth tests.
                v.position[2] = (v.position[2] - low[2]) * scale + 32;
                if (std::ranges::any_of(v.position, [](float value) {
                        return !std::isfinite(value) || std::abs(value) > 1e7f;
                    })) {
                    return std::unexpected("Invalid track-preview coordinate.");
                }
            }
        }
        const auto edges = Edges(triangles);
        std::size_t skirtTriangles = 0;
        for (const auto &[key, edge] : edges) {
            if (edge.count != 1 || !edge.upward ||
                std::max(edge.from.position[2], edge.to.position[2]) < 1) {
                continue;
            }
            auto a = edge.from;
            auto b = edge.to;
            a.position[2] = b.position[2] = 0;
            for (auto *vertex : {&a, &b}) {
                for (auto &channel : vertex->color) {
                    channel *= 0.65f;
                }
            }
            triangles.push_back({edge.from, b, edge.to});
            triangles.push_back({edge.from, a, b});
            skirtTriangles += 2;
        }
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::map<std::array<float, 6>, unsigned int> unique;
        for (const auto &triangle : triangles) {
            for (const auto &v : triangle) {
                const std::array<float, 6> key{v.position[0],
                                               v.position[1],
                                               v.position[2],
                                               v.color[0],
                                               v.color[1],
                                               v.color[2]};
                const auto [entry, inserted] =
                    unique.try_emplace(key, static_cast<unsigned int>(vertices.size()));
                if (inserted) {
                    vertices.push_back(v);
                }
                indices.push_back(entry->second);
            }
        }
        std::vector<unsigned int> simplified(indices.size());
        constexpr std::array<float, 3> weights{0.6f, 0.6f, 0.6f};
        const auto count =
            meshopt_simplifyWithAttributes(simplified.data(),
                                           indices.data(),
                                           indices.size(),
                                           vertices[0].position.data(),
                                           vertices.size(),
                                           sizeof(Vertex),
                                           vertices[0].color.data(),
                                           sizeof(Vertex),
                                           weights.data(),
                                           weights.size(),
                                           nullptr,
                                           std::min<std::size_t>(indices.size(), 3600),
                                           0.015f,
                                           meshopt_SimplifyPermissive | meshopt_SimplifyPrune,
                                           nullptr);
        simplified.resize(count);
        if (simplified.empty()) {
            return std::unexpected("Track-preview simplification removed every triangle.");
        }
        GltfModel result;
        const auto outputVertices = simplified.size() * 2;
        for (std::size_t i = 0; i < simplified.size(); i += 3) {
            const auto &a = vertices[simplified[i]].position;
            const auto &b = vertices[simplified[i + 1]].position;
            const auto &c = vertices[simplified[i + 2]].position;
            auto normal = Cross(Subtract(b, a), Subtract(c, a));
            const float length = std::sqrt(Dot(normal, normal));
            for (auto &value : normal) {
                value = length > 0 ? value / length : 0;
            }
            for (std::size_t corner = 0; corner < 3; ++corner) {
                const auto &v = vertices[simplified[i + corner]];
                Point raised = v.position;
                raised[2] += PREVIEW_LIFT;
                for (const auto &values : {raised, normal}) {
                    for (float value : values) {
                        AppendWord(result.buffer, std::bit_cast<std::uint32_t>(value));
                    }
                }
                for (float channel : v.color) {
                    result.buffer.push_back(static_cast<std::uint8_t>(
                        std::lround(std::clamp(channel, 0.0f, 1.0f) * 255)));
                }
                result.buffer.push_back(255);
            }
            for (std::size_t corner = 0; corner < 3; ++corner) {
                const auto &v = vertices[simplified[i + corner]];
                const Point shadow{v.position[0], v.position[1], SHADOW_HEIGHT};
                for (const auto &values : {shadow, Point{0, 0, 1}}) {
                    for (float value : values) {
                        AppendWord(result.buffer, std::bit_cast<std::uint32_t>(value));
                    }
                }
                for (int channel = 0; channel < 3; ++channel) {
                    result.buffer.push_back(102);
                }
                result.buffer.push_back(255);
            }
        }
        const auto indexOffset = result.buffer.size();
        for (std::size_t i = 0; i < outputVertices; ++i) {
            AppendWord(result.buffer, static_cast<std::uint32_t>(i));
        }
        using Json = nlohmann::ordered_json;
        Json accessors = Json::array();
        for (int i = 0; i < 2; ++i) {
            accessors.push_back({{"bufferView", 0},
                                 {"byteOffset", i * 12},
                                 {"componentType", 5126},
                                 {"count", outputVertices},
                                 {"type", "VEC3"}});
        }
        accessors.push_back({{"bufferView", 0},
                             {"byteOffset", 24},
                             {"componentType", 5121},
                             {"count", outputVertices},
                             {"type", "VEC4"},
                             {"normalized", true}});
        accessors.push_back({{"bufferView", 1},
                             {"componentType", 5125},
                             {"count", outputVertices},
                             {"type", "SCALAR"}});
        low = {1e10f, 1e10f, 1e10f};
        high = {-1e10f, -1e10f, -1e10f};
        for (auto index : simplified) {
            for (std::size_t axis = 0; axis < 3; ++axis) {
                low[axis] = std::min(low[axis], vertices[index].position[axis]);
                high[axis] = std::max(high[axis], vertices[index].position[axis]);
            }
        }
        low[2] = SHADOW_HEIGHT;
        high[2] += PREVIEW_LIFT;
        accessors[0]["min"] = low;
        accessors[0]["max"] = high;
        result.json = {
            {"asset", {{"version", "2.0"}, {"generator", "Coastline track preview"}}},
            {"scene", 0},
            {"scenes", {{{"nodes", {0}}}}},
            {"nodes", {{{"mesh", 0}, {"rotation", {-0.70710678f, 0, 0, 0.70710678f}}}}},
            {"meshes",
             {{{"primitives",
                {{{"attributes", {{"POSITION", 0}, {"NORMAL", 1}, {"COLOR_0", 2}}},
                  {"indices", 3},
                  {"material", 0}}}}}}},
            {"materials",
             {{{"doubleSided", true},
               {"pbrMetallicRoughness", {{"metallicFactor", 0}, {"roughnessFactor", 1}}}}}},
            {"buffers", {{{"uri", bufferUri}, {"byteLength", result.buffer.size()}}}},
            {"bufferViews",
             {{{"buffer", 0}, {"byteLength", indexOffset}, {"byteStride", 28}, {"target", 34962}},
              {{"buffer", 0},
               {"byteOffset", indexOffset},
               {"byteLength", outputVertices * 4},
               {"target", 34963}}}},
            {"accessors", accessors},
            {"extras",
             {{"coastlinePreview",
               {{"sourceTriangles", triangles.size() - skirtTriangles - capTriangles},
                {"capTriangles", capTriangles},
                {"skirtTriangles", skirtTriangles},
                {"triangles", count / 3}}}}}};
        return result;
    } catch (const std::exception &error) {
        return std::unexpected(error.what());
    }
}

} // namespace coastline
