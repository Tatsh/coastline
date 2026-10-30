#include "enginetextures.h"

namespace coastline {

namespace {

std::vector<EngineTexture> g_engineTextures;

// A big-endian RGBA5551 colour as 8-bit RGBA.
void AppendColor(std::vector<std::uint8_t> &rgba, const unsigned char *half) {
    unsigned value = static_cast<unsigned>(half[0]) << 8 | half[1];
    rgba.push_back(static_cast<std::uint8_t>((value >> 11 & 0x1f) * 255 / 31));
    rgba.push_back(static_cast<std::uint8_t>((value >> 6 & 0x1f) * 255 / 31));
    rgba.push_back(static_cast<std::uint8_t>((value >> 1 & 0x1f) * 255 / 31));
    rgba.push_back((value & 1) != 0 ? 0xff : 0);
}

} // namespace

unsigned CreateEngineTexture(const unsigned char *image,
                             const unsigned char *palette,
                             int width,
                             int height,
                             int lineBytes) {
    EngineTexture texture;
    texture.width = width;
    texture.height = height;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            AppendColor(texture.rgba, &palette[image[y * lineBytes + x] * 2]);
        }
    }
    g_engineTextures.push_back(std::move(texture));
    return static_cast<unsigned>(g_engineTextures.size());
}

int DecodeEngineTexture(void *destination,
                        const unsigned char *image,
                        const unsigned char *palette,
                        int width,
                        int height,
                        int lineBytes) {
    auto *out = static_cast<unsigned char *>(destination);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const unsigned char *half = &palette[image[y * lineBytes + x] * 2];
            *out++ = half[0];
            *out++ = half[1];
        }
    }
    return width * height * 2;
}

void LoadEngineTexture(unsigned entry, const unsigned char *colors) {
    if (entry == 0 || entry > g_engineTextures.size()) {
        return;
    }
    EngineTexture &texture = g_engineTextures[entry - 1];
    texture.rgba.clear();
    for (int texel = 0; texel < texture.width * texture.height; ++texel) {
        AppendColor(texture.rgba, &colors[texel * 2]);
    }
    ++texture.version;
}

const EngineTexture *FindEngineTexture(unsigned entry) {
    return entry == 0 || entry > g_engineTextures.size() ? nullptr : &g_engineTextures[entry - 1];
}

void ClearEngineTextures() {
    g_engineTextures.clear();
}

} // namespace coastline
