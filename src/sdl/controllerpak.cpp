#include "controllerpak.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <string_view>

#include <SDL3/SDL.h>
#include <fmt/format.h>

namespace coastline {

namespace {

// The image of a pak: the identity on page 0, the table of the pages on page 1 and its copy on page
// 2, the table of the notes on pages 3 and 4, and the pages of the notes after.
constexpr std::size_t IDENTITY_PAGE = 0;
constexpr std::size_t INODE_PAGE = 1;
constexpr std::size_t INODE_BACKUP_PAGE = 2;
constexpr std::size_t NOTE_TABLE_PAGE = 3;
constexpr std::size_t FIRST_DATA_PAGE = 5;
constexpr std::size_t IMAGE_PAGES = PAK_IMAGE_BYTES / PAK_PAGE_BYTES;

// mupen64plus stores the paks of the four controllers in one file.
constexpr std::size_t FOUR_PAK_IMAGE_BYTES = 4 * PAK_IMAGE_BYTES;

// The places of the identity block and its copies on page 0, and its size.
constexpr std::array<std::size_t, 4> IDENTITY_BLOCKS{0x20, 0x60, 0x80, 0xC0};
constexpr std::size_t IDENTITY_SIZE = 0x20;

// The entries of the table of the pages: a page that ends a note, and a free page.
constexpr std::uint16_t INODE_LAST_PAGE = 0x0001;
constexpr std::uint16_t INODE_FREE_PAGE = 0x0003;

// An entry of the table of the notes, and the status of an entry in use.
constexpr std::size_t NOTE_ENTRY_SIZE = 0x20;
constexpr std::uint8_t NOTE_OCCUPIED = 0x02;
constexpr std::size_t NOTE_NAME_SIZE = 16;

// The game code and the company code each release gives its notes, every note of the release
// included: NGRE and 5D for the USA, NTRP and 7F for Europe, and NTRJ and 28 for Japan.
struct PakCodes {
    std::uint32_t game;
    std::uint16_t company;
};
constexpr std::array<PakCodes, 3> RELEASE_CODES{{
    {0x4E475245, 0x3544},
    {0x4E545250, 0x3746},
    {0x4E54524A, 0x3238},
}};

const PakCodes &CodesOf(PakRelease release) {
    return RELEASE_CODES[static_cast<std::size_t>(release)];
}

// The characters of the names of the notes, from 0x0F for a space.
constexpr std::string_view NOTE_CHARACTERS = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#'*+,-./:=?@";
constexpr std::uint8_t NOTE_FIRST_CHARACTER = 0x0F;

std::uint16_t Get16(const std::vector<std::uint8_t> &image, std::size_t at) {
    return static_cast<std::uint16_t>(image[at] << 8 | image[at + 1]);
}

void Put16(std::vector<std::uint8_t> &image, std::size_t at, std::uint16_t value) {
    image[at] = static_cast<std::uint8_t>(value >> 8);
    image[at + 1] = static_cast<std::uint8_t>(value);
}

// The checksum of the table of the pages, the low byte of the sum of the bytes of the entries of
// the data pages, as __osSumcalc() of libultra computes it.
std::uint16_t InodeChecksum(const std::vector<std::uint8_t> &image, std::size_t page) {
    unsigned sum = 0;
    for (std::size_t at = FIRST_DATA_PAGE * 2; at < PAK_PAGE_BYTES; ++at) {
        sum += image[page * PAK_PAGE_BYTES + at];
    }
    return static_cast<std::uint16_t>(sum & 0xFF);
}

// A name of a note in the characters of the pak, or nothing for a name it cannot write.
std::optional<std::array<std::uint8_t, NOTE_NAME_SIZE>> EncodeName(const std::string &name) {
    std::array<std::uint8_t, NOTE_NAME_SIZE> encoded{};
    if (name.size() > NOTE_NAME_SIZE) {
        return std::nullopt;
    }
    for (std::size_t i = 0; i < name.size(); ++i) {
        auto found = NOTE_CHARACTERS.find(name[i]);
        if (found == std::string_view::npos) {
            return std::nullopt;
        }
        encoded[i] = static_cast<std::uint8_t>(NOTE_FIRST_CHARACTER + found);
    }
    return encoded;
}

std::optional<std::string> DecodeName(const std::uint8_t *name) {
    std::string decoded;
    for (std::size_t i = 0; i < NOTE_NAME_SIZE && name[i] != 0; ++i) {
        std::size_t index = name[i] - NOTE_FIRST_CHARACTER;
        if (name[i] < NOTE_FIRST_CHARACTER || index >= NOTE_CHARACTERS.size()) {
            return std::nullopt;
        }
        decoded += NOTE_CHARACTERS[index];
    }
    return decoded;
}

std::vector<std::uint8_t> ReadFile(const std::filesystem::path &file) {
    std::ifstream stream(file, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(stream), {});
}

} // namespace

ControllerPak::ControllerPak(std::filesystem::path directory,
                             std::filesystem::path settingsDirectory)
    : m_directory(std::move(directory)), m_settingsDirectory(std::move(settingsDirectory)) {
    if (m_settingsDirectory.empty()) {
        m_settingsDirectory = m_directory;
    }
}

const std::filesystem::path &ControllerPak::DirectoryOf(const std::string &name) const {
    return name == PAK_SETTINGS_NOTE ? m_settingsDirectory : m_directory;
}

std::optional<std::vector<std::uint8_t>> ControllerPak::ReadNote(const std::string &name) const {
    std::ifstream file(DirectoryOf(name) / name, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), {});
}

