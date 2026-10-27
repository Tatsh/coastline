#include <bit>
#include <cstring>
#include <type_traits>

#include "displaylistops.h"
#include "model.h"
#include "platform/win/boot/app.h"
#include "platform/win/render.h"
#include "platform/win/texturescan.h"
#include "platform/win/trackrelocation.h"
#include "vehiclecolor.h"
#include "weather.h"

template <typename T>
    requires(std::is_trivially_copyable_v<T> && (sizeof(T) == 2 || sizeof(T) == 4))
static inline void SwapModelValue(T &value) {
    if constexpr (sizeof(T) == 2) {
        value = std::bit_cast<T>(std::byteswap(std::bit_cast<u16>(value)));
    } else {
        value = std::bit_cast<T>(std::byteswap(std::bit_cast<u32>(value)));
    }
}

template <typename T>
    requires((std::is_object_v<T> || std::is_void_v<T>) && sizeof(T *) == sizeof(unsigned))
static inline void RelocateModelPointer(T *&pointer) {
    unsigned word = std::bit_cast<unsigned>(pointer);
    RelocatePointerValue(&word);
    pointer = std::bit_cast<T *>(word);
}

template <size_t Size>
    requires(Size % sizeof(u32) == 0)
static inline void SwapModelWords(u8 (&bytes)[Size]) {
    for (size_t i = 0; i < Size; i += sizeof(u32)) {
        u32 word;
        std::memcpy(&word, &bytes[i], sizeof(word));
        word = std::byteswap(word);
        std::memcpy(&bytes[i], &word, sizeof(word));
    }
}

template <size_t Count>
    requires(Count == VEHICLE_DISPLAY_LISTS || Count == VEHICLE_WHEEL_DISPLAY_LISTS)
static inline void ConvertCarDisplayLists(GbiCommand *(&lists)[VEHICLE_LODS][Count]) {
    for (auto &lod : lists) {
        for (auto &commands : lod) {
            SwapModelValue(commands);
            RelocateModelPointer(commands);
            if (!IsPointerInRelocatedList(commands)) {
                ByteSwapAndRelocateDisplayList(commands);
                SetTextureLoadCategory(2);
                g_pfnPreprocessDisplayListTextures(commands);
            }
        }
    }
}

static inline void CreateCarLightTextures(VehicleModel *model) {
    ModelTexture *texture = &model->textures[model->materialTextures[VEHICLE_MATERIAL_REAR_LIGHTS]];
    u16 *palette = texture->palette;
    if (palette == nullptr || g_dwNullStubEnable) {
        model->lightTextureEntries[3] = 0;
        model->lightTextureEntries[2] = 0;
        model->lightTextureEntries[1] = 0;
        model->lightTextureEntries[0] = 0;
        model->rearLightTextureEntry = 0;
        return;
    }
    u8 *image = texture->image;
    g_pfnCreateDeviceTextureFromImageHeader(texture, 1);
    model->rearLightTextureEntry = texture->textureEntry;
    for (s32 i = 0; i < 4; i++) {
        SetVehicleLightPalette(palette, i, i == 0 && (g_dwNightMode || g_dwRainMode));
        model->lightTextureEntries[i] = g_pfnCloneTextureEntryWithNewImageData(
            model->rearLightTextureEntry, image, std::bit_cast<const unsigned char *>(palette));
    }
}

void ByteSwapAndRelocateCarModel(VehicleModel *model) {
    SetPointerRelocationBase(0x803c8000, std::bit_cast<unsigned>(&model->triangleCount));
    SetCopyTextureDataOnRelocate(0);
    g_pTextureCaptureVehicleModel = model;
    model->textureEntryCount = 0;
    SwapModelValue(model->triangleCount);
    SwapModelValue(model->unreconstructed4);
    RelocateModelPointer(model->unreconstructed4);
    SwapModelValue(model->unreconstructed8);
    SwapModelValue(model->unreconstructedC);
    RelocateModelPointer(model->unreconstructedC);
    SwapModelValue(model->textureCount);
    SwapModelValue(model->textures);
    RelocateModelPointer(model->textures);
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
    ClearDamageTextureLevels();
    ConvertCarDisplayLists(model->displayLists);
    SwapModelValue(model->unreconstructed90);
    /* The original relocates the following field without swapping it. */
    RelocateModelPointer(model->unreconstructed94);
    SwapModelWords(model->unreconstructed98);
    SwapModelValue(model->cockpitForward);
    SwapModelValue(model->mirrorForward);
    SwapModelValue(model->cockpitUp);
    ConvertCarDisplayLists(model->wheelDisplayLists);
    SetTextureLoadCategory(2);
    g_pfnCreateDeviceTextureFromImageHeader(&model->textures[6], 0);
    g_pfnCreateDeviceTextureFromImageHeader(&model->textures[3], 0);
    g_pfnCreateDeviceTextureFromImageHeader(&model->textures[5], 0);
    CreateCarLightTextures(model);
    for (auto &wheel : model->wheels) {
        SwapModelValue(wheel.x);
        SwapModelValue(wheel.y);
        SwapModelValue(wheel.z);
    }
    SwapModelValue(model->decalMasks);
    RelocateModelPointer(model->decalMasks);
}

