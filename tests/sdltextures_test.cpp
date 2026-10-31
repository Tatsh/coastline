#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <SDL3/SDL.h>

#include "common/hudassets.h"
#include "enginetextures.h"

extern "C" {
unsigned char g_abTunnelReflectionTexture[2048];
}

namespace {

void Check(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "Failed. " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

} // namespace

int main() {
    using namespace coastline;
    Check(SDL_Init(0), "SDL properties start");
    const std::array<unsigned char, 6> image{1, 2, 255, 3, 0, 255};
    std::array<unsigned char, 512> palette{};
    palette[2] = 0xF8;
    palette[3] = 0x01;
    palette[4] = 0x07;
    palette[5] = 0xC1;
    palette[7] = 0x3F;
    const std::vector<std::uint8_t> rgba{
        255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 0, 0, 0, 0};
    const std::array<unsigned char, 8> colors{0xF8, 0x01, 0x07, 0xC1, 0, 0x3F, 0, 0};
    for (int run = 0; run < 100; ++run) {
        {
            EngineTextures textures;
            Check(EngineTextures::Active() == &textures, "the collection is active in its scope");
            Check(g_dwHudOverlayTextureId == 0 && g_dwHudOverlayHalfTextureId == 0 &&
                      g_dwParticleTextureId == 0 && g_dwTireTrailTextureId == 0 &&
                      g_dwVehicleEnvMapTextureId == 0 && g_dwFallbackEnvMapTextureId == 0 &&
                      g_dwAlphaRampStripRowCount == 0 && g_bDisableCarShadow == 0 &&
                      g_bForceTextureClone == 0 && g_adwPrecipitationTextureIds[0] == 0 &&
                      g_adwPrecipitationTextureIds[1] == 0,
                  "fresh collections reset effect texture entries and renderer settings");
            Check(std::ranges::all_of(g_abDaySkyTexture, [](auto byte) { return byte == 0; }) &&
                      std::ranges::all_of(g_abNightSkyTexture, [](auto byte) { return byte == 0; }),
                  "fresh collections clear sky images");
            Check(g_apVehicleReflectionTextures[0].image == g_abDaySkyTexture &&
                      g_apVehicleReflectionTextures[1].image == g_abNightSkyTexture &&
                      g_apVehicleReflectionTextures[2].image == g_abNightSkyTexture &&
                      g_apVehicleReflectionTextures[3].image == g_abTunnelReflectionTexture,
                  "reflection descriptors reference the current images");
            Check(g_abVehicleReflectionTexSelect[0][0] == 3 &&
                      g_abVehicleReflectionTexSelect[0][1] == 3 &&
                      g_abVehicleReflectionTexSelect[1][0] == 0 &&
                      g_abVehicleReflectionTexSelect[1][1] == 1 &&
                      g_abVehicleReflectionTexSelect[2][0] == 2 &&
                      g_abVehicleReflectionTexSelect[2][1] == 2,
                  "fresh collections restore reflection selection");
            auto create = [&](int size, int format) {
                return g_pfnCreateTextureFromN64Image(
                    image.data(), palette.data(), 2, 2, 3, size, format, 0, 0, 0, 0, 0, 0, 0, 0);
            };
            Check(create(0, 2) == 0 && create(1, 0) == 0,
                  "unsupported image formats do not allocate texture entries");
            Check(create(1, 2) == 1, "texture entries restart from one for each collection");
            const auto *texture = FindEngineTexture(1);
            Check(texture && texture->width == 2 && texture->height == 2 && texture->rgba == rgba &&
                      texture->version == 0,
                  "the renderer hook decodes padded indexed rows and RGBA5551 colors");
            std::array<unsigned char, 8> decoded{};
            Check(g_pfnDecodeN64TextureToPixelBuffer(
                      decoded.data(), image.data(), palette.data(), 2, 2, 3, 1, 2) == 8 &&
                      decoded == colors,
                  "the decode hook preserves big-endian color bytes");
            Check(g_pfnDecodeN64TextureToPixelBuffer(
                      decoded.data(), image.data(), palette.data(), 2, 2, 3, 0, 2) == 0,
                  "the decode hook rejects an unsupported format");
            LoadEngineTexture(1, colors.data());
            Check(texture->rgba == rgba && texture->version == 1,
                  "reloading texture colors increments the revision");
            LoadEngineTexture(0, colors.data());
            LoadEngineTexture(2, colors.data());
            Check(!FindEngineTexture(0) && !FindEngineTexture(2),
                  "invalid texture entries are ignored");
            g_pfnAliasTextureEntrySurface(1, 1);
            g_pfnInitializeTextureSubsystem();
            g_pfnRenderBackendInit();
            g_pfnRenderBackendUpdateFog();
            g_pfnRenderBackendSubmitDisplayList(nullptr);
            g_pfnRenderBackendShutdown();
            g_pfnShutdownTextureSubsystem();
            Check(!FindEngineTexture(1), "texture shutdown discards decoded entries");
            g_dwHudOverlayTextureId = 1;
            g_dwHudOverlayHalfTextureId = 2;
            g_dwParticleTextureId = 3;
            g_dwTireTrailTextureId = 4;
            g_dwVehicleEnvMapTextureId = 5;
            g_dwFallbackEnvMapTextureId = 6;
            g_dwAlphaRampStripRowCount = 7;
            g_bDisableCarShadow = 1;
            g_bForceTextureClone = 1;
            g_adwPrecipitationTextureIds[0] = 8;
            g_adwPrecipitationTextureIds[1] = 9;
            std::ranges::fill(g_abDaySkyTexture, 0xFF);
            std::ranges::fill(g_abNightSkyTexture, 0xFF);
            g_apVehicleReflectionTextures[0].entry = 100;
            g_abVehicleReflectionTexSelect[0][0] = 0;
            textures.Backend().createTexture = nullptr;
            textures.Backend().decodeTexture = nullptr;
            textures.Backend().shutdownTextures = nullptr;
        }
        Check(!EngineTextures::Active() && !FindEngineTexture(1),
              "collection destruction removes the active texture bridge");
        Check(CreateEngineTexture(image.data(), palette.data(), 2, 2, 3) == 0,
              "texture creation outside an active engine returns zero");
    }
    SDL_Quit();
    return EXIT_SUCCESS;
}
