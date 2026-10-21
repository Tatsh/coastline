#pragma once

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <vector>

#include "error.h"

/** Assembly of a cartridge image from the built program and the extracted assets. */
namespace Tools::BuildRom {

/** Length of a cartridge image. */
constexpr std::size_t kRomSize = 8 * 1024 * 1024;

/** Cartridge offset the IPL3 loads the program from. */
constexpr std::uint32_t kProgramOffset = 0x1000;

/**
 * Write the CIC-NUS-6102 checksum of an image into its header.
 *
 * The checksum covers the MiB from kProgramOffset, the part the IPL3 loads, and the IPL3 refuses
 * an image whose header does not match.
 *
 * @param rom The image, at least kProgramOffset + 1 MiB long.
 */
void writeChecksum(std::span<std::uint8_t> rom);

/**
 * Assemble a cartridge image.
 *
 * The header and the IPL3 come from `boot.bin`, the program goes at kProgramOffset, and every
 * asset of the manifest goes at its own offset. The program must end before the first asset.
 *
 * @param boot The first 0x1000 bytes of the original cartridge.
 * @param program The linked program as flat bytes, text first.
 * @param assets Directory extract-rom wrote, with `manifest.txt`.
 * @return The image with its checksum, or an error.
 */
std::expected<std::vector<std::uint8_t>, Common::Error> build(std::span<const std::uint8_t> boot,
                                                              std::span<const std::uint8_t> program,
                                                              const std::filesystem::path &assets);

} // namespace Tools::BuildRom
