#include "isz.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>

#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <bzlib.h>
#include <fmt/format.h>
#include <zlib.h>

namespace coastline {
namespace {
using Bytes = std::span<const std::uint8_t>;
constexpr std::size_t BUFFER_SIZE = 65536;
constexpr std::string_view INVALID = "Invalid or unsupported ISZ image.";
constexpr std::string_view CANCELLED = "Import cancelled.";

std::uint64_t Number(Bytes bytes, std::size_t offset, std::size_t count) {
    std::uint64_t result = 0;
    for (std::size_t index = 0; index < count; ++index) {
        result |= static_cast<std::uint64_t>(bytes[offset + index]) << (index * 8);
    }
    return result;
}

void Deobfuscate(std::span<std::uint8_t> bytes) {
    constexpr std::array<std::uint8_t, 4> KEY{0xb6, 0x8c, 0xa5, 0xde};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] ^= KEY[index % KEY.size()];
    }
}

class Part;

struct Range {
    Part *part;
    std::uint64_t offset;
    std::uint64_t size;
};

class Part {
public:
    bool Open(const std::filesystem::path &image) {
        m_path = image;
        std::error_code error;
        m_size = std::filesystem::file_size(m_path, error);
        if (error ||
            m_size > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
            return false;
        }
        m_file.open(m_path, std::ios::binary);
        if (!Read(0, std::span(m_header).first(48)) ||
            !std::ranges::equal(std::span(m_header).first(4), std::string_view("IsZ!")) ||
            (m_header[4] != 48 && m_header[4] != 64) || m_header[5] != 1) {
            return false;
        }
        return m_header[4] == 48 || Read(48, std::span(m_header).subspan(48));
    }

    bool Read(std::uint64_t position, std::span<std::uint8_t> bytes) {
        if (position > m_size || bytes.size() > m_size - position) {
            return false;
        }
        m_file.clear();
        m_file.seekg(static_cast<std::streamoff>(position));
        return static_cast<bool>(m_file.read(reinterpret_cast<char *>(bytes.data()),
                                             static_cast<std::streamsize>(bytes.size())));
    }

    bool Matches(Bytes header, std::size_t index) const {
        return m_header[34] == index &&
               std::ranges::equal(std::span(m_header).first(34), header.first(34));
    }

    bool
    LoadSegment(Bytes entry, std::uint64_t first, std::uint64_t blocks, std::uint64_t headerSize) {
        m_size = Number(entry, 0, 8);
        m_first = Number(entry, 12, 4);
        m_count = Number(entry, 8, 4);
        m_offset = Number(entry, 16, 4);
        m_spill = Number(entry, 20, 4);
        return m_first == first && m_count <= blocks - first && m_offset >= headerSize &&
               m_offset <= m_size && (m_count != 0 || m_spill == 0);
    }

    void Start(std::uint64_t offset, std::uint64_t blocks = 0) {
        m_count = blocks;
        m_offset = offset;
    }

    Range Take(std::uint64_t length) {
        const Range range{this, m_offset, length};
        m_offset += length;
        return range;
    }

    std::uint64_t EndBlock() const {
        return m_first + m_count;
    }

    std::uint64_t SpillAfter(std::uint64_t block) const {
        return block + 1 == m_first + m_count ? m_spill : 0;
    }

    bool Spills() const {
        return m_spill != 0;
    }

    const std::filesystem::path &Path() const {
        return m_path;
    }

    const std::array<std::uint8_t, 64> &Header() const {
        return m_header;
    }

    std::uint64_t Size() const {
        return m_size;
    }

    std::uint64_t Offset() const {
        return m_offset;
    }

private:
    std::filesystem::path m_path;
    std::ifstream m_file;
    std::uint64_t m_size = 0;
    std::uint64_t m_offset = 0;
    std::uint64_t m_first = 0;
    std::uint64_t m_count = 0;
    std::uint64_t m_spill = 0;
    std::array<std::uint8_t, 64> m_header{};
};

