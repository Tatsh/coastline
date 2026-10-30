#include "importer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <sstream>

#include <FLAC/stream_encoder.h>
#include <SDL3/SDL.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <zlib.h>

#include "assetchain.h"
#include "cartridge/eu.h"
#include "cartridge/jp.h"
#include "cartridge/us.h"
#include "common/ghost.h"
#include "common/podarchive.h"
#include "common/rigidbody.h"
#include "common/weather.h"
#include "sdl/ghostfile.h"
#include "sdl/indexedimage.h"
#include "sdl/n64attract.h"
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
        return fmt::format("{} cannot be written", (directory / "segment.bin").string());
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
            return fmt::format("the manifest does not fit the image at '{}'", line);
        }
        position = offset + size;
        auto bytes = rom.subspan(offset, size);
        if (name == "padding") {
            if (std::ranges::any_of(bytes, [](std::uint8_t byte) { return byte != 0; })) {
                return fmt::format("the padding at {:#x} is not zero", offset);
            }
            continue;
        }
        std::vector<std::uint8_t> inflated;
        if (kind == "zlib") {
            auto data = Tools::Common::decompress(bytes);
            if (size < 4 || (static_cast<std::size_t>(bytes[0]) << 24 | bytes[1] << 16 |
                             bytes[2] << 8 | bytes[3]) != size) {
                return fmt::format("{} at {:#x} is not an asset of {} bytes", name, offset, size);
            }
            if (!data) {
                return fmt::format("{} at {:#x} does not inflate", name, offset);
            }
            inflated = std::move(*data);
            bytes = inflated;
        }
        auto path = directory / (name + ".bin");
        std::filesystem::create_directories(path.parent_path(), error);
        if (!WriteBytes(path, bytes)) {
            return fmt::format("{} cannot be written", path.string());
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
        return fmt::format("{} cannot be written.", destination.string());
    }
    std::vector<std::uint8_t> chunk;
    constexpr std::uint32_t CHUNK_SECTORS = 256;
    std::uint32_t remaining = entry.size;
    for (std::uint32_t sector = 0; remaining > 0; sector += CHUNK_SECTORS) {
        std::uint32_t bytes = std::min(remaining, CHUNK_SECTORS * DATA_SECTOR);
        if (!reader.Read(entry.extent + sector, (bytes + DATA_SECTOR - 1) / DATA_SECTOR, chunk)) {
            return fmt::format("{} is cut short on the disc.", entry.path.string());
        }
        out.write(reinterpret_cast<const char *>(chunk.data()), bytes);
        remaining -= bytes;
    }
    return out ? std::nullopt :
                 std::optional(fmt::format("{} was not written.", destination.string()));
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
        return fmt::format("{} cannot be read: {}", bitmap.string(), SDL_GetError());
    }
    if (!SDL_SavePNG(original.get(), png.string().c_str())) {
        return fmt::format("{} cannot be written: {}", png.string(), SDL_GetError());
    }
    Surface converted(SDL_LoadPNG(png.string().c_str()));
    if (!converted || !SamePixels(original.get(), converted.get())) {
        return fmt::format("{} does not read back as {}.", png.string(), bitmap.string());
    }
    std::error_code error;
    std::filesystem::remove(bitmap, error);
    return std::nullopt;
}