void ByteSwapTrackInstance(TrackInstance *instance) {
    for (auto &row : instance->matrix) {
        for (float &value : row) {
            SwapModelValue(value);
        }
    }
    SwapModelValue(instance->invScale);
    SwapModelValue(instance->displayList);
    RelocateModelPointer(instance->displayList);
    SwapModelValue(instance->groups);
    SwapModelValue(instance->hidesGroups);
    SwapModelValue(instance->flags);
    SwapModelValue(instance->vertexCount);
    SwapModelValue(instance->triangleCount);
    SwapModelValue(instance->polygonCount);
    ByteSwapAndRelocateDisplayList(instance->displayList);
    ProcessTrackNodeDisplayList(instance);
    SetTextureLoadCategory(1);
    g_pfnPreprocessDisplayListTextures(instance->displayList);
}

void ByteSwapTrackInstances(TrackHeader *track) {
    TrackInstance *instance = track->instances;
    for (int i = 0; i < track->instanceCount; ++i, ++instance) {
        ByteSwapTrackInstance(instance);
    }
}

void RelocateModelPointers(Model *model) {
    SwapModelValue(model->wUnknown0);
    SwapModelValue(model->partCount);
    if (model->animations) {
        SwapModelValue(model->animations);
        RelocateModelPointer(model->animations);
        SwapModelValue(model->animations->count);
        for (int i = 0; i < model->animations->count; ++i) {
            SwapModelValue(model->animations->entries[i]);
            RelocateModelPointer(model->animations->entries[i]);
            SwapModelValue(model->animations->entries[i]->vertexCount);
            SwapModelValue(model->animations->entries[i]->vertices.pSource);
            RelocateModelPointer(model->animations->entries[i]->vertices.pSource);
            ConvertN64VerticesCached(&model->animations->entries[i]->vertices,
                                     model->animations->entries[i]->vertexCount);
            SwapModelValue(model->animations->entries[i]->unreconstructed8);
            RelocateModelPointer(model->animations->entries[i]->unreconstructed8);
            SwapModelValue(model->animations->entries[i]->keyCount);
            SwapModelValue(model->animations->entries[i]->flags);
            SwapModelValue(model->animations->entries[i]->startKey);
            SwapModelValue(model->animations->entries[i]->startTime);
            SwapModelValue(model->animations->entries[i]->endTime);
            SwapModelValue(model->animations->entries[i]->time);
            for (int k = 0; k < model->animations->entries[i]->keyCount; ++k) {
                SwapModelValue(model->animations->entries[i]->keys[k]);
                RelocateModelPointer(model->animations->entries[i]->keys[k]);
                SwapModelValue(model->animations->entries[i]->keys[k]->time);
                for (int j = 0;
                     j < static_cast<int>(
                             static_cast<u32>(model->animations->entries[i]->vertexCount) * 3u);
                     ++j) {
                    SwapModelValue(model->animations->entries[i]->keys[k]->positions[j / 3][j % 3]);
                }
            }
        }
    }
    for (int i = 0; i < model->partCount; ++i) {
        if (model->parts[i].displayList) {
            SwapModelValue(model->parts[i].displayList);
            RelocateModelPointer(model->parts[i].displayList);
            SwapModelValue(model->parts[i].flags);
            SwapModelValue(model->parts[i].wUnknown6);
            SwapModelValue(model->parts[i].position.x);
            SwapModelValue(model->parts[i].position.y);
            SwapModelValue(model->parts[i].position.z);
            ByteSwapAndRelocateDisplayList(model->parts[i].displayList);
            SetTextureLoadCategory(8);
            g_pfnPreprocessDisplayListTextures(model->parts[i].displayList);
        }
    }
}
