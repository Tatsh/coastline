#ifndef COASTLINE_EAR_REGISTRY_H
#define COASTLINE_EAR_REGISTRY_H

#include <bit>
#include <cstdint>
#include <type_traits>

#include "ear/mfc.h"

class EarEnvironment;
class EarWaveSample;
class EarPlayback;
class EarChannel;
class EarPresetTable;

/**
 * Map 16-bit handles to borrowed object pointers without deleting registered objects.
 *
 * @tparam Value The registered class type.
 */
template <typename Value>
    requires std::is_class_v<Value>
class EarWordPointerRegistry : public CMapWordToPtr {
public:
    /**
     * Look up a registered pointer without changing the output on a missing key.
     *
     * @param key The 16-bit object handle.
     * @param value The output borrowed pointer, valid while the object remains allocated.
     * @return TRUE when the key exists, including an entry with a null pointer; otherwise FALSE.
     * @ghidraAddress EAR: 0x1000c300
     */
    BOOL LookupWordPointerMapValue(std::uint16_t key, Value *&value) const {
        void *entry;
        if (!Lookup(key, entry)) {
            return FALSE;
        }
        value = static_cast<Value *>(entry);
        return TRUE;
    }
    /**
     * Insert or replace a pointer without releasing the previous object.
     *
     * @param key The 16-bit object handle.
     * @param value The borrowed object pointer, possibly null.
     * @ghidraAddress EAR: 0x10006750
     */
    void SetWordPointerMapValue(std::uint16_t key, Value *value) {
        SetAt(key, value);
    }
    /**
     * Rebuild with ten additional buckets when entry count equals bucket count.
     * Registered object pointers remain unchanged; association positions become invalid.
     */
    void GrowWhenFull() {
        if (GetCount() != static_cast<int>(GetHashTableSize())) {
            return;
        }
        EarWordPointerRegistry copy;
        copy.InitHashTable(GetCount(), TRUE);
        POSITION position = GetStartPosition();
        do {
            WORD key;
            void *entry;
            GetNextAssoc(position, key, entry);
            copy.SetWordPointerMapValue(key, static_cast<Value *>(entry));
        } while (position);
        RemoveAll();
        InitHashTable(copy.GetCount() + 10, TRUE);
        position = copy.GetStartPosition();
        do {
            WORD key;
            void *entry;
            copy.GetNextAssoc(position, key, entry);
            SetWordPointerMapValue(key, static_cast<Value *>(entry));
        } while (position);
        copy.RemoveAll();
    }
};

/**
 * Map 32-bit handles through pointer keys without deleting registered objects.
 *
 * @tparam Value The registered class type.
 */
template <typename Value>
    requires std::is_class_v<Value>
class EarDwordPointerRegistry : public CMapPtrToPtr {
public:
    /**
     * Insert or replace a borrowed object pointer using the handle bits as the map key.
     *
     * @param key The 32-bit handle, widened without sign extension on 64-bit platforms.
     * @param value The borrowed object pointer, possibly null.
     * @ghidraAddress EAR: 0x100039e0
     */
    void SetDwordPointerMapValue(DWORD key, Value *value) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        SetAt(std::bit_cast<void *>(key), value);
#else
        // A pointer of the map is wider than the key on a 64-bit processor.
        SetAt(std::bit_cast<void *>(static_cast<std::uintptr_t>(key)), value);
#endif
    }
    /**
     * Remove a mapping without releasing the registered object.
     *
     * @param key The 32-bit handle supplied at registration.
     * @return TRUE when an entry was removed; otherwise FALSE.
     * @ghidraAddress EAR: 0x10009500
     */
    BOOL RemovePlaybackRegistryKey(DWORD key) {
#if TGR_PLATFORM == TGR_PLATFORM_WIN
        return RemoveKey(std::bit_cast<void *>(key));
#else
        return RemoveKey(std::bit_cast<void *>(static_cast<std::uintptr_t>(key)));
#endif
    }
};

/** Map 32-bit event identifiers to 16-bit channel handles using pooled associations. */
class EarActiveEventRegistry : public CObject {
public:
    /**
     * Initialise an empty registry with 17 hash buckets allocated on first insertion.
     *
     * @param blockSize The positive number of associations allocated per pool block.
     */
    explicit EarActiveEventRegistry(int blockSize = 10);
    /** Release hash buckets and association pools without changing external events. */
    ~EarActiveEventRegistry() override;

    /**
     * @return Whether the registry has no events.
     * @ghidraAddress EAR: 0x10006670
     */
    BOOL IsEmpty() const {
        return count == 0;
    }
    /**
     * Look up a channel handle without changing the output on a missing key.
     *
     * @param key The 32-bit event identifier.
     * @param value The output channel handle.
     * @return TRUE when the identifier exists; otherwise FALSE.
     */
    BOOL Lookup(DWORD key, WORD &value) const;
    /**
     * Insert or replace a channel handle.
     *
     * @param key The 32-bit event identifier.
     * @param value The channel handle to store by value.
     */
    void SetAt(DWORD key, WORD value);
    /**
     * Remove an association without stopping the event.
     *
     * @param key The 32-bit event identifier.
     * @return TRUE when an association was removed; otherwise FALSE.
     */
    BOOL RemoveKey(DWORD key);

private:
    class Association;
    Association **hashTable;
    UINT hashTableSize;
    int count;
    Association *freeAssociations;
    CPlex *blocks;
    int blockSize;
};

/**
 * Borrowed spatial environments indexed by 16-bit environment handle.
 * @ghidraAddress EAR: 0x1003b4a8
 */
/** @cond */
extern EarWordPointerRegistry<EarEnvironment> g_EnvironmentRegistry;
/**
 * Borrowed wave samples indexed by 16-bit preset handle.
 * @ghidraAddress EAR: 0x1003b480
 */
extern EarWordPointerRegistry<EarWaveSample> g_PresetRegistry;
/**
 * Active event identifiers mapped to their 16-bit channel handles.
 * @ghidraAddress EAR: 0x1003b390
 */
extern EarActiveEventRegistry g_ActiveEventRegistry;

/**
 * Borrowed playback instances indexed by 32-bit playback handle.
 * @ghidraAddress EAR: 0x1003b620
 */
extern EarDwordPointerRegistry<EarPlayback> g_PlaybackRegistry;

/**
 * Borrowed channels indexed by 16-bit channel handle.
 * @ghidraAddress EAR: 0x1003b460
 */
extern EarWordPointerRegistry<EarChannel> g_ChannelRegistry;
/**
 * Borrowed directional preset tables indexed by 16-bit matrix handle.
 * @ghidraAddress EAR: 0x1003b370
 */
extern EarWordPointerRegistry<EarPresetTable> g_MatrixRegistry;
/** @endcond */

#endif /* COASTLINE_EAR_REGISTRY_H */
