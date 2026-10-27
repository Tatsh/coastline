#include <cstring>

#include "platform/win/render.h"
#include "platform/win/trackrelocation.h"

void ComputeTrackMaxCellIndex(TrackHeader *track) {
    int maximum = 0;
    int count = track->gridCellStarts[4096];
    for (int i = 1; i < count; i++) {
        if (track->gridTriangles[i] > maximum) {
            maximum = track->gridTriangles[i];
        }
    }
    track->surfaceCount = maximum + 1;
}

void ByteSwapTerrainVertex(TrackPoint *point) {
    ByteSwap3DWords(&point->left);
    ByteSwap3DWords(&point->position);
    ByteSwap3DWords(&point->right);
    unsigned distance;
    std::memcpy(&distance, &point->distance, sizeof(distance));
    distance = ((distance & 0x000000ffu) << 24) | ((distance & 0x0000ff00u) << 8) |
               ((distance & 0x00ff0000u) >> 8) | ((distance & 0xff000000u) >> 24);
    std::memcpy(&point->distance, &distance, sizeof(distance));
}

static inline void CopyTrackTextureBytes(void *destination, const u8 *source, unsigned count) {
    u8 *output = static_cast<u8 *>(destination);
    for (unsigned wordIndex = 0; wordIndex < count / sizeof(unsigned); wordIndex++) {
        unsigned word;
        std::memcpy(&word, source, sizeof(word));
        std::memcpy(output, &word, sizeof(word));
        source += sizeof(word);
        output += sizeof(word);
    }
    for (unsigned byteIndex = 0; byteIndex < count % sizeof(unsigned); byteIndex++) {
        *output++ = *source++;
    }
}

void CopyTextureAndPaletteFromSegment(ModelTexture *textures, int count) {
    for (int i = 0; i < count; i++) {
        ModelTexture &texture = textures[i];
        if (texture.image == nullptr || !MODEL_TEXTURE_ANIMATED(&texture)) {
            continue;
        }
        TextureAnimation *animation = texture.source.animation;
        if (animation->frameCount != 2 || animation->frames[0].time != 0xffffffffu ||
            MODEL_TEXTURE_SIZE(&texture) == 0) {
            continue;
        }
        CopyTrackTextureBytes(texture.image,
                              g_pTrackTextureTable + animation->frames[0].imageRom,
                              MODEL_TEXTURE_SIZE(&texture));
        if (texture.palette != nullptr) {
            /* Palette size follows the loaded track's table, even for another input table. */
            unsigned paletteSize =
                MODEL_TEXTURE_FORMAT(&g_LoadedTrack.textures[i]) == TEXTURE_FORMAT_CI4 ? 32 : 512;
            CopyTrackTextureBytes(texture.palette,
                                  g_pTrackTextureTable +
                                      texture.source.animation->frames[0].paletteRom,
                                  paletteSize);
        }
    }
}

void ByteSwapTrackVec3Array(TrackHeader *track) {
    Vec3f *point = track->cameraPoints;
    for (int i = 0; i < track->cameraPointCount; i++, point++) {
        ByteSwap3DWords(point);
    }
}
