#include <algorithm>
#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "disclayout.h"
#include "sha1.h"

namespace {

constexpr std::size_t SECTOR = 2352;

void Write(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
    assert(file.good());
}

void Text(const std::filesystem::path &path, const std::string &text) {
    std::ofstream file(path);
    file << text;
    assert(file.good());
}

std::vector<std::uint8_t> DataTrack() {
    std::vector<std::uint8_t> bytes(SECTOR * 32);
    auto bcd = [](std::size_t value) {
        return static_cast<std::uint8_t>((value / 10) * 16 + value % 10);
    };
    for (std::size_t i = 0; i < 32; ++i) {
        auto *sector = bytes.data() + i * SECTOR;
        std::fill_n(sector + 1, 10, 0xff);
        sector[12] = bcd((i + 150) / 4500);
        sector[13] = bcd((i + 150) / 75 % 60);
        sector[14] = bcd((i + 150) % 75);
        sector[15] = 1;
    }
    auto *descriptor = bytes.data() + 16 * SECTOR + 16;
    descriptor[0] = 1;
    std::copy_n("CD001", 5, descriptor + 1);
    descriptor[6] = 1;
    descriptor[80] = 32;
    return bytes;
}

void CheckHash() {
    std::istringstream abc("abc");
    assert(Tools::Common::sha1Hex(abc) == "a9993e364706816aba3e25717850c26c9cd0d89d");
    std::istringstream empty;
    assert(Tools::Common::sha1Hex(empty) == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
    for (std::size_t size = 0; size < 200; ++size) {
        std::vector<std::uint8_t> bytes(size, 0x5a);
        std::istringstream input(std::string(bytes.begin(), bytes.end()));
        assert(Tools::Common::sha1Hex(input) == Tools::Common::sha1Hex(bytes));
    }
    std::istringstream broken("abc");
    broken.setstate(std::ios::badbit);
    assert(!Tools::Common::sha1Hex(broken));
    broken.setstate(std::ios::eofbit);
    assert(!Tools::Common::sha1Hex(broken));
}

void CheckDiscovery(const std::filesystem::path &root) {
    auto bytes = DataTrack();
    for (int track = 0; track < 12; ++track) {
        bytes.insert(bytes.end(), SECTOR * 150, 0);
        bytes.insert(bytes.end(), SECTOR * 300, static_cast<std::uint8_t>(track + 1));
    }
    const auto bin = root / "unknown.bin";
    Write(bin, bytes);
    auto tracks = coastline::ReadDiscTracks(bin);
    assert(tracks && tracks->size() == 13);
    assert(tracks->front().sectorCount == 32);
    for (std::size_t i = 1; i < tracks->size(); ++i) {
        assert((*tracks)[i].number == static_cast<int>(i + 1));
        assert((*tracks)[i].audio && (*tracks)[i].sectorCount == 300);
        assert((*tracks)[i].firstSector == 32 + 150 + (i - 1) * 450);
    }
    bytes.resize(bytes.size() - 450 * SECTOR);
    Write(bin, bytes);
    tracks = coastline::ReadDiscTracks(bin);
    assert(tracks && tracks->size() == 1);
    auto invalid = DataTrack();
    invalid[20 * SECTOR + 14] = 0xff;
    invalid.insert(invalid.end(), bytes.begin() + 32 * SECTOR, bytes.end());
    Write(bin, invalid);
    tracks = coastline::ReadDiscTracks(bin);
    assert(tracks && tracks->size() == 1);
    Write(bin, DataTrack());
    Text(root / "UNKNOWN.CUE",
         "FILE \"UNKNOWN.BIN\" BINARY\n"
         " TRACK 01 MODE1/2352\n INDEX 01 00:00:00\n");
    tracks = coastline::ReadDiscTracks(bin);
    assert(tracks && tracks->size() == 1 && tracks->front().file == bin);
    std::filesystem::rename(root / "UNKNOWN.CUE", root / "different.cue");
    assert(coastline::ReadDiscTracks(bin));
    std::filesystem::remove(root / "different.cue");
}

void CheckCloneCd(const std::filesystem::path &root) {
    auto bytes = DataTrack();
    bytes.insert(bytes.end(), 450 * SECTOR, 0x42);
    Write(root / "CLONE.IMG", bytes);
    Text(root / "clone.ccd",
         "[CloneCD]\nVersion=3\n[TRACK 1]\nMODE=1\nINDEX 1=0\n"
         "[TRACK 2]\nMODE=0\nINDEX 0=32\nINDEX 1=182\n");
    for (const auto *name : {"clone.ccd", "CLONE.IMG", "clone.sub"}) {
        auto tracks = coastline::ReadDiscTracks(root / name);
        assert(tracks && tracks->size() == 2);
        assert(tracks->front().sectorCount == 32 && !tracks->front().audio);
        assert((*tracks)[1].firstSector == 182 && (*tracks)[1].sectorCount == 300);
    }
    Text(root / "clone.ccd",
         "[TRACK 1]\nMODE=1\nINDEX 1=0\n"
         "[TRACK 2]\nMODE=0\nINDEX 1=999999999\n");
    assert(!coastline::ReadDiscTracks(root / "clone.ccd"));
}

void CheckNumbered(const std::filesystem::path &root) {
    Write(root / "Track 01.bin", DataTrack());
    for (int i = 2; i <= 13; ++i) {
        auto name = root / ("Track " + (i < 10 ? std::string("0") : std::string{}) +
                            std::to_string(i) + ".bin");
        std::vector<std::uint8_t> audio(450 * SECTOR, 0x42);
        std::fill_n(audio.begin(), 150 * SECTOR, 0);
        Write(name, audio);
    }
    auto tracks = coastline::ReadDiscTracks(root / "Track 01.bin");
    assert(tracks && tracks->size() == 13);
    assert((*tracks)[1].firstSector == 150 && (*tracks)[1].sectorCount == 300);
    std::filesystem::resize_file(root / "Track 13.bin", 449 * SECTOR);
    tracks = coastline::ReadDiscTracks(root / "Track 01.bin");
    assert(tracks && tracks->size() == 1);
    std::filesystem::remove(root / "Track 12.bin");
    tracks = coastline::ReadDiscTracks(root / "Track 01.bin");
    assert(tracks && tracks->size() == 1);
}

} // namespace

int main(int argc, char **argv) {
    CheckHash();
    const auto root = std::filesystem::temp_directory_path() / "coastline-disc-layout-test";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    CheckDiscovery(root);
    CheckCloneCd(root);
    CheckNumbered(root);
    if (argc == 2) {
        const auto cue = coastline::ParseCueSheet(argv[1]);
        assert(cue && !cue->empty());
        const auto bin = root / "verified.bin";
        std::filesystem::create_symlink(cue->front().file, bin);
        const auto tracks = coastline::ReadDiscTracks(bin);
        assert(tracks && tracks->size() == cue->size());
        for (std::size_t i = 0; i < tracks->size(); ++i) {
            assert((*tracks)[i].number == (*cue)[i].number);
            assert((*tracks)[i].audio == (*cue)[i].audio);
            assert((*tracks)[i].firstSector == (*cue)[i].firstSector);
            assert((*tracks)[i].sectorCount == (*cue)[i].sectorCount);
        }
    }
    std::filesystem::remove_all(root);
}
