// The byte swapping and the relocation of the tracks and the models the race engine of the PC
// release reads, on a 64-bit processor. The images of the console store big-endian 32-bit
// addresses, and the Windows libraries turn each into a pointer of 32 bits. The SDL game turns each
// into the 32-bit offset of common/ptr32.h, from the anchor of the static data. Every object the
// images address is static (the buffers of the track and the models, and the pool of the
// converted vertices here). The SDL renderer makes the textures of the converted images, and the
// routines of the renderers of the Windows libraries that make them are not called here.
#include "trackload.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <SDL3/SDL.h>

#include "common/convertedvertex.h"
#include "common/damagetexture.h"
#include "common/fileio.h"
#include "common/gbicommand.h"
#include "common/model.h"
#include "common/ptr32.h"
#include "common/relocation.h"
#include "common/renderbackend.h"
#include "common/settings.h"
#include "common/track.h"
#include "common/trackrelocation.h"
#include "common/vehiclemodel.h"
#include "common/weather.h"

using coastline::Ptr32;

unsigned char *g_pTrackTextureTable;
unsigned g_dwTextureMapAddressBias;

namespace coastline {

namespace {

// The vertices the pool of the converted vertices stores, as the Windows libraries size it.
constexpr std::size_t MODEL_VERTEX_POOL_SIZE = 32768;
constexpr int VERTEX_CONV_CACHE_SIZE = 0x800;
constexpr int TEXTURE_MAP_ENTRY_COUNT = 256;
// The palettes of a texture of 4 and of 8 bits a texel, in bytes.
constexpr unsigned PALETTE_CI4_BYTES = 32;
constexpr unsigned PALETTE_CI8_BYTES = 512;
// The address of the console the image of a car is based at.
constexpr unsigned CAR_IMAGE_BASE = 0x803c8000u;

struct VertexConvCacheEntry {
    const Vtx *source;
    int vertexCount;
    ConvertedModelVertex *converted;
};

struct TextureMapEntry {
    unsigned id;
    unsigned address;
    int width;
    int height;
};

/** The state of the relocation, the pool of the converted vertices, and the texture map. */
class ModelLoader {
public:
    static ModelLoader &Get() {
        static ModelLoader loader;
        return loader;
    }

    unsigned oldBase = 0;
    unsigned newBase = 0;
    int copyTextureData = 0;
    std::array<const void *, RELOCATED_POINTER_COUNT> relocated{};
    int relocatedCount = 0;
    std::array<VertexConvCacheEntry, VERTEX_CONV_CACHE_SIZE> cache{};
    int cacheCount = 0;
    std::size_t poolUsed = 0;
    unsigned convertedVertexCount = 0;
    std::array<TextureMapEntry, TEXTURE_MAP_ENTRY_COUNT> textureMap{};
    int textureMapCount = 0;
    unsigned hiResTextureMemoryThreshold = 0;

    ConvertedModelVertex *AllocateVertices(int count);

private:
    ModelLoader() = default;
};

// The pool is static data, addressed by a 32-bit offset from the anchor.
ConvertedModelVertex g_afModelVertexPool[MODEL_VERTEX_POOL_SIZE];

// The vertices of each car DeformVehicleMeshByImpact() moves, each with where it rested as the car
// loaded.
std::map<const VehicleModel *, std::vector<std::pair<const ConvertedModelVertex *, Vec3f>>>
    g_carRestPositions;

// The vertices the commands of the display lists of a car load, as DeformVehicleMeshByImpact()
// walks the lists.
void RecordCarRestPositions(const VehicleModel *model) {
    auto &rest = g_carRestPositions[model];
    rest.clear();
    std::set<const ConvertedModelVertex *> recorded;
    for (const auto &lod : model->displayLists) {
        for (const auto &list : lod) {
            for (const GbiCommand *command = list;
                 command != nullptr && command->w0 >> 24 != GBI_OP_END_DISPLAY_LIST;
                 ++command) {
                const ConvertedModelVertex *vertex = command->vertexData.pConverted;
                if (command->w0 >> 24 != GBI_OP_VERTEX || vertex == nullptr) {
                    continue;
                }
                for (unsigned count = command->w0 >> 10 & 0x3f; count != 0; --count, ++vertex) {
                    if (recorded.insert(vertex).second) {
                        rest.emplace_back(vertex, vertex->position);
                    }
                }
            }
        }
    }
}

// A position of a model, whose coordinates are those of the vertices of the console.
std::uint64_t PackPosition(float x, float y, float z) {
    return static_cast<std::uint64_t>(static_cast<std::uint16_t>(static_cast<std::int16_t>(x))) |
           static_cast<std::uint64_t>(static_cast<std::uint16_t>(static_cast<std::int16_t>(y)))
               << 16 |
           static_cast<std::uint64_t>(static_cast<std::uint16_t>(static_cast<std::int16_t>(z)))
               << 32;
}

ConvertedModelVertex *ModelLoader::AllocateVertices(int count) {
    if (poolUsed + static_cast<std::size_t>(count) > MODEL_VERTEX_POOL_SIZE) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "The pool of the converted vertices is full (%zu of %zu).",
                     poolUsed,
                     MODEL_VERTEX_POOL_SIZE);
        return nullptr;
    }
    ConvertedModelVertex *first = g_afModelVertexPool + poolUsed;
    poolUsed += static_cast<std::size_t>(count);
    convertedVertexCount += static_cast<unsigned>(count);
    return first;
}

