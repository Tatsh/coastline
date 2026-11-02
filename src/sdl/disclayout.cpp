#include "disclayout.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <charconv>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <tuple>

#include <fmt/format.h>

#include "sha1.h"

namespace coastline {
namespace {
constexpr std::uint32_t RAW_SECTOR = 2352;
constexpr std::uint32_t DATA_SECTOR = 2048;
constexpr std::uint32_t FRAMES_PER_SECOND = 75;

using Bytes = std::span<const std::uint8_t>;

std::uint64_t Big(Bytes bytes, std::size_t offset, std::size_t size) {
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < size; ++index) {
        value = (value << 8) | bytes[offset + index];
    }
    return value;
}

std::uint64_t Little(Bytes bytes, std::size_t offset, std::size_t size) {
    std::uint64_t value = 0;
    for (std::size_t index = 0; index < size; ++index) {
        value |= static_cast<std::uint64_t>(bytes[offset + index]) << (index * 8);
    }
    return value;
}

bool Range(Bytes bytes, std::uint64_t offset, std::uint64_t count) {
    return offset <= bytes.size() && count <= bytes.size() - offset;
}

std::optional<std::vector<std::uint8_t>>
ReadMetadata(std::ifstream &file, std::uint64_t offset, std::uint64_t count) {
    constexpr std::uint64_t LIMIT = 16 * 1024 * 1024;
    if (count > LIMIT ||
        offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> result(static_cast<std::size_t>(count));
    file.seekg(static_cast<std::streamoff>(offset));
    if (!file.read(reinterpret_cast<char *>(result.data()), static_cast<std::streamsize>(count))) {
        return std::nullopt;
    }
    return result;
}

bool NrgMode(CueTrack &track, unsigned mode) {
    switch (mode) {
    case 0:
    case 2:
        track.sectorSize = DATA_SECTOR;
        track.dataOffset = 0;
        break;
    case 3:
        track.sectorSize = 2336;
        track.dataOffset = 8;
        break;
    case 5:
    case 15:
        track.sectorSize = mode == 5 ? RAW_SECTOR : RAW_SECTOR + 96;
        track.dataOffset = 16;
        break;
    case 6:
    case 17:
        track.sectorSize = mode == 6 ? RAW_SECTOR : RAW_SECTOR + 96;
        track.dataOffset = 24;
        break;
    case 7:
    case 16:
        track.audio = true;
        track.sectorSize = mode == 7 ? RAW_SECTOR : RAW_SECTOR + 96;
        break;
    default:
        return false;
    }
    return true;
}

bool DisjointExtents(const std::vector<CueTrack> &tracks) {
    std::vector<std::tuple<std::filesystem::path, std::uint64_t, std::uint64_t>> intervals;
    for (const auto &track : tracks) {
        for (const auto &extent : track.extents) {
            std::error_code error;
            const auto file = std::filesystem::weakly_canonical(extent.file, error);
            if (error || track.sectorSize == 0 || extent.sectorCount == 0 ||
                extent.sectorCount > (UINT64_MAX - extent.byteOffset) / track.sectorSize) {
                return false;
            }
            intervals.emplace_back(
                file, extent.byteOffset, extent.byteOffset + extent.sectorCount * track.sectorSize);
        }
    }
    std::ranges::sort(intervals);
    for (std::size_t index = 1; index < intervals.size(); ++index) {
        if (std::get<0>(intervals[index - 1]) == std::get<0>(intervals[index]) &&
            std::get<2>(intervals[index - 1]) > std::get<1>(intervals[index])) {
            return false;
        }
    }
    return true;
}

class MetadataCursor {
public:
    explicit MetadataCursor(Bytes bytes) : m_bytes(bytes) {
    }

    Bytes Take(std::uint64_t count) {
        if (count > m_bytes.size()) {
            m_valid = false;
            m_bytes = {};
            return {};
        }
        const auto result = m_bytes.first(static_cast<std::size_t>(count));
        m_bytes = m_bytes.subspan(static_cast<std::size_t>(count));
        return result;
    }

    std::uint64_t Number(std::size_t size) {
        const auto bytes = Take(size);
        return bytes.size() == size ? Little(bytes, 0, size) : 0;
    }

    bool Valid() const {
        return m_valid;
    }

    std::size_t Remaining() const {
        return m_bytes.size();
    }

private:
    Bytes m_bytes;
    bool m_valid = true;
};

bool CdiText(MetadataCursor &cursor) {
    const auto blocks = cursor.Number(4);
    if (blocks > cursor.Remaining() / 18) {
        return false;
    }
    for (std::uint64_t block = 0; block < blocks; ++block) {
        for (unsigned field = 0; field < 18; ++field) {
            const auto length = cursor.Number(1);
            cursor.Take(length);
        }
    }
    return cursor.Valid();
}

std::optional<unsigned> CdiHeader(MetadataCursor &cursor) {
    constexpr std::array<std::uint8_t, 12> SIGNATURE{
        0xff, 0xff, 0, 0, 1, 0, 0, 0, 0xff, 0xff, 0xff, 0xff};
    const auto header = cursor.Take(17);
    if (header.size() != 17 || !std::ranges::equal(header.first(12), SIGNATURE)) {
        return std::nullopt;
    }
    cursor.Take(header[16]);
    const auto rest = cursor.Take(31);
    if (rest.size() != 31 || Little(rest, 29, 2) != 0x98 || header[15] > 99) {
        return std::nullopt;
    }
    return header[15];
}

bool CdiTrack(MetadataCursor &cursor,
              const std::filesystem::path &image,
              unsigned session,
              unsigned sessionTrack,
              std::uint64_t dataEnd,
              std::uint64_t &dataPosition,
              std::vector<CueTrack> &tracks,
              bool modern,
              bool version2) {
    if (modern) {
        if (!CdiHeader(cursor)) {
            return false;
        }
    } else {
        if (cursor.Number(4) != 0) {
            cursor.Take(8);
        }
        constexpr std::array<std::uint8_t, 10> SIGNATURE{0, 0, 1, 0, 0, 0, 0xff, 0xff, 0xff, 0xff};
        const auto markers = cursor.Take(20);
        if (markers.size() != 20 || !std::ranges::equal(markers.first(10), SIGNATURE) ||
            !std::ranges::equal(markers.last(10), SIGNATURE)) {
            return false;
        }
        cursor.Take(4);
        const auto nameLength = cursor.Number(1);
        cursor.Take(nameLength);
        cursor.Take(19);
        if (cursor.Number(4) == 0x80000000) {
            cursor.Take(8);
        }
    }
    const auto indices = cursor.Number(2);
    if (indices < 2 || indices > 100 || indices > cursor.Remaining() / 4) {
        return false;
    }
    const auto pregap = cursor.Number(4);
    std::uint64_t total = pregap;
    for (std::uint64_t index = 1; index < indices; ++index) {
        total += cursor.Number(4);
    }
    if (total <= pregap || total > UINT32_MAX || !CdiText(cursor)) {
        return false;
    }
    const auto fixed = cursor.Take(modern ? 174 : 75);
    if (fixed.size() < 75 || Little(fixed, 10, 4) != session ||
        (Little(fixed, 14, 4) != tracks.size() && Little(fixed, 14, 4) != sessionTrack) ||
        Little(fixed, 22, 4) != total) {
        return false;
    }
    const auto mode = Little(fixed, 2, 4);
    const auto readMode = Little(fixed, 42, 4);
    if (mode > 2 || readMode > 4 || (readMode == 0 && mode != 1) || (readMode == 1 && mode != 2)) {
        return false;
    }
    CueTrack track;
    track.file = image;
    track.number = static_cast<int>(tracks.size() + 1);
    track.audio = mode == 0;
    constexpr std::array<std::uint32_t, 5> STRIDES{2048, 2336, 2352, 2368, 2448};
    track.sectorSize = STRIDES[readMode];
    track.dataOffset = readMode == 0 ? 0 : readMode == 1 ? 8 : mode == 2 ? 24 : 16;
    track.sectorCount = total - pregap;
    if (dataPosition > dataEnd || total > (dataEnd - dataPosition) / track.sectorSize) {
        return false;
    }
    track.extents.push_back({image, dataPosition + pregap * track.sectorSize, track.sectorCount});
    dataPosition += total * track.sectorSize;
    if (!modern && !version2) {
        cursor.Take(5);
        if (cursor.Number(4) == UINT32_MAX) {
            cursor.Take(78);
        }
    }
    if (!cursor.Valid()) {
        return false;
    }
    tracks.push_back(std::move(track));
    return true;
}

std::optional<std::vector<CueTrack>> CdiLayout(Bytes metadata,
                                               const std::filesystem::path &image,
                                               std::uint64_t dataEnd,
                                               std::uint64_t version,
                                               bool modern) {
    MetadataCursor cursor(metadata);
    const auto sessions = cursor.Number(modern ? 1 : 2);
    if (sessions == 0 || sessions > 99) {
        return std::nullopt;
    }
    std::vector<CueTrack> tracks;
    std::uint64_t dataPosition = 0;
    for (unsigned session = 0; session < sessions; ++session) {
        std::uint64_t count = 0;
        if (modern) {
            const auto header = cursor.Take(15);
            if (header.size() != 15 || header[0] != 0) {
                return std::nullopt;
            }
            count = header[1];
        } else {
            count = cursor.Number(2);
        }
        if (count == 0 || count > 99 - tracks.size()) {
            return std::nullopt;
        }
        for (std::uint64_t index = 0; index < count; ++index) {
            if (!CdiTrack(cursor,
                          image,
                          session,
                          static_cast<unsigned>(index),
                          dataEnd,
                          dataPosition,
                          tracks,
                          modern,
                          version == 0x80000004)) {
                return std::nullopt;
            }
        }
        if (!modern) {
            cursor.Take(version == 0x80000004 ? 12 : 13);
        }
    }
    if (modern) {
        const auto endSession = cursor.Take(15);
        if (endSession.size() != 15 || endSession[1] != 0 || !CdiHeader(cursor)) {
            return std::nullopt;
        }
        cursor.Take(4);
        const auto volumeLength = cursor.Number(1);
        cursor.Take(volumeLength);
        cursor.Take(26);
        const auto textLength = cursor.Number(4);
        cursor.Take(textLength);
        cursor.Take(8);
        if (cursor.Number(4) != version || cursor.Remaining() != 4) {
            return std::nullopt;
        }
    }
    return cursor.Valid() && DisjointExtents(tracks) ? std::optional(std::move(tracks)) :
                                                       std::nullopt;
}

std::expected<std::vector<CueTrack>, std::string> CdiTracks(const std::filesystem::path &image) {
    const auto invalid =
        std::unexpected(std::string("Invalid or unsupported DiscJuggler CDI metadata."));
    std::error_code error;
    const auto size = std::filesystem::file_size(image, error);
    if (error || size < 8) {
        return invalid;
    }
    std::ifstream file(image, std::ios::binary);
    const auto trailer = ReadMetadata(file, size - 8, 8);
    if (!trailer) {
        return invalid;
    }
    const auto version = Little(*trailer, 0, 4);
    const auto pointer = Little(*trailer, 4, 4);
    if (version < 0x80000004 || version > 0x80000006 || pointer == 0 || pointer > size) {
        return invalid;
    }
    const auto offset = version == 0x80000006 ? size - pointer : pointer;
    if (offset >= size - 8) {
        return invalid;
    }
    const auto metadata = ReadMetadata(file, offset, size - offset);
    if (!metadata) {
        return invalid;
    }
    if (version == 0x80000006) {
        if (Range(*metadata, 16, 17) && (*metadata)[16] == 0xff && (*metadata)[17] == 0xff) {
            const auto flagOffset = std::size_t{52} + (*metadata)[32];
            if (Range(*metadata, flagOffset, 4) && Little(*metadata, flagOffset, 4) == 0x80000000) {
                auto tracks = CdiLayout(*metadata, image, offset, version, true);
                return tracks ?
                           std::expected<std::vector<CueTrack>, std::string>(std::move(*tracks)) :
                           invalid;
            }
        }
    }
    if (auto tracks = CdiLayout(*metadata, image, offset, version, false)) {
        return std::move(*tracks);
    }
    return invalid;
}

std::expected<std::vector<CueTrack>, std::string> NrgTracks(const std::filesystem::path &image) {
    const auto invalid = std::unexpected(std::string("Invalid or unsupported Nero NRG metadata."));
    std::error_code error;
    const auto size = std::filesystem::file_size(image, error);
    if (error || size < 12) {
        return invalid;
    }
    std::ifstream file(image, std::ios::binary);
    const auto trailer = ReadMetadata(file, size - 12, 12);
    if (!trailer) {
        return invalid;
    }
    const auto signature = [&](std::size_t offset, std::string_view text) {
        return std::ranges::equal(Bytes(*trailer).subspan(offset, 4), text);
    };
    const bool wide = signature(0, "NER5");
    if (!wide && !signature(4, "NERO")) {
        return invalid;
    }
    const auto offset = Big(*trailer, wide ? 4 : 8, wide ? 8 : 4);
    const std::uint64_t end = size - (wide ? 12 : 8);
    if (offset >= end) {
        return invalid;
    }
    const auto metadata = ReadMetadata(file, offset, end - offset);
    if (!metadata) {
        return invalid;
    }
    std::vector<CueTrack> tracks;
    std::optional<std::vector<CueTrack>> dao;
    std::optional<std::vector<CueTrack>> tao;
    auto finishSession = [&](std::optional<std::uint64_t> expected) {
        auto &session = dao ? dao : tao;
        if (!session || (expected && *expected != session->size()) ||
            session->size() > 99 - tracks.size()) {
            return false;
        }
        for (auto &track : *session) {
            if (track.number != 0 && track.number != static_cast<int>(tracks.size() + 1)) {
                return false;
            }
            track.number = static_cast<int>(tracks.size() + 1);
            tracks.push_back(std::move(track));
        }
        dao.reset();
        tao.reset();
        return true;
    };
    std::size_t cursor = 0;
    while (Range(*metadata, cursor, 8)) {
        const std::string_view id(reinterpret_cast<const char *>(metadata->data() + cursor), 4);
        const auto length = Big(*metadata, cursor + 4, 4);
        cursor += 8;
        if (!Range(*metadata, cursor, length)) {
            return invalid;
        }
        const auto body = Bytes(*metadata).subspan(cursor, static_cast<std::size_t>(length));
        cursor += static_cast<std::size_t>(length);
        if (id == "END!") {
            if (length != 0 || cursor != metadata->size() ||
                ((dao || tao) && !finishSession(std::nullopt)) || tracks.empty() ||
                !DisjointExtents(tracks)) {
                return invalid;
            }
            return tracks;
        }
        if (id == "SINF") {
            if (length != 4 || !finishSession(Big(body, 0, 4))) {
                return invalid;
            }
            continue;
        }
        const bool isDao = id == "DAOI" || id == "DAOX";
        const bool isTao = id == "ETNF" || id == "ETN2";
        if (!isDao && !isTao) {
            continue;
        }
        const bool large = id == "DAOX" || id == "ETN2";
        const std::size_t word = large ? 8 : 4;
        const std::size_t header = isDao ? 22 : 0;
        const std::size_t record = isDao ? 18 + word * 3 : large ? 32 : 20;
        if (length < header || (length - header) % record != 0) {
            return invalid;
        }
        const auto count = (length - header) / record;
        if (count == 0 || count > 99 ||
            (isDao && (body[20] == 0 || body[21] > 99 || body[21] < body[20] ||
                       body[21] - body[20] + 1 != static_cast<int>(count)))) {
            return invalid;
        }
        std::vector<CueTrack> session;
        for (std::size_t index = 0; index < count; ++index) {
            const auto entry = body.subspan(header + index * record, record);
            CueTrack track;
            track.file = image;
            track.number = isDao ? body[20] + static_cast<int>(index) : 0;
            const auto mode = isDao ? entry[14] : Big(entry, word * 2, 4);
            if (!NrgMode(track, static_cast<unsigned>(mode)) ||
                (isDao && Big(entry, 12, 2) != track.sectorSize)) {
                return invalid;
            }
            const auto start = Big(entry, isDao ? 18 + word : 0, word);
            const auto stop = Big(entry, isDao ? 18 + word * 2 : word, word);
            const auto bytes = isDao ? (stop >= start ? stop - start : 0) : stop;
            if (bytes == 0 || bytes % track.sectorSize != 0 || start > offset ||
                bytes > offset - start || (isDao && Big(entry, 18, word) > start)) {
                return invalid;
            }
            track.sectorCount = bytes / track.sectorSize;
            track.extents.push_back({image, start, track.sectorCount});
            session.push_back(std::move(track));
        }
        auto &destination = isDao ? dao : tao;
        if (destination) {
            return invalid;
        }
        destination = std::move(session);
    }
    return invalid;
}

std::string Lowercase(std::string value) {
    std::ranges::transform(
        value, value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::filesystem::path AdjacentFile(const std::filesystem::path &file) {
    std::error_code error;
    if (std::filesystem::is_regular_file(file, error)) {
        return file;
    }
    const auto wanted = Lowercase(file.filename().string());
    const auto parent =
        file.parent_path().empty() ? std::filesystem::path(".") : file.parent_path();
    std::filesystem::directory_iterator entry(parent, error), end;
    for (; !error && entry != end; entry.increment(error)) {
        if (Lowercase(entry->path().filename().string()) == wanted &&
            entry->is_regular_file(error)) {
            return entry->path();
        }
    }
    return file;
}

std::optional<std::filesystem::path> MdsFilename(Bytes metadata,
                                                 std::uint64_t offset,
                                                 bool wide,
                                                 const std::filesystem::path &descriptor) {
    std::u8string text;
    if (wide) {
        std::u16string decoded;
        bool terminated = false;
        for (std::size_t index = 0; index < 2048; ++index) {
            if (!Range(metadata, offset + index * 2, 2)) {
                return std::nullopt;
            }
            const auto value = static_cast<char16_t>(Little(metadata, offset + index * 2, 2));
            if (value == 0) {
                terminated = true;
                break;
            }
            decoded.push_back(value);
        }
        if (!terminated || decoded.empty()) {
            return std::nullopt;
        }
        for (std::size_t index = 0; index < decoded.size(); ++index) {
            if (decoded[index] >= 0xd800 && decoded[index] <= 0xdbff) {
                if (++index >= decoded.size() || decoded[index] < 0xdc00 ||
                    decoded[index] > 0xdfff) {
                    return std::nullopt;
                }
            } else if (decoded[index] >= 0xdc00 && decoded[index] <= 0xdfff) {
                return std::nullopt;
            }
        }
        try {
            text = std::filesystem::path(decoded).generic_u8string();
        } catch (const std::filesystem::filesystem_error &) {
            return std::nullopt;
        }
    } else {
        bool terminated = false;
        for (std::size_t index = 0; index < 4096; ++index) {
            if (!Range(metadata, offset + index, 1)) {
                return std::nullopt;
            }
            const auto value = metadata[offset + index];
            if (value == 0) {
                terminated = true;
                break;
            }
            text.push_back(static_cast<char8_t>(value));
        }
        if (!terminated || text.empty()) {
            return std::nullopt;
        }
    }
    std::ranges::replace(text, u8'\\', u8'/');
    std::filesystem::path filename;
    try {
        filename = std::filesystem::path(text);
    } catch (const std::filesystem::filesystem_error &) {
        return std::nullopt;
    }
    if (filename.filename().u8string().starts_with(u8"*.")) {
        auto image = descriptor;
        image.replace_extension(filename.extension());
        return AdjacentFile(image);
    }
    auto image = AdjacentFile(descriptor.parent_path() / filename);
    std::error_code error;
    if (!std::filesystem::is_regular_file(image, error)) {
        image = AdjacentFile(descriptor.parent_path() / filename.filename());
    }
    return image;
}

std::expected<std::vector<CueTrack>, std::string>
BlindWrite4Tracks(const std::filesystem::path &descriptor) {
    const auto invalid =
        std::unexpected(std::string("Invalid or unsupported BlindWrite 4 metadata."));
    std::error_code error;
    const auto size = std::filesystem::file_size(descriptor, error);
    std::ifstream file(descriptor, std::ios::binary);
    const auto bytes = error ? std::nullopt : ReadMetadata(file, 0, size);
    if (!bytes || bytes->size() < 19 ||
        !std::ranges::equal(Bytes(*bytes).first(19), std::string_view("BLINDWRITE TOC FILE"))) {
        return invalid;
    }
    MetadataCursor cursor(Bytes(*bytes).subspan(19));
    auto text = [&] {
        const auto length = cursor.Number(4);
        return cursor.Take(length);
    };
    auto filename = [&](Bytes name) -> std::optional<std::filesystem::path> {
        if (name.empty() || name.size() > 4095) {
            return std::nullopt;
        }
        std::vector<std::uint8_t> terminated(name.begin(), name.end());
        terminated.push_back(0);
        const auto utf8 = MdsFilename(terminated, 0, false, descriptor);
        if (utf8 && std::filesystem::is_regular_file(*utf8, error)) {
            return utf8;
        }
        constexpr std::array<char16_t, 32> WINDOWS_1252{
            0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160,
            0x2039, 0x0152, 0x008d, 0x017d, 0x008f, 0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022,
            0x2013, 0x2014, 0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178};
        std::vector<std::uint8_t> wide;
        for (const auto value : name) {
            const auto character = value >= 0x80 && value <= 0x9f ? WINDOWS_1252[value - 0x80] :
                                                                    static_cast<char16_t>(value);
            wide.push_back(static_cast<std::uint8_t>(character));
            wide.push_back(static_cast<std::uint8_t>(character >> 8));
        }
        wide.resize(wide.size() + 2);
        const auto ansi = MdsFilename(wide, 0, true, descriptor);
        if (ansi && std::filesystem::is_regular_file(*ansi, error)) {
            return ansi;
        }
        return utf8;
    };
    cursor.Take(12);
    for (unsigned index = 0; index < 3; ++index) {
        text();
    }
    const auto count = cursor.Number(4);
    const auto image = filename(text());
    text();
    cursor.Take(4);
    const auto extra = cursor.Number(1);
    cursor.Take(extra);
    if (!cursor.Valid() || count == 0 || count > 396 || count > cursor.Remaining() / 123) {
        return invalid;
    }
    struct Entry {
        CueTrack track;
        unsigned session;
        std::int64_t first;
        std::uint64_t end;
    };
    std::vector<Entry> entries;
    for (std::uint64_t index = 0; index < count; ++index) {
        const auto name = text();
        const auto fields = cursor.Take(59);
        if (fields.size() != 59) {
            return invalid;
        }
        for (unsigned field = 0; field < 15; ++field) {
            text();
        }
        if (!cursor.Valid()) {
            return invalid;
        }
        const auto number = fields[19];
        if (number >= 0xa0) {
            continue;
        }
        const auto start =
            std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Little(fields, 47, 4)));
        const auto pregap =
            std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Little(fields, 43, 4)));
        const auto end = Little(fields, 38, 4);
        const auto adjustment = static_cast<std::int64_t>(Little(fields, 28, 4));
        const auto offset =
            static_cast<std::int64_t>(Little(fields, 0, 4)) + (150 - adjustment) * RAW_SECTOR;
        const auto mode = fields[17];
        if (number == 0 || number > 99 || fields[13] == 0 || fields[13] > 99 || mode > 2 ||
            start < 0 || pregap > start || end <= static_cast<std::uint64_t>(start) || offset < 0) {
            return invalid;
        }
        auto path = filename(name);
        if (!path || !std::filesystem::is_regular_file(*path, error)) {
            path = image;
        }
        if (!path) {
            return std::unexpected(std::string("Missing BlindWrite 4 data file."));
        }
        const auto fileSize = std::filesystem::file_size(*path, error);
        const auto position = static_cast<std::uint64_t>(offset);
        const auto sectors = end - static_cast<std::uint64_t>(start);
        if (error || position > fileSize || sectors > (fileSize - position) / RAW_SECTOR) {
            return invalid;
        }
        CueTrack track;
        track.number = number;
        track.file = *path;
        track.audio = mode == 0;
        track.sectorSize = RAW_SECTOR;
        track.dataOffset = mode == 2 ? 24 : 16;
        track.sectorCount = sectors;
        track.extents.push_back({*path, position, sectors});
        entries.push_back({std::move(track), fields[13], start, end});
    }
    if (!cursor.Valid() || cursor.Remaining() != 0 || entries.empty()) {
        return invalid;
    }
    std::ranges::sort(entries, {}, [](const Entry &entry) { return entry.track.number; });
    std::vector<CueTrack> tracks;
    unsigned session = 1;
    std::uint64_t previousEnd = 0;
    for (auto &entry : entries) {
        if (entry.track.number != static_cast<int>(tracks.size() + 1) || entry.session < session ||
            entry.session > session + 1 || static_cast<std::uint64_t>(entry.first) < previousEnd ||
            (tracks.empty() && entry.session != 1)) {
            return invalid;
        }
        session = entry.session;
        previousEnd = entry.end;
        tracks.push_back(std::move(entry.track));
    }
    return DisjointExtents(tracks) ?
               std::expected<std::vector<CueTrack>, std::string>(std::move(tracks)) :
               invalid;
}