// A sound of the console, signed 8-bit samples, as a WAV file of the same name, in place of the
// samples once the file reads back as them. The mixer of the sound effects writes each sample to
// two frames of the 21998 Hz output, so a sound plays at half that rate.
std::optional<std::string> ConvertConsoleSound(const std::filesystem::path &sound) {
    constexpr std::uint32_t SOUND_RATE = 21998 / 2;
    auto samples = ReadWholeFile(sound);
    if (!samples) {
        return fmt::format("{} cannot be read.", sound.string());
    }
    auto wav = std::filesystem::path(sound).replace_extension(".wav");
    std::vector<std::uint8_t> file;
    auto put = [&file](std::uint32_t value, int bytes) {
        for (int i = 0; i < bytes; ++i) {
            file.push_back(static_cast<std::uint8_t>(value >> (i * 8)));
        }
    };
    auto size = static_cast<std::uint32_t>(samples->size());
    file.insert(file.end(), {'R', 'I', 'F', 'F'});
    put(36 + size, 4);
    file.insert(file.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    put(16, 4);
    put(1, 2); // PCM
    put(1, 2); // One channel.
    put(SOUND_RATE, 4);
    put(SOUND_RATE, 4);
    put(1, 2);
    put(8, 2);
    file.insert(file.end(), {'d', 'a', 't', 'a'});
    put(size, 4);
    // A WAV file of 8 bits holds unsigned samples about 128.
    for (std::uint8_t sample : *samples) {
        file.push_back(static_cast<std::uint8_t>(sample ^ 0x80));
    }
    if (size % 2 != 0) {
        file.push_back(0);
    }
    if (!WriteBytes(wav, file)) {
        return fmt::format("{} cannot be written.", wav.string());
    }
    SDL_AudioSpec spec;
    Uint8 *read = nullptr;
    Uint32 length = 0;
    bool same = SDL_LoadWAV(wav.string().c_str(), &spec, &read, &length) &&
                spec.format == SDL_AUDIO_U8 && spec.channels == 1 &&
                spec.freq == static_cast<int>(SOUND_RATE) && length == size &&
                std::ranges::equal(std::span(read, length), *samples, {}, [](std::uint8_t value) {
                    return static_cast<std::uint8_t>(value ^ 0x80);
                });
    SDL_free(read);
    if (!same) {
        return fmt::format("{} does not read back as {}.", wav.string(), sound.string());
    }
    std::error_code error;
    std::filesystem::remove(sound, error);
    return std::nullopt;
}

// The addresses of the fonts of a release in its main segment, which starts at 0x80200000.
struct FontAddresses {
    std::uint32_t largeAtlas;
    std::uint32_t smallAtlas;
    std::uint32_t largeGradient;
    std::uint32_t largeShadowGradient;
    std::uint32_t smallGradient;
    std::uint32_t smallShadowGradient;
    std::uint32_t charToGlyph;
    std::uint32_t largeOffsets;
    std::uint32_t smallOffsets;
};

constexpr FontAddresses USA_FONTS{0x8028FE00,
                                  0x8029DA00,
                                  0x8028BDF0,
                                  0x8028BF30,
                                  0x8028C070,
                                  0x8028C1B0,
                                  0x802A1740,
                                  0x802A17A0,
                                  0x802A187C};
constexpr FontAddresses EUR_FONTS{0x802925C0,
                                  0x802A01C0,
                                  0x8028E550,
                                  0x8028E690,
                                  0x8028E7D0,
                                  0x8028E910,
                                  0x802A3F00,
                                  0x802A3F60,
                                  0x802A403C};
constexpr FontAddresses JPN_FONTS{0x80292060,
                                  0x8029FC60,
                                  0x8028DFA0,
                                  0x8028E0E0,
                                  0x8028E220,
                                  0x8028E360,
                                  0x802A39A0,
                                  0x802A3A00,
                                  0x802A3ADC};

// The two fonts of the menus as PNG images of their glyphs and of the gradients that colour them,
// and fonts.json with the layout of the glyphs. The glyphs are 4-bit intensity and alpha, two
// rows of cells, and each gradient is 8 texels across and 40 down.
std::optional<std::string> ConvertConsoleFonts(const std::filesystem::path &destination,
                                               RomRelease release) {
    constexpr std::uint32_t SEGMENT_BASE = 0x80200000;
    constexpr int LARGE_WIDTH = 704;
    constexpr int LARGE_CELL = 40;
    constexpr int SMALL_WIDTH = 392;
    constexpr int SMALL_CELL = 20;
    constexpr int GRADIENT_WIDTH = 8;
    constexpr int GRADIENT_HEIGHT = 40;
    constexpr std::size_t OFFSETS = 55;
    constexpr std::size_t CHARACTERS = 96;
    const FontAddresses &fonts = release == RomRelease::Usa ? USA_FONTS :
                                 release == RomRelease::Eur ? EUR_FONTS :
                                                              JPN_FONTS;
    auto segment = ReadWholeFile(destination / "segment.bin");
    if (!segment) {
        return fmt::format("{} cannot be read.", (destination / "segment.bin").string());
    }
    auto bytesAt = [&](std::uint32_t address,
                       std::size_t size) -> std::optional<std::span<const std::uint8_t>> {
        if (address < SEGMENT_BASE || address - SEGMENT_BASE > segment->size() ||
            size > segment->size() - (address - SEGMENT_BASE)) {
            return std::nullopt;
        }
        return std::span<const std::uint8_t>(*segment).subspan(address - SEGMENT_BASE, size);
    };
    auto fontsDirectory = destination / "fonts";
    std::error_code error;
    std::filesystem::create_directories(fontsDirectory, error);
    auto writeImage = [&](std::string_view name,
                          std::uint32_t address,
                          int width,
                          int height) -> std::optional<std::string> {
        auto texels = bytesAt(address, static_cast<std::size_t>(width) * height);
        if (!texels) {
            return fmt::format("The texels of {} are outside the main segment.", name);
        }
        // The console holds the rows from the bottom up, and draws them with a negative step.
        std::vector<std::uint8_t> rgba;
        rgba.reserve(texels->size() * 4);
        for (int row = height - 1; row >= 0; --row) {
            for (int column = 0; column < width; ++column) {
                std::uint8_t texel = (*texels)[static_cast<std::size_t>(row) * width + column];
                auto intensity = static_cast<std::uint8_t>((texel >> 4) * 17);
                rgba.insert(rgba.end(),
                            {intensity,
                             intensity,
                             intensity,
                             static_cast<std::uint8_t>((texel & 0xf) * 17)});
            }
        }
        auto path = fontsDirectory / (std::string(name) + ".png");
        if (!SaveRgba8Png(path, rgba, width, height)) {
            return fmt::format("{} does not read back as its texels.", path.string());
        }
        return std::nullopt;
    };
    for (auto [name, address, width, height] :
         {std::tuple{"large", fonts.largeAtlas, LARGE_WIDTH, 2 * LARGE_CELL},
          {"small", fonts.smallAtlas, SMALL_WIDTH, 2 * SMALL_CELL},
          {"large-gradient", fonts.largeGradient, GRADIENT_WIDTH, GRADIENT_HEIGHT},
          {"large-shadow-gradient", fonts.largeShadowGradient, GRADIENT_WIDTH, GRADIENT_HEIGHT},
          {"small-gradient", fonts.smallGradient, GRADIENT_WIDTH, GRADIENT_HEIGHT},
          {"small-shadow-gradient", fonts.smallShadowGradient, GRADIENT_WIDTH, GRADIENT_HEIGHT}}) {
        if (auto failure = writeImage(name, address, width, height)) {
            return failure;
        }
    }
    auto words = [&](std::uint32_t address) -> std::optional<nlohmann::ordered_json> {
        auto bytes = bytesAt(address, OFFSETS * 4);
        if (!bytes) {
            return std::nullopt;
        }
        nlohmann::ordered_json list = nlohmann::ordered_json::array();
        for (std::size_t i = 0; i < OFFSETS; ++i) {
            list.push_back(
                static_cast<std::int32_t>((*bytes)[i * 4] << 24 | (*bytes)[i * 4 + 1] << 16 |
                                          (*bytes)[i * 4 + 2] << 8 | (*bytes)[i * 4 + 3]));
        }
        return list;
    };
    auto glyphs = bytesAt(fonts.charToGlyph, CHARACTERS);
    auto largeOffsets = words(fonts.largeOffsets);
    auto smallOffsets = words(fonts.smallOffsets);
    if (!glyphs || !largeOffsets || !smallOffsets) {
        return std::string("The glyph tables of the fonts are outside the main segment.");
    }
    // The glyph of each character from '!', and where each glyph starts in its row. The images are
    // upright, and the first row of glyphs is the upper row of cells.
    nlohmann::ordered_json layout{
        {"firstCharacter", "!"},
        {"glyphOfCharacter", std::vector<int>(glyphs->begin(), glyphs->end())},
        {"firstRowGlyphs", 27},
        {"large",
         {{"image", "large.png"},
          {"cell", LARGE_CELL},
          {"padding", 7},
          {"gradient", "large-gradient.png"},
          {"shadowGradient", "large-shadow-gradient.png"},
          {"glyphStarts", *largeOffsets}}},
        {"small",
         {{"image", "small.png"},
          {"cell", SMALL_CELL},
          {"padding", 4},
          {"gradient", "small-gradient.png"},
          {"shadowGradient", "small-shadow-gradient.png"},
          {"glyphStarts", *smallOffsets}}}};
    std::ofstream(fontsDirectory / "fonts.json", std::ios::trunc) << layout.dump(1) << '\n';
    return std::nullopt;
}

// The addresses of the lights of the splash screens and the menus of a release: an ambient light
// of 8 bytes and a directional light after it.
struct LightAddresses {
    std::uint32_t splash;
    std::uint32_t menu;
};

constexpr LightAddresses USA_LIGHTS{0x80270820, 0x80271F88};
constexpr LightAddresses EUR_LIGHTS{0x80272800, 0x80273F68};
constexpr LightAddresses JPN_LIGHTS{0x802722C8, 0x80273A28};

// The lights of the splash screens and the menus as lights.json: the colour of each ambient light,
// and the colour and the direction toward each directional light, in the space of the console.
std::optional<std::string> ConvertConsoleLights(const std::filesystem::path &destination,
                                                RomRelease release) {
    constexpr std::uint32_t SEGMENT_BASE = 0x80200000;
    constexpr std::size_t AMBIENT_AND_DIRECTIONAL = 8 + 16;
    const LightAddresses &lights = release == RomRelease::Usa ? USA_LIGHTS :
                                   release == RomRelease::Eur ? EUR_LIGHTS :
                                                                JPN_LIGHTS;
    auto segment = ReadWholeFile(destination / "segment.bin");
    if (!segment) {
        return fmt::format("{} cannot be read.", (destination / "segment.bin").string());
    }
    nlohmann::ordered_json json = nlohmann::ordered_json::object();
    for (auto [name, address] :
         {std::pair{"splash", lights.splash}, std::pair{"menu", lights.menu}}) {
        std::size_t at = address - SEGMENT_BASE;
        if (address < SEGMENT_BASE || at + AMBIENT_AND_DIRECTIONAL > segment->size()) {
            return fmt::format("The lights of the {} are outside the main segment.", name);
        }
        const std::uint8_t *bytes = segment->data() + at;
        json[name] = {{"ambient", {bytes[0], bytes[1], bytes[2]}},
                      {"directional", {bytes[8], bytes[9], bytes[10]}},
                      {"direction",
                       {static_cast<std::int8_t>(bytes[16]),
                        static_cast<std::int8_t>(bytes[17]),
                        static_cast<std::int8_t>(bytes[18])}}};
    }
    std::ofstream(destination / "lights.json", std::ios::trunc) << json.dump(1) << '\n';
    return std::nullopt;
}

std::uint32_t Big32(const std::uint8_t *bytes) {
    return static_cast<std::uint32_t>(bytes[0]) << 24 | bytes[1] << 16 | bytes[2] << 8 | bytes[3];
}

// The offset and the name of an asset of the manifest of a cartridge, and the offset of the next
// asset after any padding.
struct ManifestAsset {
    std::uint32_t romStart;
    std::uint32_t romEnd;
    std::string name;
};

std::vector<ManifestAsset> ManifestAssets(std::span<const unsigned char> manifest) {
    std::vector<ManifestAsset> assets;
    std::istringstream lines(std::string(manifest.begin(), manifest.end()));
    std::string line;
    while (std::getline(lines, line)) {
        std::istringstream fields(line);
        std::uint32_t offset = 0;
        std::uint32_t size = 0;
        std::string kind;
        std::string name;
        if (!(fields >> std::hex >> offset >> std::dec >> size >> kind >> name) ||
            name == "padding") {
            continue;
        }
        if (!assets.empty()) {
            assets.back().romEnd = offset;
        }
        assets.push_back({offset, offset + size, std::move(name)});
    }
    return assets;
}

// The offsets of the 4-aligned records of the main segment whose words at the offset field of the
// record are the start and the end of the asset.
std::vector<std::size_t> FindAssetRecords(std::span<const std::uint8_t> segment,
                                          const ManifestAsset &asset,
                                          std::size_t field,
                                          std::size_t recordSize) {
    std::vector<std::size_t> records;
    for (std::size_t at = field; at + recordSize - field <= segment.size(); at += 4) {
        if (Big32(&segment[at]) == asset.romStart && Big32(&segment[at + 4]) == asset.romEnd) {
            records.push_back(at - field);
        }
    }
    return records;
}

// The lowest bit and the width of a channel of a texel. A width of 0 is an opaque alpha.
struct TexelChannel {
    int shift;
    int bits;
};

// The red, green, blue, and alpha of a texel of a G_IM_FMT_* format and a G_IM_SIZ_* size.
struct TexelLayout {
    std::uint8_t format;
    std::uint8_t size;
    std::array<TexelChannel, 4> channels;
};

constexpr std::array<TexelLayout, 7> TEXEL_LAYOUTS{{
    {0, 2, {{{11, 5}, {6, 5}, {1, 5}, {0, 1}}}},
    {0, 3, {{{24, 8}, {16, 8}, {8, 8}, {0, 8}}}},
    {3, 0, {{{1, 3}, {1, 3}, {1, 3}, {0, 1}}}},
    {3, 1, {{{4, 4}, {4, 4}, {4, 4}, {0, 4}}}},
    {3, 2, {{{8, 8}, {8, 8}, {8, 8}, {0, 8}}}},
    {4, 0, {{{0, 4}, {0, 4}, {0, 4}, {0, 0}}}},
    {4, 1, {{{0, 8}, {0, 8}, {0, 8}, {0, 0}}}},
}};

// A value of the given number of bits scaled to 8 bits by repeating its bits. Shifting the result
// right by 8 less that number restores the value.
std::uint8_t ExpandBits(std::uint32_t value, int bits) {
    std::uint32_t result = 0;
    for (int shift = 8 - bits; shift > -bits; shift -= bits) {
        result |= shift >= 0 ? value << shift : value >> -shift;
    }
    return static_cast<std::uint8_t>(result);
}

// Each texture of the interface in data/ as a PNG image, upright, in place of its texels once the
// image builds the same texels, and data/textures.json with the format, the bits a texel, and the
// dimensions of each. Each texture is found by its Texture descriptor in the main segment. A
// texture without a descriptor or a PNG form remains as its texels.
std::optional<std::string> ConvertConsoleTextures(const std::filesystem::path &destination,
                                                  std::span<const std::uint8_t> segment,
                                                  std::span<const ManifestAsset> assets) {
    constexpr std::size_t DESCRIPTOR = 20;
    constexpr std::array<const char *, 5> FORMATS{"RGBA", "YUV", "CI", "IA", "I"};
    nlohmann::ordered_json json = nlohmann::ordered_json::object();
    for (const ManifestAsset &asset : assets) {
        if (!asset.name.starts_with("data/")) {
            continue;
        }
        auto path = destination / (asset.name + ".bin");
        auto records = FindAssetRecords(segment, asset, 0, DESCRIPTOR);
        auto descriptor = std::ranges::find_if(records, [&](std::size_t at) {
            return Big32(&segment[at + 8]) == 0 && (segment[at + 12] | segment[at + 13]) != 0 &&
                   (segment[at + 14] | segment[at + 15]) != 0 &&
                   segment[at + 16] < FORMATS.size() && segment[at + 17] <= 3 &&
                   segment[at + 18] == 0 && segment[at + 19] == 0;
        });
        if (descriptor == records.end()) {
            SDL_Log("The main segment has no texture descriptor of %s.", path.string().c_str());
            continue;
        }
        const std::uint8_t *fields = &segment[*descriptor];
        int width = fields[12] << 8 | fields[13];
        int height = fields[14] << 8 | fields[15];
        std::uint8_t format = fields[16];
        std::uint8_t size = fields[17];
        int bits = 4 << size;
        json[path.stem().string()] = {
            {"format", FORMATS[format]}, {"size", bits}, {"width", width}, {"height", height}};
        const auto *layout = std::ranges::find_if(TEXEL_LAYOUTS, [&](const TexelLayout &entry) {
            return entry.format == format && entry.size == size;
        });
        auto texels = ReadWholeFile(path);
        if (!texels) {
            return fmt::format("{} cannot be read.", path.string());
        }
        auto count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
        if (layout == TEXEL_LAYOUTS.end() || count == 0 ||
            texels->size() != (count * static_cast<std::size_t>(bits) + 7) / 8) {
            SDL_Log("The %zu bytes of %s %d-bit texels, %dx%d, of %s have no PNG form.",
                    texels->size(),
                    FORMATS[format],
                    bits,
                    width,
                    height,
                    path.string().c_str());
            continue;
        }
        auto bytesPerTexel = static_cast<std::size_t>(bits) / 8;
        auto texelAt = [&](std::size_t index) {
            if (bits == 4) {
                return static_cast<std::uint32_t>((*texels)[index / 2] >> (index % 2 == 0 ? 4 : 0) &
                                                  0xf);
            }
            std::uint32_t value = 0;
            for (std::size_t i = 0; i < bytesPerTexel; ++i) {
                value = value << 8 | (*texels)[index * bytesPerTexel + i];
            }
            return value;
        };
        // The console stores the rows from the bottom up, and draws them with a negative step.
        auto rowOfPixel = [&](std::size_t pixel) {
            return (static_cast<std::size_t>(height) - 1 -
                    pixel / static_cast<std::size_t>(width)) *
                       static_cast<std::size_t>(width) +
                   pixel % static_cast<std::size_t>(width);
        };
        std::vector<std::uint8_t> rgba;
        rgba.reserve(count * 4);
        for (std::size_t pixel = 0; pixel < count; ++pixel) {
            std::uint32_t texel = texelAt(rowOfPixel(pixel));
            for (auto [shift, channelBits] : layout->channels) {
                rgba.push_back(
                    channelBits == 0 ?
                        0xff :
                        ExpandBits(texel >> shift & ((1u << channelBits) - 1), channelBits));
            }
        }
        auto png = std::filesystem::path(path).replace_extension(".png");
        if (!SaveRgba8Png(png, rgba, width, height)) {
            return fmt::format("{} does not read back as its pixels.", png.string());
        }
        std::vector<std::uint8_t> rebuilt(texels->size());
        for (std::size_t pixel = 0; pixel < count; ++pixel) {
            std::uint32_t texel = 0;
            for (std::size_t channel = 0; channel < 4; ++channel) {
                auto [shift, channelBits] = layout->channels[channel];
                if (channelBits != 0) {
                    texel |=
                        static_cast<std::uint32_t>(rgba[pixel * 4 + channel] >> (8 - channelBits))
                        << shift;
                }
            }
            std::size_t index = rowOfPixel(pixel);
            if (bits == 4) {
                rebuilt[index / 2] |= static_cast<std::uint8_t>(texel << (index % 2 == 0 ? 4 : 0));
                continue;
            }
            for (std::size_t i = 0; i < bytesPerTexel; ++i) {
                rebuilt[index * bytesPerTexel + i] =
                    static_cast<std::uint8_t>(texel >> (8 * (bytesPerTexel - 1 - i)));
            }
        }
        std::error_code error;
        if (rebuilt != *texels) {
            SDL_Log(
                "%s does not build the texels of %s.", png.string().c_str(), path.string().c_str());
            std::filesystem::remove(png, error);
            continue;
        }
        std::filesystem::remove(path, error);
    }
    std::ofstream(destination / "data" / "textures.json", std::ios::trunc) << json.dump(2) << '\n';
    return std::nullopt;
}

// Writes the text and the MENU_ITEM_* flags of the MenuItem record of each model of menu-items/ to
// items.json in the directory of the model, by the name of the model.
void WriteMenuItems(const std::filesystem::path &destination,
                    std::span<const std::uint8_t> segment,
                    std::span<const ManifestAsset> assets) {
    constexpr std::uint32_t SEGMENT_BASE = 0x80200000;
    constexpr std::size_t MENU_ITEM = 20;
    std::map<std::filesystem::path, nlohmann::ordered_json> menus;
    for (const ManifestAsset &asset : assets) {
        if (!asset.name.starts_with("menu-items/")) {
            continue;
        }
        auto model = destination / asset.name;
        auto records = FindAssetRecords(segment, asset, 12, MENU_ITEM);
        auto record = std::ranges::find_if(records, [&](std::size_t at) {
            std::uint32_t text = Big32(&segment[at]);
            return Big32(&segment[at + 8]) == 0 && text >= SEGMENT_BASE &&
                   text - SEGMENT_BASE < segment.size() &&
                   std::ranges::find(segment.subspan(text - SEGMENT_BASE), 0) != segment.end();
        });
        if (record == records.end()) {
            SDL_Log("%s has no menu item in the main segment.", asset.name.c_str());
            continue;
        }
        const auto *text =
            reinterpret_cast<const char *>(&segment[Big32(&segment[*record]) - SEGMENT_BASE]);
        menus[model.parent_path()][model.filename().string()] = {
            {"name", std::string(text)}, {"flags", Big32(&segment[*record + 4])}};
    }
    for (const auto &[directory, items] : menus) {
        std::ofstream(directory / "items.json", std::ios::trunc)
            << items.dump(2, ' ', false, nlohmann::ordered_json::error_handler_t::replace) << '\n';
    }
}

// The addresses of the vertices of the choice marker of the menus, drawn by
// g_aMenuChoiceMarkerDisplayList.
constexpr std::uint32_t USA_MENU_CHOICE_MARKER = 0x80271E68;
constexpr std::uint32_t EUR_MENU_CHOICE_MARKER = 0x80273E48;
constexpr std::uint32_t JPN_MENU_CHOICE_MARKER = 0x80273908;

// Writes the position and the colour of each of the 12 vertices of the choice marker of the menus,
// and the triangles of its display list, as data/menu-choice-marker.json.
std::optional<std::string> WriteMenuChoiceMarker(const std::filesystem::path &destination,
                                                 std::span<const std::uint8_t> segment,
                                                 RomRelease release) {
    constexpr std::uint32_t SEGMENT_BASE = 0x80200000;
    constexpr std::size_t VERTICES = 12;
    constexpr std::size_t VERTEX = 16;
    std::uint32_t address = release == RomRelease::Usa ? USA_MENU_CHOICE_MARKER :
                            release == RomRelease::Eur ? EUR_MENU_CHOICE_MARKER :
                                                         JPN_MENU_CHOICE_MARKER;
    if (address - SEGMENT_BASE + VERTICES * VERTEX > segment.size()) {
        return std::string("The choice marker of the menus is outside the main segment.");
    }
    nlohmann::ordered_json vertices = nlohmann::ordered_json::array();
    for (std::size_t i = 0; i < VERTICES; ++i) {
        const std::uint8_t *vertex = &segment[address - SEGMENT_BASE + i * VERTEX];
        auto coordinate = [vertex](int index) {
            return static_cast<std::int16_t>(vertex[index * 2] << 8 | vertex[index * 2 + 1]);
        };
        vertices.push_back({{"position", {coordinate(0), coordinate(1), coordinate(2)}},
                            {"color", {vertex[12], vertex[13], vertex[14], vertex[15]}}});
    }
    nlohmann::ordered_json marker{{"vertices", vertices},
                                  {"triangles", {{0, 1, 2}, {6, 7, 8}, {3, 4, 5}, {9, 10, 11}}}};
    std::ofstream(destination / "data" / "menu-choice-marker.json", std::ios::trunc)
        << marker.dump(2) << '\n';
    return std::nullopt;
}

// The addresses of the tables of the track menu and the championship of each release:
// g_aTrackDescriptors, g_apWeatherMenuItems, g_dwNumTracks, g_dwCarCount, g_aCarModels, and
// g_SeasonTable.
struct TrackMenuAddresses {
    std::uint32_t trackDescriptors;
    std::uint32_t weatherMenuItems;
    std::uint32_t numTracks;
    std::uint32_t carCount;
    std::uint32_t carModels;
    std::uint32_t seasonTable;
};
constexpr TrackMenuAddresses USA_TRACK_MENU{
    0x80270854, 0x802722A4, 0x8028AE04, 0x8028AE08, 0x8028AE0C, 0x8028B944};
constexpr TrackMenuAddresses EUR_TRACK_MENU{
    0x80272834, 0x80274284, 0x8028D0B4, 0x8028D0B8, 0x8028D0BC, 0x8028DBF4};
constexpr TrackMenuAddresses JPN_TRACK_MENU{
    0x802722F4, 0x80273D34, 0x8028CAE4, 0x8028CAE8, 0x8028CAEC, 0x8028D624};

// Writes the items of the tracks on the track menu, the names of the weathers, the cars, and the
// seasons of the championship as data/track-menu.json. Each track gives its name, its MENU_ITEM_
// flags, whether its geometry exists, and the glTF file of its model in the import. Each car gives
// its name and its directory in the import, and each season the indices of its two cars.
std::optional<std::string> WriteTrackMenu(const std::filesystem::path &destination,
                                          std::span<const std::uint8_t> segment,
                                          std::span<const ManifestAsset> assets,
                                          RomRelease release) {
    constexpr std::uint32_t SEGMENT_BASE = 0x80200000;
    constexpr std::uint32_t TRACK_DESCRIPTOR = 0x17C;
    constexpr std::uint32_t TRACK_DESCRIPTORS = 11;
    constexpr std::uint32_t WEATHERS = 5;
    constexpr std::uint32_t CAR_MODEL = 0x60;
    constexpr std::uint32_t SEASON = 0x1C;
    constexpr std::uint32_t SEASONS = 6;
    constexpr std::uint32_t SEASON_RACES = 4;
    const TrackMenuAddresses &at = release == RomRelease::Usa ? USA_TRACK_MENU :
                                   release == RomRelease::Eur ? EUR_TRACK_MENU :
                                                                JPN_TRACK_MENU;
    auto inside = [&](std::uint32_t address, std::uint32_t size) {
        return address >= SEGMENT_BASE && address - SEGMENT_BASE + size <= segment.size();
    };
    auto word = [&](std::uint32_t address) { return Big32(&segment[address - SEGMENT_BASE]); };
    auto text = [&](std::uint32_t address) -> std::optional<std::string> {
        if (!inside(address, 1)) {
            return std::nullopt;
        }
        auto rest = segment.subspan(address - SEGMENT_BASE);
        auto end = std::ranges::find(rest, 0);
        if (end == rest.end()) {
            return std::nullopt;
        }
        return std::string(rest.begin(), end);
    };
    if (!inside(at.trackDescriptors, TRACK_DESCRIPTOR * TRACK_DESCRIPTORS) ||
        !inside(at.weatherMenuItems, 4 * WEATHERS) || !inside(at.numTracks, 4) ||
        !inside(at.carCount, 4) || !inside(at.seasonTable, SEASON * SEASONS)) {
        return std::string("The tables of the track menu are outside the main segment.");
    }
    nlohmann::ordered_json tracks = nlohmann::ordered_json::array();
    for (std::uint32_t i = 0; i < TRACK_DESCRIPTORS; ++i) {
        std::uint32_t entry = at.trackDescriptors + i * TRACK_DESCRIPTOR;
        auto name = text(word(entry));
        std::uint32_t modelStart = word(entry + 0x0C);
        auto asset = std::ranges::find(assets, modelStart, &ManifestAsset::romStart);
        if (!name || asset == assets.end()) {
            return std::format("Track {} of the track menu has no name or no model.", i);
        }
        tracks.push_back({{"name", *name},
                          {"flags", word(entry + 0x04)},
                          {"exists", word(entry + 0x18) != 0},
                          {"model", asset->name + ".gltf"}});
    }
    nlohmann::ordered_json weathers = nlohmann::ordered_json::array();
    for (std::uint32_t i = 0; i < WEATHERS; ++i) {
        std::uint32_t item = word(at.weatherMenuItems + i * 4);
        auto name = inside(item, 4) ? text(word(item)) : std::nullopt;
        if (!name) {
            return std::format("Weather {} of the menus has no name.", i);
        }
        weathers.push_back(*name);
    }
    std::uint32_t carCount = word(at.carCount);
    if (!inside(at.carModels, CAR_MODEL * carCount)) {
        return std::string("The cars are outside the main segment.");
    }
    // The directory of a car in the import is its name in lowercase, each run of other characters
    // a hyphen.
    nlohmann::ordered_json cars = nlohmann::ordered_json::array();
    for (std::uint32_t i = 0; i < carCount; ++i) {
        auto name = text(word(at.carModels + i * CAR_MODEL));
        if (!name) {
            return std::format("Car {} has no name.", i);
        }
        std::string directory;
        for (char c : *name) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                directory += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            } else if (!directory.empty() && directory.back() != '-') {
                directory += '-';
            }
        }
        while (!directory.empty() && directory.back() == '-') {
            directory.pop_back();
        }
        cars.push_back({{"name", *name}, {"directory", "cars/" + directory}});
    }
    nlohmann::ordered_json seasons = nlohmann::ordered_json::array();
    for (std::uint32_t i = 0; i < SEASONS; ++i) {
        std::uint32_t entry = at.seasonTable + i * SEASON;
        const std::uint8_t *bytes = &segment[entry - SEGMENT_BASE];
        auto name = text(word(entry));
        auto subtitle = text(word(entry + 4));
        if (!name || !subtitle || bytes[16] >= cars.size() || bytes[17] >= cars.size()) {
            return std::format("Season {} of the championship is not readable.", i);
        }
        nlohmann::ordered_json races = nlohmann::ordered_json::array();
        for (std::uint32_t race = 0; race < SEASON_RACES; ++race) {
            races.push_back({bytes[20 + race * 2], bytes[21 + race * 2]});
        }
        seasons.push_back({{"name", *name},
                           {"subtitle", *subtitle},
                           {"raceCount", static_cast<std::int32_t>(word(entry + 8))},
                           {"pointsNeeded", static_cast<std::int32_t>(word(entry + 12))},
                           {"cars", {bytes[16], bytes[17]}},
                           {"unlocks", bytes[18] << 8 | bytes[19]},
                           {"races", races}});
    }
    nlohmann::ordered_json menu{{"numTracks", static_cast<std::int32_t>(word(at.numTracks))},
                                {"tracks", tracks},
                                {"weathers", weathers},
                                {"cars", cars},
                                {"seasons", seasons}};
    std::ofstream(destination / "data" / "track-menu.json", std::ios::trunc)
        << menu.dump(2, ' ', false, nlohmann::ordered_json::error_handler_t::replace) << '\n';
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
            return fmt::format("{} does not convert to JSON without loss.", recording.string());
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
            return fmt::format(
                "{} does not read back as {}.", destination.string(), recording.string());
        }
        std::filesystem::remove(recording, error);
    }
    return std::nullopt;
}

