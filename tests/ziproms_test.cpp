#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <zlib.h>

#include "ziproms.h"

namespace {

void Check(bool condition, const char *message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void Put16(std::vector<std::uint8_t> &bytes, std::size_t at, std::uint32_t value) {
    bytes[at] = static_cast<std::uint8_t>(value);
    bytes[at + 1] = static_cast<std::uint8_t>(value >> 8);
}

void Put32(std::vector<std::uint8_t> &bytes, std::size_t at, std::uint32_t value) {
    Put16(bytes, at, value);
    Put16(bytes, at + 2, value >> 16);
}

struct Entry {
    std::string name;
    std::string text;
    unsigned method = 0;
    unsigned flags = 0;
    bool badCrc = false;
    std::uint32_t claimedSize = 0;
};

std::vector<std::uint8_t> Archive(const std::vector<Entry> &entries) {
    std::vector<std::uint8_t> zip, central;
    for (const auto &entry : entries) {
        std::vector<std::uint8_t> input(entry.text.begin(), entry.text.end());
        auto compressed = input;
        if (entry.method == 8) {
            z_stream stream{};
            Check(deflateInit2(&stream, 6, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) == Z_OK,
                  "Raw deflate must initialise.");
            compressed.resize(compressBound(static_cast<uLong>(input.size())));
            stream.next_in = input.data();
            stream.avail_in = static_cast<uInt>(input.size());
            stream.next_out = compressed.data();
            stream.avail_out = static_cast<uInt>(compressed.size());
            Check(deflate(&stream, Z_FINISH) == Z_STREAM_END, "Fixture compression must finish.");
            compressed.resize(stream.total_out);
            deflateEnd(&stream);
        }
        const auto crc =
            static_cast<std::uint32_t>(crc32(0, input.data(), static_cast<uInt>(input.size()))) ^
            (entry.badCrc ? 1u : 0u);
        const auto size =
            entry.claimedSize ? entry.claimedSize : static_cast<std::uint32_t>(input.size());
        const auto offset = static_cast<std::uint32_t>(zip.size());
        std::vector<std::uint8_t> local(30);
        Put32(local, 0, 0x04034B50);
        Put16(local, 4, 20);
        Put16(local, 6, entry.flags);
        Put16(local, 8, entry.method);
        Put32(local, 14, crc);
        Put32(local, 18, static_cast<std::uint32_t>(compressed.size()));
        Put32(local, 22, size);
        Put16(local, 26, static_cast<std::uint32_t>(entry.name.size()));
        zip.insert(zip.end(), local.begin(), local.end());
        zip.insert(zip.end(), entry.name.begin(), entry.name.end());
        zip.insert(zip.end(), compressed.begin(), compressed.end());
        std::vector<std::uint8_t> header(46);
        Put32(header, 0, 0x02014B50);
        Put16(header, 4, 20);
        Put16(header, 6, 20);
        Put16(header, 8, entry.flags);
        Put16(header, 10, entry.method);
        Put32(header, 16, crc);
        Put32(header, 20, static_cast<std::uint32_t>(compressed.size()));
        Put32(header, 24, size);
        Put16(header, 28, static_cast<std::uint32_t>(entry.name.size()));
        Put32(header, 42, offset);
        central.insert(central.end(), header.begin(), header.end());
        central.insert(central.end(), entry.name.begin(), entry.name.end());
    }
    std::vector<std::uint8_t> end(22);
    Put32(end, 0, 0x06054B50);
    Put16(end, 8, static_cast<std::uint32_t>(entries.size()));
    Put16(end, 10, static_cast<std::uint32_t>(entries.size()));
    Put32(end, 12, static_cast<std::uint32_t>(central.size()));
    Put32(end, 16, static_cast<std::uint32_t>(zip.size()));
    zip.insert(zip.end(), central.begin(), central.end());
    zip.insert(zip.end(), end.begin(), end.end());
    return zip;
}

void Write(const std::filesystem::path &path, const std::vector<std::uint8_t> &bytes) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char *>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
}

} // namespace

int main() {
    const auto path =
        std::filesystem::temp_directory_path() /
        ("coastline-zip-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".zip");
    try {
        const std::string large(200000, 'x');
        auto bytes = Archive({{"../folder/game.z64", "stored"},
                              {".hidden/GAME.V64", large, 8, 8},
                              {"readme.txt", "ignored"},
                              {"bad.n64", "bad CRC", 0, 0, true},
                              {"encrypted.z64", "encrypted", 0, 1},
                              {"large.z64", "oversized", 0, 0, false, 33554433}});
        Write(path, bytes);
        std::vector<std::string> found;
        Check(!coastline::VisitZipRoms(path,
                                       [&](auto &data) {
                                           found.emplace_back(data.begin(), data.end());
                                           return true;
                                       }),
              "A ZIP with stored and deflated entries must be readable.");
        Check(found == std::vector<std::string>{"stored", large},
              "Only bounded, unencrypted ROM entries with valid CRCs must be visited.");
        unsigned visits = 0;
        Check(coastline::VisitZipRoms(
                  path,
                  [&](auto &) {
                      ++visits;
                      return true;
                  },
                  [](const auto &, double) {
                      return false;
                  }).has_value() &&
                  visits == 0,
              "Cancellation must stop before extracting an entry.");
        bytes.resize(bytes.size() - 10);
        Write(path, bytes);
        Check(coastline::VisitZipRoms(path, [](auto &) { return true; }).has_value(),
              "Truncated ZIP directories must be rejected.");
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        std::filesystem::remove(path);
        return 1;
    }
    std::filesystem::remove(path);
    return 0;
}