unsigned SwapWord(unsigned word) {
    return std::byteswap(word);
}

unsigned short SwapHalf(unsigned short half) {
    return std::byteswap(half);
}

template <typename T>
    requires(std::is_trivially_copyable_v<T> && (sizeof(T) == 2 || sizeof(T) == 4))
void SwapValue(T &value) {
    if constexpr (sizeof(T) == 2) {
        value = std::bit_cast<T>(std::byteswap(std::bit_cast<u16>(value)));
    } else {
        value = std::bit_cast<T>(std::byteswap(std::bit_cast<u32>(value)));
    }
}

template <typename T>
void SwapValue(Ptr32<T> &pointer) {
    pointer.SetRaw(std::byteswap(pointer.Raw()));
}

template <typename T>
void RelocatePointer(Ptr32<T> &pointer) {
    unsigned word = pointer.Raw();
    RelocatePointerValue(&word);
    pointer.SetRaw(word);
}

template <typename T>
void SwapAndRelocatePointer(Ptr32<T> &pointer) {
    SwapValue(pointer);
    RelocatePointer(pointer);
}

void ByteSwapVertexArray(Vtx *vertices, int count) {
    for (int i = 0; i < count; i++) {
        for (auto &coordinate : vertices[i].v.ob) {
            coordinate = static_cast<short>(SwapHalf(static_cast<unsigned short>(coordinate)));
        }
        vertices[i].v.flag = SwapHalf(vertices[i].v.flag);
        for (auto &coordinate : vertices[i].v.tc) {
            coordinate = static_cast<short>(SwapHalf(static_cast<unsigned short>(coordinate)));
        }
    }
}

ConvertedModelVertex *ConvertVerticesToFloat(const Vtx *vertices, int count) {
    ConvertedModelVertex *first = ModelLoader::Get().AllocateVertices(count);
    if (first == nullptr) {
        return nullptr;
    }
    for (int i = 0; i < count; i++) {
        ConvertedModelVertex &vertex = first[i];
        vertex.position.x = vertices[i].n.ob[0];
        vertex.position.y = vertices[i].n.ob[1];
        vertex.position.z = vertices[i].n.ob[2];
        vertex.texture.x = vertices[i].n.tc[0];
        vertex.texture.y = vertices[i].n.tc[1];
        vertex.normal.x = vertices[i].n.n[0] * (1.0f / 128.0f);
        vertex.normal.y = vertices[i].n.n[1] * (1.0f / 128.0f);
        vertex.normal.z = vertices[i].n.n[2] * (1.0f / 128.0f);
    }
    return first;
}

// Swap the vertices of the console once and convert them to floats, reusing a conversion of the
// same vertices.
void ConvertN64VerticesCached(ModelVertexData *vertices, int count) {
    Vtx *source = vertices->pSource;
    if (source == nullptr || count == 0) {
        return;
    }
    ModelLoader &loader = ModelLoader::Get();
    for (int i = 0; i < loader.cacheCount; i++) {
        if (loader.cache[i].source == source && loader.cache[i].vertexCount == count) {
            vertices->pConverted = loader.cache[i].converted;
            return;
        }
    }
    ByteSwapVertexArray(source, count);
    ConvertedModelVertex *converted = ConvertVerticesToFloat(source, count);
    if (loader.cacheCount < VERTEX_CONV_CACHE_SIZE) {
        loader.cache[loader.cacheCount++] = {source, count, converted};
    }
    vertices->pConverted = converted;
}

