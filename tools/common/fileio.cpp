#include "fileio.h"

#include <format>
#include <fstream>
#include <iterator>

namespace Tools::Common {

namespace {

std::unexpected<Error> ioError(std::string_view action, const std::filesystem::path &path) {
    return std::unexpected(
        Error{ErrorCode::Io, std::format("Cannot {} {}.", action, path.string())});
}

std::expected<void, Error>
writeChars(const std::filesystem::path &path, const char *data, std::size_t size) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(data, static_cast<std::streamsize>(size));
    if (!file.flush()) {
        return ioError("write", path);
    }
    return {};
}

} // namespace

std::expected<std::vector<std::uint8_t>, Error> readFile(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return ioError("read", path);
    }
    std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(file),
                                   std::istreambuf_iterator<char>()};
    if (file.bad()) {
        return ioError("read", path);
    }
    return data;
}

std::expected<void, Error> writeFile(const std::filesystem::path &path,
                                     std::span<const std::uint8_t> data) {
    // The streams write char.
    return writeChars(path, reinterpret_cast<const char *>(data.data()), data.size());
}

std::expected<void, Error> writeFile(const std::filesystem::path &path, std::string_view text) {
    return writeChars(path, text.data(), text.size());
}

} // namespace Tools::Common
