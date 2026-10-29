#include "importer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <sstream>

#include <FLAC/stream_encoder.h>
#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>

#include "assetchain.h"
#include "cartridge/eu.h"
#include "cartridge/jp.h"
#include "cartridge/us.h"
#include "sdl/ghostfile.h"
#include "sdl/indexedimage.h"
#include "sdl/n64model.h"
#include "sdl/stringresources.h"
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
    const char *directory;                   // The directory of the release under n64/ in the data.
    std::span<const unsigned char> manifest; // The assets, as tools/sdl/cartridgemanifest.py
                                             // writes them.
};

const std::array<Release, 3> RELEASES{{
    {RomRelease::Usa, "bfc51f086ebdb78904dc24ad0c3a2d946e26d022", "usa", CARTRIDGE_US},
    {RomRelease::Jpn, "e0415b87c5a5b69ac581e79137406bdaa79b354a", "jpn", CARTRIDGE_JP},
    {RomRelease::Eur, "9fae5d98fe3464f00d59034690df90118646eb95", "eur", CARTRIDGE_EU},
}};

// The main segment: its code and data at this offset of the cartridge, up to the end of its
// read-only data.
constexpr std::size_t SEGMENT_ROM = 0x1000;
constexpr std::size_t SEGMENT_SIZE = 0xAC400;

bool WriteBytes(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char *>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(out);
}

// Write the main segment and every asset of the manifest of a cartridge as <name>.bin, a zlib
// asset inflated. The manifest covers every byte from its first asset to the end of the image,
// and the bytes of padding must be zero.
std::optional<std::string> ExtractCartridge(std::span<const std::uint8_t> rom,
                                            std::span<const unsigned char> manifest,
                                            const std::filesystem::path &directory) {
    if (rom.size() < SEGMENT_ROM + SEGMENT_SIZE) {
        return "the image is too short";
    }
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (!WriteBytes(directory / "segment.bin", rom.subspan(SEGMENT_ROM, SEGMENT_SIZE))) {
        return std::format("{} cannot be written", (directory / "segment.bin").string());
    }
    std::istringstream lines(std::string(manifest.begin(), manifest.end()));
    std::optional<std::size_t> position;
    std::string line;
    while (std::getline(lines, line)) {
        std::istringstream fields(line);
        std::size_t offset = 0;
        std::size_t size = 0;
        std::string kind;
        std::string name;
        if (!(fields >> std::hex >> offset >> std::dec >> size >> kind >> name) ||
            (position && offset != *position) || offset > rom.size() ||
            size > rom.size() - offset) {
            return std::format("the manifest does not fit the image at '{}'", line);
        }
        position = offset + size;
        auto bytes = rom.subspan(offset, size);
        if (name == "padding") {
            if (std::ranges::any_of(bytes, [](std::uint8_t byte) { return byte != 0; })) {
                return std::format("the padding at {:#x} is not zero", offset);
            }
            continue;
        }
        std::vector<std::uint8_t> inflated;
        if (kind == "zlib") {
            auto data = Tools::Common::decompress(bytes);
            if (size < 4 || (static_cast<std::size_t>(bytes[0]) << 24 | bytes[1] << 16 |
                             bytes[2] << 8 | bytes[3]) != size) {
                return std::format("{} at {:#x} is not an asset of {} bytes", name, offset, size);
            }
            if (!data) {
                return std::format("{} at {:#x} does not inflate", name, offset);
            }
            inflated = std::move(*data);
            bytes = inflated;
        }
        auto path = directory / (name + ".bin");
        std::filesystem::create_directories(path.parent_path(), error);
        if (!WriteBytes(path, bytes)) {
            return std::format("{} cannot be written", path.string());
        }
    }
    if (position != rom.size()) {
        return "the manifest ends before the end of the image";
    }
    return std::nullopt;
}

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

std::string Lowercase(std::string text) {
    std::ranges::transform(text, text.begin(), [](unsigned char c) { return std::tolower(c); });
    return text;
}