void RelocateGbiVertexCommand(GbiCommand *command) {
    command->w1 = (ModelLoader::Get().oldBase & 0xff000000) | (command->w1 & 0xffffff);
    RelocatePointerValue(&command->w1);
    ConvertN64VerticesCached(&command->vertexData, command->w0 >> 10 & 0x3f);
}

void HalveTriangleVertexIndices(GbiCommand *command) {
    command->w1 = (command->w1 & 0xff000000) | (command->w1 >> 1 & 0x7f7f7f);
}

template <std::size_t Count>
void ConvertCarDisplayLists(Ptr32<GbiCommand> (&lists)[VEHICLE_LODS][Count]) {
    for (auto &lod : lists) {
        for (auto &commands : lod) {
            SwapValue(commands);
            RelocatePointer(commands);
            if (!IsPointerInRelocatedList(commands)) {
                ByteSwapAndRelocateDisplayList(commands);
            }
        }
    }
}

} // namespace

int LoadedTextureMapEntries() {
    return ModelLoader::Get().textureMapCount;
}

unsigned ConvertedVertexCount() {
    return ModelLoader::Get().convertedVertexCount;
}

std::vector<std::array<float, 3>> CarDentOffsets(const VehicleModel *model,
                                                 std::span<const std::array<float, 3>> rest) {
    auto found = g_carRestPositions.find(model);
    if (found == g_carRestPositions.end()) {
        return {};
    }
    std::unordered_map<std::uint64_t, std::array<float, 3>> moved;
    for (const auto &[vertex, position] : found->second) {
        std::array<float, 3> offset{vertex->position.x - position.x,
                                    vertex->position.y - position.y,
                                    vertex->position.z - position.z};
        if (offset != std::array<float, 3>{}) {
            moved.try_emplace(PackPosition(position.x, position.y, position.z), offset);
        }
    }
    if (moved.empty()) {
        return {};
    }
    std::vector<std::array<float, 3>> offsets(rest.size());
    for (std::size_t i = 0; i < rest.size(); ++i) {
        auto offset = moved.find(PackPosition(rest[i][0], rest[i][1], rest[i][2]));
        if (offset != moved.end()) {
            offsets[i] = offset->second;
        }
    }
    return offsets;
}

} // namespace coastline

using coastline::ModelLoader;

void SetPointerRelocationBase(unsigned dwOldBase, unsigned dwNewBase) {
    ResetVertexConversionCache();
    ModelLoader::Get().oldBase = dwOldBase;
    ModelLoader::Get().newBase = dwNewBase;
}

void ResetVertexConversionCache(void) {
    ModelLoader::Get().cacheCount = 0;
    ModelLoader::Get().relocatedCount = 0;
}

void AddPointerToRelocatedList(void *pPointer) {
    ModelLoader &loader = ModelLoader::Get();
    if (loader.relocatedCount < RELOCATED_POINTER_COUNT) {
        loader.relocated[static_cast<std::size_t>(loader.relocatedCount++)] = pPointer;
    }
}

int IsPointerInRelocatedList(void *pPointer) {
    if (pPointer == nullptr) {
        return true;
    }
    const ModelLoader &loader = ModelLoader::Get();
    return std::find(loader.relocated.begin(),
                     loader.relocated.begin() + loader.relocatedCount,
                     pPointer) != loader.relocated.begin() + loader.relocatedCount;
}

void RelocatePointerValue(unsigned *pdwPointer) {
    unsigned dwPointer = *pdwPointer;
    if (dwPointer == 0) {
        return;
    }
    const ModelLoader &loader = ModelLoader::Get();
    // The comparison is signed, as in the game.
    if (static_cast<int>(dwPointer) < static_cast<int>(loader.oldBase)) {
        *pdwPointer = 0;
        return;
    }
    // The new base is an offset from the anchor, and so is the sum.
    *pdwPointer = loader.newBase - loader.oldBase + dwPointer;
}

void SetCopyTextureDataOnRelocate(int bCopy) {
    ModelLoader::Get().copyTextureData = bCopy;
}

void ResetModelDataPool(void) {
    ModelLoader::Get().convertedVertexCount = 0;
    ModelLoader::Get().poolUsed = 0;
    coastline::g_carRestPositions.clear();
}

void ByteSwapArray16(unsigned short *pData, int nCount) {
    for (int i = 0; i < nCount; i++) {
        pData[i] = coastline::SwapHalf(pData[i]);
    }
}

