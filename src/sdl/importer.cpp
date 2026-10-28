#include "importer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <format>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>

#include <FLAC/stream_encoder.h>

#include "sha1.h"

namespace coastline {

namespace {

constexpr std::uint32_t RAW_SECTOR = 2352;
constexpr std::uint32_t DATA_SECTOR = 2048;
constexpr std::uint32_t FRAMES_PER_SECOND = 75;
constexpr std::uint32_t SAMPLES_PER_SECTOR = RAW_SECTOR / 4;

struct Release {
    RomRelease release;
    const char *sha1;
    const char *file;
};

constexpr std::array<Release, 3> RELEASES{{
    {RomRelease::Usa, "bfc51f086ebdb78904dc24ad0c3a2d946e26d022", "usa.z64"},
    {RomRelease::Jpn, "e0415b87c5a5b69ac581e79137406bdaa79b354a", "jpn.z64"},
    {RomRelease::Eur, "9fae5d98fe3464f00d59034690df90118646eb95", "eur.z64"},
}};

std::optional<std::vector<std::uint8_t>> ReadWholeFile(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), {});
}

std::uint32_t Little32(const std::uint8_t *bytes) {
    return bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

// Reads the user data of the sectors of a data track, raw or cooked.
class SectorReader {
public:
    SectorReader(std::ifstream &file, std::uint32_t sectorSize, std::uint64_t firstSector)
        : m_file(file), m_sectorSize(sectorSize), m_firstSector(firstSector),
          m_dataOffset(sectorSize == RAW_SECTOR ? 16 : 0) {
    }

    bool Read(std::uint64_t sector, std::uint32_t count, std::vector<std::uint8_t> &out) {
        out.resize(static_cast<std::size_t>(count) * DATA_SECTOR);
        for (std::uint32_t i = 0; i < count; ++i) {
            m_file.seekg(static_cast<std::streamoff>((m_firstSector + sector + i) * m_sectorSize +
                                                     m_dataOffset));
            if (!m_file.read(reinterpret_cast<char *>(out.data() + i * DATA_SECTOR), DATA_SECTOR)) {
                return false;
            }
        }
        return true;
    }

private:
    std::ifstream &m_file;
    std::uint32_t m_sectorSize;
    std::uint64_t m_firstSector;
    std::uint32_t m_dataOffset;
};

struct IsoEntry {
    std::filesystem::path path;
    std::uint32_t extent = 0;
    std::uint32_t size = 0;
};

std::string DecodeName(const std::uint8_t *name, std::size_t length, bool joliet) {
    std::string result;
    if (joliet) {
        for (std::size_t i = 0; i + 1 < length; i += 2) {
            char32_t c = static_cast<char32_t>((name[i] << 8) | name[i + 1]);
            if (c < 0x80) {
                result += static_cast<char>(c);
            } else if (c < 0x800) {
                result += static_cast<char>(0xC0 | (c >> 6));
                result += static_cast<char>(0x80 | (c & 0x3F));
            } else {
                result += static_cast<char>(0xE0 | (c >> 12));
                result += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (c & 0x3F));
            }
        }
    } else {
        result.assign(reinterpret_cast<const char *>(name), length);
    }
    if (auto version = result.find(';'); version != std::string::npos) {
        result.erase(version);
    }
    if (!result.empty() && result.back() == '.') {
        result.pop_back();
    }
    return result;
}

bool ListDirectory(SectorReader &reader,
                   std::uint32_t extent,
                   std::uint32_t size,
                   const std::filesystem::path &prefix,
                   bool joliet,
                   int depth,
                   std::vector<IsoEntry> &entries) {
    // A disc nests at most eight levels; the bound also stops a looping image.
    if (depth > 8) {
        return false;
    }
    std::vector<std::uint8_t> data;
    if (!reader.Read(extent, (size + DATA_SECTOR - 1) / DATA_SECTOR, data)) {
        return false;
    }
    std::size_t offset = 0;
    while (offset < size) {
        std::uint8_t length = data[offset];
        if (length == 0) {
            offset = (offset / DATA_SECTOR + 1) * DATA_SECTOR;
            continue;
        }
        const std::uint8_t *record = &data[offset];
        std::uint8_t nameLength = record[32];
        bool special = nameLength == 1 && (record[33] == 0 || record[33] == 1);
        if (!special) {
            auto path = prefix / DecodeName(record + 33, nameLength, joliet);
            std::uint32_t childExtent = Little32(record + 2);
            std::uint32_t childSize = Little32(record + 10);
            if (record[25] & 0x02) {
                if (!ListDirectory(
                        reader, childExtent, childSize, path, joliet, depth + 1, entries)) {
                    return false;
                }
            } else {
                entries.push_back({path, childExtent, childSize});
            }
        }
        offset += length;
    }
    return true;
}