using ModelConverter = std::function<std::expected<GltfModel, std::string>(
    std::span<const std::uint8_t> bytes, std::string_view bufferUri)>;

// Writes a model of the console as glTF with its buffer and images, and removes the model once
// the written files build the same bytes. The race engine of the PC release reads the image of a
// model of the PC release itself, and that image is retained.
std::optional<std::string> ConvertModel(const std::filesystem::path &car,
                                        const ModelConverter &convert,
                                        const std::filesystem::path &gltfPath,
                                        const std::filesystem::path &bufferPath,
                                        bool retainSource = false) {
    std::error_code error;
    {
        auto bytes = ReadWholeFile(car);
        if (!bytes) {
            return fmt::format("{} cannot be read.", car.string());
        }
        auto model = convert(*bytes, bufferPath.filename().string());
        if (!model) {
            return fmt::format("{} is not a model ({}).", car.string(), model.error());
        }
        std::ofstream(gltfPath, std::ios::trunc) << model->json.dump(1) << '\n';
        if (!WriteBytes(bufferPath, model->buffer)) {
            return fmt::format("{} cannot be written.", bufferPath.string());
        }
        for (const GltfImage &texture : model->images) {
            auto path = car.parent_path() / texture.uri;
            // A texture the colour of the car paints keeps its indices and its palette.
            bool saved = texture.indices.empty() ?
                             SaveRgba8Png(path, texture.rgba, texture.width, texture.height) :
                             SaveIndexed4Png(path,
                                             texture.indices,
                                             std::span<const std::uint8_t, 32>(
                                                 texture.palette.data(), texture.palette.size()),
                                             texture.width,
                                             texture.height);
            if (!saved) {
                return fmt::format("{} does not read back as its texels.", path.string());
            }
        }
        std::ifstream written(gltfPath);
        auto parsed = nlohmann::ordered_json::parse(written, nullptr, false);
        auto buffer = ReadWholeFile(bufferPath);
        auto rebuilt = parsed.is_discarded() || !buffer ?
                           std::unexpected(std::string("the files cannot be read")) :
                           ModelFromGltf(parsed, *buffer);
        if (!rebuilt || *rebuilt != *bytes) {
            return fmt::format("{} does not read back as {}.", gltfPath.string(), car.string());
        }
        const auto &coverage = model->json["extras"]["coastlineCoverage"];
        SDL_Log("%s draws %zu of %zu triangles with state glTF expresses.",
                gltfPath.filename().string().c_str(),
                coverage["exactTriangles"].get<std::size_t>(),
                coverage["triangles"].get<std::size_t>());
    }
    if (!retainSource) {
        std::filesystem::remove(car, error);
    }
    return std::nullopt;
}

