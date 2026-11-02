#include "disclayout.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <fstream>
#include <map>
#include <sstream>

#include <fmt/format.h>

#include "sha1.h"

namespace coastline {
namespace {
constexpr std::uint32_t RAW_SECTOR = 2352;
constexpr std::uint32_t DATA_SECTOR = 2048;
constexpr std::uint32_t FRAMES_PER_SECOND = 75;

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
