#include "assets.h"

#include <format>
#include <string>

#include <spdlog/spdlog.h>
#include <zlib.h>

#include "byteorder.h"
#include "fileio.h"

namespace Tools::ExtractRom {

namespace {

using Common::Error;
using Common::ErrorCode;
using Common::readBig32;

constexpr std::uint32_t kHeaderSize = 8;
constexpr std::uint32_t kChunkHeaderSize = 4;
constexpr std::size_t kBootSize = 0x1000;
constexpr std::size_t kInflateStep = 16000;
constexpr std::uint8_t kZlibDeflate = 0x78;

std::uint32_t padToEven(std::uint32_t length) {
    return (length + 1) & ~std::uint32_t{1};
}

// A zlib header is a deflate method byte and a flags byte whose 16-bit value is a multiple of 31.
bool hasZlibHeader(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return offset + 2 <= bytes.size() && bytes[offset] == kZlibDeflate &&
           ((bytes[offset] << 8) | bytes[offset + 1]) % 31 == 0;
}

std::expected<void, Error> inflateChunk(std::span<const std::uint8_t> chunk,
                                        std::vector<std::uint8_t> &output) {
    z_stream stream{};
    if (inflateInit(&stream) != Z_OK) {
        return std::unexpected(Error{ErrorCode::InvalidInput, "zlib cannot initialize."});
    }
    stream.next_in = const_cast<Bytef *>(chunk.data());
    stream.avail_in = static_cast<uInt>(chunk.size());
    int result = Z_OK;
    while (result == Z_OK) {
        const std::size_t before = output.size();
        output.resize(before + kInflateStep);
        stream.next_out = output.data() + before;
        stream.avail_out = kInflateStep;
        result = inflate(&stream, Z_NO_FLUSH);
        output.resize(before + kInflateStep - stream.avail_out);
    }
    inflateEnd(&stream);
    if (result != Z_STREAM_END || stream.avail_in != 0) {
        return std::unexpected(Error{ErrorCode::InvalidInput, "A zlib chunk is malformed."});
    }
    return {};
}

} // namespace

std::expected<std::vector<std::uint8_t>, Error> decompress(std::span<const std::uint8_t> blob) {
    if (blob.size() < kHeaderSize + kChunkHeaderSize) {
        return std::unexpected(Error{ErrorCode::InvalidInput, "The asset has no chunk."});
    }
    const std::uint32_t total = readBig32(blob, 0);
    const std::uint32_t decompressedSize = readBig32(blob, 4);
    if (total < kHeaderSize + kChunkHeaderSize || total > blob.size()) {
        return std::unexpected(Error{ErrorCode::InvalidInput, "The asset length is wrong."});
    }
    std::vector<std::uint8_t> output;
    output.reserve(decompressedSize);
    std::uint32_t position = kHeaderSize;
    while (position < total) {
        if (total - position < kChunkHeaderSize) {
            return std::unexpected(Error{ErrorCode::InvalidInput, "A chunk header is cut off."});
        }
        const std::uint32_t length = readBig32(blob, position);
        position += kChunkHeaderSize;
        if (length == 0 || padToEven(length) > total - position || !hasZlibHeader(blob, position)) {
            return std::unexpected(Error{ErrorCode::InvalidInput, "A chunk is malformed."});
        }
        if (auto inflated = inflateChunk(blob.subspan(position, length), output); !inflated) {
            return std::unexpected(inflated.error());
        }
        position += padToEven(length);
    }
    if (output.size() != decompressedSize) {
        return std::unexpected(
            Error{ErrorCode::InvalidInput, "The decompressed length does not match the header."});
    }
    return output;
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
