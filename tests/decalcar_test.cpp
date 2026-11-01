#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

#include "decalcar.h"
#include "indexedimage.h"
#include "n64model.h"

int main() {
    constexpr std::uint32_t base = 0x803c8000;
    std::vector<std::uint8_t> bytes(0x900);
    auto word = [&](std::size_t at, std::uint32_t value) {
        for (int shift = 24; shift >= 0; shift -= 8) {
            bytes[at++] = static_cast<std::uint8_t>(value >> shift);
        }
    };
    auto half = [&](std::size_t at, std::uint16_t value) {
        bytes[at] = static_cast<std::uint8_t>(value >> 8);
        bytes[at + 1] = static_cast<std::uint8_t>(value);
    };
    word(0x10, 7);
    word(0x14, base + 0x280);
    word(0x18 + 3 * 4, base + 0x200);
    word(0x18 + 6 * 4, base + 0x220);
    for (std::size_t list : {0x200, 0x220}) {
        word(list, 0x04000c00);
        word(list + 4, base + 0x800);
        word(list + 8, 0xbf000000);
        word(list + 12, 0x00000204);
        word(list + 16, 0xb8000000);
    }
    auto texture = [&](int entry, int width, std::uint32_t texels, std::uint32_t palette) {
        const auto at = 0x280 + entry * 36;
        word(at, base + texels);
        word(at + 4, base + palette);
        half(at + 12, static_cast<std::uint16_t>(width));
        half(at + 14, 4);
        word(at + 32, 0x01000008);
    };
    texture(0, 16, 0x600, 0x700);
    texture(3, 32, 0x640, 0x700);
    texture(6, 16, 0x680, 0x720);
    std::fill_n(bytes.begin() + 0x600, 32, 0x12);
    std::fill_n(bytes.begin() + 0x640, 64, 0x34);
    std::fill_n(bytes.begin() + 0x680, 32, 0x56);
    for (int entry = 0; entry < 16; ++entry) {
        half(0x700 + entry * 2, static_cast<std::uint16_t>(entry * 0x842 | 1));
    }
    std::copy_n(bytes.begin() + 0x700, 32, bytes.begin() + 0x720);
    const auto model = coastline::CarModelToGltf(bytes, 0, "mesh.bin");
    assert(model && model->images.size() == 2);
    const auto root = std::filesystem::temp_directory_path() / "coastline-decalcar-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "painted");
    std::ofstream(root / "model.gltf") << model->json.dump();
    {
        std::ofstream buffer(root / "mesh.bin", std::ios::binary);
        buffer.write(reinterpret_cast<const char *>(model->buffer.data()), model->buffer.size());
    }
    for (const auto &image : model->images) {
        if (image.indices.empty()) {
            assert(
                coastline::SaveRgba8Png(root / image.uri, image.rgba, image.width, image.height));
        } else {
            assert(coastline::SaveIndexed4Png(
                root / image.uri,
                image.indices,
                std::span<const std::uint8_t, 32>(image.palette.data(), 32),
                image.width,
                image.height));
        }
    }
    auto car = coastline::DecalCar::Load(root);
    assert(car && car->Texture(2).width == 16);
    car->Texels(0)[0] = 0xab;
    car->SetPaletteColor(2, 0, 0xf801);
    assert(car->WriteImages(root / "painted"));
    for (std::size_t index = 0; index < model->images.size(); ++index) {
        const auto &image = model->images[index];
        const auto output = root / "painted" / ("image-" + std::to_string(index) + ".png");
        if (image.address == 0x640) {
            std::vector<std::uint8_t> texels(64);
            std::array<std::uint8_t, 32> palette{};
            assert(coastline::LoadIndexed4Png(output, texels, palette));
            assert(std::ranges::all_of(texels, [](auto texel) { return texel == 0x34; }));
            assert(palette[0] == 0xf8 && palette[1] == 1);
        } else {
            assert(image.address == 0x680);
            std::ifstream original(root / image.uri, std::ios::binary);
            std::ifstream painted(output, std::ios::binary);
            const std::vector<char> expected(std::istreambuf_iterator<char>(original), {});
            const std::vector<char> actual(std::istreambuf_iterator<char>(painted), {});
            assert(actual == expected);
        }
    }
    auto document = model->json;
    auto binary = model->buffer;
    for (std::size_t index = 0; index < model->images.size(); ++index) {
        while (binary.size() % 4 != 0) {
            binary.push_back(0);
        }
        std::ifstream image(root / model->images[index].uri, std::ios::binary);
        const std::vector<std::uint8_t> encoded(std::istreambuf_iterator<char>(image), {});
        const auto view = document["bufferViews"].size();
        document["bufferViews"].push_back(
            {{"buffer", 0}, {"byteOffset", binary.size()}, {"byteLength", encoded.size()}});
        document["images"][index].erase("uri");
        document["images"][index]["bufferView"] = view;
        document["images"][index]["mimeType"] = "image/png";
        binary.insert(binary.end(), encoded.begin(), encoded.end());
    }
    document["buffers"][0].erase("uri");
    document["buffers"][0]["byteLength"] = binary.size();
    auto json = document.dump();
    while (json.size() % 4 != 0) {
        json.push_back(' ');
    }
    while (binary.size() % 4 != 0) {
        binary.push_back(0);
    }
    {
        std::ofstream glb(root / "model.glb", std::ios::binary);
        auto little = [&](std::uint32_t value) {
            for (int shift = 0; shift < 32; shift += 8) {
                glb.put(static_cast<char>(value >> shift));
            }
        };
        little(0x46546c67);
        little(2);
        little(static_cast<std::uint32_t>(28 + json.size() + binary.size()));
        little(static_cast<std::uint32_t>(json.size()));
        little(0x4e4f534a);
        glb.write(json.data(), json.size());
        little(static_cast<std::uint32_t>(binary.size()));
        little(0x004e4942);
        glb.write(reinterpret_cast<const char *>(binary.data()), binary.size());
    }
    std::filesystem::remove(root / "model.gltf");
    std::filesystem::remove(root / "mesh.bin");
    for (const auto &image : model->images) {
        std::filesystem::remove(root / image.uri);
    }
    auto packed = coastline::DecalCar::Load(root);
    assert(packed);
    packed->Texels(0)[0] = 0xab;
    packed->SetPaletteColor(2, 0, 0xf801);
    std::filesystem::create_directories(root / "packed");
    assert(packed->WriteImages(root / "packed"));
    for (std::size_t index = 0; index < model->images.size(); ++index) {
        const auto file = "image-" + std::to_string(index) + ".png";
        std::ifstream separate(root / "painted" / file, std::ios::binary);
        std::ifstream embedded(root / "packed" / file, std::ios::binary);
        const std::vector<char> expected(std::istreambuf_iterator<char>(separate), {});
        const std::vector<char> actual(std::istreambuf_iterator<char>(embedded), {});
        assert(actual == expected);
    }
    std::filesystem::remove_all(root);
}
