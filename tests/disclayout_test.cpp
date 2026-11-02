#include <algorithm>
#include <array>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include "disclayout.h"
#include "disctrackreader.h"
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

void Number(std::vector<std::uint8_t> &bytes,
            std::size_t offset,
            std::uint64_t value,
            std::size_t length,
            bool little = false) {
    for (std::size_t index = 0; index < length; ++index) {
        bytes.at(offset + (little ? index : length - index - 1)) =
            static_cast<std::uint8_t>(value >> (index * 8));
    }
}

void Tag(std::vector<std::uint8_t> &bytes, std::size_t offset, std::string_view tag) {
    assert(offset + tag.size() <= bytes.size());
    std::copy(tag.begin(), tag.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset));
}

std::vector<std::uint8_t> StoredData(std::size_t stride) {
    const auto source = DataTrack();
    std::vector<std::uint8_t> data(32 * stride, 0xa5);
    for (std::size_t index = 0; index < 32; ++index) {
        std::copy_n(source.data() + index * SECTOR + (stride == 2048 ? 16 : 0),
                    stride == 2048 ? 2048 : SECTOR,
                    data.data() + index * stride);
    }
    return data;
}

void CheckSectors(const coastline::CueTrack &track, std::span<const std::uint8_t> wanted) {
    coastline::DiscTrackReader reader(track);
    std::vector<std::uint8_t> bytes(wanted.size());
    assert(reader.Read(0, static_cast<std::uint32_t>(track.sectorCount), bytes));
    assert(std::ranges::equal(bytes, wanted));
    std::vector<std::uint8_t> last(track.sectorSize);
    assert(reader.Read(track.sectorCount - 1, 1, last));
    assert(std::ranges::equal(last, wanted.last(track.sectorSize)));
    assert(!reader.Read(track.sectorCount, 1, last));
    assert(!reader.Read(UINT64_MAX, 1, last));
    assert(!reader.Read(0, UINT32_MAX, last));
    assert(!reader.Read(0, 0, last));
    assert(reader.Read(track.sectorCount, 0, {}));
}