bool ControllerPak::WriteNote(const std::string &name, std::span<const std::uint8_t> bytes) const {
    std::error_code error;
    const std::filesystem::path &directory = DirectoryOf(name);
    std::filesystem::create_directories(directory, error);
    // The note is written beside its place and moved there, and a failed write loses no note.
    auto written = directory / (name + ".new");
    {
        std::ofstream file(written, std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char *>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()));
        if (!file) {
            return false;
        }
    }
    std::filesystem::rename(written, directory / name, error);
    return !error;
}

bool ControllerPak::DeleteNote(const std::string &name) const {
    std::error_code error;
    return std::filesystem::remove(DirectoryOf(name) / name, error);
}

std::vector<PakNote> ControllerPak::Notes() const {
    std::vector<PakNote> notes;
    std::error_code error;
    for (const auto &entry : std::filesystem::directory_iterator(m_directory, error)) {
        if (entry.is_regular_file(error) && entry.path().extension() != ".new" &&
            DirectoryOf(entry.path().filename().string()) == m_directory) {
            notes.push_back({entry.path().filename().string(),
                             static_cast<std::size_t>(entry.file_size(error))});
        }
    }
    // The settings note counts against the room of the pak like the others.
    if (m_settingsDirectory != m_directory) {
        auto settings = m_settingsDirectory / PAK_SETTINGS_NOTE;
        if (std::filesystem::is_regular_file(settings, error)) {
            notes.push_back(
                {std::string(PAK_SETTINGS_NOTE),
                 static_cast<std::size_t>(std::filesystem::file_size(settings, error))});
        }
    }
    std::ranges::sort(notes, {}, &PakNote::name);
    return notes;
}

std::size_t ControllerPak::FreeBytes() const {
    std::size_t pages = 0;
    for (const PakNote &note : Notes()) {
        pages += (note.bytes + PAK_PAGE_BYTES - 1) / PAK_PAGE_BYTES;
    }
    return pages < PAK_PAGES ? (PAK_PAGES - pages) * PAK_PAGE_BYTES : 0;
}

std::optional<std::string>
ExportPakImage(const ControllerPak &pak, const std::filesystem::path &file, PakRelease release) {
    const PakCodes &codes = CodesOf(release);
    std::vector<std::uint8_t> image(PAK_IMAGE_BYTES, 0);
    // The identity of a pak mupen64plus formats, with the checksums libultra checks.
    constexpr std::array<std::uint8_t, 28> IDENTITY{
        0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x1A, 0x5F, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x01, 0x00};
    unsigned sum = 0;
    for (std::size_t i = 0; i < IDENTITY.size(); i += 2) {
        sum += static_cast<unsigned>(IDENTITY[i] << 8 | IDENTITY[i + 1]);
    }
    for (std::size_t block : IDENTITY_BLOCKS) {
        std::ranges::copy(IDENTITY, image.begin() + static_cast<std::ptrdiff_t>(block));
        Put16(image, block + 28, static_cast<std::uint16_t>(sum));
        Put16(image, block + 30, static_cast<std::uint16_t>(0xFFF2 - sum));
    }
    for (std::size_t page = FIRST_DATA_PAGE; page < IMAGE_PAGES; ++page) {
        Put16(image, INODE_PAGE * PAK_PAGE_BYTES + page * 2, INODE_FREE_PAGE);
    }

    std::size_t next = FIRST_DATA_PAGE;
    std::size_t entry = 0;
    for (const PakNote &note : pak.Notes()) {
        auto name = EncodeName(note.name);
        if (!name) {
            return fmt::format("The note {} has a name a Controller Pak cannot write.", note.name);
        }
        auto bytes = pak.ReadNote(note.name);
        std::size_t pages = (note.bytes + PAK_PAGE_BYTES - 1) / PAK_PAGE_BYTES;
        if (!bytes || pages == 0 || entry == PAK_NOTES || next + pages > IMAGE_PAGES) {
            return fmt::format("The note {} does not fit on a Controller Pak.", note.name);
        }
        std::size_t at = NOTE_TABLE_PAGE * PAK_PAGE_BYTES + entry * NOTE_ENTRY_SIZE;
        Put16(image, at, static_cast<std::uint16_t>(codes.game >> 16));
        Put16(image, at + 2, static_cast<std::uint16_t>(codes.game));
        Put16(image, at + 4, codes.company);
        Put16(image, at + 6, static_cast<std::uint16_t>(next));
        image[at + 8] = NOTE_OCCUPIED;
        std::ranges::copy(*name, image.begin() + static_cast<std::ptrdiff_t>(at + 0x10));
        std::ranges::copy(*bytes,
                          image.begin() + static_cast<std::ptrdiff_t>(next * PAK_PAGE_BYTES));
        for (std::size_t i = 0; i < pages; ++i) {
            std::size_t page = next + i;
            Put16(image,
                  INODE_PAGE * PAK_PAGE_BYTES + page * 2,
                  i + 1 == pages ? INODE_LAST_PAGE : static_cast<std::uint16_t>(page + 1));
        }
        next += pages;
        ++entry;
    }
    Put16(image, INODE_PAGE * PAK_PAGE_BYTES, InodeChecksum(image, INODE_PAGE));
    std::copy_n(image.begin() + static_cast<std::ptrdiff_t>(INODE_PAGE * PAK_PAGE_BYTES),
                PAK_PAGE_BYTES,
                image.begin() + static_cast<std::ptrdiff_t>(INODE_BACKUP_PAGE * PAK_PAGE_BYTES));

    // The other paks of an image of four are retained.
    std::vector<std::uint8_t> written = ReadFile(file);
    if (written.size() == FOUR_PAK_IMAGE_BYTES) {
        std::ranges::copy(image, written.begin());
    } else {
        written = std::move(image);
    }
    std::ofstream stream(file, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char *>(written.data()),
                 static_cast<std::streamsize>(written.size()));
    if (!stream) {
        return fmt::format("{} cannot be written.", file.string());
    }
    SDL_Log("Wrote %zu notes to %s.", entry, file.string().c_str());
    return std::nullopt;
}

