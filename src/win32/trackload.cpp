#include <bit>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "common/physics.h"
#include "common/trackmenuitem.h"
#include "common/util.h"
#include "win32/fileio.h"
#include "win32/render.h"
#include "win32/settings.h"
#include "win32/trackrelocation.h"
#if TGR_PLATFORM == TGR_PLATFORM_SDL
#include <filesystem>

#include "sdl/gamefiles.h"
#include "sdl/indexedimage.h"
#endif

namespace {
/**
 * @ghidraAddress BRD3D: 0x106c56ec
 * @ghidraAddress BRGlide: 0x106ec77c
 */
[[maybe_unused, gnu::used]] unsigned g_dwTrackDataRelocationDelta;
} // namespace

#if TGR_PLATFORM == TGR_PLATFORM_SDL
static void LoadSkyImage(unsigned char (&texels)[2048],
                         unsigned char (&palette)[32],
                         const SkyTextureFiles *files) {
    std::filesystem::path png = std::filesystem::path(files->image).replace_extension(".png");
    auto found = coastline::FindGameFile(png.string().c_str());
    if (!found || !coastline::LoadIndexed4Png(*found, texels, palette)) {
        std::printf("%s cannot be read.\n", png.string().c_str());
        std::exit(1);
    }
}
#endif

static inline bool IsUnitTrackScale(long double value) {
    return !(value < 1.0L || value > 1.0L);
}

void LoadTrackGeometryFromRom(s32 track) {
    LoadTrackTextureMap(track);
    constexpr unsigned originalBase = 0x80025c00u;
#if TGR_PLATFORM == TGR_PLATFORM_SDL
    unsigned geometryBase = coastline::EncodePtr32(&g_abTrackGeometryBuffer[0]);
#else
    unsigned geometryBase = std::bit_cast<unsigned>(&g_abTrackGeometryBuffer[0]);
#endif
    g_dwTextureMapAddressBias = originalBase - geometryBase;
    SetPointerRelocationBase(originalBase, geometryBase);
    SetCopyTextureDataOnRelocate(1);
    char path[1024];
    std::strcpy(path, g_szTracks);
    std::strcat(path, g_apszTrackFilenames[track]);
    GameFile *file = CHK_FReadOpen(path);
    long fileLength = GetFileLength(file);
    ReadAndRelocateTrackHeader(&g_LoadedTrack, file);
    if (fileLength > TRACK_MAX_SIZE) {
        std::printf("Track %d is too big (%d vs. %d)\n",
                    track,
                    static_cast<int>(fileLength),
                    TRACK_MAX_SIZE);
        std::exit(1);
    }
    CHK_FRead(g_abTrackGeometryBuffer + TRACK_HEADER_SIZE,
              1,
              static_cast<unsigned>(fileLength) - TRACK_HEADER_SIZE,
              file);
    CHK_FClose(file);
    ByteSwapTrackAnimationTable(&g_LoadedTrack);
#if TGR_PLATFORM == TGR_PLATFORM_SDL
    // The import converts each palette and image of a sky to one indexed PNG image.
    LoadSkyImage(g_abDaySkyTexture, g_abDaySkyPalette, g_aTrackDescriptors[track].daySkyFiles);
    LoadSkyImage(
        g_abNightSkyTexture, g_abNightSkyPalette, g_aTrackDescriptors[track].nightSkyFiles);
#else
    ReadEntireFile(g_abDaySkyPalette, g_aTrackDescriptors[track].daySkyFiles->palette, 32);
    ReadEntireFile(g_abDaySkyTexture, g_aTrackDescriptors[track].daySkyFiles->image, -1);
    ReadEntireFile(g_abNightSkyPalette, g_aTrackDescriptors[track].nightSkyFiles->palette, 32);
    ReadEntireFile(g_abNightSkyTexture, g_aTrackDescriptors[track].nightSkyFiles->image, -1);
#endif
    CreateDaySkyTexture();
    CreateNightSkyTexture();
    g_pTrackTextureTable = g_abTrackGeometryBuffer + g_LoadedTrack.size;
    g_dwTrackDataRelocationDelta = geometryBase - g_LoadedTrack.headerSize;
    ByteSwapTrackData(&g_LoadedTrack);
    g_dwStreamDmaPrevSegment = -1;
    g_dwStreamedSnowMode = -1;
    for (int i = 0; i < g_LoadedTrack.instanceCount; ++i) {
        Vec3f axis = {1.0f, 0.0f, 0.0f};
        Mtx3x3TransformVec(&axis, &axis, g_LoadedTrack.instances[i].matrix);
        long double length = Vec3fLength(&axis);
        if (length == 0.0L || std::isnan(length)) {
            continue;
        }
        long double scale = 1.0L / length;
        TrackInstance &instance = g_LoadedTrack.instances[i];
        if (IsUnitTrackScale(instance.matrix[0][0] * scale) &&
            IsUnitTrackScale(instance.matrix[1][1] * scale) &&
            IsUnitTrackScale(instance.matrix[2][2] * scale)) {
            // The original equality tests also accept unordered diagonal products.
            instance.flags |= TRACK_INSTANCE_UNROTATED;
        }
        instance.invScale = scale;
    }
    if (g_LoadedTrack.instanceCount > TRACK_MAX_INSTANCES) {
        std::printf("ERROR: instances (%d) > MAX_INSTANCES (%d)\n",
                    g_LoadedTrack.instanceCount,
                    TRACK_MAX_INSTANCES);
        std::exit(1);
    }
    if (g_LoadedTrack.headerSize != TRACK_HEADER_SIZE) {
        std::printf("Error: Track header size mismatch(%d != %d)\n",
                    g_LoadedTrack.headerSize,
                    TRACK_HEADER_SIZE);
        std::exit(1);
    }
    PrepareTrackSections();
    SelectVehiclePhysicsTables(track);
}