std::expected<std::vector<IsoEntry>, std::string> ListIso(SectorReader &reader) {
    std::vector<std::uint8_t> descriptor;
    std::optional<std::array<std::uint32_t, 2>> primary;
    std::optional<std::array<std::uint32_t, 2>> joliet;
    for (std::uint64_t sector = 16; sector < 64; ++sector) {
        if (!reader.Read(sector, 1, descriptor)) {
            return std::unexpected("The data track ends before its volume descriptors.");
        }
        if (std::string_view(reinterpret_cast<char *>(&descriptor[1]), 5) != "CD001") {
            return std::unexpected("The data track is not an ISO 9660 volume.");
        }
        std::array<std::uint32_t, 2> root{Little32(&descriptor[156 + 2]),
                                          Little32(&descriptor[156 + 10])};
        if (descriptor[0] == 1) {
            primary = root;
        } else if (descriptor[0] == 2 && descriptor[88] == '%' && descriptor[89] == '/' &&
                   (descriptor[90] == '@' || descriptor[90] == 'C' || descriptor[90] == 'E')) {
            joliet = root;
        } else if (descriptor[0] == 255) {
            break;
        }
    }
    auto root = joliet ? joliet : primary;
    if (!root) {
        return std::unexpected("The volume has no primary descriptor.");
    }
    std::vector<IsoEntry> entries;
    if (!ListDirectory(reader, (*root)[0], (*root)[1], {}, joliet.has_value(), 0, entries)) {
        return std::unexpected("A directory of the volume cannot be read.");
    }
    return entries;
}

// The installers, DirectX, and the programs of the Windows release are not game data.
bool IsGameData(const std::filesystem::path &path) {
    auto lower = [](std::string text) {
        std::ranges::transform(text, text.begin(), [](unsigned char c) { return std::tolower(c); });
        return text;
    };
    if (lower(path.begin()->string()) == "directx") {
        return false;
    }
    // The text of the game is in the string resources of BRString.dll, which the game reads.
    if (lower(path.filename().string()) == "brstring.dll") {
        return true;
    }
    static constexpr std::array<std::string_view, 10> skipped{
        ".exe", ".dll", ".cab", ".ex_", ".inf", ".url", ".tag", ".ini", ".bin", ".hdr"};
    auto extension = lower(path.extension().string());
    return std::ranges::find(skipped, extension) == skipped.end();
}

std::optional<std::string>
CopyIsoFile(SectorReader &reader, const IsoEntry &entry, const std::filesystem::path &destination) {
    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    std::ofstream out(destination, std::ios::binary | std::ios::trunc);
    if (!out) {
        return std::format("{} cannot be written.", destination.string());
    }
    std::vector<std::uint8_t> chunk;
    constexpr std::uint32_t CHUNK_SECTORS = 256;
    std::uint32_t remaining = entry.size;
    for (std::uint32_t sector = 0; remaining > 0; sector += CHUNK_SECTORS) {
        std::uint32_t bytes = std::min(remaining, CHUNK_SECTORS * DATA_SECTOR);
        if (!reader.Read(entry.extent + sector, (bytes + DATA_SECTOR - 1) / DATA_SECTOR, chunk)) {
            return std::format("{} is cut short on the disc.", entry.path.string());
        }
        out.write(reinterpret_cast<const char *>(chunk.data()), bytes);
        remaining -= bytes;
    }
    return out ? std::nullopt :
                 std::optional(std::format("{} was not written.", destination.string()));
}

struct EncoderDeleter {
    void operator()(FLAC__StreamEncoder *encoder) const {
        FLAC__stream_encoder_delete(encoder);
    }
};

