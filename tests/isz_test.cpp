#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>

#if defined(_WIN32) && !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <bzlib.h>
#include <zlib.h>

#include "disctrackreader.h"
#include "isz.h"

namespace {
using Bytes = std::vector<std::uint8_t>;

void Number(Bytes &bytes, std::size_t at, std::uint64_t value, std::size_t count) {
    for (std::size_t index = 0; index < count; ++index) {
        bytes.at(at + index) = static_cast<std::uint8_t>(value >> (index * 8));
    }
}

void Write(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    assert(file.good());
}

Bytes Read(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(file), {});
}

void Obfuscate(Bytes &bytes) {
    constexpr std::uint8_t KEY[]{0xb6, 0x8c, 0xa5, 0xde};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] ^= KEY[index % 4];
    }
}

void Check(const std::filesystem::path &root,
           unsigned pointerSize,
           bool split,
           bool extended,
           std::size_t blockSize = 4096) {
    const auto BLOCK = blockSize;
    const auto TOTAL = 8 * BLOCK + 2048;
    const auto COUNT = (TOTAL + BLOCK - 1) / BLOCK;
    const unsigned headerSize = extended ? 64 : 48;
    Bytes wanted(TOTAL), table(COUNT * pointerSize), payload;
    std::size_t splitAt = 0, spill = 0;
    uLong compressedCrc = crc32(0, nullptr, 0);
    for (std::size_t index = 0; index < COUNT; ++index) {
        const auto size = std::min(BLOCK, TOTAL - index * BLOCK);
        const unsigned method = pointerSize == 0 ? 1 : pointerSize == 1 ? 2 : index % 4;
        if (method != 0 && pointerSize != 1) {
            for (std::size_t byte = 0; byte < size; ++byte) {
                wanted[index * BLOCK + byte] = static_cast<std::uint8_t>(byte * 17 + byte / 31);
            }
        }
        const auto chunk = std::span(wanted).subspan(index * BLOCK, size);
        Bytes stored;
        if (method == 1) {
            stored.assign(chunk.begin(), chunk.end());
        } else if (method == 2) {
            uLongf length = compressBound(size);
            stored.resize(length);
            assert(compress2(stored.data(), &length, chunk.data(), size, 9) == Z_OK);
            stored.resize(length);
        } else if (method == 3) {
            unsigned length = static_cast<unsigned>(size * 2 + 600);
            stored.resize(length);
            assert(BZ2_bzBuffToBuffCompress(
                       reinterpret_cast<char *>(stored.data()),
                       &length,
                       reinterpret_cast<char *>(const_cast<std::uint8_t *>(chunk.data())),
                       size,
                       9,
                       0,
                       30) == BZ_OK);
            stored.resize(length);
            stored[0] = 'I';
            stored[1] = 's';
            stored[2] = 'Z';
        }
        if (pointerSize != 0) {
            const auto value = (static_cast<std::uint64_t>(method) << (pointerSize * 8 - 2)) |
                               (method == 0 ? size : stored.size());
            Number(table, index * pointerSize, value, pointerSize);
        }
        if (!stored.empty()) {
            compressedCrc = crc32(compressedCrc, stored.data(), stored.size());
        }
        if (index == 3) {
            spill = stored.size() / 2;
            splitAt = payload.size() + stored.size() - spill;
        }
        payload.insert(payload.end(), stored.begin(), stored.end());
    }
    Obfuscate(table);
    Bytes header(headerSize);
    std::copy_n("IsZ!", 4, header.begin());
    header[4] = headerSize;
    header[5] = 1;
    Number(header, 6, 0x12345678, 4);
    Number(header, 10, 2048, 2);
    Number(header, 12, TOTAL / 2048, 4);
    Number(header, 25, COUNT, 4);
    Number(header, 29, BLOCK, 4);
    header[33] = pointerSize;
    const bool indexed = split && pointerSize != 0;
    if (split && !indexed) {
        splitAt = payload.size() * 2 / 3;
    }
    const auto tableOffset = headerSize + (indexed ? 72 : 0);
    const auto dataOffset = tableOffset + table.size();
    Number(header, 35, pointerSize == 0 ? 0 : tableOffset, 4);
    Number(header, 39, indexed ? headerSize : 0, 4);
    Number(header, 43, dataOffset, 4);
    if (extended) {
        Number(header, 48, static_cast<std::uint32_t>(~crc32(0, wanted.data(), wanted.size())), 4);
        Number(header, 52, wanted.size(), 4);
        Number(header, 60, static_cast<std::uint32_t>(~compressedCrc), 4);
    }
    const auto image = root / "image.isz";
    Bytes first = header;
    Bytes segments;
    if (split) {
        Number(header, 17, dataOffset + splitAt, 8);
        first = header;
    }
    if (indexed) {
        segments.resize(72);
        Number(segments, 0, dataOffset + splitAt, 8);
        Number(segments, 8, 4, 4);
        Number(segments, 16, dataOffset, 4);
        Number(segments, 20, spill, 4);
        Number(segments, 24, headerSize + payload.size() - splitAt, 8);
        Number(segments, 32, COUNT - 4, 4);
        Number(segments, 36, 4, 4);
        Number(segments, 40, headerSize + spill, 4);
        Obfuscate(segments);
        first.insert(first.end(), segments.begin(), segments.end());
    }
    if (split) {
        Bytes second = header;
        second[34] = 1;
        second.insert(second.end(), payload.begin() + splitAt, payload.end());
        Write(root / "image.i01", second);
    } else {
        splitAt = payload.size();
    }
    first.insert(first.end(), table.begin(), table.end());
    first.insert(first.end(), payload.begin(), payload.begin() + splitAt);
    Write(image, first);
    double previous = -1.0;
    auto decoded = coastline::DecodeIsz(image, [&](const std::string &, double fraction) {
        assert(fraction >= previous && fraction <= 1.0);
        previous = fraction;
        return true;
    });
    assert(decoded && previous == 1.0 && Read(decoded->file) == wanted);
    if (split) {
        for (const auto *suffix : {".part01.isz", ".part001.isz", ".ISZ"}) {
            const auto variant = root / (std::string("variant") + suffix);
            const auto next = root / (std::string("variant") +
                                      (std::string_view(suffix) == ".part01.isz"  ? ".part02.isz" :
                                       std::string_view(suffix) == ".part001.isz" ? ".part002.isz" :
                                                                                    ".I01"));
            Write(variant, first);
            std::filesystem::copy_file(
                root / "image.i01", next, std::filesystem::copy_options::overwrite_existing);
            const auto alternate = coastline::DecodeIsz(variant);
            assert(alternate && Read(alternate->file) == wanted);
        }
    }
    const auto temporary = decoded->file;
    coastline::CueTrack track;
    track.file = temporary;
    track.sectorSize = 2048;
    track.sectorCount = TOTAL / 2048;
    track.storage = decoded->storage;
    auto reader = std::make_unique<coastline::DiscTrackReader>(track);
    decoded = std::unexpected("released");
    track.storage.reset();
    assert(std::filesystem::is_regular_file(temporary));
    Bytes actual(TOTAL);
    assert(reader->Read(0, TOTAL / 2048, actual) && actual == wanted);
    reader.reset();
    assert(!std::filesystem::exists(temporary.parent_path()));
    for (const double limit : {0.0, 0.5, 1.0}) {
        const auto cancelled = coastline::DecodeIsz(
            image, [&](const std::string &, double fraction) { return fraction < limit; });
        assert(!cancelled && cancelled.error() == "Import cancelled.");
    }
    auto invalid = [&](std::size_t offset, std::uint64_t value, std::size_t length) {
        auto broken = first;
        Number(broken, offset, value, length);
        Write(image, broken);
        assert(!coastline::DecodeIsz(image));
    };
    invalid(0, 0, 1);
    invalid(4, 49, 1);
    invalid(5, 2, 1);
    invalid(10, 0, 2);
    invalid(12, UINT32_MAX, 4);
    invalid(16, 2, 1);
    invalid(25, UINT32_MAX, 4);
    invalid(29, UINT32_MAX, 4);
    invalid(34, 1, 1);
    invalid(43, UINT32_MAX, 4);
    if (pointerSize != 0) {
        invalid(33, 0, 1);
        invalid(33, 5, 1);
        invalid(35, UINT32_MAX, 4);
        invalid(tableOffset, first[tableOffset] ^ 1, 1);
    }
    if (extended) {
        invalid(48, 0, 4);
        invalid(60, 0, 4);
    }
    if (indexed) {
        invalid(39, UINT32_MAX, 4);
        invalid(headerSize + 12, 0, 4);
        invalid(headerSize + 20, UINT32_MAX, 4);
        invalid(headerSize + 40, 0, 4);
    }
    for (std::size_t length = 0; length < dataOffset; ++length) {
        Write(image, std::span(first).first(length));
        assert(!coastline::DecodeIsz(image));
    }
    Write(image, std::span(first).first(first.size() - 1));
    assert(!coastline::DecodeIsz(image));
    Write(image, first);
    if (split) {
        std::filesystem::remove(root / "image.i01");
        assert(!coastline::DecodeIsz(image));
    }
}
} // namespace

int main() {
    const auto root = std::filesystem::temp_directory_path() / "coastline-isz-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    for (const bool extended : {false, true}) {
        for (const unsigned pointerSize : {0, 1, 2, 3, 4}) {
            Check(root, pointerSize, false, extended);
            Check(root, pointerSize, true, extended);
        }
    }
    Check(root, 3, true, true, 262144);
    std::filesystem::remove_all(root);
}