// Writes a glTF file of the same name beside each .rca file of cars, and retains the .rca file
// for the race engine.
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
                                        std::filesystem::path(car).replace_extension(".bin"),
                                        true)) {
            return failure;
        }
    }
    return std::nullopt;
}

// Writes a glTF file of the same name beside each .trk file of tracks, and retains the .trk file
// for the race engine.
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
                                        std::filesystem::path(track).replace_extension(".bin"),
                                        true)) {
            return failure;
        }
    }
    return std::nullopt;
}

// Each sky of cargfx, the 4-bit texels of a .ci4 file and the palette of the .lut4 file of the same
// name, as an indexed PNG image of 32 texels a row, in place of the two files.
// The layout of a POD archive: a header of its signature, version, entry count, and the offset of
// its directory, and an entry of the directory for each file, its offset, size, two attributes, two
// reserved bytes, and its name in capitals.
constexpr std::size_t POD_HEADER_SIZE = 16;
constexpr std::size_t POD_ENTRY_SIZE = 76;
constexpr std::size_t POD_NAME_AT = 12;
constexpr std::size_t POD_NAME_SIZE = 64;

std::uint32_t Little32(std::span<const std::uint8_t> bytes, std::size_t at) {
    return static_cast<std::uint32_t>(bytes[at]) | static_cast<std::uint32_t>(bytes[at + 1]) << 8 |
           static_cast<std::uint32_t>(bytes[at + 2]) << 16 |
           static_cast<std::uint32_t>(bytes[at + 3]) << 24;
}