void ByteSwapFourShorts(TrackSurface *pData) {
    for (unsigned short &vertex : pData->vertices) {
        vertex = coastline::SwapHalf(vertex);
    }
    pData->instance = coastline::SwapHalf(pData->instance);
}

void ByteSwapShortQuadArray(TrackSurface *pData, int nCount) {
    for (int i = 0; i < nCount; i++) {
        ByteSwapFourShorts(&pData[i]);
    }
}

void ByteSwap3DWords(Vec3f *pData) {
    coastline::SwapValue(pData->x);
    coastline::SwapValue(pData->y);
    coastline::SwapValue(pData->z);
}

void ByteSwap3DWordsArray(Vec3f *pData, int nCount) {
    for (int i = 0; i < nCount; i++) {
        ByteSwap3DWords(&pData[i]);
    }
}

void ByteSwapTerrainVertex(TrackPoint *point) {
    ByteSwap3DWords(&point->left);
    ByteSwap3DWords(&point->position);
    ByteSwap3DWords(&point->right);
    coastline::SwapValue(point->distance);
}

void ByteSwapAndRelocateDisplayList(GbiCommand *pCommands) {
    if (pCommands == nullptr) {
        return;
    }
    AddPointerToRelocatedList(pCommands);
    for (GbiCommand *pCommand = pCommands;; pCommand++) {
        pCommand->w0 = coastline::SwapWord(pCommand->w0);
        pCommand->w1 = coastline::SwapWord(pCommand->w1);
        switch (pCommand->w0 >> 24) {
        case G_VTX:
            coastline::RelocateGbiVertexCommand(pCommand);
            break;
        case G_TRI2:
            pCommand->w0 = (pCommand->w0 & 0xff000000) | (pCommand->w0 >> 1 & 0x7f7f7f);
            coastline::HalveTriangleVertexIndices(pCommand);
            break;
        case G_ENDDL:
            return;
        case G_TRI1:
            coastline::HalveTriangleVertexIndices(pCommand);
            break;
        case G_SETTIMG:
            RelocatePointerValue(&pCommand->w1);
            break;
        }
    }
}

void ByteSwapTextureVariantTable(TextureAnimation *pAnimation) {
    if (pAnimation == nullptr) {
        return;
    }
    pAnimation->frameCount = coastline::SwapHalf(pAnimation->frameCount);
    unsigned header;
    std::memcpy(&header, pAnimation->unreconstructed4, sizeof(header));
    header = coastline::SwapWord(header);
    std::memcpy(pAnimation->unreconstructed4, &header, sizeof(header));
    for (int i = 0; i < pAnimation->frameCount; i++) {
        pAnimation->frames[i].time = coastline::SwapWord(pAnimation->frames[i].time);
        pAnimation->frames[i].imageRom = coastline::SwapWord(pAnimation->frames[i].imageRom);
        pAnimation->frames[i].paletteRom = coastline::SwapWord(pAnimation->frames[i].paletteRom);
    }
}

void ByteSwapAndRelocateTextureDescriptor(ModelTexture *pTexture) {
    coastline::SwapAndRelocatePointer(pTexture->image);
    coastline::SwapAndRelocatePointer(pTexture->palette);
    pTexture->source.paletteRomIndex = coastline::SwapWord(pTexture->source.paletteRomIndex);
    coastline::SwapValue(pTexture->width);
    coastline::SwapValue(pTexture->height);
    coastline::SwapValue(pTexture->tileT0);
    coastline::SwapValue(pTexture->tileS0);
    coastline::SwapValue(pTexture->tileT1);
    coastline::SwapValue(pTexture->tileS1);
    pTexture->textureFlags = coastline::SwapWord(pTexture->textureFlags);
    if (!ModelLoader::Get().copyTextureData) {
        return;
    }
    unsigned paletteBytes = (pTexture->textureFlags & 0xf000000) == 0x1000000 ?
                                coastline::PALETTE_CI4_BYTES :
                                coastline::PALETTE_CI8_BYTES;
    if (pTexture->textureFlags & 0x100000) {
        RelocatePointerValue(&pTexture->source.paletteRomIndex);
        TextureAnimation *pAnimation = pTexture->source.animation;
        ByteSwapTextureVariantTable(pAnimation);
        // The sentinel suppresses image copying even when the selected image exists.
        int frame = pAnimation->frameCount == 2 && pAnimation->frames[0].time == 0xffffffff;
        unsigned imageOffset = pAnimation->frames[frame].imageRom;
        unsigned paletteOffset = pAnimation->frames[frame].paletteRom;
        unsigned imageBytes = pTexture->textureFlags & 0x3ffff;
        if (frame == 0 && imageBytes != 0 && imageOffset != 0xffffffff) {
            std::memcpy(pTexture->image, g_pTrackTextureTable + imageOffset, imageBytes);
        }
        if (pTexture->palette != nullptr && paletteOffset != 0xffffffff) {
            std::memcpy(pTexture->palette, g_pTrackTextureTable + paletteOffset, paletteBytes);
        }
    } else if (pTexture->palette != nullptr) {
        unsigned paletteOffset = (pTexture->source.paletteRomIndex & 0xfff) << 5;
        std::memcpy(pTexture->palette, g_pTrackTextureTable + paletteOffset, paletteBytes);
    }
}

