#include "stringresources.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <optional>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "common/strings.h"

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

// Windows-1252 for a character; others become '?', as the default character of
// WideCharToMultiByte() does.
char ToWindows1252Character(std::uint32_t c) {
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

void AppendUtf8(std::string &text, std::uint32_t c) {
    if (c < 0x80) {
        text += static_cast<char>(c);
    } else if (c < 0x800) {
        text += static_cast<char>(0xC0 | c >> 6);
        text += static_cast<char>(0x80 | (c & 0x3F));
    } else if (c < 0x10000) {
        text += static_cast<char>(0xE0 | c >> 12);
        text += static_cast<char>(0x80 | (c >> 6 & 0x3F));
        text += static_cast<char>(0x80 | (c & 0x3F));
    } else {
        text += static_cast<char>(0xF0 | c >> 18);
        text += static_cast<char>(0x80 | (c >> 12 & 0x3F));
        text += static_cast<char>(0x80 | (c >> 6 & 0x3F));
        text += static_cast<char>(0x80 | (c & 0x3F));
    }
}

} // namespace

namespace coastline {

std::map<unsigned, std::string> ReadLibraryStrings(std::span<const std::uint8_t> image) {
    std::map<unsigned, std::string> strings;
    PeImage pe(image);
    if (!pe.Parse()) {
        return strings;
    }
    // RT_STRING, then each block of sixteen strings, then its first language.
    std::size_t root = pe.Resources();
    auto types = pe.Entry(root, RT_STRING);
    if (!types || !(*types & SUBDIRECTORY)) {
        return strings;
    }
    std::size_t blocks = root + (*types & ~SUBDIRECTORY);
    auto named = pe.U16(blocks + 12);
    auto numbered = pe.U16(blocks + 14);
    if (!named || !numbered) {
        return strings;
    }
    for (std::uint32_t i = 0; i < *named + *numbered; ++i) {
        auto block = pe.U32(blocks + 16 + i * 8);
        auto target = pe.U32(blocks + 16 + i * 8 + 4);
        if (!block || !target || !(*target & SUBDIRECTORY) || *block == 0) {
            continue;
        }
        auto language = pe.Entry(root + (*target & ~SUBDIRECTORY), std::nullopt);
        if (!language || (*language & SUBDIRECTORY)) {
            continue;
        }
        auto rva = pe.U32(root + *language);
        auto offset = rva ? pe.FileOffset(*rva) : std::nullopt;
        if (!offset) {
            continue;
        }
        // Each string of the block is a count of UTF-16 units and the units.
        std::size_t position = *offset;
        for (unsigned index = 0; index < 16; ++index) {
            auto length = pe.U16(position);
            if (!length) {
                break;
            }
            std::string text;
            for (std::uint32_t unit = 0; unit < *length; ++unit) {
                auto c = pe.U16(position + 2 + unit * 2);
                auto low = pe.U16(position + 4 + unit * 2);
                if (!c) {
                    break;
                }
                if (*c >= 0xD800 && *c < 0xDC00 && low && *low >= 0xDC00 && *low < 0xE000) {
                    AppendUtf8(text, 0x10000 + ((*c - 0xD800) << 10) + (*low - 0xDC00));
                    ++unit;
                } else {
                    AppendUtf8(text, *c);
                }
            }
            if (*length != 0) {
                strings[(*block - 1) * 16 + index] = std::move(text);
            }
            position += 2 + *length * 2;
        }
    }
    return strings;
}

std::string ToWindows1252(std::string_view text) {
    std::string converted;
    for (std::size_t i = 0; i < text.size();) {
        auto lead = static_cast<unsigned char>(text[i]);
        int extra = lead < 0x80 ? 0 : lead < 0xE0 ? 1 : lead < 0xF0 ? 2 : 3;
        std::uint32_t c = extra == 0 ? lead : lead & (0x3F >> extra);
        for (int j = 1; j <= extra && i + j < text.size(); ++j) {
            c = c << 6 | (static_cast<unsigned char>(text[i + j]) & 0x3F);
        }
        converted += ToWindows1252Character(c);
        i += static_cast<std::size_t>(extra) + 1;
    }
    return converted;
}

} // namespace coastline

// The SDL game reads the strings of BRString.dll from strings/pc.json, the import of the library,
// in the code page LoadStringA() converts them to, into the table of the race.
extern "C" void LoadStringTableFromDll(void) {
    std::memset(&g_apszStringResourceTable[1], 0, STRING_RESOURCE_COUNT * sizeof(char *));
    if (g_pStringTableBuffer != nullptr) {
        return;
    }
    std::ifstream file("strings/pc.json");
    auto json = nlohmann::json::parse(file, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "strings/pc.json is not the strings of the race.");
        return;
    }
    std::vector<std::pair<unsigned, std::string>> strings;
    std::size_t size = 0;
    for (const auto &[id, text] : json.items()) {
        unsigned number = static_cast<unsigned>(std::strtoul(id.c_str(), nullptr, 10));
        if (number < 1 || number > STRING_RESOURCE_COUNT || !text.is_string()) {
            continue;
        }
        strings.emplace_back(number, coastline::ToWindows1252(text.get<std::string>()));
        size += strings.back().second.size() + 1;
    }
    g_pStringTableBuffer = static_cast<char *>(std::malloc(size));
    if (g_pStringTableBuffer == nullptr) {
        return;
    }
    g_dwStringTableBufferSize = size;
    g_dwStringTableUsedBytes = 0;
    for (const auto &[number, text] : strings) {
        char *entry = g_pStringTableBuffer + g_dwStringTableUsedBytes;
        std::memcpy(entry, text.c_str(), text.size() + 1);
        g_apszStringResourceTable[number] = entry;
        g_dwStringTableUsedBytes += text.size() + 1;
    }
}
