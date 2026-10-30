#include "consolesegment.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <span>
#include <sstream>

#include <SDL3/SDL.h>

#include "cartridge/eu.h"
#include "cartridge/jp.h"
#include "cartridge/us.h"

namespace coastline {

namespace {

// Where the main segment of the console is loaded.
constexpr u32 SEGMENT_BASE = 0x80200000;

} // namespace

std::optional<ConsoleSegment> ConsoleSegment::Load(const std::filesystem::path &release) {
    auto name = release.filename().string();
    std::span<const unsigned char> manifest;
    if (name == "usa") {
        manifest = CARTRIDGE_US;
    } else if (name == "eur") {
        manifest = CARTRIDGE_EU;
    } else if (name == "jpn") {
        manifest = CARTRIDGE_JP;
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "%s is not the import of a release.",
                     release.string().c_str());
        return std::nullopt;
    }
    ConsoleSegment segment;
    std::ifstream file(release / "segment.bin", std::ios::binary);
    segment.m_bytes.assign(std::istreambuf_iterator<char>(file), {});
    if (segment.m_bytes.empty()) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION, "%s has no main segment.", release.string().c_str());
        return std::nullopt;
    }
    std::istringstream lines(std::string(manifest.begin(), manifest.end()));
    std::string line;
    while (std::getline(lines, line)) {
        std::istringstream fields(line);
        u32 offset = 0;
        std::size_t size = 0;
        std::string kind;
        std::string asset;
        if (fields >> std::hex >> offset >> std::dec >> size >> kind >> asset &&
            asset != "padding") {
            segment.m_assets[offset] = asset;
        }
    }
    return segment;
}

bool ConsoleSegment::Has(u32 address, u32 size) const {
    return address >= SEGMENT_BASE && address - SEGMENT_BASE + size <= m_bytes.size();
}

u8 ConsoleSegment::Byte(u32 address) const {
    return Has(address, 1) ? m_bytes[address - SEGMENT_BASE] : 0;
}

u32 ConsoleSegment::Word(u32 address) const {
    return static_cast<u32>(Byte(address)) << 24 | Byte(address + 1) << 16 |
           Byte(address + 2) << 8 | Byte(address + 3);
}

s32 ConsoleSegment::Signed(u32 address) const {
    return static_cast<s32>(Word(address));
}

f32 ConsoleSegment::Float(u32 address) const {
    u32 word = Word(address);
    f32 value = 0.0f;
    std::memcpy(&value, &word, sizeof(value));
    return value;
}

std::string ConsoleSegment::Text(u32 address) const {
    std::string text;
    while (Has(address, 1) && Byte(address) != 0) {
        text.push_back(static_cast<char>(Byte(address++)));
    }
    return text;
}

std::optional<std::string> ConsoleSegment::AssetName(u32 romStart) const {
    auto found = m_assets.find(romStart);
    if (found == m_assets.end()) {
        return std::nullopt;
    }
    return found->second;
}

} // namespace coastline