void ByteSwapTextureDescriptorArray(ModelTexture *pTextures, int nCount) {
    for (int i = 0; i < nCount; i++) {
        ByteSwapAndRelocateTextureDescriptor(&pTextures[i]);
    }
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

void ByteSwapTrackInstance(TrackInstance *instance) {
    for (auto &row : instance->matrix) {
        for (float &value : row) {
            coastline::SwapValue(value);
        }
    }
    coastline::SwapValue(instance->invScale);
    coastline::SwapAndRelocatePointer(instance->displayList);
    coastline::SwapValue(instance->groups);
    coastline::SwapValue(instance->hidesGroups);
    coastline::SwapValue(instance->flags);
    coastline::SwapValue(instance->vertexCount);
    coastline::SwapValue(instance->triangleCount);
    coastline::SwapValue(instance->polygonCount);
    ByteSwapAndRelocateDisplayList(instance->displayList);
}

void ByteSwapTrackInstances(TrackHeader *track) {
    TrackInstance *instance = track->instances;
    for (int i = 0; i < track->instanceCount; ++i, ++instance) {
        ByteSwapTrackInstance(instance);
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
        std::memcpy(texture.image,
                    g_pTrackTextureTable + animation->frames[0].imageRom,
                    MODEL_TEXTURE_SIZE(&texture));
        if (texture.palette != nullptr) {
            /* Palette size follows the loaded track's table, even for another input table. */
            unsigned paletteSize =
                MODEL_TEXTURE_FORMAT(&g_LoadedTrack.textures[i]) == TEXTURE_FORMAT_CI4 ?
                    coastline::PALETTE_CI4_BYTES :
                    coastline::PALETTE_CI8_BYTES;
            std::memcpy(texture.palette,
                        g_pTrackTextureTable + animation->frames[0].paletteRom,
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
    if (g_dwCurrentWeather == WEATHER_SNOW) {
        CopyTextureAndPaletteFromSegment(track->textures, track->textureCount);
    }
    ByteSwapTrackInstances(track);
    ByteSwapArray16(track->gridInstanceStarts, 4097);
    ByteSwapArray16(track->gridInstances, track->gridInstanceStarts[4096]);
    ByteSwapTrackSegmentList(track);
    ByteSwapTrackVec3Array(track);
}

void LoadTextureMapFile(const char *path) {
    ModelLoader &loader = ModelLoader::Get();
    char line[1024];
    loader.textureMapCount = 0;
    loader.hiResTextureMemoryThreshold = 0x300000;
    if (!CHK_FileExists(path)) {
        return;
    }
    GameFile *file = CHK_FReadOpen(path);
    if (FileReadLine(line, sizeof(line), file) != nullptr) {
        std::sscanf(line, "%u", &loader.hiResTextureMemoryThreshold);
        while (loader.textureMapCount < coastline::TEXTURE_MAP_ENTRY_COUNT &&
               FileReadLine(line, sizeof(line), file) != nullptr) {
            coastline::TextureMapEntry &entry =
                loader.textureMap[static_cast<std::size_t>(loader.textureMapCount)];
            std::sscanf(
                line, "%u %x %d %d", &entry.id, &entry.address, &entry.width, &entry.height);
            loader.textureMapCount++;
        }
    }
    CHK_FClose(file);
}

void LoadTrackTextureMap(int track) {
    char path[1024];
    std::snprintf(path, sizeof(path), "%s%s", g_szTracks, g_apszTrackFilenames[track]);
    // The SDL game takes the texture map of the Direct3D library.
    if (char *extension = std::strrchr(path, '.')) {
        std::strcpy(extension, ".hnd");
    }
    LoadTextureMapFile(path);
}

void ByteSwapAndRelocateCarModel(VehicleModel *model) {
    SetPointerRelocationBase(coastline::CAR_IMAGE_BASE,
                             coastline::EncodePtr32(&model->triangleCount));
    SetCopyTextureDataOnRelocate(0);
    g_pTextureCaptureVehicleModel = model;
    model->textureEntryCount = 0;
    coastline::SwapValue(model->triangleCount);
    coastline::SwapAndRelocatePointer(model->unreconstructed4);
    coastline::SwapValue(model->unreconstructed8);
    coastline::SwapAndRelocatePointer(model->unreconstructedC);
    coastline::SwapValue(model->textureCount);
    coastline::SwapAndRelocatePointer(model->textures);
    ByteSwapTextureDescriptorArray(model->textures, model->textureCount);
    u16 *palette = model->textures[model->materialTextures[VEHICLE_MATERIAL_HEADLIGHTS]].palette;
    if (palette != nullptr) {
        for (s32 i = 12; i < 16; i++) {
            if (g_dwNightMode == 0 && g_dwRainMode == 0) {
                palette[i] |= 0x0100;
            } else {
                palette[i] &= 0xfeff;
            }
        }
    }
    ClearDamageTextureEntries();
    coastline::ConvertCarDisplayLists(model->displayLists);
    coastline::RecordCarRestPositions(model);
    coastline::SwapValue(model->unreconstructed90);
    /* The original relocates the following field without swapping it. */
    coastline::RelocatePointer(model->unreconstructed94);
    for (std::size_t i = 0; i < sizeof(model->unreconstructed98); i += sizeof(u32)) {
        u32 word;
        std::memcpy(&word, &model->unreconstructed98[i], sizeof(word));
        word = std::byteswap(word);
        std::memcpy(&model->unreconstructed98[i], &word, sizeof(word));
    }
    coastline::SwapValue(model->cockpitForward);
    coastline::SwapValue(model->mirrorForward);
    coastline::SwapValue(model->cockpitUp);
    coastline::ConvertCarDisplayLists(model->wheelDisplayLists);
    for (auto &wheel : model->wheels) {
        coastline::SwapValue(wheel.x);
        coastline::SwapValue(wheel.y);
        coastline::SwapValue(wheel.z);
    }
    coastline::SwapAndRelocatePointer(model->decalMasks);
}

void RelocateModelPointers(Model *model) {
    coastline::SwapValue(model->wUnknown0);
    coastline::SwapValue(model->partCount);
    if (model->animations.Raw() != 0) {
        coastline::SwapAndRelocatePointer(model->animations);
        ModelAnimationList *animations = model->animations;
        coastline::SwapValue(animations->count);
        for (int i = 0; i < animations->count; ++i) {
            coastline::SwapAndRelocatePointer(animations->entries[i]);
            ModelAnimation *animation = animations->entries[i];
            coastline::SwapValue(animation->vertexCount);
            coastline::SwapAndRelocatePointer(animation->vertices.pSource);
            coastline::ConvertN64VerticesCached(&animation->vertices, animation->vertexCount);
            coastline::SwapAndRelocatePointer(animation->unreconstructed8);
            coastline::SwapValue(animation->keyCount);
            coastline::SwapValue(animation->flags);
            coastline::SwapValue(animation->startKey);
            coastline::SwapValue(animation->startTime);
            coastline::SwapValue(animation->endTime);
            coastline::SwapValue(animation->time);
            for (int k = 0; k < animation->keyCount; ++k) {
                coastline::SwapAndRelocatePointer(animation->keys[k]);
                ModelKeyframe *key = animation->keys[k];
                coastline::SwapValue(key->time);
                for (int j = 0; j < static_cast<int>(static_cast<u32>(animation->vertexCount) * 3u);
                     ++j) {
                    coastline::SwapValue(key->positions[j / 3][j % 3]);
                }
            }
        }
    }
    for (int i = 0; i < model->partCount; ++i) {
        if (model->parts[i].displayList.Raw() != 0) {
            coastline::SwapAndRelocatePointer(model->parts[i].displayList);
            coastline::SwapValue(model->parts[i].flags);
            coastline::SwapValue(model->parts[i].wUnknown6);
            coastline::SwapValue(model->parts[i].position.x);
            coastline::SwapValue(model->parts[i].position.y);
            coastline::SwapValue(model->parts[i].position.z);
            ByteSwapAndRelocateDisplayList(model->parts[i].displayList);
        }
    }
}