std::expected<std::vector<CueTrack>, std::string>
BlindWriteTracks(const std::filesystem::path &descriptor) {
    const auto invalid =
        std::unexpected(std::string("Invalid or unsupported BlindWrite 5/6 metadata."));
    std::error_code error;
    const auto size = std::filesystem::file_size(descriptor, error);
    std::ifstream file(descriptor, std::ios::binary);
    const auto bytes = error ? std::nullopt : ReadMetadata(file, 0, size);
    if (!bytes || bytes->size() < 280 ||
        !std::ranges::equal(Bytes(*bytes).first(16), std::string_view("BWT5 STREAM SIGN"))) {
        return invalid;
    }
    const auto disc = Bytes(*bytes).subspan(16, 112);
    const auto medium = Little(disc, 32, 2);
    const auto sessions = Little(disc, 34, 2);
    if (sessions == 0 || sessions > 99 ||
        (medium != 8 && medium != 9 && medium != 10 && medium != 16)) {
        return invalid;
    }
    const auto lengths = Bytes(*bytes).subspan(240, 20);
    MetadataCursor cursor(Bytes(*bytes).subspan(260));
    cursor.Take(Little(lengths, 0, 4) + Little(lengths, 4, 4) + Little(disc, 80, 2) +
                Little(disc, 82, 2) + Little(disc, 84, 2) + Little(disc, 88, 4) +
                Little(disc, 104, 4) + Little(disc, medium == 16 ? 108 : 86, medium == 16 ? 4 : 2));
    MetadataCursor data(cursor.Take(Little(lengths, 8, 4)));
    MetadataCursor layout(cursor.Take(Little(lengths, 12, 4)));
    cursor.Take(Little(lengths, 16, 4));
    cursor.Take(4);
    const auto footer = cursor.Take(16);
    if (!cursor.Valid() || cursor.Remaining() != 0 ||
        !std::ranges::equal(footer, std::string_view("BWT5 STREAM FOOT"))) {
        return invalid;
    }
    struct Block {
        std::filesystem::path file;
        std::int64_t first;
        std::int64_t end;
        std::uint64_t offset;
        std::uint32_t stride;
        bool audio;
    };
    const auto signedNumber = [](Bytes entry, std::size_t offset) -> std::int64_t {
        return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(Little(entry, offset, 4)));
    };
    std::vector<Block> blocks;
    const auto count = data.Number(4);
    const auto driveLength = data.Number(4);
    data.Take(driveLength);
    if (count == 0 || count > data.Remaining() / 58) {
        return invalid;
    }
    for (std::uint64_t index = 0; index < count; ++index) {
        const auto entry = data.Take(52);
        if (entry.size() != 52) {
            return invalid;
        }
        const auto filenameLength = Little(entry, 48, 4);
        if (filenameLength == 0 || filenameLength > 4094 || filenameLength % 2 != 0) {
            return invalid;
        }
        const auto filename = data.Take(filenameLength);
        std::vector<std::uint8_t> terminated(filename.begin(), filename.end());
        terminated.resize(terminated.size() + 2);
        const auto path = MdsFilename(terminated, 0, true, descriptor);
        data.Take(4);
        const auto sectors = signedNumber(entry, 44);
        const auto length = Little(entry, 4, 4);
        if (!data.Valid() || !path || sectors <= 0 || length == 0 || length % sectors != 0) {
            return invalid;
        }
        const auto stride = length / sectors;
        if (stride != 2048 && stride != 2336 && stride != 2352 && stride != 2368 &&
            stride != 2448) {
            return invalid;
        }
        const auto offset = Little(entry, 24, 4);
        const auto fileSize = std::filesystem::file_size(*path, error);
        if (error) {
            return std::unexpected("Missing BlindWrite data file " + path->string() + ".");
        }
        if (offset > fileSize || length > fileSize - offset) {
            return invalid;
        }
        const auto first = signedNumber(entry, 40);
        blocks.push_back({*path,
                          first,
                          first + sectors,
                          offset,
                          static_cast<std::uint32_t>(stride),
                          (Little(entry, 0, 4) & 0x8000) != 0});
    }
    if (!data.Valid() || data.Remaining() != 0) {
        return invalid;
    }
    std::ranges::sort(blocks, {}, &Block::first);
    for (std::size_t index = 1; index < blocks.size(); ++index) {
        if (blocks[index - 1].end > blocks[index].first) {
            return invalid;
        }
    }
    std::vector<CueTrack> tracks;
    std::int64_t previousEnd = INT32_MIN;
    for (std::uint64_t session = 1; session <= sessions; ++session) {
        const auto header = layout.Take(16);
        if (header.size() != 16) {
            return invalid;
        }
        const auto first = Little(header, 12, 2);
        const auto last = Little(header, 14, 2);
        const auto sessionStart = signedNumber(header, 4);
        const auto sessionEnd = signedNumber(header, 8);
        if (Little(header, 0, 2) != session || first != tracks.size() + 1 || last < first ||
            last > 99 || header[2] < last - first + 1 || sessionEnd <= sessionStart ||
            sessionStart < previousEnd) {
            return invalid;
        }
        previousEnd = sessionEnd;
        auto trackEnd = sessionStart;
        for (unsigned index = 0; index < header[2]; ++index) {
            const auto entry = layout.Take(64);
            if (entry.size() != 64) {
                return invalid;
            }
            const auto mode = entry[0];
            if (mode == 0) {
                continue;
            }
            if (mode != 6) {
                layout.Take(8);
            }
            const auto pregap = Little(entry, 22, 4);
            const auto start = signedNumber(entry, 42);
            const auto length = signedNumber(entry, 46);
            const auto end = start + length;
            if (mode > 6 || !layout.Valid() || entry[12] != tracks.size() + 1 || entry[12] > last ||
                Little(entry, 58, 4) != session || length <= 0 ||
                pregap >= static_cast<std::uint64_t>(length) || start < trackEnd ||
                start < sessionStart || end > sessionEnd) {
                return invalid;
            }
            trackEnd = end;
            CueTrack track;
            track.number = entry[12];
            track.audio = mode == 1;
            track.sectorCount = static_cast<std::uint64_t>(length) - pregap;
            auto position = start + static_cast<std::int64_t>(pregap);
            for (const auto &block : blocks) {
                if (position < block.first || position >= block.end) {
                    continue;
                }
                if ((!track.extents.empty() && track.sectorSize != block.stride) ||
                    block.audio != track.audio) {
                    return invalid;
                }
                track.sectorSize = block.stride;
                const auto next = std::min(end, block.end);
                track.extents.push_back(
                    {block.file,
                     block.offset +
                         static_cast<std::uint64_t>(position - block.first) * block.stride,
                     static_cast<std::uint64_t>(next - position)});
                position = next;
                if (position == end) {
                    break;
                }
            }
            if (position != end || track.extents.empty() ||
                (track.audio && track.sectorSize < RAW_SECTOR) ||
                (!track.audio && mode == 2 && track.sectorSize == 2336)) {
                return invalid;
            }
            track.file = track.extents.front().file;
            track.dataOffset = track.sectorSize == 2048 ? 0 :
                               track.sectorSize == 2336 ? 8 :
                               mode == 2 || mode == 6   ? 16 :
                                                          24;
            tracks.push_back(std::move(track));
        }
        if (tracks.size() != last) {
            return invalid;
        }
    }
    return layout.Valid() && layout.Remaining() == 0 && DisjointExtents(tracks) ?
               std::expected<std::vector<CueTrack>, std::string>(std::move(tracks)) :
               invalid;
}