// The files of the disc the game opens. The video, the installers, DirectX, the programs, and the
// pictures of the menus and the loading screen of the Windows release, which the front end of the
// Nintendo 64 release replaces, are left on the disc.
bool IsGameData(const std::filesystem::path &path) {
    static constexpr std::array<std::string_view, 5> directories{
        "cargfx", "cars", "paint", "sfx", "tracks"};
    // The text of the game is in the string resources of BRString.dll.
    static constexpr std::array<std::string_view, 6> files{"bossrally.pod",
                                                           "brstring.dll",
                                                           "rallycredits.dat",
                                                           "rallyintro1.dat",
                                                           "rallyintro2.dat",
                                                           "rallyoutro.dat"};
    std::string first = Lowercase(path.begin()->string());
    if (std::next(path.begin()) != path.end()) {
        return std::ranges::find(directories, first) != directories.end();
    }
    return std::ranges::find(files, first) != files.end();
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

struct SurfaceDeleter {
    void operator()(SDL_Surface *surface) const {
        SDL_DestroySurface(surface);
    }
};
using Surface = std::unique_ptr<SDL_Surface, SurfaceDeleter>;

// Whether two surfaces have the same size and the same pixels, as RGB24.
bool SamePixels(SDL_Surface *a, SDL_Surface *b) {
    Surface first(SDL_ConvertSurface(a, SDL_PIXELFORMAT_RGB24));
    Surface second(SDL_ConvertSurface(b, SDL_PIXELFORMAT_RGB24));
    if (!first || !second || first->w != second->w || first->h != second->h) {
        return false;
    }
    for (int row = 0; row < first->h; ++row) {
        if (std::memcmp(static_cast<const char *>(first->pixels) +
                            static_cast<std::size_t>(row) * static_cast<std::size_t>(first->pitch),
                        static_cast<const char *>(second->pixels) +
                            static_cast<std::size_t>(row) * static_cast<std::size_t>(second->pitch),
                        static_cast<std::size_t>(first->w) * 3) != 0) {
            return false;
        }
    }
    return true;
}

// A bitmap as a PNG image of the same name, in place of the bitmap once the image reads back with
// the same pixels.
std::optional<std::string> ConvertBitmap(const std::filesystem::path &bitmap) {
    auto png = std::filesystem::path(bitmap).replace_extension(".png");
    Surface original(SDL_LoadBMP(bitmap.string().c_str()));
    if (!original) {
        return std::format("{} cannot be read: {}", bitmap.string(), SDL_GetError());
    }
    if (!SDL_SavePNG(original.get(), png.string().c_str())) {
        return std::format("{} cannot be written: {}", png.string(), SDL_GetError());
    }
    Surface converted(SDL_LoadPNG(png.string().c_str()));
    if (!converted || !SamePixels(original.get(), converted.get())) {
        return std::format("{} does not read back as {}.", png.string(), bitmap.string());
    }
    std::error_code error;
    std::filesystem::remove(bitmap, error);
    return std::nullopt;
}

// Each ghost recording of the demos as JSON of the same name, in place of the file once the JSON
// builds the same bytes.
std::optional<std::string> ConvertGhostRecordings(const std::filesystem::path &dataDirectory) {
    std::error_code error;
    std::vector<std::filesystem::path> recordings;
    for (const auto &entry : std::filesystem::directory_iterator(dataDirectory, error)) {
        std::string name = Lowercase(entry.path().filename().string());
        if (name.starts_with("rally") && name.ends_with(".dat")) {
            recordings.push_back(entry.path());
        }
    }
    for (const auto &recording : recordings) {
        std::ifstream in(recording, std::ios::binary);
        std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>(in), {});
        in.close();
        auto json = coastline::GhostToJson(bytes);
        auto rebuilt = json ? coastline::GhostFromJson(*json) : std::nullopt;
        if (!rebuilt || *rebuilt != bytes) {
            return std::format("{} does not convert to JSON without loss.", recording.string());
        }
        auto destination = std::filesystem::path(recording).replace_extension(".json");
        std::ofstream out(destination, std::ios::trunc);
        out << coastline::FormatGhostJson(*json);
        out.close();
        // The text written is read back and must build the same bytes.
        std::ifstream written(destination);
        auto parsed = nlohmann::ordered_json::parse(written, nullptr, false);
        auto reread = parsed.is_discarded() ? std::nullopt : coastline::GhostFromJson(parsed);
        if (!reread || *reread != bytes) {
            return std::format(
                "{} does not read back as {}.", destination.string(), recording.string());
        }
        std::filesystem::remove(recording, error);
    }
    return std::nullopt;
}