std::filesystem::path PartPath(const std::filesystem::path &image, std::size_t index) {
    if (index == 0) {
        return image;
    }
    auto name = image.filename().u8string();
    auto lower = name;
    std::ranges::transform(lower, lower.begin(), [](char8_t value) {
        return value >= u8'A' && value <= u8'Z' ? static_cast<char8_t>(value + 32) : value;
    });
    std::filesystem::path result;
    if (lower.ends_with(u8".part001.isz")) {
        result = image.parent_path() / std::filesystem::path(name.substr(0, name.size() - 12));
        result += fmt::format(".part{:03}.isz", index + 1);
    } else if (lower.ends_with(u8".part01.isz")) {
        result = image.parent_path() / std::filesystem::path(name.substr(0, name.size() - 11));
        result += fmt::format(".part{:02}.isz", index + 1);
    } else {
        result = image;
        result.replace_extension(fmt::format(".i{:02}", index));
    }
    std::error_code error;
    if (std::filesystem::is_regular_file(result, error)) {
        return result;
    }
    auto wanted = result.filename().u8string();
    auto equal = [](char8_t a, char8_t b) {
        const auto lower = [](char8_t value) {
            return value >= u8'A' && value <= u8'Z' ? static_cast<char8_t>(value + 32) : value;
        };
        return lower(a) == lower(b);
    };
    const auto parent =
        result.parent_path().empty() ? std::filesystem::path(".") : result.parent_path();
    std::filesystem::directory_iterator entry(parent, error), end;
    for (; !error && entry != end; entry.increment(error)) {
        if (std::ranges::equal(wanted, entry->path().filename().u8string(), equal)) {
            return entry->path();
        }
    }
    return result;
}

struct Block {
    unsigned method;
    std::uint64_t length;
};

std::optional<Block> ReadBlock(Part &part,
                               std::uint64_t pointers,
                               unsigned pointerSize,
                               std::uint64_t index,
                               std::uint64_t expected,
                               std::uint64_t blockSize) {
    Block block{1, expected};
    if (pointers != 0) {
        std::array<std::uint8_t, 4> pointer{};
        auto entry = std::span(pointer).first(pointerSize);
        if (!part.Read(pointers + index * pointerSize, entry)) {
            return std::nullopt;
        }
        constexpr std::array<std::uint8_t, 4> KEY{0xb6, 0x8c, 0xa5, 0xde};
        for (std::size_t byte = 0; byte < pointerSize; ++byte) {
            entry[byte] ^= KEY[(index * pointerSize + byte) % KEY.size()];
        }
        const auto value = Number(entry, 0, pointerSize);
        const auto bits = pointerSize * 8 - 2;
        block.method = static_cast<unsigned>(value >> bits);
        block.length = value & ((std::uint64_t{1} << bits) - 1);
    }
    return block.length != 0 && block.length <= blockSize &&
                   (block.method >= 2 || block.length == expected) ?
               std::optional(block) :
               std::nullopt;
}

class ChunkInput {
public:
    explicit ChunkInput(std::vector<Range> ranges) : m_ranges(std::move(ranges)) {
    }

    bool Read(std::span<std::uint8_t> bytes) {
        while (!bytes.empty() && m_index < m_ranges.size()) {
            auto &range = m_ranges[m_index];
            const auto count =
                static_cast<std::size_t>(std::min<std::uint64_t>(range.size, bytes.size()));
            if (!range.part->Read(range.offset, bytes.first(count))) {
                return false;
            }
            bytes = bytes.subspan(count);
            range.offset += count;
            range.size -= count;
            if (range.size == 0) {
                ++m_index;
            }
        }
        return bytes.empty();
    }

private:
    std::vector<Range> m_ranges;
    std::size_t m_index = 0;
};

class Inflater {
public:
    explicit Inflater(unsigned type)
        : m_method(type), m_valid(type == 2 ? inflateInit(&m_z) == Z_OK :
                                              BZ2_bzDecompressInit(&m_b, 0, 0) == BZ_OK) {
    }

    Inflater(const Inflater &) = delete;
    Inflater &operator=(const Inflater &) = delete;