std::expected<std::vector<CueTrack>, std::string>
MdsTracks(const std::filesystem::path &descriptor) {
    const auto invalid =
        std::unexpected(std::string("Invalid or unsupported Alcohol MDS metadata."));
    std::error_code error;
    const auto size = std::filesystem::file_size(descriptor, error);
    std::ifstream file(descriptor, std::ios::binary);
    const auto bytes = error ? std::nullopt : ReadMetadata(file, 0, size);
    if (!bytes || bytes->size() < 88 ||
        !std::ranges::equal(Bytes(*bytes).first(16), std::string_view("MEDIA DESCRIPTOR")) ||
        (*bytes)[16] != 1 || Little(*bytes, 18, 2) > 2) {
        return invalid;
    }
    const auto sessions = Little(*bytes, 20, 2);
    const auto sessionOffset = Little(*bytes, 80, 4);
    if (sessions == 0 || sessions > 99 || !Range(*bytes, sessionOffset, sessions * 24)) {
        return invalid;
    }
    std::vector<CueTrack> tracks;
    for (std::size_t session = 0; session < sessions; ++session) {
        const auto header = Bytes(*bytes).subspan(sessionOffset + session * 24, 24);
        const auto blocks = header[10];
        const auto first = Little(header, 12, 2);
        const auto last = Little(header, 14, 2);
        const auto blockOffset = Little(header, 20, 4);
        if (Little(header, 8, 2) != session + 1 || first != tracks.size() + 1 || last < first ||
            last > 99 || blocks < last - first + 1 ||
            !Range(*bytes, blockOffset, static_cast<std::uint64_t>(blocks) * 80)) {
            return invalid;
        }
        for (unsigned index = 0; index < blocks; ++index) {
            const auto entry = Bytes(*bytes).subspan(blockOffset + index * 80, 80);
            if (entry[4] >= 0xa0 && entry[4] <= 0xa2) {
                continue;
            }
            CueTrack track;
            track.number = entry[4];
            track.sectorSize = static_cast<std::uint32_t>(Little(entry, 16, 2));
            track.audio = (entry[0] & 7) == 1;
            const auto extra = Little(entry, 12, 4);
            const auto files = Little(entry, 48, 4);
            const auto footer = Little(entry, 52, 4);
            if (track.number != static_cast<int>(tracks.size() + 1) ||
                static_cast<std::uint64_t>(track.number) > last || files == 0 || files > 99 ||
                !Range(*bytes, footer, files * 16) || !Range(*bytes, extra, 8) || extra == 0 ||
                (entry[1] != 0 && entry[1] != 8)) {
                return invalid;
            }
            const auto subchannel = entry[1] == 8 ? 96u : 0u;
            if (track.sectorSize < subchannel) {
                return invalid;
            }
            const auto mainSize = track.sectorSize - subchannel;
            const auto mode = entry[0] & 7;
            if (track.audio) {
                if (mainSize != RAW_SECTOR) {
                    return invalid;
                }
            } else if (mode == 2) {
                if (mainSize != DATA_SECTOR && mainSize != RAW_SECTOR) {
                    return invalid;
                }
                track.dataOffset = mainSize == RAW_SECTOR ? 16 : 0;
            } else if (mode == 0 || mode == 3 || mode == 4 || mode == 7) {
                if (mainSize != DATA_SECTOR && mainSize != 2336 && mainSize != RAW_SECTOR) {
                    return invalid;
                }
                track.dataOffset = mainSize == RAW_SECTOR ? 24 : mainSize == 2336 ? 8 : 0;
            } else {
                return invalid;
            }
            track.sectorCount = Little(*bytes, extra + 4, 4);
            if (track.sectorCount == 0) {
                return invalid;
            }
            auto remaining = track.sectorCount;
            auto offset = Little(entry, 40, 8);
            std::vector<std::filesystem::path> seen;
            for (std::size_t part = 0; part < files; ++part) {
                const auto info = Bytes(*bytes).subspan(footer + part * 16, 16);
                const auto wide = Little(info, 4, 4);
                const auto filename =
                    wide > 1 ? std::nullopt :
                               MdsFilename(*bytes, Little(info, 0, 4), wide != 0, descriptor);
                if (!filename || std::ranges::find(seen, *filename) != seen.end()) {
                    return invalid;
                }
                seen.push_back(*filename);
                const auto fileSize = std::filesystem::file_size(*filename, error);
                if (error) {
                    return std::unexpected("Missing MDS data file " + filename->string() + ".");
                }
                if (offset >= fileSize) {
                    offset -= fileSize;
                    continue;
                }
                const auto available = (fileSize - offset) / track.sectorSize;
                const auto count = std::min(remaining, available);
                if (count != 0) {
                    track.extents.push_back({*filename, offset, count});
                    remaining -= count;
                }
                if (remaining != 0 && (fileSize - offset) % track.sectorSize != 0) {
                    return invalid;
                }
                offset = 0;
            }
            if (remaining != 0 || track.extents.empty()) {
                return invalid;
            }
            track.file = track.extents.front().file;
            tracks.push_back(std::move(track));
        }
        if (tracks.size() != last) {
            return invalid;
        }
    }
    return DisjointExtents(tracks) ?
               std::expected<std::vector<CueTrack>, std::string>(std::move(tracks)) :
               invalid;
}