void PutLittle32(std::vector<std::uint8_t> &bytes, std::size_t at, std::uint32_t value) {
    for (std::size_t i = 0; i < 4; ++i) {
        bytes[at + i] = static_cast<std::uint8_t>(value >> (i * 8));
    }
}

// The file of an entry of the archive in the directory of its import, by its name in lower case,
// and the glTF of a model in place of the model.
std::filesystem::path PodEntryFile(std::string name) {
    name = Lowercase(name);
    if (name.ends_with(".blob")) {
        return std::filesystem::path(name).replace_extension(".gltf");
    }
    return name;
}

// The bytes of an entry from the directory of the import: the model built from its glTF, or the
// file as it is.
std::optional<std::vector<std::uint8_t>> ReadPodEntry(const std::filesystem::path &file) {
    if (file.extension() != ".gltf") {
        return ReadWholeFile(file);
    }
    std::ifstream text(file);
    auto json = nlohmann::ordered_json::parse(text, nullptr, false);
    if (json.is_discarded()) {
        return std::nullopt;
    }
    auto buffer = ReadWholeFile(file.parent_path() / json["buffers"][0]["uri"].get<std::string>());
    if (!buffer) {
        return std::nullopt;
    }
    auto bytes = ModelFromGltf(json, *buffer);
    if (!bytes) {
        return std::nullopt;
    }
    return std::move(*bytes);
}