    ~Inflater() {
        if (m_valid) {
            if (m_method == 2) {
                inflateEnd(&m_z);
            } else {
                BZ2_bzDecompressEnd(&m_b);
            }
        }
    }

    bool Valid() const {
        return m_valid;
    }

    void Feed(std::uint8_t *data, unsigned count) {
        m_z.next_in = data;
        m_z.avail_in = count;
        m_b.next_in = reinterpret_cast<char *>(data);
        m_b.avail_in = count;
    }

    int Decompress(std::span<std::uint8_t> out, std::size_t &produced) {
        if (m_method == 2) {
            m_z.next_out = out.data();
            m_z.avail_out = static_cast<uInt>(out.size());
            const int status = inflate(&m_z, Z_NO_FLUSH);
            produced = out.size() - m_z.avail_out;
            return status;
        }
        m_b.next_out = reinterpret_cast<char *>(out.data());
        m_b.avail_out = static_cast<unsigned>(out.size());
        const int status = BZ2_bzDecompress(&m_b);
        produced = out.size() - m_b.avail_out;
        return status;
    }

    unsigned Remaining() const {
        return m_method == 2 ? m_z.avail_in : m_b.avail_in;
    }

private:
    z_stream m_z{};
    bz_stream m_b{};
    unsigned m_method;
    bool m_valid;
};

std::optional<std::string> DecodeChunk(ChunkInput input,
                                       unsigned method,
                                       std::uint64_t stored,
                                       std::uint64_t expected,
                                       std::ofstream &file,
                                       uLong &compressedCrc,
                                       uLong &decodedCrc,
                                       const ImportProgress &progress) {
    const auto invalid = std::string(INVALID);
    std::array<std::uint8_t, BUFFER_SIZE> in{}, out{};
    std::uint64_t written = 0;
    auto write = [&](std::span<const std::uint8_t> bytes) -> std::optional<std::string> {
        if (bytes.size() > expected - written) {
            return invalid;
        }
        if (!file.write(reinterpret_cast<const char *>(bytes.data()),
                        static_cast<std::streamsize>(bytes.size()))) {
            return "Cannot write decoded disc image.";
        }
        decodedCrc = crc32(decodedCrc, bytes.data(), static_cast<uInt>(bytes.size()));
        written += bytes.size();
        if (progress && !progress("Decoding ISZ image...",
                                  static_cast<double>(written) / static_cast<double>(expected))) {
            return std::string(CANCELLED);
        }
        return std::nullopt;
    };
    if (method < 2) {
        while (written < expected) {
            const auto count =
                static_cast<std::size_t>(std::min<std::uint64_t>(expected - written, out.size()));
            if (method == 1) {
                if (!input.Read(std::span(out).first(count))) {
                    return invalid;
                }
                compressedCrc = crc32(compressedCrc, out.data(), static_cast<uInt>(count));
            }
            if (auto failure = write(std::span(out).first(count))) {
                return failure;
            }
        }
        return std::nullopt;
    }
    Inflater decoder(method);
    if (!decoder.Valid()) {
        return "Cannot initialize disc decompression.";
    }
    bool first = true;
    unsigned remaining = 0;
    while (true) {
        if (remaining == 0 && stored != 0) {
            const auto count = static_cast<unsigned>(std::min<std::uint64_t>(stored, in.size()));
            if (!input.Read(std::span(in).first(count))) {
                return invalid;
            }
            compressedCrc = crc32(compressedCrc, in.data(), count);
            if (method == 3 && first) {
                if (count < 4) {
                    return invalid;
                }
                in[0] = 'B';
                in[1] = 'Z';
                in[2] = 'h';
            }
            first = false;
            stored -= count;
            remaining = count;
            decoder.Feed(in.data(), count);
        }
        const auto previous = remaining;
        std::size_t produced = 0;
        const int status = decoder.Decompress(out, produced);
        remaining = decoder.Remaining();
        if (auto failure = write(std::span(out).first(produced))) {
            return failure;
        }
        if (status == (method == 2 ? Z_STREAM_END : BZ_STREAM_END)) {
            return remaining == 0 && stored == 0 && written == expected ? std::nullopt :
                                                                          std::optional(invalid);
        }
        if (status != (method == 2 ? Z_OK : BZ_OK) || (produced == 0 && previous == remaining)) {
            return invalid;
        }
    }
}
} // namespace