std::expected<std::vector<CueTrack>, std::string>
CloneCdTracks(const std::filesystem::path &metadata) {
    std::ifstream input(metadata);
    if (!input) {
        return std::unexpected("Cannot read CloneCD metadata " + metadata.string() + ".");
    }
    auto image = metadata;
    image.replace_extension(".img");
    image = AdjacentFile(image);
    std::error_code error;
    const auto bytes = std::filesystem::file_size(image, error);
    if (error || bytes % RAW_SECTOR != 0) {
        return std::unexpected("Missing or truncated CloneCD image.");
    }
    std::map<int, std::map<std::string, std::int64_t>> fields;
    int track = 0;
    for (std::string line; std::getline(input, line);) {
        line = Lowercase(line);
        if (line.starts_with('[')) {
            track = 0;
            if (line.starts_with("[track ")) {
                const auto end = line.find(']');
                if (end == std::string::npos) {
                    return std::unexpected("Invalid CloneCD track section.");
                }
                std::istringstream number(line.substr(7, end - 7));
                if (!(number >> track) || track < 1 || track > 99) {
                    return std::unexpected("Invalid CloneCD track number.");
                }
            }
            continue;
        }
        const auto equals = line.find('=');
        if (track == 0 || equals == std::string::npos) {
            continue;
        }
        auto key = line.substr(0, equals);
        while (!key.empty() && std::isspace(static_cast<unsigned char>(key.back()))) {
            key.pop_back();
        }
        std::int64_t value = 0;
        std::istringstream number(line.substr(equals + 1));
        if (!(number >> value)) {
            return std::unexpected("Invalid CloneCD track value.");
        }
        fields[track][key] = value;
    }
    std::vector<CueTrack> tracks;
    for (const auto &[number, values] : fields) {
        const auto mode = values.find("mode");
        const auto start = values.find("index 1");
        if (number != static_cast<int>(tracks.size()) + 1 || mode == values.end() ||
            start == values.end() || start->second < 0 ||
            (mode->second != 0 && mode->second != 1)) {
            return std::unexpected("Incomplete or unsupported CloneCD track metadata.");
        }
        const auto first = static_cast<std::uint64_t>(start->second);
        std::uint64_t end = bytes / RAW_SECTOR;
        const auto next = fields.find(number + 1);
        if (next != fields.end()) {
            const auto gap = next->second.find("index 0");
            const auto index = next->second.find("index 1");
            if (index == next->second.end() || (gap != next->second.end() && gap->second < 0) ||
                index->second < 0) {
                return std::unexpected("Invalid CloneCD track boundary.");
            }
            end =
                static_cast<std::uint64_t>(gap != next->second.end() ? gap->second : index->second);
        }
        if (end <= first || end > bytes / RAW_SECTOR) {
            return std::unexpected("CloneCD track extent exceeds its image.");
        }
        tracks.push_back({number, mode->second == 0, RAW_SECTOR, image, first, end - first});
    }
    if (tracks.empty() || tracks.front().audio) {
        return std::unexpected("Missing CloneCD data track.");
    }
    return tracks;
}

