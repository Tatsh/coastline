#ifndef COASTLINE_EAR_REGISTRY_H
#define COASTLINE_EAR_REGISTRY_H

#include <bit>
#include <cstdint>
#include <type_traits>

#include "mfc.h"

class EarEnvironment;
class EarWaveSample;
class EarPlayback;
class EarChannel;
class EarPresetTable;

template <typename Value>
    requires std::is_class_v<Value>
class EarWordPointerRegistry : public CMapWordToPtr {
public:
    /** @ghidraAddress EAR: 0x1000c300 */
    BOOL LookupWordPointerMapValue(std::uint16_t key, Value *&value) const {
        void *entry;
        if (!Lookup(key, entry)) {
            return FALSE;
        }
        value = static_cast<Value *>(entry);
        return TRUE;
    }
};

template <typename Value>
    requires std::is_class_v<Value>
class EarDwordPointerRegistry : public CMapPtrToPtr {
public:
    /** @ghidraAddress EAR: 0x100039e0 */
    void SetDwordPointerMapValue(DWORD key, Value *value) {
        SetAt(std::bit_cast<void *>(key), value);
    }
    /** @ghidraAddress EAR: 0x10009500 */
    BOOL RemovePlaybackRegistryKey(DWORD key) {
        return RemoveKey(std::bit_cast<void *>(key));
    }
};

class EarActiveEventRegistry : public CObject {
public:
    /**
     * @return Whether the registry has no events.
     * @ghidraAddress EAR: 0x10006670
     */
    BOOL IsEmpty() const {
        return count == 0;
    }
    /**
     * @ghidraAddress EAR: 0x10008ac0
     * @stub
     */
    BOOL RemoveKey(std::uint16_t key);

private:
    class Association;
    Association **hashTable;
    UINT hashTableSize;
    int count;
    Association *freeAssociations;
    CPlex *blocks;
    int blockSize;
};

/** @ghidraAddress EAR: 0x1003b4a8 */
extern EarWordPointerRegistry<EarEnvironment> g_EnvironmentRegistry;
/** @ghidraAddress EAR: 0x1003b480 */
extern EarWordPointerRegistry<EarWaveSample> g_PresetRegistry;
/** @ghidraAddress EAR: 0x1003b390 */
extern EarActiveEventRegistry g_ActiveEventRegistry;

/** @ghidraAddress EAR: 0x1003b620 */
extern EarDwordPointerRegistry<EarPlayback> g_PlaybackRegistry;

/** @ghidraAddress EAR: 0x1003b460 */
extern EarWordPointerRegistry<EarChannel> g_ChannelRegistry;
/** @ghidraAddress EAR: 0x1003b370 */
extern EarWordPointerRegistry<EarPresetTable> g_MatrixRegistry;

#endif /* COASTLINE_EAR_REGISTRY_H */
