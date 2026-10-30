#ifndef COASTLINE_PTR32_H
#define COASTLINE_PTR32_H

/**
 * @file
 * Pointers of 32 bits in the data the game reads from files and in its display lists. The
 * Nintendo 64 and the Windows libraries have 32-bit pointers, and TGR_PTR(T) is T *. The SDL game
 * runs on 64-bit processors, where TGR_PTR(T) is a Ptr32, a signed 32-bit offset from an anchor in
 * the static data of the program. Every object such a pointer addresses is static data or comes
 * from the static pool of CHK_AllocateMemory(), and lies within 2 GiB of the anchor.
 */

#if defined(TGR_PLATFORM) && TGR_PLATFORM == TGR_PLATFORM_SDL && defined(__cplusplus)

#include <cstddef>
#include <cstdint>
#include <type_traits>

extern "C" {
/**
 * The anchor the offsets of Ptr32 count from. No pointer addresses it. Its size is not declared, as
 * the compiler would otherwise take every decoded pointer for an access past its end.
 */
extern unsigned char g_abPtr32Anchor[];
}

// The headers of the game may include this one inside extern "C".
extern "C++" {
namespace coastline {

/**
 * Encode an address as its offset from the anchor.
 *
 * @param address The address, or nullptr.
 * @return The offset, or 0 for nullptr.
 */
inline std::uint32_t EncodePtr32(const volatile void *address) {
    if (address == nullptr) {
        return 0;
    }
    return static_cast<std::uint32_t>(
        static_cast<std::int32_t>(reinterpret_cast<std::intptr_t>(address) -
                                  reinterpret_cast<std::intptr_t>(g_abPtr32Anchor)));
}

/**
 * Decode an offset from the anchor.
 *
 * @param offset The offset, or 0.
 * @return The address, or nullptr for 0.
 */
inline void *DecodePtr32(std::uint32_t offset) {
    if (offset == 0) {
        return nullptr;
    }
    return g_abPtr32Anchor + static_cast<std::int32_t>(offset);
}

/**
 * A pointer to T stored in 32 bits as its offset from the anchor. It has no constructors, and is
 * an aggregate the anonymous structures of the game can have as a member.
 */
template <typename T>
class Ptr32 {
public:
    Ptr32 &operator=(T *pointer) {
        m_offset = EncodePtr32(pointer);
        return *this;
    }
    Ptr32 &operator=(std::nullptr_t) {
        m_offset = 0;
        return *this;
    }

    operator T *() const {
        return static_cast<T *>(DecodePtr32(m_offset));
    }
    T *operator->() const {
        return *this;
    }
    template <typename U = T>
        requires(!std::is_void_v<U>)
    U &operator*() const {
        return *static_cast<T *>(*this);
    }
    template <typename U = T>
        requires(!std::is_void_v<U>)
    U &operator[](std::ptrdiff_t index) const {
        return static_cast<T *>(*this)[index];
    }
    Ptr32 &operator++() {
        *this = static_cast<T *>(*this) + 1;
        return *this;
    }
    Ptr32 operator++(int) {
        Ptr32 old = *this;
        ++*this;
        return old;
    }
    Ptr32 &operator+=(std::ptrdiff_t count) {
        *this = static_cast<T *>(*this) + count;
        return *this;
    }

    /** @return The offset, as the file or the display list stores it. */
    std::uint32_t Raw() const {
        return m_offset;
    }

    /** @param offset The offset to store, as the file or the display list stores it. */
    void SetRaw(std::uint32_t offset) {
        m_offset = offset;
    }

    std::uint32_t m_offset; /*!< The offset from the anchor, 0 for a null pointer. */
};

} // namespace coastline
}

/** A pointer of 32 bits to T. */
#define TGR_PTR(T) coastline::Ptr32<T>

#else

/** A pointer of 32 bits to T. */
#define TGR_PTR(T) T *

#endif

#endif /* COASTLINE_PTR32_H */