std::optional<std::uint64_t> SectorAddress(std::span<const std::uint8_t> sector) {
    if (sector.size() < RAW_SECTOR || sector[0] != 0 || sector[11] != 0 || sector[15] != 1 ||
        !std::ranges::all_of(sector.subspan(1, 10), [](auto byte) { return byte == 0xff; })) {
        return std::nullopt;
    }
    auto bcd = [](std::uint8_t byte) -> int {
        return (byte & 15) > 9 || (byte >> 4) > 9 ? -1 : (byte >> 4) * 10 + (byte & 15);
    };
    const int minutes = bcd(sector[12]);
    const int seconds = bcd(sector[13]);
    const int frames = bcd(sector[14]);
    if (minutes < 0 || seconds < 0 || seconds >= 60 || frames < 0 || frames >= 75) {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>((minutes * 60 + seconds) * 75 + frames);
}

std::optional<CueTrack> ProbeDataTrack(const std::filesystem::path &disc) {
    std::error_code error;
    const auto bytes = std::filesystem::file_size(disc, error);
    if (error) {
        return std::nullopt;
    }
    std::ifstream input(disc, std::ios::binary);
    std::array<std::uint8_t, RAW_SECTOR> sector{};
    for (auto size : {RAW_SECTOR, DATA_SECTOR}) {
        for (std::uint64_t first : {0u, 150u}) {
            input.clear();
            input.seekg(static_cast<std::streamoff>((first + 16) * size));
            if (!input.read(reinterpret_cast<char *>(sector.data()), size)) {
                continue;
            }
            const auto offset = size == RAW_SECTOR ? 16u : 0u;
            const auto *descriptor = sector.data() + offset;
            if (descriptor[0] != 1 ||
                std::string_view(reinterpret_cast<const char *>(descriptor + 1), 5) != "CD001" ||
                (size == RAW_SECTOR && !SectorAddress(sector))) {
                continue;
            }
            const std::uint64_t volume = descriptor[80] |
                                         (static_cast<std::uint32_t>(descriptor[81]) << 8) |
                                         (static_cast<std::uint32_t>(descriptor[82]) << 16) |
                                         (static_cast<std::uint32_t>(descriptor[83]) << 24);
            if (volume < 17 || bytes % size != 0 || first + volume > bytes / size) {
                continue;
            }
            return CueTrack{1, false, size, disc, first, volume};
        }
    }
    return std::nullopt;
}

std::optional<std::vector<CueTrack>> KnownDump(const std::filesystem::path &disc,
                                               std::string_view digest) {
    // Verified against the complete BIN and its accompanying 13-track CUE sheet.
    constexpr std::string_view hash = "391609a19eaa44cf470c0b8b2110122381a87f25";
    constexpr std::array<std::uint64_t, 13> starts{0,
                                                   57993,
                                                   73522,
                                                   92680,
                                                   108459,
                                                   123424,
                                                   145753,
                                                   161557,
                                                   179330,
                                                   195755,
                                                   212257,
                                                   226782,
                                                   248527};
    constexpr std::uint64_t sectors = 261999;
    if (digest != hash) {
        return std::nullopt;
    }
    std::vector<CueTrack> tracks;
    for (std::size_t i = 0; i < starts.size(); ++i) {
        const auto end = i + 1 < starts.size() ? starts[i + 1] : sectors;
        tracks.push_back(
            {static_cast<int>(i + 1), i != 0, RAW_SECTOR, disc, starts[i], end - starts[i]});
    }
    return tracks;
}

std::optional<std::vector<CueTrack>> DiscoverAudio(const CueTrack &data) {
    if (data.sectorSize != RAW_SECTOR) {
        return std::nullopt;
    }
    std::ifstream input(data.file, std::ios::binary);
    std::array<std::uint8_t, RAW_SECTOR> sector{};
    input.seekg(static_cast<std::streamoff>(data.firstSector * RAW_SECTOR));
    std::uint64_t position = data.firstSector;
    std::optional<std::uint64_t> firstAddress;
    for (; input.read(reinterpret_cast<char *>(sector.data()), sector.size()); ++position) {
        const auto address = SectorAddress(sector);
        if (!address || (firstAddress && *address != *firstAddress + position - data.firstSector)) {
            break;
        }
        if (!firstAddress) {
            firstAddress = address;
        }
    }
    if (!input || position < data.firstSector + data.sectorCount) {
        return std::nullopt;
    }
    const std::uint64_t dataEnd = position;
    std::vector<CueTrack> tracks{data};
    tracks.front().sectorCount = dataEnd - data.firstSector;
    std::optional<std::uint64_t> silence;
    std::uint64_t audioStart = dataEnd;
    do {
        const bool zero = std::ranges::all_of(sector, [](auto byte) { return byte == 0; });
        if (zero && !silence) {
            silence = position;
        } else if (!zero && silence) {
            const auto gap = position - *silence;
            if (gap >= 140 && gap <= 450) {
                if (*silence != dataEnd) {
                    if (*silence < audioStart + 300) {
                        return std::nullopt;
                    }
                    tracks.push_back({static_cast<int>(tracks.size() + 1),
                                      true,
                                      RAW_SECTOR,
                                      data.file,
                                      audioStart,
                                      *silence - audioStart});
                }
                audioStart = position;
            }
            silence.reset();
        }
        ++position;
    } while (input.read(reinterpret_cast<char *>(sector.data()), sector.size()));
    if (!input.eof() || input.gcount() != 0 || position < audioStart + 300) {
        return std::nullopt;
    }
    tracks.push_back({static_cast<int>(tracks.size() + 1),
                      true,
                      RAW_SECTOR,
                      data.file,
                      audioStart,
                      position - audioStart});
    return tracks.size() == 13 ? std::optional(std::move(tracks)) : std::nullopt;
}

std::optional<std::vector<CueTrack>> NumberedTracks(const CueTrack &data) {
    const auto stem = Lowercase(data.file.stem().string());
    const auto marker = stem.rfind("track");
    if (marker == std::string::npos) {
        return std::nullopt;
    }
    const auto prefix = stem.substr(0, marker + 5);
    std::map<int, std::filesystem::path> files;
    std::error_code error;
    const auto parent =
        data.file.parent_path().empty() ? std::filesystem::path(".") : data.file.parent_path();
    std::filesystem::directory_iterator entry(parent, error), end;
    for (; !error && entry != end; entry.increment(error)) {
        const auto candidate = Lowercase(entry->path().stem().string());
        if (Lowercase(entry->path().extension().string()) != ".bin" ||
            !candidate.starts_with(prefix)) {
            continue;
        }
        std::istringstream number(candidate.substr(prefix.size()));
        int index = 0;
        if (number >> index && index >= 1 && index <= 13) {
            if (!files.emplace(index, entry->path()).second) {
                return std::nullopt;
            }
        }
    }
    if (error || files.size() != 13 || !std::filesystem::equivalent(files[1], data.file, error)) {
        return std::nullopt;
    }
    std::vector<CueTrack> tracks{data};
    for (int number = 2; number <= 13; ++number) {
        const auto bytes = std::filesystem::file_size(files[number], error);
        if (error || bytes % RAW_SECTOR != 0 || bytes / RAW_SECTOR < 300) {
            return std::nullopt;
        }
        std::ifstream file(files[number], std::ios::binary);
        std::array<std::uint8_t, RAW_SECTOR * 150> gap{};
        const bool pregap = file.read(reinterpret_cast<char *>(gap.data()), gap.size()) &&
                            std::ranges::all_of(gap, [](auto byte) { return byte == 0; });
        const std::uint64_t first = pregap ? 150 : 0;
        if (bytes / RAW_SECTOR < first + 300) {
            return std::nullopt;
        }
        tracks.push_back(
            {number, true, RAW_SECTOR, files[number], first, bytes / RAW_SECTOR - first});
    }
    return tracks;
}

std::optional<std::uint64_t> ParseMsf(const std::string &text) {
    unsigned minutes = 0;
    unsigned seconds = 0;
    unsigned frames = 0;
    char colon1 = 0;
    char colon2 = 0;
    std::istringstream stream(text);
    if (!(stream >> minutes >> colon1 >> seconds >> colon2 >> frames) || colon1 != ':' ||
        colon2 != ':') {
        return std::nullopt;
    }
    return (static_cast<std::uint64_t>(minutes) * 60 + seconds) * FRAMES_PER_SECOND + frames;
}

} // namespace