void CheckNrg(const std::filesystem::path &root) {
    for (const bool wide : {false, true}) {
        for (const bool dao : {false, true}) {
            for (const std::size_t stride : {2048, 2352, 2448}) {
                const auto data = StoredData(stride);
                const std::size_t audioStride = stride == 2448 ? 2448 : SECTOR;
                const std::vector<std::uint8_t> audio(audioStride * 3, 0x42);
                std::vector<std::uint8_t> image(17, 0x99);
                image.insert(image.end(), data.begin(), data.end());
                const auto gap = image.size();
                image.insert(image.end(), audioStride * 2, 0);
                const auto audioStart = image.size();
                image.insert(image.end(), audio.begin(), audio.end());
                const auto offset = image.size();
                const std::size_t word = wide ? 8 : 4;
                const std::size_t record = dao ? 18 + word * 3 : wide ? 32 : 20;
                const std::size_t header = dao ? 22 : 0;
                const std::size_t length = header + record * 2;
                const std::size_t trailer = offset + 8 + length + 12 + 8;
                image.resize(trailer + (wide ? 12 : 8));
                Tag(image, offset, dao ? (wide ? "DAOX" : "DAOI") : (wide ? "ETN2" : "ETNF"));
                Number(image, offset + 4, length, 4);
                const auto body = offset + 8;
                if (dao) {
                    Number(image, body, length, 4, true);
                    image[body + 20] = 1;
                    image[body + 21] = 2;
                }
                for (std::size_t index = 0; index < 2; ++index) {
                    const auto entry = body + header + index * record;
                    const auto start = index == 0 ? 17 : audioStart;
                    const auto size = index == 0 ? data.size() : audio.size();
                    const auto mode = index == 0 ? (stride == 2048 ? 0 :
                                                    stride == 2352 ? 5 :
                                                                     15) :
                                                   (audioStride == 2448 ? 16 : 7);
                    if (dao) {
                        Number(image, entry + 12, index == 0 ? stride : audioStride, 2);
                        image[entry + 14] = static_cast<std::uint8_t>(mode);
                        Number(image, entry + 18, index == 0 ? 0 : gap, word);
                        Number(image, entry + 18 + word, start, word);
                        Number(image, entry + 18 + word * 2, start + size, word);
                    } else {
                        Number(image, entry, start, word);
                        Number(image, entry + word, size, word);
                        Number(image, entry + word * 2, mode, 4);
                    }
                }
                Tag(image, body + length, "SINF");
                Number(image, body + length + 4, 4, 4);
                Number(image, body + length + 8, 2, 4);
                Tag(image, body + length + 12, "END!");
                Tag(image, trailer, wide ? "NER5" : "NERO");
                Number(image, trailer + 4, offset, word);
                const auto path = root / "test.nrg";
                Write(path, image);
                const auto tracks = coastline::ReadDiscTracks(path);
                assert(tracks && tracks->size() == 2);
                assert(!tracks->front().audio && tracks->back().audio);
                assert(tracks->front().sectorCount == 32 && tracks->back().sectorCount == 3);
                CheckSectors(tracks->front(), data);
                CheckSectors(tracks->back(), audio);
                auto overlapping = image;
                const auto second = body + header + record;
                Number(overlapping, second + (dao ? 18 + word : 0), 17, word);
                if (dao) {
                    Number(overlapping, second + 18, 17, word);
                    Number(overlapping, second + 18 + word * 2, 17 + audio.size(), word);
                }
                Write(path, overlapping);
                assert(!coastline::ReadDiscTracks(path));
                if (wide && dao && stride == 2352) {
                    constexpr std::uint64_t LARGE = std::uint64_t{1} << 32;
                    auto shifted = image;
                    Number(shifted, trailer + 4, offset + LARGE, 8);
                    for (std::size_t index = 0; index < 2; ++index) {
                        const auto entry = body + header + index * record;
                        const std::array<std::uint64_t, 3> positions{
                            index == 0 ? 0 : gap,
                            index == 0 ? 17 : audioStart,
                            index == 0 ? 17 + data.size() : audioStart + audio.size()};
                        for (std::size_t field = 0; field < 3; ++field) {
                            Number(shifted, entry + 18 + field * 8, positions[field] + LARGE, 8);
                        }
                    }
                    const auto large = root / "large.nrg";
                    {
                        std::ofstream sparse(large, std::ios::binary);
                        sparse.seekp(static_cast<std::streamoff>(LARGE));
                        sparse.write(reinterpret_cast<const char *>(shifted.data()),
                                     static_cast<std::streamsize>(shifted.size()));
                        assert(sparse.good());
                    }
                    const auto largeTracks = coastline::ReadDiscTracks(large);
                    assert(largeTracks &&
                           largeTracks->front().extents.front().byteOffset > UINT32_MAX);
                    CheckSectors(largeTracks->front(), data);
                    CheckSectors(largeTracks->back(), audio);
                    std::filesystem::remove(large);
                }
                for (const auto mutation : {offset + 4, trailer + 4}) {
                    auto bad = image;
                    std::fill_n(bad.begin() + static_cast<std::ptrdiff_t>(mutation), 4, 0xff);
                    Write(path, bad);
                    assert(!coastline::ReadDiscTracks(path));
                }
                for (std::size_t cut = offset; cut < image.size(); ++cut) {
                    Write(path, std::span(image).first(cut));
                    assert(!coastline::ReadDiscTracks(path));
                }
            }
        }
    }
}