// BossRally.pod, the archive of the start lights of the PC release, as the directory BossRally of
// its entries. pod.json lists the version and each entry with its attributes and its file, and a
// model becomes glTF. The archive is removed once its entries build it again.
std::optional<std::string> ConvertPodArchive(const std::filesystem::path &dataDirectory) {
    auto archivePath = dataDirectory / "BossRally.pod";
    auto archive = ReadWholeFile(archivePath);
    if (!archive) {
        return std::nullopt;
    }
    const std::vector<std::uint8_t> &bytes = *archive;
    if (bytes.size() < POD_HEADER_SIZE || std::memcmp(bytes.data(), "POD", 3) != 0) {
        return fmt::format("{} is not a POD archive.", archivePath.string());
    }
    std::uint32_t count = Little32(bytes, 8);
    std::uint32_t directory = Little32(bytes, 12);
    if (directory > bytes.size() || count > (bytes.size() - directory) / POD_ENTRY_SIZE) {
        return fmt::format("{} has a directory outside it.", archivePath.string());
    }
    auto output = dataDirectory / "BossRally";
    std::error_code error;
    std::filesystem::create_directories(output, error);
    nlohmann::ordered_json json{{"version", Little32(bytes, 4)},
                                {"entries", nlohmann::ordered_json::array()}};
    for (std::uint32_t i = 0; i < count; ++i) {
        std::size_t at = directory + i * POD_ENTRY_SIZE;
        std::uint32_t offset = Little32(bytes, at);
        std::uint32_t size = Little32(bytes, at + 4);
        if (offset > bytes.size() || size > bytes.size() - offset) {
            return fmt::format("{} has an entry outside it.", archivePath.string());
        }
        std::string name(
            reinterpret_cast<const char *>(&bytes[at + POD_NAME_AT]),
            strnlen(reinterpret_cast<const char *>(&bytes[at + POD_NAME_AT]), POD_NAME_SIZE));
        auto raw = output / Lowercase(name);
        if (!WriteBytes(raw, std::span(bytes).subspan(offset, size))) {
            return fmt::format("{} cannot be written.", raw.string());
        }
        auto file = PodEntryFile(name);
        if (file.extension() == ".gltf") {
            auto stem = file.stem().string();
            auto failure = ConvertModel(
                raw, FrontEndModelToGltf, output / file, output / (stem + "-mesh.bin"));
            if (failure) {
                return failure;
            }
        }
        json["entries"].push_back({{"name", name},
                                   {"file", file.string()},
                                   {"offset", offset},
                                   {"attributes", {bytes[at + 8], bytes[at + 9]}},
                                   {"reserved", {bytes[at + 10], bytes[at + 11]}}});
    }
    json["directory"] = directory;
    json["size"] = bytes.size();

    // The archive built from the entries, with the bytes between them zero.
    std::vector<std::uint8_t> rebuilt(bytes.size(), 0);
    std::memcpy(rebuilt.data(), "POD", 4);
    PutLittle32(rebuilt, 4, json["version"].get<std::uint32_t>());
    PutLittle32(rebuilt, 8, count);
    PutLittle32(rebuilt, 12, directory);
    for (std::size_t i = 0; i < json["entries"].size(); ++i) {
        const auto &entry = json["entries"][i];
        auto data = ReadPodEntry(output / entry["file"].get<std::string>());
        auto offset = entry["offset"].get<std::uint32_t>();
        if (!data || offset > rebuilt.size() || data->size() > rebuilt.size() - offset) {
            return fmt::format("{} does not read back.", archivePath.string());
        }
        std::ranges::copy(*data, rebuilt.begin() + offset);
        std::size_t at = directory + i * POD_ENTRY_SIZE;
        PutLittle32(rebuilt, at, offset);
        PutLittle32(rebuilt, at + 4, static_cast<std::uint32_t>(data->size()));
        rebuilt[at + 8] = entry["attributes"][0].get<std::uint8_t>();
        rebuilt[at + 9] = entry["attributes"][1].get<std::uint8_t>();
        rebuilt[at + 10] = entry["reserved"][0].get<std::uint8_t>();
        rebuilt[at + 11] = entry["reserved"][1].get<std::uint8_t>();
        auto name = entry["name"].get<std::string>();
        std::memcpy(&rebuilt[at + POD_NAME_AT], name.data(), std::min(name.size(), POD_NAME_SIZE));
    }
    if (rebuilt != bytes) {
        // An archive whose entries do not build it again, with other bytes between them, stays.
        return std::nullopt;
    }
    std::ofstream(output / "pod.json", std::ios::trunc) << json.dump(2) << '\n';
    std::filesystem::remove(archivePath, error);
    return std::nullopt;
}

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
            return fmt::format("{} and {} are not a sky image.", image.string(), lut.string());
        }
        auto png = std::filesystem::path(image).replace_extension(".png");
        int height = static_cast<int>(texels.size() / (SKY_WIDTH / 2));
        if (!coastline::SaveIndexed4Png(
                png, texels, std::span<const std::uint8_t, 32>(palette), SKY_WIDTH, height)) {
            return fmt::format("{} does not read back as {}.", png.string(), image.string());
        }
        std::filesystem::remove(image, error);
        std::filesystem::remove(lut, error);
    }
    return std::nullopt;
}

