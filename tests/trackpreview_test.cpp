#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

#include "trackpreview.h"

namespace {

using Point = std::array<float, 3>;
using Triangle = std::array<Point, 3>;
using Json = nlohmann::ordered_json;

void Check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void Word(std::vector<std::uint8_t> &buffer, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        buffer.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

float Float(const std::vector<std::uint8_t> &buffer, std::size_t offset) {
    std::uint32_t word = 0;
    for (unsigned shift = 0; shift < 32; shift += 8) {
        word |= static_cast<std::uint32_t>(buffer[offset + shift / 8]) << shift;
    }
    return std::bit_cast<float>(word);
}

coastline::GltfModel Model(const std::vector<Triangle> &triangles) {
    coastline::GltfModel model;
    for (const auto &triangle : triangles) {
        for (const auto &point : triangle) {
            for (float value : point) {
                Word(model.buffer, std::bit_cast<std::uint32_t>(value));
            }
        }
    }
    const auto positions = model.buffer.size();
    for (std::size_t i = 0; i < triangles.size() * 3; ++i) {
        Word(model.buffer, static_cast<std::uint32_t>(i));
    }
    model.json["nodes"] =
        Json::array({{{"mesh", 0},
                      {"matrix", {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1}},
                      {"extras", {{"coastlineInstance", {{"index", 1}, {"flags", 0}}}}}}});
    model.json["meshes"] = Json::array(
        {{{"primitives",
           Json::array({{{"attributes", {{"POSITION", 0}}}, {"indices", 1}, {"material", 0}}})}}});
    model.json["materials"] =
        Json::array({{{"pbrMetallicRoughness", {{"baseColorFactor", {0.3f, 0.6f, 0.2f, 1}}}}}});
    model.json["bufferViews"] =
        Json::array({{{"byteLength", positions}},
                     {{"byteOffset", positions}, {"byteLength", triangles.size() * 12}}});
    model.json["accessors"] = Json::array(
        {{{"bufferView", 0}, {"componentType", 5126}, {"count", triangles.size() * 3}},
         {{"bufferView", 1}, {"componentType", 5125}, {"count", triangles.size() * 3}}});
    return model;
}

void Grid() {
    std::vector<Triangle> triangles;
    const auto point = [](int x, int y) -> Point {
        return {static_cast<float>(x),
                static_cast<float>(y),
                2 + std::sin(x * 0.03f) + std::sin(y * 0.03f)};
    };
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            triangles.push_back({point(x, y), point(x + 1, y), point(x, y + 1)});
            triangles.push_back({point(x + 1, y), point(x + 1, y + 1), point(x, y + 1)});
        }
    }
    auto source = Model(triangles);
    auto preview = coastline::TrackPreviewToGltf(source, "menu.bin");
    Check(preview.has_value(), "A terrain grid must produce a menu miniature.");
    const auto &stats = preview->json.at("extras").at("coastlinePreview");
    Check(stats.at("triangles").get<std::size_t>() < triangles.size() / 2,
          "Simplification must substantially reduce actual terrain geometry.");
    Check(stats.at("skirtTriangles").get<int>() > 0,
          "Exposed terrain edges must extend to the base.");
    Check(preview->json.at("accessors")[0].at("max")[0].get<float>() <= 1600.01f,
          "The miniature must fit the menu stage.");
    Check(preview->json.at("accessors")[0].at("count").get<std::size_t>() ==
              stats.at("triangles").get<std::size_t>() * 6,
          "Every visible triangle must have a projected shadow triangle.");
    for (std::size_t corner = 0; corner < 3; ++corner) {
        for (std::size_t axis = 0; axis < 2; ++axis) {
            Check(Float(preview->buffer, corner * 28 + axis * 4) ==
                      Float(preview->buffer, (corner + 3) * 28 + axis * 4),
                  "Shadow vertices must match the visible triangle outline.");
        }
        Check(Float(preview->buffer, (corner + 3) * 28 + 8) == 4.0f,
              "Shadow vertices must rest on the menu stage.");
    }
    Check(!preview->json.contains("images"),
          "A miniature must not require runtime texture generation.");
    auto repeated = coastline::TrackPreviewToGltf(source, "menu.bin");
    Check(repeated && repeated->buffer == preview->buffer && repeated->json == preview->json,
          "Generation must be deterministic.");
    source.buffer.pop_back();
    Check(!coastline::TrackPreviewToGltf(source, "menu.bin"),
          "Truncated accessor data must fail safely.");
}

void OpenBuilding() {
    const std::array<Point, 8> points{{{0, 0, 0},
                                       {10, 0, 0},
                                       {10, 10, 0},
                                       {0, 10, 0},
                                       {0, 0, 10},
                                       {10, 0, 10},
                                       {10, 10, 10},
                                       {0, 10, 10}}};
    std::vector<Triangle> triangles;
    for (const auto &face : std::array<std::array<int, 4>, 5>{
             {{0, 3, 2, 1}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {4, 5, 6, 7}}}) {
        triangles.push_back({points[face[0]], points[face[1]], points[face[2]]});
        triangles.push_back({points[face[0]], points[face[2]], points[face[3]]});
    }
    auto source = Model(triangles);
    auto preview = coastline::TrackPreviewToGltf(source, "building.bin");
    Check(preview.has_value(), "An open building must produce a miniature.");
    Check(preview->json.at("extras").at("coastlinePreview").at("capTriangles") == 2,
          "A missing external building wall must receive two triangles.");
    source.json["nodes"][0].erase("extras");
    Check(!coastline::TrackPreviewToGltf(source, "building.bin"),
          "Sky and unrelated meshes must not become the miniature.");
    source = Model(triangles);
    Word(source.buffer, 0);
    const auto nan = std::bit_cast<std::uint32_t>(std::numeric_limits<float>::quiet_NaN());
    for (unsigned i = 0; i < 4; ++i) {
        source.buffer[i] = static_cast<std::uint8_t>(nan >> (i * 8));
    }
    Check(!coastline::TrackPreviewToGltf(source, "building.bin"),
          "Non-finite geometry must fail before quantisation or simplification.");
    source = Model({{{{0, 0, 0}, {1e-40f, 0, 0}, {0, 1e-40f, 0}}}});
    Check(!coastline::TrackPreviewToGltf(source, "building.bin"),
          "A scale overflow must fail before coordinate quantisation.");
}

} // namespace

int main() {
    Grid();
    OpenBuilding();
    return 0;
}