using ModelConverter = std::function<std::expected<GltfModel, std::string>(
    std::span<const std::uint8_t> bytes, std::string_view bufferUri)>;

// Writes a model of the console as glTF with its buffer and images, and removes the model once
// the written files build the same bytes.
std::optional<std::string> ConvertModel(const std::filesystem::path &car,
                                        const ModelConverter &convert,
                                        const std::filesystem::path &gltfPath,
                                        const std::filesystem::path &bufferPath) {
    std::error_code error;
    {
        auto bytes = ReadWholeFile(car);
        if (!bytes) {
            return std::format("{} cannot be read.", car.string());
        }
        auto model = convert(*bytes, bufferPath.filename().string());
        if (!model) {
            return std::format("{} is not a model ({}).", car.string(), model.error());
        }
        std::ofstream(gltfPath, std::ios::trunc) << model->json.dump(1) << '\n';
        if (!WriteBytes(bufferPath, model->buffer)) {
            return std::format("{} cannot be written.", bufferPath.string());
        }
        for (const GltfImage &texture : model->images) {
            auto path = car.parent_path() / texture.uri;
            if (!SaveRgba8Png(path, texture.rgba, texture.width, texture.height)) {
                return std::format("{} does not read back as its texels.", path.string());
            }
        }
        std::ifstream written(gltfPath);
        auto parsed = nlohmann::ordered_json::parse(written, nullptr, false);
        auto buffer = ReadWholeFile(bufferPath);
        auto rebuilt = parsed.is_discarded() || !buffer ?
                           std::unexpected(std::string("the files cannot be read")) :
                           ModelFromGltf(parsed, *buffer);
        if (!rebuilt || *rebuilt != *bytes) {
            return std::format("{} does not read back as {}.", gltfPath.string(), car.string());
        }
    }
    std::filesystem::remove(car, error);
    return std::nullopt;
}

// Replaces each .rca file of cars with a glTF file of the same name.
std::optional<std::string> ConvertCarModels(const std::filesystem::path &dataDirectory) {
    constexpr std::size_t RCA_HEADER = 0x8000;
    std::error_code error;
    std::vector<std::filesystem::path> cars;
    for (const auto &entry : std::filesystem::directory_iterator(dataDirectory / "cars", error)) {
        if (Lowercase(entry.path().extension().string()) == ".rca") {
            cars.push_back(entry.path());
        }
    }
    auto convert = [](std::span<const std::uint8_t> bytes, std::string_view bufferUri) {
        return CarModelToGltf(bytes, RCA_HEADER, bufferUri);
    };
    for (const auto &car : cars) {
        if (auto failure = ConvertModel(car,
                                        convert,
                                        std::filesystem::path(car).replace_extension(".gltf"),
                                        std::filesystem::path(car).replace_extension(".bin"))) {
            return failure;
        }
    }
    return std::nullopt;
}

