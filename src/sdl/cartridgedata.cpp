#include "cartridgedata.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <fstream>
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

std::filesystem::path g_assetDirectory;
// The raw data of the cartridge: the bytes of each span by where it starts.
std::map<std::uint32_t, std::uint32_t> g_rawSpans;
std::vector<std::uint8_t> g_segment;
std::unordered_map<std::string, std::uint32_t> g_symbols;
std::map<std::uint32_t, HostDefinition> g_definitions;
std::vector<std::uint32_t> g_symbolAddresses;
std::unordered_map<std::uint32_t, void (*)()> g_routines;

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

bool CartridgeData::Load(const std::filesystem::path &directory) {
    std::size_t size = kSegmentEnd - kSegmentAddress;
    std::ifstream file(directory / "segment.bin", std::ios::binary);
    std::vector<std::uint8_t> segment(std::istreambuf_iterator<char>(file), {});
    if (segment.size() != size) {
        return false;
    }
    g_segment = std::move(segment);
    g_assetDirectory = directory / "assets";
    g_rawSpans.clear();
    std::error_code error;
    for (const auto &entry : std::filesystem::directory_iterator(g_assetDirectory, error)) {
        std::uint32_t offset = 0;
        std::string stem = entry.path().stem().string();
        if (entry.path().extension() == ".raw" &&
            std::from_chars(stem.data(), stem.data() + stem.size(), offset, 16).ec == std::errc()) {
            g_rawSpans[offset] = static_cast<std::uint32_t>(entry.file_size(error));
        }
    }
    if (g_symbols.empty()) {
        ReadSymbols();
        for (const auto &[name, address] : g_symbols) {
            g_symbolAddresses.push_back(address);
        }
        std::ranges::sort(g_symbolAddresses);
    }
    return true;
}

const std::uint8_t *CartridgeData::Bytes(std::uint32_t address) {
    if (address < kSegmentAddress || address >= kSegmentAddress + g_segment.size()) {
        return nullptr;
    }
    return g_segment.data() + (address - kSegmentAddress);
}

std::optional<std::vector<std::uint8_t>> CartridgeData::Asset(std::uint32_t romOffset) {
    std::ifstream file(g_assetDirectory / std::format("{:06x}.bin", romOffset), std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), {});
}

bool CartridgeData::ReadRaw(void *destination, std::uint32_t romOffset, std::size_t size) {
    auto span = g_rawSpans.upper_bound(romOffset);
    if (span == g_rawSpans.begin()) {
        return false;
    }
    --span;
    if (romOffset - span->first + size > span->second) {
        return false;
    }
    std::ifstream file(g_assetDirectory / std::format("{:06x}.raw", span->first), std::ios::binary);
    file.seekg(romOffset - span->first);
    file.read(static_cast<char *>(destination), static_cast<std::streamsize>(size));
    return static_cast<bool>(file);
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

bool CartridgeData::IsDefined(std::uint32_t address) {
    auto found = g_definitions.upper_bound(address);
    if (found == g_definitions.begin()) {
        return false;
    }
    --found;
    return address - found->first < found->second.consoleElementSize * found->second.count;
}

bool CartridgeData::IsSegmentCopy(const void *host) {
    auto *byte = static_cast<const std::uint8_t *>(host);
    return !g_segment.empty() && byte >= g_segment.data() &&
           byte < g_segment.data() + g_segment.size();
}

bool CartridgeData::IsCartridgeData(const void *host) {
    if (IsSegmentCopy(host)) {
        return true;
    }
    auto *byte = static_cast<const std::uint8_t *>(host);
    for (const auto &[address, definition] : g_definitions) {
        auto *start = static_cast<const std::uint8_t *>(definition.host);
        if (byte >= start && byte < start + definition.hostElementSize * definition.count) {
            return true;
        }
    }
    return false;
}

std::size_t CartridgeData::SymbolLength(std::uint32_t address) {
    auto found = std::ranges::upper_bound(g_symbolAddresses, address);
    if (found == g_symbolAddresses.begin() || *(found - 1) != address) {
        return 0;
    }
    std::uint32_t end = found != g_symbolAddresses.end() ? *found : kSegmentEnd;
    return end - address;
}

void CartridgeData::RegisterRoutine(std::string_view name, void (*host)()) {
    if (std::uint32_t address = AddressOf(name); address != 0) {
        g_routines[address] = host;
    }
}

void (*CartridgeData::RelocateRoutine(std::uint32_t address))() {
    if (address == 0) {
        return nullptr;
    }
    auto found = g_routines.find(address);
    if (found == g_routines.end()) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "The data addresses the routine 0x%08x, which has no routine on the host.",
                    address);
        return nullptr;
    }
    return found->second;
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