std::optional<std::string> EncodeAudioTrack(std::ifstream &file,
                                            const CueTrack &track,
                                            const std::filesystem::path &destination) {
    std::unique_ptr<FLAC__StreamEncoder, EncoderDeleter> encoder(FLAC__stream_encoder_new());
    if (!encoder) {
        return "The FLAC encoder cannot be created.";
    }
    FLAC__stream_encoder_set_channels(encoder.get(), 2);
    FLAC__stream_encoder_set_bits_per_sample(encoder.get(), 16);
    FLAC__stream_encoder_set_sample_rate(encoder.get(), 44100);
    FLAC__stream_encoder_set_compression_level(encoder.get(), 8);
    FLAC__stream_encoder_set_total_samples_estimate(encoder.get(),
                                                    track.sectorCount * SAMPLES_PER_SECTOR);
    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    if (FLAC__stream_encoder_init_file(
            encoder.get(), destination.string().c_str(), nullptr, nullptr) !=
        FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
        return std::format("{} cannot be written.", destination.string());
    }
    constexpr std::uint64_t CHUNK_SECTORS = 75;
    std::vector<std::uint8_t> raw(CHUNK_SECTORS * RAW_SECTOR);
    std::vector<FLAC__int32> samples(CHUNK_SECTORS * SAMPLES_PER_SECTOR * 2);
    file.seekg(static_cast<std::streamoff>(track.firstSector * RAW_SECTOR));
    for (std::uint64_t done = 0; done < track.sectorCount; done += CHUNK_SECTORS) {
        std::uint64_t count = std::min(CHUNK_SECTORS, track.sectorCount - done);
        if (!file.read(reinterpret_cast<char *>(raw.data()),
                       static_cast<std::streamsize>(count * RAW_SECTOR))) {
            return std::format("Track {} is cut short in the image.", track.number);
        }
        // CD audio is 16-bit little-endian stereo.
        std::size_t values = count * RAW_SECTOR / 2;
        for (std::size_t i = 0; i < values; ++i) {
            samples[i] = static_cast<std::int16_t>(raw[i * 2] | (raw[i * 2 + 1] << 8));
        }
        if (!FLAC__stream_encoder_process_interleaved(
                encoder.get(), samples.data(), static_cast<unsigned>(values / 2))) {
            return std::format("Track {} was not encoded.", track.number);
        }
    }
    if (!FLAC__stream_encoder_finish(encoder.get())) {
        return std::format("Track {} was not finished.", track.number);
    }
    return std::nullopt;
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

bool NormalizeRomByteOrder(std::span<std::uint8_t> image) {
    if (image.size() < 4 || image.size() % 4 != 0) {
        return false;
    }
    std::uint32_t first = (image[0] << 24) | (image[1] << 16) | (image[2] << 8) | image[3];
    if (first == 0x80371240) {
        return true;
    }
    if (first == 0x37804012) {
        for (std::size_t i = 0; i < image.size(); i += 2) {
            std::swap(image[i], image[i + 1]);
        }
        return true;
    }
    if (first == 0x40123780) {
        for (std::size_t i = 0; i < image.size(); i += 4) {
            std::swap(image[i], image[i + 3]);
            std::swap(image[i + 1], image[i + 2]);
        }
        return true;
    }
    return false;
}

std::expected<RomRelease, std::string> IdentifyRom(std::span<const std::uint8_t> image) {
    auto digest = Tools::Common::sha1Hex(image);
    for (const auto &release : RELEASES) {
        if (digest == release.sha1) {
            return release.release;
        }
    }
    return std::unexpected(std::format(
        "The image is not an unmodified release of Top Gear Rally (SHA-1 {}).", digest));
}

std::expected<RomRelease, std::string> ImportRom(const std::filesystem::path &image,
                                                 const std::filesystem::path &dataDirectory) {
    auto bytes = ReadWholeFile(image);
    if (!bytes) {
        return std::unexpected(std::format("{} cannot be read.", image.string()));
    }
    if (!NormalizeRomByteOrder(*bytes)) {
        return std::unexpected(std::format("{} is not a Nintendo 64 image.", image.string()));
    }
    auto release = IdentifyRom(*bytes);
    if (!release) {
        return release;
    }
    auto destination =
        dataDirectory / "roms" / std::ranges::find(RELEASES, *release, &Release::release)->file;
    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    std::ofstream out(destination, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char *>(bytes->data()),
              static_cast<std::streamsize>(bytes->size()));
    if (!out) {
        return std::unexpected(std::format("{} cannot be written.", destination.string()));
    }
    return release;
}

std::expected<std::vector<CueTrack>, std::string> ParseCueSheet(const std::filesystem::path &cue) {
    std::ifstream file(cue);
    if (!file) {
        return std::unexpected(std::format("{} cannot be read.", cue.string()));
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
                return std::unexpected("A FILE line of the CUE sheet has no quoted name.");
            }
            current = cue.parent_path() / line.substr(open + 1, close - open - 1);
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
                return std::unexpected(std::format("The index {} cannot be read.", msf));
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
        return std::unexpected("The CUE sheet lists no tracks.");
    }
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        std::uint64_t end = 0;
        if (i + 1 < tracks.size() && tracks[i + 1].file == tracks[i].file) {
            end = nextStarts[i + 1];
        } else {
            std::error_code error;
            auto bytes = std::filesystem::file_size(tracks[i].file, error);
            if (error) {
                return std::unexpected(std::format("{} cannot be read.", tracks[i].file.string()));
            }
            end = bytes / tracks[i].sectorSize;
        }
        if (end < tracks[i].firstSector) {
            return std::unexpected(
                std::format("Track {} ends before it starts.", tracks[i].number));
        }
        tracks[i].sectorCount = end - tracks[i].firstSector;
    }
    return tracks;
}