// Replaces each .trk file of tracks with a glTF file of the same name.
std::optional<std::string> ConvertTracks(const std::filesystem::path &dataDirectory) {
    std::error_code error;
    std::vector<std::filesystem::path> tracks;
    for (const auto &entry : std::filesystem::directory_iterator(dataDirectory / "tracks", error)) {
        if (Lowercase(entry.path().extension().string()) == ".trk") {
            tracks.push_back(entry.path());
        }
    }
    for (const auto &track : tracks) {
        if (auto failure = ConvertModel(track,
                                        TrackToGltf,
                                        std::filesystem::path(track).replace_extension(".gltf"),
                                        std::filesystem::path(track).replace_extension(".bin"))) {
            return failure;
        }
    }
    return std::nullopt;
}

// Each sky of cargfx, the 4-bit texels of a .ci4 file and the palette of the .lut4 file of the same
// name, as an indexed PNG image of 32 texels a row, in place of the two files.
std::optional<std::string> ConvertSkyImages(const std::filesystem::path &dataDirectory) {
    constexpr int SKY_WIDTH = 32;
    std::error_code error;
    std::vector<std::filesystem::path> images;
    for (const auto &entry : std::filesystem::directory_iterator(dataDirectory / "cargfx", error)) {
        if (Lowercase(entry.path().extension().string()) == ".ci4") {
            images.push_back(entry.path());
        }
    }
    for (const auto &image : images) {
        auto lut = std::filesystem::path(image).replace_extension(".lut4");
        std::ifstream texelFile(image, std::ios::binary);
        std::vector<std::uint8_t> texels(std::istreambuf_iterator<char>(texelFile), {});
        std::ifstream paletteFile(lut, std::ios::binary);
        std::vector<std::uint8_t> palette(std::istreambuf_iterator<char>(paletteFile), {});
        if (palette.size() != 32 || texels.empty() || texels.size() % (SKY_WIDTH / 2) != 0) {
            return std::format("{} and {} are not a sky image.", image.string(), lut.string());
        }
        auto png = std::filesystem::path(image).replace_extension(".png");
        int height = static_cast<int>(texels.size() / (SKY_WIDTH / 2));
        if (!coastline::SaveIndexed4Png(
                png, texels, std::span<const std::uint8_t, 32>(palette), SKY_WIDTH, height)) {
            return std::format("{} does not read back as {}.", png.string(), image.string());
        }
        std::filesystem::remove(image, error);
        std::filesystem::remove(lut, error);
    }
    return std::nullopt;
}

