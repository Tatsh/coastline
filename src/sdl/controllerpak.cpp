#include "controllerpak.h"

#include <algorithm>
#include <fstream>
#include <iterator>

namespace coastline {

ControllerPak::ControllerPak(std::filesystem::path directory) : m_directory(std::move(directory)) {
}

std::optional<std::vector<std::uint8_t>> ControllerPak::ReadNote(const std::string &name) const {
    std::ifstream file(m_directory / name, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(file), {});
}

bool ControllerPak::WriteNote(const std::string &name, std::span<const std::uint8_t> bytes) const {
    std::error_code error;
    std::filesystem::create_directories(m_directory, error);
    // The note is written beside its place and moved there, and a failed write loses no note.
    auto written = m_directory / (name + ".new");
    {
        std::ofstream file(written, std::ios::binary | std::ios::trunc);
        file.write(reinterpret_cast<const char *>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()));
        if (!file) {
            return false;
        }
    }
    std::filesystem::rename(written, m_directory / name, error);
    return !error;
}

bool ControllerPak::DeleteNote(const std::string &name) const {
    std::error_code error;
    return std::filesystem::remove(m_directory / name, error);
}

std::vector<PakNote> ControllerPak::Notes() const {
    std::vector<PakNote> notes;
    std::error_code error;
    for (const auto &entry : std::filesystem::directory_iterator(m_directory, error)) {
        if (entry.is_regular_file(error) && entry.path().extension() != ".new") {
            notes.push_back({entry.path().filename().string(),
                             static_cast<std::size_t>(entry.file_size(error))});
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

} // namespace coastline