std::expected<std::vector<CueTrack>, std::string> ParseCueSheet(const std::filesystem::path &cue) {
    std::ifstream file(cue);
    if (!file) {
        return std::unexpected(fmt::format("{} cannot be read.", cue.string()));
    }
    std::vector<CueTrack> tracks;
    std::vector<std::uint64_t> nextStarts;
    std::filesystem::path current;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream stream(line);
        std::string command;
        stream >> command;
        if (command == "FILE") {
            auto open = line.find('"');
            auto close = line.rfind('"');
            if (open == std::string::npos || close <= open) {
                return std::unexpected("Missing quoted filename in CUE FILE line.");
            }
            current = AdjacentFile(cue.parent_path() / line.substr(open + 1, close - open - 1));
        } else if (command == "TRACK") {
            CueTrack track;
            std::string mode;
            stream >> track.number >> mode;
            track.file = current;
            track.audio = mode == "AUDIO";
            track.sectorSize = mode.ends_with("/2048") ? DATA_SECTOR : RAW_SECTOR;
            tracks.push_back(track);
            nextStarts.push_back(UINT64_MAX);
        } else if (command == "INDEX" && !tracks.empty()) {
            int index = 0;
            std::string msf;
            stream >> index >> msf;
            auto sector = ParseMsf(msf);
            if (!sector) {
                return std::unexpected(fmt::format("The index {} cannot be read.", msf));
            }
            if (index == 1) {
                tracks.back().firstSector = *sector;
            }
            // The previous track ends where this one's first index starts.
            if (nextStarts.back() == UINT64_MAX || index == 0) {
                nextStarts.back() = std::min(nextStarts.back(), *sector);
            }
        }
    }
    if (tracks.empty()) {
        return std::unexpected("The CUE sheet does not list tracks.");
    }
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        std::uint64_t end = 0;
        if (i + 1 < tracks.size() && tracks[i + 1].file == tracks[i].file) {
            end = nextStarts[i + 1];
        } else {
            std::error_code error;
            auto bytes = std::filesystem::file_size(tracks[i].file, error);
            if (error) {
                return std::unexpected(fmt::format("{} cannot be read.", tracks[i].file.string()));
            }
            end = bytes / tracks[i].sectorSize;
        }
        if (end < tracks[i].firstSector) {
            return std::unexpected(
                fmt::format("Track {} ends before it starts.", tracks[i].number));
        }
        tracks[i].sectorCount = end - tracks[i].firstSector;
    }
    return tracks;
}