// The strings of BRString.dll as strings/pc.json, by identifier, in place of the library.
std::optional<std::string> ConvertStringLibrary(const std::filesystem::path &library,
                                                const std::filesystem::path &dataDirectory) {
    std::ifstream in(library, std::ios::binary);
    std::vector<std::uint8_t> image(std::istreambuf_iterator<char>(in), {});
    auto strings = coastline::ReadLibraryStrings(image);
    if (strings.empty()) {
        return fmt::format("{} has no strings.", library.string());
    }
    nlohmann::ordered_json json = nlohmann::ordered_json::object();
    for (const auto &[id, text] : strings) {
        json[std::to_string(id)] = text;
    }
    auto destination = dataDirectory / "strings" / "pc.json";
    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    std::ofstream out(destination, std::ios::trunc);
    out << json.dump(2) << '\n';
    if (!out) {
        return fmt::format("{} cannot be written.", destination.string());
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
        return fmt::format("{} cannot be written.", destination.string());
    }
    constexpr std::uint64_t CHUNK_SECTORS = 75;
    std::vector<std::uint8_t> raw(CHUNK_SECTORS * RAW_SECTOR);
    std::vector<FLAC__int32> samples(CHUNK_SECTORS * SAMPLES_PER_SECTOR * 2);
    file.seekg(static_cast<std::streamoff>(track.firstSector * RAW_SECTOR));
    for (std::uint64_t done = 0; done < track.sectorCount; done += CHUNK_SECTORS) {
        std::uint64_t count = std::min(CHUNK_SECTORS, track.sectorCount - done);
        if (!file.read(reinterpret_cast<char *>(raw.data()),
                       static_cast<std::streamsize>(count * RAW_SECTOR))) {
            return fmt::format("Track {} is cut short in the image.", track.number);
        }
        // CD audio is 16-bit little-endian stereo.
        std::size_t values = count * RAW_SECTOR / 2;
        for (std::size_t i = 0; i < values; ++i) {
            samples[i] = static_cast<std::int16_t>(raw[i * 2] | (raw[i * 2 + 1] << 8));
        }
        if (!FLAC__stream_encoder_process_interleaved(
                encoder.get(), samples.data(), static_cast<unsigned>(values / 2))) {
            return fmt::format("Track {} was not encoded.", track.number);
        }
    }
    if (!FLAC__stream_encoder_finish(encoder.get())) {
        return fmt::format("Track {} was not finished.", track.number);
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

// The steps a second of the race of the PC release. It plays a frame of a ghost each step.
constexpr double GHOST_STEPS_PER_SECOND = 30.0;
// A move between two steps of a demo longer than this is a cut of its camera to another part of
// the track, and the two steps are not blended.
constexpr float DEMO_CUT_DISTANCE = 20.0f;
// The bits of the buttons of an input that brake.
constexpr std::uint32_t BRAKE_BUTTONS = 0xC0000;

// The pose and the controls of the PC release a step of a demo of the console gives, as
// BuildVehicleKeyframe() takes them from a vehicle, with the wheels in the order 0, 2, 1, and 3.
KeyframeState DemoKeyframe(const N64DemoFrame &frame) {
    constexpr std::array<int, 4> WHEELS{0, 2, 1, 3};
    KeyframeState state{};
    f32 matrix[4][4];
    std::memcpy(matrix, frame.matrix, sizeof(matrix));
    MatrixToQuaternion(matrix, &state.orientation);
    state.position = {frame.matrix[3][0], frame.matrix[3][1], frame.matrix[3][2]};
    state.fBodyRollAngle = frame.bodyRoll;
    state.fSteerAngle = frame.steerAngle;
    for (std::size_t i = 0; i < WHEELS.size(); ++i) {
        auto wheel = static_cast<std::size_t>(WHEELS[i]);
        state.afWheelAngle[i] = frame.wheelRoll[wheel];
        state.afWheelContact[i] = static_cast<float>(frame.wheelContact[wheel]);
        state.afWheelSurface[i] =
            static_cast<float>(static_cast<std::int8_t>(frame.wheelSurface[wheel]));
    }
    state.fGearControl = frame.skidding;
    state.fBraking = (frame.buttons & BRAKE_BUTTONS) != 0 ? 1.0f : 0.0f;
    state.fReverse = !(frame.throttle >= 0.0f) ? 1.0f : 0.0f;
    state.fProgress = frame.distance;
    state.fEngineRpm = frame.engineRpm;
    state.afLight[0] = frame.vehicleImpactVolume;
    state.afLight[1] = frame.impactVolume;
    state.afEffectFlags[0] = frame.landed;
    state.afEffectFlags[1] = frame.effect;
    return state;
}

// The steps of a demo at the rate of the race of the PC release. The poses between two steps of
// the console are blended, and the wheels, the flags, and the sounds are those of the nearer step.
std::vector<KeyframeState> DemoKeyframes(const N64Demo &demo) {
    std::vector<KeyframeState> steps;
    for (const N64DemoFrame &frame : demo.frames) {
        steps.push_back(DemoKeyframe(frame));
    }
    double rate = 1.0 / demo.stepSeconds;
    if (steps.empty() || std::abs(rate - GHOST_STEPS_PER_SECOND) < 0.01) {
        return steps;
    }
    auto count = static_cast<std::size_t>(static_cast<double>(steps.size() - 1) / rate *
                                          GHOST_STEPS_PER_SECOND) +
                 1;
    std::vector<KeyframeState> resampled;
    for (std::size_t k = 0; k < count; ++k) {
        double at = static_cast<double>(k) * rate / GHOST_STEPS_PER_SECOND;
        auto i = std::min(static_cast<std::size_t>(at), steps.size() - 1);
        auto share = static_cast<float>(at - static_cast<double>(i));
        if (i + 1 >= steps.size() || share <= 0.0f) {
            resampled.push_back(steps[i]);
            continue;
        }
        KeyframeState &from = steps[i];
        KeyframeState &to = steps[i + 1];
        float dx = to.position.x - from.position.x;
        float dy = to.position.y - from.position.y;
        float dz = to.position.z - from.position.z;
        const KeyframeState &nearer = share < 0.5f ? from : to;
        if (dx * dx + dy * dy + dz * dz > DEMO_CUT_DISTANCE * DEMO_CUT_DISTANCE) {
            resampled.push_back(nearer);
            continue;
        }
        KeyframeState blended = nearer;
        InterpolateKeyframeState(&blended, share, &from, &to);
        std::memcpy(blended.afWheelAngle, nearer.afWheelAngle, sizeof(blended.afWheelAngle));
        std::memcpy(blended.afWheelContact, nearer.afWheelContact, sizeof(blended.afWheelContact));
        std::memcpy(blended.afWheelSurface, nearer.afWheelSurface, sizeof(blended.afWheelSurface));
        std::memcpy(blended.afLight, nearer.afLight, sizeof(blended.afLight));
        std::memcpy(blended.afEffectFlags, nearer.afEffectFlags, sizeof(blended.afEffectFlags));
        blended.fGearControl = nearer.fGearControl;
        blended.fBraking = nearer.fBraking;
        blended.fReverse = nearer.fReverse;
        resampled.push_back(blended);
    }
    return resampled;
}

void AppendLittle32(std::vector<std::uint8_t> &bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

// A demo of the console as a ghost recording of the PC release, in fog as the console races it.
std::vector<std::uint8_t> DemoGhost(const N64Demo &demo, std::string_view slotName) {
    constexpr std::size_t SLOT_NAME_SIZE = 128;
    constexpr std::size_t RESERVED_BYTES = 8;
    std::vector<std::uint8_t> body{static_cast<std::uint8_t>(demo.track),
                                   static_cast<std::uint8_t>(demo.car),
                                   demo.options[0],
                                   demo.options[1],
                                   demo.options[2],
                                   demo.options[3],
                                   demo.options[4],
                                   WEATHER_FOG};
    body.resize(body.size() + RESERVED_BYTES, 0);
    for (const KeyframeState &state : DemoKeyframes(demo)) {
        std::array<std::uint8_t, GHOST_FRAME_SIZE> frame{};
        PackKeyframeToCompactBuffer(frame.data(), &state);
        body.insert(body.end(), frame.begin(), frame.end());
    }
    uLong checksum = adler32(0, nullptr, 0);
    checksum = adler32(checksum, body.data(), static_cast<uInt>(body.size()));
    std::vector<std::uint8_t> bytes{'R', 'G', 'h', 'o'};
    AppendLittle32(bytes, static_cast<std::uint32_t>(body.size() + 4));
    AppendLittle32(bytes, static_cast<std::uint32_t>(checksum));
    bytes.insert(bytes.end(), body.begin(), body.end());
    // The settings of the menus: the transmission, the tyres, the suspension, the car, the
    // handling, and one lap.
    for (std::uint32_t setting : {static_cast<std::uint32_t>(demo.options[1]),
                                  static_cast<std::uint32_t>(demo.options[2]),
                                  static_cast<std::uint32_t>(demo.options[3]),
                                  static_cast<std::uint32_t>(demo.car),
                                  static_cast<std::uint32_t>(demo.options[0]),
                                  1u}) {
        AppendLittle32(bytes, setting);
    }
    std::vector<std::uint8_t> slot(slotName.begin(), slotName.end());
    slot.resize(SLOT_NAME_SIZE, 0);
    bytes.insert(bytes.end(), slot.begin(), slot.end());
    return bytes;
}

// Record the attract demo of a cartridge and write it as attract.json, the JSON of a ghost
// recording, once it reads back as the same recording.
std::optional<std::string> WriteAttractDemo(std::span<const std::uint8_t> rom,
                                            RomRelease release,
                                            const std::filesystem::path &destination) {
    N64DemoRelease demoRelease = release == RomRelease::Usa ? N64DemoRelease::Usa :
                                 release == RomRelease::Eur ? N64DemoRelease::Eur :
                                                              N64DemoRelease::Jpn;
    auto demo = RecordN64AttractDemo(rom, demoRelease);
    if (!demo) {
        return fmt::format("The attract demo was not recorded: {}", demo.error());
    }
    auto bytes = DemoGhost(*demo, fmt::format("n64 {}", destination.filename().string()));
    auto json = GhostToJson(bytes);
    auto rebuilt = json ? GhostFromJson(*json) : std::nullopt;
    if (!rebuilt || *rebuilt != bytes) {
        return "The attract demo does not convert to JSON without loss.";
    }
    auto path = destination / "attract.json";
    std::ofstream out(path, std::ios::trunc);
    out << FormatGhostJson(*json);
    out.close();
    std::ifstream written(path);
    auto parsed = nlohmann::ordered_json::parse(written, nullptr, false);
    auto reread = parsed.is_discarded() ? std::nullopt : GhostFromJson(parsed);
    if (!reread || *reread != bytes) {
        return fmt::format("{} does not read back as the attract demo.", path.string());
    }
    SDL_Log("The attract demo of %s has %zu steps of the console, from %llu instructions.",
            destination.filename().string().c_str(),
            demo->frames.size(),
            static_cast<unsigned long long>(demo->instructions));
    return std::nullopt;
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
    return std::unexpected(fmt::format(
        "The image is not an unmodified release of Top Gear Rally (SHA-1 {}).", digest));
}

std::expected<RomRelease, std::string> ImportRom(const std::filesystem::path &image,
                                                 const std::filesystem::path &dataDirectory) {
    auto bytes = ReadWholeFile(image);
    if (!bytes) {
        return std::unexpected(fmt::format("{} cannot be read.", image.string()));
    }
    if (!NormalizeRomByteOrder(*bytes)) {
        return std::unexpected(fmt::format("{} is not a Nintendo 64 image.", image.string()));
    }
    auto release = IdentifyRom(*bytes);
    if (!release) {
        return release;
    }
    const Release &known = *std::ranges::find(RELEASES, *release, &Release::release);
    auto destination = dataDirectory / "n64" / known.directory;
    if (auto failure = ExtractCartridge(*bytes, known.manifest, destination)) {
        return std::unexpected(
            fmt::format("{} cannot be imported ({}).", image.string(), *failure));
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
            return std::unexpected(fmt::format("{} does not begin with its size.",
                                               (track.path() / "geometry.bin").string()));
        }
        auto joined = track.path() / "track.trk";
        geometry->insert(geometry->end(), textures->begin(), textures->end());
        if (!WriteBytes(joined, *geometry)) {
            return std::unexpected(fmt::format("{} cannot be written.", joined.string()));
        }
        if (auto failure = ConvertModel(
                joined, TrackToGltf, track.path() / "track.gltf", track.path() / "track.bin")) {
            return std::unexpected(*failure);
        }
        std::filesystem::remove(track.path() / "geometry.bin", carsError);
        std::filesystem::remove(track.path() / "textures.bin", carsError);
    }
    // Replaces each model of the splash screens with a glTF file of the same name.
    std::vector<std::filesystem::path> splashModels;
    std::vector<std::filesystem::path> sounds;
    for (std::string_view name : {"splash_developer_logo",
                                  "splash_first_logo",
                                  "splash_producer_logo",
                                  "splash_title_a",
                                  "splash_title_b",
                                  "splash_title_c",
                                  "splash_title_d"}) {
        // The Japanese release has no producer logo, and draws its title from a texture.
        if (auto model = destination / "constants" / (std::string(name) + ".bin");
            std::filesystem::exists(model)) {
            splashModels.push_back(model);
        }
    }
    // The start lights of the races, a model of the same format.
    if (auto lights = destination / "segments" / "startLights.bin";
        std::filesystem::exists(lights)) {
        splashModels.push_back(lights);
    }
    for (std::string_view name : {"constants/splash_sound_a",
                                  "constants/splash_sound_b",
                                  "segments/scrollSound",
                                  "segments/selectSound"}) {
        if (auto sound = destination / (std::string(name) + ".bin");
            std::filesystem::exists(sound)) {
            sounds.push_back(sound);
        }
    }
    // The sounds of the splash screens, the menus, and the races as WAV files.
    for (const auto &sound :
         std::filesystem::directory_iterator(destination / "sounds", carsError)) {
        if (sound.path().extension() == ".bin") {
            sounds.push_back(sound.path());
        }
    }
    for (const auto &sound : sounds) {
        if (auto failure = ConvertConsoleSound(sound)) {
            return std::unexpected(*failure);
        }
    }
    if (auto failure = ConvertConsoleFonts(destination, *release)) {
        return std::unexpected(*failure);
    }
    if (auto failure = ConvertConsoleLights(destination, *release)) {
        return std::unexpected(*failure);
    }
    for (const auto &model : splashModels) {
        auto stem = model.stem().string();
        if (auto failure = ConvertModel(model,
                                        FrontEndModelToGltf,
                                        std::filesystem::path(model).replace_extension(".gltf"),
                                        model.parent_path() / (stem + "-mesh.bin"))) {
            return std::unexpected(*failure);
        }
    }
    auto segment = std::span<const std::uint8_t>(*bytes).subspan(SEGMENT_ROM, SEGMENT_SIZE);
    auto assets = ManifestAssets(known.manifest);
    if (auto failure = ConvertConsoleTextures(destination, segment, assets)) {
        return std::unexpected(*failure);
    }
    if (auto failure = WriteMenuChoiceMarker(destination, segment, *release)) {
        return std::unexpected(*failure);
    }
    WriteMenuItems(destination, segment, assets);
    // The game plays without the attract demo of a release. A failure to record it is logged and
    // the import goes on.
    if (auto failure = WriteAttractDemo(*bytes, *release, destination)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s", failure->c_str());
    }
    if (auto failure = WriteTrackMenu(destination, segment, assets, *release)) {
        return std::unexpected(*failure);
    }
    // Replaces each model of the menu items and of the tracks on the track menu with a glTF file
    // of the same name.
    for (const ManifestAsset &asset : assets) {
        if (!asset.name.starts_with("menu-items/") && !asset.name.ends_with("/menu-model")) {
            continue;
        }
        auto model = destination / (asset.name + ".bin");
        auto stem = model.stem().string();
        if (auto failure = ConvertModel(model,
                                        FrontEndModelToGltf,
                                        std::filesystem::path(model).replace_extension(".gltf"),
                                        model.parent_path() / (stem + "-mesh.bin"))) {
            SDL_Log("%s", failure->c_str());
        }
    }
    std::error_code error;
    if (*release == RomRelease::Usa) {
        // The XM modules of the menus and of the tracks, as the music pack default.
        for (const UsaMusicModule &entry : USA_MUSIC_MODULES) {
            auto module = Tools::Common::decompress(std::span(*bytes).subspan(entry.offset));
            if (!module) {
                return std::unexpected(module.error().message);
            }
            auto path = dataDirectory / "music" / (std::string(entry.name) + ".xm");
            std::filesystem::create_directories(path.parent_path(), error);
            std::ofstream xm(path, std::ios::binary | std::ios::trunc);
            xm.write(reinterpret_cast<const char *>(module->data()),
                     static_cast<std::streamsize>(module->size()));
            if (!xm) {
                return std::unexpected(fmt::format("{} cannot be written.", path.string()));
            }
        }
    }
    return release;
}

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
        return std::unexpected(fmt::format("{} cannot be read.", data->file.string()));
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
    if (auto failure = ConvertPodArchive(dataDirectory)) {
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
        auto name = fmt::format("track{:02}.flac", track.number);
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

bool ReadImportedPodEntry(const char *pszPath, void *pBuffer) {
    // The archive names an entry by the name of its file, as CleanupName() takes it.
    std::string_view path(pszPath);
    std::size_t separator = path.find_last_of("\\/");
    std::string name(separator == std::string_view::npos ? path : path.substr(separator + 1));
    auto bytes =
        coastline::ReadPodEntry(std::filesystem::path("BossRally") / coastline::PodEntryFile(name));
    if (!bytes) {
        return false;
    }
    std::memcpy(pBuffer, bytes->data(), bytes->size());
    return true;
}
