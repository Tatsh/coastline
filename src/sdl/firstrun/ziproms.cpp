#include "ziproms.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <span>

#include <zlib.h>

namespace coastline {
namespace {

std::uint16_t Read16(const std::uint8_t *bytes) {
    return static_cast<std::uint16_t>(bytes[0] | (bytes[1] << 8));
}

std::uint32_t Read32(const std::uint8_t *bytes) {
    return Read16(bytes) | (static_cast<std::uint32_t>(Read16(bytes + 2)) << 16);
}

bool Read(std::ifstream &file, std::uint64_t offset, std::span<std::uint8_t> bytes) {
    file.clear();
    file.seekg(static_cast<std::streamoff>(offset));
    return static_cast<bool>(file.read(reinterpret_cast<char *>(bytes.data()),
                                       static_cast<std::streamsize>(bytes.size())));
}

} // namespace

std::optional<std::string>
VisitZipRoms(const std::filesystem::path &archive,
             const std::function<bool(std::vector<std::uint8_t> &)> &visit,
             const std::function<bool(const std::string &, double)> &progress) {
    constexpr std::uint32_t MAX_ROM = 32 * 1024 * 1024;
    const std::string invalid = "The ZIP archive cannot be read.";
    const std::string cancelled = "The search was cancelled.";
    std::ifstream file(archive, std::ios::binary | std::ios::ate);
    if (!file || file.tellg() < 22) {
        return invalid;
    }
    const auto size = static_cast<std::uint64_t>(file.tellg());
    std::vector<std::uint8_t> tail(static_cast<std::size_t>(std::min<std::uint64_t>(size, 65557)));
    if (!Read(file, size - tail.size(), tail)) {
        return invalid;
    }
    std::optional<std::size_t> end;
    for (std::size_t at = tail.size() - 22;; --at) {
        if (Read32(tail.data() + at) == 0x06054B50 &&
            at + 22 + Read16(tail.data() + at + 20) == tail.size()) {
            end = at;
            break;
        }
        if (at == 0) {
            break;
        }
    }
    if (!end) {
        return invalid;
    }
    const auto *record = tail.data() + *end;
    const auto count = Read16(record + 10);
    const std::uint64_t centralSize = Read32(record + 12);
    std::uint64_t at = Read32(record + 16);
    const auto centralEnd = at + centralSize;
    if (Read16(record + 4) != 0 || Read16(record + 6) != 0 || Read16(record + 8) != count ||
        count == 0xFFFF || centralEnd > size - tail.size() + *end) {
        return invalid;
    }
    for (unsigned index = 0; index < count; ++index) {
        std::array<std::uint8_t, 46> header{};
        if (at + header.size() > centralEnd || !Read(file, at, header) ||
            Read32(header.data()) != 0x02014B50) {
            return invalid;
        }
        const auto nameSize = Read16(header.data() + 28);
        const auto extraSize = Read16(header.data() + 30);
        const auto commentSize = Read16(header.data() + 32);
        const auto next = at + header.size() + nameSize + extraSize + commentSize;
        if (next > centralEnd) {
            return invalid;
        }
        std::vector<std::uint8_t> nameBytes(nameSize);
        if (!Read(file, at + header.size(), nameBytes)) {
            return invalid;
        }
        std::string name(nameBytes.begin(), nameBytes.end());
        if (progress &&
            !progress(name, static_cast<double>(index) / std::max<unsigned>(count, 1))) {
            return cancelled;
        }
        at = next;
        std::string extension = name.substr(
            name.find_last_of('.') == std::string::npos ? name.size() : name.find_last_of('.'));
        std::ranges::transform(extension, extension.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        const auto flags = Read16(header.data() + 8);
        const auto method = Read16(header.data() + 10);
        const auto compressed = Read32(header.data() + 20);
        const auto unpacked = Read32(header.data() + 24);
        const std::uint64_t local = Read32(header.data() + 42);
        if ((extension != ".z64" && extension != ".n64" && extension != ".v64" &&
             extension != ".bin") ||
            (flags & 1) || (method != 0 && method != 8) || unpacked == 0 || unpacked > MAX_ROM ||
            compressed > MAX_ROM * 2 || Read16(header.data() + 34) != 0) {
            continue;
        }
        std::array<std::uint8_t, 30> localHeader{};
        if (local + localHeader.size() > centralEnd || !Read(file, local, localHeader) ||
            Read32(localHeader.data()) != 0x04034B50 || Read16(localHeader.data() + 8) != method ||
            Read16(localHeader.data() + 6) != flags) {
            continue;
        }
        const auto payload = local + localHeader.size() + Read16(localHeader.data() + 26) +
                             Read16(localHeader.data() + 28);
        if (payload + compressed > centralEnd) {
            continue;
        }
        std::vector<std::uint8_t> input(compressed);
        if (!Read(file, payload, input)) {
            continue;
        }
        std::vector<std::uint8_t> output(unpacked);
        if (method == 0) {
            if (compressed != unpacked) {
                continue;
            }
            output = std::move(input);
        } else {
            z_stream stream{};
            stream.next_in = input.data();
            stream.avail_in = static_cast<uInt>(input.size());
            if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
                continue;
            }
            int status = Z_OK;
            bool stopped = false;
            while (status == Z_OK && stream.total_out < output.size()) {
                stream.next_out = output.data() + stream.total_out;
                stream.avail_out = static_cast<uInt>(
                    std::min<std::size_t>(output.size() - stream.total_out, 65536));
                status = inflate(&stream, Z_NO_FLUSH);
                if (progress &&
                    !progress(name, static_cast<double>(index) / std::max<unsigned>(count, 1))) {
                    stopped = true;
                    break;
                }
            }
            const bool complete = status == Z_STREAM_END && stream.total_out == unpacked &&
                                  stream.total_in == compressed;
            inflateEnd(&stream);
            if (stopped) {
                return cancelled;
            }
            if (!complete) {
                continue;
            }
        }
        if (crc32(0, output.data(), static_cast<uInt>(output.size())) !=
            Read32(header.data() + 16)) {
            continue;
        }
        if (!visit(output)) {
            return cancelled;
        }
    }
    return std::nullopt;
}

} // namespace coastline
