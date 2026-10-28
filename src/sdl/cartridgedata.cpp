#include "cartridgedata.h"

#include <charconv>
#include <iterator>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>

#include "symbol_addrs_us.h"

namespace coastline {

namespace {

// The main segment: its code at this offset of the cartridge, run from this address, up to the
// end of its read-only data. The BSS after it is zero.
constexpr std::size_t kSegmentRom = 0x1000;
constexpr std::uint32_t kSegmentAddress = 0x80200000;
constexpr std::uint32_t kSegmentEnd = 0x802AC400;

struct HostDefinition {
    void *host;
    std::size_t consoleElementSize;
    std::size_t hostElementSize;
    std::size_t count;
};

std::vector<std::uint8_t> g_segment;
std::unordered_map<std::string, std::uint32_t> g_symbols;
std::map<std::uint32_t, HostDefinition> g_definitions;

// Read the lines "name = 0xADDRESS; // ..." of the symbols of splat.
void ReadSymbols() {
    std::string_view text(reinterpret_cast<const char *>(SYMBOL_ADDRS_US), sizeof(SYMBOL_ADDRS_US));
    while (!text.empty()) {
        std::size_t end = text.find('\n');
        std::string_view line = text.substr(0, end);
        text = end == std::string_view::npos ? std::string_view() : text.substr(end + 1);
        std::size_t equals = line.find(" = 0x");
        if (equals == std::string_view::npos) {
            continue;
        }
        std::uint32_t address = 0;
        const char *first = line.data() + equals + 5;
        if (std::from_chars(first, line.data() + line.size(), address, 16).ec == std::errc()) {
            g_symbols.emplace(std::string(line.substr(0, equals)), address);
        }
    }
}

} // namespace

bool CartridgeData::Load(std::span<const std::uint8_t> rom) {
    std::size_t size = kSegmentEnd - kSegmentAddress;
    if (rom.size() < kSegmentRom + size) {
        return false;
    }
    g_segment.assign(rom.begin() + kSegmentRom, rom.begin() + kSegmentRom + size);
    if (g_symbols.empty()) {
        ReadSymbols();
    }
    return true;
}

const std::uint8_t *CartridgeData::Bytes(std::uint32_t address) {
    if (address < kSegmentAddress || address >= kSegmentAddress + g_segment.size()) {
        return nullptr;
    }
    return g_segment.data() + (address - kSegmentAddress);
}

std::uint32_t CartridgeData::AddressOf(std::string_view name) {
    auto found = g_symbols.find(std::string(name));
    return found != g_symbols.end() ? found->second : 0;
}

void CartridgeData::Register(std::uint32_t address,
                             void *host,
                             std::size_t consoleElementSize,
                             std::size_t hostElementSize,
                             std::size_t count) {
    g_definitions[address] = {host, consoleElementSize, hostElementSize, count};
}

void *CartridgeData::Relocate(std::uint32_t address, bool bytes) {
    if (address == 0) {
        return nullptr;
    }
    auto found = g_definitions.upper_bound(address);
    if (found != g_definitions.begin()) {
        --found;
        const HostDefinition &definition = found->second;
        std::size_t offset = address - found->first;
        if (offset < definition.consoleElementSize * definition.count) {
            std::size_t element = offset / definition.consoleElementSize;
            std::size_t within = offset % definition.consoleElementSize;
            // An address inside an element is kept at the same offset, which is exact for the
            // bytes at the start of an element and for arrays of bytes.
            return static_cast<std::uint8_t *>(definition.host) +
                   element * definition.hostElementSize + within;
        }
    }
    if (bytes) {
        return const_cast<std::uint8_t *>(Bytes(address));
    }
    SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "The data addresses 0x%08x, which has no definition on the host.",
                address);
    return nullptr;
}

std::uint64_t BigEndianReader::Word(std::size_t size) {
    const std::uint8_t *bytes = CartridgeData::Bytes(m_address);
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < size; ++i) {
        value =
            value << 8 | (bytes != nullptr && CartridgeData::Bytes(m_address + i) ? bytes[i] : 0);
    }
    m_address += static_cast<std::uint32_t>(size);
    return value;
}

} // namespace coastline
