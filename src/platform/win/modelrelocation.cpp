#include <bit>
#include <type_traits>

#include "displaylistops.h"
#include "model.h"
#include "platform/win/render.h"
#include "platform/win/texturescan.h"
#include "platform/win/trackrelocation.h"

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
