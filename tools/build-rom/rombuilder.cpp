#include "rombuilder.h"

#include <bit>
#include <format>
#include <fstream>
#include <string>

#include <spdlog/spdlog.h>

#include "byteorder.h"
#include "fileio.h"

namespace Tools::BuildRom {

namespace {

using Common::Error;
using Common::ErrorCode;

constexpr std::size_t kBootSize = 0x1000;
constexpr std::size_t kChecksumLength = 0x100000;
constexpr std::size_t kChecksumOffset = 0x10;
constexpr std::uint32_t kSeed6102 = 0xf8ca4ddc;

std::unexpected<Error> invalid(std::string message) {
    return std::unexpected(Error{ErrorCode::InvalidInput, std::move(message)});
}

} // namespace

void writeChecksum(std::span<std::uint8_t> rom) {
    std::uint32_t t1 = kSeed6102;
    std::uint32_t t2 = kSeed6102;
    std::uint32_t t3 = kSeed6102;
    std::uint32_t t4 = kSeed6102;
    std::uint32_t t5 = kSeed6102;
    std::uint32_t t6 = kSeed6102;
    for (std::size_t i = kProgramOffset; i < kProgramOffset + kChecksumLength; i += 4) {
        const std::uint32_t d = Common::readBig32(rom, i);
        if (t6 + d < t6) {
            ++t4;
        }
        t6 += d;
        t3 ^= d;
        const std::uint32_t r = std::rotl(d, static_cast<int>(d & 0x1f));
        t5 += r;
        if (t2 > d) {
            t2 ^= r;
        } else {
            t2 ^= t6 ^ d;
        }
        t1 += t5 ^ d;
    }
    Common::writeBig32(rom, kChecksumOffset, t6 ^ t4 ^ t3);
    Common::writeBig32(rom, kChecksumOffset + 4, t5 ^ t2 ^ t1);
}

std::expected<std::vector<std::uint8_t>, Error> build(std::span<const std::uint8_t> boot,
                                                      std::span<const std::uint8_t> program,
                                                      const std::filesystem::path &assets) {
    if (boot.size() != kBootSize) {
        return invalid(std::format("The boot image is {} bytes, not {}.", boot.size(), kBootSize));
    }
    std::vector<std::uint8_t> rom(kRomSize);
    std::ranges::copy(boot, rom.begin());
    std::ifstream manifest(assets / "manifest.txt");
    if (!manifest) {
        return std::unexpected(Error{
            ErrorCode::Io, std::format("Cannot read {}.", (assets / "manifest.txt").string())});
    }
    std::uint32_t firstAsset = 0;
    std::uint32_t expected = 0;
    std::string offsetText;
    std::string kind;
    std::size_t size = 0;
    while (manifest >> offsetText >> kind >> size) {
        manifest.ignore(1024, '\n');
        const auto offset = static_cast<std::uint32_t>(std::stoul(offsetText, nullptr, 16));
        if (firstAsset == 0) {
            firstAsset = offset;
            expected = offset;
        }
        if (offset != expected) {
            return invalid(
                std::format("The manifest skips from {:#x} to {:#x}.", expected, offset));
        }
        const auto file = assets / (offsetText + (kind == "zlib" ? ".z" : ".bin"));
        const auto data = Common::readFile(file);
        if (!data) {
            return std::unexpected(data.error());
        }
        if (data->size() != size || static_cast<std::size_t>(offset) + size > kRomSize) {
            return invalid(std::format("{} does not fit its manifest entry.", file.string()));
        }
        std::ranges::copy(*data, rom.begin() + offset);
        expected = offset + static_cast<std::uint32_t>(size);
    }
    if (firstAsset == 0) {
        return invalid("The manifest lists no asset.");
    }
    if (program.size() > firstAsset - kProgramOffset) {
        return invalid(std::format("The program is {} bytes and {} are free before the assets.",
                                   program.size(),
                                   firstAsset - kProgramOffset));
    }
    std::ranges::copy(program, rom.begin() + kProgramOffset);
    writeChecksum(rom);
    spdlog::info("The program uses {} of {} bytes before the assets.",
                 program.size(),
                 firstAsset - kProgramOffset);
    return rom;
}

} // namespace Tools::BuildRom
