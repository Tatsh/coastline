// The textures every race uses, as CreateGlobalEffectTextures() of BRD3D creates them. RaceView
// loads the overlay, the particles, the trails of the tyres, and the maps of the environment from
// the import, and MenuText draws the glyphs of the display of the race. The textures of the
// precipitation are the texels the race writes here, 8-bit RGBA.
#include <array>

#include "common/renderbackend.h"
#include "common/weather.h"

namespace {

// The texels of CreateAlphaRampTextures(): white, each alpha of 4 bits widened to 8.
constexpr int ALPHA_RAMP_STEPS = 16;
constexpr int RAMP_TEXEL_BYTES = 4;
std::array<u8, ALPHA_RAMP_STEPS * RAMP_TEXEL_BYTES> g_alphaRamp;

// The size of the textures of the precipitation, which the SDL game does not reduce.
int g_precipitationWidth;
int g_precipitationHeight;

} // namespace

void CreateGlobalEffectTextures(void) {
    for (int alpha = 0; alpha < ALPHA_RAMP_STEPS; ++alpha) {
        u8 *texel = &g_alphaRamp[static_cast<std::size_t>(alpha * RAMP_TEXEL_BYTES)];
        texel[0] = 255;
        texel[1] = 255;
        texel[2] = 255;
        texel[3] = static_cast<u8>(alpha << 4 | alpha);
    }
    g_dwAlphaRampStripRowCount = RAMP_TEXEL_BYTES;
}

void GetPrecipitationTextureDimensions(int nIndex) {
    g_precipitationWidth = RAIN_STREAK_TEXTURE_SIDE;
    g_precipitationHeight = RAIN_STREAK_TEXTURE_SIDE;
}

void CopyTileColumnScaled(int nTileIndex, int nSrcX, int nSrcY, int nSrcRow) {
    int row = nSrcX * g_precipitationHeight / RAIN_STREAK_TEXTURE_SIDE;
    int column = nSrcY * g_precipitationWidth / RAIN_STREAK_TEXTURE_SIDE;
    for (int byte = 0; byte < g_dwAlphaRampStripRowCount; ++byte) {
        int destination = (row * g_precipitationWidth + column) * g_dwAlphaRampStripRowCount + byte;
        g_abPrecipitationTextures[nTileIndex][destination] =
            g_alphaRamp[static_cast<std::size_t>(nSrcRow * g_dwAlphaRampStripRowCount + byte)];
    }
}
