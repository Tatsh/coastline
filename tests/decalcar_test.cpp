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
    bytes[0x110 + 3] = 3;
    std::fill_n(bytes.begin() + 0x600, 32, 0x12);
    std::fill_n(bytes.begin() + 0x640, 64, 0x34);
    std::fill_n(bytes.begin() + 0x680, 32, 0x56);
    for (int entry = 0; entry < 16; ++entry) {
        half(0x700 + entry * 2, static_cast<std::uint16_t>(entry * 0x842 | 1));
    }
    std::copy_n(bytes.begin() + 0x700, 32, bytes.begin() + 0x720);
    auto model = coastline::CarModelToGltf(bytes, 0, "mesh.bin");
    assert(model && model->images.size() == 2);
    for (std::size_t index = 0; index < model->images.size(); ++index) {
        if (model->images[index].address == 0x640) {
            model->json["images"][index]["extras"]["coastlineImage"]["width"] = 35;
            model->json["images"][index]["extras"]["coastlinePaint"]["material"] = 3;
        }
    }
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
    const auto snapshot = car->CopyEditableData();
    auto *originalTexels = car->Texels(0);
    const auto originalColor = car->PaletteColor(2, 0);
    originalTexels[0] = 0xab;
    car->SetPaletteColor(2, 0, 0xf801);
    assert(!car->RestoreEditableData(std::span<const u8>(snapshot).first(1)));
    assert(car->RestoreEditableData(snapshot));
    assert(car->Texels(0) == originalTexels);
    assert(originalTexels[0] == snapshot[car->Texture(0).texels]);
    assert(car->PaletteColor(2, 0) == originalColor);
    assert(!car->ValidNote({}));
    assert(!car->ValidNote(std::vector<u8>(32)));
    assert(car->ValidNote(std::vector<u8>(0x3e00)));
    const auto milkDirectory = root / "milk-truck";
    std::filesystem::create_directories(milkDirectory);
    std::filesystem::copy_file(root / "model.gltf", milkDirectory / "model.gltf");
    std::filesystem::copy_file(root / "mesh.bin", milkDirectory / "mesh.bin");
    for (const auto &image : model->images) {
        std::filesystem::copy_file(root / image.uri, milkDirectory / image.uri);
    }
    const auto milk = coastline::DecalCar::Load(milkDirectory);
    assert(milk && milk->DecalNote() == "COASTLINE MT");
    std::size_t milkBytes = 32;
    for (int panel = 0; panel < milk->PanelCount(); ++panel) {
        if (milk->HasMaterial(panel)) {
            const auto texture = milk->Texture(panel);
            milkBytes += static_cast<std::size_t>((texture.width + 1) / 2 * texture.height);
        }
    }
    assert(milk->ValidNote(std::vector<u8>(milkBytes)));
    assert(!milk->ValidNote(std::vector<u8>(milkBytes - 1)));
    car->Texels(0)[0] = 0xab;
    car->Texels(3)[coastline::TexelOffset(2, 1, 32)] = 0xa4;
    car->SetPaletteColor(2, 0, 0xf801);
    assert(car->WriteImages(root / "painted"));
    for (std::size_t index = 0; index < model->images.size(); ++index) {
        const auto &image = model->images[index];
        const auto output = root / "painted" / ("image-" + std::to_string(index) + ".png");
        if (image.address == 0x640) {
            std::vector<std::uint8_t> texels(72);
            std::array<std::uint8_t, 32> palette{};
            assert(coastline::LoadIndexed4Png(output, texels, palette));
            for (int y = 0; y < 4; ++y) {
                for (int x = 0; x < 35; ++x) {
                    const auto source =
                        car->Texels(3)[coastline::TexelOffset(x, y, car->Texture(3).width)];
                    const auto actual = texels[static_cast<std::size_t>(y * 18 + x / 2)];
                    assert(((x & 1) ? actual & 15 : actual >> 4) ==
                           ((x & 1) ? source & 15 : source >> 4));
                }
            }
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
    packed->Texels(3)[coastline::TexelOffset(2, 1, 32)] = 0xa4;
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
    const auto bonus = root / "cupra";
    std::filesystem::create_directories(bonus);
    auto generic = model->json;
    generic["images"].push_back(generic["images"][0]);
    for (auto &image : generic["images"]) {
        image["uri"] = "base.png";
    }
    std::vector<std::uint8_t> source(24 * 10 * 4);
    for (std::size_t pixel = 0; pixel < source.size() / 4; ++pixel) {
        source[pixel * 4] = static_cast<std::uint8_t>(pixel);
        source[pixel * 4 + 1] = static_cast<std::uint8_t>(255 - pixel);
        source[pixel * 4 + 2] = 51;
        source[pixel * 4 + 3] = static_cast<std::uint8_t>(pixel * 2);
    }
    assert(coastline::SaveRgba8Png(bonus / "base.png", source, 24, 10));
    std::ofstream(bonus / "model.gltf") << generic.dump();
    {
        std::ofstream buffer(bonus / "mesh.bin", std::ios::binary);
        buffer.write(reinterpret_cast<const char *>(model->buffer.data()), model->buffer.size());
    }
    auto layers = coastline::DecalCar::Load(bonus);
    assert(layers && layers->ImagePanels() && layers->PanelCount() == 3);
    assert(layers->PanelPixels(0) == source);
    assert(layers->PanelPixels(1) == source);
    assert(layers->PanelPixels(2) == source);
    assert(layers->DecalNote() == "COASTLINE CP");
    assert(!layers->HasMaterial(3) && layers->Mask(0) == nullptr);
    auto note = layers->NoteHeader();
    assert(!layers->ValidNote(note));
    note.resize(note.size() + 32 + 3 * 120);
    assert(layers->ValidNote(note));
    note[7] ^= 1;
    assert(!layers->ValidNote(note) && !layers->ValidNote({}));
    const auto offset = layers->PixelOffset(23, 9, 24);
    layers->Texels(1)[offset] = 0x06;
    layers->SetPaletteColor(1, 6, 0xf801);
    auto expected = source;
    const auto pixel = (24 * 10 - 1) * 4;
    expected[pixel] = 255;
    expected[pixel + 1] = 0;
    expected[pixel + 2] = 0;
    expected[pixel + 3] = 255;
    assert(layers->PanelPixels(1) == expected);
    assert(layers->PanelPixels(0) == source && layers->PanelPixels(2) == source);
    assert(layers->WriteImages(bonus / "painted"));
    auto packedJson = generic;
    auto packedBytes = model->buffer;
    while (packedBytes.size() % 4 != 0) {
        packedBytes.push_back(0);
    }
    std::ifstream image(bonus / "base.png", std::ios::binary);
    const std::vector<std::uint8_t> png(std::istreambuf_iterator<char>(image), {});
    const auto view = packedJson["bufferViews"].size();
    packedJson["bufferViews"].push_back(
        {{"buffer", 0}, {"byteOffset", packedBytes.size()}, {"byteLength", png.size()}});
    packedBytes.insert(packedBytes.end(), png.begin(), png.end());
    for (auto &image : packedJson["images"]) {
        image.erase("uri");
        image["bufferView"] = view;
        image["mimeType"] = "image/png";
    }
    packedJson["buffers"][0].erase("uri");
    packedJson["buffers"][0]["byteLength"] = packedBytes.size();
    auto text = packedJson.dump();
    while (text.size() % 4 != 0) {
        text.push_back(' ');
    }
    while (packedBytes.size() % 4 != 0) {
        packedBytes.push_back(0);
    }
    {
        std::ofstream glb(bonus / "model.glb", std::ios::binary);
        auto little = [&](std::uint32_t value) {
            for (int shift = 0; shift < 32; shift += 8) {
                glb.put(static_cast<char>(value >> shift));
            }
        };
        little(0x46546c67);
        little(2);
        little(static_cast<std::uint32_t>(28 + text.size() + packedBytes.size()));
        little(static_cast<std::uint32_t>(text.size()));
        little(0x4e4f534a);
        glb.write(text.data(), text.size());
        little(static_cast<std::uint32_t>(packedBytes.size()));
        little(0x004e4942);
        glb.write(reinterpret_cast<const char *>(packedBytes.data()), packedBytes.size());
    }
    std::filesystem::remove(bonus / "model.gltf");
    std::filesystem::remove(bonus / "mesh.bin");
    std::filesystem::remove(bonus / "base.png");
    auto packedLayers = coastline::DecalCar::Load(bonus);
    assert(packedLayers && packedLayers->PanelPixels(0) == source);
    assert(packedLayers->NoteHeader() == layers->NoteHeader());
    packedLayers->Texels(1)[offset] = 0x06;
    packedLayers->SetPaletteColor(1, 6, 0xf801);
    assert(packedLayers->PanelPixels(1) == expected);
    assert(packedLayers->WriteImages(bonus / "packed"));
    for (int panel = 0; panel < layers->PanelCount(); ++panel) {
        const auto file = "image-" + std::to_string(panel) + ".png";
        std::ifstream separate(bonus / "painted" / file, std::ios::binary);
        std::ifstream embedded(bonus / "packed" / file, std::ios::binary);
        const std::vector<char> expected(std::istreambuf_iterator<char>(separate), {});
        const std::vector<char> actual(std::istreambuf_iterator<char>(embedded), {});
        assert(actual == expected);
    }
    layers->SetPaletteColor(2, 0, 0xf800);
    auto tinted = source;
    for (std::size_t pixel = 0; pixel < tinted.size() / 4; ++pixel) {
        tinted[pixel * 4 + 1] = 0;
        tinted[pixel * 4 + 2] = 0;
    }
    assert(layers->PanelPixels(2) == tinted);
    std::filesystem::remove_all(root);
}