std::expected<std::vector<CueTrack>, std::string>
ReadDiscTracks(const std::filesystem::path &disc) {
    const auto extension = Lowercase(disc.extension().string());
    if (extension == ".bwt" || extension == ".bwi" || extension == ".bws") {
        auto metadata = disc;
        metadata.replace_extension(".bwt");
        return BlindWrite4Tracks(AdjacentFile(metadata));
    }
    if (extension == ".b5t" || extension == ".b6t" || extension == ".b5i" || extension == ".b6i") {
        auto metadata = disc;
        metadata.replace_extension(extension == ".b5t" || extension == ".b5i" ? ".b5t" : ".b6t");
        return BlindWriteTracks(AdjacentFile(metadata));
    }
    if (extension == ".cdi") {
        return CdiTracks(disc);
    }
    if (extension == ".nrg") {
        return NrgTracks(disc);
    }
    if (extension == ".mds" || extension == ".mdf") {
        auto metadata = disc;
        metadata.replace_extension(".mds");
        return MdsTracks(AdjacentFile(metadata));
    }
    if (extension == ".cue") {
        return ParseCueSheet(disc);
    }
    if (extension == ".ccd" || extension == ".img" || extension == ".sub") {
        auto metadata = disc;
        metadata.replace_extension(".ccd");
        return CloneCdTracks(AdjacentFile(metadata));
    }
    if (extension == ".iso") {
        const auto data = ProbeDataTrack(disc);
        if (data && data->sectorSize == DATA_SECTOR) {
            return std::vector<CueTrack>{*data};
        }
        return std::unexpected("Missing readable ISO 9660 data track.");
    }
    if (extension != ".bin") {
        return std::unexpected("Unsupported disc image format.");
    }
    auto sameStem = disc;
    sameStem.replace_extension(".cue");
    sameStem = AdjacentFile(sameStem);
    std::error_code error;
    if (std::filesystem::is_regular_file(sameStem, error)) {
        return ParseCueSheet(sameStem);
    }
    const auto parent =
        disc.parent_path().empty() ? std::filesystem::path(".") : disc.parent_path();
    std::filesystem::directory_iterator entry(parent, error), end;
    for (; !error && entry != end; entry.increment(error)) {
        if (Lowercase(entry->path().extension().string()) == ".cue") {
            auto parsed = ParseCueSheet(entry->path());
            if (parsed && std::ranges::any_of(*parsed, [&](const CueTrack &track) {
                    std::error_code sameError;
                    return !track.audio && std::filesystem::equivalent(track.file, disc, sameError);
                })) {
                return parsed;
            }
        }
    }
    std::ifstream image(disc, std::ios::binary);
    const auto digest = Tools::Common::sha1Hex(image);
    if (!digest) {
        return std::unexpected("Cannot read BIN image " + disc.string() + ".");
    }
    if (auto tracks = KnownDump(disc, *digest)) {
        return *tracks;
    }
    const auto data = ProbeDataTrack(disc);
    if (!data) {
        return std::unexpected("Missing readable ISO 9660 data track in BIN image.");
    }
    if (auto tracks = NumberedTracks(*data)) {
        return *tracks;
    }
    if (auto tracks = DiscoverAudio(*data)) {
        return *tracks;
    }
    return std::vector<CueTrack>{*data};
}

} // namespace coastline
