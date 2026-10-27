#include <cstdio>
#include <cstring>

#include "platform/win/fileio.h"
#include "platform/win/render.h"
#include "platform/win/settings.h"
#include "platform/win/texturescan.h"
#include "platform/win/trackrelocation.h"
#include "weather.h"

void ByteSwapTrackData(TrackHeader *track) {
    ByteSwap3DWordsArray(track->vertices, track->vertexCount);
    ByteSwapTextureDescriptorArray(track->textures, track->textureCount);
    ByteSwapArray16(track->gridCellStarts, 4097);
    ByteSwapArray16(track->gridTriangles, track->gridCellStarts[4096]);
    // Both maximum-index scans deliberately omit element zero.
    int maxTriangle = 0;
    for (int i = track->gridCellStarts[4096] - 1; i > 0; --i) {
        if (track->gridTriangles[i] > maxTriangle) {
            maxTriangle = track->gridTriangles[i];
        }
    }
    ByteSwapArray16(track->triangleNearbyInstances, maxTriangle + 1);
    int lastNearbyStart = 0;
    for (int i = maxTriangle; i > 0; --i) {
        if (track->triangleNearbyInstances[i] > lastNearbyStart) {
            lastNearbyStart = track->triangleNearbyInstances[i];
        }
    }
    int nearbyCount = lastNearbyStart;
    while (track->nearbyInstances[nearbyCount] != 0) {
        ++nearbyCount;
    }
    ByteSwapArray16(track->nearbyInstances, nearbyCount);
    ComputeTrackMaxCellIndex(track);
    ByteSwapShortQuadArray(track->surfaces, track->surfaceCount);
    ByteSwapAndRelocateDisplayList(track->skyDisplayList);
    SetTextureLoadCategory(4);
    if (g_dwCurrentWeather == WEATHER_SNOW) {
        CopyTextureAndPaletteFromSegment(track->textures, track->textureCount);
    }
    g_pfnPreprocessDisplayListTextures(track->skyDisplayList);
    ByteSwapTrackInstances(track);
    SetTextureLoadCategory(1);
    g_pfnUploadTrackTextureVariants(track->textures, track->textureCount);
    ByteSwapArray16(track->gridInstanceStarts, 4097);
    ByteSwapArray16(track->gridInstances, track->gridInstanceStarts[4096]);
    ByteSwapTrackSegmentList(track);
    ByteSwapTrackVec3Array(track);
}

void LoadTextureMapFile(const char *path) {
    char line[1024];
    g_dwTextureMapEntryCount = 0;
    g_dwHiResTextureMemoryThreshold = 0x300000;
    if (CHK_FileExists(path)) {
        GameFile *file = CHK_FReadOpen(path);
        if (FileReadLine(line, sizeof(line), file) != nullptr) {
            std::sscanf(line, "%u", &g_dwHiResTextureMemoryThreshold);
            if (FileReadLine(line, sizeof(line), file) != nullptr) {
                do {
                    TextureMapEntry &entry = g_aTextureMapEntries[g_dwTextureMapEntryCount];
                    std::sscanf(line,
                                "%u %x %d %d",
                                &entry.nId,
                                &entry.dwAddress,
                                &entry.nWidth,
                                &entry.nHeight);
                    g_dwTextureMapEntryCount++;
                    if (g_dwTextureMapEntryCount >= TEXTURE_MAP_ENTRY_COUNT) {
                        break;
                    }
                } while (FileReadLine(line, sizeof(line), file) != nullptr);
            }
        }
        CHK_FClose(file);
    }
    SelectTextureQualityFromVideoMemory();
}

void LoadTrackTextureMap(int track) {
    char path[1024];
    std::sprintf(path, "%s%s", g_szTracks, g_apszTrackFilenames[track]);
    /* Extension replacement starts at the first dot in the complete path. */
    std::strcpy(std::strchr(path, '.'), ".hnd");
    LoadTextureMapFile(path);
}

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
