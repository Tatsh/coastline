#include "elfobject.h"

#include <algorithm>
#include <format>
#include <stdexcept>

#include "fileio.h"

namespace Tools::Common {
namespace {

std::uint16_t read16(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 2) {
        throw std::runtime_error("The object has a truncated ELF field.");
    }
    return static_cast<std::uint16_t>(bytes[offset] << 8 | bytes[offset + 1]);
}

std::string sectionName(std::span<const std::uint8_t> names, std::uint32_t offset) {
    if (offset >= names.size()) {
        throw std::runtime_error("The object has an invalid ELF string offset.");
    }
    const auto end = std::find(names.begin() + offset, names.end(), 0);
    if (end == names.end()) {
        throw std::runtime_error("The object has an unterminated ELF string.");
    }
    return {names.begin() + offset, end};
}

} // namespace

std::uint32_t readBigEndian32(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(read16(bytes, offset)) << 16 | read16(bytes, offset + 2);
}

ElfObject readElfObject(const std::filesystem::path &path) {
    const auto contents = readFile(path);
    if (!contents) {
        throw std::runtime_error(
            std::format("Cannot read {}: {}", path.string(), contents.error().message));
    }
    const auto &data = *contents;
    if (data.size() < 52 || data[0] != 0x7f || data[1] != 'E' || data[2] != 'L' || data[3] != 'F' ||
        data[4] != 1 || data[5] != 2) {
        throw std::runtime_error(
            std::format("{} is not a big-endian ELF32 object.", path.string()));
    }
    const auto sectionOffset = readBigEndian32(data, 32);
    const auto sectionSize = read16(data, 46);
    const auto sectionCount = read16(data, 48);
    const auto nameIndex = read16(data, 50);
    if (sectionSize < 40 || nameIndex >= sectionCount ||
        static_cast<std::uint64_t>(sectionOffset) +
                static_cast<std::uint64_t>(sectionSize) * sectionCount >
            data.size()) {
        throw std::runtime_error("The object has an invalid ELF section table.");
    }
    const auto section = [&](std::size_t index) {
        return static_cast<std::size_t>(sectionOffset) + index * sectionSize;
    };
    const auto sectionData = [&](std::size_t index) -> std::span<const std::uint8_t> {
        const auto header = section(index);
        const auto offset = readBigEndian32(data, header + 16);
        const auto size = readBigEndian32(data, header + 20);
        if (static_cast<std::uint64_t>(offset) + size > data.size()) {
            throw std::runtime_error("The object has a truncated ELF section.");
        }
        return std::span(data).subspan(offset, size);
    };
    const auto names = sectionData(nameIndex);
    ElfObject result;
    std::size_t textIndex = sectionCount;
    for (std::size_t index = 0; index < sectionCount; ++index) {
        const auto header = section(index);
        const auto name = sectionName(names, readBigEndian32(data, header));
        if (name == ".text") {
            const auto bytes = sectionData(index);
            result.text.assign(bytes.begin(), bytes.end());
            textIndex = index;
        } else if (name == ".rel.text" || name == ".rela.text") {
            const auto bytes = sectionData(index);
            const std::size_t entrySize = name == ".rel.text" ? 8 : 12;
            if (bytes.size() % entrySize != 0) {
                throw std::runtime_error("The object has a truncated ELF relocation table.");
            }
            for (std::size_t offset = 0; offset < bytes.size(); offset += entrySize) {
                const auto position = readBigEndian32(bytes, offset);
                const auto kind = readBigEndian32(bytes, offset + 4) & 0xff;
                if (kind == 4 || kind == 5 || kind == 6) {
                    result.relocations[position] = kind == 4 ? 0xfc000000 : 0xffff0000;
                }
            }
        }
    }
    for (std::size_t index = 0; index < sectionCount; ++index) {
        const auto header = section(index);
        if (readBigEndian32(data, header + 4) != 2) {
            continue;
        }
        const auto namesIndex = readBigEndian32(data, header + 24);
        if (namesIndex >= sectionCount) {
            throw std::runtime_error("The object has an invalid ELF symbol string table.");
        }
        const auto strings = sectionData(namesIndex);
        const auto symbols = sectionData(index);
        if (symbols.size() % 16 != 0) {
            throw std::runtime_error("The object has a truncated ELF symbol table.");
        }
        for (std::size_t offset = 0; offset < symbols.size(); offset += 16) {
            if (read16(symbols, offset + 14) != textIndex) {
                continue;
            }
            const auto name = sectionName(strings, readBigEndian32(symbols, offset));
            if (name.empty()) {
                continue;
            }
            result.symbols.push_back({name,
                                      readBigEndian32(symbols, offset + 4),
                                      readBigEndian32(symbols, offset + 8),
                                      (symbols[offset + 12] & 0xf) == 2});
        }
    }
    return result;
}

} // namespace Tools::Common
