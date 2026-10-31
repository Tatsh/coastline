#include "enginetextures.h"

#include <cassert>

#include <SDL3/SDL.h>

#include "common/hudassets.h"

namespace coastline {

namespace {

constexpr const char *ENGINE_TEXTURES_PROPERTY = "coastline.engine-textures";
constexpr int IMAGE_SIZE_8 = 1;
constexpr int IMAGE_FORMAT_CI = 2;

// A big-endian RGBA5551 colour as 8-bit RGBA.
void AppendColor(std::vector<std::uint8_t> &rgba, const unsigned char *half) {
    unsigned value = static_cast<unsigned>(half[0]) << 8 | half[1];
    rgba.push_back(static_cast<std::uint8_t>((value >> 11 & 0x1f) * 255 / 31));
    rgba.push_back(static_cast<std::uint8_t>((value >> 6 & 0x1f) * 255 / 31));
    rgba.push_back(static_cast<std::uint8_t>((value >> 1 & 0x1f) * 255 / 31));
    rgba.push_back((value & 1) != 0 ? 0xff : 0);
}

} // namespace

EngineTextures::EngineTextures() {
    assert(Active() == nullptr);
    m_backend.aliasTexture = [](int, int) {};
    m_backend.createTexture = [](const unsigned char *image,
                                 const unsigned char *palette,
                                 int width,
                                 int height,
                                 int lineBytes,
                                 int size,
                                 int format,
                                 int,
                                 int,
                                 int,
                                 int,
                                 int,
                                 int,
                                 int,
                                 int) -> unsigned {
        if (size != IMAGE_SIZE_8 || format != IMAGE_FORMAT_CI) {
            return 0;
        }
        return CreateEngineTexture(image, palette, width, height, lineBytes);
    };
    m_backend.decodeTexture = [](void *destination,
                                 const unsigned char *image,
                                 const unsigned char *palette,
                                 int width,
                                 int height,
                                 int lineBytes,
                                 int size,
                                 int format) {
        if (size != IMAGE_SIZE_8 || format != IMAGE_FORMAT_CI) {
            return 0;
        }
        return DecodeEngineTexture(destination, image, palette, width, height, lineBytes);
    };
    m_backend.initializeTextures = [] {};
    m_backend.initialize = [] {};
    m_backend.shutdown = [] {};
    m_backend.submitDisplayList = [](GbiCommand *) {};
    m_backend.updateFog = [] {};
    m_backend.shutdownTextures = [] { ClearEngineTextures(); };
    m_reflections.textures[0].image = m_reflections.daySky;
    m_reflections.textures[1].image = m_reflections.nightSky;
    m_reflections.textures[2].image = m_reflections.nightSky;
    m_reflections.textures[3].image = g_abTunnelReflectionTexture;
    m_reflections.selection[0][0] = 3;
    m_reflections.selection[0][1] = 3;
    m_reflections.selection[1][0] = 0;
    m_reflections.selection[1][1] = 1;
    m_reflections.selection[2][0] = 2;
    m_reflections.selection[2][1] = 2;
    SDL_SetPointerProperty(SDL_GetGlobalProperties(), ENGINE_TEXTURES_PROPERTY, this);
}

EngineTextures::~EngineTextures() {
    SDL_ClearProperty(SDL_GetGlobalProperties(), ENGINE_TEXTURES_PROPERTY);
}

EngineTextures *EngineTextures::Active() {
    return static_cast<EngineTextures *>(
        SDL_GetPointerProperty(SDL_GetGlobalProperties(), ENGINE_TEXTURES_PROPERTY, nullptr));
}

unsigned EngineTextures::Create(const unsigned char *image,
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
    m_textures.push_back(std::move(texture));
    return static_cast<unsigned>(m_textures.size());
}

unsigned CreateEngineTexture(const unsigned char *image,
                             const unsigned char *palette,
                             int width,
                             int height,
                             int lineBytes) {
    auto *textures = EngineTextures::Active();
    return textures ? textures->Create(image, palette, width, height, lineBytes) : 0;
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

void EngineTextures::Load(unsigned entry, const unsigned char *colors) {
    if (entry == 0 || entry > m_textures.size()) {
        return;
    }
    EngineTexture &texture = m_textures[entry - 1];
    texture.rgba.clear();
    for (int texel = 0; texel < texture.width * texture.height; ++texel) {
        AppendColor(texture.rgba, &colors[texel * 2]);
    }
    ++texture.version;
}

const EngineTexture *EngineTextures::Find(unsigned entry) const {
    return entry == 0 || entry > m_textures.size() ? nullptr : &m_textures[entry - 1];
}

void EngineTextures::Clear() {
    m_textures.clear();
}

void LoadEngineTexture(unsigned entry, const unsigned char *colors) {
    if (auto *textures = EngineTextures::Active()) {
        textures->Load(entry, colors);
    }
}

const EngineTexture *FindEngineTexture(unsigned entry) {
    const auto *textures = EngineTextures::Active();
    return textures ? textures->Find(entry) : nullptr;
}

void ClearEngineTextures() {
    if (auto *textures = EngineTextures::Active()) {
        textures->Clear();
    }
}

} // namespace coastline

extern "C" {

SdlRenderBackendState *GetSdlRenderBackendState(void) {
    auto *textures = coastline::EngineTextures::Active();
    assert(textures != nullptr);
    return &textures->Backend();
}

SdlVehicleReflections *GetSdlVehicleReflections(void) {
    auto *textures = coastline::EngineTextures::Active();
    assert(textures != nullptr);
    return &textures->Reflections();
}
}
