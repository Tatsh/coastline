#include "assetchain.h"

#include <zlib.h>

#include "byteorder.h"

namespace Tools::Common {

namespace {

constexpr std::size_t kInflateStep = 16000;
constexpr std::uint8_t kZlibDeflate = 0x78;

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

bool hasZlibHeader(std::span<const std::uint8_t> bytes, std::size_t offset) {
    return offset + 2 <= bytes.size() && bytes[offset] == kZlibDeflate &&
           ((bytes[offset] << 8) | bytes[offset + 1]) % 31 == 0;
}

std::expected<std::vector<std::uint8_t>, Error> decompress(std::span<const std::uint8_t> blob) {
    if (blob.size() < kAssetHeaderSize + kAssetChunkHeaderSize) {
        return std::unexpected(Error{ErrorCode::InvalidInput, "The asset has no chunk."});
    }
    const std::uint32_t total = readBig32(blob, 0);
    const std::uint32_t decompressedSize = readBig32(blob, 4);
    if (total < kAssetHeaderSize + kAssetChunkHeaderSize || total > blob.size()) {
        return std::unexpected(Error{ErrorCode::InvalidInput, "The asset length is wrong."});
    }
    std::vector<std::uint8_t> output;
    output.reserve(decompressedSize);
    std::uint32_t position = kAssetHeaderSize;
    while (position < total) {
        if (total - position < kAssetChunkHeaderSize) {
            return std::unexpected(Error{ErrorCode::InvalidInput, "A chunk header is cut off."});
        }
        const std::uint32_t length = readBig32(blob, position);
        position += kAssetChunkHeaderSize;
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

} // namespace Tools::Common