// The strings of BRString.dll as strings/default/pc.json, by identifier, in place of the library.
std::optional<std::string> ConvertStringLibrary(const std::filesystem::path &library,
                                                const std::filesystem::path &dataDirectory) {
    std::ifstream in(library, std::ios::binary);
    std::vector<std::uint8_t> image(std::istreambuf_iterator<char>(in), {});
    auto strings = coastline::ReadLibraryStrings(image);
    if (strings.empty()) {
        return std::format("{} has no strings.", library.string());
    }
    nlohmann::ordered_json json = nlohmann::ordered_json::object();
    for (const auto &[id, text] : strings) {
        json[std::to_string(id)] = text;
    }
    auto destination = dataDirectory / "strings" / "default" / "pc.json";
    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    std::ofstream out(destination, std::ios::trunc);
    out << json.dump(2) << '\n';
    if (!out) {
        return std::format("{} cannot be written.", destination.string());
    }
    std::filesystem::remove(library, error);
    return std::nullopt;
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
    const Release &known = *std::ranges::find(RELEASES, *release, &Release::release);
    auto destination = dataDirectory / "n64" / known.directory;
    if (auto failure = ExtractCartridge(*bytes, known.manifest, destination)) {
        return std::unexpected(
            std::format("{} cannot be imported ({}).", image.string(), *failure));
    }
    // Replaces the model.bin of each car with model.gltf, its buffer mesh.bin, and its images.
    std::error_code carsError;
    for (const auto &car : std::filesystem::directory_iterator(destination / "cars", carsError)) {
        auto model = car.path() / "model.bin";
        if (!std::filesystem::exists(model)) {
            continue;
        }
        auto convert = [](std::span<const std::uint8_t> bytes, std::string_view bufferUri) {
            return CarModelToGltf(bytes, 0, bufferUri);
        };
        if (auto failure =
                ConvertModel(model, convert, car.path() / "model.gltf", car.path() / "mesh.bin")) {
            return std::unexpected(*failure);
        }
    }
    // The geometry of each track and the textures after it are the layout of a .trk file of the
    // PC release, whose first word is the size of the geometry. The two are joined and replaced
    // with track.gltf, its buffer track.bin, and its images.
    for (const auto &track :
         std::filesystem::directory_iterator(destination / "tracks", carsError)) {
        auto geometry = ReadWholeFile(track.path() / "geometry.bin");
        auto textures = ReadWholeFile(track.path() / "textures.bin");
        if (!geometry || !textures) {
            continue;
        }
        if (geometry->size() < 4 ||
            (static_cast<std::size_t>((*geometry)[0]) << 24 | (*geometry)[1] << 16 |
             (*geometry)[2] << 8 | (*geometry)[3]) != geometry->size()) {
            return std::unexpected(std::format("{} does not begin with its size.",
                                               (track.path() / "geometry.bin").string()));
        }
        auto joined = track.path() / "track.trk";
        geometry->insert(geometry->end(), textures->begin(), textures->end());
        if (!WriteBytes(joined, *geometry)) {
            return std::unexpected(std::format("{} cannot be written.", joined.string()));
        }
        if (auto failure = ConvertModel(
                joined, TrackToGltf, track.path() / "track.gltf", track.path() / "track.bin")) {
            return std::unexpected(*failure);
        }
        std::filesystem::remove(track.path() / "geometry.bin", carsError);
        std::filesystem::remove(track.path() / "textures.bin", carsError);
    }
    std::error_code error;
    if (*release == RomRelease::Usa) {
        // The XM modules of the menus and of the tracks, as the music pack default.
        for (const UsaMusicModule &entry : USA_MUSIC_MODULES) {
            auto module = Tools::Common::decompress(std::span(*bytes).subspan(entry.offset));
            if (!module) {
                return std::unexpected(module.error().message);
            }
            auto path = dataDirectory / "music" / "default" / (std::string(entry.name) + ".xm");
            std::filesystem::create_directories(path.parent_path(), error);
            std::ofstream xm(path, std::ios::binary | std::ios::trunc);
            xm.write(reinterpret_cast<const char *>(module->data()),
                     static_cast<std::streamsize>(module->size()));
            if (!xm) {
                return std::unexpected(std::format("{} cannot be written.", path.string()));
            }
        }
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
    if (Lowercase(disc.extension().string()) == ".cue") {
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
        std::string name = Lowercase(entry.path.filename().string());
        std::optional<std::string> failure;
        if (name == "brstring.dll") {
            failure = ConvertStringLibrary(dataDirectory / entry.path, dataDirectory);
        } else if (name.ends_with(".bmp")) {
            failure = ConvertBitmap(dataDirectory / entry.path);
        }
        if (failure) {
            return std::unexpected(*failure);
        }
        ++written;
    }
    if (auto failure = ConvertSkyImages(dataDirectory)) {
        return std::unexpected(*failure);
    }
    if (auto failure = ConvertGhostRecordings(dataDirectory)) {
        return std::unexpected(*failure);
    }
    if (auto failure = ConvertCarModels(dataDirectory)) {
        return std::unexpected(*failure);
    }
    if (auto failure = ConvertTracks(dataDirectory)) {
        return std::unexpected(*failure);
    }
    for (const auto &track : tracks) {
        if (!track.audio) {
            continue;
        }
        auto name = std::format("track{:02}.flac", track.number);
        if (progress) {
            progress("music/default/" + name, static_cast<double>(written) / steps);
        }
        std::ifstream audio(track.file, std::ios::binary);
        if (auto failure =
                EncodeAudioTrack(audio, track, dataDirectory / "music" / "default" / name)) {
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
