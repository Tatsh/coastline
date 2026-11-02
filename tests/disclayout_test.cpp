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

std::size_t Append(std::vector<std::uint8_t> &bytes, std::size_t count) {
    const auto offset = bytes.size();
    bytes.resize(offset + count);
    return offset;
}

void CdiHeader(std::vector<std::uint8_t> &bytes) {
    constexpr std::array<std::uint8_t, 12> SIGNATURE{
        0xff, 0xff, 0, 0, 1, 0, 0, 0, 0xff, 0xff, 0xff, 0xff};
    const auto offset = Append(bytes, 53);
    std::copy(SIGNATURE.begin(), SIGNATURE.end(), bytes.begin() + offset);
    bytes[offset + 15] = 2;
    bytes[offset + 16] = 5;
    Tag(bytes, offset + 17, "x.cdi");
    bytes[offset + 28 + 5] = 2;
    bytes[offset + 39 + 5] = 0x80;
    bytes[offset + 46 + 5] = 0x98;
}

void CheckBlindWrite(const std::filesystem::path &root) {
    for (const auto *extension : {".b5t", ".b6t"}) {
        for (const std::size_t stride : {2048, 2352, 2368, 2448}) {
            for (const bool separateSession : {false, true}) {
                const auto source = StoredData(stride);
                const auto audioStride = stride == 2048 ? SECTOR : stride;
                std::vector<std::uint8_t> part1(37 + 2 * stride, 0xcc);
                part1.insert(part1.end(), source.begin(), source.begin() + 16 * stride);
                std::vector<std::uint8_t> part2(19, 0xdd);
                part2.insert(part2.end(), source.begin() + 16 * stride, source.end());
                const auto audioOffset = part2.size();
                const std::vector<std::uint8_t> audio(3 * audioStride, 0x42);
                part2.resize(part2.size() + audioStride, 0xee);
                part2.insert(part2.end(), audio.begin(), audio.end());
                const auto firstFile = std::filesystem::path(u8"track-\u65e5\u672c.b00");
                const auto secondFile = std::filesystem::path(u8"track-\u00e9.b01");
                Write(root / firstFile, part1);
                Write(root / secondFile, part2);
                std::vector<std::uint8_t> data(8);
                Number(data, 0, 3, 4, true);
                auto block = [&](const std::filesystem::path &filename,
                                 std::uint32_t offset,
                                 int first,
                                 unsigned count,
                                 std::size_t sectorSize,
                                 bool isAudio) {
                    const auto text = filename.u16string();
                    const auto entry = Append(data, 52 + text.size() * 2 + 4);
                    Number(data, entry, isAudio ? 0x8000 : 0, 4, true);
                    Number(data, entry + 4, count * sectorSize, 4, true);
                    Number(data, entry + 24, offset, 4, true);
                    Number(data, entry + 40, static_cast<std::uint32_t>(first), 4, true);
                    Number(data, entry + 44, count, 4, true);
                    Number(data, entry + 48, text.size() * 2, 4, true);
                    for (std::size_t index = 0; index < text.size(); ++index) {
                        Number(data, entry + 52 + index * 2, text[index], 2, true);
                    }
                    return entry;
                };
                const int audioStart = separateSession ? 100 : 32;
                block(secondFile, 19, 16, 16, stride, false);
                block(secondFile,
                      static_cast<std::uint32_t>(audioOffset),
                      audioStart,
                      4,
                      audioStride,
                      true);
                const auto firstBlock = block(firstFile, 37, -2, 18, stride, false);
                std::vector<std::uint8_t> layout;
                auto session = [&](unsigned number, int first, int end, unsigned track) {
                    const auto entry = Append(layout, 16);
                    Number(layout, entry, number, 2, true);
                    layout[entry + 2] = separateSession ? 2 : 3;
                    Number(layout, entry + 4, static_cast<std::uint32_t>(first), 4, true);
                    Number(layout, entry + 8, end, 4, true);
                    Number(layout, entry + 12, track, 2, true);
                    Number(layout, entry + 14, separateSession ? track : 2, 2, true);
                    const auto lead = Append(layout, 64);
                    layout[lead + 12] = 0xa0;
                };
                auto track = [&](unsigned number, unsigned mode, int first, unsigned count) {
                    const auto entry = Append(layout, 72);
                    layout[entry] = static_cast<std::uint8_t>(mode);
                    layout[entry + 12] = static_cast<std::uint8_t>(number);
                    Number(layout, entry + 22, number == 1 ? 2 : 1, 4, true);
                    Number(layout, entry + 42, static_cast<std::uint32_t>(first), 4, true);
                    Number(layout, entry + 46, count, 4, true);
                    Number(layout, entry + 58, separateSession ? number : 1, 4, true);
                    return entry;
                };
                session(1, -2, separateSession ? 32 : 36, 1);
                const auto firstTrack = track(1, 2, -2, 34);
                if (separateSession) {
                    session(2, 100, 104, 2);
                }
                track(2, 1, audioStart, 4);
                std::vector<std::uint8_t> metadata(260);
                Tag(metadata, 0, "BWT5 STREAM SIGN");
                Number(metadata, 48, 8, 2, true);
                Number(metadata, 50, separateSession ? 2 : 1, 2, true);
                Number(metadata, 240, 3, 4, true);
                Number(metadata, 244, 4, 4, true);
                Number(metadata, 16 + 80, 5, 2, true);
                Number(metadata, 16 + 82, 6, 2, true);
                Number(metadata, 16 + 84, 7, 2, true);
                Number(metadata, 16 + 86, 8, 2, true);
                Number(metadata, 248, data.size(), 4, true);
                Number(metadata, 252, layout.size(), 4, true);
                Number(metadata, 256, 9, 4, true);
                Append(metadata, 3 + 4 + 5 + 6 + 7 + 8);
                const auto dataOffset = metadata.size();
                metadata.insert(metadata.end(), data.begin(), data.end());
                const auto layoutOffset = metadata.size();
                metadata.insert(metadata.end(), layout.begin(), layout.end());
                Append(metadata, 9);
                const auto footer = Append(metadata, 20);
                Number(metadata, footer, metadata.size(), 4, true);
                Tag(metadata, footer + 4, "BWT5 STREAM FOOT");
                const auto descriptor = root / (std::string("blind") + extension);
                Write(descriptor, metadata);
                const auto tracks = coastline::ReadDiscTracks(descriptor);
                assert(tracks && tracks->size() == 2);
                assert(tracks->front().sectorCount == 32 && tracks->front().extents.size() == 2);
                assert(tracks->front().dataOffset == (stride == 2048 ? 0 : 16));
                assert((*tracks)[1].audio && (*tracks)[1].sectorCount == 3);
                CheckSectors(tracks->front(), source);
                CheckSectors((*tracks)[1], audio);
                auto companion = root / "blind.b5i";
                if (std::string_view(extension) == ".b6t") {
                    companion.replace_extension(".b6i");
                }
                std::filesystem::copy_file(
                    root / firstFile, companion, std::filesystem::copy_options::overwrite_existing);
                assert(coastline::ReadDiscTracks(companion));
                auto invalid = [&](std::size_t offset, std::uint64_t value, std::size_t width) {
                    auto broken = metadata;
                    Number(broken, offset, value, width, true);
                    Write(descriptor, broken);
                    assert(!coastline::ReadDiscTracks(descriptor));
                };
                invalid(0, 0, 1);
                invalid(50, 0, 2);
                invalid(240, UINT32_MAX, 4);
                invalid(248, UINT32_MAX, 4);
                invalid(dataOffset, UINT32_MAX, 4);
                invalid(dataOffset + firstBlock + 44, UINT32_MAX, 4);
                invalid(dataOffset + firstBlock + 40, 1, 4);
                invalid(dataOffset + firstBlock + 40, static_cast<std::uint32_t>(-3), 4);
                invalid(dataOffset + firstBlock + 4, 18 * stride - 1, 4);
                invalid(dataOffset + firstBlock, 0x8000, 4);
                invalid(dataOffset + firstBlock + 24, UINT32_MAX, 4);
                invalid(dataOffset + firstBlock + 48, 4095, 4);
                invalid(dataOffset + firstBlock + 52, 0xd800, 2);
                invalid(layoutOffset + firstTrack + 22, 34, 4);
                invalid(layoutOffset + firstTrack + 46, UINT32_MAX, 4);
                invalid(layoutOffset + firstTrack + 58, 9, 4);
                invalid(layoutOffset + firstTrack, 7, 1);
                invalid(footer + 4, 0, 1);
                for (std::size_t size = 0; size < metadata.size(); ++size) {
                    Write(descriptor, std::span(metadata).first(size));
                    assert(!coastline::ReadDiscTracks(descriptor));
                }
                Write(descriptor, metadata);
                std::filesystem::resize_file(root / firstFile, part1.size() - 1);
                assert(!coastline::ReadDiscTracks(descriptor));
                std::filesystem::remove(root / firstFile);
                assert(!coastline::ReadDiscTracks(descriptor));
            }
        }
    }
}

