#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

#include "n64model.h"

int main() {
    constexpr std::uint32_t base = 0x803c8000;
    std::vector<std::uint8_t> bytes(0x700);
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
        word(list + 4, base + 0x600);
        word(list + 8, 0xbf000000);
        word(list + 12, 0x00000204);
        word(list + 16, 0xb8000000);
    }
    auto texture = [&](int entry, std::uint32_t texels, std::uint32_t palette) {
        const auto at = 0x280 + entry * 36;
        word(at, base + texels);
        word(at + 4, base + palette);
        half(at + 12, 4);
        half(at + 14, 4);
        word(at + 32, 0x01000008);
    };
    texture(0, 0x400, 0x500);
    texture(3, 0x410, 0x500);
    texture(6, 0x420, 0x520);
    for (std::size_t at = 0x400; at < 0x428; ++at) {
        bytes[at] = static_cast<std::uint8_t>(at);
    }
    for (int entry = 0; entry < 16; ++entry) {
        half(0x500 + entry * 2, static_cast<std::uint16_t>(entry * 0x842 | 1));
    }
    std::copy_n(bytes.begin() + 0x500, 32, bytes.begin() + 0x520);
    const auto model = coastline::CarModelToGltf(bytes, 0, "mesh.bin");
    assert(model && model->images.size() == 2);
    for (std::size_t index = 0; index < model->images.size(); ++index) {
        const auto &image = model->images[index];
        if (image.address == 0x410) {
            assert(image.paintMaterial == 11 && image.indices.size() == 8);
            assert(image.palette.size() == 32);
            assert(std::equal(image.palette.begin(), image.palette.end(), bytes.begin() + 0x500));
            assert(model->json["images"][index]["extras"]["coastlinePaint"]["material"] == 11);
        } else {
            assert(image.address == 0x420);
            assert(image.paintMaterial == -1 && image.indices.empty() && image.palette.empty());
        }
    }
    const auto rebuilt = coastline::ModelFromGltf(model->json, model->buffer);
    assert(rebuilt && *rebuilt == bytes);
}
