#include "common/ptr32.h"

#include <cstdint>
#include <initializer_list>
#include <limits>
#include <new>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "ptr32memory.h"

const unsigned char g_abPtr32Anchor[16] = {};

namespace coastline {

namespace {

void ReleaseMapping(void *data, std::size_t bytes) {
#ifdef _WIN32
    (void)bytes;
    VirtualFree(data, 0, MEM_RELEASE);
#else
    munmap(data, bytes);
#endif
}

bool FitsOffsetRange(void *data, std::size_t bytes) {
    const auto anchor = reinterpret_cast<std::uintptr_t>(g_abPtr32Anchor);
    const auto begin = reinterpret_cast<std::uintptr_t>(data);
    if (begin > std::numeric_limits<std::uintptr_t>::max() - bytes) {
        return false;
    }
    const auto end = begin + bytes - 1;
    if (begin <= anchor && end >= anchor) {
        return false;
    }
    auto fits = [anchor](std::uintptr_t address) {
        return address >= anchor ? address - anchor <= std::numeric_limits<std::int32_t>::max() :
                                   anchor - address <= std::uint64_t{1} << 31;
    };
    return fits(begin) && fits(end);
}

} // namespace

Ptr32Memory::Ptr32Memory(std::size_t bytes) : m_bytes(bytes) {
    if (bytes == 0 || bytes > std::numeric_limits<std::int32_t>::max()) {
        throw std::length_error("The converted asset mapping has an invalid size.");
    }
#ifdef _WIN32
    SYSTEM_INFO information{};
    GetSystemInfo(&information);
    const std::uintptr_t granularity = information.dwAllocationGranularity;
#else
    const long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize <= 0) {
        throw std::bad_alloc();
    }
    const auto granularity = static_cast<std::uintptr_t>(pageSize);
#endif
    const auto anchor = reinterpret_cast<std::uintptr_t>(g_abPtr32Anchor);
    const auto aligned = anchor - anchor % granularity;
    constexpr std::uintptr_t STEP = 16 * 1024 * 1024;
    for (std::uintptr_t distance = STEP;
         distance <= static_cast<std::uintptr_t>(std::numeric_limits<std::int32_t>::max());
         distance += STEP) {
        for (bool above : {true, false}) {
            if ((above && aligned > std::numeric_limits<std::uintptr_t>::max() - distance) ||
                (!above && aligned <= distance)) {
                continue;
            }
            const auto address = above ? aligned + distance : aligned - distance;
            auto *hint = reinterpret_cast<void *>(address);
#ifdef _WIN32
            void *mapped = VirtualAlloc(hint, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (mapped == nullptr) {
                continue;
            }
#else
            // An address hint never replaces an existing mapping.
            void *mapped =
                mmap(hint, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (mapped == MAP_FAILED) {
                continue;
            }
#endif
            if (FitsOffsetRange(mapped, bytes)) {
                m_data = mapped;
                return;
            }
            ReleaseMapping(mapped, bytes);
        }
    }
    throw std::bad_alloc();
}

Ptr32Memory::~Ptr32Memory() {
    ReleaseMapping(m_data, m_bytes);
}

} // namespace coastline
