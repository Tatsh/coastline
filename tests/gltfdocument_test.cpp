#include <cstdlib>
#include <fstream>
#include <iostream>
#include <span>
#include <string_view>

#include "gltfdocument.h"

namespace {

using Bytes = std::vector<std::uint8_t>;
int g_failures = 0;

void Check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "Failed. " << message << '\n';
        ++g_failures;
    }
}

void Write(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}

void Word(Bytes &bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

Bytes Container(const nlohmann::ordered_json &json, Bytes binary = {}) {
    auto text = json.dump();
    while (text.size() % 4 != 0) {
        text += ' ';
    }
    while (binary.size() % 4 != 0) {
        binary.push_back(0);
    }
    Bytes bytes;
    Word(bytes, 0x46546C67);
    Word(bytes, 2);
    Word(bytes, 20 + text.size() + (binary.empty() ? 0 : 8 + binary.size()));
    Word(bytes, text.size());
    Word(bytes, 0x4E4F534A);
    bytes.insert(bytes.end(), text.begin(), text.end());
    if (!binary.empty()) {
        Word(bytes, binary.size());
        Word(bytes, 0x004E4942);
        bytes.insert(bytes.end(), binary.begin(), binary.end());
    }
    return bytes;
}

} // namespace

int main() {
    auto root = std::filesystem::temp_directory_path() / "coastline-gltfdocument-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    auto path = root / "model.glb";
    const nlohmann::ordered_json document{
        {"asset", {{"version", "2.0"}}},
        {"buffers", {{{"byteLength", 7}}}},
        {"bufferViews", {{{"buffer", 0}, {"byteOffset", 4}, {"byteLength", 3}}}},
        {"images",
         {{{"bufferView", 0},
           {"mimeType", "image/png"},
           {"extras", {{"coastlinePaint", {{"material", 2}}}}}}}},
        {"extras", {{"coastlineModel", {{"residualBufferView", 0}}}}}};
    const Bytes binary{1, 2, 3, 4, 'p', 'n', 'g'};
    const Bytes image{'p', 'n', 'g'};
    const auto valid = Container(document, binary);
    Write(path, valid);
    auto read = coastline::ReadGltfDocument(root / "model.gltf");
    Check(read && read->path == path && read->buffer == binary,
          "a missing glTF selects GLB and excludes BIN padding");
    Check(read && read->Image(0) == image, "embedded image bytes come from their buffer view");
    Check(read && read->json["extras"] == document["extras"] &&
              read->json["images"][0]["extras"] == document["images"][0]["extras"],
          "model and paint metadata remain intact");
    Check(read && !read->Image(1), "an invalid image index reports an error");

    auto separate = document;
    separate["buffers"][0]["uri"] = "mesh.bin";
    separate["images"][0].erase("bufferView");
    separate["images"][0]["uri"] = "image.png";
    Write(root / "mesh.bin", binary);
    Write(root / "image.png", image);
    auto text = separate.dump();
    Write(root / "model.gltf", {reinterpret_cast<const std::uint8_t *>(text.data()), text.size()});
    read = coastline::ReadGltfDocument(path);
    Check(read && read->path.extension() == ".gltf" && read->buffer == binary &&
              read->Image(0) == image,
          "glTF takes precedence even through a GLB reference and reads external resources");
    Write(root / "model.gltf", {});
    Check(!coastline::ReadGltfDocument(path), "invalid glTF does not fall back to valid GLB");
    std::filesystem::remove(root / "model.gltf");

    auto multiple = document;
    multiple["buffers"].push_back(
        {{"uri", "data:application/octet-stream;base64,BQYH"}, {"byteLength", 3}});
    multiple["bufferViews"].push_back({{"buffer", 1}, {"byteLength", 3}});
    multiple["images"][0]["bufferView"] = 1;
    multiple["images"].push_back({{"uri", "data:image/png;base64,cG5n"}});
    Write(path, Container(multiple, binary));
    read = coastline::ReadGltfDocument(path);
    Check(read && read->buffer == Bytes({1, 2, 3, 4, 'p', 'n', 'g', 0, 5, 6, 7}) &&
              read->json["bufferViews"][1]["buffer"] == 0 &&
              read->json["bufferViews"][1]["byteOffset"] == 8 &&
              read->Image(0) == Bytes({5, 6, 7}) && read->Image(1) == image,
          "multiple buffers are aligned and joined with embedded and data-URI images");

    for (auto length : {0u, 11u, 19u, static_cast<unsigned>(valid.size() - 1)}) {
        Write(path, std::span(valid).first(length));
        Check(!coastline::ReadGltfDocument(path), "truncated GLB files report errors");
    }
    for (auto offset : {0u, 4u, 8u, 12u, 16u}) {
        auto corrupt = valid;
        corrupt[offset] ^= 0xFF;
        Write(path, corrupt);
        Check(!coastline::ReadGltfDocument(path), "invalid GLB headers and chunks report errors");
    }
    auto invalid = document;
    invalid["bufferViews"][0]["byteLength"] = 99;
    Write(path, Container(invalid, binary));
    Check(!coastline::ReadGltfDocument(path), "out-of-range buffer views report errors");
    invalid = document;
    invalid["buffers"][0]["byteLength"] = 99;
    Write(path, Container(invalid, binary));
    Check(!coastline::ReadGltfDocument(path), "a short BIN chunk reports an error");
    Write(path, Container({{"asset", {{"version", "2.0"}}}}));
    Check(coastline::ReadGltfDocument(path).has_value(),
          "a JSON-only GLB does not need a BIN chunk");
    std::filesystem::remove_all(root);
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
