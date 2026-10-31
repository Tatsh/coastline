#include "importer.h"

#include <algorithm>
#include <array>
#include <bit>
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
#include "common/profile.h"
#include "common/rigidbody.h"
#include "common/rle.h"
#include "common/weather.h"
#include "sdl/consolestrings.h"
#include "sdl/ghostfile.h"
#include "sdl/iconfile.h"
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
    const char *language;                    // The language of the text of the release.
};

const std::array<Release, 3> RELEASES{{
    {RomRelease::Usa, "bfc51f086ebdb78904dc24ad0c3a2d946e26d022", "usa", CARTRIDGE_US, "en-us"},
    {RomRelease::Jpn, "e0415b87c5a5b69ac581e79137406bdaa79b354a", "jpn", CARTRIDGE_JP, "ja"},
    {RomRelease::Eur, "9fae5d98fe3464f00d59034690df90118646eb95", "eur", CARTRIDGE_EU, "en-gb"},
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

// The files of the disc the game opens, and the icon of the window. The video, the installers,
// DirectX, the programs, and the pictures of the menus and the loading screen of the Windows
// release, which the front end of the Nintendo 64 release replaces, are not copied.
bool IsGameData(const std::filesystem::path &path) {
    static constexpr std::array<std::string_view, 5> directories{
        "cargfx", "cars", "paint", "sfx", "tracks"};
    // The text of the game is in the string resources of BRString.dll.
    static constexpr std::array<std::string_view, 7> files{"boss.ico",
                                                           "bossrally.pod",
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

// An image of the icon as a PNG image named for its size, which reads back with the same pixels. A
// PNG image in the icon file is written as it is.
std::optional<std::string> ConvertIconImage(const IconEntry &entry,
                                            const std::filesystem::path &directory) {
    static constexpr std::array<std::uint8_t, 8> PNG_SIGNATURE{
        0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    if (entry.image.size() >= PNG_SIGNATURE.size() &&
        std::ranges::equal(entry.image.first(PNG_SIGNATURE.size()), PNG_SIGNATURE)) {
        Surface image(
            SDL_LoadPNG_IO(SDL_IOFromConstMem(entry.image.data(), entry.image.size()), true));
        if (!image) {
            return fmt::format("A PNG image of the icon cannot be read: {}", SDL_GetError());
        }
        auto png = directory / IconName({image->w, image->h, static_cast<int>(entry.bitCount)});
        Surface written(WriteBytes(png, entry.image) ? SDL_LoadPNG(png.string().c_str()) : nullptr);
        if (!written || written->w != image->w || written->h != image->h) {
            return fmt::format("{} does not read back as the image of the icon.", png.string());
        }
        return std::nullopt;
    }
    auto pixels = DecodeIconBitmap(entry.image);
    if (!pixels) {
        return fmt::format(
            "An image of the icon is not a bitmap of {} bits a pixel the game reads.",
            entry.bitCount);
    }
    auto png =
        directory / IconName({pixels->width, pixels->height, static_cast<int>(entry.bitCount)});
    if (!SaveRgba8Png(png, pixels->rgba, pixels->width, pixels->height)) {
        return fmt::format("{} does not read back as the pixels of the icon.", png.string());
    }
    return std::nullopt;
}

// Boss.ico, the icon of the Windows release, moved unchanged to icon/coastline.ico, with each of
// its images as a PNG image in icon.
std::optional<std::string> ConvertIconFile(const std::filesystem::path &file,
                                           const std::filesystem::path &dataDirectory) {
    auto icon = ReadWholeFile(file);
    auto entries = icon ? ListIconImages(*icon) : std::vector<IconEntry>{};
    if (entries.empty()) {
        return fmt::format("{} is not an icon file with images.", file.string());
    }
    auto directory = dataDirectory / "icon";
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    for (const auto &entry : entries) {
        if (auto failure = ConvertIconImage(entry, directory)) {
            return failure;
        }
    }
    std::filesystem::rename(file, directory / "coastline.ico", error);
    if (error) {
        return fmt::format("{} cannot be moved to {}: {}",
                           file.string(),
                           (directory / "coastline.ico").string(),
                           error.message());
    }
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

// The characters of the glyph codes of the Japanese release from FIRST_JAPANESE_GLYPH.
constexpr std::u32string_view JAPANESE_GLYPHS =
    U"！？。、ー☀☁☂❄☾゛゜"
    U"あかさたなはまやらがざだばぱぁっいきしちにひみゆりぎじぢびぴぃゃ"
    U"うくすつぬふむよるぐずづぶぷぅゅえけせてねへめわれげぜでべぺぇょ"
    U"おこそとのほもをろごぞどぼぽぉん"
    U"アカサタナハマヤラガザダバパァッイキシチニヒミユリギジヂビピィャ"
    U"ウクスツヌフムヨルグズヅブプゥュエケセテネヘメワレゲゼデベペェョ"
    U"オコソトノホモヲロゴゾドボポォン"
    U"異常振動交換必要左右上下反転使用消空足読書込中選元他全前後発生";
constexpr unsigned FIRST_JAPANESE_GLYPH = 0x24;

// The text of a string of a release in UTF-8 without %j and %e, or nothing for bytes that are not
// text. The console draws each byte between %j and %e as a glyph code of the Japanese release, and
// a %% there as the glyph of '%'. Every other byte is ASCII.
std::optional<std::string> DecodeConsoleText(std::string_view text) {
    std::string decoded;
    bool japanese = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
        auto c = static_cast<unsigned char>(text[i]);
        if (c == '%' && i + 1 < text.size()) {
            char next = text[i + 1];
            if (next == 'j' || next == 'e') {
                japanese = next == 'j';
                ++i;
                continue;
            }
            // A code of the colours applies between %j and %e as well.
            if (next == '%' && japanese) {
                c = '%';
                ++i;
            } else if (next == '%' || next == 'i' || next == 'n') {
                decoded += text.substr(i, 2);
                ++i;
                continue;
            } else if (i + 2 < text.size()) {
                decoded += text.substr(i, 3);
                i += 2;
                continue;
            }
        }
        if (!japanese || c == ' ') {
            if (c < 0x20 || c >= 0x7F) {
                return std::nullopt;
            }
            decoded += static_cast<char>(c);
        } else if (c >= FIRST_JAPANESE_GLYPH && c - FIRST_JAPANESE_GLYPH < JAPANESE_GLYPHS.size()) {
            AppendUtf8(decoded, JAPANESE_GLYPHS[c - FIRST_JAPANESE_GLYPH]);
        } else {
            return std::nullopt;
        }
    }
    return decoded;
}

// The glyphs of the Japanese text in a row of japanese.png, 36 texels across and 40 down, from the
// glyphs the console caches from the cartridge, and their layout for fonts.json with the glyph of
// each code point. The glyphs are 4-bit intensity and alpha, upside down.
std::expected<nlohmann::ordered_json, std::string>
ConvertJapaneseGlyphs(const std::filesystem::path &glyphs, const std::filesystem::path &image) {
    constexpr int WIDTH = 36;
    constexpr int HEIGHT = 40;
    constexpr int COLUMNS = 16;
    constexpr std::size_t SLOT = WIDTH * HEIGHT;
    // The table of offsets starts at the glyph code 0x20.
    constexpr unsigned FIRST_OFFSET = 0x20;
    auto bytes = ReadWholeFile(glyphs);
    if (!bytes) {
        return std::unexpected(fmt::format("{} cannot be read.", glyphs.string()));
    }
    auto offset = [&](std::size_t entry) -> std::optional<std::int32_t> {
        std::size_t at = entry * 4;
        if (at + 4 > bytes->size()) {
            return std::nullopt;
        }
        return static_cast<std::int32_t>(static_cast<std::uint32_t>((*bytes)[at]) << 24 |
                                         (*bytes)[at + 1] << 16 | (*bytes)[at + 2] << 8 |
                                         (*bytes)[at + 3]);
    };
    const int count = static_cast<int>(JAPANESE_GLYPHS.size());
    const int rows = (count + COLUMNS - 1) / COLUMNS;
    const int imageWidth = COLUMNS * WIDTH;
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(imageWidth) * rows * HEIGHT * 4);
    nlohmann::ordered_json glyphOfCodePoint = nlohmann::ordered_json::object();
    for (int glyph = 0; glyph < count; ++glyph) {
        std::size_t entry = FIRST_JAPANESE_GLYPH - FIRST_OFFSET + static_cast<std::size_t>(glyph);
        auto start = offset(entry);
        auto end = offset(entry + 1);
        if (!start || !end) {
            return std::unexpected(fmt::format("{} has no table of glyphs.", glyphs.string()));
        }
        // A negative offset marks a glyph stored without run-length compression.
        std::size_t from = static_cast<std::size_t>(std::abs(*start));
        std::size_t to = static_cast<std::size_t>(std::abs(*end));
        if (from > to || to > bytes->size()) {
            return std::unexpected(fmt::format("{} has a glyph outside it.", glyphs.string()));
        }
        // The expansion does not check the size of its buffer, given room for twice a glyph.
        std::vector<std::uint8_t> texels(SLOT * 2);
        std::vector<std::uint8_t> stored(bytes->begin() + static_cast<std::ptrdiff_t>(from),
                                         bytes->begin() + static_cast<std::ptrdiff_t>(to));
        // The run-length block starts with its length, big-endian on the cartridge, which the
        // expansion reads in the order of the host.
        std::uint32_t length = 0;
        if (*start >= 0) {
            if (stored.size() < 4) {
                return std::unexpected(fmt::format("{} has a glyph cut short.", glyphs.string()));
            }
            length = static_cast<std::uint32_t>(stored[0]) << 24 | stored[1] << 16 |
                     stored[2] << 8 | stored[3];
            if (length > stored.size() - 4) {
                return std::unexpected(fmt::format("{} has a glyph cut short.", glyphs.string()));
            }
            std::memcpy(stored.data(), &length, 4);
        }
        if (*start < 0) {
            std::copy_n(stored.begin(), std::min(stored.size(), SLOT), texels.begin());
        } else if (DecompressInterleavedRle(
                       texels.data(), static_cast<s32>(SLOT), stored.data(), 1) !=
                   static_cast<s32>(SLOT)) {
            return std::unexpected(fmt::format("{} has a glyph of another size.", glyphs.string()));
        }
        int left = glyph % COLUMNS * WIDTH;
        int top = glyph / COLUMNS * HEIGHT;
        for (int row = 0; row < HEIGHT; ++row) {
            for (int column = 0; column < WIDTH; ++column) {
                std::uint8_t texel = texels[static_cast<std::size_t>(row) * WIDTH + column];
                std::size_t pixel = (static_cast<std::size_t>(top + HEIGHT - 1 - row) * imageWidth +
                                     left + column) *
                                    4;
                auto intensity = static_cast<std::uint8_t>((texel >> 4) * 17);
                rgba[pixel] = intensity;
                rgba[pixel + 1] = intensity;
                rgba[pixel + 2] = intensity;
                rgba[pixel + 3] = static_cast<std::uint8_t>((texel & 0xF) * 17);
            }
        }
        glyphOfCodePoint[std::to_string(static_cast<std::uint32_t>(JAPANESE_GLYPHS[glyph]))] =
            glyph;
    }
    if (!SaveRgba8Png(image, rgba, imageWidth, rows * HEIGHT)) {
        return std::unexpected(fmt::format("{} does not read back as its texels.", image.string()));
    }
    return nlohmann::ordered_json{{"image", image.filename().string()},
                                  {"width", WIDTH},
                                  {"columns", COLUMNS},
                                  {"glyphs", glyphOfCodePoint}};
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
    // The console indexes its table by the byte of a character from '!', each byte an ASCII
    // character, and the game by code point.
    nlohmann::ordered_json glyphOfCodePoint = nlohmann::ordered_json::object();
    for (std::size_t i = 0; i < CHARACTERS && '!' + i < 0x80; ++i) {
        glyphOfCodePoint[std::to_string('!' + i)] = (*glyphs)[i];
    }
    // The glyph of each code point, and where each glyph starts in its row. The images are
    // upright, and the first row of glyphs is the upper row of cells.
    nlohmann::ordered_json layout{{"glyphs", glyphOfCodePoint},
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
    if (release == RomRelease::Jpn) {
        auto japanese = ConvertJapaneseGlyphs(destination / "segments" / "cachedGlyphs.bin",
                                              fontsDirectory / "japanese.png");
        if (!japanese) {
            return japanese.error();
        }
        layout["japanese"] = *japanese;
    }
    std::ofstream(fontsDirectory / "fonts.json", std::ios::trunc) << layout.dump(1) << '\n';
    return std::nullopt;
}

// The start of the keys of the files of strings that the import of a cartridge writes (n64.NAME),
// and of the n64/ADDRESS keys an older import wrote.
constexpr std::string_view CONSOLE_KEY_PREFIX = "n64";

// Write @p strings to the file of strings @p destination. The keys of the file that @p console
// does not select (the keys of a cartridge when true, of the disc when false) are retained.
std::optional<std::string> WriteStringsFile(const std::filesystem::path &destination,
                                            const nlohmann::ordered_json &strings,
                                            bool console) {
    nlohmann::ordered_json merged = nlohmann::ordered_json::object();
    std::ifstream in(destination);
    if (in) {
        auto existing = nlohmann::ordered_json::parse(in, nullptr, false);
        if (existing.is_object()) {
            for (const auto &[key, text] : existing.items()) {
                if (key.starts_with(CONSOLE_KEY_PREFIX) != console) {
                    merged[key] = text;
                }
            }
        }
    }
    in.close();
    merged.update(strings);
    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    std::ofstream out(destination, std::ios::trunc);
    out << merged.dump(2) << '\n';
    if (!out) {
        return fmt::format("{} cannot be written.", destination.string());
    }
    return std::nullopt;
}

// The messages of CONSOLE_STRINGS that @p release draws, from the main segment of @p rom, as
// strings/LANGUAGE.json in UTF-8 for the language of the release. The lines of a message are
// joined with a newline.
std::optional<std::string> WriteConsoleStrings(std::span<const std::uint8_t> rom,
                                               const Release &release,
                                               const std::filesystem::path &dataDirectory) {
    constexpr std::uint32_t SEGMENT_BASE = 0x80200000;
    std::string_view bytes(reinterpret_cast<const char *>(rom.data()), rom.size());
    nlohmann::ordered_json strings = nlohmann::ordered_json::object();
    for (const auto &message : CONSOLE_STRINGS) {
        const auto &lines = release.release == RomRelease::Usa ? message.usa :
                            release.release == RomRelease::Eur ? message.eur :
                                                                 message.jpn;
        if (lines.empty()) {
            continue;
        }
        std::string joined;
        for (std::size_t line = 0; line < lines.size(); ++line) {
            auto address = lines[line];
            std::size_t start = SEGMENT_ROM + (address - SEGMENT_BASE);
            auto end = start < bytes.size() ? bytes.find('\0', start) : std::string_view::npos;
            auto decoded = end == std::string_view::npos ?
                               std::nullopt :
                               DecodeConsoleText(bytes.substr(start, end - start));
            if (!decoded) {
                return fmt::format(
                    "The string at {:08x} for {} is not text.", address, message.key);
            }
            if (line > 0) {
                joined += '\n';
            }
            joined += *decoded;
        }
        strings[std::string(message.key)] = joined;
    }
    return WriteStringsFile(
        dataDirectory / "strings" / (std::string(release.language) + ".json"), strings, true);
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
        auto layout = std::ranges::find_if(TEXEL_LAYOUTS, [&](const TexelLayout &entry) {
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

// The position and the colour of each of a number of vertices of the console in the main segment,
// whose bounds the caller checks.
nlohmann::ordered_json
VerticesJson(std::span<const std::uint8_t> segment, std::uint32_t address, std::size_t count) {
    constexpr std::uint32_t SEGMENT_BASE = 0x80200000;
    constexpr std::size_t VERTEX = 16;
    nlohmann::ordered_json vertices = nlohmann::ordered_json::array();
    for (std::size_t i = 0; i < count; ++i) {
        const std::uint8_t *vertex = &segment[address - SEGMENT_BASE + i * VERTEX];
        auto coordinate = [vertex](int index) {
            return static_cast<std::int16_t>(vertex[index * 2] << 8 | vertex[index * 2 + 1]);
        };
        vertices.push_back({{"position", {coordinate(0), coordinate(1), coordinate(2)}},
                            {"color", {vertex[12], vertex[13], vertex[14], vertex[15]}}});
    }
    return vertices;
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
    nlohmann::ordered_json marker{{"vertices", VerticesJson(segment, address, VERTICES)},
                                  {"triangles", {{0, 1, 2}, {6, 7, 8}, {3, 4, 5}, {9, 10, 11}}}};
    std::ofstream(destination / "data" / "menu-choice-marker.json", std::ios::trunc)
        << marker.dump(2) << '\n';
    return std::nullopt;
}

// The addresses of the vertices of the choice marker of the stage of the cars, drawn by
// g_aCarStageChoiceMarkerDisplayList, of the shadow of a car on the stage, drawn by
// g_aCarStageShadowDisplayList, and of g_apVehicleReflectionTextures in each release.
struct CarStageAddresses {
    std::uint32_t choiceMarker;
    std::uint32_t shadow;
    std::uint32_t reflectionTextures;
};
constexpr CarStageAddresses USA_CAR_STAGE{0x8028C430, 0x8028C550, 0x8028C308};
constexpr CarStageAddresses EUR_CAR_STAGE{0x8028EB90, 0x8028ECB0, 0x8028EA68};
constexpr CarStageAddresses JPN_CAR_STAGE{0x8028E5E0, 0x8028E700, 0x8028E4B8};

// Writes the 12 vertices and the triangles of the choice marker of the stage of the cars and the 8
// vertices and the triangles of the shadow of a car on it as data/car-stage.json, and the
// reflection of the sky on the cars of the menus as data/menu-reflection.png. The menus draw the
// reflection of view mode 0 in the open, a 32 by 128 texture of 4-bit indices with its palette.
std::optional<std::string> WriteCarStage(const std::filesystem::path &destination,
                                         std::span<const std::uint8_t> segment,
                                         RomRelease release) {
    constexpr std::uint32_t SEGMENT_BASE = 0x80200000;
    constexpr std::size_t MARKER_VERTICES = 12;
    constexpr std::size_t SHADOW_VERTICES = 8;
    constexpr std::size_t VERTEX = 16;
    // g_apVehicleReflectionTluts follows the 4 textures, and g_abVehicleReflectionTexSelect
    // follows them after g_dwVehicleViewModeIndex.
    constexpr std::uint32_t TLUTS = 0x10;
    constexpr std::uint32_t SELECT = 0x24;
    constexpr int REFLECTION_WIDTH = 32;
    constexpr int REFLECTION_HEIGHT = 128;
    constexpr std::size_t REFLECTION_BYTES = REFLECTION_WIDTH * REFLECTION_HEIGHT / 2;
    constexpr std::size_t PALETTE_BYTES = 32;
    const CarStageAddresses &at = release == RomRelease::Usa ? USA_CAR_STAGE :
                                  release == RomRelease::Eur ? EUR_CAR_STAGE :
                                                               JPN_CAR_STAGE;
    auto inside = [&](std::uint32_t address, std::size_t size) {
        return address >= SEGMENT_BASE && address - SEGMENT_BASE + size <= segment.size();
    };
    if (!inside(at.choiceMarker, MARKER_VERTICES * VERTEX) ||
        !inside(at.shadow, SHADOW_VERTICES * VERTEX) ||
        !inside(at.reflectionTextures, SELECT + 1)) {
        return std::string("The stage of the cars is outside the main segment.");
    }
    nlohmann::ordered_json stage{
        {"choiceMarker",
         {{"vertices", VerticesJson(segment, at.choiceMarker, MARKER_VERTICES)},
          {"triangles", {{0, 1, 2}, {6, 7, 8}, {3, 4, 5}, {9, 10, 11}}}}},
        {"shadow",
         {{"vertices", VerticesJson(segment, at.shadow, SHADOW_VERTICES)},
          {"triangles", {{0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 5}, {0, 5, 6}, {0, 6, 7}}}}}};
    std::ofstream(destination / "data" / "car-stage.json", std::ios::trunc)
        << stage.dump(2) << '\n';

    std::uint32_t reflection = segment[at.reflectionTextures + SELECT - SEGMENT_BASE];
    if (reflection >= 4) {
        return std::string("The menus select no reflection of the sky.");
    }
    std::uint32_t texels = Big32(&segment[at.reflectionTextures + reflection * 4 - SEGMENT_BASE]);
    std::uint32_t palette =
        Big32(&segment[at.reflectionTextures + TLUTS + reflection * 4 - SEGMENT_BASE]);
    if (!inside(texels, REFLECTION_BYTES) || !inside(palette, PALETTE_BYTES)) {
        return std::string("The reflection of the menus is outside the main segment.");
    }
    auto png = destination / "data" / "menu-reflection.png";
    if (!SaveIndexed4Png(png,
                         segment.subspan(texels - SEGMENT_BASE, REFLECTION_BYTES),
                         segment.subspan(palette - SEGMENT_BASE).first<PALETTE_BYTES>(),
                         REFLECTION_WIDTH,
                         REFLECTION_HEIGHT)) {
        return fmt::format("{} does not read back as its texels.", png.string());
    }
    return std::nullopt;
}

// The addresses of the tables of the track menu and the championship of each release:
// g_aTrackDescriptors, g_apWeatherMenuItems, g_dwNumTracks, g_dwCarCount, g_aCarModels, and
// g_SeasonTable, and of the car select screen: g_apszEngineLayouts, g_apszDriveTypes, and the
// options of the handling, the transmission, the tyres, the suspension, and the decals.
struct TrackMenuAddresses {
    std::uint32_t trackDescriptors;
    std::uint32_t weatherMenuItems;
    std::uint32_t numTracks;
    std::uint32_t carCount;
    std::uint32_t carModels;
    std::uint32_t seasonTable;
    std::uint32_t engineLayouts;
    std::uint32_t driveTypes;
    std::array<std::uint32_t, 5> options;
};
constexpr TrackMenuAddresses USA_TRACK_MENU{
    0x80270854,
    0x802722A4,
    0x8028AE04,
    0x8028AE08,
    0x8028AE0C,
    0x8028B944,
    0x80272030,
    0x8027203C,
    {0x802720B4, 0x802720F0, 0x8027213C, 0x8027218C, 0x802721DC}};
constexpr TrackMenuAddresses EUR_TRACK_MENU{
    0x80272834,
    0x80274284,
    0x8028D0B4,
    0x8028D0B8,
    0x8028D0BC,
    0x8028DBF4,
    0x80274010,
    0x8027401C,
    {0x80274094, 0x802740D0, 0x8027411C, 0x8027416C, 0x802741BC}};
constexpr TrackMenuAddresses JPN_TRACK_MENU{
    0x802722F4,
    0x80273D34,
    0x8028CAE4,
    0x8028CAE8,
    0x8028CAEC,
    0x8028D624,
    0x80273AD0,
    0x80273ADC,
    {0x80273B40, 0x80273B7C, 0x80273BC8, 0x80273C18, 0x80273C68}};

// Writes the items of the tracks on the track menu, the names of the weathers, the cars, the
// seasons of the championship, and the options of the car select screen as data/menus.json. Each
// track gives its name, its MENU_ITEM_ flags, whether its geometry exists, and the glTF file of its
// model in the import. Each car gives its name, its directory in the import, whether its model
// exists, what the car select screen shows of it, and the size of its shadow on the stage of the
// cars, and each season the indices of its two cars. Each option gives its name, its flags, and the
// glTF file of its model, or none.
std::optional<std::string> WriteMenus(const std::filesystem::path &destination,
                                      std::span<const std::uint8_t> segment,
                                      std::span<const ManifestAsset> assets,
                                      RomRelease release) {
    constexpr std::uint32_t SEGMENT_BASE = 0x80200000;
    constexpr std::uint32_t TRACK_DESCRIPTOR = 0x17C;
    constexpr std::uint32_t TRACK_DESCRIPTORS = 11;
    constexpr std::uint32_t WEATHERS = 5;
    constexpr std::uint32_t CAR_MODEL = 0x60;
    constexpr std::uint32_t SEASON = 0x1C;
    constexpr std::uint32_t MENU_ITEM = 20;
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
            return fmt::format("Track {} of the track menu has no name or no model.", i);
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
            return fmt::format("Weather {} of the menus has no name.", i);
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
            return fmt::format("Car {} has no name.", i);
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
        std::uint32_t entry = at.carModels + i * CAR_MODEL;
        cars.push_back({{"name", *name},
                        {"directory", "cars/" + directory},
                        {"exists", word(entry + 0x18) != 0},
                        {"driveType", static_cast<std::int32_t>(word(entry + 0x3C))},
                        {"engineLayout", static_cast<std::int32_t>(word(entry + 0x40))},
                        {"acceleration", std::bit_cast<float>(word(entry + 0x44))},
                        {"maxSpeed", std::bit_cast<float>(word(entry + 0x48))},
                        {"gearCount", static_cast<std::int32_t>(word(entry + 0x54))},
                        {"shadowLength", std::bit_cast<float>(word(entry + 0x58))},
                        {"shadowHalfWidth", std::bit_cast<float>(word(entry + 0x5C))}});
    }
    auto names = [&](std::uint32_t address,
                     std::uint32_t count) -> std::optional<nlohmann::ordered_json> {
        nlohmann::ordered_json list = nlohmann::ordered_json::array();
        for (std::uint32_t i = 0; i < count; ++i) {
            auto name = inside(address + i * 4, 4) ? text(word(address + i * 4)) : std::nullopt;
            if (!name) {
                return std::nullopt;
            }
            list.push_back(*name);
        }
        return list;
    };
    // A list of options ends with a null pointer, and an option without a model starts at 0.
    auto options = [&](std::uint32_t address) -> std::optional<nlohmann::ordered_json> {
        constexpr std::uint32_t MOST_OPTIONS = 16;
        nlohmann::ordered_json list = nlohmann::ordered_json::array();
        for (std::uint32_t i = 0; i < MOST_OPTIONS && inside(address + i * 4, 4); ++i) {
            std::uint32_t item = word(address + i * 4);
            if (item == 0) {
                return list;
            }
            auto name = inside(item, MENU_ITEM) ? text(word(item)) : std::nullopt;
            if (!name) {
                return std::nullopt;
            }
            nlohmann::ordered_json model = nullptr;
            std::uint32_t modelStart = word(item + 0x0C);
            if (modelStart != 0) {
                auto asset = std::ranges::find(assets, modelStart, &ManifestAsset::romStart);
                if (asset == assets.end()) {
                    return std::nullopt;
                }
                model = asset->name + ".gltf";
            }
            list.push_back({{"name", *name}, {"flags", word(item + 0x04)}, {"model", model}});
        }
        return std::nullopt;
    };
    constexpr std::array<const char *, 5> OPTION_LISTS{
        "handling", "transmission", "tires", "suspension", "decals"};
    nlohmann::ordered_json carSelect;
    auto engineLayouts = names(at.engineLayouts, 3);
    auto driveTypes = names(at.driveTypes, 3);
    if (!engineLayouts || !driveTypes) {
        return std::string("The engine layouts and drive types of the cars are not readable.");
    }
    carSelect["engineLayouts"] = *engineLayouts;
    carSelect["driveTypes"] = *driveTypes;
    for (std::size_t i = 0; i < OPTION_LISTS.size(); ++i) {
        auto list = options(at.options[i]);
        if (!list) {
            return fmt::format("The {} options of the car select screen are not readable.",
                               OPTION_LISTS[i]);
        }
        carSelect[OPTION_LISTS[i]] = *list;
    }
    nlohmann::ordered_json seasons = nlohmann::ordered_json::array();
    for (std::uint32_t i = 0; i < SEASONS; ++i) {
        std::uint32_t entry = at.seasonTable + i * SEASON;
        const std::uint8_t *bytes = &segment[entry - SEGMENT_BASE];
        auto name = text(word(entry));
        auto subtitle = text(word(entry + 4));
        if (!name || !subtitle || bytes[16] >= cars.size() || bytes[17] >= cars.size()) {
            return fmt::format("Season {} of the championship is not readable.", i);
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
                                {"seasons", seasons},
                                {"carSelect", carSelect}};
    std::ofstream(destination / "data" / "menus.json", std::ios::trunc)
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

// The strings of BRString.dll as strings/en-us.json, by identifier, in place of the library.
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
    if (auto failure = WriteStringsFile(dataDirectory / "strings" / "en-us.json", json, false)) {
        return failure;
    }
    std::error_code error;
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
    if (auto failure = WriteConsoleStrings(*bytes, known, dataDirectory)) {
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
    if (auto failure = WriteCarStage(destination, segment, *release)) {
        return std::unexpected(*failure);
    }
    WriteMenuItems(destination, segment, assets);
    // The game plays without the attract demo of a release. A failure to record it is logged and
    // the import goes on.
    if (auto failure = WriteAttractDemo(*bytes, *release, destination)) {
        SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "%s", failure->c_str());
    }
    if (auto failure = WriteMenus(destination, segment, assets, *release)) {
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
        } else if (name == "boss.ico") {
            failure = ConvertIconFile(dataDirectory / entry.path, dataDirectory);
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
