#ifndef COASTLINE_EAR_REGISTRY_H
#define COASTLINE_EAR_REGISTRY_H

#include <bit>
#include <cstdint>
#include <type_traits>

#include "mfc.h"

class EarEnvironment;
class EarPreset;
class EarPlayback;

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
    void SetDwordPointerMapValue(DWORD key, Value *value) {
        SetAt(std::bit_cast<void *>(key), value);
    }
    BOOL RemovePlaybackRegistryKey(DWORD key) {
        return RemoveKey(std::bit_cast<void *>(key));
    }
};

class EarActiveEventRegistry : public CObject {
public:
    /**
     * @ghidraAddress EAR: 0x10006670
     * @stub
     */
    BOOL IsEmpty() const;
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
extern EarWordPointerRegistry<EarPreset> g_PresetRegistry;
/** @ghidraAddress EAR: 0x1003b390 */
extern EarActiveEventRegistry g_ActiveEventRegistry;

/** @ghidraAddress EAR: 0x1003b620 */
extern EarDwordPointerRegistry<EarPlayback> g_PlaybackRegistry;

#endif