std::expected<DecodedDisc, std::string> DecodeIsz(const std::filesystem::path &image,
                                                  const ImportProgress &progress) {
    const auto invalid = std::unexpected(std::string(INVALID));
    std::vector<Part> parts(1);
    if (!parts[0].Open(image)) {
        return invalid;
    }
    const auto header = parts[0].Header();
    if (header[16] != 0) {
        return std::unexpected(std::string("Encrypted ISZ images are not supported."));
    }
    const auto sectorSize = Number(header, 10, 2);
    const auto total = Number(header, 12, 4) * sectorSize;
    const auto count = Number(header, 25, 4);
    const auto blockSize = Number(header, 29, 4);
    const auto pointerSize = header[33];
    const auto pointers = Number(header, 35, 4);
    const auto segments = Number(header, 39, 4);
    const auto data = Number(header, 43, 4);
    if (header[34] != 0 || (sectorSize != 2048 && sectorSize != 2352) || total == 0 || count == 0 ||
        blockSize == 0 || blockSize % sectorSize != 0 ||
        count != (total + blockSize - 1) / blockSize || data < header[4] ||
        data > parts[0].Size() ||
        (pointers != 0 && (pointerSize == 0 || pointerSize > 4 || pointers < header[4] ||
                           pointers > data || count * pointerSize > data - pointers))) {
        return invalid;
    }
    auto temporary = CreateDecodedDisc(sectorSize != 2048, total);
    if (!temporary) {
        return std::unexpected(temporary.error());
    }
    if (progress && !progress("Reading ISZ metadata...", 0.0)) {
        return std::unexpected(std::string(CANCELLED));
    }
    std::uint64_t metadataEnd = header[4];
    if (segments != 0) {
        if (segments < header[4] || segments > data) {
            return invalid;
        }
        std::uint64_t first = 0;
        for (std::size_t index = 0; index <= 99; ++index) {
            std::array<std::uint8_t, 24> entry{};
            if (!parts[0].Read(segments + index * entry.size(), entry)) {
                return invalid;
            }
            Deobfuscate(entry);
            const auto size = Number(entry, 0, 8);
            metadataEnd = segments + (index + 1) * entry.size();
            if (size == 0) {
                if (index == 0 || first != count) {
                    return invalid;
                }
                break;
            }
            if (index == 99) {
                return invalid;
            }
            if (index != 0) {
                parts.emplace_back();
                if (!parts.back().Open(PartPath(image, index))) {
                    return std::unexpected(
                        fmt::format("Missing or invalid ISZ segment {}.", index));
                }
            }
            auto &part = parts[index];
            if (!part.Matches(header, index) || size > part.Size() || size < header[4] ||
                !part.LoadSegment(entry, first, count, header[4])) {
                return invalid;
            }
            first = part.EndBlock();
        }
        if (parts.back().Spills() || metadataEnd > data ||
            (pointers != 0 &&
             !(metadataEnd <= pointers || pointers + count * pointerSize <= segments))) {
            return invalid;
        }
    } else {
        parts[0].Start(data, count);
        const auto segmentSize = Number(header, 17, 8);
        if (segmentSize != 0) {
            std::uint64_t stored = 0;
            for (std::uint64_t index = 0; index < count; ++index) {
                if (index % 1024 == 0 && progress && !progress("Reading ISZ metadata...", 0.0)) {
                    return std::unexpected(std::string(CANCELLED));
                }
                const auto expected = std::min(blockSize, total - index * blockSize);
                const auto block =
                    ReadBlock(parts[0], pointers, pointerSize, index, expected, blockSize);
                if (!block) {
                    return invalid;
                }
                stored += block->method == 0 ? 0 : block->length;
            }
            for (std::size_t index = 0;; ++index) {
                auto &part = parts.back();
                if (segmentSize <= part.Offset() || part.Size() > segmentSize ||
                    !part.Matches(header, index)) {
                    return invalid;
                }
                if (stored <= part.Size() - part.Offset()) {
                    break;
                }
                if (part.Size() != segmentSize || index + 1 >= 99) {
                    return invalid;
                }
                stored -= part.Size() - part.Offset();
                parts.emplace_back();
                if (!parts.back().Open(PartPath(image, index + 1))) {
                    return std::unexpected(
                        fmt::format("Missing or invalid ISZ segment {}.", index + 1));
                }
                parts.back().Start(header[4]);
            }
        }
    }
    metadataEnd = std::max(metadataEnd, pointers == 0 ? 0 : pointers + count * pointerSize);
    if (parts[0].Offset() < metadataEnd || parts[0].Offset() < data) {
        return invalid;
    }
    auto decoded = std::move(*temporary);
    for (const auto &part : parts) {
        decoded.sources.push_back(part.Path());
    }
    std::ofstream output(decoded.file, std::ios::binary);
    if (!output) {
        return std::unexpected(std::string("Cannot create decoded disc image."));
    }
    uLong compressedCrc = crc32(0, nullptr, 0), decodedCrc = compressedCrc;
    std::uint64_t written = 0;
    std::size_t segment = 0;
    if (progress && !progress("Decoding ISZ image...", 0.0)) {
        return std::unexpected(std::string(CANCELLED));
    }
    for (std::uint64_t index = 0; index < count; ++index) {
        const auto expected = std::min(blockSize, total - written);
        const auto block = ReadBlock(parts[0], pointers, pointerSize, index, expected, blockSize);
        if (!block) {
            return invalid;
        }
        const auto stored = block->method == 0 ? 0 : block->length;
        std::vector<Range> ranges;
        if (segments == 0) {
            auto remaining = stored;
            while (remaining != 0) {
                while (segment < parts.size() && parts[segment].Offset() == parts[segment].Size()) {
                    ++segment;
                }
                if (segment == parts.size()) {
                    return invalid;
                }
                auto &part = parts[segment];
                const auto length = std::min(remaining, part.Size() - part.Offset());
                ranges.push_back(part.Take(length));
                remaining -= length;
            }
        } else {
            while (segment < parts.size() && index >= parts[segment].EndBlock()) {
                ++segment;
            }
            if (segment == parts.size()) {
                return invalid;
            }
            auto &part = parts[segment];
            const auto spill = part.SpillAfter(index);
            if (spill > stored || part.Offset() > part.Size() ||
                stored - spill > part.Size() - part.Offset()) {
                return invalid;
            }
            ranges.push_back(part.Take(stored - spill));
            if (spill != 0) {
                if (segment + 1 == parts.size() || spill > parts[segment + 1].Size() - header[4] ||
                    header[4] + spill > parts[segment + 1].Offset()) {
                    return invalid;
                }
                ranges.push_back({&parts[segment + 1], header[4], spill});
            }
        }
        const ImportProgress report =
            progress ?
                [&](const std::string &step, double fraction) {
                    return progress(step,
                                    (static_cast<double>(written) + fraction * expected) /
                                        static_cast<double>(total));
                } :
                ImportProgress{};
        if (auto failure = DecodeChunk(ChunkInput(std::move(ranges)),
                                       block->method,
                                       stored,
                                       expected,
                                       output,
                                       compressedCrc,
                                       decodedCrc,
                                       report)) {
            return std::unexpected(*failure);
        }
        written += expected;
    }
    output.close();
    if (!output) {
        return std::unexpected(std::string("Cannot finish decoded disc image."));
    }
    if (header[4] == 64 && (static_cast<std::uint32_t>(~decodedCrc) != Number(header, 48, 4) ||
                            static_cast<std::uint32_t>(~compressedCrc) != Number(header, 60, 4))) {
        return std::unexpected(std::string("ISZ image checksum mismatch."));
    }
    return decoded;
}
} // namespace coastline