std::optional<std::string> ImportPakImage(const ControllerPak &pak,
                                          const std::filesystem::path &file) {
    std::vector<std::uint8_t> image = ReadFile(file);
    if (image.size() != PAK_IMAGE_BYTES && image.size() != FOUR_PAK_IMAGE_BYTES) {
        return fmt::format("{} is not the image of a Controller Pak.", file.string());
    }
    image.resize(PAK_IMAGE_BYTES);
    // The table of the pages is read from its copy when its checksum fails.
    std::size_t inodes = INODE_PAGE;
    if (Get16(image, INODE_PAGE * PAK_PAGE_BYTES) != InodeChecksum(image, INODE_PAGE)) {
        inodes = INODE_BACKUP_PAGE;
        if (Get16(image, inodes * PAK_PAGE_BYTES) != InodeChecksum(image, inodes)) {
            return fmt::format("{} has no valid table of pages.", file.string());
        }
    }
    int imported = 0;
    for (std::size_t entry = 0; entry < PAK_NOTES; ++entry) {
        std::size_t at = NOTE_TABLE_PAGE * PAK_PAGE_BYTES + entry * NOTE_ENTRY_SIZE;
        std::uint32_t game =
            static_cast<std::uint32_t>(Get16(image, at)) << 16 | Get16(image, at + 2);
        if (game == 0 && Get16(image, at + 4) == 0) {
            continue;
        }
        auto name = DecodeName(&image[at + 0x10]);
        std::uint16_t company = Get16(image, at + 4);
        bool ours = std::ranges::any_of(RELEASE_CODES, [&](const PakCodes &codes) {
            return codes.game == game && codes.company == company;
        });
        if (!ours || !name || image[at + 0x0C] != 0) {
            SDL_Log("Skipped note %zu of %s, a note of another game or release.",
                    entry,
                    file.string().c_str());
            continue;
        }
        // The pages of the note in the order of the table. A chain longer than the pak loops.
        std::vector<std::uint8_t> bytes;
        std::size_t page = Get16(image, at + 6);
        bool ended = false;
        for (std::size_t count = 0; count < IMAGE_PAGES && !ended; ++count) {
            if (page < FIRST_DATA_PAGE || page >= IMAGE_PAGES) {
                break;
            }
            auto start = image.begin() + static_cast<std::ptrdiff_t>(page * PAK_PAGE_BYTES);
            bytes.insert(bytes.end(), start, start + static_cast<std::ptrdiff_t>(PAK_PAGE_BYTES));
            std::uint16_t link = Get16(image, inodes * PAK_PAGE_BYTES + page * 2);
            ended = link == INODE_LAST_PAGE;
            page = link;
        }
        if (!ended) {
            SDL_Log("Skipped the note %s of %s, whose pages are not valid.",
                    name->c_str(),
                    file.string().c_str());
            continue;
        }
        if (pak.ReadNote(*name)) {
            SDL_Log("The note %s replaces the note of the same name.", name->c_str());
        }
        if (!pak.WriteNote(*name, bytes)) {
            return fmt::format("The note {} cannot be written.", *name);
        }
        SDL_Log("Imported the note %s of %zu bytes.", name->c_str(), bytes.size());
        ++imported;
    }
    SDL_Log("Imported %d notes from %s.", imported, file.string().c_str());
    return std::nullopt;
}

} // namespace coastline
