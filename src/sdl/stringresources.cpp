#include "stringresources.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "common/strings.h"
#include "consolestrings.h"
#include "language.h"

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

constexpr const char *ACTIVE_TEXT_PROPERTY = "coastline.game-text";

coastline::StringTable ReadStringsFile(const std::filesystem::path &path) {
    coastline::StringTable strings;
    std::ifstream file(path);
    auto json = nlohmann::json::parse(file, nullptr, false);
    if (json.is_discarded() || !json.is_object()) {
        SDL_LogWarn(
            SDL_LOG_CATEGORY_APPLICATION, "%s is not a file of strings.", path.string().c_str());
        return strings;
    }
    for (const auto &[key, text] : json.items()) {
        if (text.is_string()) {
            strings.emplace(key, text.get<std::string>());
        }
    }
    return strings;
}

} // namespace

namespace coastline {

void AppendUtf8(std::string &text, char32_t c) {
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

char32_t NextCodePoint(std::string_view text, std::size_t &position) {
    auto lead = static_cast<unsigned char>(text[position]);
    int extra = lead < 0xC0 ? 0 : lead < 0xE0 ? 1 : lead < 0xF0 ? 2 : 3;
    char32_t c = extra == 0 ? lead : lead & (0x3F >> extra);
    for (int i = 1; i <= extra && position + i < text.size(); ++i) {
        c = c << 6 | (static_cast<unsigned char>(text[position + i]) & 0x3F);
    }
    position += static_cast<std::size_t>(extra) + 1;
    return c;
}

GameText::GameText(const std::filesystem::path &assets, std::string language)
    : m_language(std::move(language)) {
    assert(Active() == nullptr);
    auto directory = assets / "strings";
    auto fallback = ReadStringsFile(directory / (std::string(DEFAULT_LANGUAGE) + ".json"));
    auto selected = directory / (m_language + ".json");
    std::error_code error;
    if (m_language != DEFAULT_LANGUAGE && !std::filesystem::exists(selected, error)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "%s is missing, and the strings of %s are used.",
                    selected.string().c_str(),
                    std::string(DEFAULT_LANGUAGE).c_str());
    }
    m_strings = m_language == DEFAULT_LANGUAGE || !std::filesystem::exists(selected, error) ?
                    std::move(fallback) :
                    MergeStrings(std::move(fallback), ReadStringsFile(selected));
    SDL_SetPointerProperty(SDL_GetGlobalProperties(), ACTIVE_TEXT_PROPERTY, this);
}

GameText::~GameText() {
    SDL_ClearProperty(SDL_GetGlobalProperties(), ACTIVE_TEXT_PROPERTY);
}

GameText *GameText::Active() {
    return static_cast<GameText *>(
        SDL_GetPointerProperty(SDL_GetGlobalProperties(), ACTIVE_TEXT_PROPERTY, nullptr));
}

const std::string *GameText::Find(std::string_view key) const {
    auto found = m_strings.find(key);
    return found == m_strings.end() ? nullptr : &found->second;
}

std::string_view GameText::Resolve(std::string_view key) {
    const std::string *found = Find(key);
    if (found != nullptr) {
        return *found;
    }
    if (m_logged.emplace(key).second) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "The string %.*s is in no file of strings. Import the cartridges again.",
                    static_cast<int>(key.size()),
                    key.data());
    }
    return key;
}

const std::string *FindString(std::string_view key) {
    const auto *text = GameText::Active();
    return text ? text->Find(key) : nullptr;
}

std::string_view GameString(std::string_view key) {
    auto *text = GameText::Active();
    return text ? text->Resolve(key) : key;
}

const std::string &SelectedLanguage() {
    const auto *text = GameText::Active();
    static const std::string fallback(DEFAULT_LANGUAGE);
    return text ? text->Language() : fallback;
}

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

} // namespace coastline

// The SDL game puts the active catalogue's strings into the table of the race in UTF-8, each by the
// number of BRString.dll its key in LIBRARY_STRING_KEYS has.
extern "C" void LoadStringTableFromDll(void) {
    std::memset(&g_apszStringResourceTable[1], 0, STRING_RESOURCE_COUNT * sizeof(char *));
    if (g_pStringTableBuffer != nullptr) {
        return;
    }
    const auto *catalog = coastline::GameText::Active();
    if (!catalog || catalog->Empty()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "The strings of the race are missing.");
        return;
    }
    std::vector<std::pair<unsigned, std::string_view>> strings;
    std::size_t size = 0;
    for (unsigned number = 1; number <= STRING_RESOURCE_COUNT; ++number) {
        std::string_view text = coastline::GameString(coastline::LIBRARY_STRING_KEYS[number - 1]);
        strings.emplace_back(number, text);
        size += text.size() + 1;
    }
    g_pStringTableBuffer = static_cast<char *>(std::malloc(size));
    if (g_pStringTableBuffer == nullptr) {
        return;
    }
    g_dwStringTableBufferSize = size;
    g_dwStringTableUsedBytes = 0;
    for (const auto &[number, text] : strings) {
        char *entry = g_pStringTableBuffer + g_dwStringTableUsedBytes;
        std::memcpy(entry, text.data(), text.size());
        entry[text.size()] = '\0';
        g_apszStringResourceTable[number] = entry;
        g_dwStringTableUsedBytes += text.size() + 1;
    }
}
