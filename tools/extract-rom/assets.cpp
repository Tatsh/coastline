#include "assets.h"

#include <format>
#include <string>

#include <spdlog/spdlog.h>

#include "assetchain.h"
#include "byteorder.h"
#include "fileio.h"

namespace Tools::ExtractRom {

namespace {

using Common::Error;
using Common::ErrorCode;
using Common::hasZlibHeader;
using Common::padToEven;
using Common::readBig32;

constexpr std::uint32_t kHeaderSize = Common::kAssetHeaderSize;
constexpr std::uint32_t kChunkHeaderSize = Common::kAssetChunkHeaderSize;
constexpr std::size_t kBootSize = 0x1000;

} // namespace

std::expected<std::vector<std::uint8_t>, Error> decompress(std::span<const std::uint8_t> blob) {
    return Common::decompress(blob);
}

std::vector<Asset> scan(const Rom &rom) {
    const std::span<const std::uint8_t> bytes(rom.getBytes());
    const auto end = static_cast<std::uint32_t>(bytes.size());
    std::vector<Asset> assets;
    std::uint32_t rawStart = rom.getAssetStart();
    auto flushRaw = [&](std::uint32_t rawEnd) {
        if (rawEnd > rawStart) {
            assets.push_back({rawStart, rawEnd - rawStart, false, 0, 0});
        }
    };
    for (std::uint32_t position = rom.getAssetStart();
         position + kHeaderSize + kChunkHeaderSize < end;
         position += 2) {
        const std::uint32_t total = readBig32(bytes, position);
        if (total < kHeaderSize + kChunkHeaderSize * 2 || total > end - position ||
            readBig32(bytes, position + 4) == 0 ||
            !hasZlibHeader(bytes, position + kHeaderSize + kChunkHeaderSize)) {
            continue;
        }
        const auto blob = bytes.subspan(position, total);
        const auto data = decompress(blob);
        if (!data) {
            continue;
        }
        std::uint32_t chunks = 0;
        for (std::uint32_t chunk = kHeaderSize; chunk < total;
             chunk += kChunkHeaderSize + padToEven(readBig32(blob, chunk))) {
            ++chunks;
        }
        flushRaw(position);
        const std::uint32_t size = padToEven(total);
        assets.push_back({position, size, true, static_cast<std::uint32_t>(data->size()), chunks});
        spdlog::debug(
            "{:#08x}: {} chunks, {} bytes to {} bytes.", position, chunks, total, data->size());
        position += size - 2;
        rawStart = position + 2;
    }
    flushRaw(end);
    return assets;
}

std::expected<void, Error>
extract(const Rom &rom, const std::vector<Asset> &assets, const std::filesystem::path &directory) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        return std::unexpected(
            Error{ErrorCode::Io, std::format("Cannot create {}.", directory.string())});
    }
    const std::span<const std::uint8_t> boot(rom.getBytes().data(), kBootSize);
    if (auto written = Common::writeFile(directory / "boot.bin", boot); !written) {
        return written;
    }
    std::string manifest;
    for (const auto &asset : assets) {
        const std::span<const std::uint8_t> blob(rom.getBytes().data() + asset.offset, asset.size);
        const std::string stem = std::format("{:06X}", asset.offset);
        if (asset.compressed) {
            if (auto written = Common::writeFile(directory / (stem + ".z"), blob); !written) {
                return written;
            }
            const auto data = decompress(blob);
            if (!data) {
                return std::unexpected(data.error());
            }
            if (auto written = Common::writeFile(directory / (stem + ".bin"), *data); !written) {
                return written;
            }
        } else if (auto written = Common::writeFile(directory / (stem + ".bin"), blob); !written) {
            return written;
        }
        manifest += std::format("{:06X} {} {} {} {}\n",
                                asset.offset,
                                asset.compressed ? "zlib" : "raw",
                                asset.size,
                                asset.decompressedSize,
                                asset.chunks);
    }
    return Common::writeFile(directory / "manifest.txt", manifest);
}

} // namespace Tools::ExtractRom
