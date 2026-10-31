#pragma once

#include <cstddef>

namespace coastline {

/** Writable memory within the signed offset range of the converted asset pointers. */
class Ptr32Memory {
public:
    /** Allocate @p bytes, or throw when no representable mapping is available. */
    explicit Ptr32Memory(std::size_t bytes);
    ~Ptr32Memory();
    Ptr32Memory(const Ptr32Memory &) = delete;
    Ptr32Memory &operator=(const Ptr32Memory &) = delete;

    /** @return The start of the mapping. */
    void *Data() const {
        return m_data;
    }

private:
    void *m_data = nullptr;
    std::size_t m_bytes;
};

} // namespace coastline