std::expected<std::size_t, std::string> ImportDisc(const std::filesystem::path &disc,
                                                   const std::filesystem::path &dataDirectory,
                                                   const ImportProgress &progress) {
    std::vector<CueTrack> tracks;
    auto extension = disc.extension().string();
    std::ranges::transform(
        extension, extension.begin(), [](unsigned char c) { return std::tolower(c); });
    if (extension == ".cue") {
        auto parsed = ParseCueSheet(disc);
        if (!parsed) {
            return std::unexpected(parsed.error());
        }
        tracks = std::move(*parsed);
    } else {
        tracks.push_back({1, false, DATA_SECTOR, disc, 0, 0});
    }
    auto data = std::ranges::find(tracks, false, &CueTrack::audio);
    if (data == tracks.end()) {
        return std::unexpected("The disc has no data track.");
    }
    std::ifstream dataFile(data->file, std::ios::binary);
    if (!dataFile) {
        return std::unexpected(std::format("{} cannot be read.", data->file.string()));
    }
    SectorReader reader(dataFile, data->sectorSize, data->firstSector);
    auto entries = ListIso(reader);
    if (!entries) {
        return std::unexpected(entries.error());
    }
    std::erase_if(*entries, [](const IsoEntry &entry) { return !IsGameData(entry.path); });
    if (std::ranges::none_of(*entries, [](const IsoEntry &entry) {
            return entry.path.filename() == "BossRally.pod" ||
                   entry.path.filename() == "BOSSRALLY.POD";
        })) {
        return std::unexpected("The disc is not Boss Rally: BossRally.pod is missing.");
    }
    std::size_t audioCount = std::ranges::count(tracks, true, &CueTrack::audio);
    double steps = static_cast<double>(entries->size() + audioCount);
    std::size_t written = 0;
    for (const auto &entry : *entries) {
        if (progress) {
            progress(entry.path.generic_string(), static_cast<double>(written) / steps);
        }
        if (auto failure = CopyIsoFile(reader, entry, dataDirectory / entry.path)) {
            return std::unexpected(*failure);
        }
        ++written;
    }
    for (const auto &track : tracks) {
        if (!track.audio) {
            continue;
        }
        auto name = std::format("track{:02}.flac", track.number);
        if (progress) {
            progress("music/" + name, static_cast<double>(written) / steps);
        }
        std::ifstream audio(track.file, std::ios::binary);
        if (auto failure = EncodeAudioTrack(audio, track, dataDirectory / "music" / name)) {
            return std::unexpected(*failure);
        }
        ++written;
    }
    if (progress) {
        progress("done", 1.0);
    }
    return written;
}

} // namespace coastline
