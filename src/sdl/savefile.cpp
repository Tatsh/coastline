#include "savefile.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <fstream>
#include <limits>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>

#include "sha1.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace coastline {
namespace {

constexpr std::array<std::uint8_t, 8> SAVE_SIGNATURE{'C', 'o', 'a', 's', 't', 'S', 'V', '1'};
constexpr std::size_t SAVE_HEADER_BYTES = SAVE_SIGNATURE.size() + 8 + 40;

std::filesystem::path Parent(const std::filesystem::path &path) {
    return path.has_parent_path() ? path.parent_path() : std::filesystem::path(".");
}

std::filesystem::path Temporary(const std::filesystem::path &path) {
    static std::atomic<unsigned long long> sequence{};
    return Parent(path) /
           (".coastline-save-" +
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
            std::to_string(sequence.fetch_add(1)));
}

#ifdef _WIN32
class Handle {
public:
    HANDLE value = INVALID_HANDLE_VALUE;
    ~Handle() {
        if (value != INVALID_HANDLE_VALUE) {
            CloseHandle(value);
        }
    }
};
#else
class Descriptor {
public:
    int value = -1;
    ~Descriptor() {
        if (value >= 0) {
            close(value);
        }
    }
};
#endif

bool WriteTemporary(const std::filesystem::path &path,
                    std::span<const std::uint8_t> bytes,
                    bool &created) {
#ifdef _WIN32
    Handle file;
    file.value = CreateFileW(
        path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file.value == INVALID_HANDLE_VALUE) {
        return false;
    }
    created = true;
    while (!bytes.empty()) {
        const auto length = static_cast<DWORD>(
            std::min<std::size_t>(bytes.size(), std::numeric_limits<DWORD>::max()));
        DWORD written = 0;
        if (!WriteFile(file.value, bytes.data(), length, &written, nullptr) || written == 0) {
            return false;
        }
        bytes = bytes.subspan(written);
    }
    return FlushFileBuffers(file.value) != 0;
#else
    Descriptor file;
    file.value = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0666);
    if (file.value < 0) {
        return false;
    }
    created = true;
    while (!bytes.empty()) {
        const auto length = std::min<std::size_t>(bytes.size(), 1024 * 1024);
        const auto written = write(file.value, bytes.data(), length);
        if (written < 0 && errno == EINTR) {
            continue;
        }
        if (written <= 0) {
            return false;
        }
        bytes = bytes.subspan(static_cast<std::size_t>(written));
    }
    return fsync(file.value) == 0;
#endif
}

} // namespace

struct SaveFile::State {
    std::filesystem::path path;
    mutable std::mutex mutex;
#ifdef _WIN32
    Handle lock;
#else
    Descriptor lock;
#endif
};

std::optional<std::vector<std::uint8_t>> ReadSaveFile(const std::filesystem::path &path,
                                                      std::size_t limit) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error)) {
        return std::nullopt;
    }
#ifdef _WIN32
    Handle file;
    file.value = CreateFileW(path.c_str(),
                             GENERIC_READ,
                             FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                             nullptr,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             nullptr);
    LARGE_INTEGER size{};
    if (file.value == INVALID_HANDLE_VALUE || !GetFileSizeEx(file.value, &size) ||
        size.QuadPart < 0 || static_cast<unsigned long long>(size.QuadPart) > limit) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size.QuadPart));
    std::size_t position = 0;
    while (position < bytes.size()) {
        const auto length = static_cast<DWORD>(
            std::min<std::size_t>(bytes.size() - position, std::numeric_limits<DWORD>::max()));
        DWORD read = 0;
        if (!ReadFile(file.value, bytes.data() + position, length, &read, nullptr) || read == 0) {
            return std::nullopt;
        }
        position += read;
    }
    std::uint8_t extra = 0;
    DWORD read = 0;
    if (!ReadFile(file.value, &extra, 1, &read, nullptr) || read != 0) {
        return std::nullopt;
    }
#else
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    const auto size = file.tellg();
    if (!file || size < 0 || static_cast<unsigned long long>(size) > limit) {
        return std::nullopt;
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file || file.peek() != std::char_traits<char>::eof() || file.bad()) {
        return std::nullopt;
    }
#endif
    return bytes;
}

SaveFile::SaveFile(std::unique_ptr<State> state) : m_state(std::move(state)) {
}
SaveFile::~SaveFile() = default;
SaveFile::SaveFile(SaveFile &&) noexcept = default;
SaveFile &SaveFile::operator=(SaveFile &&) noexcept = default;

