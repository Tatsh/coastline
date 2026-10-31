#include <cassert>
#include <cerrno>
#include <cstdint>
#include <iostream>
#include <limits>
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

#include "common/ptr32.h"
#include "ptr32memory.h"

int main() {
    using namespace coastline;
    for (std::size_t bytes :
         {std::size_t{0},
          static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) + 1,
          std::numeric_limits<std::size_t>::max()}) {
        bool rejected = false;
        try {
            Ptr32Memory memory(bytes);
        } catch (const std::length_error &) {
            rejected = true;
        }
        assert(rejected);
    }
    assert(DecodePtr32(0) == nullptr && EncodePtr32(nullptr) == 0);
    for (std::uint32_t offset : {1u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
        assert(EncodePtr32(DecodePtr32(offset)) == offset);
    }
    constexpr std::size_t BYTES = 1024 * 1024 + 17;
    for (int run = 0; run < 100; ++run) {
        void *released = nullptr;
        {
            Ptr32Memory first(BYTES);
            auto *data = static_cast<unsigned char *>(first.Data());
            released = data;
            for (std::size_t at : {std::size_t{0}, BYTES / 2, BYTES - 1}) {
                const auto offset = EncodePtr32(data + at);
                assert(offset != 0 && DecodePtr32(offset) == data + at);
                data[at] = static_cast<unsigned char>(at + run);
            }
            Ptr32Memory second(BYTES);
            auto *other = static_cast<unsigned char *>(second.Data());
            assert(other != data);
            other[0] = static_cast<unsigned char>(run + 1);
            other[BYTES - 1] = static_cast<unsigned char>(run + 2);
            assert(DecodePtr32(EncodePtr32(other + BYTES - 1)) == other + BYTES - 1);
            for (std::size_t at : {std::size_t{0}, BYTES / 2, BYTES - 1}) {
                assert(data[at] == static_cast<unsigned char>(at + run));
            }
        }
#ifdef _WIN32
        MEMORY_BASIC_INFORMATION information{};
        assert(VirtualQuery(released, &information, sizeof(information)));
        assert(information.State == MEM_FREE);
#else
#ifdef __APPLE__
        char residency = 0;
#else
        unsigned char residency = 0;
#endif
        errno = 0;
        assert(mincore(released, 1, &residency) == -1 && errno == ENOMEM);
#endif
    }
    std::cout << "Mapped asset pointers and released memory pass all checks.\n";
}
