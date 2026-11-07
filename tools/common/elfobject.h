#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace Tools::Common {

/** A defined symbol in an ELF object's text section. */
struct ElfSymbol {
    std::string name;
    std::uint32_t offset;
    std::uint32_t size;
    bool function;
};

/** The text, relocations, and symbols of a big-endian MIPS ELF object. */
struct ElfObject {
    std::vector<std::uint8_t> text;
    std::map<std::uint32_t, std::uint32_t> relocations;
    std::vector<ElfSymbol> symbols;
};

/**
 * Read a big-endian integer from a byte buffer.
 *
 * @param bytes Byte buffer.
 * @param offset Offset of the integer.
 * @return The integer in host byte order.
 */
std::uint32_t readBigEndian32(std::span<const std::uint8_t> bytes, std::size_t offset);

/**
 * Read the code and relocation metadata of a MIPS object.
 *
 * @param path Object file to read.
 * @return The object's text, relocations, and defined text symbols.
 */
ElfObject readElfObject(const std::filesystem::path &path);

} // namespace Tools::Common