std::optional<SaveFile> SaveFile::Open(const std::filesystem::path &path) {
    if (path.filename().empty()) {
        return std::nullopt;
    }
    std::error_code error;
    std::filesystem::create_directories(Parent(path), error);
    if (error) {
        return std::nullopt;
    }
    auto state = std::make_unique<State>();
    state->path = std::filesystem::weakly_canonical(path, error);
    if (error) {
        return std::nullopt;
    }
    auto lockPath = state->path;
    lockPath += ".lock";
#ifdef _WIN32
    state->lock.value = CreateFileW(lockPath.c_str(),
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    nullptr,
                                    OPEN_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    nullptr);
    OVERLAPPED offset{};
    if (state->lock.value == INVALID_HANDLE_VALUE ||
        !LockFileEx(state->lock.value, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &offset)) {
        return std::nullopt;
    }
#else
    state->lock.value = open(lockPath.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0666);
    if (state->lock.value < 0) {
        return std::nullopt;
    }
    while (flock(state->lock.value, LOCK_EX) != 0) {
        if (errno != EINTR) {
            return std::nullopt;
        }
    }
#endif
    return SaveFile(std::move(state));
}

std::optional<std::vector<std::uint8_t>> SaveFile::Read(std::size_t limit) const {
    std::lock_guard lock(m_state->mutex);
    return ReadSaveFile(m_state->path, limit);
}

bool SaveFile::Write(std::span<const std::uint8_t> bytes) const {
    std::lock_guard lock(m_state->mutex);
    const auto temporary = Temporary(m_state->path);
    std::error_code error;
    bool created = false;
    bool written = WriteTemporary(temporary, bytes, created);
    if (written) {
#ifdef _WIN32
        written = MoveFileExW(temporary.c_str(),
                              m_state->path.c_str(),
                              MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        std::filesystem::rename(temporary, m_state->path, error);
        written = !error;
        if (written) {
            Descriptor parent;
            parent.value = open(Parent(m_state->path).c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
            written = parent.value >= 0 && fsync(parent.value) == 0;
        }
#endif
    }
    if (!written && created) {
        std::filesystem::remove(temporary, error);
    }
    return written;
}

bool SaveFile::Remove() const {
    std::lock_guard lock(m_state->mutex);
    std::error_code error;
    std::filesystem::remove(m_state->path, error);
    return !error;
}

bool WriteSaveFile(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
    auto save = SaveFile::Open(path);
    return save && save->Write(bytes);
}

bool WriteSaveFile(const std::filesystem::path &path, std::string_view text) {
    return WriteSaveFile(path, {reinterpret_cast<const std::uint8_t *>(text.data()), text.size()});
}

std::optional<std::vector<std::uint8_t>>
ReadVerifiedSaveFile(const std::filesystem::path &path,
                     std::size_t limit,
                     bool resetInvalid,
                     const SavePayloadValidator &validate) {
    if (limit > std::numeric_limits<std::size_t>::max() - SAVE_HEADER_BYTES) {
        return std::nullopt;
    }
    auto save = SaveFile::Open(path);
    if (!save) {
        return std::nullopt;
    }
    auto file = save->Read(limit + SAVE_HEADER_BYTES);
    if (!file) {
        std::error_code error;
        const auto size = std::filesystem::file_size(path, error);
        if (resetInvalid && !error && size > limit + SAVE_HEADER_BYTES) {
            save->Remove();
        }
        return std::nullopt;
    }
    bool valid =
        file->size() >= SAVE_HEADER_BYTES &&
        std::ranges::equal(SAVE_SIGNATURE,
                           std::span<const std::uint8_t>(*file).first(SAVE_SIGNATURE.size()));
    if (valid) {
        std::uint64_t length = 0;
        for (std::size_t index = 0; index < 8; ++index) {
            length |= static_cast<std::uint64_t>((*file)[SAVE_SIGNATURE.size() + index])
                      << (index * 8);
        }
        const auto payload = std::span<const std::uint8_t>(*file).subspan(SAVE_HEADER_BYTES);
        const std::string_view checksum(
            reinterpret_cast<const char *>(file->data() + SAVE_SIGNATURE.size() + 8), 40);
        valid = length == payload.size() && Tools::Common::sha1Hex(payload) == checksum &&
                (!validate || validate(payload));
    }
    if (!valid) {
        if (resetInvalid) {
            save->Remove();
        }
        return std::nullopt;
    }
    file->erase(file->begin(), file->begin() + static_cast<std::ptrdiff_t>(SAVE_HEADER_BYTES));
    return file;
}

bool WriteVerifiedSaveFile(const std::filesystem::path &path, std::span<const std::uint8_t> bytes) {
    if (bytes.size() > std::numeric_limits<std::size_t>::max() - SAVE_HEADER_BYTES) {
        return false;
    }
    std::vector<std::uint8_t> file(SAVE_SIGNATURE.begin(), SAVE_SIGNATURE.end());
    file.reserve(SAVE_HEADER_BYTES + bytes.size());
    for (std::size_t index = 0; index < 8; ++index) {
        file.push_back(
            static_cast<std::uint8_t>(static_cast<std::uint64_t>(bytes.size()) >> (index * 8)));
    }
    const auto checksum = Tools::Common::sha1Hex(bytes);
    file.insert(file.end(), checksum.begin(), checksum.end());
    file.insert(file.end(), bytes.begin(), bytes.end());
    return WriteSaveFile(path, file);
}

} // namespace coastline
