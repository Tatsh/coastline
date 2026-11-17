#pragma once

/**
 * @file
 * A memory mapping placed within 32-bit offsets of an anchor for the pointers of converted assets.
 */

#include <cstddef>

namespace coastline {

/** Writable memory within signed 32-bit offsets of g_abPtr32Anchor for converted asset pointers. */
class Ptr32Memory {
public:
    /**
     * Allocate writable memory, or throw when no representable mapping is available.
     *
     * @param bytes The requested mapping size, in bytes.
     * @throws std::length_error If @p bytes is zero or exceeds INT32_MAX.
     * @throws std::bad_alloc If a representable mapping cannot be allocated.
     */
    explicit Ptr32Memory(std::size_t bytes);
    /** Release the mapping after all converted pointers into it are no longer used. */
    ~Ptr32Memory();
    /** Copying is disabled because the mapping has exclusive lifetime management. */
    Ptr32Memory(const Ptr32Memory &) = delete;
    /**
     * Assignment is disabled because converted pointers require a stable mapping address.
     *
     * @return This mapping; the operator is deleted and never called.
     */
    Ptr32Memory &operator=(const Ptr32Memory &) = delete;

    /**
     * Get the address of the writable mapping.
     *
     * @return The start of the mapping.
     */
    void *Data() const {
        return m_data;
    }

private:
    void *m_data = nullptr; // The start of the mapping, released by the destructor.
    std::size_t m_bytes;    // The size of the mapping in bytes, passed to the release.
};

} // namespace coastline