void CheckCdi(const std::filesystem::path &root) {
    for (const bool modern : {false, true}) {
        for (const std::uint32_t version : {0x80000004u, 0x80000005u, 0x80000006u}) {
            if (modern && version != 0x80000006) {
                continue;
            }
            for (const std::size_t stride : {2048, 2352, 2368, 2448}) {
                for (const bool extended : {false, true}) {
                    const auto data = StoredData(stride);
                    const auto audioStride = stride == 2048 ? SECTOR : stride;
                    const std::vector<std::uint8_t> audio(audioStride * 3, 0x42);
                    std::vector<std::uint8_t> image(stride * 2);
                    image.insert(image.end(), data.begin(), data.end());
                    image.resize(image.size() + audioStride);
                    image.insert(image.end(), audio.begin(), audio.end());
                    const auto metadata = image.size();
                    const unsigned sessions = extended ? 2 : 1;
                    Number(image, Append(image, modern ? 1 : 2), sessions, modern ? 1 : 2, true);
                    std::array<std::size_t, 2> indices{}, text{}, fixed{};
                    for (unsigned session = 0; session < sessions; ++session) {
                        const unsigned count = sessions == 2 ? 1 : 2;
                        const auto section = Append(image, modern ? 15 : 2);
                        Number(image, section + (modern ? 1 : 0), count, modern ? 1 : 2, true);
                        for (unsigned index = 0; index < count; ++index) {
                            const unsigned track = sessions == 2 ? session : index;
                            if (modern) {
                                CdiHeader(image);
                            } else {
                                const auto prefix = Append(image, extended ? 12 : 4);
                                Number(image, prefix, extended, 4, true);
                                constexpr std::array<std::uint8_t, 10> MARK{
                                    0, 0, 1, 0, 0, 0, 0xff, 0xff, 0xff, 0xff};
                                for (unsigned marker = 0; marker < 2; ++marker) {
                                    const auto position = Append(image, MARK.size());
                                    std::copy(MARK.begin(), MARK.end(), image.begin() + position);
                                }
                                Append(image, 4);
                                image.push_back(5);
                                Tag(image, Append(image, 5), "x.cdi");
                                Append(image, 19);
                                Number(image, Append(image, 4), extended ? 0x80000000 : 0, 4, true);
                                if (extended) {
                                    Append(image, 8);
                                }
                            }
                            indices[track] = Append(image, 10);
                            Number(image, indices[track], 2, 2, true);
                            Number(image, indices[track] + 2, track == 0 ? 2 : 1, 4, true);
                            Number(image, indices[track] + 6, track == 0 ? 32 : 3, 4, true);
                            text[track] = Append(image, 4);
                            Number(image, text[track], modern ? 1 : 0, 4, true);
                            if (modern) {
                                image.push_back(3);
                                Tag(image, Append(image, 3), "abc");
                                Append(image, 17);
                            }
                            fixed[track] = Append(image, modern ? 174 : 75);
                            Number(image, fixed[track] + 2, track == 0 ? 1 : 0, 4, true);
                            Number(image, fixed[track] + 10, session, 4, true);
                            Number(image, fixed[track] + 14, extended ? index : track, 4, true);
                            Number(image, fixed[track] + 22, track == 0 ? 34 : 4, 4, true);
                            Number(image, fixed[track] + 51, track == 0 ? 34 : 4, 4, true);
                            const auto size = track == 0 ? stride : audioStride;
                            Number(image,
                                   fixed[track] + 42,
                                   size == 2048 ? 0 :
                                   size == 2352 ? 2 :
                                   size == 2368 ? 3 :
                                                  4,
                                   4,
                                   true);
                            if (!modern && version != 0x80000004) {
                                Append(image, 5);
                                Number(image, Append(image, 4), extended ? UINT32_MAX : 0, 4, true);
                                if (extended) {
                                    Append(image, 78);
                                }
                            }
                        }
                        if (!modern) {
                            Append(image, version == 0x80000004 ? 12 : 13);
                        }
                    }
                    if (modern) {
                        Append(image, 15);
                        CdiHeader(image);
                        const auto disc = Append(image, 47);
                        Number(image, disc + 43, version, 4, true);
                    } else {
                        Number(image, Append(image, 4), version, 4, true);
                    }
                    const auto pointer = Append(image, 4);
                    Number(image,
                           pointer,
                           version == 0x80000006 ? image.size() - metadata : metadata,
                           4,
                           true);
                    const auto path = root / "test.cdi";
                    Write(path, image);
                    const auto tracks = coastline::ReadDiscTracks(path);
                    assert(tracks && tracks->size() == 2);
                    assert(!tracks->front().audio && tracks->back().audio);
                    CheckSectors(tracks->front(), data);
                    CheckSectors(tracks->back(), audio);
                    for (const auto position :
                         {indices[0], text[0], fixed[0] + 22, fixed[0] + 42, pointer}) {
                        auto broken = image;
                        Number(broken, position, UINT32_MAX, 4, true);
                        Write(path, broken);
                        assert(!coastline::ReadDiscTracks(path));
                    }
                    const auto end = modern ? image.size() - 8 : fixed[1] + 75;
                    for (std::size_t cut = metadata; cut < end; cut += 7) {
                        auto truncated = std::vector(image.begin(), image.begin() + cut);
                        const auto footer = Append(truncated, 8);
                        Number(truncated, footer, version, 4, true);
                        Number(truncated,
                               footer + 4,
                               version == 0x80000006 ? truncated.size() - metadata : metadata,
                               4,
                               true);
                        Write(path, truncated);
                        assert(!coastline::ReadDiscTracks(path));
                    }
                }
            }
        }
    }
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
    CheckCdi(root);
    CheckBlindWrite(root);
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