void CheckMds(const std::filesystem::path &root) {
    for (const bool split : {false, true}) {
        for (const bool wide : {false, true}) {
            for (const std::size_t stride : {2352, 2448}) {
                const auto data = StoredData(stride);
                const std::vector<std::uint8_t> audio(stride * 3, 0x42);
                std::vector<std::uint8_t> image(17, 0x99);
                image.insert(image.end(), data.begin(), data.end());
                image.insert(image.end(), audio.begin(), audio.end());
                const auto boundary = split ? 17 + stride * 19 : image.size();
                const auto firstFile =
                    wide ? root / std::filesystem::path(u8"disc-日本.mdf") : root / "TEST.MDF";
                Write(firstFile, std::span(image).first(boundary));
                if (split) {
                    Write(root / "TEST.MD0", std::span(image).subspan(boundary));
                }
                std::vector<std::uint8_t> metadata(1024);
                Tag(metadata, 0, "MEDIA DESCRIPTOR");
                metadata[16] = 1;
                Number(metadata, 20, 1, 2, true);
                Number(metadata, 80, 88, 4, true);
                Number(metadata, 88 + 8, 1, 2, true);
                metadata[88 + 10] = 5;
                metadata[88 + 11] = 3;
                Number(metadata, 88 + 12, 1, 2, true);
                Number(metadata, 88 + 14, 2, 2, true);
                Number(metadata, 88 + 20, 112, 4, true);
                for (std::size_t index = 0; index < 3; ++index) {
                    metadata[112 + index * 80 + 4] = static_cast<std::uint8_t>(0xa0 + index);
                }
                for (std::size_t index = 0; index < 2; ++index) {
                    const auto entry = 112 + (index + 3) * 80;
                    metadata[entry] = index == 0 ? 0xaa : 0xa9;
                    metadata[entry + 1] = stride == 2448 ? 8 : 0;
                    metadata[entry + 4] = static_cast<std::uint8_t>(index + 1);
                    Number(metadata, entry + 12, 0x260 + index * 8, 4, true);
                    Number(metadata, entry + 16, stride, 2, true);
                    Number(metadata, entry + 40, index == 0 ? 17 : 17 + data.size(), 8, true);
                    Number(metadata, entry + 48, split ? 2 : 1, 4, true);
                    Number(metadata, entry + 52, 0x280 + index * 32, 4, true);
                    Number(metadata, 0x260 + index * 8, 150, 4, true);
                    Number(metadata, 0x264 + index * 8, index == 0 ? 32 : 3, 4, true);
                    for (std::size_t part = 0; part < (split ? 2u : 1u); ++part) {
                        Number(
                            metadata, 0x280 + index * 32 + part * 16, 0x300 + part * 32, 4, true);
                        Number(metadata, 0x284 + index * 32 + part * 16, wide, 4, true);
                    }
                }
                for (std::size_t part = 0; part < 2; ++part) {
                    const std::u16string name =
                        part == 0 ? (wide ? u"disc-日本.mdf" : u"*.mdf") : u"*.md0";
                    for (std::size_t index = 0; index < name.size(); ++index) {
                        Number(metadata,
                               0x300 + part * 32 + index * (wide ? 2 : 1),
                               name[index],
                               wide ? 2 : 1,
                               true);
                    }
                }
                const auto path = root / "test.mds";
                Write(path, metadata);
                for (const auto &input : {path, root / "TEST.MDF"}) {
                    const auto tracks = coastline::ReadDiscTracks(input);
                    assert(tracks && tracks->size() == 2);
                    assert(tracks->front().extents.size() == (split ? 2 : 1));
                    assert(!tracks->front().audio && tracks->back().audio);
                    CheckSectors(tracks->front(), data);
                    CheckSectors(tracks->back(), audio);
                }
                for (const auto pointer : {80, 108, 352 + 12, 352 + 52, 0x280}) {
                    auto bad = metadata;
                    Number(bad, pointer, UINT32_MAX, 4, true);
                    Write(path, bad);
                    assert(!coastline::ReadDiscTracks(path));
                }
                auto overlapping = metadata;
                Number(overlapping, 432 + 40, 17, 8, true);
                Write(path, overlapping);
                assert(!coastline::ReadDiscTracks(path));
                if (wide) {
                    auto invalidUnicode = metadata;
                    Number(invalidUnicode, 0x300, 0xd800, 2, true);
                    Write(path, invalidUnicode);
                    assert(!coastline::ReadDiscTracks(path));
                }
                Write(path, metadata);
                std::filesystem::resize_file(firstFile, 10);
                assert(!coastline::ReadDiscTracks(path));
            }
        }
    }
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
    CheckNrg(root);
    CheckMds(root);
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
