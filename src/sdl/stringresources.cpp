#include "stringresources.h"

#include <cstdint>
#include <optional>
#include <span>

namespace {

constexpr std::uint32_t RT_STRING = 6;
constexpr std::uint32_t RESOURCE_DIRECTORY = 2;
constexpr std::uint32_t SUBDIRECTORY = 0x80000000;

class PeImage {
public:
    explicit PeImage(std::span<const std::uint8_t> bytes) : m_bytes(bytes) {
    }

    std::optional<std::uint32_t> U16(std::size_t offset) const {
        if (offset + 2 > m_bytes.size()) {
            return std::nullopt;
        }
        return static_cast<std::uint32_t>(m_bytes[offset] | (m_bytes[offset + 1] << 8));
    }

    std::optional<std::uint32_t> U32(std::size_t offset) const {
        if (offset + 4 > m_bytes.size()) {
            return std::nullopt;
        }
        return static_cast<std::uint32_t>(m_bytes[offset] | (m_bytes[offset + 1] << 8) |
                                          (m_bytes[offset + 2] << 16) |
                                          (static_cast<std::uint32_t>(m_bytes[offset + 3]) << 24));
    }

    // The file offset of a relative virtual address, through the section that maps it.
    std::optional<std::size_t> FileOffset(std::uint32_t rva) const {
        for (std::uint32_t i = 0; i < m_sectionCount; ++i) {
            std::size_t section = m_sections + i * 40;
            auto virtualSize = U32(section + 8);
            auto virtualAddress = U32(section + 12);
            auto rawSize = U32(section + 16);
            auto rawOffset = U32(section + 20);
            if (!virtualSize || !virtualAddress || !rawSize || !rawOffset) {
                return std::nullopt;
            }
            std::uint32_t size = *virtualSize > *rawSize ? *virtualSize : *rawSize;
            if (rva >= *virtualAddress && rva < *virtualAddress + size) {
                return static_cast<std::size_t>(*rawOffset) + (rva - *virtualAddress);
            }
        }
        return std::nullopt;
    }

    bool Parse() {
        auto peOffset = U32(0x3C);
        if (!peOffset || U32(*peOffset) != 0x00004550u) {
            return false;
        }
        std::size_t fileHeader = *peOffset + 4;
        auto sectionCount = U16(fileHeader + 2);
        auto optionalSize = U16(fileHeader + 16);
        if (!sectionCount || !optionalSize) {
            return false;
        }
        std::size_t optional = fileHeader + 20;
        auto magic = U16(optional);
        if (!magic) {
            return false;
        }
        // The data directories follow 96 bytes of the 32-bit header and 112 of the 64-bit one.
        std::size_t directories = optional + (*magic == 0x20B ? 112 : 96);
        auto resourceRva = U32(directories + RESOURCE_DIRECTORY * 8);
        m_sectionCount = *sectionCount;
        m_sections = optional + *optionalSize;
        if (!resourceRva) {
            return false;
        }
        auto resources = FileOffset(*resourceRva);
        if (!resources) {
            return false;
        }
        m_resources = *resources;
        return true;
    }

    // The entry of a resource directory with the identifier, or the first entry for any.
    std::optional<std::uint32_t> Entry(std::size_t directory,
                                       std::optional<std::uint32_t> id) const {
        auto named = U16(directory + 12);
        auto numbered = U16(directory + 14);
        if (!named || !numbered) {
            return std::nullopt;
        }
        for (std::uint32_t i = 0; i < *named + *numbered; ++i) {
            std::size_t entry = directory + 16 + i * 8;
            auto name = U32(entry);
            auto target = U32(entry + 4);
            if (!name || !target) {
                return std::nullopt;
            }
            if (!id || *name == *id) {
                return *target;
            }
        }
        return std::nullopt;
    }

    std::size_t Resources() const {
        return m_resources;
    }

private:
    std::span<const std::uint8_t> m_bytes;
    std::uint32_t m_sectionCount = 0;
    std::size_t m_sections = 0;
    std::size_t m_resources = 0;
};

// Windows-1252 for the characters of UTF-16 the library can use; others become '?', as the default
// character of WideCharToMultiByte() does.
char ToWindows1252(std::uint32_t c) {
    if (c < 0x80 || (c >= 0xA0 && c <= 0xFF)) {
        return static_cast<char>(c);
    }
    static constexpr std::uint16_t HIGH[32] = {
        0x20AC, 0,      0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
        0x2039, 0x0152, 0,      0x017D, 0,      0,      0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
        0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0,      0x017E, 0x0178};
    for (int i = 0; i < 32; ++i) {
        if (HIGH[i] != 0 && HIGH[i] == c) {
            return static_cast<char>(0x80 + i);
        }
    }
    return '?';
}

} // namespace

extern "C" int LoadLibraryString(
    const unsigned char *image, size_t imageSize, unsigned id, char *buffer, int bufferSize) {
    if (bufferSize <= 0) {
        return 0;
    }
    buffer[0] = '\0';
    PeImage pe(std::span<const std::uint8_t>(image, imageSize));
    if (!pe.Parse()) {
        return 0;
    }
    // RT_STRING, then the block of sixteen strings, then the first language.
    std::size_t root = pe.Resources();
    auto types = pe.Entry(root, RT_STRING);
    if (!types || !(*types & SUBDIRECTORY)) {
        return 0;
    }
    auto blocks = pe.Entry(root + (*types & ~SUBDIRECTORY), id / 16 + 1);
    if (!blocks || !(*blocks & SUBDIRECTORY)) {
        return 0;
    }
    auto languages = pe.Entry(root + (*blocks & ~SUBDIRECTORY), std::nullopt);
    if (!languages || (*languages & SUBDIRECTORY)) {
        return 0;
    }
    std::size_t dataEntry = root + *languages;
    auto rva = pe.U32(dataEntry);
    if (!rva) {
        return 0;
    }
    auto offset = pe.FileOffset(*rva);
    if (!offset) {
        return 0;
    }
    // Each string of the block is a count of UTF-16 units and the units.
    std::size_t position = *offset;
    for (unsigned index = 0; index < id % 16; ++index) {
        auto length = pe.U16(position);
        if (!length) {
            return 0;
        }
        position += 2 + *length * 2;
    }
    auto length = pe.U16(position);
    if (!length || *length == 0) {
        return 0;
    }
    int copied = 0;
    for (std::uint32_t i = 0; i < *length && copied < bufferSize - 1; ++i) {
        auto unit = pe.U16(position + 2 + i * 2);
        if (!unit) {
            break;
        }
        buffer[copied++] = ToWindows1252(*unit);
    }
    buffer[copied] = '\0';
    return copied;
}
